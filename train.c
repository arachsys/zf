#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ZDICT_STATIC_LINKING_ONLY
#include <zdict.h>
#include "util.h"

static char *buffer;
static size_t cursor, size;

static size_t *lengths;
static size_t slots, count;

static void load(const char *path, size_t head, int mode) {
  int fd = open(path, O_RDONLY);

  if (fd < 0)
    err(1, "open %s", path);

  while (slots < count + 1) {
    slots += 1 << 18; /* aim never to reallocate */
    if (!(lengths = realloc(lengths, slots * sizeof(size_t))))
      err(1, "realloc");
  }

  while (size < cursor + head) {
    size += 1 << 24; /* grow by 16 MB at a time */
    if (!(buffer = realloc(buffer, size)))
      err(1, "realloc");
  }

  lengths[count] = getmsg(fd, buffer + cursor, head, mode);
  cursor += lengths[count++];
  close(fd);
}

static void train(char *out, size_t size, const char *buffer,
    const size_t *lengths, size_t count, int level, int k, int d, int f) {
  ZDICT_fastCover_params_t params = {
    .zParams = {
      .compressionLevel = level
    },
    .k = k,
    .d = d,
    .f = f
  };

  if (ZDICT_isError(ZDICT_trainFromBuffer_fastCover(out, size, buffer,
        lengths, count, params)))
    errx(1, "Failed to train dictionary");
}

static int usage(const char *progname) {
  fprintf(stderr, "\
Usage: %s [OPTION]... [FILE]... > DICTIONARY\n\
Train a dictionary from message files specified as arguments or as \n\
newline-separated filenames on stdin.\n\
\n\
Options:\n\
  -0    filenames on stdin are separated by zero bytes not newlines\n\
  -B    train a dictionary for message bodies, ignoring headers\n\
  -H    train a dictionary for message headers, ignoring bodies\n\
  -d #  set d size for the fast cover algorithm (default 6)\n\
  -k #  set k size for the fast cover algortihm (default 100)\n\
  -l #  set the target zstd compression level (default 12)\n\
  -n #  train on the first # bytes of each message (default 16384)\n\
  -s #  set the output dictionary size in bytes (default 1048576)\n\
", progname);
  return 64;
}

int main(int argc, char **argv) {
  int d = 6, k = 100, level = 12, mode = 0, option;
  char delim = '\n', *dict, *path, *tail;
  size_t head = 16384, size = 1 << 20;

  while ((option = getopt(argc, argv, ":0BHd:k:l:n:s:")) > 0)
    switch (option) {
      case '0':
        delim = 0;
        break;
      case 'B':
        mode |= 1;
        break;
      case 'H':
        mode |= 2;
        break;
      case 'd':
        d = strtoul(optarg, &tail, 10);
        if (*tail)
          return usage(argv[0]);
        break;
      case 'k':
        k = strtoul(optarg, &tail, 10);
        if (*tail)
          return usage(argv[0]);
        break;
      case 'l':
        level = strtoul(optarg, &tail, 10);
        if (*tail)
          return usage(argv[0]);
        break;
      case 'n':
        head = strtoul(optarg, &tail, 10);
        if (!strcasecmp(tail, "k"))
          head <<= 10;
        else if (!strcasecmp(tail, "M"))
          head <<= 20;
        else if (*tail)
          return usage(argv[0]);
        break;
      case 's':
        size = strtoul(optarg, &tail, 10);
        if (!strcasecmp(tail, "k"))
          size <<= 10;
        else if (!strcasecmp(tail, "M"))
          size <<= 20;
        else if (*tail)
          return usage(argv[0]);
        break;
      default:
        return usage(argv[0]);
    }

  if (size < 256)
    errx(1, "Dictionary cannot be smaller than 256 bytes");
  if (level > 255)
    errx(1, "Compression level cannot exceed 255");
  if (d != 6 && d != 8)
    errx(1, "Fast cover is only implemented for d = 6 or d = 8");
  if (k < d)
    errx(1, "Fast cover k cannot be less than d");
  if (k > size)
    errx(1, "Fast cover k cannot exceed dictionary size");
  if (isatty(1))
    return usage(argv[0]);

  if (optind == argc)
    while (readpath(&path, delim) >= 0)
      load(path, head, mode);
  while (optind < argc)
    load(argv[optind++], head, mode);

  if (count < 5)
    errx(1, "Training a dictionary requires at least five samples");

  if (!(dict = malloc(size)))
    err(1, "malloc");
  train(dict, size, buffer, lengths, count, level, k, d, 24);

  put(1, "ZF24", 4);
  put(1, (char[]) { head, head >> 8, head >> 16, head >> 24 }, 4);
  put(1, (char[]) { level, mode }, 2);
  put(1, dict, size);
  return 0;
}

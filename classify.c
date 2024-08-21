#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <zstd.h>
#include "util.h"

static size_t mapdict(char **out, const char *path) {
  int fd = open(path, O_RDONLY);
  struct stat status;

  if (fd < 0)
    err(1, "open %s", path);
  if (fstat(fd, &status) < 0)
    err(1, "fstat %s", path);

  *out = mmap(0, status.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (*out == MAP_FAILED)
    err(1, "mmap %s", path);

  close(fd);
  return status.st_size;
}

static uint32_t unpack(const char s[4]) {
  uint32_t result = (uint8_t) s[0];
  result += (uint8_t) s[1] << 8;
  result += (uint8_t) s[2] << 16;
  result += (uint8_t) s[3] << 24;
  return result;
}

static int usage(const char *progname) {
  fprintf(stderr, "\
Usage: %s DICTIONARY [FILE]...\n\
Report the size of each message specified as an argument or the message\n\
supplied on stdin when compressed with the given dictionary.\n\
", progname);
  return 64;
}

int main(int argc, char **argv) {
  int arg, fd, level, mode;
  char *dict, *in, *out;
  size_t head, size;
  ZSTD_CCtx *context;

  if (argc < 2)
    return usage(argv[0]);
  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

  size = mapdict(&dict, argv[1]);
  if (size < 10 || memcmp(dict, "ZF24", 4))
    errx(1, "Invalid classifier dictionary");

  head = unpack(dict + 4);
  level = (uint8_t) dict[8];
  mode = (uint8_t) dict[9];

  if (!(in = malloc(head)))
    err(1, "malloc");
  if (!(out = malloc(ZSTD_compressBound(head))))
    err(1, "malloc");
  if (!(context = ZSTD_createCCtx()))
    err(1, "malloc");

  if (ZSTD_isError(ZSTD_CCtx_setParameter(context,
        ZSTD_c_compressionLevel, level)))
    errx(1, "Failed to set compression level");
  if (ZSTD_isError(ZSTD_CCtx_loadDictionary(context, dict + 10, size - 10)))
    errx(1, "Failed to load dictionary");

  if (argc == 2)
    printf("%zu\n", ZSTD_compress2(context, out, ZSTD_compressBound(head),
      in, getmsg(0, in, head, mode)));
  for (arg = 2; arg < argc; arg++) {
    if ((fd = open(argv[arg], O_RDONLY)) < 0)
      err(1, "open %s", argv[arg]);
    printf("%zu\n", ZSTD_compress2(context, out, ZSTD_compressBound(head),
      in, getmsg(fd, in, head, mode)));
    close(fd);
  }
  return 0;
}

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline size_t get(int fd, char *data, size_t size) {
  size_t length = 0;
  ssize_t count;

  while (size > 0 && (count = read(fd, data, size))) {
    if (count < 0 && errno != EINTR && errno != EAGAIN)
      err(1, "read");
    if (count > 0)
      data += count, size -= count, length += count;
  }
  return length;
}

static inline size_t getmsg(int fd, char *data, size_t size, int mode) {
  size_t length = get(fd, data, size);
  char *separator;

  switch (mode) {
    case 1: /* body only */
      while (length > 1) {
        if ((separator = memmem(data, length, "\n\n", 2))) {
          length = data + length - separator - 2;
          memmove(data, separator + 2, length);
        } else if (data[length - 1] == '\n') {
          data[0] = '\n';
          length = 1;
        } else {
          length = 0;
        }
        length += get(fd, data + length, size - length);
        if (separator)
          return length;
      }
      return 0;
    case 2: /* header only */
      separator = memmem(data, length, "\n\n", 2);
      if (separator)
        length = 1 + separator - data;
  }
  return length;
}

static inline void put(int fd, const char *data, size_t length) {
  ssize_t count;

  while (length > 0) {
    count = write(fd, data, length);
    if (count < 0 && errno != EINTR && errno != EAGAIN)
      err(1, "write");
    if (count > 0)
      data += count, length -= count;
  }
}

static inline ssize_t readpath(char **out, int delim) {
  static size_t size;
  static char *line;
  ssize_t length;

  if (line == NULL) {
    size = PATH_MAX; /* aim never to reallocate */
    line = malloc(size);
  }
  if (line == NULL)
    err(1, "malloc");

  length = getdelim(&line, &size, delim, stdin);
  if (length < 0 && errno == ENOMEM)
    err(1, "realloc");
  while (length > 0 && line[length - 1] == delim)
    line[--length] = '\0';

  *out = line;
  return length;
}

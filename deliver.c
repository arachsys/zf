#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "util.h"

static const char *hostname(void) {
  static char *hostname;
  struct utsname utsname;
  struct addrinfo *info;

  const struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_CANONNAME
  };

  if (hostname == NULL && uname(&utsname) == 0) {
    if (!strchr(utsname.nodename, '.')
          && !getaddrinfo(utsname.nodename, NULL, &hints, &info)
          && info != NULL) {
      hostname = strdup(info->ai_canonname);
      freeaddrinfo(info);
    } else {
      hostname = strdup(utsname.nodename);
    }
  }
  return hostname ? hostname : "localhost";
}

int main(int argc, char **argv) {
  char buffer[1 << 20], name[256];
  struct timeval now;
  size_t length;
  int fd;

  if (argc > 2 || isatty(0)) {
    fprintf(stderr, "Usage: %s [TMPDIR] < MESSAGE\n", argv[0]);
    return 64;
  }

  if (argc > 1 && chdir(argv[1]) < 0)
    err(1, "chdir %s", argv[1]);
  if (gettimeofday(&now, NULL) < 0)
    err(1, "gettimeofday");
  snprintf(name, sizeof name, "%ld.M%06ldP%d.%s", now.tv_sec, now.tv_usec,
    getpid(), hostname());

  if ((fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0)
    err(1, "open %s", name);

  while ((length = get(0, buffer, sizeof(buffer))))
    put(fd, buffer, length);
  fsync(fd);
  close(fd);

  if (argc > 1)
    printf("%s/", argv[1]);
  printf("%s\n", name);
  return 0;
}

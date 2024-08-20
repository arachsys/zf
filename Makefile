BINDIR := $(PREFIX)/lib/zf
CFLAGS := -O2 -Wall

all: classify deliver train

classify: classify.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o classify classify.c -lzstd

deliver: deliver.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o deliver deliver.c

train: train.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o train train.c -lzstd

install: classify deliver train
	mkdir -p $(DESTDIR)$(BINDIR)
	install -s classify deliver train $(DESTDIR)$(BINDIR)

clean:
	rm -f classify deliver train

.PHONY: all install clean

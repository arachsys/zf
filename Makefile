BINDIR := $(PREFIX)/lib/zf
CFLAGS := -O2 -Wall
CC ?= cc
GO ?= go

all install: classify deliver train
all install: $(shell $(GO) version >/dev/null 2>&1 && echo header)

classify: classify.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o classify classify.c -lzstd

deliver: deliver.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o deliver deliver.c

header: header.go Makefile
	$(GO) build -tags netgo -o header header.go

train: train.c util.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o train train.c -lzstd

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	install -s $^ $(DESTDIR)$(BINDIR)

clean:
	rm -f classify deliver header train

.PHONY: all install clean

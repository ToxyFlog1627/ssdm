CC = gcc
CFLAGS = -O2 -std=c17 -Wall -Wextra -pedantic
LDFLAGS = -lncurses -ltinfo

ifeq ($(DEBUG),1)
	CFLAGS += -g3 -fsanitize={address,undefined,pointer-compare,pointer-subtract,leak} -fstack-protector
endif

.PHONY: all
all: pre-install ssdm

.PHONY: pre-install
pre-install:
	mkdir -p build

.PHONY: ssdm
ssdm: build/ui.o
	$(CC) $(CFLAGS) src/main.c $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: install
install: all
	install -m 755 ssdm /usr/bin/ssdm

.PHONY: clean
clean:
	rm -f ssdm build/*
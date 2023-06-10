CC = gcc
CFLAGS = -O2 -std=c17 -Wall -Wextra -pedantic
LDFLAGS = -lncurses -ltinfo -lpam

ifeq ($(DEBUG),1)
	CFLAGS += -g3 -fsanitize=undefined -fstack-protector
endif

.PHONY: all
all: pre-install ssdm

.PHONY: pre-install
pre-install:
	mkdir -p build

.PHONY: ssdm
ssdm: build/ui.o build/pam.o
	$(CC) $(CFLAGS) src/main.c $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: install
install: all
	install -m 755 ssdm /usr/bin/ssdm
	install -m 644 assets/pam.conf /etc/pam.d/ssdm

.PHONY: clean
clean:
	rm -f ssdm build/*
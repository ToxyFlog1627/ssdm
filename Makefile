CC = gcc
CFLAGS = -O2 -std=c17 -Wall -Wextra -pedantic
LDFLAGS = -lncurses -ltinfo -lpam

ifeq ($(DEBUG),1)
	CFLAGS += -g3 -fsanitize=undefined -fstack-protector -DDEBUG
else
	CFLAGS += -DNDEBUG
endif


.PHONY: all
all: init ssdm

.PHONY: ssdm
ssdm: build/ui.o build/pam.o build/config.o build/store.o
	$(CC) $(CFLAGS) src/main.c $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@


.PHONY: init
init:
	mkdir -p build

.PHONY: install
install: all
	install -m 755 ssdm /usr/bin/ssdm
	install -m 644 assets/pam.conf /etc/pam.d/ssdm
	install -m 644 assets/default.conf /etc/ssdm.conf

.PHONY: clean
clean:
	rm -f ssdm build/*
CC = gcc
FLAGS = -O2 -std=c17 -Wall -Wextra -pedantic
INSTALL_LOC = /usr/bin

ifeq ($(DEBUG),1)
	FLAGS += -g3 -fsanitize={address,undefined,pointer-compare,pointer-subtract,leak} -fstack-protector
endif

.PHONY: all
all: pre-install ssdm

.PHONY: pre-install
pre-install:
	mkdir -p build
	clean

.PHONY: ssdm
ssdm:
	$(CC) $(FLAGS) src/main.c $^ -o $@

build/%.o: src/%.c
	$(CC) $(FLAGS) -c $^ -o $@

.PHONY: install
install:
	install -m755 ssdm $(INSTALL_LOC)

.PHONY: clean
clean:
	rm -f ssdm build/*
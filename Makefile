CC = gcc
CFLAGS = -O2 -std=c17 -Wall -Wextra -pedantic -Wno-int-conversion -Wno-int-to-pointer-cast
LDFLAGS = -lncurses -ltinfo -lpam -lxcb
FILES = ui pam config store xorg login
OBJECTS = $(patsubst %, build/%.o, $(FILES))

ifeq ($(DEBUG),1)
	CFLAGS += -g3 -fstack-protector -DDEBUG
else
	CFLAGS += -s -DNDEBUG
endif


.PHONY: all
all: init ssdm

.PHONY: ssdm
ssdm: $(OBJECTS)
	$(CC) $(CFLAGS) src/main.c $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@


.PHONY: init
init:
	mkdir -p build

.PHONY: install
install: all installnoconf installconf

.PHONY: installnoconf
installnoconf: all
	install -m 755 ssdm /usr/bin/ssdm
	install -D -m 755 assets/xsetup.sh /usr/share/ssdm/xsetup.sh
	install -m 644 assets/pam.conf /etc/pam.d/ssdm
	install -m 644 assets/ssdm.service /usr/lib/systemd/system/ssdm.service

.PHONY: installconf
installconf: all
	install -m 644 assets/default.conf /etc/ssdm.conf

.PHONY: uninstall
uninstall:
	rm /usr/bin/ssdm /etc/pam.d/ssdm /usr/lib/systemd/system/ssdm.service

.PHONY: clean
clean:
	rm -f ssdm build/*
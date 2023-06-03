# Super Simple Display Manager

SSDM is a very simple display manager for linux. \
It is meant to be used only with DE/WMs that start from `.xinitrc`, e.g. [DWM](https://dwm.suckless.org/).

## Installation

Requirments: ncurses, gmake, gcc

1. Clone - `git clone https://github.com/ToxyFlog1627/ssdm.git`
2. Make - `sudo make all install clean` (sudo is needed for copying files to `bin` directory)
3. Enable - it depends on distribution and/or init system. \
   For OpenRC Gentoo edit `/etc/inittab`, go to `# TERMINALS` and choose tty on which you want it to open (usually 2nd). \
   Then append `-nl /usr/bin/ssdm` after `/sbin/agetty` and save.

## Contributions

Contributions and new features are welcome

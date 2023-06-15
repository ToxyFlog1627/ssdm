# Super Simple Display Manager

ssdm is a very simple display manager for linux \
It is meant to be used only with WMs that start from `.xinitrc`, e.g. [DWM](https://dwm.suckless.org/)

## Installation

Requirements: ncurses, gmake, gcc, system logger

1. Clone - `git clone https://github.com/ToxyFlog1627/ssdm.git`
2. Make - `sudo make all install clean` (sudo is needed for copying files to `bin` directory)
3. Enable - it depends on distribution and/or init system \
   For OpenRC Gentoo edit `/etc/inittab`, go to `# TERMINALS` and choose tty on which you want it to open (usually 2nd) \
   Then append `-nl /usr/bin/ssdm` after `/sbin/agetty` and save

## Configuration

After going through installation, ssdm can be configured by editing values in `/etc/ssdm.conf` \
Comments start with `#`

## Debugging

To make debug build run `make DEBUG=1` \
Logs are kept at `/var/log/auth.log` (unless location of syslog for `LOG_AUTH` is changed in config)

## Contributions

Contributions and new features are welcome

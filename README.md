# Super Simple Display Manager

![Preview](https://github-production-user-asset-6210df.s3.amazonaws.com/59206807/248037296-aafb5ecb-cc01-4b45-a8ef-97a27c747e42.png)

ssdm is a very simple display manager for linux, inspired by [ly](https://github.com/fairyglade/ly) \
It only works with DE/WMs that are started from `.xinitrc`, e.g. [DWM](https://dwm.suckless.org/) \
\
Tested on Gentoo with OpenRC and DWM

## Installation

Requirements: ncurses, gmake, gcc, system logger

1. Clone - `git clone https://github.com/ToxyFlog1627/ssdm.git`
2. Make - `sudo make clean install`
3. Enable - it depends on distribution and/or init system \
\
On Gentoo edit `/etc/inittab`, go to sections saying `# TERMINALS` and choose tty on which you want it to open (usually 2nd) \
Then append `-nl /usr/bin/ssdm` after `/sbin/agetty`

## Configuration

After going through installation, ssdm can be configured by editing values in `/etc/ssdm.conf`

## Debugging

To make debug build run `make DEBUG=1` \
Logs are kept at `/var/log/auth.log` (unless location of syslog for `LOG_AUTH` is changed in config)

## Contributions

Contributions and new features are welcome

# Super Simple Display Manager

![Preview](https://github-production-user-asset-6210df.s3.amazonaws.com/59206807/248037296-aafb5ecb-cc01-4b45-a8ef-97a27c747e42.png)

ssdm is a very simple display manager for linux, inspired by [ly](https://github.com/fairyglade/ly) \
It only works with DE/WMs which are started by `.xinitrc`, e.g. [DWM](https://dwm.suckless.org/)

## Installation

Requirements: ncurses, gmake, gcc, system logger, PAM

1. Clone - `git clone https://github.com/ToxyFlog1627/ssdm.git`
2. Make - `sudo make install` (or `installnoconf` not to overwrite config)
3. Enable - `sudo systemctl enable ssdm.service`

## Configuration

After installation, ssdm can be configured in `/etc/ssdm.conf` \
All of the option and their default values are present in default config

## Debugging

To make debug build run `make DEBUG=1`

## Contributions

Contributions are welcome
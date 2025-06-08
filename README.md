# padavan 4.4 for K2P 32+512 NO USB #

This project is based on original rt-n56u with latest mtk 4.4.198 kernel, which is fetch from D-LINK GPL code.

#### Extra functions / changes
- Adding user/chinadns-ng , and fix shadowsocks + chinadns-ng using local domain whitellist.
- AP Relay auto-daemon


#### SS/SSR
- Transparent proxy (iptables) wasn't cleaned completely, this issue is fixed.
- Adding DNSProxy , Local DNS integrated with SS/SSR 
- Dnsmasq optimization specially for SS/SSR 
- Resolve DNS pollution - Adding DNS i/p in china-route mode
- Fast-open option is enabled according to linux version
##### Enhancements in this repo

- commits has beed rewritten on top of [hanwckf/rt-n56u](https://github.com/hanwckf/rt-n56u) repo for better history tracking
- Optimized Makefiles and build scripts, added a toplevel Makefile
- Added ccache support, may save up to 50%+ build time
- Upgraded the toolchain and libc:
  - gcc 13.3.0
  - musl 1.2.5 / uClibc-ng 1.0.52
 - OpenWrt style package Makefile
 - Enabled kernel cgroups support
 - Fixed K2P led label names
 - Replaced udpxy with msd_lite
 - Replaced Web Console with ttyd
 - Upgraded libs and user packages
 - And a lot of package related fixes
 - ...

# Features

- Based on 4.4.198 Linux kernel
- Support MT7621 based devices
- Support MT7615D/MT7615N/MT7915D wireless chips
- Support raeth and mt7621 hwnat with legency driver
- Support qca shortcut-fe
- Support IPv6 NAT based on netfilter
- Support WireGuard integrated in kernel
- Support fullcone NAT (by Chion82)
- Support LED&GPIO control via sysfs

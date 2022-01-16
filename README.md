# ClamAV native win32 port - 0.103.5

Copyright (c) 2005-2022 Gianluigi Tiesi <sherpya@netfarm.it>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this software; if not, write to the
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

## Features

- [**clamav**] small footprint
- [**clamav**] native msvc win64 port, legacy mingw 32bit build for old systems
- [**clamav**] support for unc paths, exotic/long file names
- [**clamav**] nice icons ;)
- [**clamscan**] memory scanner (in memory loaded modules are scanned n-disk)
- [**llvm**] jit dll is optional, and since not supported on old os will be loaded dinamically
- [**freshclam**] native dns txt query on Win98/ME/NT4 / DnsAPI query on Win2k+
- [**clamdtop**] curses ui to connect to a running clamd service
- [**clamd/freshclam**] as Windows Service

## 3rdparty libraries and versions

- OpenSSL: 1.1.1k
- bzip2: 1.0.8
- curl: 7.77.0
- gnulib: old version but still fine for my needs
- json-c: 0.15-20200726
- libunicows: 1.1.2 (32bit builds only)
- libxml2: 2.9.12
- pcre2: 10.37
- PDCurses: master@a96c85d5
- Pthreads-w32: 2.9.1 + some fixes
- zlib: 1.2.11

## Getting sources

Clone repository using:

`git clone --recursive https://github.com/clamwin/clamav-win32.git`

## Notes

- You can store database and config paths in the registry using
  clamav.reg file (I may make a nsis installer in the future),
  paths can be also `REG_EXPAND_SZ`, environment variables are
  allowed here (i.e. you can use paths like `%HomeDrive%\ClamAV`).

- libclamunrar needs `unicows.dll` when used on Windows 9x, you can download
  [Open Layer For Unicode](https://oss.netfarm.it/clamav/files/opencow-0.7.7z)

- On WinNT4 you may need to set OPENSSL\_ia32cap environment variable to 0x16980010 because
  NT4 does not support SSE out of the box (it will crash with illegal instruction).

  Support for SSE is included in SP6a using Intel SSE driver (intlfxsr.sys),
  but the installer does not always install it.

  You can manually install the driver from unpacked SP6a:

  `rundll32 setupapi.dll,InstallHinfSection IntelSection 132 "<path-to-extracted-sp6a-files>\update\update.inf"`,

  then point it to path-to-extracted-sp6a-files and reboot.

  Thanks Zachary for these infos.

  If you get SSL Certificates problems when launching **freshclam** you need to install
  [Baltimore CyberTrust Root](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt)
  (double click on .crt file and follow the wizard)

## Running Clamd and FreshClam as services

- import `clamav.reg` file
- create `C:\ClamAV` and `C:\ClamAV\db`
- put executables in `C:\ClamAV`
- create freshclam.conf in `C:\ClamAV`
- create clamd.conf in `C:\ClamAV`
- install the service with (in Administrator cmd prompt): `clamd.exe --install`
- launch `freshclam.exe` to download the virus database

freshclam.conf

```text
DatabaseMirror database.clamav.net
DNSDatabaseInfo current.cvd.clamav.net
```

clamd.conf

```text
TCPSocket 3310
TCPAddr 127.0.0.1
MaxThreads 2
LogFile C:\ClamAV\clamd.log
DatabaseDirectory C:\ClamAV\db
```

Make sure `C:\Clamav` is writable by the `System Service` user.

Finally start the service with net start clamd

---

## Please dot report bugs directly to bugs@clamav.net, instead use github issue tracker <https://github.com/clamwin/clamav-win32/issues>

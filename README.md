# ClamAV native win32 port - 1.4.2-r1

Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>

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
- [**clamav**] native msvc win64 port, legacy mingw 32bit/64bit build for WinXP+
- [**clamav**] support for unc paths, exotic/long file names
- [**clamav**] nice icons ;)
- [**clamscan**] memory scanner (in memory loaded modules are scanned on-disk)
- [**llvm**] msvc build Only
- [**clamdtop**] curses ui to connect to a running clamd service
- [**clamd/freshclam**] as Windows Service

## 3rdparty libraries and versions

- OpenSSL: 3.4.1
- bzip2: 1.0.8
- curl: 8.12.1
- json-c: 0.18-20240915
- libxml2: 2.13.6
- pcre2: 10.45
- PDCurses: git-6ba6df38
- Winpthreads: git-b025331
- zlib-ng: 2.2.4
- LLVM 13.0.1

## Getting sources

Clone repository using:

`git clone --recursive https://github.com/clamwin/clamav-win32.git`

## Notes

- You can store database and config paths in the registry using
  clamav.reg file (I may make a nsis installer in the future).

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

## Please dot report bugs directly to <bugs@clamav.net>, instead use github issue tracker <https://github.com/clamwin/clamav-win32/issues>

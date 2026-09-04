# ClamAV native win32 port - 1.5.2

Copyright (c) 2005-2026 Gianluigi Tiesi <sherpya@gmail.com>

Upstream ClamAV Project:
Copyright (c) Cisco Systems, Inc. and/or its affiliates. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## Features

- [**clamav**] small footprint
- [**clamav**] modern MSVC 64bit port for Windows 7+, legacy 32bit down to NT4, legacy 64bit for WinXP+
- [**clamav**] support for UNC paths, exotic/long file names
- [**clamav**] nice icons ;)
- [**clamscan**] memory scanner (in memory loaded modules are scanned on-disk)
- [**clamdtop**] curses UI to connect to a running clamd service
- [**clamd/freshclam**] as Windows Service

## 3rdparty libraries and versions

- OpenSSL: 3.5.7
- bzip2: 1.0.8
- curl: 8.22.0
- json-c: 0.18-20240915
- libxml2: 2.15.2
- pcre2: 10.48
- PDCurses: git-6ba6df38
- Winpthreads: from MinGW-W64 v13.0.0
- zlib: 1.3.1
- zlib-ng: 2.3.3
- UnRAR: 7.2.7
- Opencow (custom build) on Win9x

## Getting sources

Clone repository using:

`git clone --recursive https://github.com/clamwin/clamav-win32.git`

## Notes

- You can store database and config paths in the registry using
  clamav.reg file (I may make a nsis installer in the future).

  If you get SSL Certificates problems when launching **freshclam** you need to install
  [Baltimore CyberTrust Root](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt)
  (double click on .crt file and follow the wizard)

  On legacy versions a crt bundle is used instead, if you get a message like:
  `ERROR: Failed to convert system certificate to x509.`
  it's probabily and old bogus cert you can safely remove from windows cert manager,
  on my system `Japan Certification Services, Inc. SecureSign RootCA1, RootCA2 and RootCA3`

## Mini FAQ

Q: freshclam: 403 or Forbidden; Blocked by CDN while trying to update

A: <https://docs.clamav.net/faq/faq-troubleshoot.html#how-do-i-know-if-my-ip-address-has-been-blocked>, often it's enough to delete freshclam.dat in the database directory

## Running Clamd and FreshClam as services

- import `clamav.reg` file
- create `C:\ClamAV` and `C:\ClamAV\db`
- create `C:\ClamAV\certs` and copy `clamav.crt` into it (optional)
- put executables in `C:\ClamAV`
- create freshclam.conf in `C:\ClamAV`
- create clamd.conf in `C:\ClamAV`
- install the service with (in Administrator cmd prompt): `clamd.exe --install`
- launch `freshclam.exe` to download the virus database

freshclam.conf

```text
DatabaseMirror database.clamav.net
DNSDatabaseInfo current.cvd.clamav.net
# Optional explicit cert path (recommended for portable/non-default layouts):
# CVDCertsDirectory "C:\\ClamAV\\certs"
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

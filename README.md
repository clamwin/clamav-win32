# ClamAV native win32 port

Copyright (c) 2005-2019 Gianluigi Tiesi <sherpya@netfarm.it>

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

* [**clamav**] small footprint (all distribution is around 1.5 MiB - except llvm fat ;) dll)
* [**clamav**] native win64 port
* [**clamav**] support for unc paths, exotic/long file names
* [**clamav**] nice icons ;)
* [**clamscan**] memory scanner (in memory loaded modules are scanned n-disk)
* [**llvm**] jit dll is optional, and since not supported on old os will be loaded dinamically
* [**freshclam**] native dns txt query on win98/ME / DnsAPI query on Win2k+
* [**clamd/freshclam**] also as Windows Services


## Getting sources

Clone repository using:

`git clone --recursive https://github.com/clamwin/clamav-win32.git`

## Notes

* You can store database and config paths in the registry using
  clamav.reg file (I may make a nsis installer in the future),
  paths can be also `REG_EXPAND_SZ`, environment variables are
  allowed here (i.e. you can use paths like `%HomeDrive%\ClamAV`).

* To use binaries compiled with Visual Studio 2005,
  you need the msvcrt80 side by side assembly, the "simple way" is
  uncompress Microsoft.VC80.CRT.zip in the directory of executables,
  the directory Microsoft.VC80.CRT must be placed as is, putting
  directly dlls in the same directory of the executable will not work.
  On windows 9x you should put the dll and the manifest in windows
  system directory without the `Microsoft.VC80.CRT` directory.
  For more info about this refer to the relative msdn-page.
  You can also download the redist package directly from Microsoft.

* libclamunrar needs `unicows.dll` when used on Windows 9x, it's bundled
  in the archive, you can safely remove it if not using Windows 9x

## Running Clamd and FreshClam as daemon

* import `clamav.reg` file
* create `C:\ClamAV` and `C:\ClamAV\db`
* put executables in `C:\ClamAV`
* create freshclam.conf in `C:\ClamAV`
* create clamd.conf in `C:\ClamAV`
* install the service with (in Administrator cmd prompt): `clamd.exe --install`
* launch `freshclam.exe` to download the virus database

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

## Please dot report bugs directly to bugs@clamav.net

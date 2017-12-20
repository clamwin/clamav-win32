top=..
clamav=../clamav
3rdparty=../3rdparty

CFLAGS=-I$(top)/include -I$(3rdparty)/gnulib -I$(3rdparty)/json-c -I$(3rdparty)/openssl/include
CFLAGS+=-I$(clamav) -I$(clamav)/shared -I$(clamav)/libclamav -I$(clamav)/libclamav/nsis
CFLAGS+=-I$(clamav)/win32/3rdparty/bzip2 -I$(clamav)/win32/3rdparty/pthreads
CFLAGS+=-I$(clamav)/win32/3rdparty/zlib -I$(clamav)/win32/3rdparty/pcre
CFLAGS+=-I$(clamav)/libclamav/libmspack-0.5alpha/mspack
CFLAGS+=-DCLAMWIN -DHAVE_CONFIG_H -DNDEBUG -DWIN32_LEAN_AND_MEAN -DPCRE_STATIC
CFLAGS+=-Wall -Wno-unused -Wno-format -Wno-uninitialized -Wno-attributes -Wno-switch
CFLAGS+=-pipe -fno-strict-aliasing -mno-ms-bitfields
CFLAGS+=-O2 -mtune=generic -fomit-frame-pointer

LDFLAGS=-Wl,--enable-stdcall-fixup

LLVM=-I$(clamav)/libclamav/c++ -I$(clamav)/libclamav/c++/llvm/include
LLVM+=-I$(clamav)/libclamav/c++/llvm/lib/Target/X86 -I$(top)/include/llvmbuild
LLVM+=-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS

CC=$(MINGW32_CROSS_PREFIX)gcc
CXX=$(MINGW32_CROSS_PREFIX)g++
WINDRES=$(MINGW32_CROSS_PREFIX)windres
DLLWRAP=$(MINGW32_CROSS_PREFIX)dllwrap
AR=$(MINGW32_CROSS_PREFIX)ar
RANLIB=$(MINGW32_CROSS_PREFIX)ranlib

ifneq (,$(findstring x86_64,$(shell $(CC) -dumpmachine)))
CFLAGS+=-D_TIMESPEC_DEFINED -D_TIMEZONE_DEFINED
else
CFLAGS+=-march=i686
endif

CLAMAV_PROGRAMS=clamd.exe clamdscan.exe clamscan.exe freshclam.exe sigtool.exe clambc.exe
CLAMAV_LIBS=libclamunrar.dll libclamunrar_iface.dll
CLAMAV_TOOLS=profiler.exe exeScanner.exe sigcheck.exe

%.o: %.c
	$(CC) $(CFLAGS) -Wno-pointer-sign $(DEFINES) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(DEFINES) $(LLVM) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(top)/resources -I$(top)/tools -o $@ $<

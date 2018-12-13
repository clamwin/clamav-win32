top=..
clamav=../clamav
3rdparty=../3rdparty

WARNINGS+=-Wall -Wno-unused -Wno-format -Wno-uninitialized -Wno-attributes -Wno-switch
OPTS=-O2 -mtune=generic -fomit-frame-pointer # -flto
COMMON_FLAGS=$(WARNINGS) $(OPTS) -pipe -mno-ms-bitfields -Wno-unknown-pragmas
COMMON_FLAGS+=-DCLAMWIN -DHAVE_CONFIG_H -DNDEBUG
COMMON_FLAGS+=-I$(top)/include -I$(clamav) -I$(clamav)/win32/3rdparty/pcre2
COMMON_FLAGS+=-I$(3rdparty)/json-c -I$(3rdparty)/openssl/include -I$(clamav)/win32/3rdparty/zlib

CFLAGS=$(COMMON_FLAGS) -I$(clamav)/libclamav
CFLAGS+=-I$(3rdparty)/gnulib
CFLAGS+=-I$(clamav)/shared -I$(clamav)/libclamav/nsis
CFLAGS+=-I$(clamav)/win32/3rdparty/bzip2 -I$(clamav)/win32/3rdparty/pthreads
CFLAGS+=-I$(clamav)/libclammspack/mspack
CFLAGS+=-DWIN32_LEAN_AND_MEAN -DPCRE2_STATIC -DPTW32_STATIC_LIB -D_WINDLL
CFLAGS+=-Wno-pointer-sign

CXXFLAGS=$(COMMON_FLAGS) -I$(clamav) -fpermissive -I$(clamav)/libclamav

#LDFLAGS=-flto

LLVM=-I$(clamav)/libclamav/c++ -I$(clamav)/libclamav/c++/llvm/include
LLVM+=-I$(clamav)/libclamav/c++/llvm/lib/Target/X86 -I$(top)/include/llvmbuild
LLVM+=-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS

CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++
WINDRES=$(CROSS_PREFIX)windres
DLLWRAP=$(CROSS_PREFIX)dllwrap
AR=$(CROSS_PREFIX)ar
RANLIB=$(CROSS_PREFIX)ranlib

ifneq (,$(findstring x86_64,$(shell $(CC) -dumpmachine)))
CFLAGS+=-D_TIMESPEC_DEFINED -D_TIMEZONE_DEFINED
else
CFLAGS+=-march=i686
endif

CLAMAV_PROGRAMS=clamd.exe clamdscan.exe clamscan.exe freshclam.exe sigtool.exe clambc.exe
CLAMAV_LIBS=libclamav.dll libclamunrar.dll libclamunrar_iface.dll
CLAMAV_TOOLS=exeScanner.exe sigcheck.exe

all: $(CLAMAV_LIBS) $(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS)

libclammspack_SOURCES=$(wildcard $(clamav)/libclammspack/mspack/*.c)
libclammspack_OBJECTS=$(libclammspack_SOURCES:.c=.o)
libclammspack.a: CFLAGS=-I$(top)/include -I$(clamav)/libclammspack/mspack -DHAVE_CONFIG_H
libclammspack.a: $(libclammspack_OBJECTS)
	$(AR) cru $@ $^

libclamunrar_SOURCES=$(top)/src/unrar/extinfo.cpp $(top)/src/unrar/system.cpp \
	$(addprefix $(clamav)/libclamunrar/,\
	archive.cpp arcread.cpp blake2s.cpp cmddata.cpp consio.cpp crc.cpp crypt.cpp dll.cpp encname.cpp \
	errhnd.cpp extract.cpp filcreat.cpp file.cpp filefn.cpp filestr.cpp find.cpp getbits.cpp global.cpp \
	hash.cpp headers.cpp isnt.cpp match.cpp options.cpp pathfn.cpp qopen.cpp rar.cpp rarpch.cpp rarvm.cpp \
	rawread.cpp rdwrfn.cpp rijndael.cpp scantree.cpp secpassword.cpp sha1.cpp sha256.cpp smallfn.cpp \
	strfn.cpp strlist.cpp threadpool.cpp timefn.cpp ui.cpp unicode.cpp unpack.cpp volume.cpp)
libclamunrar_OBJECTS=$(libclamunrar_SOURCES:.cpp=.o) $(top)/resources/libclamunrar-rc.o
libclamunrar.dll: CXXFLAGS+=-DRARDLL -DUNICODE -D_UNICODE
libclamunrar.dll: $(libclamunrar_OBJECTS)
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) \
		-Wl,--enable-stdcall-fixup \
		-static --def $(top)/libclamunrar.def --implib $@.a -o $@ $^ -L$(3rdparty)/libunicows -lunicows

libclamunrar_iface_SOURCES=$(clamav)/libclamunrar_iface/unrar_iface.cpp
libclamunrar_iface_OBJECTS=$(libclamunrar_iface_SOURCES:.cpp=.o) $(top)/resources/libclamunrar_iface-rc.o
libclamunrar_iface.dll: CXXFLAGS+=-DRARDLL
libclamunrar_iface.dll: $(libclamunrar_iface_OBJECTS) libclamunrar.dll
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) \
		-Wl,--enable-stdcall-fixup \
		-static --def $(top)/libclamunrar_iface.def --implib $@.a -o $@ $^ libclamunrar.dll.a

gnulib_SOURCES=$(wildcard $(3rdparty)/gnulib/*.c)
gnulib_OBJECTS=$(gnulib_SOURCES:.c=.o)

json_c_SOURCES=$(wildcard $(3rdparty)/json-c/*.c)
json_c_OBJECTS=$(json_c_SOURCES:.c=.o)

# Libclamav
libclamav_SOURCES=$(wildcard $(clamav)/libclamav/*.c)
libclamav_SOURCES+=$(foreach dir,7z lzw nsis regex,$(wildcard $(clamav)/libclamav/$(dir)/*.c))
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/tomsfastmath/*/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/win32/3rdparty/pcre2/pcre2_*.c)
libclamav_SOURCES+=$(clamav)/libclamav/jsparse/js-norm.c
libclamav_SOURCES+=$(top)/src/dllmain/pthread-mingw.c
libclamav_SOURCES+=$(addprefix $(clamav)/win32/3rdparty/bzip2/,blocksort.c bzlib.c compress.c \
	crctable.c decompress.c huffman.c randtable.c)
libclamav_SOURCES+=$(addprefix $(clamav)/win32/3rdparty/zlib/,adler32.c compress.c crc32.c \
	deflate.c gzlib.c gzread.c gzwrite.c gzclose.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)
libclamav_SOURCES+=$(wildcard $(top)/src/dllmain/*.c)

# replace others.c
libclamav_SOURCES:=$(subst $(clamav)/libclamav/others.c,$(top)/src/dllmain/win32others.c,$(libclamav_SOURCES))

# exclusions
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/regex/engine.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/bytecode_nojit.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/tomsfastmath/misc/fp_ident.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/win32/3rdparty/pcre2/pcre2_fuzzsupport.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/win32/3rdparty/pcre2/pcre2_jit_match.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/win32/3rdparty/pcre2/pcre2_jit_misc.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/win32/3rdparty/pcre2/pcre2_jit_test.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/win32/3rdparty/pcre2/pcre2_printint.c,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o) $(top)/resources/libclamav-rc.o
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS) $(json_c_OBJECTS) libclammspack.a
	$(DLLWRAP) $(LDFLAGS) \
		--def $(top)/libclamav.def \
		--implib $@.a -o $@ $^ \
		-L$(3rdparty)/openssl/lib/mingw32 -lssl -lcrypto -lws2_32 -lgdi32

libclamav.dll.a: libclamav.dll

shared_SOURCES+=$(wildcard $(clamav)/shared/*.c)
shared_SOURCES:=$(subst $(clamav)/shared/actions.c,$(top)/src/shared/win32actions.c,$(shared_SOURCES))
shared_SOURCES:=$(filter-out $(clamav)/shared/optparser.c,$(shared_SOURCES))
shared_OBJECTS=$(shared_SOURCES:.c=.o)

clamd_SOURCES=$(wildcard $(clamav)/clamd/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/service.c $(top)/src/helpers/win32poll.c
clamd_SOURCES:=$(filter-out $(clamav)/clamd/localserver.c,$(clamd_SOURCES))
clamd_SOURCES:=$(filter-out $(clamav)/clamd/dazukofs.c,$(clamd_SOURCES))
clamd_OBJECTS=$(clamd_SOURCES:.c=.o) $(top)/resources/clamd-rc.o
clamd.exe: libclamav.dll.a $(clamd_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamdscan_SOURCES=$(wildcard $(clamav)/clamdscan/*.c) \
	$(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c $(top)/src/helpers/dresult.c
clamdscan_OBJECTS=$(clamdscan_SOURCES:.c=.o) $(top)/resources/clamdscan-rc.o
clamdscan.exe: libclamav.dll.a $(clamdscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamscan_SOURCES=$(wildcard $(clamav)/clamscan/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/scanmem.c $(top)/src/helpers/exeScanner.c
clamscan_OBJECTS=$(clamscan_SOURCES:.c=.o) $(top)/resources/clamscan-rc.o
clamscan.exe: libclamav.dll.a $(clamscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

$(clamav)/freshclam/mirman.o: CFLAGS+=-fno-strict-aliasing
freshclam_SOURCES=$(wildcard $(clamav)/freshclam/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/service.c $(top)/src/helpers/dnsquery.c
freshclam_SOURCES:=$(filter-out $(clamav)/freshclam/dns.c,$(freshclam_SOURCES))
freshclam_OBJECTS=$(freshclam_SOURCES:.c=.o) $(top)/resources/freshclam-rc.o
freshclam.exe: libclamav.dll.a $(freshclam_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32 -liphlpapi

sigtool_SOURCES=$(wildcard $(clamav)/sigtool/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c
sigtool_OBJECTS=$(sigtool_SOURCES:.c=.o) $(top)/resources/sigtool-rc.o
sigtool.exe: libclamav.dll.a $(sigtool_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clambc_SOURCES=$(wildcard $(clamav)/clambc/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c
clambc_OBJECTS=$(clambc_SOURCES:.c=.o) $(top)/resources/clambc-rc.o
clambc.exe: libclamav.dll.a $(clambc_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

exeScanner_OBJECTS=$(top)/tools/exeScanner-rc.o
exeScanner.exe: $(exeScanner_OBJECTS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c
	$(CC) $(CFLAGS) -DEXESCANNER_STANDALONE $(LDFLAGS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c $(exeScanner_OBJECTS) -o $@

sigcheck_OBJECTS=$(top)/tools/sigcheck_app.o
sigcheck_OBJECTS+=$(top)/tools/sigcheck-rc.o
sigcheck.exe: libclamav.dll.a $(sigcheck_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(sigcheck_OBJECTS) -o $@ libclamav.dll.a

# LLVM
include llvm.mak
libclamav_llvm_OBJECTS=$(libclamav_llvm_SOURCES:.cpp=.o)
libclamav_llvm_OBJECTS:=$(libclamav_llvm_OBJECTS:.c=.o)
libclamav_llvm_OBJECTS+=$(top)/resources/libclamav_llvm-rc.o
libclamav_llvm.dll: $(libclamav_llvm_OBJECTS) libclamav.dll.a
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) -static --def $(top)/libclamav_llvm.def -o $@ $^ -limagehlp -lpsapi

llvm: libclamav_llvm.dll
llvm: CXXFLAGS+=-fno-omit-frame-pointer $(LLVM)

llvm-clean:
	@rm -f libclamav_llvm.dll $(libclamav_llvm_OBJECTS)
	@echo LLVM objects cleaned

clean:
	@rm -f libclamav.dll libclamav.dll.a
	@rm -f $(CLAMAV_LIBS) $(addsuffix .a,$(CLAMAV_LIBS))
	@rm -f $(gnulib_OBJECTS) $(json_c_OBJECTS) $(libclamunrar_OBJECTS) $(libclamunrar_iface_OBJECTS) $(libclamav_OBJECTS) $(libclammspack_OBJECTS) libclammspack.a
	@-rm -f *.exe $(clamd_OBJECTS) $(clamdscan_OBJECTS) $(clamscan_OBJECTS) $(freshclam_OBJECTS) $(sigtool_OBJECTS) $(clambc_OBJECTS) $(shared_OBJECTS)
	@-rm -f $(exeScanner_OBJECTS) $(sigcheck_OBJECTS)
	@echo Project cleaned

%.o: %.c
	$(CC) $(WARNINGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(WARNINGS) $(CXXFLAGS) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(top)/resources -I$(top)/tools -o $@ $<

.PHONY: all llvm clean llvm-clean

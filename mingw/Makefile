top=..
clamav=../clamav
3rdparty=../3rdparty

WARNINGS+=-Wall -Wno-unused -Wno-format -Wno-uninitialized -Wno-attributes -Wno-switch
OPTS=-O2 -mtune=generic -fomit-frame-pointer # -flto
COMMON_FLAGS=$(WARNINGS) $(OPTS) -pipe -mno-ms-bitfields -Wno-unknown-pragmas
COMMON_FLAGS+=-DCLAMWIN -DHAVE_CONFIG_H -DNDEBUG

COMMON_FLAGS+=\
	-I$(top)/include \
	-I$(clamav) \
	-I$(3rdparty) \
	-I$(3rdparty)/json-c \
	-I$(3rdparty)/gnulib \
	-I$(3rdparty)/pcre2/src \
	-I$(3rdparty)/libxml2/include \
	-I$(3rdparty)/openssl/include \

CFLAGS=$(COMMON_FLAGS) -I$(clamav)/libclamav
CFLAGS+=-I$(clamav)/shared -I$(clamav)/libclamav/nsis
CFLAGS+=-I$(clamav)/libclammspack/mspack
CFLAGS+=-I$(3rdparty)/bzip2 -I$(3rdparty)/pthreads
CFLAGS+=-DWIN32_LEAN_AND_MEAN
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
CLAMAV_LIBS=libclamav.dll libfreshclam.dll libclamunrar.dll libclamunrar_iface.dll
CLAMAV_TOOLS=exeScanner.exe sigcheck.exe
CLEAN_OBJECTS=

all: $(CLAMAV_LIBS) $(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS)

libclammspack_SOURCES=$(wildcard $(clamav)/libclammspack/mspack/*.c)
libclammspack_SOURCES:=$(filter-out $(clamav)/libclammspack/mspack/debug.c,$(libclammspack_SOURCES))
libclammspack_OBJECTS=$(libclammspack_SOURCES:.c=.o)
libclammspack.a: CFLAGS=-I$(top)/include -I$(clamav)/libclammspack/mspack -DHAVE_CONFIG_H
libclammspack.a: $(libclammspack_OBJECTS)
	$(AR) cru $@ $^
CLEAN_OBJECTS+=$(libclammspack_OBJECTS) libclammspack.a

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
CLEAN_OBJECTS+=$(libclamunrar_OBJECTS) libclamunrar.dll libclamunrar.dll.a

libclamunrar_iface_SOURCES=$(clamav)/libclamunrar_iface/unrar_iface.cpp
libclamunrar_iface_OBJECTS=$(libclamunrar_iface_SOURCES:.cpp=.o) $(top)/resources/libclamunrar_iface-rc.o
libclamunrar_iface.dll: CXXFLAGS+=-DRARDLL
libclamunrar_iface.dll: $(libclamunrar_iface_OBJECTS) libclamunrar.dll
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) \
		-Wl,--enable-stdcall-fixup \
		-static --def $(top)/libclamunrar_iface.def --implib $@.a -o $@ $^ libclamunrar.dll.a
CLEAN_OBJECTS+=$(libclamunrar_iface_OBJECTS) libclamunrar_iface.dll libclamunrar_iface.dll.a

gnulib_SOURCES=$(wildcard $(3rdparty)/gnulib/*.c)
gnulib_OBJECTS=$(gnulib_SOURCES:.c=.o)
CLEAN_OBJECTS+=$(gnulib_OBJECTS)

json_c_SOURCES=$(wildcard $(3rdparty)/json-c/*.c)
json_c_OBJECTS=$(json_c_SOURCES:.c=.o)
CLEAN_OBJECTS+=$(json_c_OBJECTS)

libxml2_SOURCES=SAX.c entities.c encoding.c error.c parserInternals.c  \
        parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c  \
        valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c  \
        xpointer.c xinclude.c nanohttp.c nanoftp.c \
        catalog.c globals.c threads.c c14n.c xmlstring.c buf.c \
        xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
        xmlreader.c relaxng.c dict.c SAX2.c \
        xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c \
        xmlmodule.c schematron.c xzlib.c
libxml2_SOURCES:=$(addprefix $(3rdparty)/libxml2/,$(libxml2_SOURCES))
libxml2_OBJECTS=$(libxml2_SOURCES:.c=.o)
CLEAN_OBJECTS+=$(libxml2_OBJECTS)

pcre2_SOURCES=pcre2_auto_possess.c pcre2_chartables.c pcre2_compile.c pcre2_context.c \
		pcre2_error.c pcre2_find_bracket.c pcre2_jit_compile.c pcre2_match.c pcre2_match_data.c \
		pcre2_newline.c pcre2_pattern_info.c pcre2_script_run.c pcre2_string_utils.c \
		pcre2_study.c pcre2_tables.c pcre2_ucd.c pcre2_valid_utf.c pcre2posix.c
pcre2_SOURCES:=$(addprefix $(3rdparty)/pcre2/src/,$(pcre2_SOURCES))
pcre2_OBJECTS=$(pcre2_SOURCES:.c=.o)
CLEAN_OBJECTS+=$(pcre2_OBJECTS)

# Libclamav
libclamav_SOURCES=$(wildcard $(clamav)/libclamav/*.c)
libclamav_SOURCES+=$(foreach dir,7z lzw nsis regex,$(wildcard $(clamav)/libclamav/$(dir)/*.c))
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/tomsfastmath/*/*.c)
libclamav_SOURCES+=$(clamav)/libclamav/jsparse/js-norm.c
libclamav_SOURCES+=$(top)/src/dllmain/pthread-mingw.c
libclamav_SOURCES+=$(addprefix $(3rdparty)/bzip2/,blocksort.c bzlib.c compress.c \
	crctable.c decompress.c huffman.c randtable.c)
libclamav_SOURCES+=$(addprefix $(3rdparty)/zlib/,adler32.c compress.c crc32.c \
	deflate.c gzlib.c gzread.c gzwrite.c gzclose.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)
libclamav_SOURCES+=$(wildcard $(top)/src/dllmain/*.c)

# replace others.c
libclamav_SOURCES:=$(subst $(clamav)/libclamav/others.c,$(top)/src/dllmain/win32others.c,$(libclamav_SOURCES))

# exclusions
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/regex/engine.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/bytecode_nojit.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(filter-out $(clamav)/libclamav/tomsfastmath/misc/fp_ident.c,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o) $(top)/resources/libclamav-rc.o
libclamav.dll: CFLAGS+=-DLIBCLAMAV_EXPORTS -DPCRE2_STATIC -DPTW32_STATIC_LIB -DLIBXML_STATIC
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS) $(json_c_OBJECTS) $(libxml2_OBJECTS) $(pcre2_OBJECTS) libclammspack.a
	$(DLLWRAP) $(LDFLAGS) \
		--def $(top)/libclamav.def \
		--implib $@.a -o $@ $^ \
		-L$(3rdparty)/openssl/lib/mingw32 -lssl -lcrypto -lws2_32 -lgdi32
libclamav.dll.a: libclamav.dll
CLEAN_OBJECTS+=$(libclamav_OBJECTS) libclamav.dll libclamav.dll.a

shared_SOURCES:=$(top)/src/shared/win32actions.c \
	$(clamav)/shared/misc.c $(clamav)/shared/output.c $(clamav)/shared/clamdcom.c $(clamav)/shared/cdiff.c $(clamav)/shared/tar.c
shared_OBJECTS=$(shared_SOURCES:.c=.o)
CLEAN_OBJECTS+=$(shared_OBJECTS)

clamd_SOURCES=$(wildcard $(clamav)/clamd/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/service.c $(top)/src/helpers/win32poll.c
clamd_SOURCES:=$(filter-out $(clamav)/clamd/localserver.c,$(clamd_SOURCES))
clamd_SOURCES+= $(clamav)/shared/idmef_logging.c
clamd_OBJECTS=$(clamd_SOURCES:.c=.o) $(top)/resources/clamd-rc.o
clamd.exe: CFLAGS+=-I$(3rdparty)/curl/include
clamd.exe: libclamav.dll.a $(clamd_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32
CLEAN_OBJECTS+=$(clamd_OBJECTS) clamd.exe

clamdscan_SOURCES=$(wildcard $(clamav)/clamdscan/*.c) \
	$(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c $(top)/src/helpers/dresult.c
clamdscan_SOURCES+=$(clamav)/shared/clamdcom.c
clamdscan_OBJECTS=$(clamdscan_SOURCES:.c=.o) $(top)/resources/clamdscan-rc.o
clamdscan.exe: libclamav.dll.a $(clamdscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32
CLEAN_OBJECTS+=$(clamdscan_OBJECTS) clamdscan.exe

clamscan_SOURCES=$(wildcard $(clamav)/clamscan/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/scanmem.c $(top)/src/helpers/exeScanner.c
clamscan_OBJECTS=$(clamscan_SOURCES:.c=.o) $(top)/resources/clamscan-rc.o
clamscan.exe: libclamav.dll.a $(clamscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32
CLEAN_OBJECTS+=$(clamscan_OBJECTS) clamscan.exe

libcurl_SOURCES=$(wildcard $(3rdparty)/curl/lib/*.c $(3rdparty)/curl/lib/*/*.c)
libcurl_OBJECTS=$(libcurl_SOURCES:.c=.o)
libcurl.a: CFLAGS=-I$(3rdparty)/curl/include -I$(3rdparty)/curl/lib -I$(3rdparty)/openssl/include
libcurl.a: CFLAGS+=-DBUILDING_LIBCURL -DCURL_STATICLIB -DCURL_DISABLE_LDAP -DUSE_OPENSSL -DHAVE_PTHREAD_H -DUSE_THREADS_POSIX -D_WIN32_WINNT=0x400
libcurl.a: $(libcurl_OBJECTS)
	$(AR) cru $@ $^
CLEAN_OBJECTS+=$(libcurl_OBJECTS) libcurl.a

libfreshclam_SOURCES=$(clamav)/libfreshclam/libfreshclam.c $(clamav)/libfreshclam/libfreshclam_internal.c $(top)/src/helpers/dnsquery.c
libfreshclam_SOURCES+=$(clamav)/shared/cert_util.c $(clamav)/shared/win/cert_util_win.c
libfreshclam_OBJECTS=$(libfreshclam_SOURCES:.c=.o)
libfreshclam.dll: CFLAGS+=-I$(3rdparty)/curl/include -DCURL_STATICLIB
libfreshclam.dll: $(libfreshclam_OBJECTS) $(shared_OBJECTS) libcurl.a libclamav.dll.a
	$(DLLWRAP) $(LDFLAGS) \
		--def $(top)/libfreshclam.def \
		--implib $@.a -o $@ $^ \
		-lws2_32 -liphlpapi -lcrypt32
libfreshclam.dll.a: libfreshclam.dll
CLEAN_OBJECTS+=$(libfreshclam_OBJECTS) libfreshclam.dll libfreshclam.dll.a

freshclam_SOURCES=$(wildcard $(clamav)/freshclam/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/service.c
freshclam_OBJECTS=$(freshclam_SOURCES:.c=.o) $(top)/resources/freshclam-rc.o
freshclam.exe: $(freshclam_OBJECTS) $(shared_OBJECTS) libfreshclam.dll.a libclamav.dll.a
	$(CC) $(LDFLAGS) $^ -o $@ -lws2_32
CLEAN_OBJECTS+=$(freshclam_OBJECTS) freshclam.exe

sigtool_SOURCES=$(wildcard $(clamav)/sigtool/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c
sigtool_OBJECTS=$(sigtool_SOURCES:.c=.o) $(top)/resources/sigtool-rc.o
sigtool.exe: $(sigtool_OBJECTS) $(shared_OBJECTS) libclamav.dll.a
	$(CC) $(LDFLAGS) $^ -o $@ -lws2_32
CLEAN_OBJECTS+=$(sigtool_OBJECTS) sigtool.exe

clambc_SOURCES=$(wildcard $(clamav)/clambc/*.c) $(top)/src/helpers/cw_main.c $(top)/src/helpers/cw_sch.c
clambc_OBJECTS=$(clambc_SOURCES:.c=.o) $(top)/resources/clambc-rc.o
clambc.exe: $(clambc_OBJECTS) $(shared_OBJECTS) libclamav.dll.a
	$(CC) $(LDFLAGS) $^ -o $@ -lws2_32
CLEAN_OBJECTS+=$(clambc_OBJECTS) clambc.exe

exeScanner_OBJECTS=$(top)/tools/exeScanner-rc.o
exeScanner.exe: $(exeScanner_OBJECTS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c
	$(CC) $(CFLAGS) -DEXESCANNER_STANDALONE $(LDFLAGS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c $(exeScanner_OBJECTS) -o $@
CLEAN_OBJECTS+=$(exeScanner_OBJECTS) exeScanner.exe

sigcheck_OBJECTS=$(top)/tools/sigcheck_app.o
sigcheck_OBJECTS+=$(top)/tools/sigcheck-rc.o
sigcheck.exe: $(sigcheck_OBJECTS) libclamav.dll.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
CLEAN_OBJECTS+=$(sigcheck_OBJECTS) sigcheck.exe

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
	@rm -f $(CLEAN_OBJECTS)
	@echo Project files cleaned

%.o: %.c
	$(CC) $(WARNINGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(WARNINGS) $(CXXFLAGS) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(top)/resources -I$(top)/tools -o $@ $<

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.PHONY: all llvm clean llvm-clean

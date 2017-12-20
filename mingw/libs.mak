include common.mak
CFLAGS+=-DPTW32_STATIC_LIB -D_WINDLL

gnulib_SOURCES=$(wildcard $(3rdparty)/gnulib/*.c)
gnulib_OBJECTS=$(gnulib_SOURCES:.c=.o)

json_c_SOURCES=$(wildcard $(3rdparty)/json-c/*.c)
json_c_OBJECTS=$(json_c_SOURCES:.c=.o)

libmspack_SOURCES=$(wildcard $(clamav)/libclamav/libmspack-0.5alpha/mspack/*.c)
libmspack_OBJECTS=$(libmspack_SOURCES:.c=.o)
libmspack.a: CFLAGS=-I$(3rdparty)/libmspack -I$(clamav)/libclamav/libmspack-0.5alpha/mspack -DHAVE_CONFIG_H
libmspack.a: $(libmspack_OBJECTS)
	$(AR) cru $@ $^

libclamunrar_SOURCES=$(wildcard $(clamav)/libclamunrar/*.c)
libclamunrar_OBJECTS=$(libclamunrar_SOURCES:.c=.o)
libclamunrar_OBJECTS+=$(top)/resources/libclamunrar-rc.o
libclamunrar.dll: $(libclamunrar_OBJECTS)
	$(DLLWRAP) $(LDFLAGS) --def $(top)/libclamunrar.def --implib $@.a -o $@ $^

libclamunrar_iface_SOURCES=$(wildcard $(clamav)/libclamunrar_iface/*.c)
libclamunrar_iface_OBJECTS=$(libclamunrar_iface_SOURCES:.c=.o)
libclamunrar_iface_OBJECTS+=$(top)/resources/libclamunrar_iface-rc.o
libclamunrar_iface.dll: $(libclamunrar_iface_OBJECTS) $(gnulib_OBJECTS) libclamunrar.dll
	$(DLLWRAP) $(LDFLAGS) --def $(top)/libclamunrar_iface.def --implib $@.a -o $@ $^ libclamunrar.dll.a

# Libclamav
libclamav_SOURCES=$(wildcard $(clamav)/libclamav/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/7z/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/lzw/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/lzma/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/nsis/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/regex/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/libclamav/tomsfastmath/*/*.c)
libclamav_SOURCES+=$(wildcard $(clamav)/win32/3rdparty/pcre/*.c)
libclamav_SOURCES+=$(clamav)/libclamav/jsparse/js-norm.c
libclamav_SOURCES+=$(clamav)/win32/3rdparty/pthreads/pthread.c
libclamav_SOURCES+=$(addprefix $(clamav)/win32/3rdparty/bzip2/,blocksort.c bzlib.c compress.c \
	crctable.c decompress.c huffman.c randtable.c)
libclamav_SOURCES+=$(addprefix $(clamav)/win32/3rdparty/zlib/,adler32.c compress.c crc32.c \
	deflate.c gzlib.c gzread.c gzwrite.c gzclose.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)
libclamav_SOURCES+=$(wildcard $(top)/src/dllmain/*.c)

# exclusions
libclamav_SOURCES:=$(subst $(clamav)/libclamav/regex/engine.c,,$(libclamav_SOURCES))
libclamav_SOURCES:=$(subst $(clamav)/libclamav/bytecode_nojit.c,,$(libclamav_SOURCES))
libclamav_SOURCES:=$(subst $(clamav)/libclamav/others.c,$(top)/src/dllmain/win32others.c,$(libclamav_SOURCES))
libclamav_SOURCES:=$(subst $(clamav)/libclamav/tomsfastmath/misc/fp_ident.c,,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o)
libclamav_OBJECTS+=$(top)/resources/libclamav-rc.o
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS) $(json_c_OBJECTS) libmspack.a
	$(DLLWRAP) $(LDFLAGS) --def $(top)/libclamav.def --implib $@.a -o $@ $^ -L$(openssl)/lib/mingw32 -lssl -lcrypto -lws2_32 -lgdi32

# LLVM
include llvm.mak
libclamav_llvm_OBJECTS=$(libclamav_llvm_SOURCES:.cpp=.o)
libclamav_llvm_OBJECTS:=$(libclamav_llvm_OBJECTS:.c=.o)
libclamav_llvm_OBJECTS+=$(top)/resources/libclamav_llvm-rc.o
libclamav_llvm.dll: $(libclamav_llvm_OBJECTS) libclamav.dll.a
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) -static --def $(top)/libclamav_llvm.def -o $@ $^ -limagehlp -lpsapi

llvm: libclamav_llvm.dll
llvm: CFLAGS+=-fno-omit-frame-pointer

llvm-clean:
	@rm -f libclamav_llvm.dll $(libclamav_llvm_OBJECTS)
	@echo LLVM objects cleaned

clean:
	@rm -f libclamav.dll libclamav.dll.a
	@rm -f $(CLAMAV_LIBS) $(addsuffix .a,$(CLAMAV_LIBS))
	@rm -f $(gnulib_OBJECTS) $(json_c_OBJECTS) $(libmspack_OBJECTS) libmspack.a $(libclamunrar_OBJECTS) $(libclamunrar_iface_OBJECTS) $(libclamav_OBJECTS)
	@echo Project cleaned

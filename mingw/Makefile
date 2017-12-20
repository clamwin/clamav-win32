include common.mak

all: libclamav.dll $(CLAMAV_LIBS) $(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS)

libclamav.dll $(CLAMAV_LIBS):
	$(MAKE) -f libs.mak $@

$(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS):
	$(MAKE) -f tools.mak $@

llvm:
	$(MAKE) -f libs.mak llvm

llvm-clean:
	$(MAKE) -f libs.mak llvm-clean

clean:
	$(MAKE) -f libs.mak clean
	$(MAKE) -f tools.mak clean

.PHONY: all llvm clean llvm-clean

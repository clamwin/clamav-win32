include common.mak

shared_SOURCES+=$(wildcard $(clamav)/shared/*.c)
shared_SOURCES:=$(subst $(clamav)/shared/actions.c,$(top)/src/shared/win32actions.c,$(shared_SOURCES))
shared_SOURCES:=$(subst $(clamav)/shared/optparser.c,,$(shared_SOURCES))
shared_OBJECTS=$(shared_SOURCES:.c=.o)

clamd_SOURCES=$(wildcard $(clamav)/clamd/*.c)
clamd_SOURCES+=$(top)/src/helpers/cw_main.c
clamd_SOURCES+=$(top)/src/helpers/service.c
clamd_SOURCES+=$(top)/src/helpers/win32poll.c
clamd_SOURCES:=$(subst $(clamav)/clamd/localserver.c,,$(clamd_SOURCES))
clamd_SOURCES:=$(subst $(clamav)/clamd/dazukofs.c,,$(clamd_SOURCES))
clamd_OBJECTS=$(clamd_SOURCES:.c=.o)
clamd_OBJECTS+=$(top)/resources/clamd-rc.o
clamd.exe: libclamav.dll $(clamd_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamdscan_SOURCES=$(wildcard $(clamav)/clamdscan/*.c)
clamdscan_SOURCES+=$(top)/src/helpers/cw_main.c
clamdscan_SOURCES+=$(top)/src/helpers/dresult.c
clamdscan_OBJECTS=$(clamdscan_SOURCES:.c=.o)
clamdscan_OBJECTS+=$(top)/resources/clamdscan-rc.o
clamdscan.exe: libclamav.dll $(clamdscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamscan_SOURCES=$(wildcard $(clamav)/clamscan/*.c)
clamscan_SOURCES+=$(top)/src/helpers/cw_main.c
clamscan_SOURCES+=$(top)/src/helpers/scanmem.c
clamscan_SOURCES+=$(top)/src/helpers/exeScanner.c
clamscan_OBJECTS=$(clamscan_SOURCES:.c=.o)
clamscan_OBJECTS+=$(top)/resources/clamscan-rc.o
clamscan.exe: libclamav.dll $(clamscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

freshclam_SOURCES=$(wildcard $(clamav)/freshclam/*.c)
freshclam_SOURCES+=$(top)/src/helpers/cw_main.c
freshclam_SOURCES+=$(top)/src/helpers/service.c
freshclam_SOURCES+=$(top)/src/helpers/dnsquery.c
freshclam_SOURCES:=$(subst $(clamav)/freshclam/dns.c,,$(freshclam_SOURCES))
freshclam_OBJECTS=$(freshclam_SOURCES:.c=.o)
freshclam_OBJECTS+=$(top)/resources/freshclam-rc.o
freshclam.exe: libclamav.dll $(freshclam_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32 -liphlpapi

sigtool_SOURCES=$(wildcard $(clamav)/sigtool/*.c)
sigtool_SOURCES+=$(top)/src/helpers/cw_main.c
sigtool_OBJECTS=$(sigtool_SOURCES:.c=.o)
sigtool_OBJECTS+=$(top)/resources/sigtool-rc.o
sigtool.exe: libclamav.dll $(sigtool_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clambc_SOURCES=$(wildcard $(clamav)/clambc/*.c)
clambc_SOURCES+=$(top)/src/helpers/cw_main.c
clambc_OBJECTS=$(clambc_SOURCES:.c=.o)
clambc_OBJECTS+=$(top)/resources/clambc-rc.o
clambc.exe: libclamav.dll $(clambc_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

profiler_SOURCES=$(top)/tools/profiler.c
profiler_OBJECTS=$(top)/tools/profiler.o
profiler_OBJECTS+=$(top)/tools/profiler-rc.o
profiler.exe: libclamav.dll $(profiler_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lwinmm

exeScanner_OBJECTS=$(top)/tools/exeScanner-rc.o
exeScanner.exe: $(exeScanner_OBJECTS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c
	$(CC) $(CFLAGS) -DEXESCANNER_STANDALONE $(LDFLAGS) $(top)/tools/exeScanner_app.c $(top)/src/helpers/exeScanner.c $(exeScanner_OBJECTS) -o $@

sigcheck_OBJECTS=$(top)/tools/sigcheck_app.o
sigcheck_OBJECTS+=$(top)/tools/sigcheck-rc.o
sigcheck.exe: $(sigcheck_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(sigcheck_OBJECTS) -o $@ libclamav.dll.a

clean:
	@-rm -f *.exe $(clamd_OBJECTS) $(clamdscan_OBJECTS) $(clamscan_OBJECTS) $(freshclam_OBJECTS) $(sigtool_OBJECTS) $(clambc_OBJECTS) $(shared_OBJECTS)
	@-rm -f $(profiler_OBJECTS) $(exeScanner_OBJECTS) $(sigcheck_OBJECTS)
	@echo Object files removed

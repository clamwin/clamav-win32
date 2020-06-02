
set(PDCURSES ${3RDPARTY_DIR}/PDCurses)
set(osdir ${PDCURSES}/wingui)

file(GLOB pdcurses_headers
    ${PDCURSES}/*.h
    ${PDCURSES}/common/*.h
    ${osdir}/*.h
)

file(GLOB pdcurses_sources ${PDCURSES}/pdcurses/*.c)

list(APPEND pdcurses_sources
    ${osdir}/pdcclip.c
    ${osdir}/pdcdisp.c
    ${osdir}/pdcgetsc.c
    ${osdir}/pdckbd.c
    ${osdir}/pdcscrn.c
    ${osdir}/pdcsetsc.c
    ${osdir}/pdcutil.c
)

add_library(PDCurses STATIC ${pdcurses_headers} ${pdcurses_sources})
target_include_directories(PDCurses PRIVATE ${PDCURSES})
if((MINGW) AND (CLAMAV_ARCH STREQUAL "x86"))
    # 0x0400 is _WIN32_WINNT_NT4
    target_compile_definitions(PDCurses PRIVATE WINVER=0x0400)
endif()

install(FILES ${PDCURSES}/README.md DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME PDCurses-README.md)

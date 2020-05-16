
set(PDCURSES ${3RDPARTY}/PDCurses)

file(GLOB pdcurses_src_files ${PDCURSES}/pdcurses/*.c)

set(osdir ${PDCURSES}/wingui)
set(pdc_src_files
    ${osdir}/pdcclip.c
    ${osdir}/pdcdisp.c
    ${osdir}/pdcgetsc.c
    ${osdir}/pdckbd.c
    ${osdir}/pdcscrn.c
    ${osdir}/pdcsetsc.c
    ${osdir}/pdcutil.c)

add_library(PDCurses STATIC ${pdc_src_files} ${pdcurses_src_files})
target_include_directories(PDCurses PRIVATE ${PDCURSES})
if((MINGW) AND (CLAMAV_ARCH STREQUAL "x86"))
    # 0x0400 is _WIN32_WINNT_NT4
    target_compile_definitions(PDCurses PRIVATE WINVER=0x0400)
endif()

install(FILES ${PDCURSES}/README.md DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME PDCurses-README.md)


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

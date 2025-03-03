
set(PDCURSES ${3RDPARTY_DIR}/PDCurses)
set(osdir ${PDCURSES}/wincon)

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
target_link_libraries(PDCurses winmm)

install(FILES ${PDCURSES}/README.md DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME PDCurses-README.md)

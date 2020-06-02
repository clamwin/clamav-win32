file(GLOB_RECURSE libxml2_headers ${CLAMWIN_DIR}/include/libxml2/*.h)

set(libxml2_sources
    SAX.c entities.c encoding.c error.c parserInternals.c
    parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c
    valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c
    xpointer.c xinclude.c nanohttp.c nanoftp.c
    catalog.c globals.c threads.c c14n.c xmlstring.c buf.c
    xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c
    xmlreader.c relaxng.c dict.c SAX2.c
    xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c
    xmlmodule.c schematron.c xzlib.c)
list(TRANSFORM libxml2_sources PREPEND ${3RDPARTY_DIR}/libxml2/)

add_library(libxml2 STATIC ${libxml2_headers} ${libxml2_sources})
set_target_properties(libxml2 PROPERTIES PREFIX "")
target_include_directories(libxml2 PRIVATE ${CLAMWIN_DIR}/include/libxml2 ${3RDPARTY_DIR}/libxml2/include)
target_compile_definitions(libxml2 PRIVATE LIBXML_STATIC)
target_compile_options(libxml2 PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4244 /wd4267>)

install(FILES ${3RDPARTY_DIR}/libxml2/Copyright DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.libxml2)

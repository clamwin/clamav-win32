file(GLOB pcre2_headers ${3RDPARTY_DIR}/pcre2/src/*.h ${CLAMWIN_DIR}/include/pcre2/*.h)

set(pcre2_sources
    pcre2_auto_possess.c pcre2_compile.c pcre2_context.c
    pcre2_error.c pcre2_find_bracket.c pcre2_jit_compile.c pcre2_match.c pcre2_match_data.c
    pcre2_newline.c pcre2_pattern_info.c pcre2_script_run.c pcre2_string_utils.c
    pcre2_study.c pcre2_tables.c pcre2_ucd.c pcre2_valid_utf.c pcre2posix.c)
list(TRANSFORM pcre2_sources PREPEND ${3RDPARTY_DIR}/pcre2/src/)

list(APPEND pcre2_sources ${CLAMWIN_DIR}/include/pcre2/pcre2_chartables.c)

add_library(pcre2 STATIC ${pcre2_headers} ${pcre2_sources})
set_target_properties(pcre2 PROPERTIES OUTPUT_NAME pcre2-8)
target_include_directories(pcre2 PRIVATE ${CLAMWIN_DIR}/include/pcre2 ${3RDPARTY_DIR}/pcre2/src)
target_compile_definitions(pcre2 PRIVATE PCRE2_STATIC HAVE_CONFIG_H PCRE2_CODE_UNIT_WIDTH=8)
target_compile_options(pcre2 PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4244>)

install(FILES ${3RDPARTY_DIR}/pcre2/LICENCE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.pcre2)

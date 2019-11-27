set(pcre2_srcs
    pcre2_auto_possess.c pcre2_chartables.c pcre2_compile.c pcre2_context.c
    pcre2_error.c pcre2_find_bracket.c pcre2_jit_compile.c pcre2_match.c pcre2_match_data.c
    pcre2_newline.c pcre2_pattern_info.c pcre2_script_run.c pcre2_string_utils.c
    pcre2_study.c pcre2_tables.c pcre2_ucd.c pcre2_valid_utf.c pcre2posix.c)
list(TRANSFORM pcre2_srcs PREPEND ${3RDPARTY}/pcre2/src/)

add_library(pcre2 STATIC ${pcre2_srcs})
target_compile_definitions(pcre2 PRIVATE PCRE2_STATIC HAVE_CONFIG_H)
set_target_properties(pcre2 PROPERTIES OUTPUT_NAME pcre2-8)

install(FILES ${3RDPARTY}/pcre2/LICENCE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.pcre2)

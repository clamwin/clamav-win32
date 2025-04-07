file(GLOB libclamav_common_win32_sources ${CLAMWIN_DIR}/src/common/*.c)

set_source_files_properties(${libclamav_common_win32_sources}
    PROPERTIES COMPILE_DEFINITIONS "${UNICODE_DEFINES}")

set(libclamav_common_sources
    ${CLAMAV_DIR}/common/clamdcom.c
    ${CLAMAV_DIR}/common/exeScanner.c
    ${CLAMAV_DIR}/common/getopt.c
    ${CLAMAV_DIR}/common/hostid.c
    ${CLAMAV_DIR}/common/idmef_logging.c
    ${CLAMAV_DIR}/common/misc.c
    ${CLAMAV_DIR}/common/optparser.c
    ${CLAMAV_DIR}/common/output.c
    ${CLAMAV_DIR}/common/tar.c
    ${CLAMAV_DIR}/win32/compat/glob.c
)

add_library(libclamav_common STATIC ${libclamav_common_sources} ${libclamav_common_win32_sources})
target_include_directories(libclamav_common PRIVATE ${CLAMWIN_INCLUDES} ${3RDPARTY_DIR}/curl/include)
target_compile_definitions(libclamav_common PRIVATE THIS_IS_LIBCLAMAV ${CLAMWIN_DEFINES})
set_target_properties(libclamav_common PROPERTIES PREFIX "")

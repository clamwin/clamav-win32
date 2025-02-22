file(GLOB libclamav_common_sources
    ${CLAMAV_DIR}/common/*.c
)

list(REMOVE_ITEM libclamav_common_sources
    ${CLAMAV_DIR}/common/actions.c
    ${CLAMAV_DIR}/common/cert_util.c
    ${CLAMAV_DIR}/common/optparser.c
)

add_library(libclamav_common STATIC ${libclamav_common_sources})
target_include_directories(libclamav_common PRIVATE ${CLAMWIN_INCLUDES} ${3RDPARTY_DIR}/curl/include)
target_compile_definitions(libclamav_common PRIVATE THIS_IS_LIBCLAMAV ${CLAMWIN_DEFINES})
set_target_properties(libclamav_common PROPERTIES PREFIX "")

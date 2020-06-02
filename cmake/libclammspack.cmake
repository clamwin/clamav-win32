file(GLOB clammspack_headers ${CLAMAV_DIR}/libclammspack/mspack/*.h)

file(GLOB clammspack_sources ${CLAMAV_DIR}/libclammspack/mspack/*.c)
list(REMOVE_ITEM clammspack_sources ${CLAMAV_DIR}/libclammspack/mspack/debug.c)

add_library(clammspack STATIC
    ${clammspack_headers}
    ${clammspack_sources}
)

target_include_directories(clammspack PRIVATE ${CLAMWIN_DIR}/include/libclammspack ${CLAMAV_DIR}/libclammspack/mspack)
target_compile_definitions(clammspack PRIVATE HAVE_CONFIG_H)
target_compile_options(clammspack PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4018 /wd4244 /wd4267>)

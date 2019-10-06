file(GLOB clammspack_srcs ${CLAMAV}/libclammspack/mspack/*.c)
list(REMOVE_ITEM clammspack_srcs ${CLAMAV}/libclammspack/mspack/debug.c)

add_library(clammspack STATIC ${clammspack_srcs})
target_include_directories(clammspack PRIVATE ${CLAMAV}/libclammspack/mspack)
target_compile_definitions(clammspack PRIVATE HAVE_CONFIG_H)

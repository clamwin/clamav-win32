include(cmake/llvm-sources.cmake)

list(TRANSFORM llvm_srcs PREPEND ${CLAMAV}/libclamav/c++/)
add_library(libclamav_llvm SHARED
    ${llvm_srcs}
    ${CMAKE_SOURCE_DIR}/resources/libclamav_llvm.rc
    ${CMAKE_SOURCE_DIR}/libclamav_llvm.def)

target_include_directories(libclamav_llvm PRIVATE
    ${CLAMAV}/libclamav
    ${CMAKE_SOURCE_DIR}/include/llvmbuild
    ${CLAMAV}/libclamav/c++
    ${CLAMAV}/libclamav/c++/llvm/include
    ${CLAMAV}/libclamav/c++/llvm/lib/Target/X86)
target_compile_definitions(libclamav_llvm PRIVATE __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS)
target_link_libraries(libclamav_llvm PRIVATE libclamav imagehlp psapi)
set_target_properties(libclamav_llvm PROPERTIES
    DEFINE_SYMBOL ""
    PREFIX ""
    OUTPUT_NAME libclamav_llvm)

file(GLOB_RECURSE llvm_headers ${CLAMWIN_DIR}/include/llvm/*.h)

include(cmake/llvm-sources.cmake)
list(TRANSFORM llvm_sources PREPEND ${CLAMAV_DIR}/libclamav/c++/)

if(CMAKE_CL_64)
    enable_language(ASM_MASM)
    set(X86CompilationCallback_Win64 ${CLAMAV_DIR}/libclamav/c++/llvm/lib/Target/X86/X86CompilationCallback_Win64.asm)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/X86CompilationCallback_Win64.obj
        COMMAND ${CMAKE_ASM_MASM_COMPILER} /Fo ${CMAKE_CURRENT_BINARY_DIR}/X86CompilationCallback_Win64.obj /c ${X86CompilationCallback_Win64}
        DEPENDS ${X86CompilationCallback_Win64})
    set(llvm_sources ${llvm_sources} ${CMAKE_CURRENT_BINARY_DIR}/X86CompilationCallback_Win64.obj)
endif()

add_library(libclamav_llvm SHARED
    ${llvm_headers}
    ${llvm_sources}
    ${CLAMWIN_DIR}/resources/libclamav_llvm.rc
    ${CLAMWIN_DIR}/libclamav_llvm.def
)

target_include_directories(libclamav_llvm PRIVATE
    ${CLAMWIN_DIR}/include/llvm
    ${CLAMAV_DIR}/libclamav/c++
    ${CLAMAV_DIR}/libclamav/c++/llvm/include
    ${CLAMAV_DIR}/libclamav/c++/llvm/lib/Target/X86
    ${CLAMWIN_INCLUDES}
)

target_compile_definitions(libclamav_llvm PRIVATE ${CLAMWIN_DEFINES} __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS)
target_link_libraries(libclamav_llvm PRIVATE libclamav imagehlp psapi)
set_target_properties(libclamav_llvm PROPERTIES DEFINE_SYMBOL "" PREFIX "" OUTPUT_NAME libclamav_llvm)

target_compile_options(libclamav_llvm PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4065 /wd4146 /wd4244 /wd4267 /wd4312 /wd4319 /wd4334 /wd4624 /wd4838>)

list(APPEND CLAMAV_INSTALL_TARGETS libclamav_llvm)

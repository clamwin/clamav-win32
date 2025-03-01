set(LLVM_DIR ${3RDPARTY_DIR}/llvm-project/llvm)
set(LLVM_ENABLE_THREADS 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE llvm_headers ${LLVM_DIR}/include/llvm/*.h)

file(GLOB llvm_sources
    ${LLVM_DIR}/lib/Analysis/*.cpp
    ${LLVM_DIR}/lib/AsmParser/*.cpp
    ${LLVM_DIR}/lib/Bitcode/Reader/*.cpp
    ${LLVM_DIR}/lib/BinaryFormat/*.cpp
    ${LLVM_DIR}/lib/CodeGen/*.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/*.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/*.cpp
    ${LLVM_DIR}/lib/CodeGen/MIRParser/*.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/*.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/*.cpp
    ${LLVM_DIR}/lib/Demangle/ItaniumDemangle.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/*.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/MCJIT/*.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/*.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/Targets/*.cpp
    ${LLVM_DIR}/lib/Linker/*.cpp
    ${LLVM_DIR}/lib/MC/*.cpp
    ${LLVM_DIR}/lib/MC/MCDisassembler/*.cpp
    ${LLVM_DIR}/lib/MC/MCParser/*.cpp
    ${LLVM_DIR}/lib/Object/*.cpp
    ${LLVM_DIR}/lib/IR/*.cpp
    ${LLVM_DIR}/lib/IRReader/*.cpp
    ${LLVM_DIR}/lib/ProfileData/*.cpp
    ${LLVM_DIR}/lib/Target/*.cpp
    ${LLVM_DIR}/lib/Target/X86/*.cpp
    ${LLVM_DIR}/lib/Target/X86/AsmParser/*.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/*.cpp
    ${LLVM_DIR}/lib/Target/X86/InstPrinter/*.cpp
    ${LLVM_DIR}/lib/Target/X86/TargetInfo/*.cpp
    ${LLVM_DIR}/lib/Target/X86/Utils/*.cpp
    ${LLVM_DIR}/lib/Transforms/AggressiveInstCombine/*.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/*.cpp
    ${LLVM_DIR}/lib/Transforms/Instrumentation/*.cpp
    ${LLVM_DIR}/lib/Transforms/IPO/*.cpp
    ${LLVM_DIR}/lib/Transforms/ObjCARC/*.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/*.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/*.cpp
    ${LLVM_DIR}/lib/Transforms/Vectorize/*.cpp
    ${LLVM_DIR}/lib/Support/*.cpp
    ${LLVM_DIR}/lib/Support/*.c
)

add_library(llvm STATIC
    ${llvm_headers}
    ${llvm_sources}
)
set_target_properties(llvm PROPERTIES LINKER_LANGUAGE CXX)

set(HAVE_ERRNO_H 1)
set(HAVE_FCNTL_H 1)
set(HAVE_LIBPSAPI 1)
if (MINGW)
  set(HAVE_LIBPTHREAD 1)
  set(HAVE_PTHREAD_GETNAME_NP 1)
  set(HAVE_PTHREAD_SETNAME_NP 1)
  set(HAVE_PTHREAD_GETSPECIFIC 1)
  set(HAVE_PTHREAD_H 1)
  set(HAVE_PTHREAD_MUTEX_LOCK 1)
  set(HAVE_PTHREAD_RWLOCK_INIT 1)
endif()
set(HAVE_SIGNAL_H 1)
set(HAVE_STRERROR 1)
set(HAVE_SYS_STAT_H 1)
set(HAVE_SYS_TYPES_H 1)
set(HAVE_UNISTD_H 1)

set(PACKAGE_NAME "LLVM")
set(PACKAGE_VERSION "8.0.1")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

configure_file(
  ${LLVM_DIR}/include/llvm/Config/config.h.cmake
  ${CMAKE_BINARY_DIR}/llvm/Config/config.h)
configure_file(
  ${LLVM_DIR}/include/llvm/Config/abi-breaking.h.cmake
  ${CMAKE_BINARY_DIR}/llvm/Config/abi-breaking.h)

target_include_directories(llvm PRIVATE
    ${CLAMWIN_DIR}/include
    ${CLAMWIN_DIR}/include/llvm/lib/IR
    ${CLAMWIN_DIR}/include/llvm/lib/Target/X86
    ${CLAMWIN_DIR}/include/llvm/lib/Transforms/InstCombine
    ${LLVM_DIR}/include
    ${LLVM_DIR}/lib/Target/X86
    ${CMAKE_BINARY_DIR}
)

if(WINXP)
  target_compile_definitions(llvm PRIVATE PSAPI_VERSION=1)
  set_source_files_properties(${LLVM_DIR}/lib/Support/Path.cpp PROPERTIES COMPILE_FLAGS "-include llvm-winxp.h")
  set_source_files_properties(${LLVM_DIR}/lib/Support/Signals.cpp PROPERTIES COMPILE_FLAGS "-include llvm-winxp.h")
 endif()

target_compile_options(llvm PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-Wno-deprecated-declarations -Wno-missing-template-keyword -Wno-init-list-lifetime>
    $<$<CXX_COMPILER_ID:MSVC>:/wd4141 /wd4146 /wd4244 /wd4267 /wd4291 /wd4319 /wd4624 /wd4805>
)

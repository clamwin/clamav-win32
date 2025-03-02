set(LLVM_DIR ${3RDPARTY_DIR}/llvm-project/llvm)

if(NOT DEFINED LLVM_VERSION_MAJOR)
  set(LLVM_VERSION_MAJOR 13)
endif()

if(NOT DEFINED LLVM_VERSION_MINOR)
  set(LLVM_VERSION_MINOR 0)
endif()

if(NOT DEFINED LLVM_VERSION_PATCH)
  set(LLVM_VERSION_PATCH 1)
endif()

if(NOT DEFINED LLVM_VERSION_SUFFIX)
  set(LLVM_VERSION_SUFFIX)
endif()

set(PACKAGE_NAME LLVM)
set(PACKAGE_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}${LLVM_VERSION_SUFFIX}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/clamwin/clamav-win32")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(LLVM_HOST_TRIPLE_PREFIX "x86_64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(LLVM_HOST_TRIPLE_PREFIX "i686")
else()
  message(FATAL_ERROR "WTF")
endif()

if(MINGW)
  set(LLVM_HOST_TRIPLE_SUFFIX "gnu")
elseif(MSVC)
  set(LLVM_HOST_TRIPLE_SUFFIX "msvc")
else()
  message(FATAL_ERROR "Unsupported compiler")
endif()

set(LLVM_ENABLE_THREADS YES)
set(LLVM_BUILD_STATIC YES)
set(LLVM_HAS_ATOMICS YES)

set(LLVM_NATIVE_ARCH X86)
set(LLVM_NATIVE_ASMPARSER LLVMInitialize${LLVM_NATIVE_ARCH}AsmParser)
set(LLVM_NATIVE_ASMPRINTER LLVMInitialize${LLVM_NATIVE_ARCH}AsmPrinter)
set(LLVM_NATIVE_DISASSEMBLER LLVMInitialize${LLVM_NATIVE_ARCH}Disassembler)
set(LLVM_NATIVE_TARGET LLVMInitialize${LLVM_NATIVE_ARCH}Target)
set(LLVM_NATIVE_TARGETINFO LLVMInitialize${LLVM_NATIVE_ARCH}TargetInfo)
set(LLVM_NATIVE_TARGETMC LLVMInitialize${LLVM_NATIVE_ARCH}TargetMC)

set(LLVM_ENUM_ASM_PARSERS "LLVM_ASM_PARSER(X86)")
set(LLVM_ENUM_ASM_PRINTERS "LLVM_ASM_PRINTER(X86)")
set(LLVM_ENUM_TARGETS "LLVM_TARGET(X86)")
set(LLVM_ENUM_DISASSEMBLERS "LLVM_DISASSEMBLER(X86)")

set(LLVM_HOST_TRIPLE "${LLVM_HOST_TRIPLE_PREFIX}-pc-windows-${LLVM_HOST_TRIPLE_SUFFIX}")
set(LLVM_DEFAULT_TARGET_TRIPLE "${LLVM_HOST_TRIPLE}")

set(LLVM_VERSION "${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR}")

if(MSVC)
  set(SHLIBEXT ".lib")
  set(stricmp "_stricmp")
  set(strdup "_strdup")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

file(GLOB_RECURSE llvm_headers ${LLVM_DIR}/include/llvm/*.h)

file(GLOB llvm_sources
  ${LLVM_DIR}/lib/Analysis/*.cpp
  ${LLVM_DIR}/lib/AsmParser/*.cpp
  ${LLVM_DIR}/lib/Bitcode/Reader/*.cpp
  ${LLVM_DIR}/lib/Bitstream/Reader/*.cpp
  ${LLVM_DIR}/lib/BinaryFormat/*.cpp
  ${LLVM_DIR}/lib/CodeGen/*.cpp
  ${LLVM_DIR}/lib/CodeGen/AsmPrinter/*.cpp
  ${LLVM_DIR}/lib/CodeGen/GlobalISel/*.cpp
  ${LLVM_DIR}/lib/CodeGen/LiveDebugValues/*.cpp
  ${LLVM_DIR}/lib/CodeGen/MIRParser/*.cpp
  ${LLVM_DIR}/lib/CodeGen/SelectionDAG/*.cpp
  ${LLVM_DIR}/lib/DebugInfo/CodeView/*.cpp
  ${LLVM_DIR}/lib/DebugInfo/DWARF/*.cpp
  ${LLVM_DIR}/lib/Demangle/*.cpp
  ${LLVM_DIR}/lib/ExecutionEngine/*.cpp
  ${LLVM_DIR}/lib/ExecutionEngine/MCJIT/*.cpp
  ${LLVM_DIR}/lib/ExecutionEngine/Orc/TargetProcess/*.cpp
  ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/*.cpp
  ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/Targets/*.cpp
  ${LLVM_DIR}/lib/Frontend/OpenMP/*.cpp
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
  ${LLVM_DIR}/lib/TextAPI/*.cpp
  ${LLVM_DIR}/lib/Transforms/AggressiveInstCombine/*.cpp
  ${LLVM_DIR}/lib/Transforms/CFGuard/*.cpp
  ${LLVM_DIR}/lib/Transforms/InstCombine/*.cpp
  ${LLVM_DIR}/lib/Transforms/Instrumentation/*.cpp
  ${LLVM_DIR}/lib/Transforms/IPO/*.cpp
  ${LLVM_DIR}/lib/Transforms/ObjCARC/*.cpp
  ${LLVM_DIR}/lib/Transforms/Scalar/*.cpp
  ${LLVM_DIR}/lib/Transforms/Utils/*.cpp
  ${LLVM_DIR}/lib/Transforms/Vectorize/*.cpp
  ${LLVM_DIR}/lib/Remarks/*.cpp
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
set(HAVE_FENV_H 1)

# set(HAVE_LIBPSAPI 1)
if(MINGW)
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

set(LLVM_MAIN_INCLUDE_DIR ${LLVM_DIR}/include)

# Produce the target definition files, which provide a way for clients to easily
# include various classes of targets.
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/AsmPrinters.def.in
  ${CMAKE_BINARY_DIR}/llvm/Config/AsmPrinters.def
)
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/AsmParsers.def.in
  ${CMAKE_BINARY_DIR}/llvm/Config/AsmParsers.def
)
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/Disassemblers.def.in
  ${CMAKE_BINARY_DIR}/llvm/Config/Disassemblers.def
)
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/Targets.def.in
  ${CMAKE_BINARY_DIR}/llvm/Config/Targets.def
)

# Configure the three LLVM configuration header files.
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/config.h.cmake
  ${CMAKE_BINARY_DIR}/llvm/Config/config.h)
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/llvm-config.h.cmake
  ${CMAKE_BINARY_DIR}/llvm/Config/llvm-config.h)
configure_file(
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Config/abi-breaking.h.cmake
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

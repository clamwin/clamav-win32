enable_language(C CXX ASM ASM_MASM)

file(GLOB libclamav_sources
    ${CLAMAV_DIR}/libclamav/*.c
    ${CLAMAV_DIR}/libclamav/7z/*.c
    ${CLAMAV_DIR}/libclamav/lzw/*.c
    ${CLAMAV_DIR}/libclamav/nsis/*.c
    ${CLAMAV_DIR}/libclamav/regex/*.c
    ${CLAMAV_DIR}/libclamav/tomsfastmath/*/*.c
    ${CLAMAV_DIR}/libclamav/jsparse/js-norm.c)
list(REMOVE_ITEM libclamav_sources
    ${CLAMAV_DIR}/libclamav/libclamav_main.c
    ${CLAMAV_DIR}/libclamav/regex/engine.c
    ${CLAMAV_DIR}/libclamav/tomsfastmath/misc/fp_ident.c)

if(ENABLE_LLVM)
    list(REMOVE_ITEM libclamav_sources ${CLAMAV_DIR}/libclamav/bytecode_nojit.c)
    list(APPEND libclamav_sources
        ${CLAMAV_DIR}/libclamav/c++/detect.cpp
        ${CLAMAV_DIR}/libclamav/c++/bytecode2llvm.cpp)
endif()

source_group(TREE ${CLAMAV_DIR}/libclamav PREFIX "Source Files" FILES ${libclamav_sources})

file(GLOB libclamav_win32_sources ${CLAMWIN_DIR}/src/dllmain/*.c)
set_source_files_properties(${libclamav_win32_sources}
    PROPERTIES COMPILE_DEFINITIONS "${UNICODE_DEFINES}"
)

list(APPEND libclamav_win32_sources
    ${CLAMAV_DIR}/win32/compat/libgen.c
    ${CLAMAV_DIR}/win32/compat/random.c
    ${CLAMAV_DIR}/win32/compat/strptime.c
    ${CLAMAV_DIR}/win32/compat/utf8_util.c
    ${CLAMAV_DIR}/win32/compat/w32_stat.c
)

if(MSVC)
    list(APPEND libclamav_win32_sources ${CLAMWIN_DIR}/src/dllmain/forward.asm)
else()
    list(APPEND libclamav_win32_sources ${CLAMWIN_DIR}/src/dllmain/forward.S)
endif()

if(CLAMWIN_WINDOWS_VERSION LESS_EQUAL 0x0501 AND CLAMAV_ARCH STREQUAL "x86")
    list(APPEND CLAMWIN_DEFINES C_WINDOWS)
endif()

file(GLOB_RECURSE libclamav_win32_headers ${CLAMWIN_DIR}/include/*.h)
source_group(TREE ${CLAMWIN_DIR}/include PREFIX "Win32 Headers" FILES ${libclamav_win32_headers})

list(APPEND libclamav_win32_sources ${CMAKE_BINARY_DIR}/libclamav.def ${CLAMWIN_DIR}/resources/libclamav.rc)
source_group("Win32 Sources" FILES ${libclamav_win32_sources})

if(WITH_WINPTHREADS)
    message(STATUS "Building Winpthreads")
    file(GLOB winpthreads_headers ${WINPTHREADS_DIR}/src/*.h)
    file(GLOB winpthreads_sources ${WINPTHREADS_DIR}/src/*.c)
    source_group("Winpthreads Files" FILES ${winpthreads_headers} ${winpthreads_sources})
    list(APPEND libclamav_win32_sources ${winpthreads_headers} ${winpthreads_sources})
    install(FILES ${WINPTHREADS_DIR}/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME winpthreads.txt)
endif()

add_library(libclamav SHARED
    ${libclamav_sources}
    ${libclamav_win32_headers}
    ${libclamav_win32_sources}
)

if(MSVC)
    target_link_options(libclamav PRIVATE
        "/DELAYLOAD:bcryptprimitives.dll"
        "/DELAYLOAD:api-ms-win-core-synch-l1-2-0.dll"
        "/DELAYLOAD:ws2_32.dll"
    )
    if(CLAMAV_ARCH STREQUAL "x86")
        target_link_options(libclamav PRIVATE "/SAFESEH:NO")
    endif()
endif()

add_library(ClamAV::libclamav ALIAS libclamav)

set_target_properties(libclamav PROPERTIES DEFINE_SYMBOL THIS_IS_LIBCLAMAV PREFIX "" OUTPUT_NAME libclamav)
target_include_directories(libclamav PRIVATE ${CLAMWIN_INCLUDES} ${CLAMAV_DIR}/win32/compat)
target_compile_definitions(libclamav PRIVATE ${CLAMWIN_DEFINES})
target_compile_options(libclamav PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-Wno-missing-template-keyword -Wno-init-list-lifetime>
)

target_link_libraries(libclamav PRIVATE
    ${CLAMWIN_LIBRARIES}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    crypt32
    wintrust
    ws2_32
    clamav_rust
)

if(ENABLE_LLVM)
    target_compile_definitions(libclamav PRIVATE LLVM_VERSION=${LLVM_VERSION})
    target_include_directories(libclamav PRIVATE ${LLVM_DIR}/include)
    target_link_libraries(libclamav PRIVATE llvm)
    target_compile_options(libclamav PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4141 /wd4146 /wd4244 /wd4291 /wd4624>)
endif()

if(MSVC)
    install(FILES ${CLAMAV_DIR}/libclamav/clamav.h DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${CLAMWIN_DIR}/include/clamav-types.h DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(TARGETS libclamav ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

list(APPEND CLAMAV_INSTALL_TARGETS libclamav)

install(FILES ${CLAMAV_DIR}/certs/clamav.crt DESTINATION ${CMAKE_INSTALL_PREFIX}/certs)
install(FILES ${CLAMAV_DIR}/COPYING.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME clamav.txt)

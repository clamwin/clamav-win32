enable_language(C CXX ASM)

if(MINGW)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY_DIR}/openssl/lib/mingw/${CLAMAV_ARCH})
elseif(MSVC)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY_DIR}/openssl/lib/msvc/${CLAMAV_ARCH})
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl libssl
    HINTS ${OPENSSL_LIBRARY_PATH}
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto libcrypto
    HINTS ${OPENSSL_LIBRARY_PATH}
)

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

if (ENABLE_LLVM)
    list(REMOVE_ITEM libclamav_sources ${CLAMAV_DIR}/libclamav/bytecode_nojit.c)
    list(APPEND libclamav_sources
        ${CLAMAV_DIR}/libclamav/c++/detect.cpp
        ${CLAMAV_DIR}/libclamav/c++/bytecode2llvm.cpp)
endif()

file(GLOB_RECURSE libclamav_win32_headers ${CLAMWIN_DIR}/include/*.h)

file(GLOB libclamav_win32_sources ${CLAMWIN_DIR}/src/dllmain/*.c)
list(APPEND libclamav_win32_sources
    ${CLAMAV_DIR}/win32/compat/dirent.c
    ${CLAMAV_DIR}/win32/compat/libgen.c
    ${CLAMAV_DIR}/win32/compat/random.c
    ${CLAMAV_DIR}/win32/compat/strptime.c
    ${CLAMAV_DIR}/win32/compat/utf8_util.c
    ${CLAMAV_DIR}/win32/compat/w32_stat.c
)

if (MINGW AND WINXP)
    list(APPEND libclamav_win32_sources ${CLAMWIN_DIR}/src/dllmain/dll_dependency.S)
endif()

list(APPEND libclamav_win32_sources ${CMAKE_BINARY_DIR}/libclamav.def)
source_group("Win32 Files" FILES ${libclamav_win32_sources})

if (MSVC)
    file(GLOB winpthreads_sources ${WINPTHREADS_DIR}/src/*.c)
    source_group("Winpthreads Files" FILES ${winpthreads_sources})
    list(APPEND libclamav_win32_sources ${winpthreads_sources})
    install(FILES ${WINPTHREADS_DIR}/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.winpthreads)
endif()

add_library(libclamav SHARED
    ${libclamav_win32_headers}
    ${libclamav_sources}
    ${libclamav_win32_sources}
    ${CLAMWIN_DIR}/resources/libclamav.rc
)

add_library(ClamAV::libclamav ALIAS libclamav)

set_target_properties(libclamav PROPERTIES DEFINE_SYMBOL THIS_IS_LIBCLAMAV PREFIX "" OUTPUT_NAME libclamav)
target_include_directories(libclamav PRIVATE ${CLAMWIN_INCLUDES} ${CLAMAV_DIR}/win32/compat)
target_compile_definitions(libclamav PRIVATE _WIN32_WINNT=0x0501 ${CLAMWIN_DEFINES})
target_compile_options(libclamav PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-Wno-missing-template-keyword -Wno-init-list-lifetime>
    $<$<C_COMPILER_ID:MSVC>:/wd4267 /wd4333 /wd4334>
)

target_link_libraries(libclamav PRIVATE
    zlib
    bzip2
    pcre2
    json-c
    libxml2
    clammspack
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    libclamav_common
    ws2_32
    psapi
    clamav_rust
)

if(ENABLE_LLVM)
    target_compile_definitions(libclamav PRIVATE LLVM_VERSION=80)
    target_include_directories(libclamav PRIVATE ${LLVM_DIR}/include)
    target_link_libraries(libclamav PRIVATE llvm)
endif()

if(MSVC)
    install(FILES ${CLAMAV_DIR}/libclamav/clamav.h DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${CLAMWIN_DIR}/include/clamav-types.h DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(TARGETS libclamav ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

list(APPEND CLAMAV_INSTALL_TARGETS libclamav)

install(FILES ${3RDPARTY_DIR}/openssl/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.openssl)

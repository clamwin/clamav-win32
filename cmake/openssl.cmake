if(MINGW)
    if(CLAMWIN_WINDOWS_VERSION LESS 0x0501)
        set(OPENSSL_ROOT_DIR ${3RDPARTY_DIR}/openssl/lib/mingw/legacy)
    else()
        set(OPENSSL_ROOT_DIR ${3RDPARTY_DIR}/openssl/lib/mingw/${CLAMAV_ARCH})
    endif()
elseif(MSVC)
    set(OPENSSL_ROOT_DIR ${3RDPARTY_DIR}/openssl/lib/msvc/${CLAMAV_ARCH})
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl libssl
    HINTS ${OPENSSL_ROOT_DIR}
    REQUIRED
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto libcrypto
    HINTS ${OPENSSL_ROOT_DIR}
    REQUIRED
)

set(OPENSSL_INCLUDE_DIR ${3RDPARTY_DIR}/openssl/include)
list(APPEND CLAMWIN_INCLUDES ${OPENSSL_INCLUDE_DIR})

install(FILES ${3RDPARTY_DIR}/openssl/LICENSE.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME openssl.txt)

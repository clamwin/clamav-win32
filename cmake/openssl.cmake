if(MINGW)
    set(OPENSSL_ROOT_DIR ${3RDPARTY_DIR}/openssl/lib/mingw/${CLAMAV_ARCH})
elseif(MSVC)
    set(OPENSSL_ROOT_DIR ${3RDPARTY_DIR}/openssl/lib/msvc/${CLAMAV_ARCH})
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl libssl
    HINTS ${OPENSSL_ROOT_DIR}
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto libcrypto
    HINTS ${OPENSSL_ROOT_DIR}
)

set(OPENSSL_INCLUDE_DIR ${3RDPARTY_DIR}/openssl/include)
list(APPEND CLAMWIN_INCLUDES ${OPENSSL_INCLUDE_DIR})

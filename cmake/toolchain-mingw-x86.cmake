SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR x86)

SET(COMPILER_PREFIX i686-w64-mingw32)

# which compilers to use for C and C++
SET(CMAKE_RC_COMPILER ${COMPILER_PREFIX}-windres)
SET(CMAKE_C_COMPILER ${COMPILER_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)
# MinGW-w64 toolchain file for cross-compiling Windows x86_64 on Linux
# Usage:
#   cmake -S . -B build-win64 \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
#     -DCMAKE_BUILD_TYPE=Release

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Root path of the MinGW sysroot
# On Arch, MinGW-w64 installs to /usr/x86_64-w64-mingw32
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Only search headers/libs/packages inside the cross root, not host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure Windows subsystem exe when needed (Qt handles this via WIN32 in target)
# set(CMAKE_WIN32_EXECUTABLE ON)

# Prefer static libstdc++ to avoid runtime deps where possible (tunable)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

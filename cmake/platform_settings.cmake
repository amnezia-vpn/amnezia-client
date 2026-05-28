if(APPLE)
    get_property(generator_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (generator_is_multi_config)
        set(CONAN_INSTALL_BUILD_CONFIGURATIONS Release Debug MinSizeRel RelWithDebInfo)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    elseif(MACOS_NE)
        set(_CONAN_INSTALL_ARGS "-o=&:macos_ne=True")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(_CONAN_INSTALL_ARGS
        "-c=tools.android:cmake_legacy_toolchain=false"
        "-c=tools.build:sharedlinkflags=['-Wl,-z,max-page-size=16384']"
        "-c=tools.build:exelinkflags=['-Wl,-z,max-page-size=16384']")
    set(CMAKE_ANDROID_STL_TYPE "c++_shared" CACHE STRING "")
endif()

if (WIN32 OR APPLE)
    set(CMAKE_INSTALL_BINDIR ".")
endif()

# Apple NE-based apps do not support any dylibs or variations
# So Qt would use the openssl bundled with system, not application
if (NOT(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR (APPLE AND MACOS_NE)))
    list(APPEND _CONAN_INSTALL_ARGS "-o=openssl/*:shared=True")
endif()

list(PREPEND _CONAN_INSTALL_ARGS "--build=missing")
list(JOIN _CONAN_INSTALL_ARGS ";" _CONAN_INSTALL_ARGS_JOINED)
set(CONAN_INSTALL_ARGS ${_CONAN_INSTALL_ARGS_JOINED} CACHE STRING "" FORCE)

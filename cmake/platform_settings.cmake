if(APPLE)
    get_property(generator_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (generator_is_multi_config)
        set(CONAN_INSTALL_BUILD_CONFIGURATIONS Release Debug MinSizeRel RelWithDebInfo)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    elseif(MACOS_NE)
        set(CONAN_INSTALL_ARGS "--build=missing;-o=&:macos_ne=True" CACHE STRING "" FORCE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(CONAN_INSTALL_ARGS
    "--build=missing"
    "-c=tools.android:cmake_legacy_toolchain=false"
    "-c=tools.build:sharedlinkflags=['-Wl,-z,max-page-size=16384']"
    "-c=tools.build:exelinkflags=['-Wl,-z,max-page-size=16384']"
    CACHE STRING "" FORCE)
    set(CMAKE_ANDROID_STL_TYPE "c++_shared" CACHE STRING "")
endif()

if (WIN32 OR APPLE)
    set(CMAKE_INSTALL_BINDIR ".")
endif()

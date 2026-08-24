# libagw — the gateway transport (envelope crypto, HTTP, proxy
# failover) used by GatewayController. Builds the SDK's C ABI (Go) into a
# static archive and exposes it as the imported target agw::agw.

if(TARGET agw::agw)
    return()
endif()

set(AGW_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rd/libagw")
if(NOT EXISTS "${AGW_SDK_DIR}/go.mod")
    message(FATAL_ERROR
        "libagw submodule is not initialised. Run:\n"
        "    git submodule update --init client/3rd/libagw")
endif()

find_program(AGW_GO_EXECUTABLE go REQUIRED)

set(_agw_out_dir "${CMAKE_BINARY_DIR}/agw")
set(AGW_LIBRARY "${_agw_out_dir}/libagw.a")

file(GLOB_RECURSE _agw_sources
    "${AGW_SDK_DIR}/go.mod"
    "${AGW_SDK_DIR}/gateway/*.go"
    "${AGW_SDK_DIR}/cabi/*.go"
    "${AGW_SDK_DIR}/cabi/*.c"
    "${AGW_SDK_DIR}/cabi/*.h"
    "${AGW_SDK_DIR}/archive/*.go")

# _agw_goarch(<cmake cpu> <out var>)
function(_agw_goarch cpu out_var)
    if(cpu MATCHES "^(arm64|aarch64|ARM64)$")
        set(${out_var} arm64 PARENT_SCOPE)
    elseif(cpu MATCHES "^(x86_64|amd64|AMD64)$")
        set(${out_var} amd64 PARENT_SCOPE)
    else()
        message(FATAL_ERROR "agw: unsupported target CPU '${cpu}'")
    endif()
endfunction()

# _agw_add_archive(<output> <goos> <goarch> [ENV <var=value>...])
function(_agw_add_archive output goos goarch)
    cmake_parse_arguments(ARG "" "" "ENV" ${ARGN})
    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_agw_out_dir}"
        COMMAND ${CMAKE_COMMAND} -E env
                CGO_ENABLED=1 GOOS=${goos} GOARCH=${goarch} ${ARG_ENV}
                "${AGW_GO_EXECUTABLE}" build -buildmode=c-archive "-ldflags=-s -w"
                -o "${output}" ./archive
        WORKING_DIRECTORY "${AGW_SDK_DIR}"
        DEPENDS ${_agw_sources}
        COMMENT "Building libagw c-archive (${goos}/${goarch})"
        VERBATIM)
endfunction()

if(IOS)
    _agw_add_archive("${AGW_LIBRARY}" ios arm64
        ENV "CC=${AGW_SDK_DIR}/scripts/ios_clang.sh")

elseif(ANDROID)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(_agw_arch arm64)
        set(_agw_cc_prefix aarch64-linux-android)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(_agw_arch arm)
        set(_agw_cc_prefix armv7a-linux-androideabi)
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(_agw_arch amd64)
        set(_agw_cc_prefix x86_64-linux-android)
    elseif(ANDROID_ABI STREQUAL "x86")
        set(_agw_arch 386)
        set(_agw_cc_prefix i686-linux-android)
    else()
        message(FATAL_ERROR "agw: unsupported ANDROID_ABI '${ANDROID_ABI}'")
    endif()
    _agw_add_archive("${AGW_LIBRARY}" android ${_agw_arch}
        ENV "CC=${ANDROID_TOOLCHAIN_ROOT}/bin/${_agw_cc_prefix}${ANDROID_PLATFORM_LEVEL}-clang")

elseif(APPLE)
    if(CMAKE_OSX_ARCHITECTURES)
        set(_agw_archs ${CMAKE_OSX_ARCHITECTURES})
    else()
        set(_agw_archs ${CMAKE_SYSTEM_PROCESSOR})
    endif()

    set(_agw_min_flag "")
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        set(_agw_min_flag " -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    set(_agw_slices "")
    foreach(_agw_cpu IN LISTS _agw_archs)
        _agw_goarch(${_agw_cpu} _agw_arch)
        set(_agw_slice "${_agw_out_dir}/libagw_${_agw_arch}.a")
        # cgo does not pass -arch itself: pin the C side to the slice's
        # architecture so cross builds (amd64 slice on an arm64 host) work.
        _agw_add_archive("${_agw_slice}" darwin ${_agw_arch}
            ENV "CGO_CFLAGS=-arch ${_agw_cpu}${_agw_min_flag}"
                "CGO_LDFLAGS=-arch ${_agw_cpu}${_agw_min_flag}")
        list(APPEND _agw_slices "${_agw_slice}")
    endforeach()

    list(LENGTH _agw_slices _agw_slice_count)
    if(_agw_slice_count EQUAL 1)
        set(AGW_LIBRARY "${_agw_slices}")
    else()
        add_custom_command(
            OUTPUT "${AGW_LIBRARY}"
            COMMAND xcrun lipo -create ${_agw_slices} -output "${AGW_LIBRARY}"
            DEPENDS ${_agw_slices}
            COMMENT "Merging libagw archives into a universal binary"
            VERBATIM)
    endif()

elseif(WIN32)
    _agw_goarch("${CMAKE_SYSTEM_PROCESSOR}" _agw_arch)
    _agw_add_archive("${AGW_LIBRARY}" windows ${_agw_arch})

else()
    _agw_goarch("${CMAKE_SYSTEM_PROCESSOR}" _agw_arch)
    _agw_add_archive("${AGW_LIBRARY}" linux ${_agw_arch})
endif()

add_custom_target(agw_build DEPENDS "${AGW_LIBRARY}")

add_library(agw_imported STATIC IMPORTED GLOBAL)
add_dependencies(agw_imported agw_build)
set_target_properties(agw_imported PROPERTIES
    IMPORTED_LOCATION "${AGW_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${AGW_SDK_DIR}/cabi")

if(APPLE)
    # Go's crypto/x509 reads the system trust store.
    set_property(TARGET agw_imported APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "-framework CoreFoundation" "-framework Security")
elseif(WIN32)
    set_property(TARGET agw_imported APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES ws2_32 winmm ntdll userenv bcrypt)
elseif(ANDROID)
    set_property(TARGET agw_imported APPEND PROPERTY INTERFACE_LINK_LIBRARIES log)
endif()

add_library(agw::agw ALIAS agw_imported)

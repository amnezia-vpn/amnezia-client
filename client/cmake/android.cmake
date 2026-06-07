message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")

set(APP_ANDROID_MIN_SDK 28)
set(ANDROID_PLATFORM "android-${APP_ANDROID_MIN_SDK}" CACHE STRING
    "The minimum API level supported by the application or library" FORCE)

# set QTP0002 policy: target properties that specify Android-specific paths may contain generator expressions
qt_policy(SET QTP0002 NEW)

set_target_properties(${PROJECT} PROPERTIES
    QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE ${APP_ANDROID_VERSION_CODE}
    QT_ANDROID_MIN_SDK_VERSION ${APP_ANDROID_MIN_SDK}
    QT_ANDROID_TARGET_SDK_VERSION 36
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION 36.0.0
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

set(QT_ANDROID_MULTI_ABI_FORWARD_VARS "QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL;CMAKE_BUILD_TYPE")

# We need to include qtprivate api's
# As QAndroidBinder is not yet implemented with a public api
# Check if Qt6::CorePrivate is available (may not be in all Qt versions/configurations)
if(TARGET Qt6::CorePrivate)
    set(LIBS ${LIBS} Qt6::CorePrivate)
endif()
set(LIBS ${LIBS} -ljnigraphics)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/platforms/android)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/protocols/androidVpnProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/utils/installedAppsImageProvider.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/protocols/androidVpnProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/utils/installedAppsImageProvider.cpp
)


find_package(awg-android REQUIRED)
set(LIBS ${LIBS} amnezia::awg-android)
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${AMNEZIA_ANDROID_LIBWG_PATH} ${AMNEZIA_ANDROID_LIBWG_QUICK_PATH})

set(_amnezia_android_shader_tools_lib "")
set(_amnezia_android_shader_tools_candidates)
if(DEFINED ENV{QT_ANDROID_SHADERTOOLS_LIB} AND NOT "$ENV{QT_ANDROID_SHADERTOOLS_LIB}" STREQUAL "")
    list(APPEND _amnezia_android_shader_tools_candidates "$ENV{QT_ANDROID_SHADERTOOLS_LIB}")
endif()
if(DEFINED ENV{QT_ROOT_PATH} AND NOT "$ENV{QT_ROOT_PATH}" STREQUAL "")
    list(APPEND _amnezia_android_shader_tools_candidates
        "$ENV{QT_ROOT_PATH}/android_${CMAKE_ANDROID_ARCH_ABI}/lib/libQt6ShaderTools_${CMAKE_ANDROID_ARCH_ABI}.so"
    )
endif()
if(DEFINED ENV{QT_INSTALL_DIR} AND NOT "$ENV{QT_INSTALL_DIR}" STREQUAL "")
    list(APPEND _amnezia_android_shader_tools_candidates
        "$ENV{QT_INSTALL_DIR}/6.10.1/android_${CMAKE_ANDROID_ARCH_ABI}/lib/libQt6ShaderTools_${CMAKE_ANDROID_ARCH_ABI}.so"
    )
endif()
if(DEFINED ENV{USER} AND NOT "$ENV{USER}" STREQUAL "")
    list(APPEND _amnezia_android_shader_tools_candidates
        "/mnt/c/Users/$ENV{USER}/Qt/6.10.1/android_${CMAKE_ANDROID_ARCH_ABI}/lib/libQt6ShaderTools_${CMAKE_ANDROID_ARCH_ABI}.so"
    )
endif()
foreach(_candidate IN LISTS _amnezia_android_shader_tools_candidates)
    file(TO_CMAKE_PATH "${_candidate}" _candidate_cmake)
    if(EXISTS "${_candidate_cmake}")
        set(_amnezia_android_shader_tools_lib "${_candidate_cmake}")
        break()
    endif()
endforeach()
if(_amnezia_android_shader_tools_lib STREQUAL "")
    message(FATAL_ERROR "Android Qt ShaderTools runtime library is required for Qt5Compat GraphicalEffects. Set QT_ANDROID_SHADERTOOLS_LIB to libQt6ShaderTools_${CMAKE_ANDROID_ARCH_ABI}.so.")
endif()
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS "${_amnezia_android_shader_tools_lib}")
message(STATUS "Bundling Android Qt ShaderTools runtime: ${_amnezia_android_shader_tools_lib}")

find_package(amnezia-libxray REQUIRED)
if(DEFINED ENV{HOME})
    set(_libxray_lock_dir "$ENV{HOME}/.cache/amnezia")
else()
    set(_libxray_lock_dir "${CMAKE_BINARY_DIR}/.android-locks")
endif()
file(MAKE_DIRECTORY "${_libxray_lock_dir}" "${CMAKE_CURRENT_SOURCE_DIR}/android/xray/libXray")
file(LOCK "${_libxray_lock_dir}/libxray-aar-copy.lock" GUARD FILE TIMEOUT 600)
configure_file(${AMNEZIA_LIBXRAY_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/android/xray/libXray/libxray.aar COPYONLY)

find_package(openvpn-pt-android REQUIRED)
set(LIBS ${LIBS} amnezia::openvpn-pt-android)
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${OPENVPN_PT_ANDROID_LIBCK_OVPN_PLUGIN_PATH})

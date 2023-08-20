message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")
# We need to include qtprivate api's
# As QAndroidBinder is not yet implemented with a public api
set(LIBS ${LIBS} Qt6::CorePrivate)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/platforms/android)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_notificationhandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/androidutils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/androidvpnactivity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/authResultReceiver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/android_vpnprotocol.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_notificationhandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/androidutils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/androidvpnactivity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/authResultReceiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/android_vpnprotocol.cpp
)

add_custom_command(
    TARGET ${PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle
    ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle/wrapper/gradle-wrapper.jar
    ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle/wrapper/gradle-wrapper.properties
    ${CMAKE_CURRENT_SOURCE_DIR}/android/gradlew
    ${CMAKE_CURRENT_SOURCE_DIR}/android/gradlew.bat
    ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle.properties
    ${CMAKE_CURRENT_SOURCE_DIR}/android/res/values/libs.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/android/res/xml/fileprovider.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/AuthHelper.java
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/IPCContract.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/NotificationUtil.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/OpenVPNThreadv3.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/Prefs.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/VPNLogger.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/VPNService.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/VPNServiceBinder.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/qt/AmneziaApp.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/qt/PackageManagerHelper.java
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/qt/VPNActivity.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/qt/VPNClientBinder.kt
    ${CMAKE_CURRENT_SOURCE_DIR}/android/src/org/amnezia/vpn/qt/VPNPermissionHelper.kt
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_property(TARGET ${PROJECT} PROPERTY
    QT_ANDROID_PACKAGE_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/android
)

foreach(abi IN ITEMS ${QT_ANDROID_ABIS})
    set_property(TARGET ${PROJECT} PROPERTY QT_ANDROID_EXTRA_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/android/${abi}/libwg.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/android/${abi}/libwg-go.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/android/${abi}/libwg-quick.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/shadowsocks/android/${abi}/libredsocks.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/shadowsocks/android/${abi}/libsslocal.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/shadowsocks/android/${abi}/libtun2socks.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libck-ovpn-plugin.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libovpn3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libovpnutil.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/librsapss.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/libssh/android/${abi}/libssh.so
    )
endforeach()

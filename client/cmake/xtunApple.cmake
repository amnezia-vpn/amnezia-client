set(XTUN_APPLE_VERSION 0.0.2-rc4)
set(XTUN_APPLE_SHA256 8d1c60932973a8e405b28d81fa2d7b361481871a4e7c0aae94dc3dafce1f6311)
set(XTUN_APPLE_DIR ${CMAKE_BINARY_DIR}/xtun-apple/${XTUN_APPLE_VERSION})
set(XTUN_APPLE_XCFRAMEWORK ${XTUN_APPLE_DIR}/Xtun.xcframework)

if(NOT EXISTS ${XTUN_APPLE_XCFRAMEWORK})
    file(DOWNLOAD
        https://github.com/amnezia-vpn/xtun-swift/releases/download/${XTUN_APPLE_VERSION}/Xtun.xcframework.zip
        ${XTUN_APPLE_DIR}/Xtun.xcframework.zip
        EXPECTED_HASH SHA256=${XTUN_APPLE_SHA256}
        STATUS XTUN_APPLE_DOWNLOAD_STATUS
    )
    list(GET XTUN_APPLE_DOWNLOAD_STATUS 0 XTUN_APPLE_DOWNLOAD_CODE)
    if(NOT XTUN_APPLE_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Xtun.xcframework: ${XTUN_APPLE_DOWNLOAD_STATUS}")
    endif()
    file(ARCHIVE_EXTRACT INPUT ${XTUN_APPLE_DIR}/Xtun.xcframework.zip DESTINATION ${XTUN_APPLE_DIR})
endif()

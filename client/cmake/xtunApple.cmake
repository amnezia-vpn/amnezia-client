set(XTUN_APPLE_VERSION 0.0.1)
set(XTUN_APPLE_SHA256 375f21371749a96ee13a02b5e830268388ff45d69349ce3fe6efeab90c405130)
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

message("MAC build")

set_target_properties(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.mm
)

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/app.icns)
set(MACOSX_BUNDLE_ICON_FILE app.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${ICON_FILE})



find_library(FW_SYSTEMCONFIG SystemConfiguration)
find_library(FW_SERVICEMGMT ServiceManagement)
find_library(FW_SECURITY Security)
find_library(FW_COREWLAN CoreWLAN)
find_library(FW_NETWORK Network)
find_library(FW_USER_NOTIFICATIONS UserNotifications)

target_link_libraries(${PROJECT} PRIVATE ${FW_SYSTEMCONFIG})
target_link_libraries(${PROJECT} PRIVATE ${FW_SERVICEMGMT})
target_link_libraries(${PROJECT} PRIVATE ${FW_SECURITY})
target_link_libraries(${PROJECT} PRIVATE ${FW_COREWLAN})
target_link_libraries(${PROJECT} PRIVATE ${FW_NETWORK})
target_link_libraries(${PROJECT} PRIVATE ${FW_USER_NOTIFICATIONS})

# Get SDK path
execute_process(
    COMMAND sh -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE OSX_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("OSX_SDK_PATH is: ${OSX_SDK_PATH}")

# Build the Wireguard Go tunnel
file(GLOB_RECURSE WIREGUARD_GO_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go/*.go)
set(WIREGUARD_GO_ENV
    GOCACHE=${CMAKE_BINARY_DIR}/go-cache
    CC=${CMAKE_C_COMPILER}
    CXX=${CMAKE_CXX_COMPILER}
    GOOS=darwin
    CGO_ENABLED=1
    GO111MODULE=on
    CGO_CFLAGS='-g -O3 -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -isysroot ${OSX_SDK_PATH}'
    CGO_LDFLAGS='-g -O3 -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -isysroot ${OSX_SDK_PATH}'
)

if(NOT CMAKE_OSX_ARCHITECTURES)
    foreach(OSXARCH ${CMAKE_OSX_ARCHITECTURES})
        message("Build wg for OSXARCH: ${OSXARCH}")

        string(REPLACE "x86_64" "amd64" GOARCH ${OSXARCH})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-${OSXARCH}
            COMMENT "Building wireguard-go for ${OSXARCH}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go
            DEPENDS
                ${WIREGUARD_GO_DEPS}
                ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go/go.mod
                ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go/go.sum
            COMMAND ${CMAKE_COMMAND} -E env ${WIREGUARD_GO_ENV} GOARCH=${GOARCH}
                    ${GOLANG_BUILD_TOOL} build -buildmode exe -trimpath -v
                        -o ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-${OSXARCH}
        )
        list(APPEND WG_GO_ARCH_BUILDS ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-${OSXARCH}/wireguard)
    endforeach()

    add_custom_target(build_wireguard_go
        COMMENT "Building wireguard-go"
        DEPENDS ${WG_GO_ARCH_BUILDS}
        COMMAND lipo -create -output ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go ${WG_GO_ARCH_BUILDS}
    )
else()
    # This only builds for the host architecture.
    add_custom_target(build_wireguard_go
        COMMENT "Building wireguard-go"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go
        DEPENDS
            ${WIREGUARD_GO_DEPS}
            ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go/go.mod
            ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-go/go.sum
        COMMAND ${CMAKE_COMMAND} -E env ${WIREGUARD_GO_ENV}
                ${GOLANG_BUILD_TOOL} build -buildmode exe -trimpath -v
                    -o ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go
    )
endif()
add_dependencies(${PROJECT} build_wireguard_go)
osx_bundle_files(${PROJECT}
    FILES ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go
    DESTINATION MacOS/
)

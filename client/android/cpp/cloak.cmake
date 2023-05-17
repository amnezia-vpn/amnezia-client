cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(TARGET ck_ovpn_plugin_go)

set(CLOAK_SRCS cloak/cmd/ck-ovpn-plugin/ck-ovpn-plugin.go)
set(CLOAK_LIB libck-ovpn-plugin.so)

list(APPEND CMAKE_PROGRAM_PATH "/usr/local/go/bin")
find_program(GO_EXEC go)

set(BUILD_CMD_ARGS build)
list(APPEND BUILD_CMD_ARGS -buildmode=c-shared -o ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB} ${CMAKE_GO_FLAGS} ./...)

set(PREPARE_ENV_ARGS env)
list(APPEND PREPARE_ENV_ARGS -w CGO_ENABLED=1 GOOS=android)


string(REGEX MATCH "[0-9]+$" ANDROID_API_LEVEL ${ANDROID_PLATFORM})
message(WARNING "build cloak plugin abi=${ANDROID_ABI}, ANDROID_API_LEVEL=${ANDROID_API_LEVEL}")

if ("${ANDROID_ABI}" STREQUAL "x86")
	list(APPEND PREPARE_ENV_ARGS GOARCH=386)
    list(APPEND PREPARE_ENV_ARGS CC=${ANDROID_TOOLCHAIN_ROOT}/bin/i686-linux-android${ANDROID_API_LEVEL}-clang)
elseif ("${ANDROID_ABI}" STREQUAL "x86_64")
	list(APPEND PREPARE_ENV_ARGS GOARCH=amd64)
    list(APPEND PREPARE_ENV_ARGS CC=${ANDROID_TOOLCHAIN_ROOT}/bin/x86_64-linux-android${ANDROID_API_LEVEL}-clang)
elseif ("${ANDROID_ABI}" STREQUAL "arm64-v8a")
	list(APPEND PREPARE_ENV_ARGS GOARCH=arm64)
    list(APPEND PREPARE_ENV_ARGS CC=${ANDROID_TOOLCHAIN_ROOT}/bin/aarch64-linux-android${ANDROID_API_LEVEL}-clang)
elseif ("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
	list(APPEND PREPARE_ENV_ARGS GOARCH=arm)
	list(APPEND PREPARE_ENV_ARGS GOARM=7)
    list(APPEND PREPARE_ENV_ARGS CC=${ANDROID_TOOLCHAIN_ROOT}/bin/armv7a-linux-androideabi${ANDROID_API_LEVEL}-clang)
endif ()

add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
		DEPENDS ${CLOAK_SRCS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
		COMMAND ${GO_EXEC} ${PREPARE_ENV_ARGS}
		COMMAND ${GO_EXEC} ${BUILD_CMD_ARGS}
		COMMENT "Building Go library")

add_custom_target(${TARGET} DEPENDS ${CLOAK_LIB} ${HEADER})
add_library(libck-ovpn-plugin STATIC IMPORTED GLOBAL)
add_dependencies(libck-ovpn-plugin ${TARGET})
set_target_properties(libck-ovpn-plugin
		PROPERTIES
		IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
		INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

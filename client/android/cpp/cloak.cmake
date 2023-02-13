set(TARGET ck_ovpn_plugin_go)

list(APPEND CMAKE_PROGRAM_PATH "/usr/local/go/bin")
message("*** pp - ${CMAKE_PROGRAM_PATH}")

find_program(GO_EXEC go)

message("*** fp - ${GO_EXEC}")

message("*** toolchain ${CMAKE_TOOLCHAIN_FILE}")
message("*** cc start $ENV{CC}")
message("*** ndk ${CMAKE_ANDROID_NDK}")

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR} HOST_TAG)
message("*** HOST: ${HOST_TAG}")

set(ANDROID_PREBUILT_TOOLCHAIN "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/${HOST_TAG}")

#set(MESSAGE_TST "TEST ")
#set(ERROR_VER "ERR ")
#message("*** ${MESSAGE_TST} ${ERROR_VER}")
#execute_process(COMMAND pwd OUTPUT_VARIABLE MESSAGE_TST ERROR_VARIABLE ERROR_VER) # go version
#message("*** after:")
#message("*** ${MESSAGE_TST} ${ERROR_VER}")

set(CLOAK_SRCS cloak/cmd/ck-ovpn-plugin/ck-ovpn-plugin.go)
set(CLOAK_LIB ck-ovpn-plugin.so)

set(ENV{GOPATH} "/Users/kolobchanin/go")
set(ENV{CGO_ENABLED} 1)
set(ENV{GOOS} "android")

message("*** os: $ENV{GOOS}")
message("*** enabled: $ENV{CGO_ENABLED}")
message("*** path: $ENV{GOPATH}")
message("*** go flags: ${CMAKE_GO_FLAGS}")

set(MIN_API 24)

if ("${ANDROID_ABI}" STREQUAL "x86")
	set(ENV{CC} "${ANDROID_PREBUILT_TOOLCHAIN}/bin/i686-linux-android${MIN_API}-clang")
	set(ENV{GOARCH} "386")

	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
			DEPENDS ${CLOAK_SRCS}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
			COMMAND ${GO_EXEC} build -buildmode=c-shared -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}" ${CMAKE_GO_FLAGS} ./...
			COMMENT "Building Go library")

elseif ("${ANDROID_ABI}" STREQUAL "x86_64")
	set(ENV{CC} "${ANDROID_PREBUILT_TOOLCHAIN}/bin/x86_64-linux-android${MIN_API}-clang")
	set(ENV{GOARCH} "amd64")

	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
			DEPENDS ${CLOAK_SRCS}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
			COMMAND ${GO_EXEC} build -buildmode=c-shared -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}" ${CMAKE_GO_FLAGS} ./...
			COMMENT "Building Go library")

elseif ("${ANDROID_ABI}" STREQUAL "arm64-v8a")
	set(ENV{CC} "${ANDROID_PREBUILT_TOOLCHAIN}/bin/aarch64-linux-android${MIN_API}-clang")
	set(ENV{GOARCH} "arm64")

	message("*** cc: $ENV{CC}")
	message("*** arch: $ENV{GOARCH}")

	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
			DEPENDS ${CLOAK_SRCS}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
			COMMAND ${GO_EXEC} build -buildmode=c-shared -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}" -ldflags=\"-s -w -v\" ./...
			> /Users/kolobchanin/log.txt
			COMMENT "Building Go library"
			USES_TERMINAL)

elseif ("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
	set(ENV{CC} "${ANDROID_PREBUILT_TOOLCHAIN}/bin/armv7a-linux-androideabi${MIN_API}-clang")
	set(ENV{GOARCH} "arm")
	set(ENV{GOARM} "7")

	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
			DEPENDS ${CLOAK_SRCS}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
			COMMAND ${GO_EXEC} build -buildmode=c-shared -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}" ${CMAKE_GO_FLAGS} ./...
			COMMENT "Building Go library")
endif ()


add_custom_target(${TARGET} DEPENDS ${CLOAK_LIB} ${HEADER})
add_library(ck-ovpn-plugin STATIC IMPORTED GLOBAL)
add_dependencies(ck-ovpn-plugin ${TARGET})
set_target_properties(ck-ovpn-plugin
		PROPERTIES
		IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
		INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

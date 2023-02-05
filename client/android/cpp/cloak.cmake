set(TARGET ck_ovpn_plugin_go)

set(CLOAK_SRCS cloak/cmd/ck-ovpn-plugin/ck-ovpn-plugin.go)
set(CLOAK_LIB ck-ovpn-plugin.so)

  if ("${ANDROID_ABI}" STREQUAL "x86")
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
	  DEPENDS ${CLOAK_SRCS}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
          COMMAND env GOPATH=$ENV{GOPATH} CGO_ENABLED=1 CC=$ENV{ANDROID_X86_CC} GOOS=android GOARCH=386 go build -buildmode=c-shared 
	  -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}"
	  ${CMAKE_GO_FLAGS} ./...
	  COMMENT "Building Go library")  
    elseif ("${ANDROID_ABI}" STREQUAL "x86_64")
    	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
	  DEPENDS ${CLOAK_SRCS}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
          COMMAND env GOPATH=$ENV{GOPATH} CGO_ENABLED=1 CC=$ENV{ANDROID_X86_64_CC} GOOS=android GOARCH=amd64 go build -buildmode=c-shared 
	  -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}"
	  ${CMAKE_GO_FLAGS} ./...
	  COMMENT "Building Go library")  
    elseif ("${ANDROID_ABI}" STREQUAL "arm64-v8a")
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
	  DEPENDS ${CLOAK_SRCS}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
	  COMMAND env GOPATH=$ENV{GOPATH} CGO_ENABLED=1 CC=$ENV{ANDROID_ARM64_CC} GOOS=android GOARCH=arm64 go build -buildmode=c-shared
	  -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}"
	  ${CMAKE_GO_FLAGS} ./...
	  COMMENT "Building Go library")
    elseif ("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
    	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
	  DEPENDS ${CLOAK_SRCS}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cloak/cmd/ck-ovpn-plugin
          COMMAND env GOPATH=$ENV{GOPATH} CGO_ENABLED=1 CC=$ENV{ANDROID_ARM_CC} GOOS=android GOARCH=arm GOARM=7 go build -buildmode=c-shared 
	  -o "${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}"
	  ${CMAKE_GO_FLAGS} ./...
	  COMMENT "Building Go library")      
    endif ()


add_custom_target(${TARGET} DEPENDS ${CLOAK_LIB} ${HEADER})
add_library(ck-ovpn-plugin STATIC IMPORTED GLOBAL)
add_dependencies(ck-ovpn-plugin ${TARGET})
set_target_properties(ck-ovpn-plugin
  PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${CLOAK_LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

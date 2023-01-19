# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

## Find the absolute path to the go build tool.
find_program(GOLANG_BUILD_TOOL NAMES go REQUIRED)

## Create a library target built from a golang c-archive.
function(add_go_library GOTARGET SOURCE)
    cmake_parse_arguments(GOLANG
        ""
        "GOOS;GOARCH"
        "CGO_CFLAGS;CGO_LDFLAGS"
        ${ARGN})
    get_filename_component(SRC_NAME ${SOURCE} NAME)
    get_filename_component(DIR_NAME ${SOURCE} DIRECTORY)
    get_filename_component(DIR_ABSOLUTE ${DIR_NAME} ABSOLUTE)

    file(GLOB_RECURSE SRC_DEPS ${DIR_NAME}/*.go)
    set(HEADER_NAME "${GOTARGET}.h")
    set(ARCHIVE_NAME "${GOTARGET}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    set(GOCACHE ${CMAKE_BINARY_DIR}/go-cache)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/go-cache)
    set(GOFLAGS -buildmode=c-archive -trimpath -v)
    if(IS_DIRECTORY ${DIR_NAME}/vendor)
        set(GOFLAGS ${GOFLAGS} -mod vendor)
    endif()

    ## Add extras to the CGO compiler and linker flags.
    execute_process(OUTPUT_VARIABLE DEFAULT_CGO_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_CFLAGS)
    execute_process(OUTPUT_VARIABLE DEFAULT_CGO_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_LDFLAGS)
    separate_arguments(DEFAULT_CGO_CFLAGS NATIVE_COMMAND ${DEFAULT_CGO_CFLAGS})
    separate_arguments(DEFAULT_CGO_LDFLAGS NATIVE_COMMAND ${DEFAULT_CGO_LDFLAGS})
    list(PREPEND GOLANG_CGO_CFLAGS ${DEFAULT_CGO_CFLAGS})
    list(PREPEND GOLANG_CGO_LDFLAGS ${DEFAULT_CGO_LDFLAGS})
    if(NOT GOLANG_GOOS)
        execute_process(OUTPUT_VARIABLE GOLANG_GOOS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env GOOS)
    endif()
    if(NOT GOLANG_GOARCH)
        execute_process(OUTPUT_VARIABLE GOLANG_GOARCH OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env GOARCH)
    endif()

    if(APPLE AND CMAKE_OSX_SYSROOT)
        execute_process(OUTPUT_VARIABLE SDKROOT OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND xcrun --sdk ${CMAKE_OSX_SYSROOT} --show-sdk-path)
        list(APPEND GOLANG_CGO_CFLAGS -isysroot ${SDKROOT})
        list(APPEND GOLANG_CGO_LDFLAGS -isysroot ${SDKROOT})
    endif()

    ## The actual commands that do the building.
    add_custom_target(golang_${GOTARGET}
        BYPRODUCTS ${ARCHIVE_NAME} ${HEADER_NAME}
        WORKING_DIRECTORY ${DIR_ABSOLUTE}
        SOURCES ${SRC_DEPS} ${DIR_NAME}/go.mod
        COMMAND ${CMAKE_COMMAND} -E env GOCACHE=${GOCACHE}
                    CGO_ENABLED=1
                    CGO_CFLAGS="${GOLANG_CGO_CFLAGS}"
                    CGO_LDFLAGS="${GOLANG_CGO_LDFLAGS}"
                    GOOS="${GOLANG_GOOS}"
                    GOARCH="${GOLANG_GOARCH}"
                ${GOLANG_BUILD_TOOL} build ${GOFLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME} ${SRC_NAME}
    )
    set_target_properties(golang_${GOTARGET} PROPERTIES FOLDER "Libs")

    ## Wrap up the built library as an imported target.
    add_library(${GOTARGET} STATIC IMPORTED GLOBAL)
    add_dependencies(${GOTARGET} golang_${GOTARGET})
    set_target_properties(${GOTARGET} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}
        INTERFACE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}
        IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME})
endfunction(add_go_library)
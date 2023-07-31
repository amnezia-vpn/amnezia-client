# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

## Find the absolute path to the go build tool.
find_program(GOLANG_BUILD_TOOL NAMES go REQUIRED)

## Build a library file from a golang project.
function(build_go_archive OUTPUT_NAME MODULE_FILE)
    cmake_parse_arguments(GOBUILD
        ""
        "GOOS;GOARCH"
        "CGO_CFLAGS;CGO_LDFLAGS;SOURCES"
        ${ARGN})

    string(REGEX REPLACE "\\.[^/]*$" ".h" GOBUILD_HEADER_FILE ${OUTPUT_NAME})
    get_filename_component(GOBUILD_MODULE_ABS ${MODULE_FILE} ABSOLUTE)
    get_filename_component(GOBUILD_MODULE_DIR ${GOBUILD_MODULE_ABS} DIRECTORY)
    set(GOBUILD_ARGS -buildmode=c-archive -buildvcs=false -trimpath -v)
    if(IS_DIRECTORY ${GOBUILD_MODULE_DIR}/vendor)
        list(APPEND GOBUILD_ARGS -mod vendor)
    endif()

    ## Collect arguments, or find their defaults.
    if(NOT GOBUILD_CGO_CFLAGS)
        execute_process(OUTPUT_VARIABLE GOBUILD_CGO_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_CFLAGS)
        separate_arguments(GOBUILD_CGO_CFLAGS NATIVE_COMMAND ${GOBUILD_CGO_CFLAGS})
    endif()
    if(NOT GOBUILD_CGO_LDFLAGS)
        execute_process(OUTPUT_VARIABLE GOBUILD_CGO_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_LDFLAGS)
        separate_arguments(GOBUILD_CGO_LDFLAGS NATIVE_COMMAND ${GOBUILD_CGO_LDFLAGS})
    endif()
    if(NOT GOBUILD_GOOS)
        execute_process(OUTPUT_VARIABLE GOBUILD_GOOS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env GOOS)
    endif()
    if(NOT GOBUILD_GOARCH)
        execute_process(OUTPUT_VARIABLE GOBUILD_GOARCH OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env GOARCH)
    endif()

    ## Use a go-cache isolated to our project
    set(GOCACHE ${CMAKE_BINARY_DIR}/go-cache)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/go-cache)

    ## The command that does the building
    get_filename_component(ABS_OUTPUT_NAME ${OUTPUT_NAME} ABSOLUTE)
    add_custom_command(
        OUTPUT ${OUTPUT_NAME} ${GOBUILD_HEADER_FILE}
        DEPENDS ${MODULE_FILE} ${GOBUILD_SOURCES}
        WORKING_DIRECTORY ${GOBUILD_MODULE_DIR}
        COMMAND ${CMAKE_COMMAND} -E env GOCACHE=${GOCACHE}
                    CC=${CMAKE_C_COMPILER}
                    CXX=${CMAKE_CXX_COMPILER}
                    CGO_ENABLED=1
                    CGO_CFLAGS="${GOBUILD_CGO_CFLAGS}"
                    CGO_LDFLAGS="${GOBUILD_CGO_LDFLAGS}"
                    GOOS=${GOBUILD_GOOS}
                    GOARCH=${GOBUILD_GOARCH}
                ${GOLANG_BUILD_TOOL} build ${GOBUILD_ARGS} -o ${ABS_OUTPUT_NAME}
    )
endfunction(build_go_archive)

## Create a library target built from a golang c-archive.
function(add_go_library GOTARGET SOURCE)
    cmake_parse_arguments(GOLANG
        ""
        ""
        "CGO_CFLAGS;CGO_LDFLAGS"
        ${ARGN})
    get_filename_component(DIR_NAME ${SOURCE} DIRECTORY)
    get_filename_component(DIR_ABSOLUTE ${DIR_NAME} ABSOLUTE)

    set(HEADER_NAME "${GOTARGET}.h")
    set(ARCHIVE_NAME "${GOTARGET}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    ## Add extras to the CGO compiler and linker flags.
    execute_process(OUTPUT_VARIABLE DEFAULT_CGO_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_CFLAGS)
    execute_process(OUTPUT_VARIABLE DEFAULT_CGO_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND ${GOLANG_BUILD_TOOL} env CGO_LDFLAGS)
    separate_arguments(DEFAULT_CGO_CFLAGS NATIVE_COMMAND ${DEFAULT_CGO_CFLAGS})
    separate_arguments(DEFAULT_CGO_LDFLAGS NATIVE_COMMAND ${DEFAULT_CGO_LDFLAGS})

    ## The actual commands that do the building.
    if((CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND CMAKE_OSX_ARCHITECTURES)
        foreach(OSXARCH ${CMAKE_OSX_ARCHITECTURES})
            string(REPLACE "x86_64" "amd64" GOARCH ${OSXARCH})
            build_go_archive(${CMAKE_CURRENT_BINARY_DIR}/${OSXARCH}/${ARCHIVE_NAME} ${DIR_NAME}/go.mod
                GOARCH ${GOARCH}
                CGO_CFLAGS ${DEFAULT_CGO_CFLAGS} ${GOLANG_CGO_CFLAGS} -arch ${OSXARCH}
                CGO_LDFLAGS ${DEFAULT_CGO_LDFLAGS} ${GOLANG_CGO_LDFLAGS} -arch ${OSXARCH}
            )
            list(APPEND ARCH_ARCHIVE_FILES ${CMAKE_CURRENT_BINARY_DIR}/${OSXARCH}/${ARCHIVE_NAME})
            list(APPEND ARCH_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/${OSXARCH}/${HEADER_NAME})
        endforeach()

        list(GET ARCH_HEADER_FILES 0 FIRST_HEADER_FILE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}
            DEPENDS ${ARCH_ARCHIVE_FILES} ${ARCH_HEADER_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND lipo -create -output ${ARCHIVE_NAME} ${ARCH_ARCHIVE_FILES}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FIRST_HEADER_FILE} ${HEADER_NAME}
        )
    else()
        ## Regular single architecture build
        build_go_archive(${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME} ${DIR_NAME}/go.mod
            CGO_CFLAGS ${DEFAULT_CGO_CFLAGS} ${GOLANG_CGO_CFLAGS}
            CGO_LDFLAGS ${DEFAULT_CGO_LDFLAGS} ${GOLANG_CGO_LDFLAGS}
        )
    endif()

    set_source_files_properties({CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME} PROPERTIES
        GENERATED TRUE
        SKIP_AUTOGEN TRUE
    )

    ## Wrap up the built library as an imported target.
    add_library(${GOTARGET} STATIC IMPORTED GLOBAL)
    set_target_properties(${GOTARGET} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}
        INTERFACE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}
        IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME})

    ## Some dependency glue to ensure we actually build the library.
    add_custom_target(${GOTARGET}_builder DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE_NAME})
    add_dependencies(${GOTARGET} ${GOTARGET}_builder)
    set_target_properties(${GOTARGET}_builder PROPERTIES FOLDER "Libs")

endfunction(add_go_library)

if(WIN32)
    file(GLOB_RECURSE BINARIES
        "${CPACK_TEMPORARY_DIRECTORY}/*.dll"
        "${CPACK_TEMPORARY_DIRECTORY}/*.exe"
    )

    if(BINARIES AND SIGNTOOL_SUBJECT_NAME)
        include(${CMAKE_CURRENT_LIST_DIR}/util/signtool.cmake)
        signtool_sign_files("${BINARIES}" "${SIGNTOOL_SUBJECT_NAME}")
    endif()
endif()

if(APPLE)
    file(GLOB_RECURSE framework_files
        "${CPACK_TEMPORARY_DIRECTORY}/*.framework"
        "${CPACK_TEMPORARY_DIRECTORY}/*.dylib"
        "${CPACK_TEMPORARY_DIRECTORY}/*.appex"
        "${CPACK_TEMPORARY_DIRECTORY}/*.bundle"
        "${CPACK_TEMPORARY_DIRECTORY}/*.xpc"
    )
    file(GLOB_RECURSE exec_files
        "${CPACK_TEMPORARY_DIRECTORY}/Contents/MacOS/*"
    )

    set(files ${framework_files} ${exec_files})

    if (files AND CODESIGN_SIGNATURE)
        include(${CMAKE_CURRENT_LIST_DIR}/util/codesign.cmake)
        codesign_sign_files("${files}" "${CODESIGN_SIGNATURE}" "${CODESIGN_KEYCHAIN}")
    endif()
endif()

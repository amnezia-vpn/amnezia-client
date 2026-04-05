if(WIN32)
    file(GLOB_RECURSE BINARIES
        "${CPACK_TEMPORARY_DIRECTORY}/*.dll"
        "${CPACK_TEMPORARY_DIRECTORY}/*.exe"
    )

    if(BINARIES)
        include(${CMAKE_CURRENT_LIST_DIR}/util/signtool.cmake)
        signtool_sign_files("${BINARIES}" "${SIGNTOOL_SUBJECT_NAME}")
    endif()
endif()

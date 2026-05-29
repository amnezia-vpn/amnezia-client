if(NOT DEFINED SIGNTOOL_SUBJECT_NAME)
    set(SIGNTOOL_SUBJECT_NAME "$ENV{SIGNTOOL_SUBJECT_NAME}")
endif()
if(NOT DEFINED CODESIGN_SIGNATURE)
    set(CODESIGN_SIGNATURE "$ENV{CODESIGN_SIGNATURE}")
endif()
if(NOT DEFINED CODESIGN_KEYCHAIN)
    set(CODESIGN_KEYCHAIN "$ENV{CODESIGN_KEYCHAIN}")
endif()

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
    file(GLOB_RECURSE all_subdirs LIST_DIRECTORIES true "${CPACK_TEMPORARY_DIRECTORY}/*")

    set(frameworks ${all_subdirs})
    list(FILTER frameworks INCLUDE REGEX [[.*\.framework$]])

    file(GLOB_RECURSE dylibs "${CPACK_TEMPORARY_DIRECTORY}/*.dylib")

    set(bundle ${all_subdirs})
    list(FILTER bundle INCLUDE REGEX [[.*\.app$]])
    list(GET bundle 0 bundle)

    file(GLOB_RECURSE execs "${bundle}/Contents/MacOS/*")
    set(client_exec ${execs})
    list(FILTER client_exec INCLUDE REGEX [[AmneziaVPN$]])
    set(service_exec ${execs})
    list(FILTER service_exec INCLUDE REGEX [[AmneziaVPN-service$]])
    set(other_execs ${execs})
    list(FILTER other_execs EXCLUDE REGEX [[AmneziaVPN$|AmneziaVPN-service$]])

    list(APPEND files "${frameworks}" "${dylibs}" "${other_execs}" "${service_exec}" "${client_exec}" "${bundle}")

    if (files AND CODESIGN_SIGNATURE)
        include(${CMAKE_CURRENT_LIST_DIR}/util/codesign.cmake)
        codesign_sign_files("${files}" "${CODESIGN_SIGNATURE}" "${CODESIGN_KEYCHAIN}")
    endif()
endif()

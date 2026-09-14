if(NOT DEFINED SIGNTOOL_SUBJECT_NAME)
    set(SIGNTOOL_SUBJECT_NAME "$ENV{SIGNTOOL_SUBJECT_NAME}")
endif()
if(NOT DEFINED CODESIGN_SIGNATURE)
    set(CODESIGN_SIGNATURE "$ENV{CODESIGN_SIGNATURE}")
endif()
if(NOT DEFINED CODESIGN_KEYCHAIN)
    set(CODESIGN_KEYCHAIN "$ENV{CODESIGN_KEYCHAIN}")
endif()
if(NOT DEFINED CODESIGN_APP_PROVISION_PROFILE)
    set(CODESIGN_APP_PROVISION_PROFILE "$ENV{CODESIGN_APP_PROVISION_PROFILE}")
endif()
if(NOT DEFINED CODESIGN_ST_PROVISION_PROFILE)
    set(CODESIGN_ST_PROVISION_PROFILE "$ENV{CODESIGN_ST_PROVISION_PROFILE}")
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
    list(FILTER bundle INCLUDE REGEX [[/AmneziaVPN\.app$]])
    list(LENGTH bundle bundle_count)
    if(NOT bundle_count EQUAL 1)
        message(FATAL_ERROR "Expected one AmneziaVPN.app in the package staging dir, found ${bundle_count}")
    endif()
    list(GET bundle 0 bundle)

    file(GLOB_RECURSE execs "${bundle}/Contents/MacOS/*")
    set(client_exec ${execs})
    list(FILTER client_exec INCLUDE REGEX [[AmneziaVPN$]])
    set(service_exec ${execs})
    list(FILTER service_exec INCLUDE REGEX [[AmneziaVPN-service$]])
    set(other_execs ${execs})
    list(FILTER other_execs EXCLUDE REGEX [[AmneziaVPN$|AmneziaVPN-service$]])
    list(FILTER other_execs EXCLUDE REGEX [[/pf/]])

    set(sysexts ${all_subdirs})
    list(FILTER sysexts INCLUDE REGEX [[.*\.systemextension$]])

    list(APPEND inner_files "${frameworks}" "${dylibs}" "${other_execs}" "${service_exec}" "${client_exec}")
    list(FILTER inner_files EXCLUDE REGEX "^$")

    if (CODESIGN_SIGNATURE)
        if(NOT sysexts)
            message(FATAL_ERROR "macOS package is missing AmneziaVPNSplitTunnel.systemextension")
        endif()
        if(NOT EXISTS "${CPACK_AMNEZIA_MACOS_APP_ENTITLEMENTS}")
            message(FATAL_ERROR "Host entitlements file is missing: ${CPACK_AMNEZIA_MACOS_APP_ENTITLEMENTS}")
        endif()
        if(NOT EXISTS "${CPACK_AMNEZIA_MACOS_ST_ENTITLEMENTS}")
            message(FATAL_ERROR "Split-tunnel entitlements file is missing: ${CPACK_AMNEZIA_MACOS_ST_ENTITLEMENTS}")
        endif()
        if(NOT CODESIGN_APP_PROVISION_PROFILE OR NOT EXISTS "${CODESIGN_APP_PROVISION_PROFILE}")
            message(FATAL_ERROR "Set CODESIGN_APP_PROVISION_PROFILE to the host Developer ID provisioning profile")
        endif()
        if(NOT CODESIGN_ST_PROVISION_PROFILE OR NOT EXISTS "${CODESIGN_ST_PROVISION_PROFILE}")
            message(FATAL_ERROR "Set CODESIGN_ST_PROVISION_PROFILE to the network-extension Developer ID provisioning profile")
        endif()

        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CODESIGN_APP_PROVISION_PROFILE}"
                "${bundle}/Contents/embedded.provisionprofile"
            RESULT_VARIABLE _amn_copy_app_profile
        )
        if(NOT _amn_copy_app_profile EQUAL 0)
            message(FATAL_ERROR "Failed to copy host provisioning profile into ${bundle}")
        endif()
        foreach(sysex IN LISTS sysexts)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CODESIGN_ST_PROVISION_PROFILE}"
                    "${sysex}/Contents/embedded.provisionprofile"
                RESULT_VARIABLE _amn_copy_st_profile
            )
            if(NOT _amn_copy_st_profile EQUAL 0)
                message(FATAL_ERROR "Failed to copy split-tunnel provisioning profile into ${sysex}")
            endif()
        endforeach()

        include(${CMAKE_CURRENT_LIST_DIR}/util/codesign.cmake)
        if (inner_files)
            codesign_sign_files("${inner_files}" "${CODESIGN_SIGNATURE}" "${CODESIGN_KEYCHAIN}")
        endif()
        codesign_sign_files("${sysexts}" "${CODESIGN_SIGNATURE}" "${CODESIGN_KEYCHAIN}" "${CPACK_AMNEZIA_MACOS_ST_ENTITLEMENTS}")
        codesign_sign_files("${bundle}" "${CODESIGN_SIGNATURE}" "${CODESIGN_KEYCHAIN}" "${CPACK_AMNEZIA_MACOS_APP_ENTITLEMENTS}")
    endif()
endif()

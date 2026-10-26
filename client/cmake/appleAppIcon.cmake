function(check_apple_app_icon ICON_PATH)
    if(NOT EXISTS "${ICON_PATH}/icon.json")
        message(FATAL_ERROR "App icon not found: ${ICON_PATH} (expected an Icon Composer .icon bundle)")
    endif()

    execute_process(
        COMMAND xcrun actool --version --output-format human-readable-text
        OUTPUT_VARIABLE ACTOOL_VERSION_OUTPUT
        ERROR_QUIET
    )
    string(REGEX MATCH "short-bundle-version: ([0-9]+)" ACTOOL_VERSION_MATCH "${ACTOOL_VERSION_OUTPUT}")
    if(NOT ACTOOL_VERSION_MATCH OR CMAKE_MATCH_1 LESS 26)
        message(FATAL_ERROR "Xcode 26 or later is required to build the app icon ${ICON_PATH}")
    endif()

    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE HOST_MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(HOST_MACOS_VERSION VERSION_LESS 26)
        message(FATAL_ERROR "macOS 26 or later is required to build the app icon ${ICON_PATH} (found macOS ${HOST_MACOS_VERSION})")
    endif()
endfunction()

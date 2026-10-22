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
endfunction()

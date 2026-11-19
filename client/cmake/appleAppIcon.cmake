function(apple_icon_composer_supported OUT_VAR)
    set(${OUT_VAR} FALSE PARENT_SCOPE)

    execute_process(
        COMMAND xcrun actool --version --output-format human-readable-text
        OUTPUT_VARIABLE ACTOOL_VERSION_OUTPUT
        ERROR_QUIET
    )
    string(REGEX MATCH "short-bundle-version: ([0-9]+)" ACTOOL_VERSION_MATCH "${ACTOOL_VERSION_OUTPUT}")
    if(NOT ACTOOL_VERSION_MATCH OR CMAKE_MATCH_1 LESS 26)
        return()
    endif()

    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE HOST_MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(HOST_MACOS_VERSION VERSION_LESS 26)
        return()
    endif()

    set(${OUT_VAR} TRUE PARENT_SCOPE)
endfunction()

function(check_apple_app_icon ICON_PATH)
    if(NOT EXISTS "${ICON_PATH}/icon.json")
        message(FATAL_ERROR "App icon not found: ${ICON_PATH} (expected an Icon Composer .icon bundle)")
    endif()

    apple_icon_composer_supported(ICON_COMPOSER_SUPPORTED)
    if(NOT ICON_COMPOSER_SUPPORTED)
        message(FATAL_ERROR "Xcode 26 and macOS 26 or later are required to build the app icon ${ICON_PATH}")
    endif()
endfunction()

function(select_macos_app_icon ICON_PATH ICNS_PATH OUT_USE_ICON_COMPOSER)
    apple_icon_composer_supported(ICON_COMPOSER_SUPPORTED)
    if(ICON_COMPOSER_SUPPORTED)
        check_apple_app_icon(${ICON_PATH})
        set(${OUT_USE_ICON_COMPOSER} TRUE PARENT_SCOPE)
        return()
    endif()

    if(NOT EXISTS "${ICNS_PATH}")
        message(FATAL_ERROR "Xcode 26 and macOS 26 are required for ${ICON_PATH}, and the fallback icon ${ICNS_PATH} was not found")
    endif()
    message(STATUS "Icon Composer needs Xcode 26 and macOS 26, using the fallback app icon ${ICNS_PATH}")
    set(${OUT_USE_ICON_COMPOSER} FALSE PARENT_SCOPE)
endfunction()

if(NOT APPLE)
    message(FATAL_ERROR "OSX Tools are only supported on Apple targets")
endif()

set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

if(CMAKE_COLOR_MAKEFILE)
    set(COMMENT_ECHO_COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --blue --bold)
else()
    set(COMMENT_ECHO_COMMAND ${CMAKE_COMMAND} -E echo)
endif()

if(CODE_SIGN_IDENTITY)
    find_program(CODESIGN_BIN NAMES codesign)
    if(NOT CODESIGN_BIN)
        message(FATAL_ERROR "Cannot sign code, could not find 'codesign' executable")
    endif()
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${CODE_SIGN_IDENTITY})
endif()

## A helper to copy files into the application bundle
function(osx_bundle_files TARGET)
    cmake_parse_arguments(BUNDLE
        ""
        "DESTINATION"
        "FILES"
        ${ARGN})
    
    if(NOT BUNDLE_DESTINATION)
        set(BUNDLE_DESTINATION Resources)
    endif()

    foreach(FILE ${BUNDLE_FILES})
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND_EXPAND_LISTS
            COMMAND ${COMMENT_ECHO_COMMAND} "Bundling ${FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/${BUNDLE_DESTINATION}
            COMMAND ${CMAKE_COMMAND} -E copy ${FILE} $<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/${BUNDLE_DESTINATION}/
        )
    endforeach()
endfunction(osx_bundle_files)

## A helper to bundle an asset catalog.
function(osx_bundle_assetcatalog TARGET)
    cmake_parse_arguments(XCASSETS
        ""
        "CATALOG;PLATFORM"
        "DEVICES"
        ${ARGN})

    if(XCASSETS_PLATFORM)
        set(XCASSETS_TARGET_ARGS --platform ${XCASSETS_PLATFORM})
    elseif(IOS)
        set(XCASSETS_TARGET_ARGS --platform iphoneos)
    else()
        set(XCASSETS_TARGET_ARGS --platform macosx)
    endif()
    foreach(DEVNAME ${XCASSETS_DEVICES})
        list(APPEND XCASSETS_TARGET_ARGS --target-device ${DEVNAME})
    endforeach()
    list(APPEND XCASSETS_TARGET_ARGS --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET})

    ## Compile the asset catalog
    set(XCASSETS_GEN_PLIST ${CMAKE_CURRENT_BINARY_DIR}/xcassets_generated_info.plist)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xcassets/Assets.car ${XCASSETS_GEN_PLIST}
        MAIN_DEPENDENCY ${XCASSETS_CATALOG}/Contents.json
        COMMENT "Building asset catalog"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/xcassets
        COMMAND actool --output-format human-readable-text --notices --warnings
                    ${XCASSETS_TARGET_ARGS}
                    --app-icon AppIcon 
                    --output-partial-info-plist ${XCASSETS_GEN_PLIST}
                    --development-region en --enable-on-demand-resources NO
                    --compile ${CMAKE_CURRENT_BINARY_DIR}/xcassets ${XCASSETS_CATALOG}
    )

    ## Patch the asset catalog into the target bundle.
    if(NOT IOS)
        set(XCASSETS_RESOURCE_DIR "Resources")
    endif()
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMENT "Bundling asset catalog"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/xcassets $<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/${XCASSETS_RESOURCE_DIR}
        COMMAND ${CLIENT_ROOT_DIR}/scripts/macos/merge_plist.py ${XCASSETS_GEN_PLIST} -o $<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/Info.plist
    )

    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/xcassets/Assets.car)
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/xcassets/Assets.car
        ${XCASSETS_GEN_PLIST}
        PROPERTIES
        GENERATED TRUE
        HEADER_FILE_ONLY TRUE
    )

    target_sources(${TARGET} PRIVATE ${XCASSETS_GEN_PLIST})
    set_source_files_properties(${XCASSETS_GEN_PLIST} PROPERTIES GENERATED TRUE)
endfunction()

## A helper to code-sign an executable.
function(osx_codesign_target TARGET)
    ## Xcode should perform automatic code-signing for us.
    if(XCODE)
        return()
    endif()

    if(CODE_SIGN_IDENTITY)
        cmake_parse_arguments(CODESIGN
            "FORCE"
            ""
            "OPTIONS;FILES"
            ${ARGN})

        set(CODESIGN_ARGS --timestamp -s "${CODE_SIGN_IDENTITY}")
        if(CODESIGN_FORCE)
            list(APPEND CODESIGN_ARGS -f)
        endif()
        if(CODESIGN_OPTIONS)
            list(JOIN CODESIGN_OPTIONS , CODESIGN_OPTIONS_JOINED)
            list(APPEND CODESIGN_ARGS "--option=${CODESIGN_OPTIONS_JOINED}")
        endif()

        ## Process the entitlements as though Xcode has done variable expansion.
        ## This only supports the PRODUCT_BUNDLE_IDENTIFIER and DEVELOPMENT_TEAM
        ## for now.
        get_target_property(CODESIGN_ENTITLEMENTS ${TARGET} XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS)
        if(CODESIGN_ENTITLEMENTS)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CLIENT_ROOT_DIR}/scripts/utils/make_template.py ${CODESIGN_ENTITLEMENTS}
                    -k PRODUCT_BUNDLE_IDENTIFIER=$<TARGET_PROPERTY:${TARGET},XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER>
                    -k DEVELOPMENT_TEAM=$<TARGET_PROPERTY:${TARGET},XCODE_ATTRIBUTE_DEVELOPMENT_TEAM>
                    -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_codesign.entitlements
            )
            list(APPEND CODESIGN_ARGS --entitlements ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_codesign.entitlements)
        endif()

        ## If no files were specified, sign the target itself.
        if(NOT CODESIGN_FILES)
            set(CODESIGN_FILES $<TARGET_FILE:${TARGET}>)
        endif()

        foreach(FILE ${CODESIGN_FILES})
            add_custom_command(TARGET ${TARGET} POST_BUILD VERBATIM
                COMMAND ${COMMENT_ECHO_COMMAND} "Signing ${TARGET}: ${FILE}"
                COMMAND ${CODESIGN_BIN} ${CODESIGN_ARGS} ${FILE}
            )
        endforeach()
    endif()
endfunction()
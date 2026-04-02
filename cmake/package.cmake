if (LINUX)
    install(FILES
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.service"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.png"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.desktop"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/post_install.sh"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/post_uninstall.sh"
        DESTINATION "."
        COMPONENT auxiliary
    )

    set(CPACK_COMPONENTS_ALL                            client service auxiliary)
    set(CPACK_GENERATOR                                 "IFW")
    set(CPACK_IFW_PACKAGE_NAME                          "AmneziaVPN")
    set(CPACK_IFW_PACKAGE_TITLE                         "AmneziaVPN")
    set(CPACK_IFW_PACKAGE_VERSION                       "${AMNEZIAVPN_VERSION}")
    set(CPACK_IFW_TARGET_DIRECTORY                      "@ApplicationsDir@/AmneziaVPN")
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH          "600")
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT         "380")
    set(CPACK_IFW_PACKAGE_WIZARD_STYLE                  "Modern")
    set(CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR             ON)
    set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH           ON)
    set(CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS    ON)
    set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT                "${CMAKE_SOURCE_DIR}/deploy/data/linux/controlscript.js")

    include(CPack)
    include(CPackIFW)

    # cpack_ifw_add_repository(main
    #     URL "https://amneziavpn.org/updates/linux"
    #     DISPLAY_NAME "AmneziaVPN - repository for Linux"
    # )

    cpack_add_component_group(AmneziaVPN
        DISPLAY_NAME AmneziaVPN
    )

    cpack_add_component(client
        DISPLAY_NAME "AmneziaVPN Client"
        GROUP AmneziaVPN
    )
    cpack_add_component(service
        DISPLAY_NAME "AmneziaVPN Service"
        GROUP AmneziaVPN
    )
    cpack_add_component(auxiliary
        DISPLAY_NAME "Auxiliary scripts"
        GROUP AmneziaVPN
    )

    cpack_ifw_configure_component_group(AmneziaVPN
        VERSION ${AMNEZIAVPN_VERSION}
        RELEASE_DATE ${RELEASE_DATE}
        REQUIRES_ADMIN_RIGHTS
        FORCED_INSTALLATION
        SCRIPT "${CMAKE_SOURCE_DIR}/deploy/data/linux/componentscript.js"
    )
endif()

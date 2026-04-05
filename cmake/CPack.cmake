set(CPACK_PACKAGE_VENDOR            AmneziaVPN)
set(CPACK_PACKAGE_VERSION           ${AMNEZIAVPN_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY AmneziaVPN)
set(CPACK_PACKAGE_EXECUTABLES       AmneziaVPN AmneziaVPN)
# set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PRE_BUILD_SCRIPTS         ${CMAKE_CURRENT_LIST_DIR}/sign_binaries.cmake)
set(CPACK_POST_BUILD_SCRIPTS        ${CMAKE_CURRENT_LIST_DIR}/sign_packages.cmake)
set(CPACK_PROJECT_CONFIG_FILE       ${CMAKE_CURRENT_LIST_DIR}/CPackOptions.cmake)

if (LINUX AND NOT ANDROID)
    set(CPACK_GENERATOR IFW)
elseif (WIN32)
    set(CPACK_GENERATOR IFW)
endif()

set(CPACK_IFW_PACKAGE_NAME                          AmneziaVPN)
set(CPACK_IFW_PACKAGE_TITLE                         AmneziaVPN)
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH          600)
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT         380)
set(CPACK_IFW_PACKAGE_WIZARD_STYLE                  Modern)
set(CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR             ON)
set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH           ON)
set(CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS    ON)
set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT                "${CMAKE_SOURCE_DIR}/deploy/installer/qif/controlscript.js")

set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_UPGRADE_GUID "{2D55AC62-96D6-4692-8C05-0D85BBF95485}")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/client/images/app.ico")
set(CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util")

set(_AMNEZIA_WIX_PATCH_SERVICE "${CMAKE_SOURCE_DIR}/deploy/installer/wix/service_install_patch.xml")
set(_AMNEZIA_WIX_PATCH_CLOSE_APP "${CMAKE_SOURCE_DIR}/deploy/installer/wix/close_client_patch.xml")
file(TO_CMAKE_PATH "${_AMNEZIA_WIX_PATCH_SERVICE}" _AMNEZIA_WIX_PATCH_SERVICE_CMAKE)
file(TO_CMAKE_PATH "${_AMNEZIA_WIX_PATCH_CLOSE_APP}" _AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE)
set(CPACK_WIX_PATCH_FILE "${_AMNEZIA_WIX_PATCH_SERVICE_CMAKE};${_AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE}")
set(CPACK_WIX_EXTENSIONS "${CPACK_WIX_EXTENSIONS};WixToolset.Util.wixext")

if(LINUX AND NOT ANDROID)
    install(FILES
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.service"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.png"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.desktop"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/post_install.sh"
        "${CMAKE_SOURCE_DIR}/deploy/data/linux/post_uninstall.sh"
        DESTINATION "."
        COMPONENT auxiliary
    )
endif()

if(WIN32)
    install(FILES
        "${CMAKE_SOURCE_DIR}/deploy/data/windows/post_install.cmd"
        "${CMAKE_SOURCE_DIR}/deploy/data/windows/post_uninstall.cmd"
        DESTINATION "."
        COMPONENT auxiliary
    )
endif()

include(CPack)
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

include(CPackIFW)
cpack_ifw_configure_component_group(AmneziaVPN
    VERSION ${AMNEZIAVPN_VERSION}
    RELEASE_DATE ${RELEASE_DATE}
    REQUIRES_ADMIN_RIGHTS
    FORCED_INSTALLATION
    SCRIPT "${CMAKE_SOURCE_DIR}/deploy/installer/qif/componentscript.js"
)

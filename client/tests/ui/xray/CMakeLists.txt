qt6_add_resources(XRAY_PAGES_UI_QRC
    ${CLIENT_QML_QRC_FILE}
    ../../../images/images.qrc
)

add_executable(test_xray_pages_ui
    testXrayPagesUi.h
    main.cpp
    pageMain.cpp
    pageTransport.cpp
    pageSecurity.cpp
    pageFlowPadding.cpp
    pageXmux.cpp
    saveFlowSnapshots.cpp
    knownIssues.cpp
    ${XRAY_PAGES_UI_QRC}
)

target_link_libraries(test_xray_pages_ui PRIVATE
    Qt6::Test
    test_common
)

add_test(NAME XrayPagesUiTest COMMAND test_xray_pages_ui)

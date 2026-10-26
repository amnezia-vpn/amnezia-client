function(add_new_test target source)
    add_executable(${target}
        ${source}
        ${ARGN}
    )

    target_link_libraries(${target} PRIVATE
        Qt6::Test
        test_common
    )
endfunction()

add_new_test(test_ui_client_management_model testClientManagementModel.cpp)
add_new_test(test_ui_containers_model        testContainersModel.cpp)
add_new_test(test_ui_language_controller     testLanguageController.cpp)
add_new_test(test_ui_language_model          testLanguageModel.cpp)
add_new_test(test_ui_protocols_model         testProtocolsModel.cpp)
add_new_test(test_ui_servers_model           testServersModel.cpp)
add_new_test(test_ui_servers_ui_controller   testServersUiController.cpp)

add_test(NAME ClientManagementModelTest COMMAND test_ui_client_management_model)
add_test(NAME ContainersModelTest       COMMAND test_ui_containers_model)
add_test(NAME LanguageControllerTest    COMMAND test_ui_language_controller)
add_test(NAME LanguageModelTest         COMMAND test_ui_language_model)
add_test(NAME ProtocolsModelTest        COMMAND test_ui_protocols_model)
add_test(NAME ServersModelTest          COMMAND test_ui_servers_model)
add_test(NAME ServersUiControllerTest   COMMAND test_ui_servers_ui_controller)

amnezia_copy_runtime_dlls()

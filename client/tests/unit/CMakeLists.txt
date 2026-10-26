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
add_new_test(test_ui_protocols_model         testProtocolsModel.cpp)
add_new_test(test_ui_servers_model           testServersModel.cpp)

add_test(NAME ClientManagementModelTest COMMAND test_ui_client_management_model)
add_test(NAME ProtocolsModelTest        COMMAND test_ui_protocols_model)
add_test(NAME ServersModelTest          COMMAND test_ui_servers_model)

amnezia_copy_runtime_dlls()

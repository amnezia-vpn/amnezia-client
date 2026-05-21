add_executable(test_admin_self_hosted_export
    testAdminSelfHostedExport.cpp
)

target_link_libraries(test_admin_self_hosted_export PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_self_hosted_server_setup
    testSelfHostedServerSetup.cpp
)

target_link_libraries(test_self_hosted_server_setup PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_multiple_exports
    testMultipleExports.cpp
)

target_link_libraries(test_multiple_exports PRIVATE
    Qt6::Test
    test_common
)

add_custom_command(TARGET test_admin_self_hosted_export POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../test_vars.ini
        "$<TARGET_FILE_DIR:test_admin_self_hosted_export>"
)

add_test(NAME AdminSelfHostedExportTest COMMAND test_admin_self_hosted_export)
add_test(NAME SelfHostedServerSetupTest COMMAND test_self_hosted_server_setup)
add_test(NAME MultipleExportsTest COMMAND test_multiple_exports)

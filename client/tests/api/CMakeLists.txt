add_executable(test_ui_news_model_and_controller
    testUiNewsModelAndController.cpp
)

target_link_libraries(test_ui_news_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_gateway_service_import
    testGatewayServiceImport.cpp
)

target_link_libraries(test_gateway_service_import PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_services_model_and_controller
    testUiServicesModelAndController.cpp
)

target_link_libraries(test_ui_services_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_subscription_controller
    testSubscriptionUiController.cpp
)

target_link_libraries(test_ui_subscription_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_api_account_info_model
    testUiApiAccountInfoModel.cpp
)

target_link_libraries(test_ui_api_account_info_model PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_api_benefits_model
    testApiBenefitsModel.cpp
)

target_link_libraries(test_api_benefits_model PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_api_country_model
    testApiCountryModel.cpp
)

target_link_libraries(test_api_country_model PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_api_country_list_model
    testApiCountryListModel.cpp
    ${CLIENT_ROOT_DIR}/resources/countries.qrc
)

target_link_libraries(test_api_country_list_model PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_api_devices_model
    testApiDevicesModel.cpp
)

target_link_libraries(test_api_devices_model PRIVATE
    Qt6::Test
    test_common
)

add_test(NAME UiNewsModelAndControllerTest COMMAND test_ui_news_model_and_controller)
add_test(NAME GatewayServiceImportTest COMMAND test_gateway_service_import)
add_test(NAME UiServicesModelAndControllerTest COMMAND test_ui_services_model_and_controller)
add_test(NAME UiSubscriptionControllerTest COMMAND test_ui_subscription_controller)
add_test(NAME UiApiAccountInfoModelTest COMMAND test_ui_api_account_info_model)
add_test(NAME ApiBenefitsModelTest COMMAND test_api_benefits_model)
add_test(NAME ApiCountryModelTest COMMAND test_api_country_model)
add_test(NAME ApiCountryListModelTest COMMAND test_api_country_list_model)
add_test(NAME ApiDevicesModelTest COMMAND test_api_devices_model)

amnezia_copy_runtime_dlls()

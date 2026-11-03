function(add_new_test target source)
    if(ANDROID)
        qt_add_executable(${target}
            ${source}
            ${ARGN}
        )
    else()
        add_executable(${target}
            ${source}
            ${ARGN}
        )
    endif()

    target_link_libraries(${target} PRIVATE
        Qt6::Test
        test_common
    )

    # These tests are selected for execution on the Android emulator.
    if(ANDROID AND "${target}" MATCHES
       "^(test_ui_api_account_info_model|test_ui_subscription_controller|test_ui_services_model_and_controller)$")
        set_target_properties(${target} PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_TEST_PACKAGE_DIR}"
        )
    endif()
endfunction()

add_new_test(test_ui_news_model_and_controller
    testUiNewsModelAndController.cpp
)

add_new_test(test_gateway_service_import
    testGatewayServiceImport.cpp
)

add_new_test(test_ui_services_model_and_controller
    testUiServicesModelAndController.cpp
)

add_new_test(test_ui_subscription_controller
    testSubscriptionUiController.cpp
)

add_new_test(test_ui_api_account_info_model
    testUiApiAccountInfoModel.cpp
)

add_new_test(test_api_benefits_model
    testApiBenefitsModel.cpp
)

add_new_test(test_api_country_model
    testApiCountryModel.cpp
)

add_new_test(test_api_country_list_model
    testApiCountryListModel.cpp
    ${CLIENT_ROOT_DIR}/resources/countries.qrc
)

add_new_test(test_api_devices_model
    testApiDevicesModel.cpp
)

if(NOT ANDROID)
add_test(NAME UiNewsModelAndControllerTest COMMAND test_ui_news_model_and_controller)
add_test(NAME GatewayServiceImportTest COMMAND test_gateway_service_import)
add_test(NAME UiServicesModelAndControllerTest COMMAND test_ui_services_model_and_controller)
add_test(NAME UiSubscriptionControllerTest COMMAND test_ui_subscription_controller)
add_test(NAME UiApiAccountInfoModelTest COMMAND test_ui_api_account_info_model)
add_test(NAME ApiBenefitsModelTest COMMAND test_api_benefits_model)
add_test(NAME ApiCountryModelTest COMMAND test_api_country_model)
add_test(NAME ApiCountryListModelTest COMMAND test_api_country_list_model)
add_test(NAME ApiDevicesModelTest COMMAND test_api_devices_model)
endif()

amnezia_copy_runtime_dlls()

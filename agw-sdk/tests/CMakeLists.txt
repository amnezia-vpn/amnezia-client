# Тесты agw-sdk. Локально — на встроенном harness (agw_test.h), без внешнего фреймворка.

set(AGW_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/golden/fixtures")

function(agw_add_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(${name} PRIVATE agw OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(${name} PRIVATE AGW_FIXTURES_DIR="${AGW_FIXTURES_DIR}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

agw_add_test(test_crypto        unit/test_crypto.cpp)
agw_add_test(test_json          unit/test_json.cpp)
agw_add_test(test_golden        golden/test_golden.cpp)
agw_add_test(test_error_mapping unit/test_error_mapping.cpp)
agw_add_test(test_bypass_policy unit/test_bypass_policy.cpp)
agw_add_test(test_proxy_list    unit/test_proxy_list.cpp)
agw_add_test(test_thread_pool   unit/test_thread_pool.cpp)
agw_add_test(test_post          integration/test_post.cpp)
agw_add_test(test_failover      integration/test_failover.cpp)
agw_add_test(test_async         integration/test_async.cpp)
agw_add_test(test_c_abi         integration/test_c_abi.cpp)
agw_add_test(test_tier2_step0   unit/test_tier2_step0.cpp)
agw_add_test(test_api_methods   integration/test_api_methods.cpp)

# Тесты agw-sdk. Локально — на встроенном harness (agw_test.h), без внешнего фреймворка.

set(AGW_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/golden/fixtures")

function(agw_add_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(${name} PRIVATE agw OpenSSL::Crypto nlohmann_json::nlohmann_json)
    target_compile_definitions(${name} PRIVATE AGW_FIXTURES_DIR="${AGW_FIXTURES_DIR}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

agw_add_test(test_crypto        unit/test_crypto.cpp)
agw_add_test(test_json          unit/test_json.cpp)
agw_add_test(test_golden        golden/test_golden.cpp)
agw_add_test(test_error_mapping unit/test_error_mapping.cpp)
agw_add_test(test_post          integration/test_post.cpp)

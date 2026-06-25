cmake_minimum_required(VERSION 3.21)

project(agw LANGUAGES CXX VERSION 0.1.0)

# --- стандарт ---------------------------------------------------------------
# floor C++20 (см. решения в docs/plans/gateway-sdk/README.md). C++23 — opt-in позже.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- опции ------------------------------------------------------------------
option(AGW_BUILD_TESTS "Build agw-sdk tests" ON)

# Режимы зависимостей (Фаза 5): shared-deps — общий OpenSSL из Conan; vendored — бандл.
# На Фазе 1 определяем only-флаг, реальный механизм бандла появится в Фазе 5.
set(AGW_DEPS_MODE "shared-deps" CACHE STRING "Dependency mode: shared-deps | vendored")

# --- зависимости ------------------------------------------------------------
find_package(OpenSSL REQUIRED)

# libcurl (транспорт по умолчанию). Если не найден — SDK собирается без него, а клиент обязан
# передать свой IHttpClient через Config (см. default_client_fallback.cpp).
find_package(CURL QUIET)

# nlohmann/json: предпочтительно из Conan (find_package), иначе — вендоренный single-header
# (он лежит в tests/third_party и нужен только для локальной сборки без Conan).
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    add_library(agw_nlohmann_fallback INTERFACE)
    target_include_directories(agw_nlohmann_fallback INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party)
    add_library(nlohmann_json::nlohmann_json ALIAS agw_nlohmann_fallback)
    message(STATUS "agw: using vendored nlohmann/json fallback (tests/third_party)")
endif()

# --- библиотека -------------------------------------------------------------
add_library(agw STATIC
    src/client.cpp
    src/crypto/rng.cpp
    src/crypto/aes.cpp
    src/crypto/rsa.cpp
    src/crypto/hash.cpp
    src/http/curl_client.cpp
    src/http/default_client_fallback.cpp
    src/protocol/request_builder.cpp
    src/protocol/response.cpp
    src/protocol/error_mapping.cpp
    src/failover/bypass_policy.cpp
    src/failover/proxy_list.cpp
    src/failover/proxy_picker.cpp
    src/util/base64.cpp
    src/util/uuid.cpp
    src/util/json.cpp
    src/util/url.cpp
)
add_library(agw::agw ALIAS agw)

target_include_directories(agw
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(agw
    PRIVATE OpenSSL::Crypto nlohmann_json::nlohmann_json
)

if(CURL_FOUND)
    target_compile_definitions(agw PRIVATE AGW_HAVE_CURL)
    target_link_libraries(agw PRIVATE CURL::libcurl)
    message(STATUS "agw: libcurl found — default HTTP client enabled")
else()
    message(STATUS "agw: libcurl NOT found — default HTTP client disabled (inject IHttpClient via Config)")
endif()

set_target_properties(agw PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

# --- тесты ------------------------------------------------------------------
if(AGW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

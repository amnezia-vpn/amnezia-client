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
# Когда SDK подключают через add_subdirectory (отладочная сборка в клиенте), по умолчанию НЕ строим
# тесты и НЕ строим shared C-ABI, чтобы не пачкать родительскую сборку. Standalone — всё включено.
if(PROJECT_IS_TOP_LEVEL)
    set(_agw_default_aux ON)
else()
    set(_agw_default_aux OFF)
endif()
option(AGW_BUILD_TESTS "Build agw-sdk tests" ${_agw_default_aux})

# Режимы зависимостей (Фаза 5): shared-deps — общий OpenSSL из Conan; vendored — бандл.
# На Фазе 1 определяем only-флаг, реальный механизм бандла появится в Фазе 5.
set(AGW_DEPS_MODE "shared-deps" CACHE STRING "Dependency mode: shared-deps | vendored")

# --- зависимости ------------------------------------------------------------
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)  # Tier 2: qCompress/qUncompress (vpn-ключ, разбор конфигов)

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
# Один раз компилируем объекты, из них собираем static (agw) и shared C-ABI (agw_capi).
add_library(agw_obj OBJECT
    src/gateway_controller.cpp
    src/c_abi.cpp
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
    src/util/thread_pool.cpp
    src/util/zlib.cpp
    src/util/checksum.cpp
    src/services/vpn_key.cpp
    src/services/subscription.cpp
    src/services/api_methods.cpp
    src/services/models.cpp
    src/services/server_config.cpp
    src/api_public.cpp
)

target_include_directories(agw_obj
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# PUBLIC, чтобы зависимости и include-пути дотекли до static/shared (и до тестов).
target_link_libraries(agw_obj
    PUBLIC OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads ZLIB::ZLIB
)
# Скрываем всё по умолчанию: наружу торчат только agw_* (AGW_API = visibility default).
set_target_properties(agw_obj PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

if(CURL_FOUND)
    target_compile_definitions(agw_obj PRIVATE AGW_HAVE_CURL)
    target_link_libraries(agw_obj PUBLIC CURL::libcurl)
    message(STATUS "agw: libcurl found — default HTTP client enabled")
else()
    message(STATUS "agw: libcurl NOT found — default HTTP client disabled (inject IHttpClient via Config)")
endif()

# Статическая библиотека (C++ API) — для наших приложений / тестов.
add_library(agw STATIC)
target_link_libraries(agw PUBLIC agw_obj)
add_library(agw::agw ALIAS agw)

# Shared C-ABI библиотека (для dart:ffi и сторонних). Экспортирует только agw_*.
option(AGW_BUILD_CAPI_SHARED "Build shared C-ABI library (agw_capi)" ${_agw_default_aux})
if(AGW_BUILD_CAPI_SHARED)
    add_library(agw_capi SHARED $<TARGET_OBJECTS:agw_obj>)
    target_link_libraries(agw_capi PRIVATE agw_obj)
    target_compile_definitions(agw_capi PRIVATE AGW_BUILDING_SHARED)
    set_target_properties(agw_capi PROPERTIES OUTPUT_NAME agw_capi)
    message(STATUS "agw: deps mode = ${AGW_DEPS_MODE} (vendored — статическая линковка/скрытие символов, через Conan)")
endif()

set_target_properties(agw PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

# --- установка (только для Conan-пакета / standalone; не при add_subdirectory) ---------------
if(PROJECT_IS_TOP_LEVEL)
    include(GNUInstallDirs)
    install(TARGETS agw ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if(AGW_BUILD_CAPI_SHARED)
        install(TARGETS agw_capi
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/agw
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# --- тесты ------------------------------------------------------------------
if(AGW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

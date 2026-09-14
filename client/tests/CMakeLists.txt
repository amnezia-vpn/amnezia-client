find_package(Qt6 REQUIRED COMPONENTS Test)

include(CTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt6_add_resources(TEST_QRC
    ../server_scripts/serverScripts.qrc
)

set(_main_cpp "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
get_filename_component(_main_cpp "${_main_cpp}" REALPATH)

set(TEST_SOURCES ${SOURCES})
list(REMOVE_ITEM TEST_SOURCES "${_main_cpp}")

add_library(test_common OBJECT
    ${TEST_SOURCES}
    ${HEADERS}
    ../ui/utils/pageEnum.h
    utils/testUtils.cpp
    ${TEST_QRC}
)

qt_add_repc_replicas(test_common
    ../../ipc/ipc_interface.rep
    ../../ipc/ipc_process_interface.rep
)

target_link_libraries(test_common PUBLIC
    ${LIBS}
)

target_include_directories(test_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(NOT ANDROID)
    add_executable(test_multiple_imports
        testMultipleImports.cpp
    )

    target_link_libraries(test_multiple_imports PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_server_edit
        testServerEdit.cpp
    )

    target_link_libraries(test_server_edit PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_default_server_change
        testDefaultServerChange.cpp
    )

    target_link_libraries(test_default_server_change PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_server_edge_cases
        testServerEdgeCases.cpp
    )

    target_link_libraries(test_server_edge_cases PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_signal_order
        testSignalOrder.cpp
    )

    target_link_libraries(test_signal_order PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_servers_model_sync
        testServersModelSync.cpp
    )

    target_link_libraries(test_servers_model_sync PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_server_rename
        testServerRename.cpp
    )

    target_link_libraries(test_server_rename PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_settings_signals
        testSettingsSignals.cpp
    )

    target_link_libraries(test_settings_signals PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_ui_servers_model_and_controller
        testUiServersModelAndController.cpp
    )

    target_link_libraries(test_ui_servers_model_and_controller PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_xray_serialization
        testXraySerialization.cpp
    )

    target_link_libraries(test_xray_serialization PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_ui_language_model_and_controller
        testUiLanguageModelAndController.cpp
    )

    target_link_libraries(test_ui_language_model_and_controller PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_ui_ip_model_and_controller
        testUiIpModelAndController.cpp
    )

    target_link_libraries(test_ui_ip_model_and_controller PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_ui_app_st_model_and_controller
        testUiAppSTModelAndController.cpp
    )

    target_link_libraries(test_ui_app_st_model_and_controller PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_ui_allowed_dns_model_and_controller
        testUiAllowedDnsModelAndController.cpp
    )

    target_link_libraries(test_ui_allowed_dns_model_and_controller PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_backup_export_import
        testBackupExportImport.cpp
    )

    target_link_libraries(test_backup_export_import PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_connection
        testConnection.cpp
    )

    target_link_libraries(test_connection PRIVATE
        Qt6::Test
        test_common
    )

    add_executable(test_openssl_library
        testOpenSSLLibrary.cpp
    )

    target_link_libraries(test_openssl_library PRIVATE
        Qt6::Test
        test_common
    )

    if(WIN32)
        # libcrypto-3-x64.dll is a hard import of this binary.
        # Without copying Conan's DLL here, Windows loads the first match in PATH
        # (MySQL, Git, PHP...) which differs from the Conan version we compiled against,
        # causing testRuntimeVersionMatchesCompileTime and testWindowsNoDllHijack to fail.
        foreach(_ssl_tgt OpenSSL::Crypto OpenSSL::SSL)
            if(NOT TARGET "${_ssl_tgt}")
                continue()
            endif()
            set(_ssl_dll "")
            foreach(_prop IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_DEBUG
                          IMPORTED_LOCATION_MINSIZEREL IMPORTED_LOCATION_RELWITHDEBINFO
                          IMPORTED_LOCATION_NOCONFIG)
                get_target_property(_candidate "${_ssl_tgt}" "${_prop}")
                if(_candidate AND NOT _candidate MATCHES "-NOTFOUND" AND _candidate MATCHES "\\.dll$")
                    set(_ssl_dll "${_candidate}")
                    break()
                endif()
            endforeach()
            if(_ssl_dll)
                add_custom_command(TARGET test_openssl_library POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_ssl_dll}" "$<TARGET_FILE_DIR:test_openssl_library>"
                )
            else()
                message(STATUS "No .dll found for ${_ssl_tgt} (IMPORTED_LOCATION may point to .lib) — skipping copy")
            endif()
        endforeach()
    endif()
endif() # NOT ANDROID

if(ANDROID)
    set(ANDROID_TEST_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android_test_package")

    # Qt 6.x for Android ships its TLS plugin linked against libcrypto_3.so / libssl_3.so
    # (Android convention: use underscore instead of dots for versioned sonames).
    # Conan provides libcrypto.so.3 / libssl.so.3 with SONAME=libcrypto.so.3.
    # We copy them with the _3.so suffix and patch the SONAME in-place.
    # Both old ("libcrypto.so.3") and new ("libcrypto_3.so") are 14 bytes � exact fit.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_package(OpenSSL REQUIRED)

    function(_get_openssl_lib target outvar)
        foreach(_prop IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_DEBUG IMPORTED_LOCATION_NOCONFIG)
            get_target_property(_loc "${target}" "${_prop}")
            if(_loc AND NOT _loc MATCHES "NOTFOUND")
                set(${outvar} "${_loc}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endfunction()

    _get_openssl_lib(OpenSSL::Crypto _openssl_crypto_src)
    _get_openssl_lib(OpenSSL::SSL    _openssl_ssl_src)

    message(STATUS "OpenSSL Crypto src: ${_openssl_crypto_src}")
    message(STATUS "OpenSSL SSL src:    ${_openssl_ssl_src}")

    set(_openssl_crypto_dst "${CMAKE_CURRENT_BINARY_DIR}/libcrypto_3.so")
    set(_openssl_ssl_dst    "${CMAKE_CURRENT_BINARY_DIR}/libssl_3.so")

    configure_file("${_openssl_crypto_src}" "${_openssl_crypto_dst}" COPYONLY)
    configure_file("${_openssl_ssl_src}"    "${_openssl_ssl_dst}"    COPYONLY)

    # Patch SONAME bytes in our copies (in-place string replacement in .dynstr).
    # libcrypto.so.3 → libcrypto_3.so  (14 chars each, no padding needed)
    # libssl.so.3    → libssl_3.so     (11 chars each, no padding needed)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/patch_soname.py" [=[
import sys

def patch_all(path, old, new):
    assert len(old) == len(new), f"SONAME lengths differ: {old!r} vs {new!r}"
    old_b = old.encode() + b'\x00'
    new_b = new.encode() + b'\x00'
    with open(path, 'rb') as f:
        data = bytearray(f.read())
    count, idx = 0, 0
    while True:
        idx = data.find(old_b, idx)
        if idx < 0:
            break
        data[idx:idx + len(old_b)] = new_b
        count += 1
        idx += len(new_b)
    if count:
        with open(path, 'wb') as f:
            f.write(data)
        print(f'  patched {count}x "{old}" -> "{new}" in {path}')
    else:
        print(f'  WARNING: "{old}" not found in {path} (already patched?)')

# libcrypto: SONAME libcrypto.so.3 → libcrypto_3.so  (both 14 chars)
patch_all(sys.argv[1], 'libcrypto.so.3', 'libcrypto_3.so')
# libssl: SONAME libssl.so.3 → libssl_3.so  (both 11 chars)
patch_all(sys.argv[2], 'libssl.so.3',    'libssl_3.so')
# libssl DT_NEEDED on libcrypto: libcrypto.so.3 → libcrypto_3.so
patch_all(sys.argv[2], 'libcrypto.so.3', 'libcrypto_3.so')
print('SONAME patch done.')
]=])
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/patch_soname.py"
            "${_openssl_crypto_dst}" "${_openssl_ssl_dst}"
        RESULT_VARIABLE _patch_result
        OUTPUT_VARIABLE _patch_output
        ERROR_VARIABLE  _patch_error
    )
    message(STATUS "SONAME patch output: ${_patch_output}")
    if(NOT _patch_result EQUAL 0)
        message(FATAL_ERROR "SONAME patch failed:\n${_patch_error}")
    endif()

    # Redirect all link targets so tests get DT_NEEDED:libcrypto_3.so, not libcrypto.so.3.
    # Must override every config-specific variant because CMake prefers
    # IMPORTED_LOCATION_DEBUG over IMPORTED_LOCATION for Debug builds.
    foreach(_ssl_target OpenSSL::Crypto OpenSSL::SSL)
        if(_ssl_target STREQUAL "OpenSSL::Crypto")
            set(_ssl_dst "${_openssl_crypto_dst}")
        else()
            set(_ssl_dst "${_openssl_ssl_dst}")
        endif()
        foreach(_prop IMPORTED_LOCATION IMPORTED_LOCATION_DEBUG IMPORTED_LOCATION_RELEASE
                      IMPORTED_LOCATION_MINSIZEREL IMPORTED_LOCATION_RELWITHDEBINFO
                      IMPORTED_LOCATION_NOCONFIG)
            get_target_property(_cur "${_ssl_target}" "${_prop}")
            if(_cur AND NOT _cur MATCHES "NOTFOUND")
                set_property(TARGET "${_ssl_target}" PROPERTY "${_prop}" "${_ssl_dst}")
            endif()
        endforeach()
    endforeach()

    qt_add_executable(test_android_openssl
        testAndroidOpenSSL.cpp
    )

    target_link_libraries(test_android_openssl PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Gui
        Qt6::Network
    )

    target_include_directories(test_android_openssl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    set_target_properties(test_android_openssl PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_TEST_PACKAGE_DIR}"
        QT_ANDROID_EXTRA_LIBS         "${_openssl_crypto_dst};${_openssl_ssl_dst}"
    )

    qt_add_executable(test_android_restore_backup
        testAndroidRestoreBackup.cpp
    )

    target_link_libraries(test_android_restore_backup PRIVATE
        Qt6::Test
        test_common
    )

    set_target_properties(test_android_restore_backup PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_TEST_PACKAGE_DIR}"
        QT_ANDROID_EXTRA_LIBS         "${_openssl_crypto_dst};${_openssl_ssl_dst}"
    )
endif()

if(NOT ANDROID)
    add_subdirectory(selfHostedAdmin)
    add_subdirectory(api)
endif()

enable_testing()

if(NOT ANDROID)
    add_test(NAME MultipleImportsTest              COMMAND test_multiple_imports)
    add_test(NAME ServerEditTest                   COMMAND test_server_edit)
    add_test(NAME DefaultServerChangeTest          COMMAND test_default_server_change)
    add_test(NAME ServerEdgeCasesTest              COMMAND test_server_edge_cases)
    add_test(NAME SignalOrderTest                  COMMAND test_signal_order)
    add_test(NAME ServersModelSyncTest             COMMAND test_servers_model_sync)
    add_test(NAME ServerRenameTest                 COMMAND test_server_rename)
    add_test(NAME SettingsSignalsTest              COMMAND test_settings_signals)
    add_test(NAME UiServersModelAndControllerTest  COMMAND test_ui_servers_model_and_controller)
    add_test(NAME XraySerializationTest            COMMAND test_xray_serialization)
    add_test(NAME UiLanguageModelAndControllerTest COMMAND test_ui_language_model_and_controller)
    add_test(NAME UiIpModelAndControllerTest       COMMAND test_ui_ip_model_and_controller)
    add_test(NAME UiAppSTModelAndControllerTest    COMMAND test_ui_app_st_model_and_controller)
    add_test(NAME UiAllowedDnsModelAndControllerTest COMMAND test_ui_allowed_dns_model_and_controller)
    add_test(NAME BackupExportImportTest           COMMAND test_backup_export_import)
    add_test(NAME ConnectionTest                   COMMAND test_connection)
    add_test(NAME OpenSSLLibraryTest               COMMAND test_openssl_library)
endif()

# Android tests are executed on-device via androidtestrunner in CI (see .github/workflows/tests.yml).
# CTest cannot run Android binaries on the host machine, so no add_test() entries are registered here.

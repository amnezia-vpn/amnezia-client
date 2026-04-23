set(CLI_PROJECT amnezia-cli)

set(CLI_SOURCES ${SOURCES})
list(FILTER CLI_SOURCES EXCLUDE REGEX "/main\\.cpp$")
list(FILTER CLI_SOURCES EXCLUDE REGEX "/amnezia_application\\.cpp$")
list(FILTER CLI_SOURCES EXCLUDE REGEX "/core/controllers/coreController\\.cpp$")
list(FILTER CLI_SOURCES EXCLUDE REGEX "/ui/controllers/settingsController\\.cpp$")
list(FILTER CLI_SOURCES EXCLUDE REGEX "/core/osSignalHandler\\.cpp$")

set(CLI_HEADERS ${HEADERS})
list(FILTER CLI_HEADERS EXCLUDE REGEX "/amnezia_application\\.h$")
list(FILTER CLI_HEADERS EXCLUDE REGEX "/core/controllers/coreController\\.h$")
list(FILTER CLI_HEADERS EXCLUDE REGEX "/ui/controllers/settingsController\\.h$")
list(FILTER CLI_HEADERS EXCLUDE REGEX "/core/osSignalHandler\\.h$")

list(APPEND CLI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cli_context.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cli_ipc.cpp
)

qt_add_executable(${CLI_PROJECT} MANUAL_FINALIZATION)
target_include_directories(${CLI_PROJECT} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    qt_add_repc_replicas(${CLI_PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_interface.rep)
    qt_add_repc_replicas(${CLI_PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_process_interface.rep)
endif()

target_link_libraries(${CLI_PROJECT} PRIVATE ${LIBS})
target_compile_definitions(${CLI_PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    target_compile_definitions(${CLI_PROJECT} PRIVATE AMNEZIA_DESKTOP)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${CLI_PROJECT} PRIVATE "MZ_DEBUG")
endif()

target_sources(${CLI_PROJECT} PRIVATE ${CLI_SOURCES} ${CLI_HEADERS})

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_custom_command(
        TARGET ${CLI_PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
        ${CMAKE_SOURCE_DIR}/deploy/data/${DEPLOY_PLATFORM_PATH}
        $<TARGET_FILE_DIR:${CLI_PROJECT}>
        COMMAND_EXPAND_LISTS
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/client/3rd-prebuilt/deploy-prebuilt/${DEPLOY_PLATFORM_PATH}")
        add_custom_command(
            TARGET ${CLI_PROJECT} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
            ${CMAKE_SOURCE_DIR}/client/3rd-prebuilt/deploy-prebuilt/${DEPLOY_PLATFORM_PATH}
            $<TARGET_FILE_DIR:${CLI_PROJECT}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(COMMAND qt_import_qml_plugins)
    qt_import_qml_plugins(${CLI_PROJECT})
endif()
if(COMMAND qt_finalize_executable)
    qt_finalize_executable(${CLI_PROJECT})
else()
    qt_finalize_target(${CLI_PROJECT})
endif()

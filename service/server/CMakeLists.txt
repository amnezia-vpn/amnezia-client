cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN-service)
project(${PROJECT} VERSION ${AMNEZIAVPN_VERSION})

if (APPLE)
    # CMAKE_INSTALL_BINDIR must be called before find_package(Qt6) for
    # Qt to configure prefixes properly
    # FIXME: make Amnezia-service a bundle and remove this hack completely
    set(CMAKE_INSTALL_BINDIR "AmneziaVPN.app/Contents/MacOS")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets RemoteObjects Core5Compat)
qt_standard_project_setup()

configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/utilities.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.h
    ${CMAKE_CURRENT_LIST_DIR}/localserver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.h
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.h
    ${CMAKE_CURRENT_LIST_DIR}/xtunController.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/utilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/localserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtunController.cpp
)

set(RESOURCES)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/amneziavpn-service.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn-service.rc
    )

    set(RESOURCES ${RESOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn-service.rc
    )

    set(LIBS
        user32
        rasapi32
        shlwapi
        iphlpapi
        ws2_32
        iphlpapi
        gdi32
        Advapi32
        Kernel32
    )

    add_compile_definitions(_WINSOCKAPI_)
endif()

if(LINUX)
    set(LIBS
        -static-libstdc++
        -static-libgcc
        -ldl
    )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../src/qtservice.cmake)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../client
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger
    ${CMAKE_CURRENT_BINARY_DIR}
)


add_executable(${PROJECT} ${SOURCES} ${HEADERS} ${RESOURCES})
target_link_libraries(${PROJECT} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::RemoteObjects Qt6::Core5Compat ${LIBS})

find_package(xtun REQUIRED)
target_link_libraries(${PROJECT} PRIVATE amnezia::xtun)

if(APPLE)
    set_target_properties(${PROJECT} PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

qt_add_repc_sources(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_interface.rep)

if(WIN32)
    find_package(OpenSSL REQUIRED)
    set(CONAN_BINS $<TARGET_FILE:OpenSSL::SSL> $<TARGET_FILE:OpenSSL::Crypto>)

    add_custom_command(TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CONAN_BINS}
            "$<TARGET_FILE_DIR:${PROJECT}>"
    )
    install(FILES ${CONAN_BINS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
    )
endif()

# install target
install(TARGETS ${PROJECT}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME_DEPENDENCY_SET service_deps
    COMPONENT AmneziaVPN
)

if(APPLE)
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR}/../Frameworks)
else()
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR})
endif()

install(RUNTIME_DEPENDENCY_SET service_deps
    PRE_EXCLUDE_REGEXES
        [[api-ms-win-.*]]
        [[ext-ms-.*]]
        [[kernel32\.dll]]
        [[hvsifiletrust\.dll]]
        [[libc\.so\..*]] [[libgcc_s\.so\..*]] [[libm\.so\..*]] [[libstdc\+\+\.so\..*]]
        [[.*\.framework]]
        [[^(lib)?[Qq]t.*]]
    POST_EXCLUDE_REGEXES
        [[^.*[\\/]system32[\\/].*\.dll$]]
        [[^/lib.*]]
        [[^/usr/lib.*]]
    DIRECTORIES ${CONAN_RUNTIME_LIB_DIRS}
    COMPONENT AmneziaVPN
    DESTINATION "${RUNTIME_DEPS_DIR}"
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT}
    OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${QT_DEPLOY_SCRIPT}
    COMPONENT AmneziaVPN
)

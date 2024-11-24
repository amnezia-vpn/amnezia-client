set(PROJECT serverMacShared)

set(CMAKE_Swift_LANGUAGE_VERSION "5")

project(${PROJECT} LANGUAGES Swift)

add_library(${PROJECT} STATIC ${CMAKE_CURRENT_LIST_DIR}/SharedXPCMessages.swift)

target_link_libraries(${PROJECT} PUBLIC SecureXPC)

set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
)
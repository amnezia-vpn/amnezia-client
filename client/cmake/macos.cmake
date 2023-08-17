message("MAC build")

set_target_properties(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.mm
)

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/app.icns)
set(MACOSX_BUNDLE_ICON_FILE app.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${ICON_FILE})



find_library(FW_SYSTEMCONFIG SystemConfiguration)
find_library(FW_SERVICEMGMT ServiceManagement)
find_library(FW_SECURITY Security)
find_library(FW_COREWLAN CoreWLAN)
find_library(FW_NETWORK Network)
find_library(FW_USER_NOTIFICATIONS UserNotifications)

target_link_libraries(${PROJECT} PRIVATE ${FW_SYSTEMCONFIG})
target_link_libraries(${PROJECT} PRIVATE ${FW_SERVICEMGMT})
target_link_libraries(${PROJECT} PRIVATE ${FW_SECURITY})
target_link_libraries(${PROJECT} PRIVATE ${FW_COREWLAN})
target_link_libraries(${PROJECT} PRIVATE ${FW_NETWORK})
target_link_libraries(${PROJECT} PRIVATE ${FW_USER_NOTIFICATIONS})

# Get SDK path
execute_process(
    COMMAND sh -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE OSX_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("OSX_SDK_PATH is: ${OSX_SDK_PATH}")



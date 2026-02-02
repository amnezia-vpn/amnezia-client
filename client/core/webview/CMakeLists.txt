get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("Configuring " ${DIR_NAME})

set(webview_URI AmneziaWebView)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Quick)

# Widgets and WebEngineWidgets are only available on desktop platforms
if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngineWidgets)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)

set(PLUGIN_CLASS_NAME WebViewPlugin)
add_definitions(-DURI=${webview_URI})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(webview_HEADERS
       amneziawebview.h
       amneziawebview_p.h
       websettings.h
       mimecache.h
       filehandler.h
       qrchandler.h
       jshandler.h
       plugin.h
       amneziawebhistory.h
       amneziawebhistory_p.h
)

set(webview_SOURCES
    amneziawebview.cpp
    amneziawebview_p.cpp
    websettings.cpp
    mimecache.cpp
    qrchandler.cpp
    jshandler.cpp
    filehandler.cpp
    plugin.cpp
    amneziawebhistory.cpp
)

if (CMAKE_CROSSCOMPILING AND ANDROID)

    list(APPEND webview_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/jshandler_android.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/qrchandler_android.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/filehandler_android.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/amneziawebview_android.cpp"
    )

endif ()

if (CMAKE_CROSSCOMPILING AND APPLE)

    add_definitions(-DENABLE_WKWEBVIEW)
    list(APPEND webview_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/amneziawebview_ios.mm"
        "${CMAKE_CURRENT_LIST_DIR}/qrchandler_ios.mm"
        "${CMAKE_CURRENT_LIST_DIR}/jshandler_ios.mm"
        "${CMAKE_CURRENT_LIST_DIR}/filehandler_ios.mm"
    )

endif ()

if (NOT CMAKE_CROSSCOMPILING)
    # Require WebEngineWidgets for desktop platforms (QtWebKit is not available in Qt 6)
    if (Qt6WebEngineWidgets_FOUND)
        message(STATUS "Using Qt WebEngineWidgets for desktop webview")
        list(APPEND webview_HEADERS
            amneziawebview_webengine_p.h
        )
        list(APPEND webview_SOURCES
            amneziawebview_webengine.cpp
        )
    else ()
        message(FATAL_ERROR "Qt WebEngineWidgets is required for desktop builds. QtWebKit is not available in Qt 6. Please install Qt WebEngineWidgets module.")
    endif ()
endif ()

add_library(webview STATIC ${webview_SOURCES} ${webview_HEADERS})

target_compile_definitions(webview PRIVATE
    QT_PLUGIN
    QT_STATICPLUGIN
)

target_link_libraries(webview PUBLIC
          Qt${QT_VERSION_MAJOR}::Core
          Qt${QT_VERSION_MAJOR}::Quick
      )

# Widgets and WebEngineWidgets are only available on desktop platforms
if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    if(TARGET Qt${QT_VERSION_MAJOR}::Widgets)
        target_link_libraries(webview PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
    endif()
    
    if (Qt6WebEngineWidgets_FOUND)
        target_link_libraries(webview PRIVATE
            Qt${QT_VERSION_MAJOR}::WebEngineWidgets
        )
    endif ()
endif()

# Link WebKit framework for iOS
if (CMAKE_CROSSCOMPILING AND APPLE)
    find_library(FW_WEBKIT WebKit)
    if(FW_WEBKIT)
        target_link_libraries(webview PRIVATE ${FW_WEBKIT})
    endif()
endif()

set_target_properties(webview PROPERTIES AUTOMOC_MOC_OPTIONS "-Muri=${webview_URI}")

#include(precompiled.headers)
#add_precompiled_header(webview pch.h FORCEINCLUDE)

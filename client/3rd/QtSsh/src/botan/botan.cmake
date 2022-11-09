include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/external
)

if(WIN32)
    add_compile_definitions(MVPN_WINDOWS)
    add_compile_options(/bigobj)
    set(LIBS ${LIBS}
        crypt32
    )

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        include_directories(${CMAKE_CURRENT_LIST_DIR}/windows/x86_64)
        set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/windows/x86_64/botan_all.h)
        set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/windows/x86_64/botan_all.cpp)
    else()
        include_directories(${CMAKE_CURRENT_LIST_DIR}/windows/x86)
        set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/windows/x86/botan_all.h)
        set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/windows/x86/botan_all.cpp)
    endif()
endif()

#todo add mac
#todo add linux
#todo add android
#todo add ios

include_directories(${CMAKE_CURRENT_LIST_DIR})

#include(${CMAKE_CURRENT_LIST_DIR}/../common.cmake)

if(NOT WIN32)
    set(LIBS ${LIBS} Qt6::Network)
elseif(WIN32)
    set(LIBS ${LIBS} user32)
endif()

set(HEADERS ${HEADERS} 
    ${CMAKE_CURRENT_LIST_DIR}/qtservice.h
    ${CMAKE_CURRENT_LIST_DIR}/qtservice_p.h
)

set(SOURCES ${SOURCES} 
    ${CMAKE_CURRENT_LIST_DIR}/qtservice.cpp
)

if(UNIX)
    set(HEADERS ${HEADERS} 
        ${CMAKE_CURRENT_LIST_DIR}/qtunixsocket.h
        ${CMAKE_CURRENT_LIST_DIR}/qtunixserversocket.h
    )

    set(SOURCES ${SOURCES} 
        ${CMAKE_CURRENT_LIST_DIR}/qtservice_unix.cpp
        ${CMAKE_CURRENT_LIST_DIR}/qtunixsocket.cpp
        ${CMAKE_CURRENT_LIST_DIR}/qtunixserversocket.cpp
    )
endif()

if(WIN32)
    set(SOURCES ${SOURCES} 
        ${CMAKE_CURRENT_LIST_DIR}/qtservice_win.cpp
    )
endif()

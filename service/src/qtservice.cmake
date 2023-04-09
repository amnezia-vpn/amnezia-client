if(${PROJECT} STREQUAL "")
   message(FATAL_ERROR "You must set PROJECT variable")
endif()

target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

if(NOT WIN32)
    target_include_directories(${PROJECT} PRIVATE Qt6::Network)
elseif(WIN32)
    target_include_directories(${PROJECT} PRIVATE user32)
endif()

target_sources(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/qtservice.h
    ${CMAKE_CURRENT_LIST_DIR}/qtservice_p.h

    ${CMAKE_CURRENT_LIST_DIR}/qtservice.cpp
)

if(UNIX)
    target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/qtunixsocket.h
        ${CMAKE_CURRENT_LIST_DIR}/qtunixserversocket.h

        ${CMAKE_CURRENT_LIST_DIR}/qtservice_unix.cpp
        ${CMAKE_CURRENT_LIST_DIR}/qtunixsocket.cpp
        ${CMAKE_CURRENT_LIST_DIR}/qtunixserversocket.cpp
    )
endif()

if(WIN32)
    target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/qtservice_win.cpp
    )
endif()

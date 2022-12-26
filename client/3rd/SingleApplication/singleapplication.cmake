include_directories(${CMAKE_CURRENT_LIST_DIR})

find_package(Qt6 REQUIRED COMPONENTS 
    Core Network
)
set(LIBS ${LIBS} Qt6::Core Qt6::Network)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/singleapplication.h 
    ${CMAKE_CURRENT_LIST_DIR}/singleapplication_p.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/singleapplication.cpp 
    ${CMAKE_CURRENT_LIST_DIR}/singleapplication_p.cpp
)

if(WIN32)
    if(MSVC)
        set(LIBS ${LIBS} Advapi32.lib)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(LIBS ${LIBS} advapi32)
    endif()    
endif()

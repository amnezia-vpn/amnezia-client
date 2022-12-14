set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

include(${CLIENT_ROOT_DIR}/3rd/QtSsh/src/ssh/qssh.cmake)

include(${CLIENT_ROOT_DIR}/3rd/QtSsh/src/botan/botan.cmake)
if(NOT IOS AND NOT ANDROID)
   include(${CLIENT_ROOT_DIR}/3rd/SingleApplication/singleapplication.cmake)
endif()

add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} SortFilterProxyModel)

include(${CLIENT_ROOT_DIR}/3rd/qrcodegen/qrcodegen.cmake)
include(${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/QSimpleCrypto.cmake)

set(BUILD_WITH_QT6 ON)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/qtkeychain)
set(LIBS ${LIBS} qt6keychain)

include_directories(
    ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/include
    ${CLIENT_ROOT_DIR}/3rd/OpenSSL/include
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
)

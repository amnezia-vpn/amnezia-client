include_directories(${CMAKE_CURRENT_LIST_DIR})

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/include/QAead.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QBlockCipher.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QCryptoError.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QRsa.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QSimpleCrypto_global.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QX509.h
    ${CMAKE_CURRENT_LIST_DIR}/include/QX509Store.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/sources/QAead.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sources/QBlockCipher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sources/QCryptoError.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sources/QRsa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sources/QX509.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sources/QX509Store.cpp
)

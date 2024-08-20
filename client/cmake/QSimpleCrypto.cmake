set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(QSIMPLECRYPTO_DIR ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/src)

include_directories(${QSIMPLECRYPTO_DIR})

set(HEADERS ${HEADERS}
    ${QSIMPLECRYPTO_DIR}/include/QAead.h
    ${QSIMPLECRYPTO_DIR}/include/QBlockCipher.h
    ${QSIMPLECRYPTO_DIR}/include/QRsa.h
    ${QSIMPLECRYPTO_DIR}/include/QSimpleCrypto_global.h
    ${QSIMPLECRYPTO_DIR}/include/QX509.h
    ${QSIMPLECRYPTO_DIR}/include/QX509Store.h
)

set(SOURCES ${SOURCES}
    ${QSIMPLECRYPTO_DIR}/sources/QAead.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QBlockCipher.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QRsa.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509Store.cpp
)

get_filename_component(_CLIENT_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

include(${CMAKE_CURRENT_LIST_DIR}/branding/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/branding/apple.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/branding/ios.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/branding/macos.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/branding/android.cmake)

unset(_CLIENT_SRC_DIR)

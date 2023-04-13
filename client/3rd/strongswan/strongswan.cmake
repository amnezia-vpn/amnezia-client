include(ExternalProject)

set(STRONGSWAN_ROOT ${CMAKE_CURRENT_LIST_DIR}/sources)

ExternalProject_Add(
    strongswan
    UPDATE_DISCONNECTED true
    CONFIGURE_HANDLED_BY_BUILD true
    PREFIX ${STRONGSWAN_ROOT}
    SOURCE_DIR ${STRONGSWAN_ROOT}
    BINARY_DIR ${STRONGSWAN_ROOT}
    INSTALL_DIR ${STRONGSWAN_ROOT}
    CONFIGURE_COMMAND ./autogen.sh
    COMMAND ./configure --disable-kernel-netlink
    BUILD_COMMAND make #dist
    INSTALL_COMMAND ""
)

#add_custom_target(strongswan
#    DEPENDS ${PROJECT}
#)

#add_custom_command(TARGET strongswan
#  COMMAND ./autogen.sh
#  COMMAND ./configure --disable-kernel-netlink
#  #WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/foo"
#  #DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libfoo/foo.tar"
#  COMMENT "*** autogen"
#  VERBATIM
#)

#execute_process(
#    COMMAND ./autogen.sh
#    RESULT_VARIALBE autogen_var
#)

#file(WRITE "autogen_out" "${autogen_var}")

#execute_process(
#    COMMAND ./configure --disable-kernel-netlink
#    RESULT_VARIALBE config_var
#)

#file(WRITE "config_out" "${config_var}")


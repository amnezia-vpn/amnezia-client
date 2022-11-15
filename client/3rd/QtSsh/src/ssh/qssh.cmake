include_directories(${CMAKE_CURRENT_LIST_DIR})

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets Gui Network Core5Compat
)
set(LIBS ${LIBS} Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Core5Compat)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/sshsendfacility.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshremoteprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshpacketparser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshpacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshoutgoingpacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshkeygenerator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshkeyexchange.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshincomingpacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshcryptofacility.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshconnection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshchannelmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshchannel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshcapabilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftppacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpoutgoingpacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpoperation.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpincomingpacket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpdefs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpchannel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshremoteprocessrunner.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshconnectionmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshkeypasswordretriever.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sftpfilesystemmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshdirecttcpiptunnel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshhostkeydatabase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshlogging.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshtcpipforwardserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshtcpiptunnel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshforwardedtcpiptunnel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshagent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshx11channel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sshx11inforetriever.cpp
    ${CMAKE_CURRENT_LIST_DIR}/opensshkeyfilereader.cpp
)

set(PUBLIC_HEADERS ${PUBLIC_HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/sftpdefs.h
    ${CMAKE_CURRENT_LIST_DIR}/ssherrors.h
    ${CMAKE_CURRENT_LIST_DIR}/sshremoteprocess.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpchannel.h
    ${CMAKE_CURRENT_LIST_DIR}/sshkeygenerator.h
    ${CMAKE_CURRENT_LIST_DIR}/sshremoteprocessrunner.h
    ${CMAKE_CURRENT_LIST_DIR}/sshconnectionmanager.h
    ${CMAKE_CURRENT_LIST_DIR}/sshpseudoterminal.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpfilesystemmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/sshdirecttcpiptunnel.h
    ${CMAKE_CURRENT_LIST_DIR}/sshtcpipforwardserver.h
    ${CMAKE_CURRENT_LIST_DIR}/sshhostkeydatabase.h
    ${CMAKE_CURRENT_LIST_DIR}/sshforwardedtcpiptunnel.h
    ${CMAKE_CURRENT_LIST_DIR}/ssh_global.h
    ${CMAKE_CURRENT_LIST_DIR}/sshconnection.h
)

set(HEADERS ${HEADERS} 
    ${PUBLIC_HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/sshsendfacility_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshremoteprocess_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshpacketparser_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshpacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshoutgoingpacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshkeyexchange_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshincomingpacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshexception_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshcryptofacility_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshconnection_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshchannelmanager_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshchannel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshcapabilities_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshbotanconversions_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sftppacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpoutgoingpacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpoperation_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpincomingpacket_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sftpchannel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshkeypasswordretriever_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshdirecttcpiptunnel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshlogging_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshtcpipforwardserver_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshtcpiptunnel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshforwardedtcpiptunnel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshagent_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshx11channel_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshx11displayinfo_p.h
    ${CMAKE_CURRENT_LIST_DIR}/sshx11inforetriever_p.h
    ${CMAKE_CURRENT_LIST_DIR}/opensshkeyfilereader_p.h
)

# qt6_add_resources(QRC ${QRC} ${CMAKE_CURRENT_LIST_DIR}/qssh.qrc)
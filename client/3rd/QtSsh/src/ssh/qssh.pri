QT += gui network widgets

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

!win32-msvc* {
    QMAKE_CXXFLAGS += -Wextra -pedantic
}

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

SOURCES = $$PWD/sshsendfacility.cpp \
    $$PWD/sshremoteprocess.cpp \
    $$PWD/sshpacketparser.cpp \
    $$PWD/sshpacket.cpp \
    $$PWD/sshoutgoingpacket.cpp \
    $$PWD/sshkeygenerator.cpp \
    $$PWD/sshkeyexchange.cpp \
    $$PWD/sshincomingpacket.cpp \
    $$PWD/sshcryptofacility.cpp \
    $$PWD/sshconnection.cpp \
    $$PWD/sshchannelmanager.cpp \
    $$PWD/sshchannel.cpp \
    $$PWD/sshcapabilities.cpp \
    $$PWD/sftppacket.cpp \
    $$PWD/sftpoutgoingpacket.cpp \
    $$PWD/sftpoperation.cpp \
    $$PWD/sftpincomingpacket.cpp \
    $$PWD/sftpdefs.cpp \
    $$PWD/sftpchannel.cpp \
    $$PWD/sshremoteprocessrunner.cpp \
    $$PWD/sshconnectionmanager.cpp \
    $$PWD/sshkeypasswordretriever.cpp \
    $$PWD/sftpfilesystemmodel.cpp \
    $$PWD/sshdirecttcpiptunnel.cpp \
    $$PWD/sshhostkeydatabase.cpp \
    $$PWD/sshlogging.cpp \
    $$PWD/sshtcpipforwardserver.cpp \
    $$PWD/sshtcpiptunnel.cpp \
    $$PWD/sshforwardedtcpiptunnel.cpp \
    $$PWD/sshagent.cpp \
    $$PWD/sshx11channel.cpp \
    $$PWD/sshx11inforetriever.cpp \
    $$PWD/opensshkeyfilereader.cpp \

PUBLIC_HEADERS = \
    $$PWD/sftpdefs.h \
    $$PWD/ssherrors.h \
    $$PWD/sshremoteprocess.h \
    $$PWD/sftpchannel.h \
    $$PWD/sshkeygenerator.h \
    $$PWD/sshremoteprocessrunner.h \
    $$PWD/sshconnectionmanager.h \
    $$PWD/sshpseudoterminal.h \
    $$PWD/sftpfilesystemmodel.h \
    $$PWD/sshdirecttcpiptunnel.h \
    $$PWD/sshtcpipforwardserver.h \
    $$PWD/sshhostkeydatabase.h \
    $$PWD/sshforwardedtcpiptunnel.h \
    $$PWD/ssh_global.h \
    $$PWD/sshconnection.h \

HEADERS = $$PUBLIC_HEADERS \
    $$PWD/sshsendfacility_p.h \
    $$PWD/sshremoteprocess_p.h \
    $$PWD/sshpacketparser_p.h \
    $$PWD/sshpacket_p.h \
    $$PWD/sshoutgoingpacket_p.h \
    $$PWD/sshkeyexchange_p.h \
    $$PWD/sshincomingpacket_p.h \
    $$PWD/sshexception_p.h \
    $$PWD/sshcryptofacility_p.h \
    $$PWD/sshconnection_p.h \
    $$PWD/sshchannelmanager_p.h \
    $$PWD/sshchannel_p.h \
    $$PWD/sshcapabilities_p.h \
    $$PWD/sshbotanconversions_p.h \
    $$PWD/sftppacket_p.h \
    $$PWD/sftpoutgoingpacket_p.h \
    $$PWD/sftpoperation_p.h \
    $$PWD/sftpincomingpacket_p.h \
    $$PWD/sftpchannel_p.h \
    $$PWD/sshkeypasswordretriever_p.h \
    $$PWD/sshdirecttcpiptunnel_p.h \
    $$PWD/sshlogging_p.h \
    $$PWD/sshtcpipforwardserver_p.h \
    $$PWD/sshtcpiptunnel_p.h \
    $$PWD/sshforwardedtcpiptunnel_p.h \
    $$PWD/sshagent_p.h \
    $$PWD/sshx11channel_p.h \
    $$PWD/sshx11displayinfo_p.h \
    $$PWD/sshx11inforetriever_p.h \
    $$PWD/opensshkeyfilereader_p.h \

RESOURCES += $$PWD/qssh.qrc



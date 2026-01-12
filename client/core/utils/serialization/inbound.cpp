#include <QString>
#include <QJsonObject>
#include <QList>
#include "3rd/QJsonStruct/QJsonIO.hpp"
#include "transfer.h"
#include "serialization.h"

namespace amnezia::serialization::inbounds
{

//"inbounds": [
//                 {
//                     "listen": "127.0.0.1",
//                     "port": 10808,
//                     "protocol": "socks",
//                     "settings": {
//                         "udp": true
//                     }
//                 }
//],

const static QString listen = "127.0.0.1";
const static int port = 10808;
const static QString protocol = "socks";

QJsonObject GenerateInboundEntry()
{
    QJsonObject root;
    QJsonIO::SetValue(root, listen, "listen");
    QJsonIO::SetValue(root, port, "port");
    QJsonIO::SetValue(root, protocol, "protocol");
    QJsonIO::SetValue(root, true, "settings", "udp");
    return root;
}


} // namespace amnezia::serialization::inbounds


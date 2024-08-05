#ifndef TRANSFER_H
#define TRANSFER_H

#include "3rd/QJsonStruct/QJsonIO.hpp"
#include "3rd/QJsonStruct/QJsonStruct.hpp"

#define JADDEx(field) root.insert(#field, field);
#define JADD(...) FOR_EACH(JADDEx, __VA_ARGS__)

constexpr auto VMESS_USER_ALTERID_DEFAULT = 0;

namespace amnezia::serialization {

struct ShadowSocksServerObject
{
    QString address;
    QString method;
    QString password;
    int port;
    JSONSTRUCT_COMPARE(ShadowSocksServerObject, address, method, password)
    JSONSTRUCT_REGISTER(ShadowSocksServerObject, F(address, port, method, password))
};


struct VMessServerObject
{
    struct UserObject
    {
        QString id;
        int alterId = VMESS_USER_ALTERID_DEFAULT;
        QString security = "auto";
        int level = 0;
        JSONSTRUCT_COMPARE(UserObject, id, alterId, security, level)
        JSONSTRUCT_REGISTER(UserObject, F(id, alterId, security, level))
    };

    QString address;
    int port;
    QList<UserObject> users;
    JSONSTRUCT_COMPARE(VMessServerObject, address, port, users)
    JSONSTRUCT_REGISTER(VMessServerObject, F(address, port, users))
};


namespace transfer
{

struct HTTPRequestObject
{
    QString version = "1.1";
    QString method = "GET";
    QList<QString> path = { "/" };
    QMap<QString, QList<QString>> headers;
    HTTPRequestObject()
    {
        headers = {
            { "Host", { "www.baidu.com", "www.bing.com" } },
            { "User-Agent",
             { "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36",
              "Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/601.1 (KHTML, like Gecko) CriOS/53.0.2785.109 Mobile/14A456 Safari/601.1.46" } },
            { "Accept-Encoding", { "gzip, deflate" } },
            { "Connection", { "keep-alive" } },
            { "Pragma", { "no-cache" } }
        };
    }
    JSONSTRUCT_COMPARE(HTTPRequestObject, version, method, path, headers)
    JSONSTRUCT_REGISTER(HTTPRequestObject, F(version, method, path, headers))
};
//
//
struct HTTPResponseObject
{
    QString version = "1.1";
    QString status = "200";
    QString reason = "OK";
    QMap<QString, QList<QString>> headers;
    HTTPResponseObject()
    {
        headers = { { "Content-Type", { "application/octet-stream", "video/mpeg" } }, //
                   { "Transfer-Encoding", { "chunked" } },                           //
                   { "Connection", { "keep-alive" } },                               //
                   { "Pragma", { "no-cache" } } };
    }
    JSONSTRUCT_COMPARE(HTTPResponseObject, version, status, reason, headers)
    JSONSTRUCT_REGISTER(HTTPResponseObject, F(version, status, reason, headers))
};
//
//
struct TCPHeader_Internal
{
    QString type = "none";
    HTTPRequestObject request;
    HTTPResponseObject response;
    JSONSTRUCT_COMPARE(TCPHeader_Internal, type, request, response)
    JSONSTRUCT_REGISTER(TCPHeader_Internal, A(type), F(request, response))
};
//
//
struct ObfsHeaderObject
{
    QString type = "none";
    JSONSTRUCT_COMPARE(ObfsHeaderObject, type)
    JSONSTRUCT_REGISTER(ObfsHeaderObject, F(type))
};
//
//
struct TCPObject
{
    TCPHeader_Internal header;
    JSONSTRUCT_COMPARE(TCPObject, header)
    JSONSTRUCT_REGISTER(TCPObject, F(header))
};
//
//
struct KCPObject
{
    int mtu = 1350;
    int tti = 50;
    int uplinkCapacity = 5;
    int downlinkCapacity = 20;
    bool congestion = false;
    int readBufferSize = 2;
    int writeBufferSize = 2;
    QString seed;
    ObfsHeaderObject header;
    KCPObject(){};
    JSONSTRUCT_COMPARE(KCPObject, mtu, tti, uplinkCapacity, downlinkCapacity, congestion, readBufferSize, writeBufferSize, seed, header)
    JSONSTRUCT_REGISTER(KCPObject, F(mtu, tti, uplinkCapacity, downlinkCapacity, congestion, readBufferSize, writeBufferSize, header, seed))
};
//
//
struct WebSocketObject
{
    QString path = "/";
    QMap<QString, QString> headers;
    int maxEarlyData = 0;
    bool useBrowserForwarding = false;
    QString earlyDataHeaderName;
    JSONSTRUCT_COMPARE(WebSocketObject, path, headers, maxEarlyData, useBrowserForwarding, earlyDataHeaderName)
    JSONSTRUCT_REGISTER(WebSocketObject, F(path, headers, maxEarlyData, useBrowserForwarding, earlyDataHeaderName))
};
//
//
struct HttpObject
{
    QList<QString> host;
    QString path = "/";
    QString method = "PUT";
    QMap<QString, QList<QString>> headers;
    JSONSTRUCT_COMPARE(HttpObject, host, path, method, headers)
    JSONSTRUCT_REGISTER(HttpObject, F(host, path, method, headers))
};
//
//
struct DomainSocketObject
{
    QString path = "/";
    JSONSTRUCT_COMPARE(DomainSocketObject, path)
    JSONSTRUCT_REGISTER(DomainSocketObject, F(path))
};
//
//
struct QuicObject
{
    QString security = "none";
    QString key;
    ObfsHeaderObject header;
    JSONSTRUCT_COMPARE(QuicObject, security, key, header)
    JSONSTRUCT_REGISTER(QuicObject, F(security, key, header))
};
//
//
struct gRPCObject
{
    QString serviceName;
    bool multiMode = false;
    JSONSTRUCT_COMPARE(gRPCObject, serviceName, multiMode)
    JSONSTRUCT_REGISTER(gRPCObject, F(serviceName, multiMode))
};

//
//
struct SockoptObject
{
    int mark = 0;
    bool tcpFastOpen = false;
    QString tproxy = "off";
    int tcpKeepAliveInterval = 0;
    JSONSTRUCT_COMPARE(SockoptObject, mark, tcpFastOpen, tproxy, tcpKeepAliveInterval)
    JSONSTRUCT_REGISTER(SockoptObject, F(mark, tcpFastOpen, tproxy, tcpKeepAliveInterval))
};
//
//
struct CertificateObject
{
    QString usage = "encipherment";
    QString certificateFile;
    QString keyFile;
    QList<QString> certificate;
    QList<QString> key;
    JSONSTRUCT_COMPARE(CertificateObject, usage, certificateFile, keyFile, certificate, key)
    JSONSTRUCT_REGISTER(CertificateObject, F(usage, certificateFile, keyFile, certificate, key))
};
//
//
struct TLSObject
{
    QString serverName;
    bool allowInsecure = false;
    bool enableSessionResumption = false;
    bool disableSystemRoot = false;
    QList<QString> alpn;
    QList<QString> pinnedPeerCertificateChainSha256;
    QList<CertificateObject> certificates;
    JSONSTRUCT_COMPARE(TLSObject, serverName, allowInsecure, enableSessionResumption, disableSystemRoot, alpn,
                       pinnedPeerCertificateChainSha256, certificates)
    JSONSTRUCT_REGISTER(TLSObject, F(serverName, allowInsecure, enableSessionResumption, disableSystemRoot, alpn,
                                     pinnedPeerCertificateChainSha256, certificates))
};
//
//
struct XTLSObject
{
    QString serverName;
    bool allowInsecure = false;
    bool enableSessionResumption = false;
    bool disableSystemRoot = false;
    QList<QString> alpn;
    QList<CertificateObject> certificates;
    JSONSTRUCT_COMPARE(XTLSObject, serverName, allowInsecure, enableSessionResumption, disableSystemRoot, alpn, certificates)
    JSONSTRUCT_REGISTER(XTLSObject, F(serverName, allowInsecure, enableSessionResumption, disableSystemRoot, alpn, certificates))
};
} // namespace transfer

//
//
struct TrojanObject
{
    quint16 port;
    QString address;
    QString password;
    QString sni;
    bool ignoreCertificate = false;
    bool ignoreHostname = false;
    bool reuseSession = false;
    bool sessionTicket = false;
    bool reusePort = false;
    bool tcpFastOpen = false;

#define _X(name) json[#name] = name
    QJsonObject toJson() const
    {
        QJsonObject json;
        _X(port);
        _X(address);
        _X(password);
        _X(sni);
        _X(ignoreCertificate);
        _X(ignoreHostname);
        _X(reuseSession);
        _X(reusePort);
        _X(sessionTicket);
        _X(tcpFastOpen);
        return json;
    };
#undef _X

#define _X(name, type) name = root[#name].to##type()
    void loadJson(const QJsonObject &root)
    {
        _X(port, Int);
        _X(address, String);
        _X(password, String);
        _X(sni, String);
        _X(ignoreHostname, Bool);
        _X(ignoreCertificate, Bool);
        _X(reuseSession, Bool);
        _X(reusePort, Bool);
        _X(sessionTicket, Bool);
        _X(tcpFastOpen, Bool);
    }
#undef _X

    [[nodiscard]] static TrojanObject fromJson(const QJsonObject &root)
    {
        TrojanObject o;
        o.loadJson(root);
        return o;
    }
};

struct StreamSettingsObject
{
    QString network = "tcp";
    QString security = "none";
    transfer::SockoptObject sockopt;
    transfer::TLSObject tlsSettings;
    transfer::XTLSObject xtlsSettings;
    transfer::TCPObject tcpSettings;
    transfer::KCPObject kcpSettings;
    transfer::WebSocketObject wsSettings;
    transfer::HttpObject httpSettings;
    transfer::DomainSocketObject dsSettings;
    transfer::QuicObject quicSettings;
    transfer::gRPCObject grpcSettings;
    JSONSTRUCT_COMPARE(StreamSettingsObject, network, security, sockopt, //
                       tcpSettings, tlsSettings, xtlsSettings, kcpSettings, wsSettings, httpSettings, dsSettings, quicSettings, grpcSettings)
    JSONSTRUCT_REGISTER(StreamSettingsObject, F(network, security, sockopt),
                        F(tcpSettings, tlsSettings, xtlsSettings, kcpSettings, wsSettings, httpSettings, dsSettings, quicSettings, grpcSettings))
};

}
#endif //TRANSFER_H

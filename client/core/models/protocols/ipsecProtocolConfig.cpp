#include "ipsecProtocolConfig.h"

#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

IpsecProtocolConfig::IpsecProtocolConfig(const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.l2tpNet = "192.168.42.0/24";
    serverProtocolConfig.l2tpPool = "192.168.42.10-192.168.42.250";
    serverProtocolConfig.l2tpLocal = "192.168.42.1";
    serverProtocolConfig.xauthNet = "192.168.43.0/24";
    serverProtocolConfig.xauthPool = "192.168.43.10-192.168.43.250";
    serverProtocolConfig.sha2TruncBug = "yes";
    serverProtocolConfig.androidMtuFix = "yes";
    serverProtocolConfig.disableIkev2 = "no";
    serverProtocolConfig.disableL2tp = "no";
    serverProtocolConfig.disableXauth = "no";
    serverProtocolConfig.c2cTraffic = "no";
}

IpsecProtocolConfig::IpsecProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.l2tpNet = protocolConfigObject.value("l2tp_net").toString("192.168.42.0/24");
    serverProtocolConfig.l2tpPool = protocolConfigObject.value("l2tp_pool").toString("192.168.42.10-192.168.42.250");
    serverProtocolConfig.l2tpLocal = protocolConfigObject.value("l2tp_local").toString("192.168.42.1");
    serverProtocolConfig.xauthNet = protocolConfigObject.value("xauth_net").toString("192.168.43.0/24");
    serverProtocolConfig.xauthPool = protocolConfigObject.value("xauth_pool").toString("192.168.43.10-192.168.43.250");
    serverProtocolConfig.sha2TruncBug = protocolConfigObject.value("sha2_truncbug").toString("yes");
    serverProtocolConfig.androidMtuFix = protocolConfigObject.value("android_mtu_fix").toString("yes");
    serverProtocolConfig.disableIkev2 = protocolConfigObject.value("disable_ikev2").toString("no");
    serverProtocolConfig.disableL2tp = protocolConfigObject.value("disable_l2tp").toString("no");
    serverProtocolConfig.disableXauth = protocolConfigObject.value("disable_xauth").toString("no");
    serverProtocolConfig.c2cTraffic = protocolConfigObject.value("c2c_traffic").toString("no");

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;
        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

IpsecProtocolConfig::IpsecProtocolConfig(const IpsecProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
}

QJsonObject IpsecProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.l2tpNet.isEmpty()) {
        json["l2tp_net"] = serverProtocolConfig.l2tpNet;
    }
    if (!serverProtocolConfig.l2tpPool.isEmpty()) {
        json["l2tp_pool"] = serverProtocolConfig.l2tpPool;
    }
    if (!serverProtocolConfig.l2tpLocal.isEmpty()) {
        json["l2tp_local"] = serverProtocolConfig.l2tpLocal;
    }
    if (!serverProtocolConfig.xauthNet.isEmpty()) {
        json["xauth_net"] = serverProtocolConfig.xauthNet;
    }
    if (!serverProtocolConfig.xauthPool.isEmpty()) {
        json["xauth_pool"] = serverProtocolConfig.xauthPool;
    }
    if (!serverProtocolConfig.sha2TruncBug.isEmpty()) {
        json["sha2_truncbug"] = serverProtocolConfig.sha2TruncBug;
    }
    if (!serverProtocolConfig.androidMtuFix.isEmpty()) {
        json["android_mtu_fix"] = serverProtocolConfig.androidMtuFix;
    }
    if (!serverProtocolConfig.disableIkev2.isEmpty()) {
        json["disable_ikev2"] = serverProtocolConfig.disableIkev2;
    }
    if (!serverProtocolConfig.disableL2tp.isEmpty()) {
        json["disable_l2tp"] = serverProtocolConfig.disableL2tp;
    }
    if (!serverProtocolConfig.disableXauth.isEmpty()) {
        json["disable_xauth"] = serverProtocolConfig.disableXauth;
    }
    if (!serverProtocolConfig.c2cTraffic.isEmpty()) {
        json["c2c_traffic"] = serverProtocolConfig.c2cTraffic;
    }

    return json;
}

ScriptVars IpsecProtocolConfig::getScriptVars() const
{
    ScriptVars vars;

    vars.append({ "$IPSEC_VPN_L2TP_NET", serverProtocolConfig.l2tpNet });
    vars.append({ "$IPSEC_VPN_L2TP_POOL", serverProtocolConfig.l2tpPool });
    vars.append({ "$IPSEC_VPN_L2TP_LOCAL", serverProtocolConfig.l2tpLocal });
    vars.append({ "$IPSEC_VPN_XAUTH_NET", serverProtocolConfig.xauthNet });
    vars.append({ "$IPSEC_VPN_XAUTH_POOL", serverProtocolConfig.xauthPool });
    vars.append({ "$IPSEC_VPN_SHA2_TRUNCBUG", serverProtocolConfig.sha2TruncBug });
    vars.append({ "$IPSEC_VPN_VPN_ANDROID_MTU_FIX", serverProtocolConfig.androidMtuFix });
    vars.append({ "$IPSEC_VPN_DISABLE_IKEV2", serverProtocolConfig.disableIkev2 });
    vars.append({ "$IPSEC_VPN_DISABLE_L2TP", serverProtocolConfig.disableL2tp });
    vars.append({ "$IPSEC_VPN_DISABLE_XAUTH", serverProtocolConfig.disableXauth });
    vars.append({ "$IPSEC_VPN_C2C_TRAFFIC", serverProtocolConfig.c2cTraffic });

    return vars;
}

bool IpsecProtocolConfig::hasEqualServerSettings(const IpsecProtocolConfig &other) const
{
    if (serverProtocolConfig.l2tpNet != other.serverProtocolConfig.l2tpNet
        || serverProtocolConfig.l2tpPool != other.serverProtocolConfig.l2tpPool
        || serverProtocolConfig.l2tpLocal != other.serverProtocolConfig.l2tpLocal
        || serverProtocolConfig.xauthNet != other.serverProtocolConfig.xauthNet
        || serverProtocolConfig.xauthPool != other.serverProtocolConfig.xauthPool
        || serverProtocolConfig.sha2TruncBug != other.serverProtocolConfig.sha2TruncBug
        || serverProtocolConfig.androidMtuFix != other.serverProtocolConfig.androidMtuFix
        || serverProtocolConfig.disableIkev2 != other.serverProtocolConfig.disableIkev2
        || serverProtocolConfig.disableL2tp != other.serverProtocolConfig.disableL2tp
        || serverProtocolConfig.disableXauth != other.serverProtocolConfig.disableXauth
        || serverProtocolConfig.c2cTraffic != other.serverProtocolConfig.c2cTraffic) {
        return false;
    }
    return true;
}

void IpsecProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = ipsec::ClientProtocolConfig();
}

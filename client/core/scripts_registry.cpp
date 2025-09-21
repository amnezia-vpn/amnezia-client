#include "scripts_registry.h"

#include <QDebug>
#include <QFile>
#include <QObject>

QString amnezia::scriptFolder(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn: return QLatin1String("openvpn");
    case DockerContainer::Cloak: return QLatin1String("openvpn_cloak");
    case DockerContainer::ShadowSocks: return QLatin1String("openvpn_shadowsocks");
    case DockerContainer::WireGuard: return QLatin1String("wireguard");
    case DockerContainer::Awg: return QLatin1String("awg");
    case DockerContainer::Ipsec: return QLatin1String("ipsec");
    case DockerContainer::Xray: return QLatin1String("xray");

    case DockerContainer::TorWebSite: return QLatin1String("website_tor");
    case DockerContainer::Dns: return QLatin1String("dns");
    case DockerContainer::Sftp: return QLatin1String("sftp");
    case DockerContainer::Socks5Proxy: return QLatin1String("socks5_proxy");
    default: return QString();
    }
}

QString amnezia::scriptName(SharedScriptType type)
{
    switch (type) {
    case SharedScriptType::prepare_host: return QLatin1String("prepare_host.sh");
    case SharedScriptType::install_docker: return QLatin1String("install_docker.sh");
    case SharedScriptType::build_container: return QLatin1String("build_container.sh");
    case SharedScriptType::remove_container: return QLatin1String("remove_container.sh");
    case SharedScriptType::remove_all_containers: return QLatin1String("remove_all_containers.sh");
    case SharedScriptType::setup_host_firewall: return QLatin1String("setup_host_firewall.sh");
    case SharedScriptType::check_connection: return QLatin1String("check_connection.sh");
    case SharedScriptType::check_server_is_busy: return QLatin1String("check_server_is_busy.sh");
    case SharedScriptType::check_user_in_sudo: return QLatin1String("check_user_in_sudo.sh");
    default: return QString();
    }
}

QString amnezia::scriptName(ProtocolScriptType type)
{
    switch (type) {
    case ProtocolScriptType::dockerfile: return QLatin1String("Dockerfile");
    case ProtocolScriptType::run_container: return QLatin1String("run_container.sh");
    case ProtocolScriptType::configure_container: return QLatin1String("configure_container.sh");
    case ProtocolScriptType::container_startup: return QLatin1String("start.sh");
    case ProtocolScriptType::openvpn_template: return QLatin1String("template.ovpn");
    case ProtocolScriptType::wireguard_template: return QLatin1String("template.conf");
    case ProtocolScriptType::awg_template: return QLatin1String("template.conf");
    case ProtocolScriptType::xray_template: return QLatin1String("template.json");
    case ProtocolScriptType::ipsec_template: return QLatin1String("template.conf");
    default: return QString();
    }
}

QString amnezia::scriptData(amnezia::SharedScriptType type)
{
    QString fileName = QString(":/server_scripts/%1").arg(amnezia::scriptName(type));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Warning: script missing" << fileName;
        return "";
    }
    QByteArray ba = file.readAll();
    if (ba.isEmpty()) {
        qDebug() << "Warning: script is empty" << fileName;
    }
    return ba;
}

QString amnezia::scriptData(amnezia::ProtocolScriptType type, DockerContainer container)
{
    QString fileName = QString(":/server_scripts/%1/%2").arg(amnezia::scriptFolder(container), amnezia::scriptName(type));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Warning: script missing" << fileName;
        return "";
    }
    QByteArray data = file.readAll();
    data.replace("\r", "");
    return data;
}

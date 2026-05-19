#include "telemtConfigModel.h"

#include <QRegularExpression>

#include "core/utils/qrCodeUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "qrcodegen.hpp"

using namespace amnezia;

TelemtConfigModel::TelemtConfigModel(QObject *parent) : QAbstractListModel(parent) {}

void TelemtConfigModel::applyDefaults(TelemtProtocolConfig &c) {
    if (c.port.isEmpty()) {
        c.port = QString::fromUtf8(protocols::telemt::defaultPort);
    }
    if (c.transportMode.isEmpty()) {
        c.transportMode = QString::fromUtf8(protocols::telemt::transportModeStandard);
    }
    if (c.workersMode.isEmpty()) {
        c.workersMode = QString::fromUtf8(protocols::telemt::workersModeAuto);
    }
    if (c.workers.isEmpty()) {
        c.workers = QString::fromUtf8(protocols::telemt::defaultWorkers);
    }
    if (c.userName.isEmpty()) {
        c.userName = QString::fromUtf8(protocols::telemt::defaultUserName);
    }
}

int TelemtConfigModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 1;
}

bool TelemtConfigModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() != 0) {
        return false;
    }

    switch (role) {
        case Roles::PortRole: {
            m_protocolConfig.port = value.toString();
            break;
        }
        case Roles::SecretRole: {
            m_protocolConfig.secret = value.toString();
            break;
        }
        case Roles::TagRole: {
            m_protocolConfig.tag = value.toString();
            break;
        }
        case Roles::IsEnabledRole: {
            m_protocolConfig.isEnabled = value.toBool();
            break;
        }
        case Roles::PublicHostRole: {
            m_protocolConfig.publicHost = value.toString();
            break;
        }
        case Roles::TransportModeRole: {
            m_protocolConfig.transportMode = value.toString();
            break;
        }
        case Roles::TlsDomainRole: {
            m_protocolConfig.tlsDomain = value.toString();
            break;
        }
        case Roles::AdditionalSecretsRole: {
            m_protocolConfig.additionalSecrets = value.toStringList();
            break;
        }
        case Roles::WorkersModeRole: {
            m_protocolConfig.workersMode = value.toString();
            break;
        }
        case Roles::WorkersRole: {
            m_protocolConfig.workers = value.toString();
            break;
        }
        case Roles::NatEnabledRole: {
            m_protocolConfig.natEnabled = value.toBool();
            break;
        }
        case Roles::NatInternalIpRole: {
            m_protocolConfig.natInternalIp = value.toString();
            break;
        }
        case Roles::NatExternalIpRole: {
            m_protocolConfig.natExternalIp = value.toString();
            break;
        }
        case Roles::MaskEnabledRole: {
            m_protocolConfig.maskEnabled = value.toBool();
            break;
        }
        case Roles::UseMiddleProxyRole: {
            m_protocolConfig.useMiddleProxy = value.toBool();
            break;
        }
        case Roles::TlsEmulationRole: {
            m_protocolConfig.tlsEmulation = value.toBool();
            break;
        }
        case Roles::UserNameRole: {
            m_protocolConfig.userName = value.toString();
            break;
        }
        default: {
            return false;
        }
    }

    emit dataChanged(index, index, QList{role});
    return true;
}

QVariant TelemtConfigModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() != 0) {
        return QVariant();
    }

    switch (role) {
        case Roles::PortRole: {
            return m_protocolConfig.port.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultPort)
                                                   : m_protocolConfig.port;
        }
        case Roles::SecretRole: {
            return m_protocolConfig.secret;
        }
        case Roles::TagRole: {
            return m_protocolConfig.tag;
        }
        case Roles::TgLinkRole: {
            return m_protocolConfig.tgLink;
        }
        case Roles::TmeLinkRole: {
            return m_protocolConfig.tmeLink;
        }
        case Roles::IsEnabledRole: {
            return m_protocolConfig.isEnabled;
        }
        case Roles::PublicHostRole: {
            return m_protocolConfig.publicHost.isEmpty() ? m_fullConfig.value(QString(configKey::hostName)).toString()
                                                         : m_protocolConfig.publicHost;
        }
        case Roles::TransportModeRole: {
            return m_protocolConfig.transportMode.isEmpty() ? QString::fromUtf8(
                    protocols::telemt::transportModeStandard)
                                                            : m_protocolConfig.transportMode;
        }
        case Roles::TlsDomainRole: {
            return m_protocolConfig.tlsDomain;
        }
        case Roles::AdditionalSecretsRole: {
            return m_protocolConfig.additionalSecrets;
        }
        case Roles::WorkersModeRole: {
            return m_protocolConfig.workersMode.isEmpty() ? QString::fromUtf8(protocols::telemt::workersModeAuto)
                                                          : m_protocolConfig.workersMode;
        }
        case Roles::WorkersRole: {
            return m_protocolConfig.workers.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultWorkers)
                                                      : m_protocolConfig.workers;
        }
        case Roles::NatEnabledRole: {
            return m_protocolConfig.natEnabled;
        }
        case Roles::NatInternalIpRole: {
            return m_protocolConfig.natInternalIp;
        }
        case Roles::NatExternalIpRole: {
            return m_protocolConfig.natExternalIp;
        }
        case Roles::MaskEnabledRole: {
            return m_protocolConfig.maskEnabled;
        }
        case Roles::UseMiddleProxyRole: {
            return m_protocolConfig.useMiddleProxy;
        }
        case Roles::TlsEmulationRole: {
            return m_protocolConfig.tlsEmulation;
        }
        case Roles::UserNameRole: {
            return m_protocolConfig.userName.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultUserName)
                                                       : m_protocolConfig.userName;
        }
    }

    return QVariant();
}

void TelemtConfigModel::updateModel(DockerContainer container, const TelemtProtocolConfig &protocolConfig) {
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaults(m_protocolConfig);
    endResetModel();
}

void TelemtConfigModel::updateModel(const QJsonObject &config) {
    beginResetModel();

    m_fullConfig = config;
    m_protocolConfig = TelemtProtocolConfig::fromJson(config.value(QString(configKey::telemt)).toObject());
    applyDefaults(m_protocolConfig);

    endResetModel();
}

QJsonObject TelemtConfigModel::getConfig() {
    m_fullConfig.insert(QString(configKey::telemt), m_protocolConfig.toJson());
    return m_fullConfig;
}

TelemtProtocolConfig TelemtConfigModel::getProtocolConfig() {
    return m_protocolConfig;
}

void TelemtConfigModel::generateSecret() {
    QString secret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        secret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.secret = secret;
    emit dataChanged(index(0), index(0), QList<int>{SecretRole});
}

void TelemtConfigModel::setSecret(const QString &secret) {
    if (secret.isEmpty()) {
        return;
    }
    setData(index(0), secret, SecretRole);
}

bool TelemtConfigModel::validateAndSetSecret(const QString &rawSecret) {
    if (!QRegularExpression(QStringLiteral("^[0-9a-fA-F]{32}$")).match(rawSecret).hasMatch()) {
        return false;
    }
    setData(index(0), rawSecret, SecretRole);
    return true;
}

void TelemtConfigModel::setPort(const QString &port) {
    setData(index(0), port, PortRole);
}

void TelemtConfigModel::setTag(const QString &tag) {
    setData(index(0), tag, TagRole);
}

void TelemtConfigModel::setPublicHost(const QString &host) {
    setData(index(0), host, PublicHostRole);
}

void TelemtConfigModel::setTransportMode(const QString &mode) {
    setData(index(0), mode, TransportModeRole);
}

QString TelemtConfigModel::getTransportMode() const {
    return m_protocolConfig.transportMode.isEmpty() ? QString::fromUtf8(protocols::telemt::transportModeStandard)
                                                    : m_protocolConfig.transportMode;
}

QString TelemtConfigModel::getTlsDomain() const {
    return m_protocolConfig.tlsDomain.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultTlsDomain)
                                                : m_protocolConfig.tlsDomain;
}

QString TelemtConfigModel::getPublicHost() const {
    return m_protocolConfig.publicHost;
}

void TelemtConfigModel::setTlsDomain(const QString &domain) {
    setData(index(0), domain, TlsDomainRole);
}

void TelemtConfigModel::setWorkersMode(const QString &mode) {
    setData(index(0), mode, WorkersModeRole);
}

void TelemtConfigModel::setWorkers(const QString &workers) {
    setData(index(0), workers, WorkersRole);
}

void TelemtConfigModel::setNatEnabled(bool enabled) {
    setData(index(0), enabled, NatEnabledRole);
}

void TelemtConfigModel::setNatInternalIp(const QString &ip) {
    setData(index(0), ip, NatInternalIpRole);
}

void TelemtConfigModel::setNatExternalIp(const QString &ip) {
    setData(index(0), ip, NatExternalIpRole);
}

void TelemtConfigModel::setMaskEnabled(bool enabled) {
    setData(index(0), enabled, MaskEnabledRole);
}

void TelemtConfigModel::setUseMiddleProxy(bool enabled) {
    setData(index(0), enabled, UseMiddleProxyRole);
}

void TelemtConfigModel::setTlsEmulation(bool enabled) {
    setData(index(0), enabled, TlsEmulationRole);
}

void TelemtConfigModel::setUserName(const QString &name) {
    setData(index(0), name, UserNameRole);
}

void TelemtConfigModel::addAdditionalSecret() {
    QString newSecret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        newSecret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.additionalSecrets.append(newSecret);
    emit dataChanged(index(0), index(0), QList<int>{AdditionalSecretsRole});
}

void TelemtConfigModel::removeAdditionalSecret(int idx) {
    if (idx < 0 || idx >= m_protocolConfig.additionalSecrets.size()) {
        return;
    }
    m_protocolConfig.additionalSecrets.removeAt(idx);
    emit dataChanged(index(0), index(0), QList<int>{AdditionalSecretsRole});
}

void TelemtConfigModel::setEnabled(bool enabled) {
    m_protocolConfig.isEnabled = enabled;
    emit dataChanged(index(0), index(0), QList<int>{IsEnabledRole});
}

QString TelemtConfigModel::generateQrCode(const QString &text) {
    if (text.isEmpty()) {
        return "";
    }
    auto qr = qrCodeUtils::generateQrCode(text.toUtf8());
    return qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));
}

QString TelemtConfigModel::defaultTlsDomain() const {
    return QString::fromUtf8(protocols::telemt::defaultTlsDomain);
}

QString TelemtConfigModel::defaultPort() const {
    return QString::fromUtf8(protocols::telemt::defaultPort);
}

QString TelemtConfigModel::defaultWorkers() const {
    return QString::fromUtf8(protocols::telemt::defaultWorkers);
}

int TelemtConfigModel::maxWorkers() const {
    return protocols::telemt::maxWorkers;
}

QString TelemtConfigModel::transportModeStandard() const {
    return QString::fromUtf8(protocols::telemt::transportModeStandard);
}

QString TelemtConfigModel::transportModeFakeTLS() const {
    return QString::fromUtf8(protocols::telemt::transportModeFakeTLS);
}

QString TelemtConfigModel::workersModeAuto() const {
    return QString::fromUtf8(protocols::telemt::workersModeAuto);
}

QString TelemtConfigModel::workersModeManual() const {
    return QString::fromUtf8(protocols::telemt::workersModeManual);
}

QHash<int, QByteArray> TelemtConfigModel::roleNames() const {
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[SecretRole] = "secret";
    roles[TagRole] = "tag";
    roles[TgLinkRole] = "tgLink";
    roles[TmeLinkRole] = "tmeLink";
    roles[IsEnabledRole] = "isEnabled";
    roles[PublicHostRole] = "publicHost";
    roles[TransportModeRole] = "transportMode";
    roles[TlsDomainRole] = "tlsDomain";
    roles[AdditionalSecretsRole] = "additionalSecrets";
    roles[WorkersModeRole] = "workersMode";
    roles[WorkersRole] = "workers";
    roles[NatEnabledRole] = "natEnabled";
    roles[NatInternalIpRole] = "natInternalIp";
    roles[NatExternalIpRole] = "natExternalIp";
    roles[MaskEnabledRole] = "maskEnabled";
    roles[UseMiddleProxyRole] = "useMiddleProxy";
    roles[TlsEmulationRole] = "tlsEmulation";
    roles[UserNameRole] = "userName";

    return roles;
}

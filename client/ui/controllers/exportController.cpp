#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "qrcodegen.hpp"

#include "core/errorstrings.h"

ExportController::ExportController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ContainersModel> &containersModel,
                                   const std::shared_ptr<Settings> &settings,
                                   const std::shared_ptr<VpnConfigurator> &configurator,
                                   QObject *parent)
    : QObject(parent)
    , m_serversModel(serversModel)
    , m_containersModel(containersModel)
    , m_settings(settings)
    , m_configurator(configurator)
{}

void ExportController::generateFullAccessConfig()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    QJsonObject config = m_settings->server(serverIndex);

    QByteArray compressedConfig = QJsonDocument(config).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_amneziaCode = QString("vpn://%1")
                        .arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding
                                                               | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

void ExportController::generateConnectionConfig()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials credentials = qvariant_cast<ServerCredentials>(
        m_serversModel->data(serverIndex, ServersModel::ServersModelRoles::CredentialsRole));

    DockerContainer container = static_cast<DockerContainer>(
        m_containersModel->getCurrentlyProcessedContainerIndex());
    QModelIndex containerModelIndex = m_containersModel->index(container);
    QJsonObject containerConfig = qvariant_cast<QJsonObject>(
        m_containersModel->data(containerModelIndex, ContainersModel::Roles::ConfigRole));
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject protocolConfig = m_settings->protocolConfig(serverIndex, container, protocol);

        QString vpnConfig = m_configurator->genVpnProtocolConfig(credentials,
                                                                 container,
                                                                 containerConfig,
                                                                 protocol,
                                                                 &errorCode);
        if (errorCode) {
            emit exportErrorOccurred(errorString(errorCode));
            return;
        }
        protocolConfig.insert(config_key::last_config, vpnConfig);
        containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfig);
    }

    QJsonObject config = m_settings->server(serverIndex);
    if (!errorCode) {
        config.remove(config_key::userName);
        config.remove(config_key::password);
        config.remove(config_key::port);
        config.insert(config_key::containers, QJsonArray{containerConfig});
        config.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        auto dns = m_configurator->getDnsForConfig(serverIndex);
        config.insert(config_key::dns1, dns.first);
        config.insert(config_key::dns2, dns.second);
    }

    QByteArray compressedConfig = QJsonDocument(config).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_amneziaCode = QString("vpn://%1")
                        .arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding
                                                               | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

QString ExportController::getAmneziaCode()
{
    return m_amneziaCode;
}

QList<QString> ExportController::getQrCodes()
{
    return m_qrCodes;
}

void ExportController::saveFile()
{
    QString fileExtension = ".vpn";
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QUrl fileName;
    fileName = QFileDialog::getSaveFileUrl(nullptr,
                                           tr("Save AmneziaVPN config"),
                                           QUrl::fromLocalFile(docDir + "/" + "amnezia_config"),
                                           "*" + fileExtension);
    if (fileName.isEmpty())
        return;
    if (!fileName.toString().endsWith(fileExtension)) {
        fileName = QUrl(fileName.toString() + fileExtension);
    }
    if (fileName.isEmpty())
        return;

    QFile save(fileName.toLocalFile());

    save.open(QIODevice::WriteOnly);
    save.write(m_amneziaCode.toUtf8());
    save.close();

    QFileInfo fi(fileName.toLocalFile());
    QDesktopServices::openUrl(fi.absoluteDir().absolutePath());
}

QList<QString> ExportController::generateQrCodeImageSeries(const QByteArray &data)
{
    double k = 850;

    quint8 chunksCount = std::ceil(data.size() / k);
    QList<QString> chunks;
    for (int i = 0; i < data.size(); i = i + k) {
        QByteArray chunk;
        QDataStream s(&chunk, QIODevice::WriteOnly);
        s << amnezia::qrMagicCode << chunksCount << (quint8) std::round(i / k) << data.mid(i, k);

        QByteArray ba = chunk.toBase64(QByteArray::Base64UrlEncoding
                                       | QByteArray::OmitTrailingEquals);

        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(ba, qrcodegen::QrCode::Ecc::LOW);
        QString svg = QString::fromStdString(toSvgString(qr, 0));
        chunks.append(svgToBase64(svg));
    }

    return chunks;
}

QString ExportController::svgToBase64(const QString &image)
{
    return "data:image/svg;base64," + QString::fromLatin1(image.toUtf8().toBase64().data());
}

int ExportController::getQrCodesCount()
{
    return m_qrCodes.size();
}

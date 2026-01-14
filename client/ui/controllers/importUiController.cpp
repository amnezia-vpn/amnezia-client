#include "importUiController.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QDataStream>
#include <QJsonDocument>

#include "core/utils/qrCodeUtils.h"
#include "systemController.h"

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined Q_OS_ANDROID
ImportUiController* ImportUiController::mInstance = nullptr;
static QMutex qrDecodeMutex;
#endif

ImportUiController::ImportUiController(ImportController* importController, QObject *parent)
    : QObject(parent),
      m_importController(importController),
      m_isNativeWireGuardConfig(false)
{
#if defined Q_OS_ANDROID
    mInstance = this;
#endif

    connect(m_importController, &ImportController::importFinished, this, &ImportUiController::importFinished);
    connect(m_importController, &ImportController::importErrorOccurred, this, &ImportUiController::importErrorOccurred);
    connect(m_importController, &ImportController::restoreAppConfig, this, &ImportUiController::restoreAppConfig);
}

bool ImportUiController::extractConfigFromFile(const QString &fileName)
{
    QString data;
    if (!SystemController::readFile(fileName, data)) {
        emit importErrorOccurred(ErrorCode::ImportOpenConfigError, false);
        return false;
    }
    
    QString configFileName = QFileInfo(QFile(fileName).fileName()).fileName();
#ifdef Q_OS_ANDROID
    if (configFileName.isEmpty()) {
        configFileName = AndroidController::instance()->getFileName(fileName);
    }
#endif
    
    auto result = m_importController->extractConfigFromData(data, configFileName);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit importErrorOccurred(result.errorCode, false);
        return false;
    }
    
    m_config = result.config;
    m_configFileName = result.configFileName;
    m_maliciousWarningText = result.maliciousWarningText;
    m_isNativeWireGuardConfig = result.isNativeWireGuardConfig;
    
    emit importConfigChanged();
    return true;
}

bool ImportUiController::extractConfigFromData(QString data)
{
    auto result = m_importController->extractConfigFromData(data);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit importErrorOccurred(result.errorCode, false);
        return false;
    }
    
    m_config = result.config;
    m_configFileName = result.configFileName;
    m_maliciousWarningText = result.maliciousWarningText;
    m_isNativeWireGuardConfig = result.isNativeWireGuardConfig;
    
    emit importConfigChanged();
    return true;
}

bool ImportUiController::extractConfigFromQr(const QByteArray &data)
{
    auto result = m_importController->extractConfigFromQr(data);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit importErrorOccurred(result.errorCode, false);
        return false;
    }
    
    m_config = result.config;
    m_configFileName = result.configFileName;
    m_maliciousWarningText = result.maliciousWarningText;
    m_isNativeWireGuardConfig = result.isNativeWireGuardConfig;
    
    emit importConfigChanged();
    return true;
}

QString ImportUiController::getConfig()
{
    return QJsonDocument(m_config).toJson(QJsonDocument::Indented);
}

QString ImportUiController::getConfigFileName()
{
    return m_configFileName;
}

QString ImportUiController::getMaliciousWarningText()
{
    return m_maliciousWarningText;
}

bool ImportUiController::isNativeWireGuardConfig()
{
    return m_isNativeWireGuardConfig;
}

void ImportUiController::processNativeWireGuardConfig()
{
    m_config = m_importController->processNativeWireGuardConfig(m_config);
    emit importConfigChanged();
}

void ImportUiController::importConfig()
{
    m_importController->importConfig(m_config);
    
    m_config = {};
    m_configFileName.clear();
    m_maliciousWarningText.clear();
    m_isNativeWireGuardConfig = false;
    
    emit importConfigChanged();
}

void ImportUiController::clearConfigFileName()
{
    m_configFileName.clear();
    emit importConfigChanged();
}

#if defined Q_OS_ANDROID || defined Q_OS_IOS
void ImportUiController::startDecodingQr()
{
    m_qrCodeChunks.clear();
    m_totalQrCodeChunksCount = 0;
    m_receivedQrCodeChunksCount = 0;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_isQrCodeProcessed = true;
#endif
#if defined Q_OS_ANDROID
    AndroidController::instance()->startQrReaderActivity();
#endif
}

void ImportUiController::stopDecodingQr()
{
    emit qrDecodingFinished();
}

bool ImportUiController::parseQrCodeChunk(const QString &code)
{
    if (!m_isQrCodeProcessed)
        return false;

    // check if chunk received
    QByteArray ba = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream s(&ba, QIODevice::ReadOnly);
    qint16 magic;
    s >> magic;

    if (magic == qrCodeUtils::qrMagicCode) {
        quint8 chunksCount;
        s >> chunksCount;
        if (m_totalQrCodeChunksCount != chunksCount) {
            m_qrCodeChunks.clear();
        }

        m_totalQrCodeChunksCount = chunksCount;

        quint8 chunkId;
        s >> chunkId;
        s >> m_qrCodeChunks[chunkId];
        m_receivedQrCodeChunksCount = m_qrCodeChunks.size();

        if (m_qrCodeChunks.size() == m_totalQrCodeChunksCount) {
            QByteArray data;

            for (int i = 0; i < m_totalQrCodeChunksCount; ++i) {
                data.append(m_qrCodeChunks.value(i));
            }

            bool ok = extractConfigFromQr(data);
            if (ok) {
                m_isQrCodeProcessed = false;
                qDebug() << "stopDecodingQr";
                stopDecodingQr();
                return true;
            } else {
                qDebug() << "error while extracting data from qr";
                m_qrCodeChunks.clear();
                m_totalQrCodeChunksCount = 0;
                m_receivedQrCodeChunksCount = 0;
            }
        }
    } else {
        bool ok = extractConfigFromQr(ba);
        if (ok) {
            m_isQrCodeProcessed = false;
            qDebug() << "stopDecodingQr";
            stopDecodingQr();
            return true;
        }
    }
    return false;
}

double ImportUiController::getQrCodeScanProgressBarValue()
{
    if (m_totalQrCodeChunksCount == 0) {
        return 0.0;
    }
    return (1.0 / m_totalQrCodeChunksCount) * m_receivedQrCodeChunksCount;
}

QString ImportUiController::getQrCodeScanProgressString()
{
    return tr("Scanned %1 of %2.").arg(m_receivedQrCodeChunksCount).arg(m_totalQrCodeChunksCount);
}
#endif

#if defined Q_OS_ANDROID
bool ImportUiController::decodeQrCode(const QString &code)
{
    QMutexLocker lock(&qrDecodeMutex);

    if (!mInstance) {
        return false;
    }

    if (!mInstance->m_isQrCodeProcessed) {
        mInstance->m_qrCodeChunks.clear();
        mInstance->m_isQrCodeProcessed = true;
        mInstance->m_totalQrCodeChunksCount = 0;
        mInstance->m_receivedQrCodeChunksCount = 0;
    }
    return mInstance->parseQrCodeChunk(code);
}
#endif

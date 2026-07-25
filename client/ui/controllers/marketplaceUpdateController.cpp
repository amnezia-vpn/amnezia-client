#include "marketplaceUpdateController.h"

#include <QDesktopServices>

#include "core/controllers/updateController.h"
#include "logger.h"
#include "version.h"

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    #include <memory>

    #include <QJsonArray>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QLocale>
    #include <QNetworkReply>
    #include <QNetworkRequest>
    #include <QRegularExpression>
    #include <QVersionNumber>
#endif

namespace
{
    Logger logger("MarketplaceUpdateController");

#if defined(Q_OS_IOS)
    constexpr auto kIosBundleId = "org.amnezia.AmneziaVPN";
    constexpr auto kIosStoreUrlFallback = "itms-apps://itunes.apple.com/app/id1600529900";
#elif defined(Q_OS_ANDROID)
    constexpr auto kAndroidPackage = "org.amnezia.vpn";
    constexpr auto kAndroidStoreUrl = "https://play.google.com/store/apps/details?id=org.amnezia.vpn";
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    constexpr int kStoreRequestTimeoutMs = 7000;
#endif
}

MarketplaceUpdateController::MarketplaceUpdateController(UpdateController *updateController, QObject *parent)
    : QObject(parent), m_updateController(updateController)
{
}

void MarketplaceUpdateController::start()
{
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    const QUrl url = versionSourceUrl();
    if (!url.isValid()) {
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(kStoreRequestTimeoutMs);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("AmneziaVPN"));

    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            logger.error() << "Store request failed:" << reply->errorString();
            return;
        }

        QString version;
        QString storeUrl;
        if (!parseStoreVersion(reply->readAll(), version, storeUrl) || version.isEmpty()) {
            logger.error() << "Could not determine store version";
            return;
        }

        const auto current = QVersionNumber::fromString(QString(APP_VERSION)).normalized();
        const auto store = QVersionNumber::fromString(version).normalized();
        logger.info() << "Current version:" << current.toString() << "store version:" << store.toString();

        if (store <= current) {
            return;
        }

        m_storeUrl = storeUrl;
        fetchChangelogAndNotify();
    });
#endif
}

void MarketplaceUpdateController::openStoreUrl()
{
    if (m_storeUrl.isEmpty()) {
        logger.error() << "Store url is empty";
        return;
    }

    QDesktopServices::openUrl(QUrl(m_storeUrl));
}

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
void MarketplaceUpdateController::fetchChangelogAndNotify()
{
    if (!m_updateController) {
        emit updateAvailable();
        return;
    }

    // The drawer is shown once the changelog fetch settles, successfully or not:
    // an update was already detected by the store, so a missing changelog must
    // not swallow the notification.
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(m_updateController, &UpdateController::releaseInfoFetched, this, [this, connection]() {
        disconnect(*connection);
        emit updateAvailable();
    });

    m_updateController->fetchReleaseInfo();
}

QUrl MarketplaceUpdateController::versionSourceUrl() const
{
  #if defined(Q_OS_IOS)
    const QString country = QLocale::system().name().section('_', 1, 1).toLower();
    QString url = QStringLiteral("https://itunes.apple.com/lookup?bundleId=%1").arg(kIosBundleId);
    if (!country.isEmpty()) {
        url += QStringLiteral("&country=%1").arg(country);
    }
    return QUrl(url);
  #else
    return QUrl(QStringLiteral("https://play.google.com/store/apps/details?id=%1&hl=en&gl=US").arg(kAndroidPackage));
  #endif
}

bool MarketplaceUpdateController::parseStoreVersion(const QByteArray &body, QString &version, QString &storeUrl)
{
  #if defined(Q_OS_IOS)
    const auto results = QJsonDocument::fromJson(body).object().value("results").toArray();
    if (results.isEmpty()) {
        return false;
    }
    const auto first = results.first().toObject();
    version = first.value("version").toString();
    storeUrl = first.value("trackViewUrl").toString();
    if (storeUrl.isEmpty()) {
        storeUrl = QString::fromLatin1(kIosStoreUrlFallback);
    }
    return !version.isEmpty();
  #else
    const QString html = QString::fromUtf8(body);
    static const QRegularExpression re(QStringLiteral("\\[\\[\\[\"(\\d+\\.\\d+(?:\\.\\d+){0,2})\"\\]\\]"));
    const auto match = re.match(html);
    if (match.hasMatch()) {
        version = match.captured(1);
    }
    storeUrl = QString::fromLatin1(kAndroidStoreUrl);
    return !version.isEmpty();
  #endif
}
#endif

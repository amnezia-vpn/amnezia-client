#include "marketplaceUpdateController.h"

#include <QDebug>

#include "version.h"

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    #include <QJsonArray>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QLocale>
    #include <QNetworkReply>
    #include <QNetworkRequest>
    #include <QRegularExpression>
    #include <QVersionNumber>

    #include "amneziaApplication.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
#endif

#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
namespace
{
#if defined(Q_OS_IOS)
constexpr auto kIosBundleId = "org.amnezia.AmneziaVPN";
constexpr auto kIosStoreUrlFallback = "itms-apps://itunes.apple.com/app/id1600529900";
#else
constexpr auto kAndroidPackage = "org.amnezia.vpn";
constexpr auto kGithubReleasesUrl = "https://github.com/amnezia-vpn/amnezia-client/releases/latest";
#endif
} // namespace
#endif

MarketplaceUpdateController::MarketplaceUpdateController(QObject *parent) : QObject(parent)
{
}

void MarketplaceUpdateController::start()
{
#if defined(Q_OS_IOS)
    startNetworkCheck();
#elif defined(Q_OS_ANDROID)
    connect(AndroidController::instance(), &AndroidController::playUpdateResult, this,
            &MarketplaceUpdateController::onPlayUpdateResult, Qt::UniqueConnection);

    const QString restartTitle = tr("Update downloaded");
    const QString restartMessage = tr("An update to AmneziaVPN has been downloaded. Restart to install it.");
    const QString restartAction = tr("Restart");
    const QString restartLater = tr("Later");
    AndroidController::instance()->checkPlayUpdate(restartTitle, restartMessage, restartAction, restartLater);
#endif
}

#if defined(Q_OS_ANDROID)
void MarketplaceUpdateController::onPlayUpdateResult(int status)
{
    if (status == 1) {
        qInfo() << "[MarketplaceUpdate] Play unavailable, falling back to GitHub";
        startNetworkCheck();
    }
}
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
void MarketplaceUpdateController::startNetworkCheck()
{
    const QUrl url = versionSourceUrl();
    if (!url.isValid()) {
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("AmneziaVPN"));
    request.setTransferTimeout(7000);

    QNetworkReply *reply = amnApp->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[MarketplaceUpdate] network error:" << reply->errorString();
            return;
        }

        QString version;
        QString storeUrl;
        if (!parseStoreVersion(reply->readAll(), version, storeUrl) || version.isEmpty()) {
            qWarning() << "[MarketplaceUpdate] could not determine store version";
            return;
        }

        const auto current = QVersionNumber::fromString(QString(APP_VERSION)).normalized();
        const auto store = QVersionNumber::fromString(version).normalized();
        qInfo() << "[MarketplaceUpdate] current:" << current.toString() << "store:" << store.toString();

        if (store > current) {
            showUpdatePrompt(storeUrl);
        }
    });
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
    storeUrl = QString::fromLatin1(kGithubReleasesUrl);
    return !version.isEmpty();
  #endif
}

void MarketplaceUpdateController::showUpdatePrompt(const QString &storeUrl)
{
    const QString title = tr("Update available");
    const QString message = tr("A new version of AmneziaVPN is available.");
    const QString updateTitle = tr("Update");
    const QString skipTitle = tr("Skip");

  #if defined(Q_OS_IOS)
    IosController::Instance()->showUpdatePrompt(title, message, updateTitle, skipTitle, storeUrl);
  #else
    AndroidController::instance()->showUpdatePrompt(title, message, updateTitle, skipTitle, storeUrl);
  #endif
}
#endif

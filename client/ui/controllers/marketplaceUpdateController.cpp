#include "marketplaceUpdateController.h"

#include <QCoreApplication>
#include <QDebug>

#include "version.h"

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    #include <QDesktopServices>
#endif

#if defined(Q_OS_IOS) || (defined(Q_OS_ANDROID) && defined(QT_DEBUG))
    #include <QJsonArray>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QLocale>
    #include <QNetworkReply>
    #include <QNetworkRequest>
    #include <QRegularExpression>
    #include <QVersionNumber>
#endif

#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

namespace
{
#if defined(Q_OS_IOS)
constexpr auto kIosBundleId = "org.amnezia.AmneziaVPN";
constexpr auto kIosStoreUrlFallback = "itms-apps://itunes.apple.com/app/id1600529900";
#endif
#if defined(Q_OS_ANDROID)
constexpr auto kAndroidPackage = "org.amnezia.vpn";
constexpr auto kAndroidStoreUrl = "https://play.google.com/store/apps/details?id=org.amnezia.vpn";
#endif
} // namespace

MarketplaceUpdateController::MarketplaceUpdateController(QObject *parent) : QObject(parent)
{
#if defined(Q_OS_ANDROID) && !defined(QT_DEBUG)
    connect(AndroidController::instance(), &AndroidController::playUpdateAvailability, this,
            [this](bool available) { setState(available ? UpdateRequired : UpToDate); },
            Qt::QueuedConnection);
#endif
}

QString MarketplaceUpdateController::currentVersion() const
{
    return QString(APP_VERSION);
}

void MarketplaceUpdateController::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void MarketplaceUpdateController::start()
{
    setState(Checking);

#if defined(Q_OS_IOS)
    startHttpCheck(versionSourceUrl());
#elif defined(Q_OS_ANDROID)
  #if defined(QT_DEBUG)
    startHttpCheck(versionSourceUrl());
  #else
    AndroidController::instance()->checkPlayUpdate();
  #endif
#else
    setState(UpToDate);
#endif
}

void MarketplaceUpdateController::openStore()
{
#if defined(Q_OS_IOS)
    if (!m_storeUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_storeUrl));
    }
#elif defined(Q_OS_ANDROID)
  #if defined(QT_DEBUG)
    QDesktopServices::openUrl(QUrl(m_storeUrl.isEmpty() ? QString::fromLatin1(kAndroidStoreUrl) : m_storeUrl));
  #else
    AndroidController::instance()->startPlayUpdateFlow();
  #endif
#endif
}

void MarketplaceUpdateController::quit()
{
    qApp->quit();
}

#if defined(Q_OS_IOS) || (defined(Q_OS_ANDROID) && defined(QT_DEBUG))
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

bool MarketplaceUpdateController::parseVersion(const QByteArray &body, QString &version, QString &storeUrl)
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

void MarketplaceUpdateController::applyStoreVersion(const QString &version, const QString &storeUrl)
{
    m_storeVersion = version;
    m_storeUrl = storeUrl;

    const auto current = QVersionNumber::fromString(currentVersion()).normalized();
    const auto store = QVersionNumber::fromString(version).normalized();
    qInfo() << "[MarketplaceUpdate] current:" << current.toString() << "store:" << store.toString();

    setState(store > current ? UpdateRequired : UpToDate);
}

void MarketplaceUpdateController::startHttpCheck(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("AmneziaVPN"));

    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[MarketplaceUpdate] network error:" << reply->errorString();
            setState(NoInternet);
            return;
        }

        QString version;
        QString storeUrl;
        if (!parseVersion(reply->readAll(), version, storeUrl) || version.isEmpty()) {
            qWarning() << "[MarketplaceUpdate] could not determine store version";
            setState(NoInternet);
            return;
        }

        applyStoreVersion(version, storeUrl);
    });
}
#endif

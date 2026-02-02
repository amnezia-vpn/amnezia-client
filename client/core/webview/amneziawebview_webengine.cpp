#include <QCoreApplication>
#include <QWebEngineUrlScheme>

#include "amneziawebview_webengine_p.h"
#include "qrchandler.h"
#include "filehandler.h"
#include "amneziawebhistory.h"

typedef QMap<quintptr, AmneziaWebViewPrivate *> WebViews;
Q_GLOBAL_STATIC_WITH_ARGS(WebViews, g_webViews, ())

QrcHandler::QrcHandler()
{}

FileHandler::FileHandler()
{
}

JsHandler::JsHandler(AmneziaWebView *host): _host(host), scriptObjectsInjected(false)
{
    init();
}

JsHandler::~JsHandler() {}

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(parent)
{
 
}

WebPage::WebPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
 	
}

WebPage::~WebPage()
{
    disconnect(this);
}

bool WebPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type);
    // Always accept navigation requests to open links within WebView
    // This prevents opening links in external browser
    if (isMainFrame) {
        m_loadingUrl = url;
        emit loadingUrl(url);
    }
    return true;
}

QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);
    // Return this page instead of creating a new window
    // This prevents opening new browser windows/tabs
    return this;
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    QString errorString = reply->errorString();

    if (m_loadingUrl != reply->url()) {
        // sub resource of this page
        qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError && !reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
        errorString = "Unknown Content-Type";
    }

    QFile file(QLatin1String(":/notfound.html"));
    bool isOpened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpened);
    Q_UNUSED(isOpened)

    QString title = QCoreApplication::translate("webview", "Error loading page: %1").arg(reply->url().toString());
    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(errorString)
                        .arg(reply->url().toString());

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0);
    QPixmap pixmap = icon.pixmap(QSize(32,32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    if (m_loadingUrl == reply->url()) {
        setHtml(html, reply->url());
    }
}

const QString &LocalSchemeHandler::scheme() 
{
     static const QString localScheme("local"); 
     return localScheme; 
}

const QMimeDatabase &LocalSchemeHandler::mimeDatabase()
{
    static const QMimeDatabase mimeDatabase;
    return mimeDatabase;
}

void LocalSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QByteArray requestMethod = job->requestMethod();
    const QUrl requestUrl = job->requestUrl();
    QString requestScheme = requestUrl.scheme();
    DesktopWebViewPrivate *d = qobject_cast<DesktopWebViewPrivate *>(parent());

    if (!d) {
        job->fail(QWebEngineUrlRequestJob::RequestFailed);
        return;
    }

    if (requestScheme != LocalSchemeHandler::scheme()) {
        job->fail(QWebEngineUrlRequestJob::UrlInvalid);
        return;
    }

    QBuffer *buffer = nullptr;
    QMimeType mimeType = mimeDatabase().mimeTypeForFile(requestUrl.fileName(), QMimeDatabase::MatchExtension);
    
    if (d->fileHandler.canHandleUrl(requestUrl)) {
        const QByteArray content = d->fileHandler.dataForUrl(requestUrl);
        if (!content.isNull()) {
            buffer = new QBuffer();
            buffer->setData(content);
        }
    }
    if (!buffer) {

        auto historyItems = d->history()->items();
        const auto it = std::find_if(historyItems.begin(), historyItems.end(),
                    [requestUrl](const auto &item) {  
                        bool urlsMatch = item.url().matches(requestUrl, QUrl::RemoveQuery | QUrl::RemoveFragment);
                        bool hasData = (item.data().length() > 0);                
                        return urlsMatch && hasData; });

        if (it != historyItems.end()) {    
            buffer = new QBuffer(); 
            buffer->setData(it->data()); 
            mimeType = mimeDatabase().mimeTypeForName(it->mimeType()); 
        }
    }

    if (!buffer) { 
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    connect(job, &QObject::destroyed, buffer, &QObject::deleteLater);        
    job->reply(mimeType.name().toLocal8Bit(), buffer);
}

DesktopWebViewPrivate::DesktopWebViewPrivate(AmneziaWebView* q): AmneziaWebViewPrivate(q)
    , viewId(reinterpret_cast<quintptr>(this))
    , containerWindow(0)
    , window(0)  
{
    m_localHandler = new LocalSchemeHandler(this);

    container = new QWidget(0, Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Tool);
    container->setAttribute(Qt::WA_NativeWindow, true);
    container->setAttribute(Qt::WA_DontCreateNativeAncestors, true);

    QWebEngineUrlScheme localScheme(LocalSchemeHandler::scheme().toUtf8());
    localScheme.setFlags(QWebEngineUrlScheme::LocalAccessAllowed |
            QWebEngineUrlScheme::SecureScheme |
            QWebEngineUrlScheme::ViewSourceAllowed |
            QWebEngineUrlScheme::ContentSecurityPolicyIgnored |
            QWebEngineUrlScheme::CorsEnabled |
            QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(localScheme);  

    QWebEngineUrlScheme qrcScheme("qrc");
    qrcScheme.setFlags(QWebEngineUrlScheme::LocalScheme |
            QWebEngineUrlScheme::LocalAccessAllowed |
            QWebEngineUrlScheme::SecureScheme |
            QWebEngineUrlScheme::ContentSecurityPolicyIgnored |
            QWebEngineUrlScheme::CorsEnabled |
            QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(qrcScheme);

    view = new QWebEngineView(container);
    QWebEngineProfile *profile = new QWebEngineProfile(view);
    profile->installUrlSchemeHandler(LocalSchemeHandler::scheme().toUtf8(), m_localHandler);

    m_page = new WebPage(profile, profile);
    m_page->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    connect(m_page, &QWebEnginePage::fileSystemAccessRequested, this, [](QWebEngineFileSystemAccessRequest request) {
        request.accept();
    });

    view->settings()->setUnknownUrlSchemePolicy(QWebEngineSettings::AllowAllUnknownUrlSchemes);
    view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    view->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    view->settings()->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, true);

    view->setPage(m_page);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setLayout(new QHBoxLayout(container));
    container->layout()->setSpacing(0);
    container->layout()->setContentsMargins(0, 0, 0, 0);
    container->layout()->addWidget(view);

    setBackgroundColor(backgroundColor);
    g_webViews->insert(viewId, this);

    connect(view, SIGNAL(loadFinished(bool)), this, SIGNAL(loadFinished(bool)));
    connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)),  Qt::QueuedConnection);
    connect(m_page, SIGNAL(loadStarted()), this, SLOT(onPageStarted()),  Qt::QueuedConnection);
    connect(view, SIGNAL(urlChanged(const QUrl &)), this, SLOT(onUrlChanged(const QUrl &)), Qt::QueuedConnection);
    container->createWinId();
}

DesktopWebViewPrivate::~DesktopWebViewPrivate()
{
    g_webViews->take(viewId);
    disconnect(this, SLOT(loadFinished(bool)));
    disconnect(this, SLOT(applicationStateChanged(Qt::ApplicationState)));
    disconnect(this, SLOT(onUrlChanged(const QUrl &)));
    disconnect(this, SLOT(onPageStarted()));
    disconnect(this, SLOT(onLoadFinished(bool)));

    delete container;
}

AmneziaWebViewPrivate *AmneziaWebViewPrivate::create(AmneziaWebView *q)
{
    return new DesktopWebViewPrivate(q);
}

void DesktopWebViewPrivate::setWindowParent(QWindow *parent)
{
    if (window) {
        window->removeEventFilter(this);
    }

    if (parent) {

        containerWindow = qobject_cast<QWindow*>(container->windowHandle());
        containerWindow->setTransientParent(parent);
        parent->installEventFilter(this);

    }
    window = parent;
}

void DesktopWebViewPrivate::setBackgroundColor(const QColor backgroundColor)
{
    this->backgroundColor = backgroundColor;
    QPalette p = container->palette();
    p.setColor(QPalette::Window, backgroundColor);
    container->setPalette(p);
    p = view->palette();
    p.setColor(QPalette::Window, backgroundColor);
    view->setPalette(p);
    emit backgroundColorChanged();
}

/// Deprecated
void DesktopWebViewPrivate::setScale(qreal scale)
{
    Q_UNUSED(scale);
}

/// Deprecated
qreal DesktopWebViewPrivate::scale() const
{
    return 1;
}

QIcon DesktopWebViewPrivate::icon() const
{
    return QIcon();
}

QSize DesktopWebViewPrivate::contentsSize() const
{
    return QSize();
}

void DesktopWebViewPrivate::setGeometry(const QRect &geometry)
{
    Q_Q(AmneziaWebView);
    QQuickWindow *window = q->window();
    if (!window) return;
    QRect newGeometry = QRect(window->mapToGlobal(geometry.topLeft()), QSize(geometry.width(), geometry.height()));
    if (newGeometry.isValid() && container->geometry() != newGeometry ) {

        this->geometry = geometry;
        container->setGeometry(newGeometry);
        container->updateGeometry();
    }
}

bool DesktopWebViewPrivate::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    Q_Q(AmneziaWebView);

    switch (event->type()) {
          case QEvent::Move: {
                    QMoveEvent *moveEvent = static_cast<QMoveEvent*>(event);
                    QPoint p = q->mapToScene(QPointF(moveEvent->pos())).toPoint();
                    container->move(p);

                    if (visible) {
                        show();
                    }
                    return false;
                }
                break;
           case QEvent::Resize: {
                    if (visible) {
                        show();
                    }
                    return false;
                }
                break;
          case QEvent::WindowStateChange: {

                Qt::WindowState state = window->windowState();
                if ((state == Qt::WindowMaximized) || (state == Qt::WindowFullScreen) || (state == Qt::WindowActive))  {
                    show();
                }
                else {
                    hide();
                }
                return false;
            }
        default: {
            return false;
           }
    }
    return false;
}

void DesktopWebViewPrivate::hide()
{
    Q_Q(AmneziaWebView);
    if (!q->window()) return;

    if (visible) {
        QMetaObject::invokeMethod(container, "hide", Qt::QueuedConnection);
        visible = false;
    }    
}

void DesktopWebViewPrivate::show()
{
    Q_Q(AmneziaWebView);
    if (!q->window()) return;
    if (!visible) {

        QMetaObject::invokeMethod(container, "show", Qt::QueuedConnection);
        QMetaObject::invokeMethod(container, "update", Qt::QueuedConnection);
        visible = true;
    }

    containerWindow = qobject_cast<QWindow*>(container->windowHandle());

    if ((containerWindow != nullptr) &&
        ((qApp->topLevelWindows().at(0) != containerWindow) || !containerWindow->isVisible())) {

        containerWindow->raise();
    }
}

void DesktopWebViewPrivate::load(const QUrl& baseUrl)
{
    QUrl url = baseUrl;
    if (!url.isValid()) {
        url = QUrl(QLatin1String("about:blank"));
    }
    view->load(url);
    history()->append(baseUrl);
}

void DesktopWebViewPrivate::setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl)
{
    QUrl url = baseUrl;
    if (!url.isValid()) {
        url = QUrl(QLatin1String("about:blank"));
    }
    view->setContent(data, mimeType, url);
    history()->append(url, data, mimeType);
}

void DesktopWebViewPrivate::setHtml(const QString& html, const QUrl& baseUrl)
{
    if(html.isNull()) return;
    QUrl url = baseUrl;
    if (!baseUrl.isValid())
        url = QUrl(QLatin1String("about:blank"));    

    view->setHtml(html, url);
    history()->append(url, html.toUtf8(), "text/html");
}

void DesktopWebViewPrivate::evaluateJavaScript(const QString& scriptSource)
{
    view->page()->runJavaScript(scriptSource);
}

bool DesktopWebViewPrivate::canGoBack() const
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Back);
    bool can = (pageAction && pageAction->isEnabled());
    return can;
}

bool DesktopWebViewPrivate::canGoForward() const
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Forward);
    bool can = (pageAction && pageAction->isEnabled());
    return can;
}

void DesktopWebViewPrivate::back()
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Back);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::forward()
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Forward);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::reload()
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Reload);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::stop()
{
    QAction *pageAction = view->pageAction(QWebEnginePage::Stop);
    if (pageAction)
        emit pageAction->trigger();
}

bool DesktopWebViewPrivate::isLoading() const
{
    return false;
}

QString DesktopWebViewPrivate::innerHTML() const
{
    return QString();
}

void DesktopWebViewPrivate::onLoadFinished(bool success)
{
    if (success) {
        QMetaObject::invokeMethod(this, "onPageFinished", Qt::QueuedConnection);
    }
    else {
        QMetaObject::invokeMethod(this, "onPageError",  Qt::QueuedConnection);
    }
}

void DesktopWebViewPrivate::setDefaultFontSize(int size)
{
    view->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, size);
}

void DesktopWebViewPrivate::setStandardFontFamily(const QString &family)
{
    view->settings()->setFontFamily(QWebEngineSettings::StandardFont, family);
}

void DesktopWebViewPrivate::setTextZoom(int percent)
{
    Q_UNUSED(percent);
}

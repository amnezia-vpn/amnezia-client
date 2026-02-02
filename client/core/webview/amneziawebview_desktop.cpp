#include <QtCore>
#include <QDebug>
#include <QBoxLayout>
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QQuickWindow>

#include "amneziawebview_desktop_p.h"
#include "qrchandler.h"
#include "filehandler.h"

typedef QMap<quintptr, AmneziaWebViewPrivate *> WebViews;
Q_GLOBAL_STATIC(WebViews, g_webViews)


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
    : QWebPage(parent)
{
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(handleUnsupportedContent(QNetworkReply*)));
}

void WebPage::javaScriptAlert(QWebFrame *frame, const QString& msg)
{
    Q_UNUSED(frame)
    Q_UNUSED(msg)
}

WebPage::~WebPage()
{
    disconnect(this);
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    return QWebPage::acceptNavigationRequest(frame, request, type);
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
    QIcon icon = view()->style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, view());
    QPixmap pixmap = icon.pixmap(QSize(32,32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    QList<QWebFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebFrame *frame = frames.takeFirst();
        if (frame->url() == reply->url()) {
            frame->setHtml(html, reply->url());
            return;
        }
        QList<QWebFrame *> children = frame->childFrames();
        foreach(QWebFrame *frame, children)
            frames.append(frame);
    }
    if (m_loadingUrl == reply->url()) {
        mainFrame()->setHtml(html, reply->url());
    }
}

DesktopWebViewPrivate::DesktopWebViewPrivate(AmneziaWebView* q): AmneziaWebViewPrivate(q)
    , viewId(reinterpret_cast<quintptr>(this))
    , containerWindow(nullptr)
    , window(nullptr)
{
    container = new QWidget(0, Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Tool);
    container->setAttribute(Qt::WA_NativeWindow, true);
    container->setAttribute(Qt::WA_DontCreateNativeAncestors, true);

    // Do not remove next line -> prevent some sort of spontaneous crashes
    QWebSettings::setObjectCacheCapacities(0, 0, 0);

    view = new QWebView(container);
    WebPage *page = new WebPage(view);
    page->setForwardUnsupportedContent(true);
    page->setNetworkAccessManager(networkAccessManager());
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    view->setPage(page);

    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setLayout(new QHBoxLayout(container));
    container->layout()->setSpacing(0);
    container->layout()->setMargin(0);
    container->layout()->addWidget(view);

    setBackgroundColor(backgroundColor);    
    g_webViews->insert(viewId, this);

    connect(view, SIGNAL(loadFinished(bool)), this, SIGNAL(loadFinished(bool)));

    connect(page, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)),  Qt::QueuedConnection);
    connect(page, SIGNAL(loadStarted()), this, SLOT(onPageStarted()),  Qt::QueuedConnection);
    connect(view, SIGNAL(urlChanged(const QUrl &)), this, SLOT(onUrlChanged(const QUrl &)), Qt::QueuedConnection);
    container->createWinId();
}

DesktopWebViewPrivate::~DesktopWebViewPrivate()
{
    disconnect(this, SLOT(loadFinished(bool)));
    disconnect(this, SLOT(applicationStateChanged(Qt::ApplicationState)));
    disconnect(this, SLOT(onUrlChanged(const QUrl &)));
    disconnect(this, SLOT(onPageStarted()));
    disconnect(this, SLOT(onLoadFinished(bool)));

    g_webViews->take(viewId);
    view->stop();
    view->setPage(nullptr);

    container->deleteLater();
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
    p.setColor(QPalette::Background, backgroundColor);
    container->setPalette(p);
    p = view->palette();
    p.setColor(QPalette::Background, backgroundColor);
    view->setPalette(p);
    emit backgroundColorChanged();
}

/// Deprecated
void DesktopWebViewPrivate::setScale(qreal scale)
{
    Q_UNUSED(scale)
    //qreal s =  view->geometry().width() / view->page()->preferredContentsSize().width();
    //view->setZoomFactor(s);
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

void DesktopWebViewPrivate::takeSnapshot()
{
    if (geometry.isEmpty() || !containerWindow) {
        return;
    }
    else {

        container->updateGeometry();
        QPixmap pixmap = container->grab();
        snapshot = pixmap.toImage();
        emit snapshotChanged();
    }
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
    Q_UNUSED(obj)
    Q_Q(AmneziaWebView);

    switch (event->type()) {
          case QEvent::Move: {
                    QMoveEvent *moveEvent = static_cast<QMoveEvent*>(event);
                    QPoint p = q->mapToScene(QPointF(moveEvent->pos())).toPoint();
                    container->move(p);
                    //container->updateGeometry();

                    if (visible) {
                        show();
                    }
                    return false;
                }
           case QEvent::Resize: {

                    //QRect newGeometry = QRect(window->mapToGlobal(geometry.topLeft()), QSize(geometry.width(), geometry.height()));
                    //container->setGeometry(newGeometry);
                    //container->updateGeometry();
                    
                    if (visible) {
                        show();
                    }
                    return false;
                }
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
}

void DesktopWebViewPrivate::hide()
{
    Q_Q(AmneziaWebView);
    if (!q->window()) return;

    if (visible) {

        QMetaObject::invokeMethod(this, "requestSnapshot", Qt::QueuedConnection);
        QMetaObject::invokeMethod(container, "hide", Qt::QueuedConnection);
        visible = false;

        //if (q->isVisible())
        //    q->update();
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

    if ((qApp->topLevelWindows().at(0) != containerWindow) || !containerWindow->isVisible()) {

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
    view->page()->mainFrame()->evaluateJavaScript(scriptSource);
}

bool DesktopWebViewPrivate::canGoBack() const
{
    QAction *pageAction = view->pageAction(QWebPage::Back);
    bool can = (pageAction && pageAction->isEnabled());
    return can;
}

bool DesktopWebViewPrivate::canGoForward() const
{
    QAction *pageAction = view->pageAction(QWebPage::Forward);
    bool can = (pageAction && pageAction->isEnabled());
    return can;
}

void DesktopWebViewPrivate::back()
{
    QAction *pageAction = view->pageAction(QWebPage::Back);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::forward()
{
    QAction *pageAction = view->pageAction(QWebPage::Forward);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::reload()
{
    QAction *pageAction = view->pageAction(QWebPage::Reload);
    if (pageAction)
        emit pageAction->trigger();
}

void DesktopWebViewPrivate::stop()
{
    QAction *pageAction = view->pageAction(QWebPage::Stop);
    if (pageAction)
        emit pageAction->trigger();
}

bool DesktopWebViewPrivate::isLoading() const
{
    return false;
}

QString DesktopWebViewPrivate::innerHTML() const
{
    QVariant result = view->page()->mainFrame()->evaluateJavaScript("document.body.innerHTML");
    return result.toString();
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
    view->settings()->setFontSize(QWebSettings::DefaultFontSize, size);
}

void DesktopWebViewPrivate::setStandardFontFamily(const QString &family)
{
    view->settings()->setFontFamily(QWebSettings::StandardFont, family);
}

void DesktopWebViewPrivate::setTextZoom(int percent)
{
    Q_UNUSED(percent)
}
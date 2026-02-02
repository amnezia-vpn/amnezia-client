#include "amneziawebview_p.h"
#include "amneziawebhistory.h"
#include "websettings.h"

NetworkAccessManager::NetworkAccessManager(QNetworkAccessManager *manager, QObject *parent)
    : QNetworkAccessManager(parent)
{
    this->manager = manager;
    setCache(manager->cache());
    setCookieJar(manager->cookieJar());
    setProxy(manager->proxy());
    setProxyFactory(manager->proxyFactory());

    init();
}

void NetworkAccessManager::init()
{
    connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this,
                SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *device)
{
    AmneziaWebView *view = qobject_cast<AmneziaWebView *>(parent());
    AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(view);

    if (!d || !d->canHandleUrl(request.url())) {
        return QNetworkAccessManager::createRequest(operation, request, device);
    }

    if (operation == GetOperation) {
        return new DataReply(this, operation, request, view);
    }
    else

      return QNetworkAccessManager::createRequest(operation, request, device);
}

void NetworkAccessManager::handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    Q_UNUSED(errors)
    reply->ignoreSslErrors();
}

DataReply::DataReply(QObject *parent, const QNetworkAccessManager::Operation operation, const QNetworkRequest &request, AmneziaWebView *view): QNetworkReply(parent)
{
    setRequest(request);
    setUrl(request.url());
    setOperation(operation);
    setFinished(true);
    this->view = view;
    offset = 0;

    //QUrl url = request.url();
    //url.setHost(QString());
    //if (url.path().isEmpty())
    //    url.setPath(QLatin1String("/"));
    //setUrl(url);

    QMetaObject::invokeMethod(this, "setContent", Qt::QueuedConnection );
}

void DataReply::setContent()
{
    if (!view || view.isNull()) return;

    AmneziaWebViewPrivate *q = AmneziaWebViewPrivate::get(view);
    content = q->dataForUrl(url());
    QString mimeType = q->mimeTypeForUrl(url()).toLower();
    //open(ReadOnly | Unbuffered);
    QNetworkReply::open(QIODevice::ReadOnly);

    int size = content.size();
    if (size <= 0 ) {

        QString msg = QString("Error opening %1").arg(url().toString());
        qCritical() << msg;
        setError(QNetworkReply::ContentNotFoundError, msg);
                QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                    Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentNotFoundError));

       QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
       return;
    }

    setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QString("%1; charset=%2").arg(mimeType, "utf-8")));

    setHeader(QNetworkRequest::ContentLengthHeader, size);
    QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
        Q_ARG(qint64, size), Q_ARG(qint64, size));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

void DataReply::abort()
{
    QNetworkReply::close();
}

void DataReply::close()
{
    QNetworkReply::close();
}

qint64 DataReply::size() const
{
    return content.size();
}

bool DataReply::isSequential() const
{
    return true;
}

qint64 DataReply::bytesAvailable() const
{
    return content.size() - offset + QIODevice::bytesAvailable();
}
qint64 DataReply::readData(char *data, qint64 maxSize)
{
    if (offset < content.size()) {
        qint64 number = qMin(maxSize, content.size() - offset);
        memcpy(data, content.constData() + offset, number);
        offset += number;
        return number;
    } else {
        return -1;
    }
}

AmneziaWebViewPrivate::AmneziaWebViewPrivate(AmneziaWebView *q) : QObject(q)
      , pending(PendingNone)
      , status(AmneziaWebView::Null)
      , preferredwidth(0)
      , preferredheight(0)
      , progress(1.0)  
      , newWindowComponent(nullptr)
      , newWindowParent(nullptr)
      , jsHandler(q)
      , rendering(true)
      , overlapped(false)
      , backgroundColor(QColor::fromRgb(28, 29, 33))
      , visible(false)
      , networkManager(nullptr)
      , q_ptr(q)
      , m_history(new AmneziaWebHistory(q))
      , m_settings(new AmneziaWebViewSettings(q))
{
}

void AmneziaWebViewPrivate::init()
{
    Q_Q(AmneziaWebView);
    m_settings->apply();

    actions.setMapping(new QAction(q), (int)AmneziaWebView::Back);
    actions.setMapping(new QAction(q), (int)AmneziaWebView::Forward);
    actions.setMapping(new QAction(q), (int)AmneziaWebView::Stop);
    actions.setMapping(new QAction(q), (int)AmneziaWebView::Reload);
    connect(action(AmneziaWebView::Back), SIGNAL(triggered()), &actions, SLOT(map()));
    connect(action(AmneziaWebView::Forward), SIGNAL(triggered()), &actions, SLOT(map()));
    connect(action(AmneziaWebView::Forward), SIGNAL(triggered()), &actions, SLOT(map()));
    connect(action(AmneziaWebView::Forward), SIGNAL(triggered()), &actions, SLOT(map()));
    connect(&actions, SIGNAL(mappedInt(int)), this, SLOT(onAction(int)));
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(applicationStateChanged(Qt::ApplicationState)));
}

AmneziaWebViewPrivate::~AmneziaWebViewPrivate()
{
    Q_Q(AmneziaWebView);
    disconnect(this, SLOT(onAction(int)));
    disconnect(&actions, SLOT(map()));
    if (networkManager && networkManager->parent() == q)
        delete networkManager;
}

AmneziaWebViewPrivate *AmneziaWebViewPrivate::get(AmneziaWebView *q)
{
    if (!q) { return nullptr; }
    return q->d_func();
}

void AmneziaWebViewPrivate::applicationStateChanged(Qt::ApplicationState state)
{
    Q_Q(AmneziaWebView);
    if ((state == Qt::ApplicationActive) && q->isVisible())  {
        emit q->update();
    }
    else {
        hide();
    }
}

void AmneziaWebViewPrivate::requestShow()
{
    show();
}

void AmneziaWebViewPrivate::requestHide()
{
    hide();
}

void AmneziaWebViewPrivate::move(const QPoint &point)
{
    QRect newGeomentry = geometry;
    newGeomentry.moveTo(point);
    setGeometry(newGeomentry);
}

void AmneziaWebViewPrivate::windowObjectsClear(QQmlListProperty<QObject>* prop)
{
    static_cast<AmneziaWebViewPrivate*>(prop->data)->windowObjects.clear();
}

QObject *AmneziaWebViewPrivate::windowObjectsAt(QQmlListProperty<QObject> *prop, qsizetype index)
{
    return static_cast<AmneziaWebViewPrivate *>(prop->data)->windowObjects.at(index);
}

qsizetype AmneziaWebViewPrivate::windowObjectsCount(QQmlListProperty<QObject> *prop)
{
    return static_cast<AmneziaWebViewPrivate *>(prop->data)->windowObjects.count();
}

void AmneziaWebViewPrivate::windowObjectsAppend(QQmlListProperty<QObject>* prop, QObject* o)
{
    static_cast<AmneziaWebViewPrivate*>(prop->data)->windowObjects.append(o);
    static_cast<AmneziaWebViewPrivate*>(prop->data)->updateWindowObjects();
}

void AmneziaWebViewPrivate::setTitle(const QString &title)
{
    Q_Q(AmneziaWebView);
    if (this->title != title) {
        this->title = title;
        emit q->titleChanged(this->title);
    }
}

QByteArray AmneziaWebViewPrivate::dataForUrl(const QUrl &url) const
{
    QByteArray data;
    if (qrcHandler.canHandleUrl(url)) {
        data = qrcHandler.dataForUrl(url);
    }
    else if (jsHandler.canHandleUrl(url)) {
        
        data = jsHandler.dataForUrl(url);
    }
    else if (fileHandler.canHandleUrl(url)) {

        data = fileHandler.dataForUrl(url);
    }
    return data;
}

QString AmneziaWebViewPrivate::mimeTypeForUrl(const QUrl &url) const
{
    QString mimeType("application/octet-stream");
    if (qrcHandler.canHandleUrl(url)) {
        mimeType = qrcHandler.mimeTypeForUrl(url);
    }
    else if (jsHandler.canHandleUrl(url)) {
        mimeType = jsHandler.mimeTypeForUrl(url);
    }
    else if (fileHandler.canHandleUrl(url)) {
        mimeType = fileHandler.mimeTypeForUrl(url);
    }
    return mimeType;
}

void AmneziaWebViewPrivate::onPageStarted()
{
    Q_Q(AmneziaWebView);
    emit q->loadStarted();
}

void AmneziaWebViewPrivate::onPageFinished()
{
    Q_Q(AmneziaWebView);
    jsHandler.updateWebView();
    
    if (lastError.length() > 0) {
        emit q->loadFinished(false);
    }
    else {
        emit q->loadFinished(true);
    }
}

void AmneziaWebViewPrivate::onPageError()
{

}

void AmneziaWebViewPrivate::onUrlChanged(const QUrl &url)
{
    history()->append(url);
    setUrl(url);
}

void AmneziaWebViewPrivate::setUrl(const QUrl &url)
{
    Q_Q(AmneziaWebView);
    
    QString urlString = url.toString();
    while ( urlString.endsWith('#')) urlString.chop(1);
    QUrl newUrl(urlString);
    
    if (newUrl == QUrl(QLatin1String("about:blank")) ) {
        newUrl = QUrl("");
    }

    if (this->url != newUrl) {

        this->url = newUrl;
        emit q->urlChanged();
    }
}

void AmneziaWebViewPrivate::addToJavaScriptWindowObject(const QString& name, QObject* object)
{
    jsHandler.addToJavaScriptWindowObject(name, object);
}


bool AmneziaWebViewPrivate::canHandleUrl(const QUrl &url) const
{
    bool can = (jsHandler.canHandleUrl(url) || qrcHandler.canHandleUrl(url) || fileHandler.canHandleUrl(url));
    return can;
}

QString AmneziaWebViewPrivate::toHtml() const
{
    return QString();
}

void AmneziaWebViewPrivate::load(const QNetworkRequest& request, QNetworkAccessManager::Operation operation, const QByteArray& body)
{
    Q_UNUSED(request)
    Q_UNUSED(operation)
    Q_UNUSED(body)
}

QNetworkAccessManager* AmneziaWebViewPrivate::networkAccessManager()
{
    Q_Q(AmneziaWebView);

    if (!networkManager)
        networkManager = new NetworkAccessManager(q);
    return networkManager;
}

void AmneziaWebViewPrivate::setNetworkAccessManager(QNetworkAccessManager* manager)
{
    Q_Q(AmneziaWebView);
    if (manager == networkManager)
        return;
    if (networkManager && networkManager->parent() == q)
        delete networkManager;

    NetworkAccessManager *newManager = qobject_cast<NetworkAccessManager *>(manager);
    if (!newManager && manager) {
        newManager = new NetworkAccessManager(manager, q);
    }
    networkManager = newManager;
}

QAction *AmneziaWebViewPrivate::action(AmneziaWebView::WebAction action) const
{
    QAction *ret = qobject_cast<QAction*>(actions.mapping((int)action));
    if (ret) {
        switch (action) {
            case AmneziaWebView::Back: {
                bool can = canGoBack() || history()->canGoBack();
                ret->setEnabled(can);
            }
            break;
            case AmneziaWebView::Forward: {
                bool can = canGoForward() || history()->canGoForward();
                ret->setEnabled(can);
            }
            break;
            default:
                break;
        }
    }
    return ret;
}

void AmneziaWebViewPrivate::onAction(int action)
{
    switch (action) {
    case AmneziaWebView::Back: {
            if (canGoBack()) {
               back();
            }
            //else {
            //    history()->back();
            //}
        }
        break;
    case AmneziaWebView::Forward:
        if (canGoForward()) forward();
        break;
    case AmneziaWebView::Stop:
        stop();
        break;
    case AmneziaWebView::Reload:
        reload();
        break;
    default:
        break;
    }
}

AmneziaWebHistory* AmneziaWebViewPrivate::history() const
{
    return m_history.data();
}

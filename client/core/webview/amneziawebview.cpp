#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QThread>
#include <QMetaObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPen>
#include <QList>
#include <QQuickWindow>
#include <QTimer>

#include <QPainter>

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
    #define qApp qGuiApp
#else
    #include <QApplication>
#endif

#include "amneziawebview.h"
#include "amneziawebview_p.h"

QUrl defaultBaseUrl()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WINDOWS)
    return QUrl(QLatin1String("local:///"));
#else
    return QUrl(QLatin1String("file:///"));
#endif
}

QT_BEGIN_NAMESPACE

AmneziaWebView::AmneziaWebView(QQuickItem *parent) : QQuickPaintedItem(parent),
                                                 d_ptr(AmneziaWebViewPrivate::create(this))
{
    Q_D(AmneziaWebView);
    d->q_ptr = this;
    d->init();
    init();
}

AmneziaWebView::~AmneziaWebView()
{
    disconnect(this);
}

void AmneziaWebView::init()
{
    Q_D(AmneziaWebView);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QQuickItem::ItemHasContents, true);
    setOpaquePainting(true);
    
    setClip(true);

    connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(windowWasChanged(QQuickWindow*)));
    connect(this, SIGNAL(parentChanged(QQuickItem*)), this, SLOT(parentWasChanged()));
    connect(this, SIGNAL(fillColorChanged()), this,SLOT(fillColorWasChanged()));

    connect(d, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
    connect(d, SIGNAL(loadStarted()), this, SLOT(doLoadStarted()));
    connect(d, SIGNAL(loadFinished(bool)), this, SLOT(doLoadFinished(bool)));
}

void AmneziaWebView::componentComplete()
{
    Q_D(AmneziaWebView);
    QQuickItem::componentComplete();
    
    // Update geometry after component is complete
    if (window()) {
        QMetaObject::invokeMethod(this, "updateGeometry", Qt::QueuedConnection);
    }
    
    switch (d->pending) {
    case AmneziaWebViewPrivate::PendingUrl:
        // Make WebView visible before loading
        if (isVisible() && !d->visible) {
            d->show();
        }
        setUrl(d->pendingUrl);
        break;
    case AmneziaWebViewPrivate::PendingHtml:
        if (isVisible() && !d->visible) {
            d->show();
        }
        setHtml(d->pendingString, d->pendingUrl);
        break;
    case AmneziaWebViewPrivate::PendingContent:
        if (isVisible() && !d->visible) {
            d->show();
        }
        setContent(d->pendingData, d->pendingString, d->pendingUrl);
        break;
    default:
        break;
    }
}

AmneziaWebView::Status AmneziaWebView::status() const
{
    Q_D(const AmneziaWebView);
    return d->status;
}

/*!
    \qmlproperty real WebView::progress
    This property holds the progress of loading the current URL, from 0 to 1.

 If you just want to know when progress gets to 1, use
 WebView::onLoadFinished() or WebView::onLoadFailed() instead.
*/
qreal AmneziaWebView::progress() const
{
    Q_D(const AmneziaWebView);
    return d->progress;
}

void AmneziaWebView::doLoadStarted()
{
    Q_D(AmneziaWebView);
    
    if (!d->url.isEmpty()) {
        d->status = Loading;
        emit statusChanged(d->status);
    }
    emit loadStarted();
}

void AmneziaWebView::doLoadProgress(int p)
{
    Q_D(AmneziaWebView);
    
    if (d->progress == p / 100.0)
        return;
    d->progress = p / 100.0;
    emit progressChanged();
}


void AmneziaWebView::doLoadFinished(bool ok)
{
    Q_D(AmneziaWebView);
    
    if (ok) {
        d->status = d->url.isEmpty() ? Null : Ready;
        emit loadFinished();
    } else {
        d->status = Error;
        emit loadFailed();
    }
    emit statusChanged(d->status);
}

/*!
    \qmlproperty url AmneziaWebView::url
    This property holds the URL to the page displayed in this item. It can be set,
    but also can change spontaneously (eg. because of network redirection).

 If the url is empty, the page is blank.

 The url is always absolute (QML will resolve relative URL strings in the context
 of the containing QML document).
*/
QUrl AmneziaWebView::url() const
{
    Q_D(const AmneziaWebView);
    return d->url;
}

void AmneziaWebView::setUrl(const QUrl& url)
{
    Q_D(AmneziaWebView);

    QString urlString = url.toString();
    while ( urlString.endsWith('#')) urlString.chop(1);
    QUrl newUrl(urlString);

    if (newUrl == QUrl(QLatin1String("about:blank")) ) {
        newUrl = QUrl("");
    }
    
    if ((url == d->url) || (newUrl == d->url))
        return;

    if (isComponentComplete()) {
        // Make WebView visible before loading
        if (isVisible() && !d->visible) {
            d->show();
        }
        d->load(url);

    } else {
        
        d->pending = d->PendingUrl;
        d->pendingUrl = url;
    }
}

qreal AmneziaWebView::preferredWidth() const
{
    Q_D(const AmneziaWebView);
    return d->preferredwidth;
}

void AmneziaWebView::setPreferredWidth(qreal width)
{
    Q_D(AmneziaWebView);

    if (d->preferredwidth == width)
        return;

    d->preferredwidth = width;
    updateContentsSize();
    setImplicitWidth(width);
    emit preferredWidthChanged();
}

qreal AmneziaWebView::preferredHeight() const
{
    Q_D(const AmneziaWebView);
    return d->preferredheight;
}

void AmneziaWebView::setPreferredHeight(qreal height)
{
    Q_D(AmneziaWebView);

    if (d->preferredheight == height)
        return;
    
    d->preferredheight = height;
    updateContentsSize();
    setImplicitHeight(height);
    emit preferredHeightChanged();
}


/*!
    \qmlmethod bool AmneziaWebView::evaluateJavaScript(string scriptSource)

 Evaluates the \a scriptSource JavaScript inside the context of the
 main web frame, and returns the result of the last executed statement.

 Note that this JavaScript does \e not have any access to QML objects
 except as made available as windowObjects.
*/
void AmneziaWebView::evaluateJavaScript(const QString& scriptSource)
{
    Q_D(AmneziaWebView);
    if (qApp->thread() == QThread::currentThread()) {
        d->evaluateJavaScript(scriptSource);
    }
    else {
        QMetaObject::invokeMethod(d, "evaluateJavaScript", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QString, scriptSource));
    }
}

void AmneziaWebView::windowWasChanged(QQuickWindow* window)
{
    Q_D(AmneziaWebView);
    d->setWindowParent(window);
}

void AmneziaWebView::updateGeometry()
{
    Q_D(AmneziaWebView);
    QRectF geometry = QRectF(QPointF(x(), y()), QSizeF(width(), height()));
    if (!geometry.isEmpty() && window()) {
        QRectF sceneGeometry = mapRectToScene(geometry);
        QRect rect = sceneGeometry.toRect();
        qDebug() << "AmneziaWebView::updateGeometry() - local:" << geometry 
                 << "scene:" << sceneGeometry << "rect:" << rect
                 << "width:" << width() << "height:" << height();
        d->setGeometry(rect);
    }
}


void AmneziaWebView::parentWasChanged()
{
    if (parentItem()) {
        updateGeometry();
    }
}

QList<QQuickItem *> recurseChildren(QQuickItem * parentItem)
{
    QList<QQuickItem *>childs = parentItem->childItems();
    QList<QQuickItem *> items;
    int count = childs.count();
    for(int i = count - 1; i >= 0; --i) {
        QQuickItem *next = childs.at(i);
        items.append(recurseChildren(next));
    }
    items.append(childs);
    return items;
}


void AmneziaWebView::paint(QPainter *painter)
{
    Q_D(AmneziaWebView);
    if (!painter || !window() ) {
        return;
    }

    QRectF contentRect = contentsBoundingRect();
    if ((contentRect.height() <= 0) || (contentRect.width() <= 0)) return;

    painter->setOpacity(1.0);
    QMutexLocker lock(&d->renderMutex);

    QColor color = d->backgroundColor;
    painter->fillRect(contentRect, color);

}

void AmneziaWebView::afterRendering()
{
    Q_D(AmneziaWebView);

    Qt::ApplicationState state = qApp->applicationState();
    if ( state != Qt::ApplicationActive)  {
        if (d->visible && isVisible()) {
            QMetaObject::invokeMethod(d, "requestHide", Qt::QueuedConnection);
        }
        return;
    }

    if (!window()) return;

    if (isVisible()) {
        QMetaObject::invokeMethod(this, "updateGeometry", Qt::QueuedConnection);
    }

    QMetaObject::invokeMethod(d, "requestShow", Qt::QueuedConnection);

}

void AmneziaWebView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(AmneziaWebView);

    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    
    // Update WebView geometry when QML item size changes
    if (window() && !newGeometry.isEmpty() && newGeometry != oldGeometry) {
        QMetaObject::invokeMethod(this, "updateGeometry", Qt::QueuedConnection);
    }
}

void AmneziaWebView::itemChange(ItemChange change, const ItemChangeData & value)
{
    Q_D(AmneziaWebView);
    switch (change) {
    case ItemSceneChange: {
        QQuickWindow *sc = value.window;
        if (sc) {
            connect(sc, SIGNAL(afterRendering()), this, SLOT(afterRendering()), Qt::QueuedConnection);
        }
        else {
            disconnect(this, SLOT(afterRendering()));
        }
    }
    break;
    case ItemVisibleHasChanged: {

        if (!window()) break;

        if (value.boolValue) {
            // Component became visible - show WebView
            if (!d->visible) {
                d->show();
            }
        } else {
            QMetaObject::invokeMethod(d, "requestHide", Qt::QueuedConnection);
        }
        if (value.boolValue && !d->overlapped) {
            QMetaObject::invokeMethod(d, "requestShow", Qt::QueuedConnection);
        }
    }
    break;
    default:
        break;
    }    
    QQuickPaintedItem::itemChange(change, value);
}

/*!
    \qmlproperty list<object> WebView::javaScriptWindowObjects

 A list of QML objects to expose to the web page.

 Each object will be added as a property of the web frame's window object.  The
 property name is controlled by the value of \c WebView.windowObjectName
 attached property.

 Exposing QML objects to a web page allows JavaScript executing in the web
 page itself to communicate with QML, by reading and writing properties and
 by calling methods of the exposed QML objects.

 This example shows how to call into a QML method using a window object.

 \qml
 WebView {
     javaScriptWindowObjects: QtObject {
         WebView.windowObjectName: "qml"

         function qmlCall() {
             console.log("This call is in QML!");
         }
     }

     html: "<script>window.qml.qmlCall();</script>"
 }
 \endqml

 The output of the example will be:
 \code
 This call is in QML!
 \endcode

 If Javascript is not enabled for the page, then this property does nothing.
*/
QQmlListProperty<QObject> AmneziaWebView::javaScriptWindowObjects()
{
    Q_D(AmneziaWebView);
    return QQmlListProperty<QObject>(this, d, &AmneziaWebViewPrivate::windowObjectsAppend,
                                     &AmneziaWebViewPrivate::windowObjectsCount,
                                     &AmneziaWebViewPrivate::windowObjectsAt,
                                     &AmneziaWebViewPrivate::windowObjectsClear );
}

AmneziaWebViewSettings* AmneziaWebView::settingsObject() const
{
    Q_D(const AmneziaWebView);
    return d->m_settings.data();
}


AmneziaWebViewAttached* AmneziaWebView::qmlAttachedProperties(QObject* o)
{
    return new AmneziaWebViewAttached(o);
}

void AmneziaWebViewPrivate::updateWindowObjects()
{
    
    if (!q_ptr->isComponentCompletePublic())
        return;

    for (int i = 0; i < windowObjects.count(); ++i) {
        QObject* object = windowObjects.at(i);
        AmneziaWebViewAttached* attached = static_cast<AmneziaWebViewAttached *>(qmlAttachedPropertiesObject<AmneziaWebView>(object));
        if (attached && !attached->windowObjectName().isEmpty())
            addToJavaScriptWindowObject(attached->windowObjectName(), object);
    }
}

int AmneziaWebView::pressGrabTime() const
{
    return 0;
}

void AmneziaWebView::setPressGrabTime(int millis)
{
    Q_UNUSED(millis)

    emit pressGrabTimeChanged();
}

#ifndef QT_NO_ACTION
/*!
    \qmlproperty action WebView::back
    This property holds the action for causing the previous URL in the history to be displayed.
*/
QAction* AmneziaWebView::backAction() const
{
    return action(AmneziaWebView::Back);
}

/*!
    \qmlproperty action WebView::forward
    This property holds the action for causing the next URL in the history to be displayed.
*/
QAction* AmneziaWebView::forwardAction() const
{
    return action(AmneziaWebView::Forward);
}

/*!
    \qmlproperty action WebView::reload
    This property holds the action for reloading with the current URL
*/
QAction* AmneziaWebView::reloadAction() const
{
    return action(AmneziaWebView::Reload);
}

/*!
    \qmlproperty action WebView::stop
    This property holds the action for stopping loading with the current URL
*/
QAction* AmneziaWebView::stopAction() const
{
    return action(AmneziaWebView::Stop);
}
#endif // QT_NO_ACTION

/*!
    \qmlproperty string WebView::title
    This property holds the title of the web page currently viewed

 By default, this property contains an empty string.
*/
QString AmneziaWebView::title() const
{
    Q_D(const AmneziaWebView);
    return d->title;
}

/*!
    \qmlproperty pixmap WebView::icon
    This property holds the icon associated with the web page currently viewed
*/
QPixmap AmneziaWebView::icon() const
{
    Q_D(const AmneziaWebView);
    return d->icon().pixmap(QSize(256, 256));
}

/*!
    \qmlproperty string WebView::statusText

 This property is the current status suggested by the current web page. In a web browser,
 such status is often shown in some kind of status bar.
*/
void AmneziaWebView::setStatusText(const QString& text)
{
    Q_D(AmneziaWebView);
    d->statusText = text;
    emit statusTextChanged();
}

void AmneziaWebView::windowObjectCleared()
{
    Q_D(AmneziaWebView);
    d->updateWindowObjects();
}

QString AmneziaWebView::statusText() const
{
    Q_D(const AmneziaWebView);
    return d->statusText;
}


void AmneziaWebView::load(const QNetworkRequest& request, QNetworkAccessManager::Operation operation, const QByteArray& body)
{
    Q_D(AmneziaWebView);
    d->load(request, operation, body);
}

QString AmneziaWebView::html() const
{
    Q_D(const AmneziaWebView);
    return d->toHtml();
}

void AmneziaWebView::setHtml(const QString& html, const QUrl& baseUrl)
{
    Q_D(AmneziaWebView);
    auto originUrl = baseUrl.isValid() ? baseUrl : defaultBaseUrl();

    updateContentsSize();
    if (isComponentComplete()) {
        d->setHtml(html, originUrl);
    }
    else {
        d->pending = d->PendingHtml;
        d->pendingUrl = originUrl;
        d->pendingString = html;
    }
    emit htmlChanged();
}

void AmneziaWebView::setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl)
{
    Q_D(AmneziaWebView);
    
    updateContentsSize();
    auto originUrl = baseUrl.isValid() ? baseUrl : defaultBaseUrl();

    if (isComponentComplete())
        d->setContent(data, mimeType, qmlContext(this)->resolvedUrl(baseUrl));
    else {
        d->pending = d->PendingContent;
        d->pendingUrl = originUrl;
        d->pendingString = mimeType;
        d->pendingData = data;
    }
}

AmneziaWebHistory* AmneziaWebView::history() const
{
    Q_D(const AmneziaWebView);
    return d->history();
}

#ifndef QT_NO_ACTION
QAction* AmneziaWebView::action(AmneziaWebView::WebAction action) const
{
    Q_D(const AmneziaWebView);
    return d->action(action);
}
#endif

/*!
    \qmlproperty component WebView::newWindowComponent

 This property holds the component to use for new windows.
 The component must have a WebView somewhere in its structure.

 When the web engine requests a new window, it will be an instance of
 this component.

 The parent of the new window is set by newWindowParent. It must be set.
*/
QQmlComponent* AmneziaWebView::newWindowComponent() const
{
    Q_D(const AmneziaWebView);
    return d->newWindowComponent;
}

void AmneziaWebView::setNewWindowComponent(QQmlComponent* newWindow)
{
    Q_D(AmneziaWebView);
    if (newWindow == d->newWindowComponent)
        return;
    d->newWindowComponent = newWindow;
    emit newWindowComponentChanged();
}


/*!
    \qmlproperty item WebView::newWindowParent

 The parent item for new windows.

 \sa newWindowComponent
*/
QQuickItem* AmneziaWebView::newWindowParent() const
{
    Q_D(const AmneziaWebView);
    return d->newWindowParent;
}

void AmneziaWebView::setNewWindowParent(QQuickItem *parent)
{
    Q_D(AmneziaWebView);
    if (parent == d->newWindowParent)
        return;
    if (d->newWindowParent && parent) {
        QList<QQuickItem *> children = d->newWindowParent->childItems();
        for (int i = 0; i < children.count(); ++i)
            children.at(i)->setParentItem(parent);
    }
    d->newWindowParent = parent;
    emit newWindowParentChanged();
}

QSize AmneziaWebView::contentsSize() const
{
    Q_D(const AmneziaWebView);
    return d->contentsSize() * contentsScale();
}

qreal AmneziaWebView::contentsScale() const
{
    Q_D(const AmneziaWebView);
    return d->scale();
}

void AmneziaWebView::setContentsScale(qreal scale)
{
    Q_D(AmneziaWebView);
    if (scale == d->scale())
        return;
    d->setScale(scale);

           //updateGeometry();
    emit contentsScaleChanged();
}

void AmneziaWebView::setDefaultFontSize(int size)
{
    Q_D(AmneziaWebView);
    d->setDefaultFontSize(size);
}

void AmneziaWebView::setStandardFontFamily(const QString &family)
{
    Q_D(AmneziaWebView);
    d->setStandardFontFamily(family);
}

void AmneziaWebView::setTextZoom(int percent)
{
    Q_D(AmneziaWebView);
    d->setTextZoom(percent);
}


#ifdef Q_REVISION
/*!
    \qmlproperty color WebView::backgroundColor
    \since QtWebKit 1.1
    This property holds the background color of the view.
*/

QColor AmneziaWebView::backgroundColor() const
{
    Q_D(const AmneziaWebView);
    return d->backgroundColor;
}

void AmneziaWebView::setBackgroundColor(const QColor& color)
{
    setFillColor(color);
}

void AmneziaWebView::fillColorWasChanged()
{
    Q_D(AmneziaWebView);
    QColor color = fillColor();
    d->setBackgroundColor(color);
    emit backgroundColorChanged();
}

#endif


QT_END_NAMESPACE


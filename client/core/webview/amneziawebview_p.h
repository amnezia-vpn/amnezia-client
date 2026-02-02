#ifndef WEBVIEW_P_H
#define WEBVIEW_P_H

#include "amneziawebview.h"
#include "qrchandler.h"
#include "jshandler.h"
#include "filehandler.h"

#include "amneziawebhistory.h"

class WebSettings;
class QIcon;
class QSize;


class WebViewSettings;
class WebSettings;

class AmneziaWebViewPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(AmneziaWebView)
    
public:
    explicit AmneziaWebViewPrivate(AmneziaWebView *q);
    virtual ~AmneziaWebViewPrivate();
    static AmneziaWebViewPrivate *create(AmneziaWebView *q);
    void init();

    virtual void setWindowParent(QWindow *parent) = 0;
    virtual void setBackgroundColor(const QColor backgroundColor) = 0;

    virtual void setGeometry(const QRect &) = 0;
    virtual QString innerHTML() const = 0;
    virtual void load(const QUrl& url) = 0;
    virtual void setHtml(const QString& html, const QUrl& baseUrl = QUrl()) = 0;
    virtual void setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl) = 0;
    virtual void evaluateJavaScript(const QString& scriptSource) = 0;
    virtual bool isLoading() const = 0;
    virtual bool canGoBack() const = 0;
    virtual bool canGoForward() const = 0;
    virtual void back() = 0;
    virtual void forward() = 0;
    virtual void reload() = 0;
    virtual void stop() = 0;
    virtual QIcon icon() const = 0;
    virtual void setScale(qreal scale) = 0;
    virtual qreal scale() const = 0;
    virtual QSize contentsSize() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void setDefaultFontSize(int size) = 0;
    virtual void setStandardFontFamily(const QString &family) = 0;
    virtual void setTextZoom(int percent) = 0;

    virtual void setUrl(const QUrl &url);
    
    static AmneziaWebViewPrivate *get(AmneziaWebView *q);
    
    enum { PendingNone, PendingUrl, PendingHtml, PendingContent } pending;
    
    QString toHtml() const;
    AmneziaWebHistory* history() const;

    QByteArray dataForUrl(const QUrl &url) const;
    QString mimeTypeForUrl(const QUrl &url) const;
    bool canHandleUrl(const QUrl &url) const;
    
    static void windowObjectsClear(QQmlListProperty<QObject> *prop);
    static QObject *windowObjectsAt(QQmlListProperty<QObject> *prop, qsizetype index);
    static qsizetype windowObjectsCount(QQmlListProperty<QObject> *prop);
    static void windowObjectsAppend(QQmlListProperty<QObject> *prop, QObject *o);
    
    void updateWindowObjects();
    QObjectList windowObjects;
    
    QNetworkAccessManager* networkAccessManager();
    void setNetworkAccessManager(QNetworkAccessManager* manager);
    void load(const QNetworkRequest& request, QNetworkAccessManager::Operation operation, const QByteArray& body);
    QAction *action(AmneziaWebView::WebAction) const;

public Q_SLOTS:
    void setTitle(const QString &title);
    void move(const QPoint &);
    void requestHide();
    void requestShow();

Q_SIGNALS:
    void loadStarted();
    void loadFinished(bool ok);
    void loadProgress(int progress);
    void titleChanged(const QString& title);
    void urlChanged(const QUrl& url);
    void backgroundColorChanged();
public:
    
    QUrl url; 
    AmneziaWebView::Status status;
    qreal preferredwidth, preferredheight;
    qreal progress;
    QString statusText;
    QUrl pendingUrl;
    QString pendingString;
    QByteArray pendingData;

    QQmlComponent* newWindowComponent;
    QQuickItem* newWindowParent;
    
    QrcHandler qrcHandler;
    JsHandler jsHandler;
    FileHandler fileHandler;
    
    bool rendering;
    bool overlapped;

    QColor backgroundColor;
    QUrl baseUrl;
    QString title;
    QRect geometry;
    QString lastError;
    mutable bool visible;

protected Q_SLOTS:
    void applicationStateChanged(Qt::ApplicationState state);
    void onPageStarted();
    void onPageFinished();
    void onPageError();
    void onUrlChanged(const QUrl &url);
    void onAction(int);

protected:

    void addToJavaScriptWindowObject(const QString& name, QObject* object);

    QMutex renderMutex;
    QNetworkAccessManager *networkManager;
    AmneziaWebView *q_ptr;

private:   
    QSignalMapper actions;
    QScopedPointer<AmneziaWebHistory> m_history;
    QScopedPointer<AmneziaWebViewSettings> m_settings;
};

//!internal
class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit NetworkAccessManager(QNetworkAccessManager *manager, QObject *parent);
    explicit NetworkAccessManager(QObject *parent): QNetworkAccessManager(parent) { init(); }

public Q_SLOTS:
    void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = nullptr);
private:
    void init();
    QNetworkAccessManager *manager;
};

class DataReply : public QNetworkReply
{
    Q_OBJECT

public:
    explicit DataReply(QObject *parent, const QNetworkAccessManager::Operation operation, const QNetworkRequest &request, AmneziaWebView *view = nullptr);
    virtual ~DataReply() = default;

    virtual void abort();
    virtual void close();
    virtual qint64 size() const;

    virtual qint64 bytesAvailable() const;
    virtual bool isSequential() const;
protected:
    virtual qint64 readData(char *data, qint64 maxSize);
    Q_INVOKABLE void setContent();

private:
    QByteArray content;
    qint64 offset;
    QPointer<AmneziaWebView> view;
};

#endif

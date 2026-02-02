#ifndef AMNEZIAWEBVIEW_WEBENGINE_P_H
#define AMNEZIAWEBVIEW_WEBENGINE_P_H

#include <QtWebEngineWidgets/QtWebEngineWidgets>

#include <QtWebEngineWidgets>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>

#include "amneziawebview.h"
#include "amneziawebview_p.h"

class DesktopWebViewPrivate;
class LocalSchemeHandler;

class WebPage : public QWebEnginePage
{
    Q_OBJECT

signals:
    void loadingUrl(const QUrl &url);

public:
    explicit WebPage(QObject *parent = 0);
    explicit WebPage(QWebEngineProfile *profile, QObject *parent = 0);
    virtual ~WebPage();

protected:
    bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame) override;
    QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) override;

private slots:
    void handleUnsupportedContent(QNetworkReply *reply);

private:

    friend class DesktopWebViewPrivate;
    QUrl m_loadingUrl;
};

class DesktopWebViewPrivate : public AmneziaWebViewPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(AmneziaWebView)
public:

    explicit DesktopWebViewPrivate(AmneziaWebView* q);
    virtual ~DesktopWebViewPrivate();

    virtual void setWindowParent(QWindow *parent);

    virtual void setBackgroundColor(const QColor backgroundColor);
    virtual void show();
    virtual void hide();
    virtual void setGeometry(const QRect &);
    virtual QString innerHTML() const;
    virtual void load(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& baseUrl = QUrl());
    virtual void setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl);
    virtual void evaluateJavaScript(const QString& scriptSource);
    virtual bool isLoading() const;
    virtual bool canGoBack() const;
    virtual bool canGoForward() const;
    virtual void back();
    virtual void forward();
    virtual void reload();
    virtual void stop();

    virtual QIcon icon() const;
    virtual void setScale(qreal scale);
    virtual qreal scale() const;
    virtual QSize contentsSize() const;
    virtual void setDefaultFontSize(int size);
    virtual void setTextZoom(int percent);
    virtual void setStandardFontFamily(const QString &family);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onLoadFinished(bool);    

private:
    quintptr viewId;
    QWebEngineView *view;
    QWidget *container;
    QWindow *containerWindow;
    QWindow *window;
    WebPage *m_page;
    LocalSchemeHandler *m_localHandler;
};

class LocalSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT
public:
    LocalSchemeHandler(QObject *parent) : QWebEngineUrlSchemeHandler(parent)
    {
        Q_UNUSED(parent);
    }

    static const QString &scheme();
    static const QMimeDatabase &mimeDatabase();
    void requestStarted(QWebEngineUrlRequestJob *job) override;
};

#endif // AMNEZIAWEBVIEW_WEBENGINE_P_H

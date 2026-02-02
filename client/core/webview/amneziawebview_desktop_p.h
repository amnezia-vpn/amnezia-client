#ifndef AMNEZIAWEBVIEW_DESKTOP_P_H
#define AMNEZIAWEBVIEW_DESKTOP_P_H

// QtWebKit is deprecated and not available in Qt 6
// This file should only be used with Qt 5 when WebEngineWidgets is not available
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#error "amneziawebview_desktop_p.h uses QtWebKit which is not available in Qt 6. Use amneziawebview_webengine_p.h instead."
#endif

#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebSettings>

#include "amneziawebview.h"
#include "amneziawebview_p.h"

class DesktopWebViewPrivate;

class WebPage : public QWebPage
{
    Q_OBJECT

signals:
    void loadingUrl(const QUrl &url);

public:
    explicit WebPage(QObject *parent = nullptr);
    virtual ~WebPage();

protected:
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
    virtual void javaScriptAlert(QWebFrame *frame, const QString& msg);

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
    virtual void takeSnapshot();
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
    virtual void setTextZoom(int percent) { Q_UNUSED(percent); }
    virtual void setStandardFontFamily(const QString &family);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onLoadFinished(bool);    

private:
    quintptr viewId;
    QWebView *view;
    QWidget *container;
    QWindow *containerWindow;
    QWindow *window;
};


#endif // AMNEZIAWEBVIEW_DESKTOP_P_H

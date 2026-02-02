#include <QtCore/qglobal.h>
#include <QtCore>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <jni.h>
#include <android/bitmap.h>
#include <stdlib.h>
#include <string.h>

#include "amneziawebview_p.h"
#include "qrchandler.h"
#include "filehandler.h"

#include <QJniObject>
#include <QJniEnvironment>

namespace Jni
{

using Object = QJniObject;
}
static const char qtAndroidWebViewControllerClass[] = "org/amnezia/vpn/WebViewController";

class AndroidWebViewPrivate;

typedef QMap<quintptr, AndroidWebViewPrivate *> WebViews;
Q_GLOBAL_STATIC(WebViews, g_webViews)
Q_GLOBAL_STATIC(QMutex, g_webMutex)

class AndroidWebViewPrivate : public AmneziaWebViewPrivate
{
    Q_DECLARE_PUBLIC(AmneziaWebView)

public:

    explicit AndroidWebViewPrivate(AmneziaWebView* q);
    virtual ~AndroidWebViewPrivate();

    static AndroidWebViewPrivate *get(AmneziaWebView *q)
    {
        return static_cast<AndroidWebViewPrivate*>(AmneziaWebViewPrivate::get(q));
    }

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
    virtual void setUrl(const QUrl &url);
    virtual QIcon icon() const;
    virtual void setScale(qreal scale);
    virtual qreal scale() const;
    virtual QSize contentsSize() const;
    virtual void setDefaultFontSize(int size);
    virtual void setStandardFontFamily(const QString &family);
    virtual void setTextZoom(int percent);
private:

    quintptr viewId;
    Jni::Object m_viewController;
};

AndroidWebViewPrivate::AndroidWebViewPrivate(AmneziaWebView* q): AmneziaWebViewPrivate(q),
    viewId(reinterpret_cast<quintptr>(this))
{
    m_viewController = Jni::Object(qtAndroidWebViewControllerClass,
                                   "(Landroid/app/Activity;J)V",
                                   QNativeInterface::QAndroidApplication::context().object(),
                                   viewId);
    QMutexLocker lock(g_webMutex());
    g_webViews->insert(viewId, this);
    setBackgroundColor(backgroundColor);
}

AndroidWebViewPrivate::~AndroidWebViewPrivate()
{
    QMutexLocker lock(g_webMutex());
    m_viewController.callMethod<void>("release", "()V");
    g_webViews->take(viewId);
}

AmneziaWebViewPrivate *AmneziaWebViewPrivate::create(AmneziaWebView *q)
{
    return new AndroidWebViewPrivate(q);
}

void AndroidWebViewPrivate::setWindowParent(QWindow *parent)
{
    Q_UNUSED(parent);
}


void AndroidWebViewPrivate::setBackgroundColor(const QColor backgroundColor)
{
    m_viewController.callMethod<void>("setBackgroundColor", "(I)V",
            jint(backgroundColor.rgb()));
    emit backgroundColorChanged();
}


bool AndroidWebViewPrivate::isLoading() const
{
    return false;
}

/// Deprecated
void AndroidWebViewPrivate::setScale(qreal scale)
{
    Q_UNUSED(scale);
}

/// Deprecated
qreal AndroidWebViewPrivate::scale() const
{
    return 1;
}

QIcon AndroidWebViewPrivate::icon() const
{
    return QIcon();
}

QSize AndroidWebViewPrivate::contentsSize() const
{
    return QSize();
}

void AndroidWebViewPrivate::setGeometry(const QRect &geometry)
{
    if (this->geometry != geometry) {
        this->geometry = geometry;

        m_viewController.callMethod<void>("setGeometry", "(IIII)V",
                        jint(geometry.x()), jint(geometry.y()),
                        jint(geometry.width()), jint(geometry.height()) );
    }
}

void AndroidWebViewPrivate::setTextZoom(int percent)
{
    m_viewController.callMethod<void>("setTextZoom", "(I)V", jint(percent));
}

void AndroidWebViewPrivate::hide()
{
    Q_Q(AmneziaWebView);
    if (visible) {
        m_viewController.callMethod<void>("hide", "()V");
        visible = false;
        q->update();
    }
}

void AndroidWebViewPrivate::show()
{
    if (!visible) {
        m_viewController.callMethod<void>("show", "()V");
        visible = true;
    }
}


void AndroidWebViewPrivate::setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl)
{
    Q_UNUSED(data);
    Q_UNUSED(mimeType);
    Q_UNUSED(baseUrl);
}

void AndroidWebViewPrivate::setUrl(const QUrl &url)
{
    AmneziaWebViewPrivate::setUrl(url);
}

void AndroidWebViewPrivate::load(const QUrl &url)
{
    // Make WebView visible before loading
    if (!visible) {
        show();
    }
    Jni::Object jurl = Jni::Object::fromString(url.isValid() ? url.toString() : QString("about:blank"));
    m_viewController.callMethod<void>("loadUrl", "(Ljava/lang/String;)V", jurl.object<jstring>());
}

void AndroidWebViewPrivate::setHtml(const QString &html, const QUrl &baseUrl)
{
    if (html.isNull()) {
        return;
    }

    Jni::Object url = Jni::Object::fromString(baseUrl.isValid() ? baseUrl.toString() : QString("about:blank"));
    Jni::Object data = Jni::Object::fromString(html);
    Jni::Object mime = Jni::Object::fromString(QString("text/html"));
    Jni::Object encoding = Jni::Object::fromString(QString("utf-8"));

    m_viewController.callMethod<void>("loadDataWithBaseURL", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                      url.object<jstring>(), data.object<jstring>(), mime.object<jstring>(), encoding.object<jstring>());
}

void AndroidWebViewPrivate::evaluateJavaScript(const QString &scriptSource)
{
    Jni::Object script = Jni::Object::fromString(scriptSource);
    m_viewController.callMethod<void>("evaluateJavaScript", "(Ljava/lang/String;)V", script.object<jstring>());
}

bool AndroidWebViewPrivate::canGoBack() const
{
    jboolean can  = m_viewController.callMethod<jboolean>("canGoBack", "()Z");
    return can;
}

bool AndroidWebViewPrivate::canGoForward() const
{
    jboolean can  = m_viewController.callMethod<jboolean>("canGoForward", "()Z");
    return can;
}

void AndroidWebViewPrivate::back()
{
    m_viewController.callMethod<void>("goBack", "()V");
}

void AndroidWebViewPrivate::forward()
{
    m_viewController.callMethod<void>("goForward", "()V");
}

void AndroidWebViewPrivate::reload()
{
}

void AndroidWebViewPrivate::stop()
{
}

QString AndroidWebViewPrivate::innerHTML() const
{
    return QString();
}

void AndroidWebViewPrivate::setDefaultFontSize(int size)
{
    m_viewController.callMethod<void>("setDefaultFontSize", "(I)V", jint(size));
}

void AndroidWebViewPrivate::setStandardFontFamily(const QString &family)
{
    Jni::Object fontFamily = Jni::Object::fromString(family);
    m_viewController.callMethod<void>("setStandardFontFamily", "(Ljava/lang/String;)V", fontFamily.object<jstring>());
}

extern "C" {

JNIEXPORT void Java_org_amnezia_vpn_WebViewController_pageStarted(JNIEnv *env, jobject obj, jlong viewId, jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);

    QMutexLocker lock(g_webMutex());
    AndroidWebViewPrivate *view = g_webViews()->value(viewId);
    if (view) {
        const char *urlChars = env->GetStringUTFChars(url, 0);
        const QUrl url = QUrl(QString(urlChars));
        QMetaObject::invokeMethod(view, "onPageStarted", Qt::QueuedConnection);
    }
}

JNIEXPORT void Java_org_amnezia_vpn_WebViewController_pageFinished(JNIEnv *env, jobject obj, jlong viewId, jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    QMutexLocker lock(g_webMutex());
    AndroidWebViewPrivate *view = g_webViews()->value(viewId);
    if (view) {
        const char *urlChars = env->GetStringUTFChars(url, 0);
        const QUrl url = QUrl(QString(urlChars));
        QMetaObject::invokeMethod(view, "onPageFinished", Qt::QueuedConnection);
    }
}

JNIEXPORT void Java_org_amnezia_vpn_WebViewController_urlChanged(JNIEnv *env, jobject obj, jlong viewId, jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    QMutexLocker lock(g_webMutex());
    AndroidWebViewPrivate *view = g_webViews()->value(viewId);
    if (view) {

        const char *urlChars = env->GetStringUTFChars(url, 0);
        const QUrl url = QUrl(QString(urlChars));
        QMetaObject::invokeMethod(view, "onUrlChanged", Qt::QueuedConnection, Q_ARG( const QUrl, url));
    }
}

JNIEXPORT jbyteArray Java_org_amnezia_vpn_WebViewController_dataForUrl(JNIEnv *env, jobject obj, jlong viewId, jstring url, jobject mimeType, jobject encoding)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    QMutexLocker lock(g_webMutex());
    AndroidWebViewPrivate *view = g_webViews()->value(viewId);
    if (view) {

        const char *urlChars = env->GetStringUTFChars(url, 0);
        const QUrl url = QUrl(QString(urlChars));

        QByteArray buffer = view->dataForUrl(url);
        QString mime = view->mimeTypeForUrl(url);
        QString enc("utf-8");

        jstring jMimeType = env->NewStringUTF(mime.toUtf8().constData());
        Jni::Object jMimeTypeObject(mimeType);
        if (jMimeTypeObject.isValid()) {
            jMimeTypeObject.callObjectMethod("insert", "(ILjava/lang/String;)Ljava/lang/StringBuilder;", 0, jMimeType);
        }

        jstring jEncoding = env->NewStringUTF(enc.toUtf8().constData());
        Jni::Object jEncodingObject(encoding);
        if (jEncodingObject.isValid()) {

            jEncodingObject.callObjectMethod("insert", "(ILjava/lang/String;)Ljava/lang/StringBuilder;", 0, jEncoding);
        }

        env->DeleteLocalRef(jEncoding);
        env->DeleteLocalRef(jMimeType);

        jbyteArray data = env->NewByteArray(buffer.size());
        env->SetByteArrayRegion(data, 0, buffer.size(), (const jbyte*) buffer.data());
        return data;
    }

    return NULL;
}

JNIEXPORT jboolean Java_org_amnezia_vpn_WebViewController_canHandleUrl(JNIEnv *env, jobject obj, jlong viewId, jstring url)

{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    QMutexLocker lock(g_webMutex());
    AndroidWebViewPrivate *view = g_webViews()->value(viewId);
    if (view) {
        const char *urlChars = env->GetStringUTFChars(url, 0);
        const QUrl url = QUrl(QString(urlChars));
        return view->canHandleUrl(url);
    }

    return jboolean(false);
}


}

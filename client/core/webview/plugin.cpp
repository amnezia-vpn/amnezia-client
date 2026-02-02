#include "plugin.h"

#include "amneziawebview.h"

QT_BEGIN_NAMESPACE

void WebViewPlugin::registerTypes(const char* uri)
{
#ifndef QT_NO_ACTION
    qmlRegisterAnonymousType<QAction>(uri, 1);
#endif
    qmlRegisterAnonymousType<AmneziaWebViewSettings>(uri, 1);
    qmlRegisterType<AmneziaWebView>(uri, 1, 0, "AmneziaWebView");
    qmlRegisterRevision<AmneziaWebView, 0>("AmneziaWebView", 1, 0);
}

QT_END_NAMESPACE

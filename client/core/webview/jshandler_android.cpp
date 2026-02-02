#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QVariant>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QGenericArgument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include "amneziawebview_p.h"
#include "jshandler.h"
#include "mimecache.h"

JsHandler::JsHandler(AmneziaWebView *host): _host(host)
{
	init();
}

JsHandler::~JsHandler() {}


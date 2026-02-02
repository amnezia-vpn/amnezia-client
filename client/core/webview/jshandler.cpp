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
#include "mimecache.h"
#include "jshandler.h"

QString JsHandler::scheme() const
{
	return QString("js");
}

QString JsHandler::host() const
{
	qulonglong h = (qulonglong)(void*)_host;
	return QString("js%1").arg(h);
}

QString JsHandler::scriptObjectsUrl() const
{
    return QString("%1://%2/%3.js").arg(scheme()).arg(host()).arg(scriptObjectsId());
}

QString JsHandler::scriptObjectsId() const
{
    return QString("__scriptObjects__");
}

QString JsHandler::scriptObjects() const
{
    QString script;
    QList<QString> scripts = scriptParts.values();

    for ( int i = 0; i < scripts.count(); i++) {
        script = QString("%1 %2").arg(script).arg(scripts.at(i)).trimmed();
    }
    return script;
}

void JsHandler::updateWebView()
{
    if (!scriptObjectsInjected) {

        QString injector = QString(
                                  " var script = document.getElementById(\"%1\"); "
                                  " if(script === null) { "
                                  "   script = document.createElement('script'); "
                                  "   script.type = \"text/javascript\"; "
                                  "   script.id = \"%2\"; "
                                  "   document.getElementsByTagName('head')[0].appendChild(script); "
                                  " } "
                                  " script.text = \"%3\"; "
                                  ).arg(scriptObjectsId()).arg(scriptObjectsId()).arg(scriptObjects());



        /*
        QString injector = QString(
                                 " var script = document.getElementById(\"%1\"); "
                                 " if(script === null) { "
                                 "   script = document.createElement('script'); "
                                 "   script.type = \"text/javascript\"; "
                                 "   script.id = \"%2\"; "
                                 "   document.getElementsByTagName('head')[0].appendChild(script); "
                                 " } "
                                 " script.src = \"%3\"; "
                                 ).arg(scriptObjectsId()).arg(scriptObjectsId()).arg(scriptObjectsUrl());

		*/

        //убрали, так как теперь есть рабочий вебинспектор
        //_host->evaluateJavaScript(injector);
        scriptObjectsInjected = true;
	}
}

void JsHandler::addToJavaScriptWindowObject(const QString& name, QObject* object)
{
    windowObjects[name] = object;
    QString script = windowScriptObject(name, object);
    scriptParts[name] = script;
}

bool JsHandler::canHandleUrl(const QUrl &url) const
{
	if (scheme() == url.scheme() && url.host() == host())
		return true;

	if (this->host() == url.host() && (url.scheme() == QString("http")))
		return true;

	return false;
}

QString JsHandler::mimeTypeForUrl(const QUrl &url) const
{
	if (scheme() == url.scheme() && url.host() == host())
		return mimeTypeForUrl(url);

	if (this->host() == url.host() && (url.scheme() == QString("http")))
		return mimeTypeForExtension("json");

	return QString("application/octet-stream");
}

QString JsHandler::windowScriptObject(const QString& name, QObject* object) const
{
	QString script = QString(" var %1 = {}; "
                             " %2.responseText = null; "
                             ).arg(name).arg(name);
	if (object) {
		const QMetaObject *meta = object->metaObject();
		const QString prefix = QString("%1.").arg(name);

		int methodCount = meta->methodCount();
		for (int i = 0; i < methodCount; i++) {
			QMetaMethod metaMethod = meta->method(i);

			if ((metaMethod.access() == QMetaMethod::Public) && ((metaMethod.methodType() == QMetaMethod::Slot) ||
                                                                 (metaMethod.methodType() == QMetaMethod::Method)) )  {
				const QString methodName = QString("%1%2").arg(prefix).arg(QString(metaMethod.name()));

				//Parameters
				QList<QByteArray> parametersNames = metaMethod.parameterNames();
				QString methodParameters = QString("");
				QString stringifyParameters = QString("");

				for (int j = 0; j < parametersNames.count(); j++) {
					methodParameters += QString(parametersNames.at(j));

					stringifyParameters += QString("'%1=' + encodeURIComponent(%2)").arg(QString(parametersNames.at(j)))
                    .arg(QString(parametersNames.at(j)));

					if(j < (parametersNames.count() -1)) {
						methodParameters += QString(", ");
						stringifyParameters += QString(" + '&' + ");
					}
				}
				if (stringifyParameters.length() > 0)
					stringifyParameters = QString(" + '?'+ %1").arg(stringifyParameters);

				const QString methodUrl = QString("http://%1/%2").arg(host()).arg(methodName);
				//Body
				const QString bodyTemplate = QString(""
                                                     " var obj = this; "
                                                     " if (obj.responseText !== null) { "
                                                     "    var ret = eval( obj.responseText ); "
                                                     "    obj.responseText = null; "
                                                     "    return ret;"
                                                     " }; "
                                                     " var caller = arguments.callee.caller; "
                                                     " var callerArgs = caller.arguments; "
                                                     " var xhr = new XMLHttpRequest; "
													 " xhr.onload=function(){ "
                                                     "   if (xhr.status == 200) { "
                                                     "       obj.responseText = xhr.responseText;  "
                                                     "       if (obj.responseText == null) { "
                                                     "            obj.responceText = ''; "
                                                     "       } "
                                                     "       caller(callerArgs); "
                                                     "    }; "
                                                     " }; "
                                                     " xhr.open('GET', '%1'%2, true); "
													 //" xhr.setRequestHeader('Access-Control-Allow-Origin', '*'); "
												     //" xhr.setRequestHeader('Access-Control-Allow-Headers', 'Content-Type'); "
                                                     " xhr.send(null);"
                                                     ).arg(methodUrl).arg(stringifyParameters);


				QString methodBody = QString("%1").arg(bodyTemplate);

                script = script +  QString("%1=function(%2){ %3 }; " )
                .arg(methodName)
                .arg(methodParameters)
                .arg(methodBody);

			}
		}
	}
	return script;
}


const QString createPopup = QString( ""
                                    "    window.createPopup=function(){ "
                                    "        var popup=document.createElement('iframe'), "
                                    "        isShown=false, popupClicked=false; "
                                    "       popup.src='about:blank'; "
                                    "        popup.style.position='absolute'; "
                                    "        popup.style.border='0px'; "
                                    "        popup.style.display='none'; "
                                    "        popup.addEventListener('load', function(e){ "
                                    "            popup.document=(popup.contentWindow || popup.contentDocument); "
                                    "            if(popup.document.document) popup.document=popup.document.document; "
                                    "        }); "
                                    "        document.body.appendChild(popup); "
                                    "        var hidepopup=function(event){ "
                                    "            if(isShown) "
                                    "                setTimeout(function(){ "
                                    //"                    if(!popupClicked){ "
                                    "                        popup.hide(); "
                                    //"                    } "
                                    "                    popupClicked=false; "
                                    "                }, 150); "
                                    "        }; "
                                    "        popup.show=function(x, y, w, h, pElement){ "
                                    "            if(typeof(x) !== 'undefined'){ "
                                    "               var elPos=[0, 0]; "
                                    "                if(pElement) elPos=findPos(pElement); "
                                    "                elPos[0]+=y, elPos[1]+=x; "
                                    "                if(isNaN(w)) w=popup.document.scrollWidth; "
                                    "                if(isNaN(h)) h=popup.document.scrollHeight; "
                                    "                if(elPos[0] + w > document.body.clientWidth) elPos[0]=document.body.clientWidth - w - 5; "
                                    "                if(elPos[1] + h > document.body.clientHeight) elPos[1]=document.body.clientHeight - h - 5; "
                                    "                popup.style.left=elPos[0] + 'px'; "
                                    "                popup.style.top=elPos[1] + 'px'; "
                                    "                popup.style.width=(w + 'px'); "
                                    "                popup.style.height=(h + 'px'); "
                                    "            } "
                                    "            popup.style.display='block'; "
                                    "            isShown=true; "
                                    "        }; "
                                    "        popup.hide=function(){ "
                                    "            isShown=false; "
                                    "            popup.style.display='none'; "
                                    "        }; "
                                    "        window.addEventListener('click', hidepopup, true); "
                                    "        window.addEventListener('blur', hidepopup, true); "
                                    "        return popup; "
                                    "    }; "
                                    "    function findPos(obj, foundScrollLeft, foundScrollTop) { "
                                    "        var curleft = 0; "
                                    "        var curtop = 0; "
                                    "        if(obj.offsetLeft) curleft += parseInt(obj.offsetLeft); "
                                    "        if(obj.offsetTop) curtop += parseInt(obj.offsetTop); "
                                    "        if(obj.scrollTop && obj.scrollTop > 0) { "
                                    "            curtop -= parseInt(obj.scrollTop); "
                                    "            foundScrollTop = true; "
                                    "        } "
                                    "        if(obj.scrollLeft && obj.scrollLeft > 0) { "
                                    "            curleft -= parseInt(obj.scrollLeft); "
                                    "            foundScrollLeft = true; "
                                    "        } "
                                    "        if(obj.offsetParent) { "
                                    "            var pos = findPos(obj.offsetParent, foundScrollLeft, foundScrollTop); "
                                    "            curleft += pos[0]; "
                                    "            curtop += pos[1]; "
                                    "        } else if(obj.ownerDocument) { "
                                    "            var thewindow = obj.ownerDocument.defaultView; "
                                    "            if(!thewindow && obj.ownerDocument.parentWindow) "
                                    "                thewindow = obj.ownerDocument.parentWindow; "
                                    "            if(thewindow) { "
                                    "                if (!foundScrollTop && thewindow.scrollY && thewindow.scrollY > 0) curtop -= parseInt(thewindow.scrollY); "
                                    "                if (!foundScrollLeft && thewindow.scrollX && thewindow.scrollX > 0) curleft -= parseInt(thewindow.scrollX); "
                                    "                if(thewindow.frameElement) { "
                                    "                    var pos = findPos(thewindow.frameElement); "
                                    "                    curleft += pos[0]; "
                                    "                    curtop += pos[1]; "
                                    "                } "
                                    "            }"
                                    "        }"
                                    "        return [curleft,curtop]; "
                                    "    } " );


void JsHandler::init()
{
    /*
     QString consoleObject = createPopup + QString (
     "function popup(msg){ "
     " var p = window.createPopup(); "
     " var pbody = p.document.body; "
     " pbody.style.backgroundColor='lime'; "
     " pbody.style.border='solid black 1px'; "
     " pbody.innerHTML=msg; "
     " p.show(NaN,NaN,NaN,NaN, document.body); }"
     " window.console={ "
     " log=function(msg){ popup(msg); }"
     " warning=function(msg){ popup(msg); }"
     " error=function(msg){ popup(msg); }"
     " info=function(msg){ popup(msg); }"
     " }");
     */

    QString consoleObject = QString ( ""
                                      " webviewPopup=function(msg){ "
                                      " var p = window.createPopup(); "
                                      " var pbody = p.document.body; "
                                      " pbody.style.backgroundColor='white'; "
                                      " pbody.style.border='solid black 1px'; "
                                      " pbody.innerHTML=msg + '  ' + document.body.clientWidth; "
                                      " p.show(0, 0, document.body.clientWidth, NaN, document.body); }; "
                                      " window.console.log=function(msg){ webviewPopup(msg); };"
                                      " window.console.warning=function(msg){ webviewPopup(msg); };"
                                      " window.console.error=function(msg){ webviewPopup(msg); };"
                                      " window.console.info=function(msg){ webviewPopup(msg); };"
                                    ) + createPopup;
 /*
    QString object = QString( " var script = document.getElementById(\"consoleScriptObject\"); "
                             " if(script === null) { "
                             "   script = document.createElement('script'); "
                             "   script.type = \"text/javascript\";"
                             "   script.text = \"%1\";"
                             "   script.id = \"consoleScriptObject\"; "
                             "   document.getElementsByTagName('head')[0].appendChild(script); "
                             " } "
                             ).arg(consoleObject);
  */

    scriptParts["__console__"] = consoleObject;
}

QByteArray JsHandler::dataForUrl(const QUrl &url) const
{
	QByteArray buffer;
	QVariant ret = callMethodForUrl(url);
	if (ret.isValid()) {

		QJsonValue value = QJsonValue::fromVariant(ret);
		QJsonArray jsonArray;
		jsonArray.append(value);
		QJsonDocument  jsonDoc(jsonArray);
		buffer = jsonDoc.toJson();
	}
	return buffer;
}


// Method call specified by url of type: http://scripthost/object.methodname?paramName1=paramValue1&paramName2=paramValue2&...
QVariant JsHandler::callMethodForUrl(const QUrl &url) const
{
	qDebug() << url.toString();
	qDebug() << "Path: " << url.path();
	qDebug() << "Query: " << url.query();

	QVariant ret;
	QStringList objectPath = url.path().trimmed().remove('/').split(".");
	QStringList query;
	if (url.query().trimmed().length() > 0)
		query = url.query().trimmed().split("&");
	QList<QGenericArgument> arguments;
	QList<QByteArray> parametersNames;
	QList<QByteArray> parametersTypes;
	QString methodName;
	QString signature;

	AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(_host);

	if (d && objectPath.count() == 2) {
		qDebug() << "Called method: " << objectPath[0] << "."<< objectPath[1];
		QObject *object = d->jsHandler.windowObjects.value(objectPath[0]);
		if (object) {
			int methodCount = object->metaObject()->methodCount();
			int methodIndex = -1;
			for(int i = 0; i < methodCount; i++) {
				const QMetaMethod method = object->metaObject()->method(i);
				parametersNames = method.parameterNames();
				methodName = method.name();
				if ((query.count() == parametersNames.count()) && (methodName == objectPath[1])) {
					methodIndex = i;
					parametersTypes = method.parameterTypes();
					break;
				}
			}

			if (methodIndex >= 0) {
				const QMetaMethod method = object->metaObject()->method(methodIndex);
				QVariantList params;
				for (int i = 0; i < query.count(); i++) {
					QStringList param = query[i].split('=');

					params.append(QVariant(QString(param[1].toLocal8Bit())));
					QGenericArgument arg(param[0].toLocal8Bit().constData(), &params[i]);
					arguments.append(arg);
				}
				switch (arguments.count()) {
					case 1:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0]);
						break;

					case 2:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1]);
						break;

					case 3:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2]);
						break;

					case 4:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2],
								arguments[3]);
						break;

					case 5:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2],
								arguments[3], arguments[4]);
						break;
					case 6:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2],
								arguments[3], arguments[4], arguments[5]);
						break;

					case 7:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2],
								arguments[3], arguments[4], arguments[5], arguments[6]);
						break;

					case 8:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret), arguments[0], arguments[1], arguments[2],
								arguments[3], arguments[4], arguments[5], arguments[6], arguments[7]);
						break;

					default:
                        method.invoke(object, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret));
				}
			}
		}
	}

	return ret;
}


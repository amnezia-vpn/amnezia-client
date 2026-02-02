#ifndef JSHANDLER_H
#define JSHANDLER_H

class QUrl;
class QVariant;
class AmneziaWebView;

class JsHandler
{
    friend class AmneziaWebView;
    
public:
    explicit JsHandler(AmneziaWebView *host);
    virtual ~JsHandler();
    
    bool canHandleUrl(const QUrl &url) const;
    QByteArray dataForUrl(const QUrl &url) const;
    void addToJavaScriptWindowObject(const QString& name, QObject* object);
    QString mimeTypeForUrl(const QUrl &url) const;
    
    void updateWebView();
    QString host() const;
    QString scheme() const;
    QString scriptObjectsUrl() const;
    QString scriptObjectsId() const;
    QString scriptObjects() const;
    
private:
   
    QVariant callMethodForUrl(const QUrl &url) const;
    QString windowScriptObject(const QString& name, QObject* object) const;
    
    void init();
    AmneziaWebView *_host;
    bool scriptObjectsInjected;
    
    QHash<QString, QObject*> windowObjects;
    QHash<QString, QString> scriptParts;
};

#endif

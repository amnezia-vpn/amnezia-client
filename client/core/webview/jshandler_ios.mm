#import <Foundation/Foundation.h>
#import <MobileCoreServices/UTCoreTypes.h>
#import <MobileCoreServices/UTType.h>

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QRect>
#include <QColor>
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
#import "jshandler.h"

typedef QHash<QString, AmneziaWebView*> JavaScriptHosts;
Q_GLOBAL_STATIC(JavaScriptHosts, hosts);

#if !defined(ENABLE_WKWEBVIEW)

@interface JsProtocol : NSURLProtocol
+ (NSString*) requestVarsKey;
@end

@interface NSURLRequest (JsProtocol)
- (NSDictionary *)requestVars;
@end

@interface NSMutableURLRequest (JsProtocol)
- (void)setRequestVars:(NSDictionary *)vars;
@end

#endif

JsHandler::JsHandler(AmneziaWebView *h): _host(h)
    , scriptObjectsInjected(false)
{
#if !defined(ENABLE_WKWEBVIEW)
    static bool protocolRegistered = false;
    if (!protocolRegistered) {
        [NSURLProtocol registerClass:[JsProtocol class]];
        protocolRegistered = true;
    }
#endif
    QString key = host();
    if (!hosts()->keys().contains(key)) {
        hosts()->insert(key, h);
    }
    init();
}

JsHandler::~JsHandler()
{
    QString key = host();
    if (hosts()->keys().contains(key)) {
        hosts()->remove(key);
    }
}

#if !defined(ENABLE_WKWEBVIEW)

@implementation NSURLRequest (JsProtocol)

- (NSDictionary *)requestVars {
	NSLog(@"%@ received %@", self, NSStringFromSelector(_cmd));
	return [NSURLProtocol propertyForKey:[JsProtocol requestVarsKey] inRequest:self];
}

@end


@implementation NSMutableURLRequest (JsProtocol)

- (void)setRequestVars:(NSDictionary *)requestVars {
    
	NSLog(@"%@ received %@", self, NSStringFromSelector(_cmd));
	
	NSDictionary *specialVarsCopy = [requestVars copy];
	[NSURLProtocol setProperty:specialVarsCopy forKey:[JsProtocol requestVarsKey] inRequest:self];
    [specialVarsCopy release];
}

@end


@implementation JsProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
    //NSString *url = request.URL.absoluteString;
    QString host = QString::fromNSString(request.URL.host).toLower();
    if (hosts()->keys().contains(host))
        return YES;
    
    //NSLog(@"Requested Url: %@", url);
    return NO;
}

+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
    return request;
}

+ (NSString*) requestVarsKey
{
	return @"requestVars";
}

- (void)startLoading
{
    QString host = QString::fromNSString(self.request.URL.host);
    AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(hosts()->value(host));
    NSURLRequest *request = [self request];
    
    if ([request.URL.absoluteString isEqualToString: d->jsHandler.scriptObjectsUrl().toNSString()]) {
        
        //NSString *mimeType = mimeTypeForExtension(QString::fromNSString(request.URL.path.pathExtension)).toNSString();
        
        NSString *mimeType = d->jsHandler.mimeTypeForUrl(QUrl::fromNSURL(request.URL)).toNSString();
        QByteArray buffer = d->jsHandler.scriptObjects().toLocal8Bit();
        
        //NSData *data = [[NSData alloc] initWithBytes:buffer.constData() length:buffer.size()];
        NSData *data = [NSData dataWithBytes:buffer.constData() length:buffer.size()];
        
        NSURLResponse *response =[[NSURLResponse alloc]initWithURL:self.request.URL
                                                         MIMEType:mimeType
                                            expectedContentLength:[data length]
                                                 textEncodingName:@"utf-8"];
    
            
        [[self client] URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
        [[self client] URLProtocol:self didLoadData:data];
        [[self client] URLProtocolDidFinishLoading:self];
        [response autorelease];
    
        return;
    }
    
    if ( d && [request.URL.scheme isEqualToString:@"http"]
        && [request.URL.host isEqualToString:host.toNSString()]) {
        
        const QByteArray buffer = d->dataForUrl(QUrl::fromNSURL(request.URL));
        //NSData *data = data = [[NSData alloc] initWithBytes:buffer.constData() length:buffer.size()];
        NSData *data = data = [NSData dataWithBytes:buffer.constData() length:buffer.size()];
        
        //NSString *mimeType = mimeTypeForExtension(QString("json")).toNSString();
        NSString *mimeType = d->jsHandler.mimeTypeForUrl(QUrl::fromNSURL(request.URL)).toNSString();
        
        NSDictionary *headers = @{@"Access-Control-Allow-Origin" : @"*",
                                  @"Access-Control-Allow-Headers" : @"Content-Type",
                                  @"Cache-Control" : @"no-cache",
                                  @"Content-Type" : [NSString stringWithFormat:@"%@; %@", mimeType, @"charset=UTF-8"] };
        
        NSHTTPURLResponse* response = [[NSHTTPURLResponse alloc] initWithURL:request.URL
                                                                  statusCode:200
                                                                 HTTPVersion:@"HTTP/1.1"
                                                                headerFields:headers];
        
        [self.client URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
        [self.client URLProtocol:self didLoadData:data];
        [self.client URLProtocolDidFinishLoading:self];
        [response autorelease];
        return;
    }
    else {
        [[self client] URLProtocol:self didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist userInfo:nil]];
    }
}

- (void)stopLoading
{
}

@end

#endif

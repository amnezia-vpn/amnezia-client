#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <WebKit/WebKit.h>

#include <QEvent>
#include <QWindowStateChangeEvent>
#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>
#include <QDebug>
#include <QIcon>
#include <QtCore/QVariant>
#include <QScreen>
#include <QWindow>
#include <QPainter>
#include <QPaintDevice>
#include <QtCore/QMutex>
#include <QtCore/QMetaMethod>
#include <QPaintEngine>
#include <QOpenGLPaintDevice>
#include <QPixmap>
#include <QSize>
#include <QImage>
#include <QPaintDevice>
#include <QFileInfo>

#include "amneziawebview.h"
#include "amneziawebview_p.h"
#include "mimecache.h"
#include "qrchandler.h"
#include "filehandler.h"

static inline CGRect toCGRect(const QRectF &rect)
{
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

#if defined(ENABLE_WKWEBVIEW)

@interface WebSchemeHandler : NSObject <WKURLSchemeHandler>
@property AmneziaWebView *view;
@end

@interface WebDelegate: NSObject <WKNavigationDelegate>
@property AmneziaWebView *view;
@end

#else

@interface WebDelegate: NSObject <UIWebViewDelegate>
@property AmneziaWebView *view;
@end

#endif

class IosWebViewPrivate : public AmneziaWebViewPrivate
{
    Q_DECLARE_PUBLIC(AmneziaWebView)
public:

    IosWebViewPrivate(AmneziaWebView* q);
    virtual ~IosWebViewPrivate();

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
    void getSnapshot();

    WebDelegate *webDelegate;

#if defined(ENABLE_WKWEBVIEW)
    WebSchemeHandler *schemeHandler;
    WKWebViewConfiguration *configuration;
    WKWebView *webView;
    QString _innerHTML;
#else
    UIWebView *webView;
#endif
    NSString *_defaultUrl;
};

#if defined(ENABLE_WKWEBVIEW)

@implementation WebSchemeHandler
+ (NSString *)charsetFromHtml:(NSString *)html
{
    if (!html) return nil;

    NSString *charset = nil;
    NSString *charsetPattern = @"((?<=charset=)\\s*[a-zA-Z0-9-]*)";
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:charsetPattern options: NSRegularExpressionCaseInsensitive error:nil];
    NSTextCheckingResult *charsetResult = [regex firstMatchInString:html options:kNilOptions range:NSMakeRange(0, [html length])];
    if (charsetResult && charsetResult.range.location != NSNotFound) {
        charset = [[html substringWithRange:[charsetResult range]] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
        charset = [charset lowercaseString];
    }
    return charset;
}

+ (NSString*)headFromHtml:(NSString*)html
{
    if (!html) return nil;

    NSString *head = nil;
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"(?<=<head>)[\\w\\W.]*(?=</head>)" options:NSRegularExpressionCaseInsensitive error:nil];
    NSTextCheckingResult *headResult = [regex firstMatchInString:html options:0 range:NSMakeRange(0, html.length)];

    if (headResult && headResult.range.location != NSNotFound) {
        head = [html substringWithRange:[headResult range]];
    }
    return head;
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask
{
    NSURLRequest *request = urlSchemeTask.request;
    AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(self.view);
    if (!d) return;

    NSString *url = [[request URL] path];
    NSString *mimeType = mimeTypeForExtension(QString::fromNSString(url.pathExtension)).toNSString();

    QByteArray buffer;
    QUrl requestUrl = QUrl::fromNSURL(request.URL);

    if (d->qrcHandler.canHandleUrl(requestUrl)) {
        buffer = d->qrcHandler.dataForUrl(requestUrl);
    }
    else if (requestUrl.scheme().compare(d->jsHandler.scheme()) == 0) {
        buffer = d->jsHandler.dataForUrl(requestUrl);
    }
    else if (d->fileHandler.canHandleUrl(requestUrl)) {
        buffer = d->fileHandler.dataForUrl(requestUrl);
    }

    if (!buffer.isEmpty()) {

        NSData *pageData = [NSData dataWithBytes:buffer.constData() length:buffer.size()];
        NSString *encoding = @"utf-8";
        if ([mimeType isEqualToString:@"text/html"]) {

            NSString *content = [[NSString alloc] initWithBytesNoCopy: (char* )[pageData bytes] length:pageData.length encoding:NSISOLatin1StringEncoding freeWhenDone:NO];

            NSString *charset = [WebSchemeHandler charsetFromHtml:[WebSchemeHandler headFromHtml:content]];
            if (charset) {
                encoding = charset;
            }
            [content release];
        }

        NSURLResponse *response = [[NSURLResponse alloc]initWithURL:request.URL
                                                          MIMEType:mimeType
                                             expectedContentLength:[pageData length]
                                                  textEncodingName:encoding];

        [urlSchemeTask didReceiveResponse:response];
        [urlSchemeTask didReceiveData:pageData];
        [urlSchemeTask didFinish];
        [response release];
    }
    else {
        [urlSchemeTask didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist userInfo:nil]];
    }
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask
{
    Q_UNUSED(webView);
}

@end

@implementation WebDelegate

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
    Q_UNUSED(webView);

    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    if([error code] == NSURLErrorCancelled) return;

    AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(self.view);
    if (d) {
        d->lastError = QString::fromNSString(error.localizedDescription);
        QMetaObject::invokeMethod(d, "onPageFinished", Qt::QueuedConnection);
    }
}

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
    NSString *path = [webView.URL.path stringByTrimmingCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"/"]];
    path = [path stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    QMetaObject::invokeMethod(AmneziaWebViewPrivate::get(self.view), "onPageStarted", Qt::QueuedConnection);
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    NSString *urlString = [[webView.URL absoluteString] stringByRemovingPercentEncoding];
    NSString *path = [webView.URL.path stringByTrimmingCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"/"]];
    path = [path stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];

    __block __typeof(self) strongSelf = self;
    [strongSelf retain];
    [webView evaluateJavaScript:@"document.body.innerHTML" completionHandler:^(id _Nullable result, NSError * _Nullable error) {

        AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(strongSelf.view);
        if ((d != nullptr) && !error && result && [result isKindOfClass:[NSString class]]) {
            NSString *html = (NSString *)result;
            IosWebViewPrivate *iosView = static_cast<IosWebViewPrivate *>(d);
            iosView->_innerHTML = QString::fromNSString(html);
        }
        [strongSelf release];
    }];

    QUrl url = QUrl(QString::fromNSString(urlString));
    QMetaObject::invokeMethod(AmneziaWebViewPrivate::get(self.view), "onPageFinished", Qt::QueuedConnection);

    //Scale WkWebView contents to view width
    [webView evaluateJavaScript:
        [NSString stringWithFormat:@"var viewport = null;"
            "var document_head = document.getElementsByTagName( 'head' )[0];"
            "var child = document_head.firstChild;"
            "while ( child ) {"
            "   if ( null == viewport && child.nodeType == 1 && child.nodeName == 'META' && child.getAttribute( 'name' ) == 'viewport' ) {"
            "       viewport = child;"
            "       var content = child.getAttribute( 'content' );"
            "       if (content != null) {"
            "           var regex = new RegExp('width');" //or maybe "var regex = new RegExp('width\\s*=\\s*device-width');"
            "           var found = regex.test(content);"
            "           if(!found) {"
            "               content = content + ',width=device-width';"
            "               viewport.setAttribute( 'content' , content );"
            "           }"
            "       }"
            "       else {"
            "           viewport.setAttribute( 'content' , 'width=device-width' );"
            "       }"
            "   }"
            "   child = child.nextSibling;"
            "}"
            "if (null == viewport) {"
            "   var meta = document.createElement( 'meta' );"
            "   meta.setAttribute( 'name' , 'viewport' );"
            "   meta.setAttribute( 'content' , 'width=device-width' );"
            "   document_head.appendChild( meta );"
            "}"
      ] completionHandler: nil];
}

-(void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    NSURLRequest *request = navigationAction.request;
    NSURL *url = request.URL;
    NSLog(@"url: %@", url);

    if ([url.scheme isEqualToString:@"tel"]) {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }

    AmneziaWebViewPrivate *q = AmneziaWebViewPrivate::get(self.view);
    NSString *baseUrlString = [((IosWebViewPrivate *)q)->_defaultUrl stringByRemovingPercentEncoding];
    NSString *urlString = [[request.URL absoluteString] stringByRemovingPercentEncoding];
    urlString = [urlString stringByReplacingOccurrencesOfString:baseUrlString withString:@""];

    q->setUrl(QString::fromNSString(urlString));
    decisionHandler(WKNavigationActionPolicyAllow);
}

@end

#else

@implementation WebDelegate

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    Q_UNUSED(webView);

    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    if([error code] == NSURLErrorCancelled) return;

    AmneziaWebViewPrivate *d = AmneziaWebViewPrivate::get(self.view);
    if (d) {
        d->lastError = QString::fromNSString(error.localizedDescription);
        QMetaObject::invokeMethod(d, "onPageFinished", Qt::QueuedConnection);
    }
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
    NSString *path = [webView.request.URL.path stringByTrimmingCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"/"]];
    path = [path stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    QMetaObject::invokeMethod(AmneziaWebViewPrivate::get(self.view), "onPageStarted", Qt::QueuedConnection);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    NSString *urlString = [[webView.request.URL absoluteString] stringByRemovingPercentEncoding];

    NSString *path = [webView.request.URL.path stringByTrimmingCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"/"]];
    path = [path stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];

    NSString *title=[[webView stringByEvaluatingJavaScriptFromString:@"document.title"] stringByRemovingPercentEncoding];

    AmneziaWebViewPrivate::get(self.view)->setTitle(QString::fromNSString(title));

    QUrl url = QUrl(QString::fromNSString(urlString));
    QMetaObject::invokeMethod(AmneziaWebViewPrivate::get(self.view), "onPageFinished", Qt::QueuedConnection);

    //Scale UIWebView contents to view width
    [webView stringByEvaluatingJavaScriptFromString:
        [NSString stringWithFormat:@"var viewport = null;"
            "var document_head = document.getElementsByTagName( 'head' )[0];"
            "var child = document_head.firstChild;"
            "while ( child ) {"
            "   if ( null == viewport && child.nodeType == 1 && child.nodeName == 'META' && child.getAttribute( 'name' ) == 'viewport' ) {"
            "       viewport = child;"
            "       var content = child.getAttribute( 'content' );"
            "       if (content != null) {"
            "           var regex = new RegExp('width');" //or maybe "var regex = new RegExp('width\\s*=\\s*device-width');"
            "           var found = regex.test(content);"
            "           if(!found) {"
            "               content = content + ',width=device-width';"
            "               viewport.setAttribute( 'content' , content );"
            "           }"
            "       }"
            "       else {"
            "           viewport.setAttribute( 'content' , 'width=device-width' );"
            "       }"
            "   }"
            "   child = child.nextSibling;"
            "}"
            "if (null == viewport) {"
            "   var meta = document.createElement( 'meta' );"
            "   meta.setAttribute( 'name' , 'viewport' );"
            "   meta.setAttribute( 'content' , 'width=device-width' );"
            "   document_head.appendChild( meta );"
            "}"
      ]
     ];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    (void)navigationType;
    (void)webView;

    NSURL *url = request.URL;
    if ([url.scheme isEqualToString:@"tel"]) {
        return YES;
    }

    AmneziaWebViewPrivate *q = AmneziaWebViewPrivate::get(self.view);
    NSString *baseUrlString = [((IosWebViewPrivate *)q)->_defaultUrl stringByRemovingPercentEncoding];
    NSString *urlString = [[request.URL absoluteString] stringByRemovingPercentEncoding];
    urlString = [urlString stringByReplacingOccurrencesOfString:baseUrlString withString:@""];

    //QUrl url = QUrl(QString::fromNSString(urlString));
    q->setUrl(QString::fromNSString(urlString));
    return YES;
}


/*
bridge.js
    var sendObjectMessage = function(parameters) {
        var iframe = document.createElement('iframe');
        iframe.setAttribute('src', 'js:' + JSON.stringify(parameters));

        document.documentElement.appendChild(iframe);
        iframe.parentNode.removeChild(iframe);
        iframe = null;
    };


    sendObjectMessage({name: 'Bryan', company: 'Tumblr'});

*/

- (NSString *) onloadJavaScript
{
    return @""
"     window.onload = function() {                        "
"        alert(\"Hello!\");                               "
"    }                                                    "
"    window.onload = myFunction;";

}

-(void)addScript:(NSString *)script ToWebView:(UIWebView *)webView{

    NSString *execStr = [NSString stringWithFormat:@"var script = document.createElement('script');"
                         "script.type = 'text/javascript';"
                         "script.text = \"%@\";"
                         "document.getElementsByTagName('head')[0].appendChild(script);",script];

    NSString * ret = [webView stringByEvaluatingJavaScriptFromString:execStr];

    NSString *html = [webView stringByEvaluatingJavaScriptFromString:
                          @"document.getElementsByTagName('head')[0].innerHTML"];

    NSLog(@"html:\n%@",html);
}

-(void)addScriptSRC:(NSString *)scriptPath ToWebView:(UIWebView *)webView{

    NSString *execStr = [NSString stringWithFormat:@"var script = document.createElement('script');"
                         "script.type = 'text/javascript';"
                         "script.src = \"%@\";"
                         "document.getElementsByTagName('head')[0].appendChild(script);",scriptPath];

    [webView stringByEvaluatingJavaScriptFromString:execStr];
}

@end

#endif

/*
UIViewController *AmneziaWebViewPrivate::getViewController(UIView * view, UIViewController * controller)
{
    UIViewController *rootController = controller;
    if(!rootController) {
        rootController = [[view window] rootViewController];
    }

    if(view) {
        NSArray *list = [rootController childViewControllers];
        for ( NSUInteger i = 0; i < [list count]; i++) {
            UIViewController *c = [list objectAtIndex:i];
            if (c.view == view) {
                return c;
            }
            if (c) {
                c = getViewController(view, c);
                if (c && c.view == view) {
                    return c;
                }
            }
        }
    }
}
*/

AmneziaWebViewPrivate *AmneziaWebViewPrivate::create(AmneziaWebView *q)
{
    return new IosWebViewPrivate(q);
}


//AmneziaWebView::AmneziaWebView(QObject *parent): QObject(parent), d_ptr(new IosWebViewPrivate(this))
//{
//    setBackgroundColor(QColor::fromRgbF(0.5, 0.5, 0.5));
//}
//
//QColor AmneziaWebView::backgroundColor() const
//{
//    Q_D(const AmneziaWebView);
//	return d->backgroundColor;
//}

//void *AmneziaWebView::nativeWebView() const
//{
//    Q_D(const AmneziaWebView);
//    return d->nativeWebView();
//}


void IosWebViewPrivate::setWindowParent(QWindow *parent)
{
    if (parent) {

        UIView *parentView = reinterpret_cast<UIView *>(parent->winId());
        [parentView addSubview: webView];
    }
    else {
        [webView removeFromSuperview];
    }
}

void IosWebViewPrivate::setBackgroundColor(const QColor value)
{
    if (backgroundColor != value) {

        backgroundColor = value;

#if defined(ENABLE_WKWEBVIEW)
        WKWebView *webView = static_cast<IosWebViewPrivate*>(this)->webView;
#else
        UIWebView *webView = static_cast<IosWebViewPrivate*>(this)->webView;
#endif
        if (webView) {
            [webView setBackgroundColor:[UIColor colorWithRed:backgroundColor.redF() green:backgroundColor.greenF() blue:backgroundColor.   blueF() alpha:backgroundColor.alphaF()]];
        }
        emit backgroundColorChanged();
    }
}

IosWebViewPrivate::~IosWebViewPrivate()
{
    if (webView) {

#if defined(ENABLE_WKWEBVIEW)
        webView.navigationDelegate = nil;
        webView.UIDelegate = nil;
#else
        webView.delegate = nil;
#endif
    }
    if (webDelegate) {
        webDelegate.view = NULL;
        [webDelegate release];
        webDelegate = nil;
    }
    if (webView) {
        [webView release];
        webView = nil;
    }
    if (_defaultUrl) {
        [_defaultUrl release];
        _defaultUrl = nil;
    }

#if defined(ENABLE_WKWEBVIEW)
    if (schemeHandler) {
        schemeHandler.view = NULL;
        [schemeHandler release];
        schemeHandler = nil;
    }

    if (configuration) {
        [configuration release];
        configuration = nil;
    }
#endif
}

IosWebViewPrivate::IosWebViewPrivate(AmneziaWebView* q): AmneziaWebViewPrivate(q)
    , webDelegate(nil)
#if defined(ENABLE_WKWEBVIEW)
    , schemeHandler(nil)
    , configuration(nil)
#endif
    , webView(nil)
    , _defaultUrl(nil)
{
    NSArray *arr = [[NSFileManager defaultManager] URLsForDirectory: NSDocumentDirectory inDomains: NSUserDomainMask];
    webDelegate = [[WebDelegate alloc] init];
    CGRect frame = CGRectMake(0.0, 0.0, 400, 400);
#if defined(ENABLE_WKWEBVIEW)

     _defaultUrl = [[NSString alloc] initWithString:[[arr firstObject] absoluteString]];
    if ([_defaultUrl hasPrefix:@"file"]) {
        NSString *local = [[NSString alloc] initWithFormat:@"local%@", [_defaultUrl substringFromIndex:4]];
        [_defaultUrl release];
        _defaultUrl = local;
    }

    schemeHandler = [[WebSchemeHandler alloc] init];
    schemeHandler.view = q;
    configuration = [[WKWebViewConfiguration alloc] init];

    NSString *qrcSheme = qrcHandler.scheme().toNSString();
    if (![WKWebView handlesURLScheme:qrcSheme]) {
        [configuration setURLSchemeHandler:schemeHandler forURLScheme:qrcSheme];
    }

    NSString *jsSheme = jsHandler.scheme().toNSString();
    if (![WKWebView handlesURLScheme:jsSheme]) {
        [configuration setURLSchemeHandler:schemeHandler forURLScheme:jsSheme];
    }

    if (![WKWebView handlesURLScheme:@"local"]) {
        //sinonimous fo file scheme
        [configuration setURLSchemeHandler:schemeHandler forURLScheme:@"local"];
    }

    webView = [[WKWebView alloc] initWithFrame: frame configuration:configuration];
#ifdef DEBUG
    if (@available(iOS 16.4, *)) {
        [webView setInspectable: YES];
    }
#endif
    [webView setOpaque:NO];
    [webView.configuration setDataDetectorTypes: WKDataDetectorTypeLink | WKDataDetectorTypeAddress | WKDataDetectorTypeCalendarEvent];

    [webView setNavigationDelegate:webDelegate];

#else
     _defaultUrl = [[NSString alloc] initWithString:[[arr firstObject] absoluteString]];
    webView = [[UIWebView alloc] initWithFrame: frame];
    [webView setOpaque:NO];
    [webView setDataDetectorTypes: UIDataDetectorTypeLink | UIDataDetectorTypeAddress | UIDataDetectorTypeCalendarEvent];

    [webView setDelegate:webDelegate];
#endif

    webDelegate.view = q;

    [webView setHidden:TRUE];
#if !defined(ENABLE_WKWEBVIEW)
    [webView setScalesPageToFit:TRUE];
#endif
    setBackgroundColor(backgroundColor);
}

void IosWebViewPrivate::show()
{
    if ( !visible) {

        if (webView && [webView superview]) {

            if ([webView isHidden])
                [webView setHidden:FALSE];

            [[webView superview] setNeedsLayout];
            [[webView superview] setNeedsDisplay];
            [webView setNeedsLayout];
            [webView setNeedsDisplay];

            [[[webView superview] layer] setNeedsDisplay];
            [[webView layer] setNeedsDisplay];
        }
        visible = true;
    }
}

void IosWebViewPrivate::hide()
{
    Q_Q(AmneziaWebView);
    if (visible) {

        if (webView && [webView superview]) {

            if (![webView isHidden]) {
                [webView setHidden:TRUE];
            }
        }
        q->update();
        visible = false;
    }
}

void IosWebViewPrivate::setGeometry(const QRect &geometry)
{
    if (this->geometry != geometry) {
		this->geometry = geometry;
        CGRect frame =  toCGRect(geometry);

        if (webView) {

            CGRect bounds = [webView bounds];
            bounds.size = frame.size;
            [webView setFrame: frame];
            [webView setBounds:bounds];
            [webView setNeedsDisplay];
        }
	}
}

QString IosWebViewPrivate::innerHTML() const
{
#if defined(ENABLE_WKWEBVIEW)
    return _innerHTML;
#else
    //NSString *htmlSource = [d->webView stringByEvaluatingJavaScriptFromString:@"document.body.innerHTML"];
    //NSString *htmlSource = [d->webView stringByEvaluatingJavaScriptFromString:@"document.documentElement.outerHTML"];

    NSString *htmlSource = [webView stringByEvaluatingJavaScriptFromString: @"document.body.innerHTML"];
    QString innerHtml  = QString::fromNSString(htmlSource);
    return innerHtml;
#endif
}

static inline QUrl ensureAbsoluteUrl(const QUrl &url)
{
    if (!url.isValid() || !url.isRelative())
        return url;

    // This contains the URL with absolute path but without.
    // the query and the fragment part.
    QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absoluteFilePath());

    // The path is removed so the query and the fragment parts are there.
    QString pathRemoved = url.toString(QUrl::RemovePath);
    QUrl toResolve(pathRemoved);
    return baseUrl.resolved(toResolve);
}

void IosWebViewPrivate::load(const QUrl& url)
{
    QUrl effectiveUrl = url;
    QString effectivePath = effectiveUrl.path();
    QString defaultPath = QUrl(QString::fromNSString(_defaultUrl)).path();
    if ((effectiveUrl.scheme() == "file") && (effectivePath.compare(defaultPath, Qt::CaseInsensitive) >= 0)) {
        effectiveUrl.setScheme("local");
    }
    NSURL *requestUrl = ensureAbsoluteUrl(effectiveUrl).toNSURL();
    [webView loadRequest:[NSURLRequest requestWithURL:requestUrl]];
}

void IosWebViewPrivate::setHtml(const QString& html, const QUrl& baseUrl)
{
    this->baseUrl = baseUrl;

    QRegularExpression comments("<!--(?!<!)[^\[>].*?-->", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
    QString compressedHtml(html);
    compressedHtml.remove(comments);

    NSURL *baseDataUrl = nil;
    if (!baseUrl.isEmpty()) {
        baseDataUrl = [NSURL URLWithString:[NSString stringWithUTF8String: baseUrl.toString().toUtf8()]];
    }
    else {
        baseDataUrl = [NSURL URLWithString: _defaultUrl];
    }

    NSData *htmlData = [compressedHtml.toNSString() dataUsingEncoding:NSUTF8StringEncoding];
#if defined(ENABLE_WKWEBVIEW)
    [webView loadData: htmlData MIMEType:@"text/html" characterEncodingName:@"utf-8" baseURL: baseDataUrl];
#else
    [webView loadData: htmlData MIMEType:@"text/html" textEncodingName:@"utf-8" baseURL: baseDataUrl];
#endif
}

void IosWebViewPrivate::setContent(const QByteArray& data, const QString& mimeType, const QUrl& baseUrl)
{
    this->baseUrl = baseUrl;

    NSData *contentData = [NSData dataWithBytes:data.data() length:data.length()];
    NSString *contentMimeType = mimeType.toNSString();

    NSURL *baseContentUrl = nil;
    if (!baseUrl.isEmpty()) {
        baseContentUrl = [NSURL URLWithString:[NSString stringWithUTF8String: baseUrl.toString().toUtf8()]];
    }
    else {
        baseContentUrl = [NSURL URLWithString: _defaultUrl];
    }


#if defined(ENABLE_WKWEBVIEW)
    [webView loadData: contentData MIMEType:contentMimeType characterEncodingName:@"utf-8" baseURL: baseContentUrl];
#else
    [webView loadData: contentData MIMEType:contentMimeType textEncodingName:@"utf-8" baseURL: baseContentUrl];
#endif
}

QSize IosWebViewPrivate::contentsSize() const
{
    return QSize();
}


//QString AmneziaWebView::title() const
//{
//    Q_D(const WebView);
//    return d->title;
//}

//QUrl AmneziaWebView::url() const
//{
//    Q_D(const WebView);
//    return d->url;
//}

//WebSettings* AmneziaWebView::settings() const
//{
//    Q_D(const WebView);
//    return d->settings;
//}

void IosWebViewPrivate::evaluateJavaScript(const QString& scriptSource)
{
    NSString *script = scriptSource.toNSString();
#if defined(ENABLE_WKWEBVIEW)
    [webView evaluateJavaScript:script completionHandler:nil];
#else
    [webView stringByEvaluatingJavaScriptFromString:script];
#endif
}

bool IosWebViewPrivate::isLoading() const
{
    if( [webView isLoading] )
        return true;
    return false;
}

bool IosWebViewPrivate::canGoBack() const
{
    bool can = false;
    if (webView) {
        can = [webView canGoBack];
    }
    return can;
}

bool IosWebViewPrivate::canGoForward() const
{
    bool can = false;
    if (webView) {
        can = [webView canGoForward];
    }
    return can;
}

void IosWebViewPrivate::back()
{
    if(webView) {
        [webView goBack];
    }
}

void IosWebViewPrivate::forward()
{
    if (webView) {
        [webView goForward];
    }
}

void IosWebViewPrivate::reload()
{
    if (webView) {
        [webView reload];
    }
}

void IosWebViewPrivate::stop()
{
    if (webView) {
        [webView stopLoading];
    }
}


void IosWebViewPrivate::setScale(qreal scale)
{
    [[webView scrollView] setZoomScale:scale];
}

void IosWebViewPrivate::setTextZoom(int percent)
{
    Q_UNUSED(percent)
}

qreal IosWebViewPrivate::scale() const
{
    CGFloat scale = [[webView scrollView] zoomScale];
    return scale;
}

void IosWebViewPrivate::setUrl(const QUrl &url)
{
    AmneziaWebViewPrivate::setUrl(url);
}

QIcon IosWebViewPrivate::icon() const
{
    return QIcon();
}


void IosWebViewPrivate::setDefaultFontSize(int size)
{
    NSString *fontSize = [[NSNumber numberWithInt: (int)size] stringValue];
#if defined(ENABLE_WKWEBVIEW)
        [webView evaluateJavaScript:[NSString stringWithFormat: @"document.body.style.fontSize = '%@px';", fontSize] completionHandler:nil];
#else
    [webView stringByEvaluatingJavaScriptFromString: [NSString stringWithFormat: @"document.body.style.fontSize = '%@px';", fontSize]];
#endif
}
void IosWebViewPrivate::setStandardFontFamily(const QString &family)
{
    NSString *fontFamily = family.toNSString();
#if defined(ENABLE_WKWEBVIEW)
    [webView evaluateJavaScript:[NSString stringWithFormat: @"document.body.style.fontFamily = '%@';", fontFamily] completionHandler: nil];
#else
    [webView stringByEvaluatingJavaScriptFromString: [NSString stringWithFormat: @"document.body.style.fontFamily = '%@';", fontFamily]];
#endif
}

//void AmneziaWebView::setScalesPageToFit(bool enable) { }
//bool AmneziaWebView::scalesPageToFit() const { }



//void AmneziaWebView::setPreferredContentsSize(const QSize size)
//{
//    //setGeometry(0, 0, size.width(), size.height());
//
//    /*
//    if ((size.height() > 0 ) && (size.width() > 0 )) {
//        id winId = (id)((void *) effectiveWinId());
//        if ([winId isKindOfClass: [UIView class]]) {
//            UIView *view = (UIView*)winId;
//
//            [view setFrame:CGRectMake(0,0,size.width(), size.height())];
//            //[d->webView setFrame:CGRectMake(0,0,size.width(), size.height())];
//        }
//    }
//    */
//}
//
//void WebView::setResizesToContents(bool resizeToContent) { }
//
//int WebView::preferredWidth()
//{
//
//}
//
//int WebView::preferredHeight()
//{
//
//}

//void WebView::setRenderingEnabled(bool enabled) { }


//QRect WebView::elementAreaAt(int x, int y, int maxWidth, int maxHeight) const { }

//QSize WebView::preferredContentsSize() const { }






// Clear history
/*
[webView stringByEvaluatingJavaScriptFromString:[NSString stringWithFormat:@"if( window.history.length > 1 ) { window.history.go( -( window.history.length - 1 ) ) }; window.setTimeout( \"window.location.replace( '%@' )\", 300 );", targetLocation]];
*/

// Font size change
/*
-(void)webViewDidFinishLoad:(UIWebView *)webView1{

    int fontSize = 20;
    NSString *jsString = [[NSString alloc] initWithFormat:@"document.getElementsByTagName('body')[0].style.webkitTextSizeAdjust= '%d%%'", fontSize];
    [webView1 stringByEvaluatingJavaScriptFromString:jsString];
    [jsString release];

}
*/


/*

extern UIViewController *UnityGetGLViewController(); // Root view controller of Unity screen.

static UIWebView *webView;

extern "C" void _WebViewPluginInstall() {
    // Add the web view onto the root view (but don't show).
    UIViewController *rootViewController = UnityGetGLViewController();
    webView = [[UIWebView alloc] initWithFrame:rootViewController.view.frame];
    webView.hidden = YES;
    [rootViewController.view addSubview:webView];
}

extern "C" void _WebViewPluginMakeTransparentBackground() {
    [webView setBackgroundColor:[UIColor clearColor]];
    [webView setOpaque:NO];
}

extern "C" void _WebViewPluginLoadUrl(const char* url, boolean isClearCache) {
    if (isClearCache) {
        [[NSURLCache sharedURLCache] removeAllCachedResponses];
    }
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]]];
}

extern "C" void _WebViewPluginSetVisibility(bool visibility) {
    webView.hidden = visibility ? NO : YES;
}

extern "C" void _WebViewPluginSetMargins(int left, int top, int right, int bottom) {
    UIViewController *rootViewController = UnityGetGLViewController();

    CGRect frame = rootViewController.view.frame;
    CGFloat scale = rootViewController.view.contentScaleFactor;

    CGRect screenBound = [[UIScreen mainScreen] bounds];
    CGSize screenSize = screenBound.size;
    // Obtaining the current device orientation
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];

    // landscape
    if (orientation) {
        frame.size.width = screenSize.height - (left + right) / scale;
        frame.size.height = screenSize.width - (top + bottom) / scale;
    } else { // portrait
        frame.size.width = screenSize.width - (left + right) / scale;
        frame.size.height = screenSize.height - (top + bottom) / scale;
    }

    frame.origin.x += left / scale;
    frame.origin.y += top / scale;

    webView.frame = frame;
}

extern "C" char *_WebViewPluginPollMessage() {
    // Try to retrieve a message from the message queue in JavaScript context.
    NSString *message = [webView stringByEvaluatingJavaScriptFromString:@"unityWebMediatorInstance.pollMessage()"];
    if (message && message.length > 0) {
        NSLog(@"UnityWebViewPlugin: %@", message);
        char* memory = static_cast<char*>(malloc(strlen(message.UTF8String) + 1));
        if (memory) strcpy(memory, message.UTF8String);
        return memory;
    } else {
        return NULL;
    }
}

*/


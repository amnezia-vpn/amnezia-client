#import <Foundation/Foundation.h>
#import <MobileCoreServices/UTCoreTypes.h>
#import <MobileCoreServices/UTType.h>

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include "mimecache.h"
#include "filehandler.h"
#define kProtocolFileScheme @"file"

#if !defined(ENABLE_WKWEBVIEW)

@interface FileProtocol : NSURLProtocol
@end

#endif

FileHandler::FileHandler()
{
#if !defined(ENABLE_WKWEBVIEW)
    static bool protocolRegistered = false;
    if (!protocolRegistered) {
        [NSURLProtocol registerClass:[FileProtocol class]];
        protocolRegistered = true;
    }
#endif
}

#if !defined(ENABLE_WKWEBVIEW)

@implementation FileProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
    NSString *scheme = request.URL.scheme;
    if ([kProtocolFileScheme caseInsensitiveCompare:scheme] == NSOrderedSame) {
        return YES;
    }
    
    return NO;
}

+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
    return request;
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

- (void)startLoading
{
    /* retrieve the current request. */
    NSURLRequest *request = [self request];
    NSString *url = [[request URL] path];
    NSString *mimeType = mimeTypeForExtension(QString::fromNSString(url.pathExtension)).toNSString();
    
    QString requestUrl(QString("%1").arg(QString::fromNSString(url)));
    QFile resource(requestUrl);

    NSData *pageData = nil;
    if (resource.exists() && resource.open(QIODevice::ReadOnly)) {
        
        QByteArray buffer = resource.readAll();
        
        //pageData = [[NSData alloc] initWithBytes:buffer.constData() length:buffer.size()];
        pageData = [NSData dataWithBytes:buffer.constData() length:buffer.size()];
        resource.close();
    }

    if (pageData) {

        NSString *encoding = @"utf-8";
        if ([mimeType isEqualToString:@"text/html"]) {

            NSString *content = [[NSString alloc] initWithBytesNoCopy: (char* )[pageData bytes] length:pageData.length encoding:NSISOLatin1StringEncoding freeWhenDone:NO];

            NSString *charset = [FileProtocol charsetFromHtml:[FileProtocol headFromHtml:content]];
            if (charset) {
                encoding = charset;
            }
            [content autorelease];
        }

        NSURLResponse *response =[[NSURLResponse alloc]initWithURL:self.request.URL
                                                          MIMEType:mimeType
                                             expectedContentLength:[pageData length]
                                                  textEncodingName:encoding];
        
        
        [[self client] URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
        [[self client] URLProtocol:self didLoadData:pageData];
        [[self client] URLProtocolDidFinishLoading:self];
        [response autorelease];
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

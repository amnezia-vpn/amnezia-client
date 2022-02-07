/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iosauthenticationlistener.h"
#include "leakdetector.h"
#include "logger.h"
#include "mozillavpn.h"
#include "qmlengineholder.h"

#include <QApplication>
#include <QUrl>
#include <QUrlQuery>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtGui>
#include <QWindow>
#include <QQmlApplicationEngine>

#import <UIKit/UIKit.h>
#import <AuthenticationServices/ASWebAuthenticationSession.h>

namespace {

Logger logger({LOG_IOS, LOG_MAIN}, "IOSAuthenticationListener");

ASWebAuthenticationSession* session = nullptr;

}  // namespace

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
@interface ContextProvider : NSObject <ASWebAuthenticationPresentationContextProviding> {
  UIView* m_view;
}
@end

@implementation ContextProvider

- (id)initWithUIView:(UIView*)uiView {
  self = [super init];
  if (self) {
    m_view = uiView;
  }
  return self;
}

#  pragma mark - ASWebAuthenticationPresentationContextProviding
- (nonnull ASPresentationAnchor)presentationAnchorForWebAuthenticationSession:
    (nonnull ASWebAuthenticationSession*)session API_AVAILABLE(ios(13.0)) {
  return m_view.window;
}

@end
#endif

IOSAuthenticationListener::IOSAuthenticationListener(QObject* parent)
    : AuthenticationListener(parent) {
  MVPN_COUNT_CTOR(IOSAuthenticationListener);
}

IOSAuthenticationListener::~IOSAuthenticationListener() {
  MVPN_COUNT_DTOR(IOSAuthenticationListener);
}

void IOSAuthenticationListener::start(const QString& codeChallenge,
                                      const QString& codeChallengeMethod,
                                      const QString& emailAddress) {
  logger.debug() << "IOSAuthenticationListener initialize";

  QUrl url(createAuthenticationUrl(AmneziaVPN::AuthenticationInBrowser, codeChallenge,
                                   codeChallengeMethod, emailAddress));
  QUrlQuery query(url.query());
  query.addQueryItem("platform", "ios");
  url.setQuery(query);

#ifdef QT_DEBUG
  logger.debug() << "Authentication URL:" << url.toString();
#endif

  if (session) {
    [session dealloc];
    session = nullptr;
  }

  session = [[ASWebAuthenticationSession alloc]
            initWithURL:url.toNSURL()
      callbackURLScheme:@"mozilla-vpn"
      completionHandler:^(NSURL* _Nullable callbackURL, NSError* _Nullable error) {
        [session dealloc];
        session = nullptr;

        if (error) {
          logger.error() << "Authentication failed:"
                         << QString::fromNSString([error localizedDescription]);
          logger.error() << "Code:" << [error code];
          logger.error() << "Suggestion:"
                         << QString::fromNSString([error localizedRecoverySuggestion]);
          logger.error() << "Reason:" << QString::fromNSString([error localizedFailureReason]);

          if ([error code] == ASWebAuthenticationSessionErrorCodeCanceledLogin) {
            emit abortedByUser();
          } else {
            emit failed(ErrorHandler::RemoteServiceError);
          }

          return;
        }

        QUrl callbackUrl = QUrl::fromNSURL(callbackURL);
        logger.debug() << "Authentication completed";

        Q_ASSERT(callbackUrl.hasQuery());

        QUrlQuery callbackUrlQuery(callbackUrl.query());
        Q_ASSERT(callbackUrlQuery.hasQueryItem("code"));
        QString code = callbackUrlQuery.queryItemValue("code");
        emit completed(code);
      }];

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
  QObject* rootObject = QmlEngineHolder::instance()->engine()->rootObjects().first();
  QWindow* window = qobject_cast<QWindow*>(rootObject);
  Q_ASSERT(window);

  UIView* view = static_cast<UIView*>(
      QGuiApplication::platformNativeInterface()->nativeResourceForWindow("uiview", window));

  if (@available(iOS 13, *)) {
    session.presentationContextProvider = [[ContextProvider alloc] initWithUIView:view];
  }
#endif

  if (![session start]) {
    [session dealloc];
    session = nullptr;

    logger.error() << "Authentication failed: session doesn't start.";
    emit failed(ErrorHandler::RemoteServiceError);
  }
}

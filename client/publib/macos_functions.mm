#include "macos_functions.h"
#include <QDebug>
#import <AppKit/AppKit.h>

MacOSFunctions &MacOSFunctions::instance()
{
    static MacOSFunctions s;
    return s;
}

MacOSFunctions::MacOSFunctions()
{
    registerThemeNotification();
}

bool MacOSFunctions::isMenuBarUseDarkTheme() const
{
    NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    BOOL darkModeOn = ( style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"] );

    return darkModeOn;
}


void MacOSFunctions::registerThemeNotification()
{
  // [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(darkModeChanged:) name:@"AppleInterfaceThemeChangedNotification" object:nil];
}


void darkModeChanged(NSNotification*notif)
{
    Q_UNUSED(notif);
    qDebug() << "Dark mode changed" << MacOSFunctions::instance().isMenuBarUseDarkTheme();
}


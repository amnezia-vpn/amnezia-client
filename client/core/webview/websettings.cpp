
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QHash>
#include <QSharedData>
#include <QStandardPaths>
#include <QUrl>

#include "websettings.h"
#include "amneziawebview_p.h"

AmneziaWebViewSettings::AmneziaWebViewSettings(AmneziaWebView *view): QObject(view), s(new WebSettings(view))
{}

Q_GLOBAL_STATIC(QList<WebSettings*>, allSettings)

void WebSettings::apply()
{
    if (view) {
        
        WebSettings* global = WebSettings::globalSettings();


        QString family = fontFamilies.value(WebSettings::StandardFont,
                                            global->fontFamilies.value(WebSettings::StandardFont));
        view->setStandardFontFamily(family);

        
        int size = fontSizes.value(WebSettings::DefaultFontSize,
                               global->fontSizes.value(WebSettings::DefaultFontSize));
        view->setDefaultFontSize(size);

        bool value = attributes.value(WebSettings::AutoLoadImages,
                                      global->attributes.value(WebSettings::AutoLoadImages));
        
        QString encoding = !defaultTextEncoding.isEmpty() ? defaultTextEncoding: global->defaultTextEncoding;


        Q_UNUSED(value)
        
    } else {
    
        QList<WebSettings*> settings = *::allSettings();
        for (int i = 0; i < settings.count(); ++i)
            settings[i]->apply();
    }
}

WebSettings* WebSettings::globalSettings()
{
    static WebSettings *global = nullptr;
    if (!global) {
        global = new WebSettings;
    }
    return global;
}

WebSettings::WebSettings()
{
    // Initialize our global defaults
    fontSizes.insert(WebSettings::MinimumFontSize, 0);
    fontSizes.insert(WebSettings::MinimumLogicalFontSize, 0);
    fontSizes.insert(WebSettings::DefaultFontSize, 16);
    fontSizes.insert(WebSettings::DefaultFixedFontSize, 13);

    QFont defaultFont;
    defaultFont.setStyleHint(QFont::Serif);
    fontFamilies.insert(WebSettings::StandardFont, defaultFont.defaultFamily());
    fontFamilies.insert(WebSettings::SerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Fantasy);
    fontFamilies.insert(WebSettings::FantasyFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Cursive);
    fontFamilies.insert(WebSettings::CursiveFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::SansSerif);
    fontFamilies.insert(WebSettings::SansSerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Monospace);
    fontFamilies.insert(WebSettings::FixedFont, defaultFont.defaultFamily());

    attributes.insert(WebSettings::AutoLoadImages, true);
    attributes.insert(WebSettings::DnsPrefetchEnabled, false);
    attributes.insert(WebSettings::JavascriptEnabled, true);
    attributes.insert(WebSettings::SpatialNavigationEnabled, false);
    attributes.insert(WebSettings::LinksIncludedInFocusChain, true);
    attributes.insert(WebSettings::ZoomTextOnly, false);
    attributes.insert(WebSettings::PrintElementBackgrounds, true);
    attributes.insert(WebSettings::OfflineStorageDatabaseEnabled, false);
    attributes.insert(WebSettings::OfflineWebApplicationCacheEnabled, false);
    attributes.insert(WebSettings::LocalStorageEnabled, false);
    attributes.insert(WebSettings::LocalContentCanAccessRemoteUrls, false);
    attributes.insert(WebSettings::LocalContentCanAccessFileUrls, true);
    attributes.insert(WebSettings::AcceleratedCompositingEnabled, true);
    attributes.insert(WebSettings::WebGLEnabled, true);
    attributes.insert(WebSettings::WebAudioEnabled, false);
    attributes.insert(WebSettings::CSSRegionsEnabled, true);
    attributes.insert(WebSettings::CSSGridLayoutEnabled, false);
    attributes.insert(WebSettings::HyperlinkAuditingEnabled, false);
    attributes.insert(WebSettings::TiledBackingStoreEnabled, false);
    attributes.insert(WebSettings::FrameFlatteningEnabled, false);
    attributes.insert(WebSettings::SiteSpecificQuirksEnabled, true);
    attributes.insert(WebSettings::ScrollAnimatorEnabled, false);
    attributes.insert(WebSettings::CaretBrowsingEnabled, false);
    attributes.insert(WebSettings::NotificationsEnabled, true);
    
#if defined(Q_OS_WIN32) && defined(DEBUG)
    attributes.insert(WebSettings::DeveloperExtrasEnabled,true);
#endif
}

/*!
    \internal
*/
WebSettings::WebSettings(AmneziaWebView *v)
       :  view(v)

{
    allSettings()->append(this);
}

WebSettings::~WebSettings()
{
    if (view)
        allSettings()->removeAll(this);

}

void WebSettings::setFontSize(FontSize type, int size)
{
    fontSizes.insert(type, size);
    apply();
}

int WebSettings::fontSize(FontSize type) const
{
    int defaultValue = 0;
    if (view) {
        WebSettings* global = WebSettings::globalSettings();
        defaultValue = global->fontSizes.value(type);
    }
    return fontSizes.value(type, defaultValue);
}

void WebSettings::resetFontSize(FontSize type)
{
    if (view) {
        fontSizes.remove(type);
        apply();
    }
}

void WebSettings::setFontFamily(FontFamily which, const QString& family)
{
    fontFamilies.insert(which, family);
    apply();
}

QString WebSettings::fontFamily(FontFamily which) const
{
    QString defaultValue;
    if (view) {
        WebSettings* global = WebSettings::globalSettings();
        defaultValue = global->fontFamilies.value(which);
    }
    return fontFamilies.value(which, defaultValue);
}

void WebSettings::resetFontFamily(FontFamily which)
{
    if (view) {
        fontFamilies.remove(which);
        apply();
    }
}

void WebSettings::setAttribute(WebAttribute attr, bool on)
{
    attributes.insert(attr, on);
    apply();
}

bool WebSettings::testAttribute(WebAttribute attr) const
{
    bool defaultValue = false;
    if (view) {
        WebSettings* global = WebSettings::globalSettings();
        defaultValue = global->attributes.value(attr);
    }
    return attributes.value(attr, defaultValue);
}

void WebSettings::resetAttribute(WebAttribute attr)
{
    if (view) {
        attributes.remove(attr);
        apply();
    }
}

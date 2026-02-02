#ifndef WEBSETTINGS_H
#define WEBSETTINGS_H

#include <QtQml>

class AmneziaWebView;
class AmneziaWebViewPrivate;
class WebSettingsData;

class WebSettings
{
public:
    enum FontFamily {
        StandardFont,
        FixedFont,
        SerifFont,
        SansSerifFont,
        CursiveFont,
        FantasyFont
    };
    enum WebAttribute {
        AutoLoadImages,
        JavascriptEnabled,
        JavaEnabled,
        PluginsEnabled,
        PrivateBrowsingEnabled,
        JavascriptCanOpenWindows,
        JavascriptCanAccessClipboard,
        DeveloperExtrasEnabled,
        LinksIncludedInFocusChain,
        ZoomTextOnly,
        PrintElementBackgrounds,
        OfflineStorageDatabaseEnabled,
        OfflineWebApplicationCacheEnabled,
        LocalStorageEnabled,
        LocalContentCanAccessRemoteUrls,
        DnsPrefetchEnabled,
        XSSAuditingEnabled,
        AcceleratedCompositingEnabled,
        SpatialNavigationEnabled,
        LocalContentCanAccessFileUrls,
        TiledBackingStoreEnabled,
        FrameFlatteningEnabled,
        SiteSpecificQuirksEnabled,
        JavascriptCanCloseWindows,
        WebGLEnabled,
        CSSRegionsEnabled,
        HyperlinkAuditingEnabled,
        CSSGridLayoutEnabled,
        ScrollAnimatorEnabled,
        CaretBrowsingEnabled,
        NotificationsEnabled,
        WebAudioEnabled
    };
    enum WebGraphic {
        MissingImageGraphic,
        MissingPluginGraphic,
        DefaultFrameIconGraphic,
        TextAreaSizeGripCornerGraphic,
        DeleteButtonGraphic,
        InputSpeechButtonGraphic,
        SearchCancelButtonGraphic,
        SearchCancelButtonPressedGraphic
    };
    enum FontSize {
        MinimumFontSize,
        MinimumLogicalFontSize,
        DefaultFontSize,
        DefaultFixedFontSize
    };
    enum ThirdPartyCookiePolicy {
        AlwaysAllowThirdPartyCookies,
        AlwaysBlockThirdPartyCookies,
        AllowThirdPartyWithExistingCookies
    };

    static WebSettings *globalSettings();
    
    void setFontSize(FontSize type, int size);
    int fontSize(FontSize type) const;
    void resetFontSize(FontSize type);
    

    void setFontFamily(FontFamily which, const QString &family);
    QString fontFamily(FontFamily which) const;
    void resetFontFamily(FontFamily which);
    
    void setAttribute(WebAttribute attr, bool on);
    bool testAttribute(WebAttribute attr) const;
    void resetAttribute(WebAttribute attr);
    
    void apply();

    WebSettings();
    explicit WebSettings(AmneziaWebView *v);
    virtual ~WebSettings();

private:
    friend class WebSettingsData;
    friend class AmneziaWebViewPrivate;
    friend class WebViewPrivate;



    Q_DISABLE_COPY(WebSettings)

    QHash<int, QString> fontFamilies;
    QHash<int, int> fontSizes;
    QHash<int, bool> attributes;
    QString defaultTextEncoding;
    AmneziaWebView *view;

};

class AmneziaWebViewSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int defaultFontSize READ defaultFontSize WRITE setDefaultFontSize)
    Q_PROPERTY(QString standardFontFamily READ standardFontFamily WRITE setStandardFontFamily)
    Q_PROPERTY(bool developerExtrasEnabled READ developerExtrasEnabled WRITE setDeveloperExtrasEnabled)

public:
    explicit AmneziaWebViewSettings(AmneziaWebView *parent);

    int defaultFontSize() const { return s->fontSize(WebSettings::DefaultFontSize); }
    void setDefaultFontSize(int size) { s->setFontSize(WebSettings::DefaultFontSize, size); }

    QString standardFontFamily() const { return s->fontFamily(WebSettings::StandardFont); }
    void setStandardFontFamily(const QString& f) { s->setFontFamily(WebSettings::StandardFont, f); }

    bool developerExtrasEnabled() const { return s->testAttribute(WebSettings::DeveloperExtrasEnabled); }
    void setDeveloperExtrasEnabled(bool on) { s->setAttribute(WebSettings::DeveloperExtrasEnabled, on); }

    void apply() { s->apply(); }

private:
    QScopedPointer<WebSettings> s;
};

QML_DECLARE_TYPE(AmneziaWebViewSettings)


#endif

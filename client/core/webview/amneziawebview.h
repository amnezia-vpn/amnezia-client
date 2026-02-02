#ifndef DECLARATIVEWEBVIEW_H
#define DECLARATIVEWEBVIEW_H

#include <QObject>
#include <QAction>
#include <QBasicTimer>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>

#include <QtQml>
#include <QQuickPaintedItem>

#include "websettings.h"

QT_BEGIN_NAMESPACE

class AmneziaWebViewSettings;
class AmneziaWebViewPrivate;
class AmneziaWebViewAttached;
class AmneziaWebHistory;

class AmneziaWebView : public  QQuickPaintedItem
{
    Q_OBJECT

    Q_ENUMS(Status SelectionMode)

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QPixmap icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString html READ html WRITE setHtml NOTIFY htmlChanged)
    Q_PROPERTY(int pressGrabTime READ pressGrabTime WRITE setPressGrabTime NOTIFY pressGrabTimeChanged)
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth NOTIFY preferredWidthChanged)
    Q_PROPERTY(qreal preferredHeight READ preferredHeight WRITE setPreferredHeight NOTIFY preferredHeightChanged)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    

#ifndef QT_NO_ACTION
    Q_PROPERTY(QAction* reload READ reloadAction CONSTANT)
    Q_PROPERTY(QAction* back READ backAction CONSTANT)
    Q_PROPERTY(QAction* forward READ forwardAction CONSTANT)
    Q_PROPERTY(QAction* stop READ stopAction CONSTANT)
#endif

    Q_PROPERTY(AmneziaWebViewSettings* settings READ settingsObject CONSTANT)
    Q_PROPERTY(QQmlListProperty<QObject> javaScriptWindowObjects READ javaScriptWindowObjects CONSTANT)
    Q_PROPERTY(QQmlComponent* newWindowComponent READ newWindowComponent WRITE setNewWindowComponent NOTIFY newWindowComponentChanged)
    Q_PROPERTY(QQuickItem* newWindowParent READ newWindowParent WRITE setNewWindowParent NOTIFY newWindowParentChanged)
    Q_PROPERTY(QSize contentsSize READ contentsSize NOTIFY contentsSizeChanged)
    Q_PROPERTY(qreal contentsScale READ contentsScale WRITE setContentsScale NOTIFY contentsScaleChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    
    enum WebAction {
        NoWebAction = - 1,
        
        OpenLink,
        
        OpenLinkInNewWindow,
        OpenFrameInNewWindow,
        
        DownloadLinkToDisk,
        CopyLinkToClipboard,
        
        OpenImageInNewWindow,
        DownloadImageToDisk,
        CopyImageToClipboard,
        
        Back,
        Forward,
        Stop,
        Reload,
        
        Cut,
        Copy,
        Paste,
        
        Undo,
        Redo,
        MoveToNextChar,
        MoveToPreviousChar,
        MoveToNextWord,
        MoveToPreviousWord,
        MoveToNextLine,
        MoveToPreviousLine,
        MoveToStartOfLine,
        MoveToEndOfLine,
        MoveToStartOfBlock,
        MoveToEndOfBlock,
        MoveToStartOfDocument,
        MoveToEndOfDocument,
        SelectNextChar,
        SelectPreviousChar,
        SelectNextWord,
        SelectPreviousWord,
        SelectNextLine,
        SelectPreviousLine,
        SelectStartOfLine,
        SelectEndOfLine,
        SelectStartOfBlock,
        SelectEndOfBlock,
        SelectStartOfDocument,
        SelectEndOfDocument,
        DeleteStartOfWord,
        DeleteEndOfWord,
        
        SetTextDirectionDefault,
        SetTextDirectionLeftToRight,
        SetTextDirectionRightToLeft,
        
        ToggleBold,
        ToggleItalic,
        ToggleUnderline,
        
        InspectElement,
        
        InsertParagraphSeparator,
        InsertLineSeparator,
        
        SelectAll,
        ReloadAndBypassCache,
        
        PasteAndMatchStyle,
        RemoveFormat,
        
        ToggleStrikethrough,
        ToggleSubscript,
        ToggleSuperscript,
        InsertUnorderedList,
        InsertOrderedList,
        Indent,
        Outdent,
        
        AlignCenter,
        AlignJustified,
        AlignLeft,
        AlignRight,
        
        StopScheduledPageRefresh,
        
        CopyImageUrlToClipboard,
        
        WebActionCount
    };
    
    
    explicit AmneziaWebView(QQuickItem *parent = nullptr);
    virtual ~AmneziaWebView();

    QUrl url() const;
    void setUrl(const QUrl &);

    QString title() const;

    QPixmap icon() const;

    int pressGrabTime() const;
    void setPressGrabTime(int);

    qreal preferredWidth() const;
    void setPreferredWidth(qreal);
    qreal preferredHeight() const;
    void setPreferredHeight(qreal);

    enum Status { Null, Ready, Loading, Error };
    Status status() const;
    qreal progress() const;
    QString statusText() const;
    
#ifndef QT_NO_ACTION
    QAction *reloadAction() const;
    QAction *backAction() const;
    QAction *forwardAction() const;
    QAction *stopAction() const;
    QAction* action(AmneziaWebView::WebAction) const;
#endif
    
    void load(const QNetworkRequest &request, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
              const QByteArray &body = QByteArray());

    QString html() const;

    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void setContent(const QByteArray &data, const QString &mimeType = QString(), const QUrl &baseUrl = QUrl());

    AmneziaWebHistory* history() const;

    QQmlListProperty<QObject> javaScriptWindowObjects();
    AmneziaWebViewSettings* settingsObject() const;
    static AmneziaWebViewAttached* qmlAttachedProperties(QObject*);

    QQmlComponent *newWindowComponent() const;
    void setNewWindowComponent(QQmlComponent *newWindow);
    QQuickItem* newWindowParent() const;
    void setNewWindowParent(QQuickItem* newWindow);

    bool isComponentCompletePublic() const { return isComponentComplete(); }

    QSize contentsSize() const;

    void setContentsScale(qreal scale);
    qreal contentsScale() const;

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor&);
    
    void paint(QPainter *painter) override;

    void setDefaultFontSize(int size);
    void setStandardFontFamily(const QString &family);
    Q_INVOKABLE void setTextZoom(int percent);

Q_SIGNALS:
    
    void preferredWidthChanged();
    void preferredHeightChanged();
    
    void urlChanged();
    void progressChanged();
    void statusChanged(Status);
    void titleChanged(const QString&);
    void iconChanged();
    void statusTextChanged();
    void htmlChanged();
    void pressGrabTimeChanged();
    void newWindowComponentChanged();
    void newWindowParentChanged();
    void renderingEnabledChanged();
    void contentsSizeChanged(const QSize&);
    void contentsScaleChanged();
    void backgroundColorChanged();

    void loadStarted();
    void loadFinished();
    void loadFinished(bool ok);
    void loadFailed();

    void doubleClick(int clickX, int clickY);
    void zoomTo(qreal zoom, int centerX, int centerY);
    void alert(const QString& message);

public Q_SLOTS:
    void evaluateJavaScript(const QString&);

private Q_SLOTS:
    void afterRendering();
    void updateGeometry();
    void windowWasChanged(QQuickWindow* window);
    void parentWasChanged();
    void fillColorWasChanged();

    void doLoadStarted();
    void doLoadProgress(int p);
    void doLoadFinished(bool ok);
    void setStatusText(const QString&);
    void windowObjectCleared();

protected:

    void itemChange(ItemChange, const ItemChangeData &) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QScopedPointer<AmneziaWebViewPrivate> d_ptr;
    
private:
    void updateContentsSize() {}
    void init();
    void componentComplete() override;
    QTimer upadeTimer;


    Q_DISABLE_COPY(AmneziaWebView)
    Q_DECLARE_PRIVATE(AmneziaWebView)
    
    friend class QDeclarativeWebPage;
};

class AmneziaWebViewAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString windowObjectName READ windowObjectName WRITE setWindowObjectName)
public:
    explicit AmneziaWebViewAttached(QObject* parent)
        : QObject(parent)
    {
    }

    QString windowObjectName() const
    {
        return m_windowObjectName;
    }

    void setWindowObjectName(const QString &n)
    {
        m_windowObjectName = n;
    }

private:
    QString m_windowObjectName;
};
 
QT_END_NAMESPACE

QML_DECLARE_TYPE(AmneziaWebView)
QML_DECLARE_TYPEINFO(AmneziaWebView, QML_HAS_ATTACHED_PROPERTIES)

#endif

#ifndef WEBHISTORY_H
#define WEBHISTORY_H

#include <QObject>
#include <QIcon>

class AmneziaWebViewPrivate;
class AmneziaWebView;

class AmneziaWebHistory;
class AmneziaWebHistoryItemPrivate;
class AmneziaWebHistoryItem
{
    Q_DECLARE_PRIVATE(AmneziaWebHistoryItem)
public:

    AmneziaWebHistoryItem(const AmneziaWebHistoryItem &other);
    AmneziaWebHistoryItem &operator=(const AmneziaWebHistoryItem &other);
    ~AmneziaWebHistoryItem();

    QUrl url() const;
    QString title() const;
    QIcon icon() const;

    QByteArray data() const;
    QString mimeType() const;

    bool isValid() const;

private:
    explicit AmneziaWebHistoryItem(AmneziaWebHistoryItemPrivate *priv);
    friend class AmneziaWebHistory;
    friend class AmneziaWebViewPrivate;
    QExplicitlySharedDataPointer<AmneziaWebHistoryItemPrivate> d_ptr;
};

class AmneziaWebHistoryPrivate;
class AmneziaWebHistory : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AmneziaWebHistory)

public:
    virtual ~AmneziaWebHistory();

    void append(const QUrl& url, const QByteArray& data = QByteArray(), const QString& mimeType = QString("text/html"));
    void clear();

    QList<AmneziaWebHistoryItem> items() const;
    QList<AmneziaWebHistoryItem> backItems(int maxItems) const;
    QList<AmneziaWebHistoryItem> forwardItems(int maxItems) const;

    bool canGoBack() const;
    bool canGoForward() const;

    void back();
    void forward();
    void goToItem(const AmneziaWebHistoryItem &item);

    AmneziaWebHistoryItem backItem() const;
    AmneziaWebHistoryItem currentItem() const;
    AmneziaWebHistoryItem forwardItem() const;
    AmneziaWebHistoryItem itemAt(int i) const;
    int currentItemIndex() const;

    int count() const;
    int maximumItemCount() const;
    void setMaximumItemCount(int count);

private:

    friend class AmneziaWebViewPrivate;
    explicit AmneziaWebHistory(AmneziaWebView *parent);
    Q_DISABLE_COPY(AmneziaWebHistory)
    QScopedPointer<AmneziaWebHistoryPrivate> d_ptr;
};

#endif

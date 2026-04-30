#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QSet>
#include <QString>
#include <QVector>
#include <optional>

struct NewsItem
{
    QString id;
    QString title;
    QString content;
    QDateTime timestamp;
    bool isUpdate = false;
};

class NewsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ContentRole,
        TimestampRole,
        IsReadRole,
        IsProcessedRole,
        IsUpdateRole
    };
    explicit NewsModel(class SecureAppSettingsRepository* appSettingsRepository, QObject *parent = nullptr);
    Q_INVOKABLE void markAsRead(int index);
    Q_INVOKABLE void markUpdateAsSkipped();

    Q_PROPERTY(int processedIndex READ processedIndex WRITE setProcessedIndex NOTIFY processedIndexChanged)
    Q_PROPERTY(bool hasUnread READ hasUnread NOTIFY hasUnreadChanged)
    int processedIndex() const;
    void setProcessedIndex(int index);

    void setNewsList(const QJsonArray &items);
    void setUpdateNotification(const QString &id, const QString &title, const QString &content);
    bool hasUnread() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void processedIndexChanged(int index);
    void hasUnreadChanged();

private:
    QVector<NewsItem> m_items;
    QVector<NewsItem> m_apiItems;
    std::optional<NewsItem> m_updateItem;
    int m_processedIndex = -1;
    class SecureAppSettingsRepository* m_appSettingsRepository;
    QSet<QString> m_readIds;
    void loadReadIds();
    void saveReadIds() const;
    void updateModel();
};

#endif // NEWSMODEL_H

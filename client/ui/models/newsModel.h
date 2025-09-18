#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include "settings.h"
#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QSet>
#include <QString>
#include <QVector>
#include <memory>

struct NewsItem
{
    QString id;
    QString title;
    QString content;
    QDateTime timestamp;
    bool read;
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
        IsProcessedRole
    };
    explicit NewsModel(const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);
    Q_INVOKABLE void markAsRead(int index);

    Q_PROPERTY(int processedIndex READ processedIndex WRITE setProcessedIndex NOTIFY processedIndexChanged)
    Q_PROPERTY(bool hasUnread READ hasUnread NOTIFY hasUnreadChanged)
    int processedIndex() const;
    void setProcessedIndex(int index);

    void updateModel(const QJsonArray &items);
    bool hasUnread() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void processedIndexChanged(int index);
    void hasUnreadChanged();

private:
    QVector<NewsItem> m_items;
    int m_processedIndex = -1;
    std::shared_ptr<Settings> m_settings;
    QSet<QString> m_readIds;
    void loadReadIds();
    void saveReadIds() const;
};

#endif // NEWSMODEL_H

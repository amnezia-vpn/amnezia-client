#include "ui/models/newsModel.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QQmlEngine>
#include <QStandardPaths>
#include <algorithm>

NewsModel::NewsModel(const std::shared_ptr<Settings> &settings, QObject *parent) : QAbstractListModel(parent), m_settings(settings)
{
    loadReadIds();
}

int NewsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const NewsItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case TitleRole: return item.title;
    case ContentRole: return item.content;
    case TimestampRole: return item.timestamp.toString(Qt::ISODate);
    case IsReadRole: return m_readIds.contains(item.id);
    case IsProcessedRole: return index.row() == m_processedIndex;
    default: return QVariant();
    }
}

QHash<int, QByteArray> NewsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[ContentRole] = "content";
    roles[TimestampRole] = "timestamp";
    roles[IsReadRole] = "read";
    roles[IsProcessedRole] = "isProcessed";
    return roles;
}

void NewsModel::markAsRead(int index)
{
    if (index < 0 || index >= m_items.size())
        return;

    const QString &itemId = m_items.at(index).id;
    if (itemId.isEmpty() || m_readIds.contains(itemId))
        return;

    m_readIds.insert(itemId);
    saveReadIds();

    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, { IsReadRole });
    emit hasUnreadChanged();
}

int NewsModel::processedIndex() const
{
    return m_processedIndex;
}

void NewsModel::setProcessedIndex(int index)
{
    if (index < 0 || index >= m_items.size() || m_processedIndex == index)
        return;
    m_processedIndex = index;
    emit processedIndexChanged(index);
}

void NewsModel::setNewsList(const QJsonArray &serverItems)
{
    QVector<NewsItem> updatedItems;
    updatedItems.reserve(serverItems.size());

    for (const QJsonValue &value : serverItems) {
        if (!value.isObject())
            continue;

        const QJsonObject object = value.toObject();

        NewsItem item;
        item.id = object.value("id").toString();
        if (item.id.isEmpty())
            continue;
        item.title = object.value("title").toString();
        item.content = object.value("content").toString();
        item.timestamp = QDateTime::fromString(object.value("timestamp").toString(), Qt::ISODate);

        updatedItems.append(item);
    }

    m_apiItems = updatedItems;
    updateModel();
}

void NewsModel::updateModel()
{
    beginResetModel();
    m_items = m_apiItems;
    std::sort(m_items.begin(), m_items.end(), [](const NewsItem &a, const NewsItem &b) { return a.timestamp > b.timestamp; });
    endResetModel();
    emit hasUnreadChanged();
}

bool NewsModel::hasUnread() const
{
    for (const NewsItem &item : m_items) {
        if (!m_readIds.contains(item.id))
            return true;
    }
    return false;
}

void NewsModel::loadReadIds()
{
    QStringList ids = m_settings->readNewsIds();
    m_readIds = QSet<QString>(ids.begin(), ids.end());
}

void NewsModel::saveReadIds() const
{
    m_settings->setReadNewsIds(QStringList(m_readIds.begin(), m_readIds.end()));
}

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
    case IsReadRole: return item.read;
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
    if (!m_items[index].read) {
        m_items[index].read = true;
        m_readIds.insert(m_items[index].id);
        saveReadIds();
        QModelIndex idx = createIndex(index, 0);
        emit dataChanged(idx, idx, { IsReadRole });
        emit hasUnreadChanged();
    }
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

void NewsModel::updateModel(const QJsonArray &serverItems)
{
    QSet<QString> existingIds;

    for (const NewsItem &item : m_items) {
        existingIds.insert(item.id);
    }

    QList<QJsonObject> objects;
    QSet<QString> ids;

    for (const QJsonValue &value : serverItems) {
        if (!value.isObject())
            continue;

        objects.append(value.toObject());
        ids.insert(value.toObject().value("id").toString());
    }

    QList<NewsItem> newItems;

    int i = 0;
    while (!ids.contains(existingIds)) {
        if (!ids.contains(m_items[i].id)) {
            QString toRemove = m_items[i].id;
            m_items.erase(m_items.begin() + i);
            existingIds.remove(toRemove);
        }
        i++;
    }

    for (i = 0; i < objects.size(); i++) {
        if (existingIds.contains(objects[i].value("id").toString())) {
            if (m_items[i].title != objects[i].value("title").toString()
                or m_items[i].content != objects[i].value("content").toString()
                or m_items[i].timestamp != QDateTime::fromString(objects[i].value("timestamp").toString(), Qt::ISODate)) {
                QString toRemove = m_items[i].id;
                m_items.erase(m_items.begin() + i);
                existingIds.remove(toRemove);
            }
        }

        if (!existingIds.contains(objects[i].value("id").toString())) {
            NewsItem item;
            item.id = objects[i].value("id").toString();
            item.title = objects[i].value("title").toString();
            item.content = objects[i].value("content").toString();
            item.timestamp = QDateTime::fromString(objects[i].value("timestamp").toString(), Qt::ISODate);
            item.read = m_readIds.contains(objects[i].value("id").toString());
            newItems.append(item);
            existingIds.insert(item.id);
        }
    }

    beginResetModel();
    m_items.append(newItems);
    std::sort(m_items.begin(), m_items.end(), [](const NewsItem &a, const NewsItem &b) { return a.timestamp > b.timestamp; });
    endResetModel();
    emit hasUnreadChanged();
}

bool NewsModel::hasUnread() const
{
    for (const NewsItem &item : m_items) {
        if (!item.read)
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

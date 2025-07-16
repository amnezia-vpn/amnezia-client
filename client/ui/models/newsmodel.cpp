#include "ui/models/newsmodel.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QQmlEngine>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <algorithm>

NewsModel::NewsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadLocalNews();
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
    case IdRole:
        return item.id;
    case TitleRole:
        return item.title;
    case ContentRole:
        return item.content;
    case TimestampRole:
        return item.timestamp.toString(Qt::ISODate);
    case IsReadRole:
        return item.read;
    case IsProcessedRole:
        return index.row() == m_processedIndex;
    default:
        return QVariant();
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
        QModelIndex idx = createIndex(index, 0);
        emit dataChanged(idx, idx, {IsReadRole});
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

    QList<NewsItem> newItems;
    for (const QJsonValue &value : serverItems) {
        if (!value.isObject()) continue;
        const QJsonObject obj = value.toObject();
        QString id = obj.value("id").toString();

        if (!existingIds.contains(id)) {
            NewsItem item;
            item.id = id;
            item.title = obj.value("title").toString();
            item.content = obj.value("content").toString();
            item.timestamp = QDateTime::fromString(obj.value("timestamp").toString(), Qt::ISODate);
            item.read = false; // New news is always unread
            newItems.append(item);
            existingIds.insert(id);
        }
    }

    if (!newItems.isEmpty()) {
        beginResetModel();
        m_items.append(newItems);
        // Sort descending by timestamp (newest first)
        std::sort(m_items.begin(), m_items.end(), [](const NewsItem &a, const NewsItem &b) {
            return a.timestamp > b.timestamp;
        });
        endResetModel();
        emit hasUnreadChanged();
    }
}

bool NewsModel::hasUnread() const
{
    for (const NewsItem &item : m_items) {
        if (!item.read)
            return true;
    }
    return false;
}

QString NewsModel::localFilePath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path + "/news.json";
}

void NewsModel::loadLocalNews()
{
    QFile file(localFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        return;
    }

    beginResetModel();
    m_items.clear();

    QJsonArray newsArray = doc.array();
    for (const QJsonValue &value : newsArray) {
        if (!value.isObject())
            continue;
        const QJsonObject obj = value.toObject();
        NewsItem item;
        item.id = obj.value("id").toString();
        item.title = obj.value("title").toString();
        item.content = obj.value("content").toString();
        item.timestamp = QDateTime::fromString(obj.value("timestamp").toString(), Qt::ISODate);
        item.read = obj.value("read").toBool();
        m_items.append(item);
    }
    endResetModel();
}

void NewsModel::saveLocalNews() const
{
    QJsonArray newsArray;
    for (const auto &item : m_items) {
        QJsonObject obj;
        obj["id"] = item.id;
        obj["title"] = item.title;
        obj["content"] = item.content;
        obj["timestamp"] = item.timestamp.toString(Qt::ISODate);
        obj["read"] = item.read;
        newsArray.append(obj);
    }

    QJsonDocument doc(newsArray);
    QFile file(localFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
        file.close();
    } else {
        qWarning() << "Could not save news to" << localFilePath();
    }
}
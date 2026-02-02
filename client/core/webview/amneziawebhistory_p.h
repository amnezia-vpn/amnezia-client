#ifndef WEBHISTORY_P_H
#define WEBHISTORY_P_H

#include <QUrl>

#include "amneziawebhistory.h"

class AmneziaWebHistoryItemPrivate;
class AmneziaWebHistoryItem;

class AmneziaWebHistoryPrivate
{
    Q_DECLARE_PUBLIC(AmneziaWebHistory)
public:
    
    static AmneziaWebHistoryPrivate *get(AmneziaWebHistory *q)
    {
        if (!q) { return nullptr; }
        return q->d_func();
    }

    AmneziaWebHistoryPrivate(): currentIndex(-1), maximumCount(10), q_ptr(nullptr) { }
    ~AmneziaWebHistoryPrivate() = default;


private:
    friend class AmneziaWebHistoryItemPrivate;
    int currentIndex;
    int maximumCount;
    QList<AmneziaWebHistoryItem> items;
    AmneziaWebHistory *q_ptr;
};

class AmneziaWebHistoryItemPrivate : public QSharedData
{
public:
    
    static QExplicitlySharedDataPointer<AmneziaWebHistoryItemPrivate> get(AmneziaWebHistoryItem *q)
    {
        return q->d_ptr;
    }

    ~AmneziaWebHistoryItemPrivate()
    {
    }

    QUrl url() const { return _url; }
    QString title() const { return _title; }
    QIcon icon() const {return _icon;}
    QByteArray data() const {return _data;}
    QString mimeType() const {return _mimeType;}

    // Every item knows its back and forward items
    AmneziaWebHistoryItemPrivate *backItem() {return _backItem; }
    AmneziaWebHistoryItemPrivate *forwardItem() {return _forwardItem; }
    
private:
    friend class AmneziaWebHistory;
    AmneziaWebHistoryItemPrivate() = default;

    AmneziaWebHistoryItemPrivate *_backItem = nullptr;
    AmneziaWebHistoryItemPrivate *_forwardItem = nullptr;
    QIcon _icon;
    QString _title;
    QUrl _url;
    QString _html;
    QByteArray _data;
    QString _mimeType;
};


#endif

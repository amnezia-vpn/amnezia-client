#include "qmlUtils.h"

#include <QPointF>
#include <QQuickItem>
#include <QQuickWindow>

namespace FocusControl
{
    QPointF getItemCenterPointOnScene(QQuickItem *item)
    {
        const auto x0 = item->x() + (item->width() / 2);
        const auto y0 = item->y() + (item->height() / 2);
        return item->parentItem()->mapToScene(QPointF { x0, y0 });
    }

    bool isEnabled(QObject *obj)
    {
        const auto item = qobject_cast<QQuickItem *>(obj);
        return item && item->isEnabled();
    }

    bool isVisible(QObject *item)
    {
        const auto res = item->property("visible").toBool();
        return res;
    }

    bool isFocusable(QObject *item)
    {
        const auto res = item->property("isFocusable").toBool();
        return res;
    }

    bool isListView(QObject *item)
    {
        return item->inherits("QQuickListView");
    }

    bool isOnTheScene(QObject *object)
    {
        QQuickItem *item = qobject_cast<QQuickItem *>(object);
        if (!item) {
            qWarning() << "Couldn't recognize object as item";
            return false;
        }

        if (!item->isVisible()) {
            return false;
        }

        QRectF itemRect = item->mapRectToScene(item->childrenRect());

        QQuickWindow *window = item->window();
        if (!window) {
            qWarning() << "Couldn't get the window on the Scene check";
            return false;
        }

        const auto contentItem = window->contentItem();
        if (!contentItem) {
            qWarning() << "Couldn't get the content item on the Scene check";
            return false;
        }
        QRectF windowRect = contentItem->childrenRect();
        const auto res = (windowRect.contains(itemRect) || isListView(item));
        return res;
    }

    bool isMore(QObject *item1, QObject *item2)
    {
        return !isLess(item1, item2);
    }

    bool isLess(QObject *item1, QObject *item2)
    {
        const auto p1 = getItemCenterPointOnScene(qobject_cast<QQuickItem *>(item1));
        const auto p2 = getItemCenterPointOnScene(qobject_cast<QQuickItem *>(item2));
        return (p1.y() == p2.y()) ? (p1.x() < p2.x()) : (p1.y() < p2.y());
    }

    QList<QObject *> getSubChain(QObject *object)
    {
        QList<QObject *> res;
        if (!object) {
            return res;
        }

        const auto children = object->children();

        for (const auto child : children) {
            if (child && isFocusable(child) && isOnTheScene(child) && isEnabled(child)) {
                res.append(child);
            } else {
                res.append(getSubChain(child));
            }
        }
        return res;
    }

    QList<QObject *> getItemsChain(QObject *object)
    {
        QList<QObject *> res;
        if (!object) {
            return res;
        }

        const auto children = object->children();

        for (const auto child : children) {
            if (child && isFocusable(child) && isEnabled(child) && isVisible(child)) {
                res.append(child);
            } else {
                res.append(getItemsChain(child));
            }
        }
        return res;
    }

    void printItems(const QList<QObject *> &items, QObject *current_item)
    {
        for (const auto &item : items) {
            QQuickItem *i = qobject_cast<QQuickItem *>(item);
            QPointF coords { getItemCenterPointOnScene(i) };
            QString prefix = current_item == i ? "==>" : "   ";
            qDebug() << prefix << " Item: " << i << " with coords: " << coords;
        }
    }
} // namespace FocusControl
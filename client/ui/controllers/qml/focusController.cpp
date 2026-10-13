#include "focusController.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStyleHints>

#include <algorithm>

namespace
{
    constexpr qreal ensureVisibleMargin = 40;

    QQuickItem *focusChainRoot(QQuickItem *item)
    {
        for (QQuickItem *it = item; it; it = it->parentItem()) {
            if (it->inherits("QQuickPopupItem")) {
                return it;
            }
        }
        return item->window()->contentItem();
    }
}

FocusController::FocusController(QQmlApplicationEngine *engine, QObject *parent) : QObject { parent }
{
    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);

    connect(engine, &QQmlApplicationEngine::objectCreated, this, [this](QObject *object) {
        if (auto *window = qobject_cast<QQuickWindow *>(object)) {
            window->installEventFilter(this);
        }
    });
}

bool FocusController::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) {
        return false;
    }

    auto *window = qobject_cast<QQuickWindow *>(watched);
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (!window || (keyEvent->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier))) {
        return false;
    }

    QQuickItem *current = window->activeFocusItem() ? window->activeFocusItem() : window->contentItem();

    bool forward = false;
    switch (keyEvent->key()) {
    case Qt::Key_Tab: forward = true; break;
    case Qt::Key_Backtab: forward = false; break;
    case Qt::Key_Down:
    case Qt::Key_Right: forward = true; [[fallthrough]];
    case Qt::Key_Up:
    case Qt::Key_Left:
        if (wantsArrowKey(current, keyEvent->key())) {
            return false;
        }
        break;
    default: return false;
    }

    sortListViewDelegates(focusChainRoot(current));

    QQuickItem *next = current->nextItemInFocusChain(forward);
    if (next && next != current) {
        next->forceActiveFocus(forward ? Qt::TabFocusReason : Qt::BacktabFocusReason);
        ensureVisible(next);
    }
    return true;
}

bool FocusController::wantsArrowKey(const QQuickItem *item, int key)
{
    if (item->inherits("QQuickTextEdit")) {
        return !item->property("readOnly").toBool();
    }
    if (item->inherits("QQuickTextInput")) {
        return key == Qt::Key_Left || key == Qt::Key_Right;
    }
    return false;
}

void FocusController::sortListViewDelegates(QQuickItem *root)
{
    if (!root->isVisible()) {
        return;
    }

    if (root->inherits("QQuickListView")) {
        if (auto *contentItem = root->property("contentItem").value<QQuickItem *>()) {
            QList<QQuickItem *> delegates = contentItem->childItems();
            const auto byPosition = [](const QQuickItem *a, const QQuickItem *b) {
                return a->y() != b->y() ? a->y() < b->y() : a->x() < b->x();
            };
            if (!std::is_sorted(delegates.cbegin(), delegates.cend(), byPosition)) {
                std::stable_sort(delegates.begin(), delegates.end(), byPosition);
                for (qsizetype i = 1; i < delegates.size(); ++i) {
                    delegates.at(i)->stackAfter(delegates.at(i - 1));
                }
            }
        }
    }

    const QList<QQuickItem *> children = root->childItems();
    for (QQuickItem *child : children) {
        sortListViewDelegates(child);
    }
}

void FocusController::ensureVisible(QQuickItem *item)
{
    for (QQuickItem *flickable = item->parentItem(); flickable; flickable = flickable->parentItem()) {
        if (!flickable->inherits("QQuickFlickable")) {
            continue;
        }

        auto *contentItem = flickable->property("contentItem").value<QQuickItem *>();
        if (!contentItem) {
            continue;
        }

        const QRectF rect = item->mapRectToItem(contentItem, item->boundingRect());
        scrollToRange(flickable, rect.top(), rect.bottom(), flickable->height(), "contentY", "originY",
                      "contentHeight", "topMargin", "bottomMargin");
        scrollToRange(flickable, rect.left(), rect.right(), flickable->width(), "contentX", "originX",
                      "contentWidth", "leftMargin", "rightMargin");
    }
}

void FocusController::scrollToRange(QQuickItem *flickable, qreal start, qreal end, qreal viewSize, const char *position,
                                   const char *origin, const char *contentSize, const char *startMargin,
                                   const char *endMargin)
{
    const qreal current = flickable->property(position).toReal();

    qreal target = current;
    if (start < current) {
        target = start - ensureVisibleMargin;
    } else if (end > current + viewSize) {
        target = std::min(end - viewSize + ensureVisibleMargin, start - ensureVisibleMargin);
    }

    if (target != current) {
        const qreal min = flickable->property(origin).toReal() - flickable->property(startMargin).toReal();
        const qreal max = flickable->property(origin).toReal() + flickable->property(contentSize).toReal()
                + flickable->property(endMargin).toReal() - viewSize;
        flickable->setProperty(position, std::clamp(target, min, std::max(min, max)));
    }
}

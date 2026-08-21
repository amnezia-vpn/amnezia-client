#include "focusController.h"
#include "ui/utils/qmlUtils.h"

#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include "logger.h"

namespace {
    Logger logger("FocusController");

    QString rootObjectName(const QObject *object)
    {
        if (object == nullptr) {
            return QStringLiteral("null");
        }
        const QString className = QString::fromLatin1(object->metaObject()->className());
        const QString name = object->objectName();
        return name.isEmpty() ? className : className + QLatin1Char('/') + name;
    }
}

FocusController::FocusController(QQmlApplicationEngine *engine, QObject *parent)
    : QObject { parent },
      m_engine { engine },
      m_focusChain {},
      m_focusedItem { nullptr },
      m_rootObjects {},
      m_defaultFocusItem { nullptr },
      m_lvfc { nullptr }
{
    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated, this, [this](QObject *object, const QUrl &url) {
        QQuickItem *newDefaultFocusItem = object->findChild<QQuickItem *>("defaultFocusItem");
        if (newDefaultFocusItem && m_defaultFocusItem != newDefaultFocusItem) {
            m_defaultFocusItem = newDefaultFocusItem;
        }
    });

    QObject::connect(this, &FocusController::focusedItemChanged, this,
                     [this]() { m_focusedItem->forceActiveFocus(Qt::TabFocusReason); });
}

void FocusController::nextKeyTabItem()
{
    nextItem(Direction::Forward);
}

void FocusController::previousKeyTabItem()
{
    nextItem(Direction::Backward);
}

void FocusController::nextKeyUpItem()
{
    nextItem(Direction::Backward);
}

void FocusController::nextKeyDownItem()
{
    nextItem(Direction::Forward);
}

void FocusController::nextKeyLeftItem()
{
    nextItem(Direction::Backward);
}

void FocusController::nextKeyRightItem()
{
    nextItem(Direction::Forward);
}

void FocusController::setFocusItem(QQuickItem *item)
{
    if (m_focusedItem != item) {
        m_focusedItem = item;
    }
    emit focusedItemChanged();
}

void FocusController::setFocusOnDefaultItem()
{
    setFocusItem(m_defaultFocusItem);
}

void FocusController::pushRootObject(QObject *object)
{
    m_rootObjects.push(object);
    logger.debug() << "pushRootObject:" << rootObjectName(object) << "depth=" << m_rootObjects.size();
    dropListView();
    // setFocusOnDefaultItem();
}

void FocusController::dropRootObject(QObject *object)
{
    if (m_rootObjects.empty()) {
        logger.warning() << "dropRootObject: the stack is already empty, this drop had no matching push, object="
                         << rootObjectName(object);
        return;
    }

    if (m_rootObjects.top() == object) {
        m_rootObjects.pop();
        logger.debug() << "dropRootObject:" << rootObjectName(object) << "depth=" << m_rootObjects.size();
        dropListView();
        setFocusOnDefaultItem();
    } else {
        logger.warning() << "TRY TO DROP WRONG ROOT OBJECT: top=" << rootObjectName(m_rootObjects.top())
                         << "SHOULD BE:" << rootObjectName(object) << "depth=" << m_rootObjects.size()
                         << ", the stack is left as is and stays out of sync";
    }
}

void FocusController::resetRootObject()
{
    if (!m_rootObjects.empty()) {
        logger.debug() << "resetRootObject: dropping" << m_rootObjects.size() << "objects, top="
                       << rootObjectName(m_rootObjects.top());
    }
    m_rootObjects.clear();
}

void FocusController::reload(Direction direction)
{
    m_focusChain.clear();

    QObject *rootObject = (m_rootObjects.empty() ? m_engine->rootObjects().value(0) : m_rootObjects.top());

    if (!rootObject) {
        logger.error() << "No ROOT OBJECT found!";
        resetRootObject();
        dropListView();
        return;
    }

    m_focusChain.append(FocusControl::getSubChain(rootObject));

    std::sort(m_focusChain.begin(), m_focusChain.end(),
              direction == Direction::Forward ? FocusControl::isLess : FocusControl::isMore);

    if (m_focusChain.empty()) {
        logger.warning() << "Focus chain is empty!";
        resetRootObject();
        dropListView();
        return;
    }
}

void FocusController::nextItem(Direction direction)
{
    reload(direction);

    if (m_lvfc && FocusControl::isListView(m_focusedItem)) {
        direction == Direction::Forward ? focusNextListViewItem() : focusPreviousListViewItem();

        return;
    }

    if (m_focusChain.empty()) {
        logger.warning() << "There are no items to navigate";
        setFocusOnDefaultItem();
        return;
    }

    auto focusedItemIndex = m_focusChain.indexOf(m_focusedItem);

    if (focusedItemIndex == -1) {
        focusedItemIndex = 0;
    } else if (focusedItemIndex == (m_focusChain.size() - 1)) {
        focusedItemIndex = 0;
    } else {
        focusedItemIndex++;
    }

    const auto focusedItem = qobject_cast<QQuickItem *>(m_focusChain.at(focusedItemIndex));

    if (focusedItem == nullptr) {
        logger.warning() << "Failed to get item to focus on. Setting focus on default";
        setFocusOnDefaultItem();
        return;
    }

    if (FocusControl::isListView(focusedItem)) {
        m_lvfc = new ListViewFocusController(focusedItem, this);
        m_focusedItem = focusedItem;
        if (direction == Direction::Forward) {
            m_lvfc->nextDelegate();
            focusNextListViewItem();
        } else {
            m_lvfc->previousDelegate();
            focusPreviousListViewItem();
        }
        return;
    }

    setFocusItem(focusedItem);
}

void FocusController::focusNextListViewItem()
{
    m_lvfc->reloadFocusChain();
    if (m_lvfc->isLastFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        dropListView();
        nextItem(Direction::Forward);
        return;
    } else if (m_lvfc->isLastFocusItemInDelegate()) {
        m_lvfc->resetFocusChain();
        m_lvfc->nextDelegate();
    }

    m_lvfc->focusNextItem();
}

void FocusController::focusPreviousListViewItem()
{
    m_lvfc->reloadFocusChain();
    if (m_lvfc->isFirstFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        dropListView();
        nextItem(Direction::Backward);
        return;
    } else if (m_lvfc->isFirstFocusItemInDelegate()) {
        m_lvfc->resetFocusChain();
        m_lvfc->previousDelegate();
    }

    m_lvfc->focusPreviousItem();
}

void FocusController::dropListView()
{
    if (m_lvfc) {
        delete m_lvfc;
        m_lvfc = nullptr;
    }
}

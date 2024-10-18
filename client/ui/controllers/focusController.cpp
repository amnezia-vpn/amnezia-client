#include "focusController.h"

#include "listViewFocusController.h"

#include <QQuickWindow>
#include <QQmlApplicationEngine>


bool isListView(QObject* item)
{
    return item->inherits("QQuickListView");
}

bool isOnTheScene(QObject* object)
{
    QQuickItem* item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        qWarning() << "Couldn't recognize object as item";
        return false;
    }

    if (!item->isVisible()) {
        // qDebug() << "===>> The item is not visible: " << item;
        return false;
    }

    QRectF itemRect = item->mapRectToScene(item->childrenRect());

    QQuickWindow* window = item->window();
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
    // qDebug() << (res ? "===>> item is inside the Scene" : "===>> ITEM IS OUTSIDE THE SCENE") << " itemRect: " << itemRect << "; windowRect: " << windowRect;
    return res;
}

QList<QObject*> getSubChain(QObject* object)
{
    QList<QObject*> res;
    if (!object) {
        qDebug() << "The object is NULL";
        return res;
    }

    const auto children = object->children();

    for(const auto child : children) {
        if (child
            && isFocusable(child)
            && isOnTheScene(child)
            && isEnabled(child)
            ) {
            res.append(child);
        } else {
            res.append(getSubChain(child));
        }
    }
    return res;
}

FocusController::FocusController(QQmlApplicationEngine* engine, QObject *parent)
    : QObject{parent}
    , m_engine{engine}
    , m_focusChain{}
    , m_focusedItem{nullptr}
    , m_focusedItemIndex{-1}
    , m_rootObjects{}
    , m_defaultFocusItem{QSharedPointer<QQuickItem>()}
    , m_lvfc{nullptr}
{
    QObject::connect(m_engine.get(), &QQmlApplicationEngine::objectCreated, this, [this](QObject *object, const QUrl &url){
        QQuickItem* newDefaultFocusItem = object->findChild<QQuickItem*>("defaultFocusItem");
        if(newDefaultFocusItem && m_defaultFocusItem != newDefaultFocusItem) {
            m_defaultFocusItem.reset(newDefaultFocusItem);
            qDebug() << "===>> NEW DEFAULT FOCUS ITEM " << m_defaultFocusItem;
        }
    });
}

void FocusController::nextItem(Direction direction)
{
    if (m_lvfc) {
        direction == Direction::Forward ? focusNextListViewItem() : focusPreviousListViewItem();
        qDebug() << "===>> [handling the ListView]";

        return;
    }

    reload(direction);

    if(m_focusChain.empty()) {
        qWarning() << "There are no items to navigate";
        return;
    }

    if (m_focusedItemIndex == (m_focusChain.size() - 1)) {
        qDebug() << "Last focus index. Making it zero";
        m_focusedItemIndex = 0;
    } else {
        qDebug() << "Incrementing focus index";
        m_focusedItemIndex++;
    }

    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));

    if(m_focusedItem == nullptr) {
        qWarning() << "Failed to get item to focus on. Setting focus on default";
        m_focusedItem = m_defaultFocusItem.get();
        return;
    }
    
    if(isListView(m_focusedItem)) {
        qDebug() << "===>> [Found ListView]";
        m_lvfc = new ListViewFocusController(m_focusedItem, this);
        if(direction == Direction::Forward) {
            m_lvfc->viewToBegin();
            m_lvfc->nextDelegate();
            focusNextListViewItem();
        } else {
            m_lvfc->viewToEnd();
            m_lvfc->previousDelegate();
            focusPreviousListViewItem();
        }
        return;
    }

    qDebug() << "===>> Focused Item: " << m_focusedItem;
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);

    printItems(m_focusChain, m_focusedItem);

    const auto w = m_defaultFocusItem->window();

    qDebug() << "===>> CURRENT ACTIVE ITEM: " << w->activeFocusItem();
    qDebug() << "===>> CURRENT FOCUS OBJECT: " << w->focusObject();
    if(m_rootObjects.empty()) {
        qDebug() << "===>> ROOT OBJECT IS DEFAULT";
    } else {
        qDebug() << "===>> ROOT OBJECT: " << m_rootObjects.top();
    }
}

void FocusController::focusNextListViewItem()
{
    if (m_lvfc->isLastFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        qDebug() << "===>> [Last item in ListView was reached. Going to the NEXT element after ListView]";
        delete m_lvfc;
        m_lvfc = nullptr;
        nextItem(Direction::Forward);
        return;
    } else if (m_lvfc->isLastFocusItemInDelegate()) {
        qDebug() << "===>> [End of delegate elements was reached. Going to the next delegate]";
        m_lvfc->resetFocusChain();
        m_lvfc->nextDelegate();
        m_lvfc->viewAtCurrentIndex();
    }

    m_lvfc->focusNextItem();
}

void FocusController::focusPreviousListViewItem()
{
    if (m_lvfc->isFirstFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        qDebug() << "===>> [First item in ListView was reached. Going to the PREVIOUS element after ListView]";
        delete m_lvfc;
        m_lvfc = nullptr;
        nextItem(Direction::Backward);
        return;
    } else if (m_lvfc->isFirstFocusItemInDelegate()) {
        m_lvfc->resetFocusChain();
        m_lvfc->previousDelegate();
        m_lvfc->viewAtCurrentIndex();
    }

    m_lvfc->focusPreviousItem();
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

void FocusController::setFocusOnDefaultItem()
{
    qDebug() << "===>> Setting focus on DEFAULT FOCUS ITEM...";
    m_defaultFocusItem->forceActiveFocus();
}

void FocusController::reload(Direction direction)
{
        m_focusChain.clear();

    QObject* rootObject = (m_rootObjects.empty()
                               ? m_engine->rootObjects().value(0)
                               : m_rootObjects.top());

    if(!rootObject) {
        qCritical() << "No ROOT OBJECT found!";
        m_focusedItemIndex = -1;
        resetRootObject();
        setFocusOnDefaultItem();
        return;
    }

    qDebug() << "===>> ROOT OBJECTS: " << rootObject;

    m_focusChain.append(getSubChain(rootObject));

    std::sort(m_focusChain.begin(), m_focusChain.end(), direction == Direction::Forward ? isLess : isMore);

    if (m_focusChain.empty()) {
        qWarning() << "Focus chain is empty!";
        m_focusedItemIndex = -1;
        resetRootObject();
        setFocusOnDefaultItem();
        return;
    }

    m_focusedItemIndex = m_focusChain.indexOf(m_focusedItem);

    if(m_focusedItemIndex == -1) {
        qInfo() << "No focus item in chain.";
        setFocusOnDefaultItem();
        return;
    }
}

void FocusController::pushRootObject(QObject* object)
{
    m_rootObjects.push(object);
    qDebug() << "===>> ROOT OBJECT is changed to: " << m_rootObjects.top();
}

void FocusController::dropRootObject(QObject* object)
{
    if (m_rootObjects.empty()) {
        qDebug() << "ROOT OBJECT is already NULL";

        return;
    }

    if (m_rootObjects.top() == object) {
        m_rootObjects.pop();
        if(m_rootObjects.size()) {
            qDebug() << "===>> ROOT OBJECT is changed to: " << m_rootObjects.top();
        } else {
            qDebug() << "===>> ROOT OBJECT is changed to NULL";
        }
    } else {
        qWarning() << "===>> TRY TO DROP WRONG ROOT OBJECT: " << m_rootObjects.top() << " SHOULD BE: " << object;
    }
}

void FocusController::resetRootObject()
{
    m_rootObjects.clear();
    qDebug() << "===>> ROOT OBJECT IS RESETED";
}

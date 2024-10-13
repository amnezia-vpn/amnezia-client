#include "focusController.h"

#include "listViewFocusController.h"

#include <QQuickWindow>
#include <QQmlApplicationEngine>


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

void FocusController::nextItem(bool isForwardOrder)
{
    if (m_lvfc) {
        isForwardOrder ? focusNextListViewItem() : focusPreviousListViewItem();
        qDebug() << "===>> [handling the ListView]";

        return;
    }

    reload(isForwardOrder);

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
        if(isForwardOrder) {
            m_lvfc->viewToBegin();
            m_lvfc->nextElement();
            focusNextListViewItem();
        } else {
            m_lvfc->viewToEnd();
            m_lvfc->previousElement();
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
    m_lvfc->focusNextItem();

    if (m_lvfc->isLastFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        qDebug() << "===>> [Last item in ListView was reached]";
        delete m_lvfc;
        m_lvfc = nullptr;
    } else if (m_lvfc->isLastFocusItemInDelegate()) {
        qDebug() << "===>> [End of delegate elements was reached. Going to the next delegate]";
        m_lvfc->resetFocusChain();
        m_lvfc->nextElement();
        m_lvfc->viewAtCurrentIndex();
    }
}

void FocusController::focusPreviousListViewItem()
{
    m_lvfc->focusPreviousItem();

    if (m_lvfc->isFirstFocusItemInListView() || m_lvfc->isReturnNeeded()) {
        delete m_lvfc;
        m_lvfc = nullptr;
    } else if (m_lvfc->isFirstFocusItemInDelegate()) {
        m_lvfc->resetFocusChain();
        m_lvfc->decrementIndex();
        m_lvfc->viewAtCurrentIndex();
    }
}

void FocusController::nextKeyTabItem()
{
    nextItem(true);
}

void FocusController::previousKeyTabItem()
{
    nextItem(false);
}

void FocusController::nextKeyUpItem()
{
    nextItem(false);
}

void FocusController::nextKeyDownItem()
{
    nextItem(true);
}

void FocusController::nextKeyLeftItem()
{
    nextItem(false);
}

void FocusController::nextKeyRightItem()
{
    nextItem(true);
}

void FocusController::setFocusOnDefaultItem()
{
    qDebug() << "===>> Setting focus on DEFAULT FOCUS ITEM...";
    m_defaultFocusItem->forceActiveFocus();
}

void FocusController::reload(bool isForwardOrder)
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

    std::sort(m_focusChain.begin(), m_focusChain.end(), isForwardOrder? isLess : isMore);

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

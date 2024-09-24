#include "focusController.h"

#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQueue>
#include <QPointF>
#include <QRectF>


bool isVisible(QObject* item)
{
    const auto res = item->property("visible").toBool();
    // qDebug() << "==>> " << (res ? "VISIBLE" : "NOT visible") << item;
    return res;
}

bool isFocusable(QObject* item)
{
    const auto res = item->property("isFocusable").toBool();
    return res;
}

QRectF getItemCoordsOnScene(QQuickItem* item) // TODO: remove?
{
    if (!item) return {};
    return item->mapRectToScene(item->childrenRect());
}

QPointF getItemCenterPointOnScene(QQuickItem* item)
{
    const auto x0 = item->x() + (item->width() / 2);
    const auto y0 = item->y() + (item->height() / 2);
    return item->parentItem()->mapToScene(QPointF{x0, y0});
}

bool isLess(QObject* item1, QObject* item2)
{
    const auto p1 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item1));
    const auto p2 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item2));
    return (p1.y() == p2.y()) ? (p1.x() < p2.x()) : (p1.y() < p2.y());
}

bool isMore(QObject* item1, QObject* item2)
{
    return !isLess(item1, item2);
}

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

bool isEnabled(QObject* obj)
{
    const auto item = qobject_cast<QQuickItem*>(obj);
    return item && item->isEnabled();
}

QQuickItem* getPageOfItem(QQuickItem* item) // TODO: remove?
{
    if(!item) {
        qWarning() << "item is null";
        return {};
    }
    const auto pagePattern = QString::fromLatin1("Page");
    QString className{item->metaObject()->className()};
    const auto isPage = className.contains(pagePattern, Qt::CaseSensitive);
    if(isPage) {
        return item;
    } else {
        return getPageOfItem(item->parentItem());
    }
}

QList<QObject*> getSubChain(QObject* item)
{
    QList<QObject*> res;
    if (!item) {
        qDebug() << "null top item";
        return res;
    }
    const auto children = item->children();
    for(const auto child : children) {
        if (child
            && isFocusable(child)
            && (isOnTheScene(child))
            && isEnabled(child)
            ) {
            res.append(child);
        } else {
            res.append(getSubChain(child));
        }
    }
    return res;
}

template<typename T>
void printItems(const T& items, QObject* current_item)
{
    for(const auto& item : items) {
        QQuickItem* i = qobject_cast<QQuickItem*>(item);
        QPointF coords {getItemCenterPointOnScene(i)};
        QString prefix = current_item == i ? "==>" : "   ";
        qDebug() << prefix << " Item: " << i << " with coords: " << coords;
    }
}

/*!
 * \brief The ListViewFocusController class manages the focus of elements in ListView
 * \details This class object moving focus to ListView's controls since ListView stores
 *          it's data implicitly and it could be got one by one.
 *
 *          This class was made to store as less as possible data getting it from QML
 *          when it's needed.
 */
class ListViewFocusController : public QObject
{
public:
    explicit ListViewFocusController(QQuickItem* listView, QObject* parent = nullptr);
    ~ListViewFocusController();

    void incrementIndex();
    void decrementIndex();
    void positionViewAtIndex();
    void focusNextItem();
    void focusPreviousItem();
    void resetFocusChain();
    bool isListViewFirstFocusItem();
    bool isDelegateFirstFocusItem();
    bool isListViewLastFocusItem();
    bool isDelegateLastFocusItem();
    bool isReturnNeeded();
    void viewToBegin();

private:
    int size() const;
    int currentIndex() const;
    QQuickItem* itemAtIndex(const int index);
    QQuickItem* currentDelegate();
    QQuickItem* focusedItem();

    QQuickItem* m_listView;
    QList<QObject*> m_focusChain;
    QQuickItem* m_focusedItem;
    qsizetype m_focusedItemIndex;
    qsizetype m_delegateIndex;
    bool m_isReturnNeeded;
};

ListViewFocusController::ListViewFocusController(QQuickItem* listView, QObject* parent)
    : QObject{parent}
    , m_listView{listView}
    , m_focusChain{}
    , m_focusedItem{nullptr}
    , m_focusedItemIndex{-1}
    , m_delegateIndex{0}
    , m_isReturnNeeded{false}
{
}

ListViewFocusController::~ListViewFocusController()
{

}

void ListViewFocusController::positionViewAtIndex()
{
    QMetaObject::invokeMethod(m_listView, "positionViewAtIndex",
                              Q_ARG(int, m_delegateIndex),  // Index
                              Q_ARG(int, 2));    // PositionMode (0 = Visible)
}

int ListViewFocusController::size() const
{
    return m_listView->property("count").toInt();
}

int ListViewFocusController::currentIndex() const
{
    return m_delegateIndex;
}

void ListViewFocusController::incrementIndex()
{
    m_delegateIndex++;
}

void ListViewFocusController::decrementIndex()
{
    m_delegateIndex--;
}

QQuickItem* ListViewFocusController::itemAtIndex(const int index)
{
    QQuickItem* item{nullptr};

    QMetaObject::invokeMethod(m_listView, "itemAtIndex",
                              Q_RETURN_ARG(QQuickItem*, item),
                              Q_ARG(int, index));

    return item;
}

QQuickItem* ListViewFocusController::currentDelegate()
{
    return itemAtIndex(m_delegateIndex);
}

QQuickItem* ListViewFocusController::focusedItem()
{
    return m_focusedItem;
}

void ListViewFocusController::focusNextItem()
{
    if (m_focusChain.empty()) {
        qDebug() << "Empty focusChain with current delegate: " << currentDelegate() << "Scanning for elements...";
        m_focusChain = getSubChain(currentDelegate());
    }
    if (m_focusChain.empty()) {
        qWarning() << "No elements found. Returning from ListView...";
        m_isReturnNeeded = true;
        return;
    }
    m_focusedItemIndex++;
    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
    qDebug() << "==>> Focused Item: " << m_focusedItem << " with Index: " << m_focusedItemIndex;
    m_focusedItem->forceActiveFocus();
}

void ListViewFocusController::focusPreviousItem()
{
    // TODO: implement
}

void ListViewFocusController::resetFocusChain()
{
    m_focusChain.clear();
    m_focusedItem = nullptr;
    m_focusedItemIndex = -1;
}

bool ListViewFocusController::isDelegateFirstFocusItem()
{
    return m_focusedItem && (m_focusedItem == m_focusChain.first());
}

bool ListViewFocusController::isDelegateLastFocusItem()
{
    return m_focusedItem && (m_focusedItem == m_focusChain.last());
}

bool ListViewFocusController::isListViewFirstFocusItem()
{
    return (m_delegateIndex == 0) && isDelegateFirstFocusItem();
}

bool ListViewFocusController::isListViewLastFocusItem()
{
    return (m_delegateIndex == size() - 1) && isDelegateLastFocusItem();
}

bool ListViewFocusController::isReturnNeeded()
{
    return m_isReturnNeeded;
}

void ListViewFocusController::viewToBegin()
{
    QMetaObject::invokeMethod(m_listView, "positionViewAtBeginning", Qt::AutoConnection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
    // connect(this, &FocusController::rootItemChanged, this, &FocusController::onReload);
    QObject::connect(m_engine.get(), &QQmlApplicationEngine::objectCreated, this, [this](QObject *object, const QUrl &url){
        qDebug() << "===>> () CREATED " << object << " : " << url;
        QQuickItem* newDefaultFocusItem = object->findChild<QQuickItem*>("defaultFocusItem");
        if(newDefaultFocusItem && m_defaultFocusItem != newDefaultFocusItem) {
            m_defaultFocusItem.reset(newDefaultFocusItem);
            qDebug() << "===>> [] NEW DEFAULT FOCUS ITEM " << m_defaultFocusItem;
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
            focusNextListViewItem();
        } else {
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

    if (m_lvfc->isListViewLastFocusItem() || m_lvfc->isReturnNeeded()) {
        qDebug() << "===>> [Last item in ListView was reached]";
        delete m_lvfc;
        m_lvfc = nullptr;
    } else if (m_lvfc->isDelegateLastFocusItem()) {
        qDebug() << "===>> [End of delegate elements was reached. Going to the next delegate]";
        m_lvfc->resetFocusChain();
        m_lvfc->incrementIndex();
        m_lvfc->positionViewAtIndex();
    }
}

void FocusController::focusPreviousListViewItem()
{
    m_lvfc->focusPreviousItem();

    if (m_lvfc->isListViewFirstFocusItem() || m_lvfc->isReturnNeeded()) {
        delete m_lvfc;
        m_lvfc = nullptr;
    } else if (m_lvfc->isDelegateFirstFocusItem()) {
        m_lvfc->resetFocusChain();
        m_lvfc->decrementIndex();
        m_lvfc->positionViewAtIndex();
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

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
        qInfo() << "The item is not visible: " << item;
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
        // qDebug() << prefix << " Item: " << i << " with coords: " << coords; // Uncomment to visualize tab transitions
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
    void decrementCurrentIndex();
    void positionViewAtIndex();
    void focusNextItem();
    void focusPreviousItem();
    void resetFocusChain();
    bool isListViewLastFocusItem();
    bool isDelegateLastFocusItem();

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
};

ListViewFocusController::ListViewFocusController(QQuickItem* listView, QObject* parent)
    : QObject{parent}
    , m_listView{listView}
    , m_focusChain{}
    , m_focusedItem{nullptr}
    , m_focusedItemIndex{-1}
    , m_delegateIndex{0}
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

void ListViewFocusController::decrementCurrentIndex()
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
        qWarning() << "Empty focusChain with current delegate: " << currentDelegate();
        m_focusChain = getSubChain(currentDelegate());
    }
    m_focusedItemIndex++;
    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
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

bool ListViewFocusController::isDelegateLastFocusItem()
{
    return m_focusedItem && (m_focusedItem == m_focusChain.last());
}

bool ListViewFocusController::isListViewLastFocusItem()
{
    return (m_delegateIndex == size() - 1) && isDelegateLastFocusItem();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

FocusController::FocusController(QQmlApplicationEngine* engine, QObject *parent)
    : QObject{parent}
    , m_engine{engine}
    , m_focusChain{}
    , m_focusedItem{nullptr}
    , m_focusedItemIndex{-1}
    , m_rootItem{nullptr}
    , m_lvfc{nullptr}
{
    connect(this, &FocusController::rootItemChanged, this, &FocusController::reload);
}

void FocusController::resetFocus()
{
    reload();
    if (m_focusChain.empty()) {
        qWarning() << "There is no focusable elements";
        return;
    }
    if(m_focusedItemIndex == -1) {
        m_focusedItemIndex = 0;
        m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
        m_focusedItem->forceActiveFocus();
    }
}

void FocusController::nextKeyTabItem()
{
    if (m_lvfc) {
        focusNextListViewItem();
        return;
    }

    reload();

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
        qWarning() << "Failed to get item to focus on";
        return;
    }
    
    if(isListView(m_focusedItem)) {
        m_lvfc = new ListViewFocusController(m_focusedItem, this);
        focusNextListViewItem();
        return;
    }

    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);

    printItems(m_focusChain, m_focusedItem);
}

void FocusController::focusNextListViewItem()
{
    m_lvfc->focusNextItem();

    if (m_lvfc->isListViewLastFocusItem()) {
        delete m_lvfc;
        m_lvfc = nullptr;
    } else if (m_lvfc->isDelegateLastFocusItem()) {
        m_lvfc->resetFocusChain();
        m_lvfc->incrementIndex();
        m_lvfc->positionViewAtIndex();
    }
}

void FocusController::focusPreviousListViewItem()
{
    // TODO: implement
}

void FocusController::previousKeyTabItem()
{
    reload();

    if(m_focusChain.empty()) {
        return;
    }

    if (m_focusedItemIndex <= 0) {
        m_focusedItemIndex = m_focusChain.size() - 1;
    } else {
        m_focusedItemIndex--;
    }

    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);

    qDebug() << "--> Current focus was changed to " << m_focusedItem;
}

void FocusController::nextKeyUpItem()
{
    previousKeyTabItem();
}

void FocusController::nextKeyDownItem()
{
    nextKeyTabItem();
}

void FocusController::nextKeyLeftItem()
{
    previousKeyTabItem();
}

void FocusController::nextKeyRightItem()
{
    nextKeyTabItem();
}

void FocusController::reload()
{
    m_focusChain.clear();

    QObjectList rootObjects;

    const auto rootItem = m_rootItem;

    if (rootItem != nullptr) {
        rootObjects << qobject_cast<QObject*>(rootItem);
    } else {
        rootObjects = m_engine->rootObjects();
    }

    if(rootObjects.empty()) {
        qWarning() << "Empty focus chain detected!";
        emit focusChainChanged();
        return;
    }

    for(const auto object : rootObjects) {
        m_focusChain.append(getSubChain(object));
    }

    std::sort(m_focusChain.begin(), m_focusChain.end(), isLess);

    printItems(m_focusChain, m_focusedItem);

    emit focusChainChanged();

    if (m_focusChain.empty()) {
        m_focusedItemIndex = -1;
        qWarning() << "reloaded to empty focus chain";
        return;
    }

    QQuickWindow* window = qobject_cast<QQuickWindow*>(rootObjects[0]);
    if (!window) {
        window = qobject_cast<QQuickItem*>(rootObjects[0])->window();
    }

    if (!window) {
        qCritical() << "Couldn't get the current window";
        return;
    }

    m_focusedItemIndex = m_focusChain.indexOf(window->activeFocusItem());

    if(m_focusedItemIndex == -1) {
        qInfo() << "No focus item in chain. Moving focus to begin...";
        // m_focused_item_index = 0; // if not in focus chain current
        return;
    }

    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));

    m_focusedItem->forceActiveFocus();
}

void FocusController::setRootItem(QQuickItem* item)
{
    m_rootItem = item;
}

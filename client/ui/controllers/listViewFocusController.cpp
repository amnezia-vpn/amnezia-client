#include "listViewFocusController.h"
#include "utils/qmlUtils.h"

#include <QQuickWindow>

ListViewFocusController::ListViewFocusController(QQuickItem *listView, QObject *parent)
    : QObject { parent },
      m_listView { listView },
      m_focusChain {},
      m_currentSection { Section::Default },
      m_header { nullptr },
      m_footer { nullptr },
      m_focusedItem { nullptr },
      m_focusedItemIndex { -1 },
      m_delegateIndex { 0 },
      m_isReturnNeeded { false },
      m_currentSectionString { "Default", "Header", "Delegate", "Footer" }
{
    QVariant headerItemProperty = m_listView->property("headerItem");
    m_header = headerItemProperty.canConvert<QQuickItem *>() ? headerItemProperty.value<QQuickItem *>() : nullptr;

    QVariant footerItemProperty = m_listView->property("footerItem");
    m_footer = footerItemProperty.canConvert<QQuickItem *>() ? footerItemProperty.value<QQuickItem *>() : nullptr;
}

ListViewFocusController::~ListViewFocusController()
{
}

void ListViewFocusController::viewAtCurrentIndex() const
{
    switch (m_currentSection) {
    case Section::Default: [[fallthrough]];
    case Section::Header: {
        QMetaObject::invokeMethod(m_listView, "positionViewAtBeginning");
        break;
    }
    case Section::Delegate: {
        QMetaObject::invokeMethod(m_listView, "positionViewAtIndex", Q_ARG(int, m_delegateIndex), // Index
                                  Q_ARG(int, 2)); // PositionMode (0 = Visible)
        break;
    }
    case Section::Footer: {
        QMetaObject::invokeMethod(m_listView, "positionViewAtEnd");
        break;
    }
    }
}

int ListViewFocusController::size() const
{
    return m_listView->property("count").toInt();
}

int ListViewFocusController::currentIndex() const
{
    return m_delegateIndex;
}

void ListViewFocusController::setDelegateIndex(int index)
{
    m_delegateIndex = index;
    m_listView->setProperty("currentIndex", index);
}

void ListViewFocusController::nextDelegate()
{
    switch (m_currentSection) {
    case Section::Default: {
        if (hasHeader()) {
            m_currentSection = Section::Header;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    }
    case Section::Header: {
        if (size() > 0) {
            m_currentSection = Section::Delegate;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    }
    case Section::Delegate:
        if (m_delegateIndex < (size() - 1)) {
            setDelegateIndex(m_delegateIndex + 1);
            viewAtCurrentIndex();
            break;
        } else if (hasFooter()) {
            m_currentSection = Section::Footer;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    case Section::Footer: {
        m_isReturnNeeded = true;
        m_currentSection = Section::Default;
        viewAtCurrentIndex();
        break;
    }
    default: {
        qCritical() << "Current section is invalid!";
        break;
    }
    }
}

void ListViewFocusController::previousDelegate()
{
    switch (m_currentSection) {
    case Section::Default: {
        if (hasFooter()) {
            m_currentSection = Section::Footer;
            break;
        }
        [[fallthrough]];
    }
    case Section::Footer: {
        if (size() > 0) {
            m_currentSection = Section::Delegate;
            setDelegateIndex(size() - 1);
            break;
        }
        [[fallthrough]];
    }
    case Section::Delegate: {
        if (m_delegateIndex > 0) {
            setDelegateIndex(m_delegateIndex - 1);
            break;
        } else if (hasHeader()) {
            m_currentSection = Section::Header;
            break;
        }
        [[fallthrough]];
    }
    case Section::Header: {
        m_isReturnNeeded = true;
        m_currentSection = Section::Default;
        break;
    }
    default: {
        qCritical() << "Current section is invalid!";
        break;
    }
    }
}

void ListViewFocusController::decrementIndex()
{
    m_delegateIndex--;
}

QQuickItem *ListViewFocusController::itemAtIndex(const int index) const
{
    QQuickItem *item { nullptr };

    QMetaObject::invokeMethod(m_listView, "itemAtIndex", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index));

    return item;
}

QQuickItem *ListViewFocusController::currentDelegate() const
{
    QQuickItem *result { nullptr };

    switch (m_currentSection) {
    case Section::Default: {
        qWarning() << "No elements...";
        break;
    }
    case Section::Header: {
        result = m_header;
        break;
    }
    case Section::Delegate: {
        result = itemAtIndex(m_delegateIndex);
        break;
    }
    case Section::Footer: {
        result = m_footer;
        break;
    }
    }
    return result;
}

QQuickItem *ListViewFocusController::focusedItem() const
{
    return m_focusedItem;
}

void ListViewFocusController::focusNextItem()
{
    if (m_isReturnNeeded) {
        return;
    }

    reloadFocusChain();

    if (m_focusChain.empty()) {
        qWarning() << "No elements found in the delegate. Going to next delegate...";
        nextDelegate();
        focusNextItem();
        return;
    }
    m_focusedItemIndex++;
    m_focusedItem = qobject_cast<QQuickItem *>(m_focusChain.at(m_focusedItemIndex));
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);
}

void ListViewFocusController::focusPreviousItem()
{
    if (m_isReturnNeeded) {
        return;
    }

    if (m_focusChain.empty()) {
        qInfo() << "Empty focusChain with current delegate: " << currentDelegate() << "Scanning for elements...";
        reloadFocusChain();
    }
    if (m_focusChain.empty()) {
        qWarning() << "No elements found in the delegate. Going to next delegate...";
        previousDelegate();
        focusPreviousItem();
        return;
    }
    if (m_focusedItemIndex == -1) {
        m_focusedItemIndex = m_focusChain.size();
    }
    m_focusedItemIndex--;
    m_focusedItem = qobject_cast<QQuickItem *>(m_focusChain.at(m_focusedItemIndex));
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);
}

void ListViewFocusController::resetFocusChain()
{
    m_focusChain.clear();
    m_focusedItem = nullptr;
    m_focusedItemIndex = -1;
}

void ListViewFocusController::reloadFocusChain()
{
    m_focusChain = FocusControl::getItemsChain(currentDelegate());
}

bool ListViewFocusController::isFirstFocusItemInDelegate() const
{
    return m_focusedItem && (m_focusedItem == m_focusChain.first());
}

bool ListViewFocusController::isLastFocusItemInDelegate() const
{
    return m_focusedItem && (m_focusedItem == m_focusChain.last());
}

bool ListViewFocusController::hasHeader() const
{
    return m_header && !FocusControl::getItemsChain(m_header).isEmpty();
}

bool ListViewFocusController::hasFooter() const
{
    return m_footer && !FocusControl::getItemsChain(m_footer).isEmpty();
}

bool ListViewFocusController::isFirstFocusItemInListView() const
{
    switch (m_currentSection) {
    case Section::Footer: {
        return isFirstFocusItemInDelegate() && !hasHeader() && (size() == 0);
    }
    case Section::Delegate: {
        return isFirstFocusItemInDelegate() && (m_delegateIndex == 0) && !hasHeader();
    }
    case Section::Header: {
        isFirstFocusItemInDelegate();
    }
    case Section::Default: {
        return true;
    }
    default: qWarning() << "Wrong section"; return true;
    }
}

bool ListViewFocusController::isLastFocusItemInListView() const
{
    switch (m_currentSection) {
    case Section::Default: {
        return !hasHeader() && (size() == 0) && !hasFooter();
    }
    case Section::Header: {
        return isLastFocusItemInDelegate() && (size() == 0) && !hasFooter();
    }
    case Section::Delegate: {
        return isLastFocusItemInDelegate() && (m_delegateIndex == size() - 1) && !hasFooter();
    }
    case Section::Footer: {
        return isLastFocusItemInDelegate();
    }
    default: qWarning() << "Wrong section"; return true;
    }
}

bool ListViewFocusController::isReturnNeeded() const
{
    return m_isReturnNeeded;
}

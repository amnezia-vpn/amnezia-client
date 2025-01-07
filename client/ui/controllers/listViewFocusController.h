#ifndef LISTVIEWFOCUSCONTROLLER_H
#define LISTVIEWFOCUSCONTROLLER_H

#include <QList>
#include <QObject>
#include <QQuickItem>
#include <QSharedPointer>
#include <QStack>

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
    Q_OBJECT
public:
    explicit ListViewFocusController(QQuickItem *listView, QObject *parent = nullptr);
    ~ListViewFocusController();

    void nextDelegate();
    void previousDelegate();
    void decrementIndex();
    void focusNextItem();
    void focusPreviousItem();
    void resetFocusChain();
    void reloadFocusChain();
    bool isFirstFocusItemInListView() const;
    bool isFirstFocusItemInDelegate() const;
    bool isLastFocusItemInListView() const;
    bool isLastFocusItemInDelegate() const;
    bool isReturnNeeded() const;

private:
    enum class Section {
        Default,
        Header,
        Delegate,
        Footer,
    };

    int size() const;
    int currentIndex() const;
    void setDelegateIndex(int index);
    void viewAtCurrentIndex() const;
    QQuickItem *itemAtIndex(const int index) const;
    QQuickItem *currentDelegate() const;
    QQuickItem *focusedItem() const;

    bool hasHeader() const;
    bool hasFooter() const;

    QQuickItem *m_listView;
    QList<QObject *> m_focusChain;
    Section m_currentSection;
    QQuickItem *m_header;
    QQuickItem *m_footer;
    QQuickItem *m_focusedItem; // Pointer to focused item on Delegate
    qsizetype m_focusedItemIndex;
    qsizetype m_delegateIndex;
    bool m_isReturnNeeded;

    QList<QString> m_currentSectionString;
};

#endif // LISTVIEWFOCUSCONTROLLER_H

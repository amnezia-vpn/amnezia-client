#ifndef LISTVIEWFOCUSCONTROLLER_H
#define LISTVIEWFOCUSCONTROLLER_H

#include <QObject>
#include <QStack>
#include <QSharedPointer>
#include <QQuickItem>


bool isListView(QObject* item);
bool isMore(QObject* item1, QObject* item2);
bool isLess(QObject* item1, QObject* item2);
QList<QObject*> getSubChain(QObject* object);

void printItems(const QList<QObject*>& items, QObject* current_item);

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
    // Q_OBJECT
public:
    explicit ListViewFocusController(QQuickItem* listView, QObject* parent = nullptr);
    ~ListViewFocusController();

    void nextElement();
    void previousElement();
    void decrementIndex();
    void focusNextItem();
    void focusPreviousItem();
    void resetFocusChain();
    bool isFirstFocusItemInListView();
    bool isFirstFocusItemInDelegate();
    bool isLastFocusItemInListView();
    bool isLastFocusItemInDelegate();
    bool isReturnNeeded();
    void viewToBegin();
    void viewToEnd();
    void viewAtCurrentIndex();

private:
    enum class Section {
        Default,
        Header,
        Delegate,
        Footer,
    };

    int size() const;
    int currentIndex() const;
    QQuickItem* itemAtIndex(const int index);
    QQuickItem* currentDelegate();
    QQuickItem* focusedItem();

    QQuickItem* m_listView;
    QList<QObject*> m_focusChain;
    Section m_currentSection;
    QQuickItem* m_header;
    QQuickItem* m_footer;
    QQuickItem* m_focusedItem; // Pointer to focused item on Delegate
    qsizetype m_focusedItemIndex;
    qsizetype m_delegateIndex;
    bool m_isReturnNeeded;

    QList<QString> m_currentSectionString;
};

#endif // LISTVIEWFOCUSCONTROLLER_H

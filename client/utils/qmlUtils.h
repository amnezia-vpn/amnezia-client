#ifndef FOCUSCONTROL_H
#define FOCUSCONTROL_H

#include <QList>
#include <QObject>

namespace FocusControl
{
    bool isEnabled(QObject *item);
    bool isVisible(QObject *item);
    bool isFocusable(QObject *item);
    bool isListView(QObject *item);
    bool isOnTheScene(QObject *object);
    bool isMore(QObject *item1, QObject *item2);
    bool isLess(QObject *item1, QObject *item2);

    /*!
     * \brief Make focus chain of elements which are on the scene
     */
    QList<QObject *> getSubChain(QObject *object);

    /*!
     * \brief Make focus chain of elements which could be not on the scene
     */
    QList<QObject *> getItemsChain(QObject *object);

    void printItems(const QList<QObject *> &items, QObject *current_item);
} // namespace FocusControl

#endif // FOCUSCONTROL_H

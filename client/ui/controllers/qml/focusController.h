#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QQmlApplicationEngine>

class QQuickItem;

class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine *engine, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    static bool wantsArrowKey(const QQuickItem *item, int key);
    static void sortListViewDelegates(QQuickItem *root);
    static void ensureVisible(QQuickItem *item);
    static void scrollToRange(QQuickItem *flickable, qreal start, qreal end, qreal viewSize, const char *position,
                              const char *origin, const char *contentSize, const char *startMargin,
                              const char *endMargin);
};

#endif // FOCUSCONTROLLER_H

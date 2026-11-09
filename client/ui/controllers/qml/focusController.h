#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QQmlApplicationEngine>

class QKeyEvent;
class QQuickItem;
class QQuickWindow;

class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine *engine, QObject *parent = nullptr);

signals:
    void backRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool handleNavigationKey(QQuickWindow *window, QKeyEvent *keyEvent);
    bool handleBackKey(QQuickItem *contentItem, QKeyEvent *keyEvent);

    static bool wantsArrowKey(const QQuickItem *item, int key);
    static void sortListViewDelegates(QQuickItem *root);
    static void ensureVisible(QQuickItem *item);
    static void scrollToRange(QQuickItem *flickable, qreal start, qreal end, qreal viewSize, const char *position,
                              const char *origin, const char *contentSize, const char *startMargin,
                              const char *endMargin);
};

#endif // FOCUSCONTROLLER_H

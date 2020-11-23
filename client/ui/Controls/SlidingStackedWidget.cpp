#include "SlidingStackedWidget.h"

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent)
    : QStackedWidget(parent)
{
    if (parent != 0) {
        m_mainwindow = parent;
    }
    else {
        m_mainwindow = this;
        qDebug().noquote() << "ATTENTION: untested mainwindow case !";
    }
    // parent should not be 0; not tested for any other case yet !!

#ifdef Q_OS_SYMBIAN
#ifndef __S60_50__
    qDebug().noquote() << "WARNING: ONLY TESTED AND 5TH EDITION";
#endif // __S60_50__
#endif // Q_OS_SYMBIAN

    // Now, initialize some private variables with default values
    m_vertical = false;
    // setVerticalMode(true);
    m_speed = 500;
    m_animationtype = QEasingCurve::OutBack; // check out the QEasingCurve documentation for different styles
    m_now = 0;
    m_next = 0;
    m_wrap = false;
    m_pnow = QPoint(0,0);
    m_active = false;
}

SlidingStackedWidget::~SlidingStackedWidget() {
}

void SlidingStackedWidget::setVerticalMode(bool vertical) {
    m_vertical = vertical;
}

void SlidingStackedWidget::setSpeed(int speed) {
    m_speed = speed;
}

void SlidingStackedWidget::setAnimation(enum QEasingCurve::Type animationtype) {
    m_animationtype = animationtype;
}

void SlidingStackedWidget::setWrap(bool wrap) {
    m_wrap = wrap;
}

void SlidingStackedWidget::slideInNext() {
    int now = currentIndex();
    if (m_wrap || (now < count() - 1))
        // count is inherit from QStackedWidget
        slideInIdx(now + 1);
}

void SlidingStackedWidget::slideInPrev() {
    int now = currentIndex();
    if (m_wrap || (now > 0))
        slideInIdx(now - 1);
}

void SlidingStackedWidget::slideInIdx(int idx, enum t_direction direction) {
    // int idx, t_direction direction=AUTOMATIC
    if (idx > count() - 1) {
        direction = m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
        idx = (idx) % count();
    }
    else if (idx < 0) {
        direction = m_vertical ? BOTTOM2TOP: LEFT2RIGHT;
        idx = (idx + count()) % count();
    }
    slideInWgtImpl(widget(idx), direction);
    // widget() is a function inherited from QStackedWidget
}

void SlidingStackedWidget::slideInWidget(QWidget *widget, SlidingStackedWidget::t_direction direction)
{
    Q_UNUSED(direction);

#ifdef Q_OS_WIN
    int idx = indexOf(widget);
    slideInIdx(idx, direction);
#endif

#ifdef Q_OS_MAC
    setCurrentWidget(widget);
#endif
}

void SlidingStackedWidget::slideInWgtImpl(QWidget * newwidget, enum t_direction direction) {
    if (m_active) {
        return;
    }
    else m_active = true;

    enum t_direction directionhint;
    int now = currentIndex(); // currentIndex() is a function inherited from QStackedWidget
    int next = indexOf(newwidget);
    if (now == next) {
        m_active = false;
        return;
    }
    else if (now < next) {
        directionhint = m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
    }
    else {
        directionhint = m_vertical ? BOTTOM2TOP : LEFT2RIGHT;
    }
    if (direction == AUTOMATIC) {
        direction = directionhint;
    }
    // NOW....
    // calculate the shifts

    int offsetx = frameRect().width(); // inherited from mother
    int offsety = frameRect().height(); // inherited from mother

    // the following is important, to ensure that the new widget
    // has correct geometry information when sliding in first time
    widget(next)->setGeometry(0, 0, offsetx, offsety);

    if (direction == BOTTOM2TOP) {
        offsetx = 0;
        offsety = -offsety;
    }
    else if (direction == TOP2BOTTOM) {
        offsetx = 0;
        // offsety = offsety;
    }
    else if (direction == RIGHT2LEFT) {
        offsetx = -offsetx;
        offsety = 0;
    }
    else if (direction == LEFT2RIGHT) {
        // offsetx = offsetx;
        offsety = 0;
    }
    // re-position the next widget outside/aside of the display area
    QPoint pnext = widget(next)->pos();
    QPoint pnow = widget(now)->pos();
    m_pnow = pnow;

    widget(next)->move(pnext.x() - offsetx, pnext.y() - offsety);
    // make it visible/show
    widget(next)->show();
    widget(next)->raise();

    // animate both, the now and next widget to the side, using animation framework
    QPropertyAnimation *animnow = new QPropertyAnimation(widget(now), "pos");

    animnow->setDuration(m_speed);
    animnow->setEasingCurve(m_animationtype);
    animnow->setStartValue(QPoint(pnow.x(), pnow.y()));
    animnow->setEndValue(QPoint(offsetx + pnow.x(), offsety + pnow.y()));
    QPropertyAnimation *animnext = new QPropertyAnimation(widget(next), "pos");
    animnext->setDuration(m_speed);
    animnext->setEasingCurve(m_animationtype);
    animnext->setStartValue(QPoint(-offsetx + pnext.x(), offsety + pnext.y()));
    animnext->setEndValue(QPoint(pnext.x(), pnext.y()));

    QParallelAnimationGroup *animgroup = new QParallelAnimationGroup;

    animgroup->addAnimation(animnow);
    animgroup->addAnimation(animnext);

    QObject::connect(animgroup, SIGNAL(finished()),this,SLOT(animationDoneSlot()));
    m_next = next;
    m_now = now;
    m_active = true;
    animgroup->start();

    // note; the rest is done via a connect from the animation ready;
    // animation->finished() provides a signal when animation is done;
    // so we connect this to some post processing slot,
    // that we implement here below in animationDoneSlot.
}

void SlidingStackedWidget::animationDoneSlot(void) {
    // when ready, call the QStackedWidget slot setCurrentIndex(int)
    setCurrentIndex(m_next); // this function is inherited from QStackedWidget
    // then hide the outshifted widget now, and (may be done already implicitely by QStackedWidget)
    widget(m_now)->hide();
    // then set the position of the outshifted widget now back to its original
    widget(m_now)->move(m_pnow);
    // so that the application could also still call the QStackedWidget original functions/slots for changings
    // widget(m_now)->update();
    // setCurrentIndex(m_next); // this function is inherit from QStackedWidget
    m_active = false;
    emit animationFinished();
}

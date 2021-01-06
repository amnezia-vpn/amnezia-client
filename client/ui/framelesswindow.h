// This code is a part of Qt-Nice-Frameless-Window
// https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window
// Licensed by MIT License - https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window/blob/master/LICENSE


#ifndef CFRAMELESSWINDOW_H
#define CFRAMELESSWINDOW_H
#include "qsystemdetection.h"
#include <QObject>
#include <QMainWindow>

//A nice frameless window for both Windows and OS X
//Author: Bringer-of-Light
//Github: https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window
// Usage: use "CFramelessWindow" as base class instead of "QMainWindow", and enjoy
#ifdef Q_OS_WIN
#include <QWidget>
#include <QList>
#include <QMargins>
#include <QRect>
class CFramelessWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CFramelessWindow(QWidget *parent = 0);
public:

    //设置是否可以通过鼠标调整窗口大小
    //if resizeable is set to false, then the window can not be resized by mouse
    //but still can be resized programtically
    void setResizeable(bool resizeable=true);
    bool isResizeable(){return m_bResizeable;}

    //设置可调整大小区域的宽度，在此区域内，可以使用鼠标调整窗口大小
    //set border width, inside this aera, window can be resized by mouse
    void setResizeableAreaWidth(int width = 5);
protected:
    //设置一个标题栏widget，此widget会被当做标题栏对待
    //set a widget which will be treat as SYSTEM titlebar
    void setTitleBar(QWidget* titlebar);

    //在标题栏控件内，也可以有子控件如标签控件“label1”，此label1遮盖了标题栏，导致不能通过label1拖动窗口
    //要解决此问题，使用addIgnoreWidget(label1)
    //generally, we can add widget say "label1" on titlebar, and it will cover the titlebar under it
    //as a result, we can not drag and move the MainWindow with this "label1" again
    //we can fix this by add "label1" to a ignorelist, just call addIgnoreWidget(label1)
    void addIgnoreWidget(QWidget* widget);

    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
private slots:
    void onTitleBarDestroyed();
public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;
public slots:
    void showFullScreen();
private:
    QWidget* m_titlebar;
    QList<QWidget*> m_whiteList;
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;
};

#elif defined Q_OS_MAC
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPoint>
class CFramelessWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CFramelessWindow(QWidget *parent = 0);
private:
    void initUI();
public:
    //设置可拖动区域的高度，在此区域内，可以通过鼠标拖动窗口, 0表示整个窗口都可拖动
    //In draggable area, window can be moved by mouse, (height = 0) means that the whole window is draggable
    void setDraggableAreaHeight(int height = 0);

    //只有OS X10.10及以后系统，才支持OS X原生样式包括：三个系统按钮、窗口圆角、窗口阴影
    //类初始化完成后，可以通过此函数查看是否已经启用了原生样式。如果未启动，需要自定义关闭按钮、最小化按钮、最大化按钮
    //Native style（three system button/ round corner/ drop shadow） works only on OS X 10.10 or later
    //after init, we should check whether NativeStyle is OK with this function
    //if NOT ok, we should implement close button/ min button/ max button ourself
    bool isNativeStyleOK() {return m_bNativeSystemBtn;}

    //如果设置setCloseBtnQuit(false)，那么点击关闭按钮后，程序不会退出，而是会隐藏,只有在OS X 10.10 及以后系统中有效
    //if setCloseBtnQuit(false), then when close button is clicked, the application will hide itself instead of quit
    //be carefull, after you set this to false, you can NOT change it to true again
    //this function should be called inside of the constructor function of derived classes, and can NOT be called more than once
    //only works for OS X 10.10 or later
    void setCloseBtnQuit(bool bQuit = true);

    //启用或禁用关闭按钮，只有在isNativeStyleOK()返回true的情况下才有效
    //enable or disable Close button, only worked if isNativeStyleOK() returns true
    void setCloseBtnEnabled(bool bEnable = true);

    //启用或禁用最小化按钮，只有在isNativeStyleOK()返回true的情况下才有效
    //enable or disable Miniaturize button, only worked if isNativeStyleOK() returns true
    void setMinBtnEnabled(bool bEnable = true);

    //启用或禁用zoom（最大化）按钮，只有在isNativeStyleOK()返回true的情况下才有效
    //enable or disable Zoom button(fullscreen button), only worked if isNativeStyleOK() returns true
    void setZoomBtnEnabled(bool bEnable = true);

    bool isCloseBtnEnabled() {return m_bIsCloseBtnEnabled;}
    bool isMinBtnEnabled() {return m_bIsMinBtnEnabled;}
    bool isZoomBtnEnabled() {return m_bIsZoomBtnEnabled;}
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private:
    int m_draggableHeight;
    bool    m_bWinMoving;
    bool    m_bMousePressed;
    QPoint  m_MousePos;
    QPoint  m_WindowPos;
    bool m_bCloseBtnQuit;
    bool m_bNativeSystemBtn;
    bool m_bIsCloseBtnEnabled, m_bIsMinBtnEnabled, m_bIsZoomBtnEnabled;

    //===============================================
    //TODO
    //下面的代码是试验性质的
    //tentative code

    //窗口从全屏状态恢复正常大小时，标题栏又会出现，原因未知。
    //默认情况下，系统的最大化按钮(zoom button)是进入全屏，为了避免标题栏重新出现的问题，
    //以上代码已经重新定义了系统zoom button的行为，是其功能变为最大化而不是全屏
    //以下代码尝试，每次窗口从全屏状态恢复正常大小时，都再次进行设置，以消除标题栏
    //after the window restore from fullscreen mode, the titlebar will show again, it looks like a BUG
    //on OS X 10.10 and later, click the system green button (zoom button) will make the app become fullscreen
    //so we have override it's action to "maximized" in the CFramelessWindow Constructor function
    //but we may try something else such as delete the titlebar again and again...
private:
    bool m_bTitleBarVisible;

    void setTitlebarVisible(bool bTitlebarVisible = false);
    bool isTitlebarVisible() {return m_bTitleBarVisible;}
private slots:
    void onRestoreFromFullScreen();
signals:
    void restoreFromFullScreen();
protected:
    void resizeEvent(QResizeEvent *event);
};
#endif

#endif // CFRAMELESSWINDOW_H

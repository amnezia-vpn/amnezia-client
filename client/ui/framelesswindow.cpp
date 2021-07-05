// This code is a part of Qt-Nice-Frameless-Window
// https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window
// Licensed by MIT License - https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window/blob/master/LICENSE


#include "framelesswindow.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QSysInfo>
#include <QOperatingSystemVersion>

#ifdef Q_OS_WIN

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h> // Fixes error C2504: 'IUnknown' : base class undefined
#include <gdiplus.h>
#include <GdiPlusColor.h>
#pragma comment (lib,"Dwmapi.lib") // Adds missing library, fixes error LNK2019: unresolved external symbol __imp__DwmExtendFrameIntoClientArea
#pragma comment (lib,"user32.lib")

CFramelessWindow::CFramelessWindow(QWidget *parent)
    : QMainWindow(parent),
      m_titlebar(Q_NULLPTR),
      m_borderWidth(5),
      m_bJustMaximized(false),
      m_bResizeable(true)
{
//    setWindowFlag(Qt::Window,true);
//    setWindowFlag(Qt::FramelessWindowHint, true);
//    setWindowFlag(Qt::WindowSystemMenuHint, true);
//    setWindowFlag() is not avaliable before Qt v5.9, so we should use setWindowFlags instead

    if (QOperatingSystemVersion::current() > QOperatingSystemVersion::Windows7) {
        setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
        setResizeable(m_bResizeable);
    }
}

void CFramelessWindow::setResizeable(bool resizeable)
{
    bool visible = isVisible();
    m_bResizeable = resizeable;
    if (m_bResizeable){
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
//        setWindowFlag(Qt::WindowMaximizeButtonHint);

        //此行代码可以带回Aero效果，同时也带回了标题栏和边框,在nativeEvent()会再次去掉标题栏
        //
        //this line will get titlebar/thick frame/Aero back, which is exactly what we want
        //we will get rid of titlebar and thick frame again in nativeEvent() later
        HWND hwnd = (HWND)this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
    }else{
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
//        setWindowFlag(Qt::WindowMaximizeButtonHint,false);

        HWND hwnd = (HWND)this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style & ~WS_MAXIMIZEBOX & ~WS_CAPTION);
    }

    //保留一个像素的边框宽度，否则系统不会绘制边框阴影
    //
    //we better left 1 piexl width of border untouch, so OS can draw nice shadow around it
    const MARGINS shadow = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);

    setVisible(visible);
}

void CFramelessWindow::setResizeableAreaWidth(int width)
{
    if (1 > width) width = 1;
    m_borderWidth = width;
}

void CFramelessWindow::setTitleBar(QWidget* titlebar)
{
    m_titlebar = titlebar;
    if (!titlebar) return;
    connect(titlebar, SIGNAL(destroyed(QObject*)), this, SLOT(onTitleBarDestroyed()));
}

void CFramelessWindow::onTitleBarDestroyed()
{
    if (m_titlebar == QObject::sender())
    {
        m_titlebar = Q_NULLPTR;
    }
}

void CFramelessWindow::addIgnoreWidget(QWidget* widget)
{
    if (!widget) return;
    if (m_whiteList.contains(widget)) return;
    m_whiteList.append(widget);
}

bool CFramelessWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7) {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    //Workaround for known bug -> check Qt forum : https://forum.qt.io/topic/93141/qtablewidget-itemselectionchanged/13
    #if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
    #else
    MSG* msg = reinterpret_cast<MSG*>(message);
    #endif
    
    switch (msg->message)
    {
    case WM_NCCALCSIZE:
    {
        NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
 	if (params.rgrc[0].top != 0)
		params.rgrc[0].top -= 1;

        //this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
        *result = WVR_REDRAW;
        return true;
    }
    case WM_NCHITTEST:
    {
        *result = 0;

        const LONG border_width = m_borderWidth;
        RECT winrect;
        GetWindowRect(HWND(winId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if(m_bResizeable)
        {

            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if(resizeWidth)
            {
                //left border
                if (x >= winrect.left && x < winrect.left + border_width)
                {
                    *result = HTLEFT;
                }
                //right border
                if (x < winrect.right && x >= winrect.right - border_width)
                {
                    *result = HTRIGHT;
                }
            }
            if(resizeHeight)
            {
                //bottom border
                if (y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOM;
                }
                //top border
                if (y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOP;
                }
            }
            if(resizeWidth && resizeHeight)
            {
                //bottom left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMLEFT;
                }
                //bottom right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMRIGHT;
                }
                //top left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                        y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPLEFT;
                }
                //top right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                        y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPRIGHT;
                }
            }
        }
        if (0!=*result) return true;

        //*result still equals 0, that means the cursor locate OUTSIDE the frame area
        //but it may locate in titlebar area
        if (!m_titlebar) return false;

        //support highdpi
        double dpr = this->devicePixelRatioF();
        QPoint pos = m_titlebar->mapFromGlobal(QPoint(x/dpr,y/dpr));

        if (!m_titlebar->rect().contains(pos)) return false;
        QWidget* child = m_titlebar->childAt(pos);
        if (!child)
        {
            *result = HTCAPTION;
            return true;
        }else{
            if (m_whiteList.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
        }
        return false;
    } //end case WM_NCHITTEST
    case WM_GETMINMAXINFO:
    {
        if (::IsZoomed(msg->hwnd)) {
            RECT frame = { 0, 0, 0, 0 };
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

            //record frame area data
            double dpr = this->devicePixelRatioF();

            m_frames.setLeft(abs(frame.left)/dpr+0.5);
            m_frames.setTop(abs(frame.bottom)/dpr+0.5);
            m_frames.setRight(abs(frame.right)/dpr+0.5);
            m_frames.setBottom(abs(frame.bottom)/dpr+0.5);

            QMainWindow::setContentsMargins(m_frames.left()+m_margins.left(), \
                                            m_frames.top()+m_margins.top(), \
                                            m_frames.right()+m_margins.right(), \
                                            m_frames.bottom()+m_margins.bottom());
            m_bJustMaximized = true;
        }else {
            if (m_bJustMaximized)
            {
                QMainWindow::setContentsMargins(m_margins);
                m_frames = QMargins();
                m_bJustMaximized = false;
            }
        }
        return false;
    }
    default:
        return QMainWindow::nativeEvent(eventType, message, result);
    }
}

void CFramelessWindow::setContentsMargins(const QMargins &margins)
{
    QMainWindow::setContentsMargins(margins+m_frames);
    m_margins = margins;
}
void CFramelessWindow::setContentsMargins(int left, int top, int right, int bottom)
{
    QMainWindow::setContentsMargins(left+m_frames.left(),\
                                    top+m_frames.top(), \
                                    right+m_frames.right(), \
                                    bottom+m_frames.bottom());
    m_margins.setLeft(left);
    m_margins.setTop(top);
    m_margins.setRight(right);
    m_margins.setBottom(bottom);
}
QMargins CFramelessWindow::contentsMargins() const
{
    QMargins margins = QMainWindow::contentsMargins();
    margins -= m_frames;
    return margins;
}
void CFramelessWindow::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    QMainWindow::getContentsMargins(left,top,right,bottom);
    if (!(left&&top&&right&&bottom)) return;
    if (isMaximized())
    {
        *left -= m_frames.left();
        *top -= m_frames.top();
        *right -= m_frames.right();
        *bottom -= m_frames.bottom();
    }
}
QRect CFramelessWindow::contentsRect() const
{
    QRect rect = QMainWindow::contentsRect();
    int width = rect.width();
    int height = rect.height();
    rect.setLeft(rect.left() - m_frames.left());
    rect.setTop(rect.top() - m_frames.top());
    rect.setWidth(width);
    rect.setHeight(height);
    return rect;
}
void CFramelessWindow::showFullScreen()
{
    if (isMaximized())
    {
        QMainWindow::setContentsMargins(m_margins);
        m_frames = QMargins();
    }
    QMainWindow::showFullScreen();
}

#endif //Q_OS_WIN

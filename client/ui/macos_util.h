#ifndef OSXUTIL_H
#define OSXUTIL_H

#ifndef Q_OS_IOS
#include <QDialog>
#include <QWidget>

void setDockIconVisible(bool visible);
void fixWidget(QWidget *widget);
#endif

#endif

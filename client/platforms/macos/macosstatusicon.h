/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSSTATUSICON_H
#define MACOSSTATUSICON_H

#include <QByteArray>
#include <QColor>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QString>

class MacOSStatusIcon final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MacOSStatusIcon)

 public:
  explicit MacOSStatusIcon(QObject* parent);
  ~MacOSStatusIcon();

 public:
  void setIcon(const QString& iconUrl);
  void setIconFromData(const QByteArray& imageData);
  void setMenu(QMenu* menu);
  void rebuildNativeMenu();
  void setToolTip(const QString& tooltip);
  void showMessage(const QString& title, const QString& message);

 private:
  QPointer<QMenu> m_qtMenu;
};

#endif  // MACOSSTATUSICON_H

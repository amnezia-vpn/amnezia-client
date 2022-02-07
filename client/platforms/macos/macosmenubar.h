/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSMENUBAR_H
#define MACOSMENUBAR_H

#include <QObject>

class QAction;
class QMenu;
class QMenuBar;

class MacOSMenuBar final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MacOSMenuBar)

 public:
  MacOSMenuBar();
  ~MacOSMenuBar();

  static MacOSMenuBar* instance();

  void initialize();

  void retranslate();

  QMenuBar* menuBar() const { return m_menuBar; }

 public slots:
  void controllerStateChanged();

 private:
  QMenuBar* m_menuBar = nullptr;

  QAction* m_aboutAction = nullptr;
  QAction* m_preferencesAction = nullptr;
  QAction* m_closeAction = nullptr;
  QMenu* m_helpMenu = nullptr;
};

#endif  // MACOSMENUBAR_H

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosmenubar.h"
#include "leakdetector.h"
#include "logger.h"
#include "mozillavpn.h"
#include "qmlengineholder.h"
#ifdef MVPN_MACOS
#  include "platforms/macos/macosutils.h"
#endif

#include <QAction>
#include <QMenu>
#include <QMenuBar>

namespace {
Logger logger(LOG_MACOS, "MacOSManuBar");
MacOSMenuBar* s_instance = nullptr;
}  // namespace

MacOSMenuBar::MacOSMenuBar() {
  MVPN_COUNT_CTOR(MacOSMenuBar);

  Q_ASSERT(!s_instance);
  s_instance = this;
}

MacOSMenuBar::~MacOSMenuBar() {
  MVPN_COUNT_DTOR(MacOSMenuBar);

  Q_ASSERT(s_instance == this);
  s_instance = nullptr;
}

// static
MacOSMenuBar* MacOSMenuBar::instance() {
  Q_ASSERT(s_instance);
  return s_instance;
}

void MacOSMenuBar::initialize() {
  logger.debug() << "Creating menubar";

  AmneziaVPN* vpn = AmneziaVPN::instance();

  m_menuBar = new QMenuBar(nullptr);

  //% "File"
  QMenu* fileMenu = m_menuBar->addMenu(qtTrId("menubar.file.title"));

  // Do not use qtTrId here!
  QAction* quit =
      fileMenu->addAction("quit", vpn->controller(), &Controller::quit);
  quit->setMenuRole(QAction::QuitRole);

  // Do not use qtTrId here!
  m_aboutAction =
      fileMenu->addAction("about.vpn", vpn, &AmneziaVPN::requestAbout);
  m_aboutAction->setMenuRole(QAction::AboutRole);
  m_aboutAction->setVisible(vpn->state() == AmneziaVPN::StateMain);

  // Do not use qtTrId here!
  m_preferencesAction =
      fileMenu->addAction("preferences", vpn, &AmneziaVPN::requestSettings);
  m_preferencesAction->setMenuRole(QAction::PreferencesRole);
  m_preferencesAction->setVisible(vpn->state() == AmneziaVPN::StateMain);

  m_closeAction = fileMenu->addAction("close", []() {
    QmlEngineHolder::instance()->hideWindow();
#ifdef MVPN_MACOS
    MacOSUtils::hideDockIcon();
#endif
  });
  m_closeAction->setShortcut(QKeySequence::Close);

  m_helpMenu = m_menuBar->addMenu("");

  retranslate();
};

void MacOSMenuBar::controllerStateChanged() {
  AmneziaVPN* vpn = AmneziaVPN::instance();
  m_preferencesAction->setVisible(vpn->state() == AmneziaVPN::StateMain);
  m_aboutAction->setVisible(vpn->state() == AmneziaVPN::StateMain);
}

void MacOSMenuBar::retranslate() {
  logger.debug() << "Retranslate";

  //% "Close"
  m_closeAction->setText(qtTrId("menubar.file.close"));

  //% "Help"
  m_helpMenu->setTitle(qtTrId("menubar.help.title"));
  for (QAction* action : m_helpMenu->actions()) {
    m_helpMenu->removeAction(action);
  }

  AmneziaVPN* vpn = AmneziaVPN::instance();
  vpn->helpModel()->forEach([&](const char* nameId, int id) {
    m_helpMenu->addAction(qtTrId(nameId),
                          [help = vpn->helpModel(), id]() { help->open(id); });
  });
}

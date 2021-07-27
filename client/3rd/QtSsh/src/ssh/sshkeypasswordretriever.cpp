/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#include "sshkeypasswordretriever_p.h"

#include <QString>
#include <QApplication>
#include <QInputDialog>

#include <iostream>

namespace QSsh {
namespace Internal {

std::string SshKeyPasswordRetriever::get_passphrase()
{
    const bool hasGui = dynamic_cast<QApplication *>(QApplication::instance());
    if (hasGui) {
        bool ok;
        const QString &password = QInputDialog::getText(nullptr,
            QCoreApplication::translate("QSsh::Ssh", "Password Required"),
            QCoreApplication::translate("QSsh::Ssh", "Please enter the password for your private key."),
            QLineEdit::Password, QString(), &ok);
        return std::string(password.toLocal8Bit().data());
    } else {
        std::string password;
        std::cout << "Please enter the password for your private key (set echo off beforehand!): " << std::flush;
        std::cin >> password;
        return password;
    }
}

} // namespace Internal
} // namespace QSsh

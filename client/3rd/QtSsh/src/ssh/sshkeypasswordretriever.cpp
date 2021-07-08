/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sshkeypasswordretriever_p.h"

#include <QString>
#include <QApplication>
#include <QInputDialog>

#include <iostream>

namespace QSsh {
namespace Internal {

std::string SshKeyPasswordRetriever::get_passphrase(const std::string &, const std::string &,
    UI_Result &result) const
{
    const bool hasGui = dynamic_cast<QApplication *>(QApplication::instance());
    if (hasGui) {
        bool ok;
        const QString &password = QInputDialog::getText(0,
            QCoreApplication::translate("QSsh::Ssh", "Password Required"),
            QCoreApplication::translate("QSsh::Ssh", "Please enter the password for your private key."),
            QLineEdit::Password, QString(), &ok);
        result = ok ? OK : CANCEL_ACTION;
        return std::string(password.toLocal8Bit().data());
    } else {
        result = OK;
        std::string password;
        std::cout << "Please enter the password for your private key (set echo off beforehand!): " << std::flush;
        std::cin >> password;
        return password;
    }
}

} // namespace Internal
} // namespace QSsh

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

#include "sshhostkeydatabase.h"

#include "sshlogging_p.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QString>

namespace QSsh {

class SshHostKeyDatabase::SshHostKeyDatabasePrivate
{
public:
    QHash<QString, QByteArray> hostKeys;
};

SshHostKeyDatabase::SshHostKeyDatabase() : d(new SshHostKeyDatabasePrivate)
{
}

SshHostKeyDatabase::~SshHostKeyDatabase()
{
    delete d;
}

bool SshHostKeyDatabase::load(const QString &filePath, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QCoreApplication::translate("QSsh::Ssh",
                                                 "Failed to open key file \"%1\" for reading: %2")
                    .arg(QDir::toNativeSeparators(filePath), file.errorString());
        }
        return false;
    }

    d->hostKeys.clear();
    const QByteArray content = file.readAll().trimmed();
    if (content.isEmpty())
        return true;
    foreach (const QByteArray &line, content.split('\n')) {
        const QList<QByteArray> &lineData = line.trimmed().split(' ');
        if (lineData.count() != 2) {
            qCDebug(Internal::sshLog, "Unexpected line \"%s\" in file \"%s\".", line.constData(),
                   qPrintable(filePath));
            continue;
        }
        d->hostKeys.insert(QString::fromUtf8(lineData.first()),
                           QByteArray::fromHex(lineData.last()));
    }

    return true;
}

bool SshHostKeyDatabase::store(const QString &filePath, QString *error) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QCoreApplication::translate("QSsh::Ssh",
                                                 "Failed to open key file \"%1\" for writing: %2")
                    .arg(QDir::toNativeSeparators(filePath), file.errorString());
        }
        return false;
    }

    file.resize(0);
    for (auto it = d->hostKeys.constBegin(); it != d->hostKeys.constEnd(); ++it)
        file.write(it.key().toUtf8() + ' ' + it.value().toHex() + '\n');
    return true;
}

SshHostKeyDatabase::KeyLookupResult SshHostKeyDatabase::matchHostKey(const QString &hostName,
                                                                     const QByteArray &key) const
{
    auto it = d->hostKeys.constFind(hostName);
    if (it == d->hostKeys.constEnd())
        return KeyLookupNoMatch;
    if (it.value() == key)
        return KeyLookupMatch;
    return KeyLookupMismatch;
}

void SshHostKeyDatabase::insertHostKey(const QString &hostName, const QByteArray &key)
{
    d->hostKeys.insert(hostName, key);
}

} // namespace QSsh

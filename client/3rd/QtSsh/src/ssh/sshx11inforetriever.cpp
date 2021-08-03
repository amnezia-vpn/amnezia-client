/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sshx11inforetriever_p.h"

#include "sshlogging_p.h"
#include "sshx11displayinfo_p.h"

#include <QByteArrayList>
#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>

#include <botan/auto_rng.h>

namespace QSsh {
namespace Internal {

static QByteArray xauthProtocol() { return "MIT-MAGIC-COOKIE-1"; }

SshX11InfoRetriever::SshX11InfoRetriever(const QString &displayName, QObject *parent)
    : QObject(parent),
      m_displayName(displayName),
      m_xauthProc(new QProcess(this)),
      m_xauthFile(new QTemporaryFile(this))
{
    connect(m_xauthProc, &QProcess::errorOccurred, this,
        [this] {
            if (m_xauthProc->error() == QProcess::FailedToStart) {
                emitFailure(tr("Could not start xauth: %1").arg(m_xauthProc->errorString()));
            }
        }
    );
    connect(m_xauthProc, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
        [this] {
            if (m_xauthProc->exitStatus() != QProcess::NormalExit) {
                emitFailure(tr("xauth crashed: %1").arg(m_xauthProc->errorString()));
                return;
            }
            if (m_xauthProc->exitCode() != 0) {
                emitFailure(tr("xauth failed with exit code %1.").arg(m_xauthProc->exitCode()));
                return;
            }
            switch (m_state) {
            case State::RunningGenerate:
                m_state = State::RunningList;
                m_xauthProc->start(QStringLiteral("xauth"), QStringList{
                                       QStringLiteral("-f"),
                                       m_xauthFile->fileName(),
                                       QStringLiteral("list"),
                                       m_displayName});
                break;
            case State::RunningList: {
                const QByteArrayList outputLines = m_xauthProc->readAllStandardOutput().split('\n');
                if (outputLines.empty()) {
                    emitFailure(tr("Unexpected xauth output."));
                    return;
                }
                const QByteArrayList data = outputLines.first().simplified().split(' ');
                if (data.size() < 3 || data.at(1) != xauthProtocol() || data.at(2).isEmpty()) {
                    emitFailure(tr("Unexpected xauth output."));
                    return;
                }
                X11DisplayInfo displayInfo;
                displayInfo.displayName = m_displayName;
                const int colonIndex = m_displayName.indexOf(QLatin1Char(':'));
                if (colonIndex == -1) {
                    emitFailure(tr("Invalid display name \"%1\"").arg(m_displayName));
                    return;
                }
                displayInfo.hostName = m_displayName.mid(0, colonIndex);
                const int dotIndex = m_displayName.indexOf(QLatin1Char('.'), colonIndex + 1);
                const QString display = m_displayName.mid(colonIndex + 1,
                                                          dotIndex == -1 ? -1
                                                                         : dotIndex - colonIndex - 1);
                if (display.isEmpty()) {
                    emitFailure(tr("Invalid display name \"%1\"").arg(m_displayName));
                    return;
                }
                bool ok;
                displayInfo.display = display.toInt(&ok);
                if (!ok) {
                    emitFailure(tr("Invalid display name \"%1\"").arg(m_displayName));
                    return;
                }
                if (dotIndex != -1) {
                    displayInfo.screen = m_displayName.midRef(dotIndex + 1).toInt(&ok);
                    if (!ok) {
                        emitFailure(tr("Invalid display name \"%1\"").arg(m_displayName));
                        return;
                    }
                }
                displayInfo.protocol = data.at(1);
                displayInfo.cookie = QByteArray::fromHex(data.at(2));
                displayInfo.randomCookie.resize(displayInfo.cookie.size());
                try {
                    Botan::AutoSeeded_RNG rng;
                    rng.randomize(reinterpret_cast<Botan::uint8_t *>(displayInfo.randomCookie.data()),
                                  displayInfo.randomCookie.size());
                } catch (const std::exception &ex) {
                    emitFailure(tr("Failed to generate random cookie: %1")
                                .arg(QLatin1String(ex.what())));
                    return;
                }
                emit success(displayInfo);
                deleteLater();
                break;
            }
            default:
                emitFailure(tr("Internal error"));
            }
        }
    );
}

void SshX11InfoRetriever::start()
{
    if (!m_xauthFile->open()) {
        emitFailure(tr("Could not create temporary file: %1").arg(m_xauthFile->errorString()));
        return;
    }
    m_state = State::RunningGenerate;
    m_xauthProc->start(QStringLiteral("xauth"), QStringList{
                           QStringLiteral("-f"),
                           m_xauthFile->fileName(),
                           QStringLiteral("generate"),
                           m_displayName, QString::fromLatin1(xauthProtocol())});
}

void SshX11InfoRetriever::emitFailure(const QString &reason)
{
    QTimer::singleShot(0, this, [this, reason] {
        emit failure(tr("Could not retrieve X11 authentication cookie: %1").arg(reason));
        deleteLater();
    });
}

} // namespace Internal
} // namespace QSsh

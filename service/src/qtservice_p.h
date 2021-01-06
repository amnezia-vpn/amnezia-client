/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTSERVICE_P_H
#define QTSERVICE_P_H

#include <QStringList>
#include "qtservice.h"

class QtServiceControllerPrivate
{
    Q_DECLARE_PUBLIC(QtServiceController)
public:
    QString serviceName;
    QtServiceController *q_ptr;
};

class QtServiceBasePrivate
{
    Q_DECLARE_PUBLIC(QtServiceBase)
public:

    QtServiceBasePrivate(const QString &name);
    ~QtServiceBasePrivate();

    QtServiceBase *q_ptr;

    QString serviceDescription;
    QtServiceController::StartupType startupType;
    QtServiceBase::ServiceFlags serviceFlags;
    QStringList args;

    static class QtServiceBase *instance;

    QtServiceController controller;

    void startService();
    int run(bool asService, const QStringList &argList);
    bool install(const QString &account, const QString &password);

    bool start();

    QString filePath() const;
    bool sysInit();
    void sysSetPath();
    void sysCleanup();
    class QtServiceSysPrivate *sysd;
};

#endif

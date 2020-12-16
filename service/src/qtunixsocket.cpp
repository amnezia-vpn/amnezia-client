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

#include "qtunixsocket.h"
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#ifndef SUN_LEN
#define SUN_LEN(ptr) ((size_t)(((struct sockaddr_un *) 0)->sun_path) \
                      +strlen ((ptr)->sun_path))
#endif

QtUnixSocket::QtUnixSocket(QObject *parent)
    : QTcpSocket(parent)
{
}

bool QtUnixSocket::connectTo(const QString &path)
{
    bool ret = false;
    int sock = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock != -1) {
        struct sockaddr_un addr;
	::memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	size_t pathlen = strlen(path.toLatin1().constData());
        pathlen = qMin(pathlen, sizeof(addr.sun_path));
	::memcpy(addr.sun_path, path.toLatin1().constData(), pathlen);
	int err = ::connect(sock, (struct sockaddr *)&addr, SUN_LEN(&addr));
        if (err != -1) {
            setSocketDescriptor(sock);
	    ret = true;
	} else {
            ::close(sock);
        }
    }
    return ret;
}

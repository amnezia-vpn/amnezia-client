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

#ifndef SSH_GLOBAL_H
#define SSH_GLOBAL_H

#include <QtGlobal>

#ifdef _MSC_VER
// For static cmake building removing dll export/import
#  define QSSH_EXPORT
#else

#if defined(QTCSSH_LIBRARY)
#  define QSSH_EXPORT Q_DECL_EXPORT
#else
#  define QSSH_EXPORT Q_DECL_IMPORT
#endif

#endif

#define QSSH_PRINT_WARNING qWarning("Soft assert at %s:%d", __FILE__, __LINE__)
#define QSSH_ASSERT(cond) do { if (!(cond)) { QSSH_PRINT_WARNING; } } while (false)
#define QSSH_ASSERT_AND_RETURN(cond) do { if (!(cond)) { QSSH_PRINT_WARNING; return; } } while (false)
#define QSSH_ASSERT_AND_RETURN_VALUE(cond, value) do { if (!(cond)) { QSSH_PRINT_WARNING; return value; } } while (false)

#endif // SSH_GLOBAL_H

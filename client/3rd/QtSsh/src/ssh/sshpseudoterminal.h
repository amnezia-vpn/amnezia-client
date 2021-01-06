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

#pragma once

#include "ssh_global.h"

#include <QByteArray>
#include <QHash>

namespace QSsh {

class QSSH_EXPORT SshPseudoTerminal
{
  public:
    explicit SshPseudoTerminal(const QByteArray &termType = "vt100",
            int rowCount = 24, int columnCount = 80)
        : termType(termType), rowCount(rowCount), columnCount(columnCount) {}

    QByteArray termType;
    int rowCount;
    int columnCount;

    enum Mode {
        VINTR = 1,      // Interrupt character.
        VQUIT = 2,      // The quit character (sends SIGQUIT signal on POSIX systems).
        VERASE = 3,     // Erase the character to left of the cursor.
        VKILL = 4,      // Kill the current input line.
        VEOF = 5,       // End-of-file character (sends EOF from the terminal).
        VEOL = 6,       // End-of-line character in addition to carriage return and/or linefeed.
        VEOL2 = 7,      // Additional end-of-line character.
        VSTART = 8,     // Continues paused output (normally control-Q).
        VSTOP = 9,      // Pauses output (normally control-S).
        VSUSP = 10,     // Suspends the current program.
        VDSUSP = 11,    // Another suspend character.
        VREPRINT = 12,  // Reprints the current input line.
        VWERASE = 13,   // Erases a word left of cursor.
        VLNEXT = 14,    // Enter the next character typed literally, even if it is a special character.
        VFLUSH = 15,    // Character to flush output.
        VSWTCH = 16,    // Switch to a different shell layer.
        VSTATUS = 17,   // Prints system status line (load, command, pid, etc).
        VDISCARD = 18,  // Toggles the flushing of terminal output.

        IGNPAR = 30,    // The ignore parity flag.  The parameter SHOULD be 0 if this flag is FALSE, and 1 if it is TRUE.
        PARMRK = 31,    // Mark parity and framing errors.
        INPCK = 32,     // Enable checking of parity errors.
        ISTRIP = 33,    // Strip 8th bit off characters.
        INLCR = 34,     // Map NL into CR on input.
        IGNCR = 35,     // Ignore CR on input.
        ICRNL = 36,     // Map CR to NL on input.
        IUCLC = 37,     // Translate uppercase characters to lowercase.
        IXON = 38,      // Enable output flow control.
        IXANY = 39,     // Any char will restart after stop.
        IXOFF = 40,     // Enable input flow control.
        IMAXBEL = 41,   // Ring bell on input queue full.
        ISIG = 50,      // Enable signals INTR, QUIT, [D]SUSP.
        ICANON = 51,    // Canonicalize input lines.
        XCASE = 52,     // Enable input and output of uppercase characters by preceding their lowercase equivalents with "\".
        ECHO = 53,      // Enable echoing.
        ECHOE = 54,     // Visually erase chars.
        ECHOK = 55,     // Kill character discards current line.
        ECHONL = 56,    // Echo NL even if ECHO is off.
        NOFLSH = 57,    // Don't flush after interrupt.
        TOSTOP = 58,    // Stop background jobs from output.
        IEXTEN = 59,    // Enable extensions.
        ECHOCTL = 60,   // Echo control characters as ^(Char).
        ECHOKE = 61,    // Visual erase for line kill.
        PENDIN = 62,    // Retype pending input.
        OPOST = 70,     // Enable output processing.
        OLCUC = 71,     // Convert lowercase to uppercase.
        ONLCR = 72,     // Map NL to CR-NL.
        OCRNL = 73,     // Translate carriage return to newline (output).
        ONOCR = 74,     // Translate newline to carriage return-newline (output).
        ONLRET = 75,    // Newline performs a carriage return (output).
        CS7 = 90,       // 7 bit mode.
        CS8 = 91,       // 8 bit mode.
        PARENB = 92,    // Parity enable.
        PARODD = 93,    // Odd parity, else even.

        TTY_OP_ISPEED = 128,  // Specifies the input baud rate in bits per second.
        TTY_OP_OSPEED = 129   // Specifies the output baud rate in bits per second.
    };

    typedef QHash<Mode, quint32> ModeMap;
    ModeMap modes;
};

} // namespace QSsh

#include "mtproxy_public_host_input.h"

#include <QRegularExpression>

namespace {

    bool ipv4OctetTokenOk(const QString &s) {
        static const QRegularExpression re(QStringLiteral(R"(^\d{1,3}$)"));
        if (!re.match(s).hasMatch()) {
            return false;
        }
        bool ok = false;
        const int n = s.toInt(&ok);
        return ok && n >= 0 && n <= 255;
    }

// Reject labels like "312edweqwe" (digits >255 then letters).
    bool labelHasInvalidOctetLikePrefixBeforeLetters(const QString &label) {
        static const QRegularExpression re(QStringLiteral(R"(^(\d+)([a-zA-Z].*)$)"));
        const QRegularExpressionMatch m = re.match(label);
        if (!m.hasMatch()) {
            return false;
        }
        const QString digits = m.captured(1);
        if (digits.length() > 3) {
            return true;
        }
        bool ok = false;
        const int n = digits.toInt(&ok);
        if (!ok) {
            return true;
        }
        if (n > 255) {
            return true;
        }
        // Do not restrict n≤255 + letters here (e.g. "123mlkjh.example.com"); four-segment IPv4+junk is handled below.
        return false;
    }

// "123.123wqqweqweqweqwe" — first label is a real octet, second looks like an octet glued to letters (not "123.45").
    bool looksLikeTwoSegmentOctetThenDigitLetterGlue(const QString &text) {
        const QStringList parts = text.split(QLatin1Char('.'), Qt::KeepEmptyParts);
        if (parts.size() != 2) {
            return false;
        }
        if (!ipv4OctetTokenOk(parts.at(0))) {
            return false;
        }
        const QString &p1 = parts.at(1);
        static const QRegularExpression digitThenLetter(QStringLiteral(R"(^\d+[a-zA-Z])"));
        if (!digitThenLetter.match(p1).hasMatch()) {
            return false;
        }
        return !ipv4OctetTokenOk(p1);
    }

// "a.b.c.djunk" where first three parts are pure octets and last part has digits then letters (e.g. "123wdqweqweqwe").
    bool looksLikeFourOctetIpv4WithGarbageInLastSegment(const QString &text) {
        const QStringList parts = text.split(QLatin1Char('.'), Qt::KeepEmptyParts);
        if (parts.size() != 4) {
            return false;
        }
        for (int i = 0; i < 3; ++i) {
            if (!ipv4OctetTokenOk(parts.at(i))) {
                return false;
            }
        }
        static const QRegularExpression digitThenLetter(QStringLiteral(R"(^\d+[a-zA-Z])"));
        return digitThenLetter.match(parts.at(3)).hasMatch();
    }

    bool hostLabelsRejectBrokenDigitLetterMix(const QString &text) {
        if (looksLikeTwoSegmentOctetThenDigitLetterGlue(text)) {
            return false;
        }
        if (looksLikeFourOctetIpv4WithGarbageInLastSegment(text)) {
            return false;
        }
        const QStringList parts = text.split(QLatin1Char('.'), Qt::KeepEmptyParts);
        for (const QString &part: parts) {
            if (labelHasInvalidOctetLikePrefixBeforeLetters(part)) {
                return false;
            }
        }
        return true;
    }

} // namespace

bool mtproxyPublicHostInputAllowed(const QString &text) {
    if (text.length() > 253) {
        return false;
    }
    static const QRegularExpression allowed(QStringLiteral(R"(^[a-zA-Z0-9.:\-]*$)"));
    if (!allowed.match(text).hasMatch()) {
        return false;
    }
    static const QRegularExpression onlyDigits(QStringLiteral(R"(^\d+$)"));
    if (onlyDigits.match(text).hasMatch() && text.length() > 3) {
        return false;
    }
    static const QRegularExpression onlyDigitDot(QStringLiteral(R"(^[0-9.]+$)"));
    if (!text.isEmpty() && onlyDigitDot.match(text).hasMatch()) {
        static const QRegularExpression ipv4Partial(QStringLiteral(R"(^(\d{1,3}\.){0,3}\d{0,3}$)"));
        return ipv4Partial.match(text).hasMatch();
    }
    if (text.contains(QLatin1Char(':'))) {
        static const QRegularExpression ipv6Chars(QStringLiteral(R"(^[0-9a-fA-F:.]*$)"));
        if (!ipv6Chars.match(text).hasMatch()) {
            return false;
        }
        if (text.size() > 45) {
            return false;
        }
    }
    if (!hostLabelsRejectBrokenDigitLetterMix(text)) {
        return false;
    }
    return true;
}

PublicHostInputValidator::PublicHostInputValidator(QObject *parent) : QValidator(parent) {}

QValidator::State PublicHostInputValidator::validate(QString &input, int &pos) const {
    Q_UNUSED(pos)
    return mtproxyPublicHostInputAllowed(input) ? Acceptable : Invalid;
}

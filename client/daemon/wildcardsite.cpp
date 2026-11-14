/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wildcardsite.h"

#include <QUrl>

namespace {

constexpr int DNS_MAX_LABEL_LENGTH = 63;
constexpr int DNS_MAX_NAME_LENGTH = 253;

QString stripRoot(const QString &name)
{
    QString stripped = name;
    while (stripped.endsWith(QLatin1Char('.'))) {
        stripped.chop(1);
    }
    return stripped;
}

bool hasUnusableCharacter(const QString &base)
{
    for (const QChar &c : base) {
        if (c == QLatin1Char('/') || c == QLatin1Char('\\') || c == QLatin1Char('?')
            || c == QLatin1Char('#') || c == QLatin1Char('@') || c == QLatin1Char('*')
            || c == QLatin1Char(':') || c.isSpace()) {
            return true;
        }
    }
    return false;
}

bool labelsAreUsable(const QStringList &labels)
{
    if (labels.isEmpty()) {
        return false;
    }
    int total = 0;
    for (const QString &label : labels) {
        if (label.isEmpty() || label.length() > DNS_MAX_LABEL_LENGTH) {
            return false;
        }
        total += label.length() + 1;
    }
    return total - 1 <= DNS_MAX_NAME_LENGTH;
}

}

bool WildcardSite::isPattern(const QString &entry)
{
    return entry.startsWith(QLatin1String("*."));
}

QString WildcardSite::canonicalize(const QString &entry)
{
    if (!isPattern(entry)) {
        return QString();
    }

    const QString base = stripRoot(entry.mid(2).trimmed());
    if (base.isEmpty() || hasUnusableCharacter(base)) {
        return QString();
    }
    if (!labelsAreUsable(base.split(QLatin1Char('.')))) {
        return QString();
    }

    const QByteArray ace = QUrl::toAce(base);
    if (ace.isEmpty()) {
        return QString();
    }

    const QString canonicalBase = QString::fromLatin1(ace).toLower();
    if (!labelsAreUsable(canonicalBase.split(QLatin1Char('.')))) {
        return QString();
    }

    return QStringLiteral("*.") + canonicalBase;
}

bool WildcardSite::isValid(const QString &entry)
{
    return !canonicalize(entry).isEmpty();
}

QStringList WildcardSite::toLabels(const QString &name)
{
    const QString stripped = stripRoot(name).toLower();
    if (stripped.isEmpty()) {
        return QStringList();
    }
    const QStringList labels = stripped.split(QLatin1Char('.'));
    if (!labelsAreUsable(labels)) {
        return QStringList();
    }
    return labels;
}

QByteArray WildcardSite::foldLabel(const QByteArray &label)
{
    QByteArray folded = label;
    for (int i = 0; i < folded.size(); ++i) {
        const char c = folded.at(i);
        if (c >= 'A' && c <= 'Z') {
            folded[i] = static_cast<char>(c - 'A' + 'a');
        }
    }
    return folded;
}

QList<QByteArray> WildcardSite::patternLabels(const QString &canonicalPattern)
{
    QList<QByteArray> labels;
    if (!isPattern(canonicalPattern)) {
        return labels;
    }
    const QStringList parts = canonicalPattern.mid(2).split(QLatin1Char('.'));
    for (const QString &part : parts) {
        if (part.isEmpty()) {
            return QList<QByteArray>();
        }
        labels.append(foldLabel(part.toLatin1()));
    }
    return labels;
}

bool WildcardSite::matchesLabels(const QList<QByteArray> &nameLabels,
                                 const QString &canonicalPattern)
{
    const QList<QByteArray> baseLabels = patternLabels(canonicalPattern);
    if (nameLabels.isEmpty() || baseLabels.isEmpty()) {
        return false;
    }
    if (nameLabels.size() < baseLabels.size()) {
        return false;
    }

    for (int i = 1; i <= baseLabels.size(); ++i) {
        if (foldLabel(nameLabels.at(nameLabels.size() - i)) != baseLabels.at(baseLabels.size() - i)) {
            return false;
        }
    }
    return true;
}

bool WildcardSite::matches(const QString &name, const QString &canonicalPattern)
{
    if (!isPattern(canonicalPattern)) {
        return false;
    }

    const QStringList nameLabels = toLabels(name);
    const QStringList baseLabels = toLabels(canonicalPattern.mid(2));
    if (nameLabels.isEmpty() || baseLabels.isEmpty()) {
        return false;
    }
    if (nameLabels.size() < baseLabels.size()) {
        return false;
    }

    for (int i = 1; i <= baseLabels.size(); ++i) {
        if (nameLabels.at(nameLabels.size() - i) != baseLabels.at(baseLabels.size() - i)) {
            return false;
        }
    }
    return true;
}

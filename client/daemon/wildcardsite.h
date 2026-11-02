/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WILDCARDSITE_H
#define WILDCARDSITE_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace WildcardSite {

bool isPattern(const QString &entry);

QString canonicalize(const QString &entry);

bool isValid(const QString &entry);

QStringList toLabels(const QString &name);

bool matches(const QString &name, const QString &canonicalPattern);

bool matchesLabels(const QList<QByteArray> &nameLabels,
                   const QString &canonicalPattern);

QList<QByteArray> patternLabels(const QString &canonicalPattern);

QByteArray foldLabel(const QByteArray &label);

}

#endif  // WILDCARDSITE_H

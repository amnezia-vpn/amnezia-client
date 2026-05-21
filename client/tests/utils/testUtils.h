#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <optional>

#include <QString>

class SecureServersRepository;

namespace amnezia::test
{

QString getValueFromIni(const QString &key, const QString &iniFileName = QStringLiteral("test_vars.ini"));

bool isIniValueConfigured(const QString &value);

std::optional<QString> serverDescription(SecureServersRepository *repo, const QString &serverId);
std::optional<QString> serverDescriptionAt(SecureServersRepository *repo, int index);

} // namespace amnezia::test

#endif // TESTUTILS_H

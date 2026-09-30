#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <optional>

#include <QString>

class SecureServersRepository;

namespace amnezia::test
{

QString getEnvValue(const QString &key);

bool isEnvValueConfigured(const QString &value);

// Logs the env var state and value. Secret values coming from GitHub Actions
// secrets are masked as *** in CI logs automatically.
void logEnvValueState(const QString &key);

std::optional<QString> serverDescription(SecureServersRepository *repo, const QString &serverId);
std::optional<QString> serverDescriptionAt(SecureServersRepository *repo, int index);

} // namespace amnezia::test

#endif // TESTUTILS_H

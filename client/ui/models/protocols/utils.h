#pragma once

#include <QJsonObject>

template<typename T>
inline void updateConfig(const QJsonObject &inConfig, QJsonObject &outConfig, const char *config_key, T default_value)
{
    if (!inConfig.contains(config_key))
        return;

    const auto value = inConfig.value(config_key);

    if constexpr (std::is_same_v<T, const char *>) {
        outConfig[config_key] = value.toString(default_value);
    } else if constexpr (std::is_same_v<T, bool>) {
        outConfig[config_key] = value.toBool(default_value);
    } else {
        static_assert(std::is_same_v<T, void>, "updateConfig: unsupported default-value type");
    }
}

#define updateConfig(NAME, key, default_value) updateConfig(NAME, m_##NAME, key, default_value);
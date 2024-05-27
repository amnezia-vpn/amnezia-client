#pragma once

#include <QMetaEnum>

namespace utils
{
  template<typename Enum>
  QString enumToString(Enum value)
  {
    auto metaEnum = QMetaEnum::fromType<Enum>();
    return metaEnum.valueToKey(static_cast<int>(value));
  }

  template<typename Enum>
  Enum enumFromString(const QString &str, Enum defaultValue = {})
  {
    auto metaEnum = QMetaEnum::fromType<Enum>();
    bool isOk;
    auto value = metaEnum.keyToValue(str.toLatin1(), &isOk);
    if (isOk) {
      return static_cast<Enum>(value);
    }
    return defaultValue;
  }
}

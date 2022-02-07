/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BIGINTIPV6ADDR_H
#define BIGINTIPV6ADDR_H

#include "bigint.h"

#include <QHostAddress>

class BigIntIPv6Addr final {
 public:
  BigIntIPv6Addr() : m_value(16) {}

  explicit BigIntIPv6Addr(const Q_IPV6ADDR& a) : m_value(16) {
    for (int i = 0; i < 16; ++i) m_value.setValueAt(a[i], i);
  }

  BigIntIPv6Addr(const BigIntIPv6Addr& other) : m_value(16) { *this = other; }

  Q_IPV6ADDR value() const {
    Q_IPV6ADDR addr;
    for (int i = 0; i < 16; ++i) addr[i] = m_value.valueAt(i);
    return addr;
  }

  // Assign operator.

  BigIntIPv6Addr& operator=(const BigIntIPv6Addr& other) {
    m_value = other.m_value;
    return *this;
  }

  // Comparison operators.

  bool operator==(const BigIntIPv6Addr& other) const {
    return m_value == other.m_value;
  }

  bool operator!=(const BigIntIPv6Addr& other) const {
    return m_value != other.m_value;
  }

  bool operator<(const BigIntIPv6Addr& other) const {
    return m_value < other.m_value;
  }

  bool operator>(const BigIntIPv6Addr& other) const {
    return m_value > other.m_value;
  }

  bool operator<=(const BigIntIPv6Addr& other) const {
    return m_value <= other.m_value;
  }

  bool operator>=(const BigIntIPv6Addr& other) const {
    return m_value >= other.m_value;
  }

  // math operators (only some of them are implemented)

  BigIntIPv6Addr& operator++() {
    ++m_value;
    return *this;
  }

  BigIntIPv6Addr& operator+=(const BigIntIPv6Addr& b) {
    m_value += b.m_value;
    return *this;
  }

  // Shift operators

  BigIntIPv6Addr operator>>(int shift) {
    BigIntIPv6Addr x;

    x.m_value = m_value >> shift;
    return x;
  }

 private:
  BigInt m_value;
};

#endif  // BIGINTIPV6ADDR_H

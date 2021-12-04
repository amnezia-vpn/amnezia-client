/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BIGINT_H
#define BIGINT_H

#include <QVector>

// This BigInt implementation is meant to be used for IPv6 addresses. It
// doesn't support dynamic resize: when the max size is reached, the value
// overflows. If you need to change the size, use `resize()`.

class BigInt final {
 public:
  explicit BigInt(uint8_t bytes) {
    m_value.resize(bytes);
    memset(m_value.data(), 0, bytes);
  }

  BigInt(const BigInt& other) { m_value = other.m_value; }

  const uint8_t* value() const { return m_value.data(); }

  uint8_t size() const { return m_value.size(); }

  // Assign operator.

  BigInt& operator=(const BigInt& other) {
    m_value = other.m_value;
    return *this;
  }

  // Comparison operators.

  bool operator==(const BigInt& other) const {
    return m_value == other.m_value;
  }

  bool operator!=(const BigInt& other) const { return !(*this == other); }

  bool operator<(const BigInt& other) const { return cmp(other) < 0; }

  bool operator>(const BigInt& other) const { return cmp(other) > 0; }

  bool operator<=(const BigInt& other) const { return cmp(other) <= 0; }

  bool operator>=(const BigInt& other) const { return cmp(other) >= 0; }

  // math operators (only some of them are implemented)

  BigInt& operator++() {
    for (int i = size() - 1; i >= 0; --i) {
      if (m_value[i] < UINT8_MAX) {
        ++m_value[i];
        return *this;
      }
      m_value[i] = 0;
    }

    // overflow
    memset(m_value.data(), 0, size());
    return *this;
  }

  BigInt& operator+=(const BigInt& other) {
    Q_ASSERT(other.size() == size());

    uint8_t carry = 0;
    for (int i = m_value.size() - 1; i >= 0; --i) {
      uint16_t total = carry + m_value[i] + other.m_value[i];
      m_value[i] = (uint8_t)(total & UINT8_MAX);
      carry = (uint8_t)((total & 0xFF00) >> 8);
    }

    return *this;
  }

  // Shift operators

  BigInt operator>>(int shift) {
    BigInt x(size());
    x = *this;

    for (int i = 0; i < shift; i++) {
      BigInt a(size());
      a = x;

      a.m_value[size() - 1] = x.m_value[size() - 1] >> 1;
      for (int j = size() - 2; j >= 0; j--) {
        a.m_value[j] = x.m_value[j] >> 1;
        if ((x.m_value[j] & 1) != 0) {
          a.m_value[j + 1] |= 128;  // Set most significant bit or a uint8_t
        }
      }

      x = a;
    }

    return x;
  }

  void setValueAt(uint8_t value, uint8_t pos) {
    Q_ASSERT(pos < size());
    m_value[pos] = value;
  }

  uint8_t valueAt(uint8_t pos) const {
    Q_ASSERT(size() > pos);
    return m_value[pos];
  }

 private:
  int cmp(const BigInt& other) const {
    Q_ASSERT(size() == other.size());
    for (int i = 0; i < size(); i++) {
      int diff = (m_value[i] - other.m_value[i]);
      if (diff != 0) return diff;
    }
    return 0;
  }

 private:
  QVector<uint8_t> m_value;
};

#endif  // BIGINT_H

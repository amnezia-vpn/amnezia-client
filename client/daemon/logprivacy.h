/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LOGPRIVACY_H
#define LOGPRIVACY_H

#include <QString>

// While a LogPrivacy::Scope is alive on this thread, code that would write a
// destination address to the service log writes a placeholder instead.
//
// The daemon opens a scope around the routes and permits it installs for
// wildcard site entries: those addresses are learned from the user's own DNS
// lookups, so logging them would turn the service log into a timestamped list
// of the sites the user reached outside the VPN.
namespace LogPrivacy {

inline int& depth() {
  thread_local int value = 0;
  return value;
}

inline bool active() { return depth() > 0; }

// Returns `text` unless a scope is active.
inline QString address(const QString& text) {
  return active() ? QStringLiteral("<learned address>") : text;
}

class Scope {
 public:
  Scope() { ++depth(); }
  ~Scope() { --depth(); }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
};

}  // namespace LogPrivacy

#endif  // LOGPRIVACY_H

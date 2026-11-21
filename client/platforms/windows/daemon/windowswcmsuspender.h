/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSWCMSUSPENDER_H
#define WINDOWSWCMSUSPENDER_H

#include <windows.h>

// Suspends wcmSvc for a bulk route update: it reacts to every route change
// and stalls installs of thousands of routes. Same approach as RouterWin.
class WcmSuspender final {
 public:
  static constexpr int kThreshold = 500;

  explicit WcmSuspender(int pendingRouteOps);
  ~WcmSuspender();

  WcmSuspender(const WcmSuspender&) = delete;
  WcmSuspender& operator=(const WcmSuspender&) = delete;

  bool isSuspended() const { return m_suspended; }

 private:
  HANDLE m_process = nullptr;
  bool m_suspended = false;
};

#endif  // WINDOWSWCMSUSPENDER_H

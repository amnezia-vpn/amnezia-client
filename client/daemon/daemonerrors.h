/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>

enum class DaemonError : uint8_t {
  ERROR_NONE = 0u,
  ERROR_FATAL = 1u,
  ERROR_SPLIT_TUNNEL_INIT_FAILURE = 2u,
  ERROR_SPLIT_TUNNEL_START_FAILURE = 3u,
  ERROR_SPLIT_TUNNEL_EXCLUDE_FAILURE = 4u,

  DAEMON_ERROR_MAX = 5u,
};

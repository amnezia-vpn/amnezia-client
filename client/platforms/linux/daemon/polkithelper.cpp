/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "polkithelper.h"

#include <QDebug>

// No extra QT includes after this line!
#undef Q_SIGNALS
#include "polkit/polkit.h"

class Helper final {
 public:
  Helper() = default;
  ~Helper() {
    if (m_error) {
      g_error_free(m_error);
    }

    if (m_subject) {
      g_object_unref(m_subject);
    }

    if (m_result) {
      g_object_unref(m_result);
    }
  }

 public:
  GError* m_error = nullptr;
  PolkitSubject* m_subject = nullptr;
  PolkitAuthorizationResult* m_result = nullptr;
};

// static
PolkitHelper* PolkitHelper::instance() {
  static PolkitHelper s_instance;
  return &s_instance;
}

bool PolkitHelper::checkAuthorization(const QString& actionId) {
  qDebug() << "Check Authorization for" << actionId;

  Helper h;

  PolkitAuthority* authority = polkit_authority_get_sync(NULL, &h.m_error);
  if (h.m_error) {
    qDebug() << "Fail to generate a polkit authority object:"
             << h.m_error->message;
    return false;
  }

  h.m_subject = polkit_unix_process_new_for_owner(getpid(), 0, -1);

  h.m_result = polkit_authority_check_authorization_sync(
      authority, h.m_subject, actionId.toLatin1().data(), nullptr,
      POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION, nullptr,
      &h.m_error);
  if (h.m_error) {
    qDebug() << "Authorization sync failed:" << h.m_error->message;
    return false;
  }

  return polkit_authorization_result_get_is_authorized(h.m_result);
}

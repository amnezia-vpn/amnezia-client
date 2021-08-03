/*
* TLS Session Manager storing to encrypted SQL db table
* (C) 2012,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SQL_SESSION_MANAGER_H_
#define BOTAN_TLS_SQL_SESSION_MANAGER_H_

#include <botan/tls_session_manager.h>
#include <botan/database.h>

namespace Botan {

class RandomNumberGenerator;

namespace TLS {

/**
* An implementation of Session_Manager that saves values in a SQL
* database file, with the session data encrypted using a passphrase.
*
* @warning For clients, the hostnames associated with the saved
* sessions are stored in the database in plaintext. This may be a
* serious privacy risk in some situations.
*/
class BOTAN_PUBLIC_API(2,0) Session_Manager_SQL : public Session_Manager
   {
   public:
      /**
      * @param db A connection to the database to use
               The table names botan_tls_sessions and
               botan_tls_sessions_metadata will be used
      * @param passphrase used to encrypt the session data
      * @param rng a random number generator
      * @param max_sessions a hint on the maximum number of sessions
      *        to keep in memory at any one time. (If zero, don't cap)
      * @param session_lifetime sessions are expired after this many
      *        seconds have elapsed from initial handshake.
      */
      Session_Manager_SQL(std::shared_ptr<SQL_Database> db,
                          const std::string& passphrase,
                          RandomNumberGenerator& rng,
                          size_t max_sessions = 1000,
                          std::chrono::seconds session_lifetime = std::chrono::seconds(7200));

      Session_Manager_SQL(const Session_Manager_SQL&) = delete;

      Session_Manager_SQL& operator=(const Session_Manager_SQL&) = delete;

      bool load_from_session_id(const std::vector<uint8_t>& session_id,
                                Session& session) override;

      bool load_from_server_info(const Server_Information& info,
                                 Session& session) override;

      void remove_entry(const std::vector<uint8_t>& session_id) override;

      size_t remove_all() override;

      void save(const Session& session_data) override;

      std::chrono::seconds session_lifetime() const override
         { return m_session_lifetime; }

   private:
      void prune_session_cache();

      std::shared_ptr<SQL_Database> m_db;
      secure_vector<uint8_t> m_session_key;
      RandomNumberGenerator& m_rng;
      size_t m_max_sessions;
      std::chrono::seconds m_session_lifetime;
   };

}

}

#endif

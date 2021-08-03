/*
* TLS Record Handling
* (C) 2004-2012 Jack Lloyd
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_RECORDS_H_
#define BOTAN_TLS_RECORDS_H_

#include <botan/tls_algos.h>
#include <botan/tls_magic.h>
#include <botan/tls_version.h>
#include <botan/aead.h>
#include <vector>
#include <chrono>
#include <functional>

namespace Botan {

namespace TLS {

class Ciphersuite;
class Session_Keys;

class Connection_Sequence_Numbers;

/**
* TLS Cipher State
*/
class Connection_Cipher_State final
   {
   public:
      /**
      * Initialize a new cipher state
      */
      Connection_Cipher_State(Protocol_Version version,
                              Connection_Side which_side,
                              bool is_our_side,
                              const Ciphersuite& suite,
                              const Session_Keys& keys,
                              bool uses_encrypt_then_mac);

      AEAD_Mode& aead()
         {
         BOTAN_ASSERT_NONNULL(m_aead.get());
         return *m_aead.get();
         }

      std::vector<uint8_t> aead_nonce(uint64_t seq, RandomNumberGenerator& rng);

      std::vector<uint8_t> aead_nonce(const uint8_t record[], size_t record_len, uint64_t seq);

      std::vector<uint8_t> format_ad(uint64_t seq, uint8_t type,
                                  Protocol_Version version,
                                  uint16_t ptext_length);

      size_t nonce_bytes_from_handshake() const { return m_nonce_bytes_from_handshake; }
      size_t nonce_bytes_from_record() const { return m_nonce_bytes_from_record; }

      Nonce_Format nonce_format() const { return m_nonce_format; }

      std::chrono::seconds age() const
         {
         return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - m_start_time);
         }

   private:
      std::chrono::system_clock::time_point m_start_time;
      std::unique_ptr<AEAD_Mode> m_aead;

      std::vector<uint8_t> m_nonce;
      Nonce_Format m_nonce_format;
      size_t m_nonce_bytes_from_handshake;
      size_t m_nonce_bytes_from_record;
   };

class Record_Header final
   {
   public:
      Record_Header(uint64_t sequence,
                    Protocol_Version version,
                    Record_Type type) :
         m_needed(0),
         m_sequence(sequence),
         m_version(version),
         m_type(type)
         {}

      Record_Header(size_t needed) :
         m_needed(needed),
         m_sequence(0),
         m_version(Protocol_Version()),
         m_type(NO_RECORD)
         {}

      size_t needed() const { return m_needed; }

      Protocol_Version version() const
         {
         BOTAN_ASSERT_NOMSG(m_needed == 0);
         return m_version;
         }

      uint64_t sequence() const
         {
         BOTAN_ASSERT_NOMSG(m_needed == 0);
         return m_sequence;
         }

      uint16_t epoch() const
         {
         return static_cast<uint16_t>(sequence() >> 48);
         }

      Record_Type type() const
         {
         BOTAN_ASSERT_NOMSG(m_needed == 0);
         return m_type;
         }

   private:
      size_t m_needed;
      uint64_t m_sequence;
      Protocol_Version m_version;
      Record_Type m_type;
   };

/**
* Create an initial (unencrypted) TLS handshake record
* @param write_buffer the output record is placed here
* @param record_type the record layer type
* @param record_version the record layer version
* @param record_sequence the record layer sequence number
* @param message the record contents
* @param message_len is size of message
*/
void write_unencrypted_record(secure_vector<uint8_t>& write_buffer,
                              uint8_t record_type,
                              Protocol_Version record_version,
                              uint64_t record_sequence,
                              const uint8_t* message,
                              size_t message_len);

/**
* Create a TLS record
* @param write_buffer the output record is placed here
* @param record_type the record layer type
* @param record_version the record layer version
* @param record_sequence the record layer sequence number
* @param message the record contents
* @param message_len is size of message
* @param cipherstate is the writing cipher state
* @param rng is a random number generator
*/
void write_record(secure_vector<uint8_t>& write_buffer,
                  uint8_t record_type,
                  Protocol_Version record_version,
                  uint64_t record_sequence,
                  const uint8_t* message,
                  size_t message_len,
                  Connection_Cipher_State& cipherstate,
                  RandomNumberGenerator& rng);

// epoch -> cipher state
typedef std::function<std::shared_ptr<Connection_Cipher_State> (uint16_t)> get_cipherstate_fn;

/**
* Decode a TLS record
* @return zero if full message, else number of bytes still needed
*/
Record_Header read_record(bool is_datagram,
                          secure_vector<uint8_t>& read_buffer,
                          const uint8_t input[],
                          size_t input_len,
                          size_t& consumed,
                          secure_vector<uint8_t>& record_buf,
                          Connection_Sequence_Numbers* sequence_numbers,
                          get_cipherstate_fn get_cipherstate,
                          bool allow_epoch0_restart);

}

}

#endif

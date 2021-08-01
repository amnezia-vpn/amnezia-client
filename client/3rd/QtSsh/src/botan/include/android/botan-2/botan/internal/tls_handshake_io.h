/*
* TLS Handshake Serialization
* (C) 2012,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_HANDSHAKE_IO_H_
#define BOTAN_TLS_HANDSHAKE_IO_H_

#include <botan/tls_magic.h>
#include <botan/tls_version.h>
#include <functional>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <utility>

namespace Botan {

namespace TLS {

class Handshake_Message;

/**
* Handshake IO Interface
*/
class Handshake_IO
   {
   public:
      virtual Protocol_Version initial_record_version() const = 0;

      virtual std::vector<uint8_t> send(const Handshake_Message& msg) = 0;

      virtual std::vector<uint8_t> send_under_epoch(const Handshake_Message& msg, uint16_t epoch) = 0;

      virtual bool timeout_check() = 0;

      virtual std::vector<uint8_t> format(
         const std::vector<uint8_t>& handshake_msg,
         Handshake_Type handshake_type) const = 0;

      virtual void add_record(const uint8_t record[],
                              size_t record_len,
                              Record_Type type,
                              uint64_t sequence_number) = 0;

      /**
      * Returns (HANDSHAKE_NONE, std::vector<>()) if no message currently available
      */
      virtual std::pair<Handshake_Type, std::vector<uint8_t>>
         get_next_record(bool expecting_ccs) = 0;

      Handshake_IO() = default;

      Handshake_IO(const Handshake_IO&) = delete;

      Handshake_IO& operator=(const Handshake_IO&) = delete;

      virtual ~Handshake_IO() = default;
   };

/**
* Handshake IO for stream-based handshakes
*/
class Stream_Handshake_IO final : public Handshake_IO
   {
   public:
      typedef std::function<void (uint8_t, const std::vector<uint8_t>&)> writer_fn;

      explicit Stream_Handshake_IO(writer_fn writer) : m_send_hs(writer) {}

      Protocol_Version initial_record_version() const override;

      bool timeout_check() override { return false; }

      std::vector<uint8_t> send(const Handshake_Message& msg) override;

      std::vector<uint8_t> send_under_epoch(const Handshake_Message& msg, uint16_t epoch) override;

      std::vector<uint8_t> format(
         const std::vector<uint8_t>& handshake_msg,
         Handshake_Type handshake_type) const override;

      void add_record(const uint8_t record[],
                      size_t record_len,
                      Record_Type type,
                      uint64_t sequence_number) override;

      std::pair<Handshake_Type, std::vector<uint8_t>>
         get_next_record(bool expecting_ccs) override;
   private:
      std::deque<uint8_t> m_queue;
      writer_fn m_send_hs;
   };

/**
* Handshake IO for datagram-based handshakes
*/
class Datagram_Handshake_IO final : public Handshake_IO
   {
   public:
      typedef std::function<void (uint16_t, uint8_t, const std::vector<uint8_t>&)> writer_fn;

      Datagram_Handshake_IO(writer_fn writer,
                            class Connection_Sequence_Numbers& seq,
                            uint16_t mtu, uint64_t initial_timeout_ms, uint64_t max_timeout_ms) :
         m_seqs(seq),
         m_flights(1),
         m_initial_timeout(initial_timeout_ms),
         m_max_timeout(max_timeout_ms),
         m_send_hs(writer),
         m_mtu(mtu)
         {}

      Protocol_Version initial_record_version() const override;

      bool timeout_check() override;

      std::vector<uint8_t> send(const Handshake_Message& msg) override;

      std::vector<uint8_t> send_under_epoch(const Handshake_Message& msg, uint16_t epoch) override;

      std::vector<uint8_t> format(
         const std::vector<uint8_t>& handshake_msg,
         Handshake_Type handshake_type) const override;

      void add_record(const uint8_t record[],
                      size_t record_len,
                      Record_Type type,
                      uint64_t sequence_number) override;

      std::pair<Handshake_Type, std::vector<uint8_t>>
         get_next_record(bool expecting_ccs) override;
   private:
      void retransmit_flight(size_t flight);
      void retransmit_last_flight();

      std::vector<uint8_t> format_fragment(
         const uint8_t fragment[],
         size_t fragment_len,
         uint16_t frag_offset,
         uint16_t msg_len,
         Handshake_Type type,
         uint16_t msg_sequence) const;

      std::vector<uint8_t> format_w_seq(
         const std::vector<uint8_t>& handshake_msg,
         Handshake_Type handshake_type,
         uint16_t msg_sequence) const;

      std::vector<uint8_t> send_message(uint16_t msg_seq, uint16_t epoch,
                                     Handshake_Type msg_type,
                                     const std::vector<uint8_t>& msg);

      class Handshake_Reassembly final
         {
         public:
            void add_fragment(const uint8_t fragment[],
                              size_t fragment_length,
                              size_t fragment_offset,
                              uint16_t epoch,
                              uint8_t msg_type,
                              size_t msg_length);

            bool complete() const;

            uint16_t epoch() const { return m_epoch; }

            std::pair<Handshake_Type, std::vector<uint8_t>> message() const;
         private:
            uint8_t m_msg_type = HANDSHAKE_NONE;
            size_t m_msg_length = 0;
            uint16_t m_epoch = 0;

            // vector<bool> m_seen;
            // vector<uint8_t> m_fragments
            std::map<size_t, uint8_t> m_fragments;
            std::vector<uint8_t> m_message;
         };

      struct Message_Info final
         {
         Message_Info(uint16_t e, Handshake_Type mt, const std::vector<uint8_t>& msg) :
            epoch(e), msg_type(mt), msg_bits(msg) {}

         Message_Info() : epoch(0xFFFF), msg_type(HANDSHAKE_NONE) {}

         uint16_t epoch;
         Handshake_Type msg_type;
         std::vector<uint8_t> msg_bits;
         };

      class Connection_Sequence_Numbers& m_seqs;
      std::map<uint16_t, Handshake_Reassembly> m_messages;
      std::set<uint16_t> m_ccs_epochs;
      std::vector<std::vector<uint16_t>> m_flights;
      std::map<uint16_t, Message_Info> m_flight_data;

      uint64_t m_initial_timeout = 0;
      uint64_t m_max_timeout = 0;

      uint64_t m_last_write = 0;
      uint64_t m_next_timeout = 0;

      uint16_t m_in_message_seq = 0;
      uint16_t m_out_message_seq = 0;

      writer_fn m_send_hs;
      uint16_t m_mtu;
   };

}

}

#endif

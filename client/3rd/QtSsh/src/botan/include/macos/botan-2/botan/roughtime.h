/*
* Roughtime
* (C) 2019 Nuno Goncalves <nunojpg@gmail.com>
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ROUGHTIME_H_
#define BOTAN_ROUGHTIME_H_

#include <array>
#include <chrono>
#include <vector>

#include <botan/ed25519.h>

namespace Botan {

class RandomNumberGenerator;

namespace Roughtime {

const unsigned request_min_size = 1024;

class BOTAN_PUBLIC_API(2, 13) Roughtime_Error final : public Decoding_Error
   {
   public:
      explicit Roughtime_Error(const std::string& s) : Decoding_Error("Roughtime " + s) {}
      ErrorType error_type() const noexcept override { return ErrorType::RoughtimeError; }
   };

class BOTAN_PUBLIC_API(2, 13) Nonce final
   {
   public:
      Nonce() = default;
      Nonce(const std::vector<uint8_t>& nonce);
      Nonce(RandomNumberGenerator& rng);
      Nonce(const std::array<uint8_t, 64>& nonce)
         {
         m_nonce = nonce;
         }
      bool operator==(const Nonce& rhs) const { return m_nonce == rhs.m_nonce; }
      const std::array<uint8_t, 64>& get_nonce() const { return m_nonce; }
   private:
      std::array<uint8_t, 64> m_nonce;
   };


/**
* An Roughtime request.
*/
BOTAN_PUBLIC_API(2, 13)
std::array<uint8_t, request_min_size> encode_request(const Nonce& nonce);

/**
* An Roughtime response.
*/
class BOTAN_PUBLIC_API(2, 13) Response final
   {
   public:
      using microseconds32 = std::chrono::duration<uint32_t, std::micro>;
      using microseconds64 = std::chrono::duration<uint64_t, std::micro>;
      using sys_microseconds64 = std::chrono::time_point<std::chrono::system_clock, microseconds64>;

      static Response from_bits(const std::vector<uint8_t>& response, const Nonce& nonce);

      bool validate(const Ed25519_PublicKey& pk) const;

      sys_microseconds64 utc_midpoint() const { return m_utc_midpoint; }

      microseconds32 utc_radius() const { return m_utc_radius; }
   private:
      Response(const std::array<uint8_t, 72>& dele,
               const std::array<uint8_t, 64>& sig,
               sys_microseconds64 utc_midp,
               microseconds32 utc_radius)
         : m_cert_dele(dele)
         , m_cert_sig(sig)
         , m_utc_midpoint {utc_midp}
         , m_utc_radius {utc_radius}
         {}
      const std::array<uint8_t, 72> m_cert_dele;
      const std::array<uint8_t, 64> m_cert_sig;
      const sys_microseconds64 m_utc_midpoint;
      const microseconds32 m_utc_radius;
   };

class BOTAN_PUBLIC_API(2, 13) Link final
   {
   public:
      Link(const std::vector<uint8_t>& response,
           const Ed25519_PublicKey& public_key,
           const Nonce& nonce_or_blind)
         : m_response{response}
         , m_public_key{public_key}
         , m_nonce_or_blind{nonce_or_blind}
         {}
      const std::vector<uint8_t>& response() const { return m_response; }
      const Ed25519_PublicKey& public_key() const { return m_public_key; }
      const Nonce& nonce_or_blind() const { return m_nonce_or_blind; }
      Nonce& nonce_or_blind() { return m_nonce_or_blind; }

   private:
      std::vector<uint8_t> m_response;
      Ed25519_PublicKey m_public_key;
      Nonce m_nonce_or_blind;
   };

class BOTAN_PUBLIC_API(2, 13) Chain final
   {
   public:
      Chain() = default; //empty
      Chain(const std::string& str);
      const std::vector<Link>& links() const { return m_links; }
      std::vector<Response> responses() const;
      Nonce next_nonce(const Nonce& blind) const;
      void append(const Link& new_link, size_t max_chain_size);
      std::string to_string() const;
   private:
      std::vector<Link> m_links;
   };

/**
*/
BOTAN_PUBLIC_API(2, 13)
Nonce nonce_from_blind(const std::vector<uint8_t>& previous_response,
                       const Nonce& blind);

/**
* Makes an online Roughtime request via UDP and returns the Roughtime response.
* @param url Roughtime server UDP endpoint (host:port)
* @param nonce the nonce to send to the server
* @param timeout a timeout on the UDP request
* @return Roughtime response
*/
BOTAN_PUBLIC_API(2, 13)
std::vector<uint8_t> online_request(const std::string& url,
                                    const Nonce& nonce,
                                    std::chrono::milliseconds timeout = std::chrono::seconds(3));

struct BOTAN_PUBLIC_API(2, 13) Server_Information final
   {
public:
   Server_Information(const std::string& name,
                      const Botan::Ed25519_PublicKey& public_key,
                      const std::vector<std::string>& addresses)
      : m_name { name }
      , m_public_key { public_key }
      , m_addresses { addresses }
      {}
   const std::string& name() const {return m_name;}
   const Botan::Ed25519_PublicKey& public_key() const {return m_public_key;}
   const std::vector<std::string>& addresses() const {return m_addresses;}

private:
   std::string m_name;
   Botan::Ed25519_PublicKey m_public_key;
   std::vector<std::string> m_addresses;
   };

BOTAN_PUBLIC_API(2, 13)
std::vector<Server_Information> servers_from_str(const std::string& str);

}
}

#endif

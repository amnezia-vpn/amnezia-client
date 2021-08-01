/*
 * XMSS Signature Operation
 * (C) 2016,2017,2018 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_SIGNATURE_OPERATION_H_
#define BOTAN_XMSS_SIGNATURE_OPERATION_H_

#include <botan/pk_ops.h>
#include <botan/xmss.h>
#include <botan/internal/xmss_address.h>
#include <botan/internal/xmss_signature.h>
#include <botan/xmss_wots.h>

namespace Botan {

/**
 * Signature generation operation for Extended Hash-Based Signatures (XMSS) as
 * defined in:
 *
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class XMSS_Signature_Operation final : public virtual PK_Ops::Signature
   {
   public:
      XMSS_Signature_Operation(const XMSS_PrivateKey& private_key);

      /**
       * Creates an XMSS signature for the message provided through call to
       * update().
       *
       * @return serialized XMSS signature.
       **/
      secure_vector<uint8_t> sign(RandomNumberGenerator&) override;

      void update(const uint8_t msg[], size_t msg_len) override;

      size_t signature_length() const override;

   private:
      /**
       * Algorithm 11: "treeSig"
       * Generate a WOTS+ signature on a message with corresponding auth path.
       *
       * @param msg A message.
       * @param xmss_priv_key A XMSS private key.
       * @param adrs A XMSS Address.
       **/
      XMSS_WOTS_PublicKey::TreeSignature generate_tree_signature(
         const secure_vector<uint8_t>& msg,
         XMSS_PrivateKey& xmss_priv_key,
         XMSS_Address& adrs);

      /**
       * Algorithm 12: "XMSS_sign"
       * Generate an XMSS signature and update the XMSS secret key
       *
       * @param msg A message to sign of arbitrary length.
       * @param [out] xmss_priv_key A XMSS private key. The private key will be
       *              updated during the signing process.
       *
       * @return The signature of msg signed using xmss_priv_key.
       **/
      XMSS_Signature sign(
         const secure_vector<uint8_t>& msg,
         XMSS_PrivateKey& xmss_priv_key);

      wots_keysig_t build_auth_path(XMSS_PrivateKey& priv_key,
                                    XMSS_Address& adrs);

      void initialize();

      XMSS_PrivateKey m_priv_key;
      const XMSS_Parameters m_xmss_params;
      XMSS_Hash m_hash;
      secure_vector<uint8_t> m_randomness;
      uint32_t m_leaf_idx;
      bool m_is_initialized;
   };

}

#endif


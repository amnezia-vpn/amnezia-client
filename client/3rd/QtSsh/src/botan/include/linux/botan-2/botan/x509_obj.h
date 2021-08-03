/*
* X.509 SIGNED Object
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_OBJECT_H_
#define BOTAN_X509_OBJECT_H_

#include <botan/asn1_obj.h>
#include <botan/pkix_enums.h>
#include <vector>

namespace Botan {

class Public_Key;
class Private_Key;
class RandomNumberGenerator;

/**
* This class represents abstract X.509 signed objects as in the X.500
* SIGNED macro
*/
class BOTAN_PUBLIC_API(2,0) X509_Object : public ASN1_Object
   {
   public:
      /**
      * The underlying data that is to be or was signed
      * @return data that is or was signed
      */
      std::vector<uint8_t> tbs_data() const;

      /**
      * @return signature on tbs_data()
      */
      const std::vector<uint8_t>& signature() const { return m_sig; }

      /**
      * @return signed body
      */
      const std::vector<uint8_t>& signed_body() const { return m_tbs_bits; }

      /**
      * @return signature algorithm that was used to generate signature
      */
      const AlgorithmIdentifier& signature_algorithm() const { return m_sig_algo; }

      /**
      * @return hash algorithm that was used to generate signature
      */
      std::string hash_used_for_signature() const;

      /**
      * Create a signed X509 object.
      * @param signer the signer used to sign the object
      * @param rng the random number generator to use
      * @param alg_id the algorithm identifier of the signature scheme
      * @param tbs the tbs bits to be signed
      * @return signed X509 object
      */
      static std::vector<uint8_t> make_signed(class PK_Signer* signer,
                                              RandomNumberGenerator& rng,
                                              const AlgorithmIdentifier& alg_id,
                                              const secure_vector<uint8_t>& tbs);

      /**
      * Check the signature on this data
      * @param key the public key purportedly used to sign this data
      * @return status of the signature - OK if verified or otherwise an indicator of
      *         the problem preventing verification.
      */
      Certificate_Status_Code verify_signature(const Public_Key& key) const;

      /**
      * Check the signature on this data
      * @param key the public key purportedly used to sign this data
      * @return true if the signature is valid, otherwise false
      */
      bool check_signature(const Public_Key& key) const;

      /**
      * Check the signature on this data
      * @param key the public key purportedly used to sign this data
      *        the object will be deleted after use (this should have
      *        been a std::unique_ptr<Public_Key>)
      * @return true if the signature is valid, otherwise false
      */
      bool check_signature(const Public_Key* key) const;

      /**
      * DER encode an X509_Object
      * See @ref ASN1_Object::encode_into()
      */
      void encode_into(class DER_Encoder& to) const override;

      /**
      * Decode a BER encoded X509_Object
      * See @ref ASN1_Object::decode_from()
      */
      void decode_from(class BER_Decoder& from) override;

      /**
      * @return PEM encoding of this
      */
      std::string PEM_encode() const;

      X509_Object(const X509_Object&) = default;
      X509_Object& operator=(const X509_Object&) = default;

      virtual std::string PEM_label() const = 0;

      virtual std::vector<std::string> alternate_PEM_labels() const
         { return std::vector<std::string>(); }

      virtual ~X509_Object() = default;

      static std::unique_ptr<PK_Signer>
         choose_sig_format(AlgorithmIdentifier& sig_algo,
                           const Private_Key& key,
                           RandomNumberGenerator& rng,
                           const std::string& hash_fn,
                           const std::string& padding_algo);

   protected:

      X509_Object() = default;

      /**
      * Decodes from src as either DER or PEM data, then calls force_decode()
      */
      void load_data(DataSource& src);

   private:
      virtual void force_decode() = 0;

      AlgorithmIdentifier m_sig_algo;
      std::vector<uint8_t> m_tbs_bits;
      std::vector<uint8_t> m_sig;
   };

}

#endif

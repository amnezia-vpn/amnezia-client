/*
* ECIES
* (C) 2016 Philipp Weber
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECIES_H_
#define BOTAN_ECIES_H_

#include <botan/ecdh.h>
#include <botan/ec_group.h>
#include <botan/cipher_mode.h>
#include <botan/point_gfp.h>
#include <botan/pubkey.h>
#include <botan/secmem.h>
#include <botan/symkey.h>
#include <botan/mac.h>
#include <memory>
#include <string>
#include <vector>

namespace Botan {

class RandomNumberGenerator;

enum class ECIES_Flags : uint32_t
   {
   NONE = 0,

   /// if set: prefix the input of the (ecdh) key agreement with the encoded (ephemeral) public key
   SINGLE_HASH_MODE = 1,

   /// (decryption only) if set: use cofactor multiplication during (ecdh) key agreement
   COFACTOR_MODE = 2,

   /// if set: use ecdhc instead of ecdh
   OLD_COFACTOR_MODE = 4,

   /// (decryption only) if set: test if the (ephemeral) public key is on the curve
   CHECK_MODE = 8
   };

inline ECIES_Flags operator |(ECIES_Flags a, ECIES_Flags b)
   {
   return static_cast<ECIES_Flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
   }

inline ECIES_Flags operator &(ECIES_Flags a, ECIES_Flags b)
   {
   return static_cast<ECIES_Flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
   }

/**
* Parameters for ECIES secret derivation
*/
class BOTAN_PUBLIC_API(2,0) ECIES_KA_Params
   {
   public:
      /**
      * @param domain ec domain parameters of the involved ec keys
      * @param kdf_spec name of the key derivation function
      * @param length length of the secret to be derived
      * @param compression_type format of encoded keys (affects the secret derivation if single_hash_mode is used)
      * @param flags options, see documentation of ECIES_Flags
      */
      ECIES_KA_Params(const EC_Group& domain, const std::string& kdf_spec, size_t length,
                      PointGFp::Compression_Type compression_type, ECIES_Flags flags);

      ECIES_KA_Params(const ECIES_KA_Params&) = default;
      ECIES_KA_Params& operator=(const ECIES_KA_Params&) = delete;

      virtual ~ECIES_KA_Params() = default;

      inline const EC_Group& domain() const
         {
         return m_domain;
         }

      inline size_t secret_length() const
         {
         return m_length;
         }

      inline bool single_hash_mode() const
         {
         return (m_flags & ECIES_Flags::SINGLE_HASH_MODE) == ECIES_Flags::SINGLE_HASH_MODE;
         }

      inline bool cofactor_mode() const
         {
         return (m_flags & ECIES_Flags::COFACTOR_MODE) == ECIES_Flags::COFACTOR_MODE;
         }

      inline bool old_cofactor_mode() const
         {
         return (m_flags & ECIES_Flags::OLD_COFACTOR_MODE) == ECIES_Flags::OLD_COFACTOR_MODE;
         }

      inline bool check_mode() const
         {
         return (m_flags & ECIES_Flags::CHECK_MODE) == ECIES_Flags::CHECK_MODE;
         }

      inline PointGFp::Compression_Type compression_type() const
         {
         return m_compression_mode;
         }

      const std::string& kdf_spec() const
         {
         return m_kdf_spec;
         }

   private:
      const EC_Group m_domain;
      const std::string m_kdf_spec;
      const size_t m_length;
      const PointGFp::Compression_Type m_compression_mode;
      const ECIES_Flags m_flags;
   };


class BOTAN_PUBLIC_API(2,0) ECIES_System_Params final : public ECIES_KA_Params
   {
   public:
      /**
      * @param domain ec domain parameters of the involved ec keys
      * @param kdf_spec name of the key derivation function
      * @param dem_algo_spec name of the data encryption method
      * @param dem_key_len length of the key used for the data encryption method
      * @param mac_spec name of the message authentication code
      * @param mac_key_len length of the key used for the message authentication code
      */
      ECIES_System_Params(const EC_Group& domain, const std::string& kdf_spec, const std::string& dem_algo_spec,
                          size_t dem_key_len, const std::string& mac_spec, size_t mac_key_len);

      /**
      * @param domain ec domain parameters of the involved ec keys
      * @param kdf_spec name of the key derivation function
      * @param dem_algo_spec name of the data encryption method
      * @param dem_key_len length of the key used for the data encryption method
      * @param mac_spec name of the message authentication code
      * @param mac_key_len length of the key used for the message authentication code
      * @param compression_type format of encoded keys (affects the secret derivation if single_hash_mode is used)
      * @param flags options, see documentation of ECIES_Flags
      */
      ECIES_System_Params(const EC_Group& domain, const std::string& kdf_spec, const std::string& dem_algo_spec,
                          size_t dem_key_len, const std::string& mac_spec, size_t mac_key_len,
                          PointGFp::Compression_Type compression_type, ECIES_Flags flags);

      ECIES_System_Params(const ECIES_System_Params&) = default;
      ECIES_System_Params& operator=(const ECIES_System_Params&) = delete;
      virtual ~ECIES_System_Params() = default;

      /// creates an instance of the message authentication code
      std::unique_ptr<MessageAuthenticationCode> create_mac() const;

      /// creates an instance of the data encryption method
      std::unique_ptr<Cipher_Mode> create_cipher(Botan::Cipher_Dir direction) const;

      /// returns the length of the key used by the data encryption method
      inline size_t dem_keylen() const
         {
         return m_dem_keylen;
         }

      /// returns the length of the key used by the message authentication code
      inline size_t mac_keylen() const
         {
         return m_mac_keylen;
         }

   private:
      const std::string m_dem_spec;
      const size_t m_dem_keylen;
      const std::string m_mac_spec;
      const size_t m_mac_keylen;
   };


/**
* ECIES secret derivation according to ISO 18033-2
*/
class BOTAN_PUBLIC_API(2,0) ECIES_KA_Operation
   {
   public:
      /**
      * @param private_key the (ephemeral) private key which is used to derive the secret
      * @param ecies_params settings for ecies
      * @param for_encryption disable cofactor mode if the secret will be used for encryption
      * (according to ISO 18033 cofactor mode is only used during decryption)
      * @param rng the RNG to use
      */
      ECIES_KA_Operation(const PK_Key_Agreement_Key& private_key,
                         const ECIES_KA_Params& ecies_params,
                         bool for_encryption,
                         RandomNumberGenerator& rng);

      /**
      * Performs a key agreement with the provided keys and derives the secret from the result
      * @param eph_public_key_bin the encoded (ephemeral) public key which belongs to the used (ephemeral) private key
      * @param other_public_key_point public key point of the other party
      */
      SymmetricKey derive_secret(const std::vector<uint8_t>& eph_public_key_bin,
                                 const PointGFp& other_public_key_point) const;

   private:
      const PK_Key_Agreement m_ka;
      const ECIES_KA_Params m_params;
   };


/**
* ECIES Encryption according to ISO 18033-2
*/
class BOTAN_PUBLIC_API(2,0) ECIES_Encryptor final : public PK_Encryptor
   {
   public:
      /**
      * @param private_key the (ephemeral) private key which is used for the key agreement
      * @param ecies_params settings for ecies
      * @param rng random generator to use
      */
      ECIES_Encryptor(const PK_Key_Agreement_Key& private_key,
                      const ECIES_System_Params& ecies_params,
                      RandomNumberGenerator& rng);

      /**
      * Creates an ephemeral private key which is used for the key agreement
      * @param rng random generator used during private key generation
      * @param ecies_params settings for ecies
      */
      ECIES_Encryptor(RandomNumberGenerator& rng, const ECIES_System_Params& ecies_params);

      /// Set the public key of the other party
      inline void set_other_key(const Botan::PointGFp& public_point)
         {
         m_other_point = public_point;
         }

      /// Set the initialization vector for the data encryption method
      inline void set_initialization_vector(const InitializationVector& iv)
         {
         m_iv = iv;
         }

      /// Set the label which is appended to the input for the message authentication code
      inline void set_label(const std::string& label)
         {
         m_label = std::vector<uint8_t>(label.begin(), label.end());
         }

   private:
      std::vector<uint8_t> enc(const uint8_t data[], size_t length, RandomNumberGenerator&) const override;

      size_t maximum_input_size() const override;

      size_t ciphertext_length(size_t ptext_len) const override;

      const ECIES_KA_Operation m_ka;
      const ECIES_System_Params m_params;
      std::unique_ptr<MessageAuthenticationCode> m_mac;
      std::unique_ptr<Cipher_Mode> m_cipher;
      std::vector<uint8_t> m_eph_public_key_bin;
      InitializationVector m_iv;
      PointGFp m_other_point;
      std::vector<uint8_t> m_label;
   };


/**
* ECIES Decryption according to ISO 18033-2
*/
class BOTAN_PUBLIC_API(2,0) ECIES_Decryptor final : public PK_Decryptor
   {
   public:
      /**
      * @param private_key the private key which is used for the key agreement
      * @param ecies_params settings for ecies
      * @param rng the random generator to use
      */
      ECIES_Decryptor(const PK_Key_Agreement_Key& private_key,
                      const ECIES_System_Params& ecies_params,
                      RandomNumberGenerator& rng);

      /// Set the initialization vector for the data encryption method
      inline void set_initialization_vector(const InitializationVector& iv)
         {
         m_iv = iv;
         }

      /// Set the label which is appended to the input for the message authentication code
      inline void set_label(const std::string& label)
         {
         m_label = std::vector<uint8_t>(label.begin(), label.end());
         }

   private:
      secure_vector<uint8_t> do_decrypt(uint8_t& valid_mask, const uint8_t in[], size_t in_len) const override;

      size_t plaintext_length(size_t ctext_len) const override;

      const ECIES_KA_Operation m_ka;
      const ECIES_System_Params m_params;
      std::unique_ptr<MessageAuthenticationCode> m_mac;
      std::unique_ptr<Cipher_Mode> m_cipher;
      InitializationVector m_iv;
      std::vector<uint8_t> m_label;
   };

}

#endif

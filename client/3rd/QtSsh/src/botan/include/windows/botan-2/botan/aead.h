/*
* Interface for AEAD modes
* (C) 2013 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_AEAD_MODE_H_
#define BOTAN_AEAD_MODE_H_

#include <botan/cipher_mode.h>

namespace Botan {

/**
* Interface for AEAD (Authenticated Encryption with Associated Data)
* modes. These modes provide both encryption and message
* authentication, and can authenticate additional per-message data
* which is not included in the ciphertext (for instance a sequence
* number).
*/
class BOTAN_PUBLIC_API(2,0) AEAD_Mode : public Cipher_Mode
   {
   public:
      /**
      * Create an AEAD mode
      * @param algo the algorithm to create
      * @param direction specify if this should be an encryption or decryption AEAD
      * @param provider optional specification for provider to use
      * @return an AEAD mode or a null pointer if not available
      */
      static std::unique_ptr<AEAD_Mode> create(const std::string& algo,
                                               Cipher_Dir direction,
                                               const std::string& provider = "");

      /**
      * Create an AEAD mode, or throw
      * @param algo the algorithm to create
      * @param direction specify if this should be an encryption or decryption AEAD
      * @param provider optional specification for provider to use
      * @return an AEAD mode, or throw an exception
      */
      static std::unique_ptr<AEAD_Mode> create_or_throw(const std::string& algo,
                                                        Cipher_Dir direction,
                                                        const std::string& provider = "");

      bool authenticated() const override { return true; }

      /**
      * Set associated data that is not included in the ciphertext but
      * that should be authenticated. Must be called after set_key and
      * before start.
      *
      * Unless reset by another call, the associated data is kept
      * between messages. Thus, if the AD does not change, calling
      * once (after set_key) is the optimum.
      *
      * @param ad the associated data
      * @param ad_len length of add in bytes
      */
      virtual void set_associated_data(const uint8_t ad[], size_t ad_len) = 0;

      /**
      * Set associated data that is not included in the ciphertext but
      * that should be authenticated. Must be called after set_key and
      * before start.
      *
      * Unless reset by another call, the associated data is kept
      * between messages. Thus, if the AD does not change, calling
      * once (after set_key) is the optimum.
      *
      * Some AEADs (namely SIV) support multiple AD inputs. For
      * all other modes only nominal AD input 0 is supported; all
      * other values of i will cause an exception.
      *
      * @param ad the associated data
      * @param ad_len length of add in bytes
      */
      virtual void set_associated_data_n(size_t i, const uint8_t ad[], size_t ad_len);

      /**
      * Returns the maximum supported number of associated data inputs which
      * can be provided to set_associated_data_n
      *
      * If returns 0, then no associated data is supported.
      */
      virtual size_t maximum_associated_data_inputs() const { return 1; }

      /**
      * Most AEADs require the key to be set prior to setting the AD
      * A few allow the AD to be set even before the cipher is keyed.
      * Such ciphers would return false from this function.
      */
      virtual bool associated_data_requires_key() const { return true; }

      /**
      * Set associated data that is not included in the ciphertext but
      * that should be authenticated. Must be called after set_key and
      * before start.
      *
      * See @ref set_associated_data().
      *
      * @param ad the associated data
      */
      template<typename Alloc>
      void set_associated_data_vec(const std::vector<uint8_t, Alloc>& ad)
         {
         set_associated_data(ad.data(), ad.size());
         }

      /**
      * Set associated data that is not included in the ciphertext but
      * that should be authenticated. Must be called after set_key and
      * before start.
      *
      * See @ref set_associated_data().
      *
      * @param ad the associated data
      */
      template<typename Alloc>
      void set_ad(const std::vector<uint8_t, Alloc>& ad)
         {
         set_associated_data(ad.data(), ad.size());
         }

      /**
      * @return default AEAD nonce size (a commonly supported value among AEAD
      * modes, and large enough that random collisions are unlikely)
      */
      size_t default_nonce_length() const override { return 12; }

      virtual ~AEAD_Mode() = default;
   };

/**
* Get an AEAD mode by name (eg "AES-128/GCM" or "Serpent/EAX")
* @param name AEAD name
* @param direction ENCRYPTION or DECRYPTION
*/
inline AEAD_Mode* get_aead(const std::string& name, Cipher_Dir direction)
   {
   return AEAD_Mode::create(name, direction, "").release();
   }

}

#endif

/*
 * XMSS WOTS
 * (C) 2016,2018 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_WOTS_H_
#define BOTAN_XMSS_WOTS_H_

#include <botan/asn1_obj.h>
#include <botan/exceptn.h>
#include <botan/pk_keys.h>
#include <botan/rng.h>
#include <botan/secmem.h>
#include <botan/xmss_hash.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Botan {

/**
 * Descibes a signature method for XMSS Winternitz One Time Signatures,
 * as defined in:
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class XMSS_WOTS_Parameters final
   {
   public:
      enum ots_algorithm_t
         {
         WOTSP_SHA2_256 = 0x00000001,
         WOTSP_SHA2_512 = 0x00000002,
         WOTSP_SHAKE_256 = 0x00000003,
         WOTSP_SHAKE_512 = 0x00000004
         };

      XMSS_WOTS_Parameters(const std::string& algo_name);
      XMSS_WOTS_Parameters(ots_algorithm_t ots_spec);

      static ots_algorithm_t xmss_wots_id_from_string(const std::string& param_set);

      /**
       * Algorithm 1: convert input string to base.
       *
       * @param msg Input string (referred to as X in [1]).
       * @param out_size size of message in base w.
       *
       * @return Input string converted to the given base.
       **/
      secure_vector<uint8_t> base_w(const secure_vector<uint8_t>& msg, size_t out_size) const;

      secure_vector<uint8_t> base_w(size_t value) const;

      void append_checksum(secure_vector<uint8_t>& data);

      /**
       * @return XMSS WOTS registry name for the chosen parameter set.
       **/
      const std::string& name() const
         {
         return m_name;
         }

      /**
       * @return Botan name for the hash function used.
       **/
      const std::string& hash_function_name() const
         {
         return m_hash_name;
         }

      /**
       * Retrieves the uniform length of a message, and the size of
       * each node. This correlates to XMSS parameter "n" defined
       * in [1].
       *
       * @return element length in bytes.
       **/
      size_t element_size() const { return m_element_size; }

      /**
       * The Winternitz parameter.
       *
       * @return numeric base used for internal representation of
       *         data.
       **/
      size_t wots_parameter() const { return m_w; }

      size_t len() const { return m_len; }

      size_t len_1() const { return m_len_1; }

      size_t len_2() const { return m_len_2; }

      size_t lg_w() const { return m_lg_w; }

      ots_algorithm_t oid() const { return m_oid; }

      size_t estimated_strength() const { return m_strength; }

      bool operator==(const XMSS_WOTS_Parameters& p) const
         {
         return m_oid == p.m_oid;
         }

   private:
      static const std::map<std::string, ots_algorithm_t> m_oid_name_lut;
      ots_algorithm_t m_oid;
      std::string m_name;
      std::string m_hash_name;
      size_t m_element_size;
      size_t m_w;
      size_t m_len_1;
      size_t m_len_2;
      size_t m_len;
      size_t m_strength;
      uint8_t m_lg_w;
   };

class XMSS_Address;

typedef std::vector<secure_vector<uint8_t>> wots_keysig_t;

/**
 * A Winternitz One Time Signature public key for use with Extended Hash-Based
 * Signatures.
 **/
class XMSS_WOTS_PublicKey : virtual public Public_Key
   {
   public:
      class TreeSignature final
         {
         public:
            TreeSignature() = default;

            TreeSignature(const wots_keysig_t& ots_sig,
                          const wots_keysig_t& auth_path)
               : m_ots_sig(ots_sig), m_auth_path(auth_path)
               {}

            TreeSignature(wots_keysig_t&& ots_sig,
                          wots_keysig_t&& auth_path)
               : m_ots_sig(std::move(ots_sig)),
                 m_auth_path(std::move(auth_path))
               {}

            const wots_keysig_t& ots_signature() const
               {
               return m_ots_sig;
               }

            wots_keysig_t& ots_signature()
               {
               return m_ots_sig;
               }

            const wots_keysig_t& authentication_path() const
               {
               return m_auth_path;
               }

            wots_keysig_t& authentication_path()
               {
               return m_auth_path;
               }

         private:
            wots_keysig_t m_ots_sig;
            wots_keysig_t m_auth_path;
         };

      /**
       * Creates a XMSS_WOTS_PublicKey for the signature method identified by
       * oid. The public seed for this key will be initialized with a
       * uniformly random n-byte value, where "n" is the element size of the
       * selected signature method.
       *
       * @param oid Identifier for the selected signature method.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()) {}

      /**
       * Creates a XMSS_WOTS_PublicKey for the signature method identified by
       * oid. The public seed for this key will be initialized with a
       * uniformly random n-byte value, where "n" is the element size of the
       * selected signature method.
       *
       * @param oid Identifier for the selected signature method.
       * @param rng A random number generate used to generate the public seed.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                          RandomNumberGenerator& rng)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()),
           m_public_seed(rng.random_vec(m_wots_params.element_size())) {}

      /**
       * Creates a XMSS_WOTS_PrivateKey for the signature method identified by
       * oid, with a precomputed public seed.
       *
       * @param oid Identifier for the selected signature method.
       * @param public_seed A precomputed public seed of n-bytes length.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                          secure_vector<uint8_t> public_seed)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()),
           m_public_seed(public_seed) {}

      /**
       * Creates a XMSS_WOTS_PublicKey for the signature method identified by
       * oid. The public seed will be initialized with a precomputed seed and
       * and precomputed key data which should be derived from a
       * XMSS_WOTS_PrivateKey.
       *
       * @param oid Ident:s/ifier for the selected signature methods.
       * @param public_seed A precomputed public seed of n-bytes length.
       * @param key Precomputed raw key data of the XMSS_WOTS_PublicKey.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                          secure_vector<uint8_t>&& public_seed,
                          wots_keysig_t&& key)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()),
           m_key(std::move(key)),
           m_public_seed(std::move(public_seed))
         {}

      /**
       * Creates a XMSS_WOTS_PublicKey for the signature method identified by
       * oid. The public seed will be initialized with a precomputed seed and
       * and precomputed key data which should be derived from a
       * XMSS_WOTS_PrivateKey.
       *
       * @param oid Identifier for the selected signature methods.
       * @param public_seed A precomputed public seed of n-bytes length.
       * @param key Precomputed raw key data of the XMSS_WOTS_PublicKey.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                          const secure_vector<uint8_t>& public_seed,
                          const wots_keysig_t& key)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()),
           m_key(key),
           m_public_seed(public_seed)
         {}

      /**
       * Creates a XMSS_WOTS_PublicKey form a message and signature using
       * Algorithm 6 WOTS_pkFromSig defined in the XMSS standard. This
       * overload is used to verify a message using a public key.
       *
       * @param oid WOTSP algorithm identifier.
       * @param msg A message.
       * @param sig A WOTS signature for msg.
       * @param adrs An XMSS_Address.
       * @param public_seed The public public_seed.
       **/
      XMSS_WOTS_PublicKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                          const secure_vector<uint8_t>& msg,
                          const wots_keysig_t& sig,
                          XMSS_Address& adrs,
                          const secure_vector<uint8_t>& public_seed)
         : m_wots_params(oid),
           m_hash(m_wots_params.hash_function_name()),
           m_key(pub_key_from_signature(msg,
                                        sig,
                                        adrs,
                                        public_seed)),
           m_public_seed(public_seed)
         {}

      /**
       * Retrieves the i-th element out of the length len chain of
       * n-byte elements contained in the public key.
       *
       * @param i index of the element.
       * @returns n-byte element addressed by i.
       **/
      const secure_vector<uint8_t>& operator[](size_t i) const { return m_key[i]; }
      secure_vector<uint8_t>& operator[](size_t i) { return m_key[i]; }

      /**
       * Convert the key into the raw key data. The key becomes a length
       * len vector of n-byte elements.
       **/
      operator const wots_keysig_t& () const { return m_key; }

      /**
       * Convert the key into the raw key data. The key becomes a length
       * len vector of n-byte elements.
       **/
      operator wots_keysig_t& () { return m_key; }

      const secure_vector<uint8_t>& public_seed() const { return m_public_seed; }

      secure_vector<uint8_t>& public_seed() { return m_public_seed; }

      void set_public_seed(const secure_vector<uint8_t>& public_seed)
         {
         m_public_seed = public_seed;
         }

      void set_public_seed(secure_vector<uint8_t>&& public_seed)
         {
         m_public_seed = std::move(public_seed);
         }

      const wots_keysig_t& key_data() const { return m_key; }

      wots_keysig_t& key_data() { return m_key; }

      void set_key_data(const wots_keysig_t& key_data)
         {
         m_key = key_data;
         }

      void set_key_data(wots_keysig_t&& key_data)
         {
         m_key = std::move(key_data);
         }

      const XMSS_WOTS_Parameters& wots_parameters() const
         {
         return m_wots_params;
         }

      std::string algo_name() const override
         {
         return m_wots_params.name();
         }

      AlgorithmIdentifier algorithm_identifier() const override
         {
         throw Not_Implemented("No AlgorithmIdentifier available for XMSS-WOTS.");
         }

      bool check_key(RandomNumberGenerator&, bool) const override
         {
         return true;
         }

      size_t estimated_strength() const override
         {
         return m_wots_params.estimated_strength();
         }

      size_t key_length() const override
         {
         return m_wots_params.estimated_strength();
         }

      std::vector<uint8_t> public_key_bits() const override
         {
         throw Not_Implemented("No key format defined for XMSS-WOTS");
         }

      bool operator==(const XMSS_WOTS_PublicKey& key)
         {
         return m_key == key.m_key;
         }

      bool operator!=(const XMSS_WOTS_PublicKey& key)
         {
         return !(*this == key);
         }

   protected:
      /**
       * Algorithm 2: Chaining Function.
       *
       * Takes an n-byte input string and transforms it into a the function
       * result iterating the cryptographic hash function "F" steps times on
       * the input x using the outputs of the PRNG "G".
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each
       * thread.
       *
       * @param[out] x An n-byte input string, that will be transformed into
       *               the chaining function result.
       * @param start_idx The start index.
       * @param steps A number of steps.
       * @param adrs An OTS Hash Address.
       * @param public_seed A public seed.
       * @param hash Instance of XMSS_Hash, that may only by the thead
       *        executing chain.
       **/
      void chain(secure_vector<uint8_t>& x,
                 size_t start_idx,
                 size_t steps,
                 XMSS_Address& adrs,
                 const secure_vector<uint8_t>& public_seed,
                 XMSS_Hash& hash);

      /**
       * Algorithm 2: Chaining Function.
       *
       * Takes an n-byte input string and transforms it into a the function
       * result iterating the cryptographic hash function "F" steps times on
       * the input x using the outputs of the PRNG "G".
       *
       * @param[out] x An n-byte input string, that will be transformed into
       *               the chaining function result.
       * @param start_idx The start index.
       * @param steps A number of steps.
       * @param adrs An OTS Hash Address.
       * @param public_seed A public seed.
       **/
      inline void chain(secure_vector<uint8_t>& x,
                        size_t start_idx,
                        size_t steps,
                        XMSS_Address& adrs,
                        const secure_vector<uint8_t>& public_seed)
         {
         chain(x, start_idx, steps, adrs, public_seed, m_hash);
         }

      XMSS_WOTS_Parameters m_wots_params;
      XMSS_Hash m_hash;
      wots_keysig_t m_key;
      secure_vector<uint8_t> m_public_seed;

   private:
      /**
       * Algorithm 6: "WOTS_pkFromSig"
       * Computes a Winternitz One Time Signature+ public key from a message and
       * its signature.
       *
       * @param msg A message.
       * @param sig The signature for msg.
       * @param adrs An address.
       * @param public_seed A public_seed.
       *
       * @return Temporary WOTS+ public key.
       **/
      wots_keysig_t pub_key_from_signature(
         const secure_vector<uint8_t>& msg,
         const wots_keysig_t& sig,
         XMSS_Address& adrs,
         const secure_vector<uint8_t>& public_seed);
   };

/** A Winternitz One Time Signature private key for use with Extended Hash-Based
 * Signatures.
 **/
class XMSS_WOTS_PrivateKey final : public virtual XMSS_WOTS_PublicKey,
   public virtual Private_Key
   {
   public:
      /**
       * Creates a WOTS private key for the chosen XMSS WOTS signature method.
       * Members need to be initialized manually.
       *
       * @param oid Identifier for the selected signature method.
       **/
      XMSS_WOTS_PrivateKey(XMSS_WOTS_Parameters::ots_algorithm_t oid)
         : XMSS_WOTS_PublicKey(oid)
         {}

      /**
       * Creates a WOTS private key for the chosen XMSS WOTS signature method.
       *
       * @param oid Identifier for the selected signature method.
       * @param rng A random number generator to use for key generation.
       **/
      XMSS_WOTS_PrivateKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                           RandomNumberGenerator& rng)
         : XMSS_WOTS_PublicKey(oid, rng),
           m_private_seed(rng.random_vec(m_wots_params.element_size()))
         {
         set_key_data(generate(m_private_seed));
         }

      /**
       * Constructs a WOTS private key. Chains will be generated on demand
       * applying a hash function to a unique value generated from a secret
       * seed and a counter. The secret seed of length n, will be
       * automatically generated using AutoSeeded_RNG(). "n" equals
       * the element size of the chosen WOTS security parameter set.
       *
       * @param oid Identifier for the selected signature method.
       * @param public_seed A public seed used for the pseudo random generation
       *        of public keys derived from this private key.
       * @param rng A random number generator to use for key generation.
       **/
      XMSS_WOTS_PrivateKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                           const secure_vector<uint8_t>& public_seed,
                           RandomNumberGenerator& rng)
         : XMSS_WOTS_PublicKey(oid, public_seed),
           m_private_seed(rng.random_vec(m_wots_params.element_size()))
         {
         set_key_data(generate(m_private_seed));
         }

      /**
       * Constructs a WOTS private key. Chains will be generated on demand
       * applying a hash function to a unique value generated from a secret
       * seed and a counter. The secret seed of length n, will be
       * automatically generated using AutoSeeded_RNG(). "n" equals
       * the element size of the chosen WOTS security parameter set.
       *
       * @param oid Identifier for the selected signature method.
       * @param public_seed A public seed used for the pseudo random generation
       *        of public keys derived from this private key.
       **/
      XMSS_WOTS_PrivateKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                           const secure_vector<uint8_t>& public_seed)
         : XMSS_WOTS_PublicKey(oid, public_seed)
         {}

      /**
       * Constructs a WOTS private key. Chains will be generated on demand
       * applying a hash function to a unique value generated from the
       * secret seed and a counter.
       *
       * @param oid Identifier for the selected signature method.
       * @param public_seed A public seed used for the pseudo random generation
       *        of public keys derived from this private key.
       * @param private_seed A secret uniformly random n-byte value.
       **/
      XMSS_WOTS_PrivateKey(XMSS_WOTS_Parameters::ots_algorithm_t oid,
                           const secure_vector<uint8_t>& public_seed,
                           const secure_vector<uint8_t>& private_seed)
         : XMSS_WOTS_PublicKey(oid, public_seed),
           m_private_seed(private_seed)
         {
         set_key_data(generate(private_seed));
         }

      /**
       * Retrieves the i-th WOTS private key using pseudo random key
       * (re-)generation.
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each
       * thread.
       *
       * @param i Index of the key to retrieve.
       * @param hash Instance of XMSS_Hash, that may only be used by the
       *        thead executing at.
       *
       * @return WOTS secret key.
       **/
      wots_keysig_t at(size_t i, XMSS_Hash& hash);

      /**
       * Retrieves the i-th WOTS private key using pseudo random key
       * (re-)generation.
       *
       * @param i Index of the key to retrieve.
       *
       * @return WOTS secret key.
       **/
      inline wots_keysig_t operator[](size_t i)
         {
         return this->at(i, m_hash);
         }

      /**
       * Retrieves the i-th WOTS private key using pseudo random key
       * (re-)generation.
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each
       * thread.
       *
       * @param adrs The address of the key to retrieve.
       * @param hash Instance of XMSS_Hash, that may only be used by the
       *        thead executing at.
       *
       * @return WOTS secret key.
       **/
      wots_keysig_t at(const XMSS_Address& adrs, XMSS_Hash& hash);

      inline wots_keysig_t operator[](const XMSS_Address& adrs)
         {
         return this->at(adrs, m_hash);
         }

      wots_keysig_t generate_private_key(const secure_vector<uint8_t>& priv_seed);

      /**
       * Algorithm 4: "WOTS_genPK"
       * Generates a Winternitz One Time Signature+ (WOTS+) Public Key from a
       * given private key.
       *
       * @param adrs Hash function address encoding the address of the WOTS+
       *             key pair within a greater structure.
       *
       * @return A XMSS_WOTS_PublicKey.
       **/
      XMSS_WOTS_PublicKey generate_public_key(XMSS_Address& adrs);

      /**
       * Algorithm 4: "WOTS_genPK"
       * Initializes a Winternitz One Time Signature+ (WOTS+) Public Key's
       * key_data() member, with data derived from in_key_data using the
       * WOTS chaining function.
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each
       * thread.
       *
       * @param[out] pub_key Public key to initialize key_data() member on.
       * @param in_key_data Input key material from private key used for
       *        public key generation.
       * @param adrs Hash function address encoding the address of
       *        the WOTS+ key pair within a greater structure.
       * @param hash Instance of XMSS_Hash, that may only by the thead
       *        executing generate_public_key.
       **/
      void generate_public_key(XMSS_WOTS_PublicKey& pub_key,
                               wots_keysig_t&& in_key_data,
                               XMSS_Address& adrs,
                               XMSS_Hash& hash);
      /**
       * Algorithm 4: "WOTS_genPK"
       * Initializes a Winternitz One Time Signature+ (WOTS+) Public Key's
       * key_data() member, with data derived from in_key_data using the
       * WOTS chaining function.
       *
       * @param[out] pub_key Public key to initialize key_data() member on.
       * @param in_key_data Input key material from private key used for
       *        public key generation.
       * @param adrs Hash function address encoding the address of
       *        the WOTS+ key pair within a greater structure.
       **/
      inline void generate_public_key(XMSS_WOTS_PublicKey& pub_key,
                                      wots_keysig_t&& in_key_data,
                                      XMSS_Address& adrs)
         {
         generate_public_key(pub_key, std::forward<wots_keysig_t>(in_key_data), adrs, m_hash);
         }

      /**
       * Algorithm 5: "WOTS_sign"
       * Generates a signature from a private key and a message.
       *
       * @param msg A message to sign.
       * @param adrs An OTS hash address identifying the WOTS+ key pair
       *        used for signing.
       *
       * @return signature for msg.
       **/
      inline wots_keysig_t sign(const secure_vector<uint8_t>& msg,
                                XMSS_Address& adrs)
         {
         return sign(msg, adrs, m_hash);
         }

      /**
       * Algorithm 5: "WOTS_sign"
       * Generates a signature from a private key and a message.
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each
       * thread.
       *
       * @param msg A message to sign.
       * @param adrs An OTS hash address identifying the WOTS+ key pair
       *        used for signing.
       * @param hash Instance of XMSS_Hash, that may only be used by the
       *        thead executing sign.
       *
       * @return signature for msg.
       **/
      wots_keysig_t sign(const secure_vector<uint8_t>& msg,
                         XMSS_Address& adrs,
                         XMSS_Hash& hash);

      /**
       * Retrieves the secret seed used to generate WOTS+ chains. The seed
       * should be a uniformly random n-byte value.
       *
       * @return secret seed.
       **/
      const secure_vector<uint8_t>& private_seed() const
         {
         return m_private_seed;
         }

      /**
       * Sets the secret seed used to generate WOTS+ chains. The seed
       * should be a uniformly random n-byte value.
       *
       * @param private_seed Uniformly random n-byte value.
       **/
      void set_private_seed(const secure_vector<uint8_t>& private_seed)
         {
         m_private_seed = private_seed;
         }

      /**
       * Sets the secret seed used to generate WOTS+ chains. The seed
       * should be a uniformly random n-byte value.
       *
       * @param private_seed Uniformly random n-byte value.
       **/
      void set_private_seed(secure_vector<uint8_t>&& private_seed)
         {
         m_private_seed = std::move(private_seed);
         }

      AlgorithmIdentifier
      pkcs8_algorithm_identifier() const override
         {
         throw Not_Implemented("No AlgorithmIdentifier available for XMSS-WOTS.");
         }

      secure_vector<uint8_t> private_key_bits() const override
         {
         throw Not_Implemented("No PKCS8 key format defined for XMSS-WOTS.");
         }

   private:
      /**
       * Algorithm 3: "Generating a WOTS+ Private Key".
       * Generates a private key.
       *
       * This overload is used in multithreaded scenarios, where it is
       * required to provide seperate instances of XMSS_Hash to each thread.
       *
       * @param private_seed Uniformly random n-byte value.
       * @param[in] hash Instance of XMSS_Hash, that may only be used by the
       *            thead executing generate.
       *
       * @returns a vector of length key_size() of vectors of n bytes length
       *          containing uniformly random data.
       **/
      wots_keysig_t generate(const secure_vector<uint8_t>& private_seed,
                             XMSS_Hash& hash);

      inline wots_keysig_t generate(const secure_vector<uint8_t>& private_seed)
         {
         return generate(private_seed, m_hash);
         }

      secure_vector<uint8_t> m_private_seed;
   };

}

#endif

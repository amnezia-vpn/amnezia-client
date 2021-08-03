/*
 * XMSS Keys
 * (C) 2016,2017 Matthias Gierlings
 * (C) 2019 René Korthaus, Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_H_
#define BOTAN_XMSS_H_

#include <botan/pk_keys.h>
#include <botan/exceptn.h>
#include <botan/xmss_parameters.h>
#include <botan/xmss_wots.h>

namespace Botan {

class RandomNumberGenerator;
class XMSS_Verification_Operation;

/**
 * An XMSS: Extended Hash-Based Signature public key.
 *
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class BOTAN_PUBLIC_API(2,0) XMSS_PublicKey : public virtual Public_Key
   {
   public:
      /**
       * Creates a new XMSS public key for the chosen XMSS signature method.
       * New public and prf seeds are generated using rng. The appropriate WOTS
       * signature method will be automatically set based on the chosen XMSS
       * signature method.
       *
       * @param xmss_oid Identifier for the selected XMSS signature method.
       * @param rng A random number generator to use for key generation.
       **/
      XMSS_PublicKey(XMSS_Parameters::xmss_algorithm_t xmss_oid,
                     RandomNumberGenerator& rng);

      /**
       * Loads a public key.
       *
       * Public key must be encoded as in RFC
       * draft-vangeest-x509-hash-sigs-03.
       *
       * @param key_bits DER encoded public key bits
       */
      XMSS_PublicKey(const std::vector<uint8_t>& key_bits);

      /**
       * Creates a new XMSS public key for a chosen XMSS signature method as
       * well as pre-computed root node and public_seed values.
       *
       * @param xmss_oid Identifier for the selected XMSS signature method.
       * @param root Root node value.
       * @param public_seed Public seed value.
       **/
      XMSS_PublicKey(XMSS_Parameters::xmss_algorithm_t xmss_oid,
                     const secure_vector<uint8_t>& root,
                     const secure_vector<uint8_t>& public_seed)
         : m_xmss_params(xmss_oid), m_wots_params(m_xmss_params.ots_oid()),
           m_root(root), m_public_seed(public_seed) {}

      /**
       * Creates a new XMSS public key for a chosen XMSS signature method as
       * well as pre-computed root node and public_seed values.
       *
       * @param xmss_oid Identifier for the selected XMSS signature method.
       * @param root Root node value.
       * @param public_seed Public seed value.
       **/
      XMSS_PublicKey(XMSS_Parameters::xmss_algorithm_t xmss_oid,
                     secure_vector<uint8_t>&& root,
                     secure_vector<uint8_t>&& public_seed)
         : m_xmss_params(xmss_oid), m_wots_params(m_xmss_params.ots_oid()),
           m_root(std::move(root)), m_public_seed(std::move(public_seed)) {}

      /**
       * Retrieves the chosen XMSS signature method.
       *
       * @return XMSS signature method identifier.
       **/
      XMSS_Parameters::xmss_algorithm_t xmss_oid() const
         {
         return m_xmss_params.oid();
         }

      /**
       * Sets the chosen XMSS signature method
       **/
      void set_xmss_oid(XMSS_Parameters::xmss_algorithm_t xmss_oid)
         {
         m_xmss_params = XMSS_Parameters(xmss_oid);
         m_wots_params = XMSS_WOTS_Parameters(m_xmss_params.ots_oid());
         }

      /**
       * Retrieves the XMSS parameters determined by the chosen XMSS Signature
       * method.
       *
       * @return XMSS parameters.
       **/
      const XMSS_Parameters& xmss_parameters() const
         {
         return m_xmss_params;
         }

      /**
       * Retrieves the XMSS parameters determined by the chosen XMSS Signature
       * method.
       *
       * @return XMSS parameters.
       **/
      std::string xmss_hash_function() const
         {
         return m_xmss_params.hash_function_name();
         }

      /**
       * Retrieves the Winternitz One Time Signature (WOTS) method,
       * corresponding to the chosen XMSS signature method.
       *
       * @return XMSS WOTS signature method identifier.
       **/
      XMSS_WOTS_Parameters::ots_algorithm_t wots_oid() const
         {
         return m_wots_params.oid();
         }

      /**
       * Retrieves the Winternitz One Time Signature (WOTS) parameters
       * corresponding to the chosen XMSS signature method.
       *
       * @return XMSS WOTS signature method parameters.
       **/
      const XMSS_WOTS_Parameters& wots_parameters() const
         {
         return m_wots_params;
         }

      secure_vector<uint8_t>& root()
         {
         return m_root;
         }

      void set_root(const secure_vector<uint8_t>& root)
         {
         m_root = root;
         }

      void set_root(secure_vector<uint8_t>&& root)
         {
         m_root = std::move(root);
         }

      const secure_vector<uint8_t>& root() const
         {
         return m_root;
         }

      virtual secure_vector<uint8_t>& public_seed()
         {
         return m_public_seed;
         }

      virtual void set_public_seed(const secure_vector<uint8_t>& public_seed)
         {
         m_public_seed = public_seed;
         }

      virtual void set_public_seed(secure_vector<uint8_t>&& public_seed)
         {
         m_public_seed = std::move(public_seed);
         }

      virtual const secure_vector<uint8_t>& public_seed() const
         {
         return m_public_seed;
         }

      std::string algo_name() const override
         {
         return "XMSS";
         }

      AlgorithmIdentifier algorithm_identifier() const override
         {
         return AlgorithmIdentifier(get_oid(), AlgorithmIdentifier::USE_EMPTY_PARAM);
         }

      bool check_key(RandomNumberGenerator&, bool) const override
         {
         return true;
         }

      std::unique_ptr<PK_Ops::Verification>
      create_verification_op(const std::string&,
                             const std::string& provider) const override;

      size_t estimated_strength() const override
         {
         return m_xmss_params.estimated_strength();
         }

      size_t key_length() const override
         {
         return m_xmss_params.estimated_strength();
         }

      /**
       * Returns the encoded public key as defined in RFC
       * draft-vangeest-x509-hash-sigs-03.
       *
       * @return encoded public key bits
       **/
      std::vector<uint8_t> public_key_bits() const override;

      /**
       * Size in bytes of the serialized XMSS public key produced by
       * raw_public_key().
       *
       * @return size in bytes of serialized Public Key.
       **/
      virtual size_t size() const
         {
         return sizeof(uint32_t) + 2 * m_xmss_params.element_size();
         }

      /**
       * Generates a byte sequence representing the XMSS
       * public key, as defined in [1] (p. 23, "XMSS Public Key")
       *
       * @return 4-byte OID, followed by n-byte root node, followed by
       *         public seed.
       **/
      virtual std::vector<uint8_t> raw_public_key() const;

   protected:
      std::vector<uint8_t> m_raw_key;
      XMSS_Parameters m_xmss_params;
      XMSS_WOTS_Parameters m_wots_params;
      secure_vector<uint8_t> m_root;
      secure_vector<uint8_t> m_public_seed;

   private:
      XMSS_Parameters::xmss_algorithm_t deserialize_xmss_oid(
         const std::vector<uint8_t>& raw_key);
   };

template<typename> class Atomic;

class XMSS_Index_Registry;

/**
 * An XMSS: Extended Hash-Based Signature private key.
 * The XMSS private key does not support the X509 and PKCS7 standard. Instead
 * the raw format described in [1] is used.
 *
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class BOTAN_PUBLIC_API(2,0) XMSS_PrivateKey final : public virtual XMSS_PublicKey,
   public virtual Private_Key
   {
   public:
      /**
      * Creates a new XMSS private key for the chosen XMSS signature method.
      * New seeds for public/private key and pseudo random function input are
      * generated using the provided RNG. The appropriate WOTS signature method
      * will be automatically set based on the chosen XMSS signature method.
      *
      * @param xmss_algo_id Identifier for the selected XMSS signature method.
      * @param rng A random number generator to use for key generation.
      **/
      XMSS_PrivateKey(XMSS_Parameters::xmss_algorithm_t xmss_algo_id,
                      RandomNumberGenerator& rng);

      /**
       * Creates an XMSS_PrivateKey from a byte sequence produced by
       * raw_private_key().
       *
       * @param raw_key An XMSS private key serialized using raw_private_key().
       **/
      XMSS_PrivateKey(const secure_vector<uint8_t>& raw_key);

      /**
       * Creates a new XMSS private key for the chosen XMSS signature method
       * using precomputed seeds for public/private keys and pseudo random
       * function input. The appropriate WOTS signature method will be
       * automatically set, based on the chosen XMSS signature method.
       *
       * @param xmss_algo_id Identifier for the selected XMSS signature method.
       * @param idx_leaf Index of the next unused leaf.
       * @param wots_priv_seed A seed to generate a Winternitz-One-Time-
       *                      Signature private key from.
       * @param prf a secret n-byte key sourced from a secure source
       *        of uniformly random data.
       * @param root Root node of the binary hash tree.
       * @param public_seed The public seed.
       **/
      XMSS_PrivateKey(XMSS_Parameters::xmss_algorithm_t xmss_algo_id,
                      size_t idx_leaf,
                      const secure_vector<uint8_t>& wots_priv_seed,
                      const secure_vector<uint8_t>& prf,
                      const secure_vector<uint8_t>& root,
                      const secure_vector<uint8_t>& public_seed);

      bool stateful_operation() const override { return true; }

      /**
       * Retrieves the last unused leaf index of the private key. Reusing a leaf
       * by utilizing leaf indices lower than the last unused leaf index will
       * compromise security.
       *
       * @return Index of the last unused leaf.
       **/
      size_t unused_leaf_index() const;

      /**
       * Sets the last unused leaf index of the private key. The leaf index
       * will be updated automatically during every signing operation, and
       * should not be set manually.
       *
       * @param idx Index of the last unused leaf.
       **/
      void set_unused_leaf_index(size_t idx);

      size_t reserve_unused_leaf_index();

      /**
       * Winternitz One Time Signature Scheme key utilized for signing
       * operations.
       *
       * @return WOTS+ private key.
       **/
      const XMSS_WOTS_PrivateKey& wots_private_key() const
         {
         return m_wots_priv_key;
         }

      /**
       * Winternitz One Time Signature Scheme key utilized for signing
       * operations.
       *
       * @return WOTS+ private key.
       **/
      XMSS_WOTS_PrivateKey& wots_private_key()
         {
         return m_wots_priv_key;
         }

      const secure_vector<uint8_t>& prf() const
         {
         return m_prf;
         }

      secure_vector<uint8_t>& prf()
         {
         return m_prf;
         }

      void set_public_seed(
         const secure_vector<uint8_t>& public_seed) override
         {
         m_public_seed = public_seed;
         m_wots_priv_key.set_public_seed(public_seed);
         }

      void set_public_seed(secure_vector<uint8_t>&& public_seed) override
         {
         m_public_seed = std::move(public_seed);
         m_wots_priv_key.set_public_seed(m_public_seed);
         }

      const secure_vector<uint8_t>& public_seed() const override
         {
         return m_public_seed;
         }

      std::unique_ptr<PK_Ops::Signature>
      create_signature_op(RandomNumberGenerator&,
                          const std::string&,
                          const std::string& provider) const override;

      secure_vector<uint8_t> private_key_bits() const override;

      size_t size() const override
         {
         return XMSS_PublicKey::size() +
                sizeof(uint32_t) +
                2 * XMSS_PublicKey::m_xmss_params.element_size();
         }

      /**
       * Generates a non standartized byte sequence representing the XMSS
       * private key.
       *
       * @return byte sequence consisting of the following elements in order:
       *         4-byte OID, n-byte root node, n-byte public seed,
       *         8-byte unused leaf index, n-byte prf seed, n-byte private seed.
       **/
      secure_vector<uint8_t> raw_private_key() const;
      /**
       * Algorithm 9: "treeHash"
       * Computes the internal n-byte nodes of a Merkle tree.
       *
       * @param start_idx The start index.
       * @param target_node_height Height of the target node.
       * @param adrs Address of the tree containing the target node.
       *
       * @return The root node of a tree of height target_node height with the
       *         leftmost leaf being the hash of the WOTS+ pk with index
       *         start_idx.
       **/
      secure_vector<uint8_t> tree_hash(
         size_t start_idx,
         size_t target_node_height,
         XMSS_Address& adrs);

   private:
      /**
       * Fetches shared unused leaf index from the index registry
       **/
      std::shared_ptr<Atomic<size_t>> recover_global_leaf_index() const;

      inline void tree_hash_subtree(secure_vector<uint8_t>& result,
                                    size_t start_idx,
                                    size_t target_node_height,
                                    XMSS_Address& adrs)
         {
         return tree_hash_subtree(result, start_idx, target_node_height, adrs, m_hash);
         }


      /**
       * Helper for multithreaded tree hashing.
       */
      void tree_hash_subtree(secure_vector<uint8_t>& result,
                             size_t start_idx,
                             size_t target_node_height,
                             XMSS_Address& adrs,
                             XMSS_Hash& hash);

      XMSS_WOTS_PrivateKey m_wots_priv_key;
      XMSS_Hash m_hash;
      secure_vector<uint8_t> m_prf;
      XMSS_Index_Registry& m_index_reg;
   };

}

#endif

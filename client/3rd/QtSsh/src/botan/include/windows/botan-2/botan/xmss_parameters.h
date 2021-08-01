/*
 * XMSS Parameters
 * (C) 2016,2018 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_PARAMETERS_H_
#define BOTAN_XMSS_PARAMETERS_H_

#include <botan/xmss_wots.h>
#include <string>

namespace Botan {

/**
 * Descibes a signature method for XMSS, as defined in:
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class BOTAN_PUBLIC_API(2,0) XMSS_Parameters
   {
   public:
      enum xmss_algorithm_t
         {
         XMSS_SHA2_10_256 = 0x00000001,
         XMSS_SHA2_16_256 = 0x00000002,
         XMSS_SHA2_20_256 = 0x00000003,
         XMSS_SHA2_10_512 = 0x00000004,
         XMSS_SHA2_16_512 = 0x00000005,
         XMSS_SHA2_20_512 = 0x00000006,
         XMSS_SHAKE_10_256 = 0x00000007,
         XMSS_SHAKE_16_256 = 0x00000008,
         XMSS_SHAKE_20_256 = 0x00000009,
         XMSS_SHAKE_10_512 = 0x0000000a,
         XMSS_SHAKE_16_512 = 0x0000000b,
         XMSS_SHAKE_20_512 = 0x0000000c
         };

      static xmss_algorithm_t xmss_id_from_string(const std::string& algo_name);

      XMSS_Parameters(const std::string& algo_name);
      XMSS_Parameters(xmss_algorithm_t oid);

      /**
       * @return XMSS registry name for the chosen parameter set.
       **/
      const std::string& name() const
         {
         return m_name;
         }

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
       * @returns The height (number of levels - 1) of the tree
       **/
      size_t tree_height() const { return m_tree_height; }

      /**
       * The Winternitz parameter.
       *
       * @return numeric base used for internal representation of
       *         data.
       **/
      size_t wots_parameter() const { return m_w; }

      size_t len() const { return m_len; }

      xmss_algorithm_t oid() const { return m_oid; }

      XMSS_WOTS_Parameters::ots_algorithm_t ots_oid() const
         {
         return m_wots_oid;
         }

      /**
       * Returns the estimated pre-quantum security level of
       * the chosen algorithm.
       **/
      size_t estimated_strength() const
         {
         return m_strength;
         }

      bool operator==(const XMSS_Parameters& p) const
         {
         return m_oid == p.m_oid;
         }

   private:
      xmss_algorithm_t m_oid;
      XMSS_WOTS_Parameters::ots_algorithm_t m_wots_oid;
      std::string m_name;
      std::string m_hash_name;
      size_t m_element_size;
      size_t m_tree_height;
      size_t m_w;
      size_t m_len;
      size_t m_strength;
   };

}

#endif

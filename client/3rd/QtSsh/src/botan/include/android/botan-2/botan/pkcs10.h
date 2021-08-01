/*
* PKCS #10
* (C) 1999-2007 Jack Lloyd
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PKCS10_H_
#define BOTAN_PKCS10_H_

#include <botan/x509_obj.h>
#include <botan/pkix_enums.h>
#include <vector>

namespace Botan {

struct PKCS10_Data;

class Private_Key;
class Extensions;
class X509_DN;
class AlternativeName;

/**
* PKCS #10 Certificate Request.
*/
class BOTAN_PUBLIC_API(2,0) PKCS10_Request final : public X509_Object
   {
   public:
      /**
      * Get the subject public key.
      * @return subject public key
      */
      Public_Key* subject_public_key() const;

      /**
      * Get the raw DER encoded public key.
      * @return raw DER encoded public key
      */
      const std::vector<uint8_t>& raw_public_key() const;

      /**
      * Get the subject DN.
      * @return subject DN
      */
      const X509_DN& subject_dn() const;

      /**
      * Get the subject alternative name.
      * @return subject alternative name.
      */
      const AlternativeName& subject_alt_name() const;

      /**
      * Get the key constraints for the key associated with this
      * PKCS#10 object.
      * @return key constraints
      */
      Key_Constraints constraints() const;

      /**
      * Get the extendend key constraints (if any).
      * @return extended key constraints
      */
      std::vector<OID> ex_constraints() const;

      /**
      * Find out whether this is a CA request.
      * @result true if it is a CA request, false otherwise.
      */
      bool is_CA() const;

      /**
      * Return the constraint on the path length defined
      * in the BasicConstraints extension.
      * @return path limit
      */
      size_t path_limit() const;

      /**
      * Get the challenge password for this request
      * @return challenge password for this request
      */
      std::string challenge_password() const;

      /**
      * Get the X509v3 extensions.
      * @return X509v3 extensions
      */
      const Extensions& extensions() const;

      /**
      * Create a PKCS#10 Request from a data source.
      * @param source the data source providing the DER encoded request
      */
      explicit PKCS10_Request(DataSource& source);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
      /**
      * Create a PKCS#10 Request from a file.
      * @param filename the name of the file containing the DER or PEM
      * encoded request file
      */
      explicit PKCS10_Request(const std::string& filename);
#endif

      /**
      * Create a PKCS#10 Request from binary data.
      * @param vec a std::vector containing the DER value
      */
      explicit PKCS10_Request(const std::vector<uint8_t>& vec);

      /**
      * Create a new PKCS10 certificate request
      * @param key the key that will be included in the certificate request
      * @param subject_dn the DN to be placed in the request
      * @param extensions extensions to include in the request
      * @param hash_fn the hash function to use to create the signature
      * @param rng a random number generator
      * @param padding_scheme if set specifies the padding scheme, otherwise an
      *        algorithm-specific default is used.
      * @param challenge a challenge string to be included in the PKCS10 request,
      *        sometimes used for revocation purposes.
      */
      static PKCS10_Request create(const Private_Key& key,
                                   const X509_DN& subject_dn,
                                   const Extensions& extensions,
                                   const std::string& hash_fn,
                                   RandomNumberGenerator& rng,
                                   const std::string& padding_scheme = "",
                                   const std::string& challenge = "");

   private:
      std::string PEM_label() const override;

      std::vector<std::string> alternate_PEM_labels() const override;

      void force_decode() override;

      const PKCS10_Data& data() const;

      std::shared_ptr<PKCS10_Data> m_data;
   };

}

#endif

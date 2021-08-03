/*
* (C) 1999-2010,2012,2018,2020 Jack Lloyd
* (C) 2007 Yves Jerschow
* (C) 2015 Kai Michaelis
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
* (C) 2017 Fabian Weissberg, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PKIX_TYPES_H_
#define BOTAN_PKIX_TYPES_H_

#include <botan/asn1_obj.h>
#include <botan/pkix_enums.h>
#include <vector>
#include <string>
#include <iosfwd>
#include <map>
#include <set>

namespace Botan {

class X509_Certificate;
class Data_Store;
class Public_Key;

/**
* Check that key constraints are permitted for a specific public key.
* @param pub_key the public key on which the constraints shall be enforced on
* @param constraints the constraints that shall be enforced on the key
* @throw Invalid_Argument if the given constraints are not permitted for this key
*/
BOTAN_PUBLIC_API(2,0) void verify_cert_constraints_valid_for_key_type(const Public_Key& pub_key,
                                                                      Key_Constraints constraints);

std::string BOTAN_PUBLIC_API(2,0) key_constraints_to_string(Key_Constraints);

/**
* Distinguished Name
*/
class BOTAN_PUBLIC_API(2,0) X509_DN final : public ASN1_Object
   {
   public:
      X509_DN() = default;

      explicit X509_DN(const std::multimap<OID, std::string>& args)
         {
         for(auto i : args)
            add_attribute(i.first, i.second);
         }

      explicit X509_DN(const std::multimap<std::string, std::string>& args)
         {
         for(auto i : args)
            add_attribute(i.first, i.second);
         }

      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      bool has_field(const OID& oid) const;
      ASN1_String get_first_attribute(const OID& oid) const;

      /*
      * Return the BER encoded data, if any
      */
      const std::vector<uint8_t>& get_bits() const { return m_dn_bits; }

      bool empty() const { return m_rdn.empty(); }

      std::string to_string() const;

      const std::vector<std::pair<OID,ASN1_String>>& dn_info() const { return m_rdn; }

      std::multimap<OID, std::string> get_attributes() const;
      std::multimap<std::string, std::string> contents() const;

      bool has_field(const std::string& attr) const;
      std::vector<std::string> get_attribute(const std::string& attr) const;
      std::string get_first_attribute(const std::string& attr) const;

      void add_attribute(const std::string& key, const std::string& val);

      void add_attribute(const OID& oid, const std::string& val)
         {
         add_attribute(oid, ASN1_String(val));
         }

      void add_attribute(const OID& oid, const ASN1_String& val);

      static std::string deref_info_field(const std::string& key);

      /**
      * Lookup upper bounds in characters for the length of distinguished name fields
      * as given in RFC 5280, Appendix A.
      *
      * @param oid the oid of the DN to lookup
      * @return the upper bound, or zero if no ub is known to Botan
      */
      static size_t lookup_ub(const OID& oid);

   private:
      std::vector<std::pair<OID,ASN1_String>> m_rdn;
      std::vector<uint8_t> m_dn_bits;
   };

bool BOTAN_PUBLIC_API(2,0) operator==(const X509_DN& dn1, const X509_DN& dn2);
bool BOTAN_PUBLIC_API(2,0) operator!=(const X509_DN& dn1, const X509_DN& dn2);

/*
The ordering here is arbitrary and may change from release to release.
It is intended for allowing DNs as keys in std::map and similiar containers
*/
bool BOTAN_PUBLIC_API(2,0) operator<(const X509_DN& dn1, const X509_DN& dn2);

BOTAN_PUBLIC_API(2,0) std::ostream& operator<<(std::ostream& out, const X509_DN& dn);
BOTAN_PUBLIC_API(2,0) std::istream& operator>>(std::istream& in, X509_DN& dn);

/**
* Alternative Name
*/
class BOTAN_PUBLIC_API(2,0) AlternativeName final : public ASN1_Object
   {
   public:
      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      std::multimap<std::string, std::string> contents() const;

      bool has_field(const std::string& attr) const;
      std::vector<std::string> get_attribute(const std::string& attr) const;

      std::string get_first_attribute(const std::string& attr) const;

      void add_attribute(const std::string& type, const std::string& value);
      void add_othername(const OID& oid, const std::string& value, ASN1_Tag type);

      const std::multimap<std::string, std::string>& get_attributes() const
         {
         return m_alt_info;
         }

      const std::multimap<OID, ASN1_String>& get_othernames() const
         {
         return m_othernames;
         }

      X509_DN dn() const;

      bool has_items() const;

      AlternativeName(const std::string& email_addr = "",
                      const std::string& uri = "",
                      const std::string& dns = "",
                      const std::string& ip_address = "");
   private:
      std::multimap<std::string, std::string> m_alt_info;
      std::multimap<OID, ASN1_String> m_othernames;
   };

/**
* Attribute
*/
class BOTAN_PUBLIC_API(2,0) Attribute final : public ASN1_Object
   {
   public:
      void encode_into(DER_Encoder& to) const override;
      void decode_from(BER_Decoder& from) override;

      Attribute() = default;
      Attribute(const OID&, const std::vector<uint8_t>&);
      Attribute(const std::string&, const std::vector<uint8_t>&);

      const OID& get_oid() const { return oid; }

      const std::vector<uint8_t>& get_parameters() const { return parameters; }

   BOTAN_DEPRECATED_PUBLIC_MEMBER_VARIABLES:
      /*
      * These values are public for historical reasons, but in a future release
      * they will be made private. Do not access them.
      */
      OID oid;
      std::vector<uint8_t> parameters;
   };

/**
* @brief X.509 GeneralName Type
*
* Handles parsing GeneralName types in their BER and canonical string
* encoding. Allows matching GeneralNames against each other using
* the rules laid out in the RFC 5280, sec. 4.2.1.10 (Name Contraints).
*/
class BOTAN_PUBLIC_API(2,0) GeneralName final : public ASN1_Object
   {
   public:
      enum MatchResult : int
            {
            All,
            Some,
            None,
            NotFound,
            UnknownType,
            };

      /**
      * Creates an empty GeneralName.
      */
      GeneralName() = default;

      /**
      * Creates a new GeneralName for its string format.
      * @param str type and name, colon-separated, e.g., "DNS:google.com"
      */
      GeneralName(const std::string& str);

      void encode_into(DER_Encoder&) const override;

      void decode_from(BER_Decoder&) override;

      /**
      * @return Type of the name. Can be DN, DNS, IP, RFC822 or URI.
      */
      const std::string& type() const { return m_type; }

      /**
      * @return The name as string. Format depends on type.
      */
      const std::string& name() const { return m_name; }

      /**
      * Checks whether a given certificate (partially) matches this name.
      * @param cert certificate to be matched
      * @return the match result
      */
      MatchResult matches(const X509_Certificate& cert) const;

   private:
      std::string m_type;
      std::string m_name;

      bool matches_dns(const std::string&) const;
      bool matches_dn(const std::string&) const;
      bool matches_ip(const std::string&) const;
   };

std::ostream& operator<<(std::ostream& os, const GeneralName& gn);

/**
* @brief A single Name Constraint
*
* The Name Constraint extension adds a minimum and maximum path
* length to a GeneralName to form a constraint. The length limits
* are currently unused.
*/
class BOTAN_PUBLIC_API(2,0) GeneralSubtree final : public ASN1_Object
   {
   public:
      /**
      * Creates an empty name constraint.
      */
      GeneralSubtree() : m_base(), m_minimum(0), m_maximum(std::numeric_limits<std::size_t>::max())
      {}

      /***
      * Creates a new name constraint.
      * @param base name
      * @param min minimum path length
      * @param max maximum path length
      */
      GeneralSubtree(const GeneralName& base, size_t min, size_t max)
      : m_base(base), m_minimum(min), m_maximum(max)
      {}

      /**
      * Creates a new name constraint for its string format.
      * @param str name constraint
      */
      GeneralSubtree(const std::string& str);

      void encode_into(DER_Encoder&) const override;

      void decode_from(BER_Decoder&) override;

      /**
      * @return name
      */
      const GeneralName& base() const { return m_base; }

      /**
      * @return minimum path length
      */
      size_t minimum() const { return m_minimum; }

      /**
      * @return maximum path length
      */
      size_t maximum() const { return m_maximum; }

   private:
      GeneralName m_base;
      size_t m_minimum;
      size_t m_maximum;
   };

std::ostream& operator<<(std::ostream& os, const GeneralSubtree& gs);

/**
* @brief Name Constraints
*
* Wraps the Name Constraints associated with a certificate.
*/
class BOTAN_PUBLIC_API(2,0) NameConstraints final
   {
   public:
      /**
      * Creates an empty name NameConstraints.
      */
      NameConstraints() : m_permitted_subtrees(), m_excluded_subtrees() {}

      /**
      * Creates NameConstraints from a list of permitted and excluded subtrees.
      * @param permitted_subtrees names for which the certificate is permitted
      * @param excluded_subtrees names for which the certificate is not permitted
      */
      NameConstraints(std::vector<GeneralSubtree>&& permitted_subtrees,
                    std::vector<GeneralSubtree>&& excluded_subtrees)
      : m_permitted_subtrees(permitted_subtrees), m_excluded_subtrees(excluded_subtrees)
      {}

      /**
      * @return permitted names
      */
      const std::vector<GeneralSubtree>& permitted() const { return m_permitted_subtrees; }

      /**
      * @return excluded names
      */
      const std::vector<GeneralSubtree>& excluded() const { return m_excluded_subtrees; }

   private:
      std::vector<GeneralSubtree> m_permitted_subtrees;
      std::vector<GeneralSubtree> m_excluded_subtrees;
   };

/**
* X.509 Certificate Extension
*/
class BOTAN_PUBLIC_API(2,0) Certificate_Extension
   {
   public:
      /**
      * @return OID representing this extension
      */
      virtual OID oid_of() const = 0;

      /*
      * @return specific OID name
      * If possible OIDS table should match oid_name to OIDS, ie
      * OID::from_string(ext->oid_name()) == ext->oid_of()
      * Should return empty string if OID is not known
      */
      virtual std::string oid_name() const = 0;

      /**
      * Make a copy of this extension
      * @return copy of this
      */
      virtual Certificate_Extension* copy() const = 0;

      /*
      * Add the contents of this extension into the information
      * for the subject and/or issuer, as necessary.
      * @param subject the subject info
      * @param issuer the issuer info
      */
      virtual void contents_to(Data_Store& subject,
                               Data_Store& issuer) const = 0;

      /*
      * Callback visited during path validation.
      *
      * An extension can implement this callback to inspect
      * the path during path validation.
      *
      * If an error occurs during validation of this extension,
      * an appropriate status code shall be added to cert_status.
      *
      * @param subject Subject certificate that contains this extension
      * @param issuer Issuer certificate
      * @param status Certificate validation status codes for subject certificate
      * @param cert_path Certificate path which is currently validated
      * @param pos Position of subject certificate in cert_path
      */
      virtual void validate(const X509_Certificate& subject, const X509_Certificate& issuer,
            const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
            std::vector<std::set<Certificate_Status_Code>>& cert_status,
            size_t pos);

      virtual ~Certificate_Extension() = default;
   protected:
      friend class Extensions;
      virtual bool should_encode() const { return true; }
      virtual std::vector<uint8_t> encode_inner() const = 0;
      virtual void decode_inner(const std::vector<uint8_t>&) = 0;
   };

/**
* X.509 Certificate Extension List
*/
class BOTAN_PUBLIC_API(2,0) Extensions final : public ASN1_Object
   {
   public:
      /**
      * Look up an object in the extensions, based on OID Returns
      * nullptr if not set, if the extension was either absent or not
      * handled. The pointer returned is owned by the Extensions
      * object.
      * This would be better with an optional<T> return value
      */
      const Certificate_Extension* get_extension_object(const OID& oid) const;

      template<typename T>
      const T* get_extension_object_as(const OID& oid = T::static_oid()) const
         {
         if(const Certificate_Extension* extn = get_extension_object(oid))
            {
            // Unknown_Extension oid_name is empty
            if(extn->oid_name().empty())
               {
               return nullptr;
               }
            else if(const T* extn_as_T = dynamic_cast<const T*>(extn))
               {
               return extn_as_T;
               }
            else
               {
               throw Decoding_Error("Exception::get_extension_object_as dynamic_cast failed");
               }
            }

         return nullptr;
         }

      /**
      * Return the set of extensions in the order they appeared in the certificate
      * (or as they were added, if constructed)
      */
      const std::vector<OID>& get_extension_oids() const
         {
         return m_extension_oids;
         }

      /**
      * Return true if an extension was set
      */
      bool extension_set(const OID& oid) const;

      /**
      * Return true if an extesion was set and marked critical
      */
      bool critical_extension_set(const OID& oid) const;

      /**
      * Return the raw bytes of the extension
      * Will throw if OID was not set as an extension.
      */
      std::vector<uint8_t> get_extension_bits(const OID& oid) const;

      void encode_into(class DER_Encoder&) const override;
      void decode_from(class BER_Decoder&) override;
      void contents_to(Data_Store&, Data_Store&) const;

      /**
      * Adds a new extension to the list.
      * @param extn pointer to the certificate extension (Extensions takes ownership)
      * @param critical whether this extension should be marked as critical
      * @throw Invalid_Argument if the extension is already present in the list
      */
      void add(Certificate_Extension* extn, bool critical = false);

      /**
      * Adds a new extension to the list unless it already exists. If the extension
      * already exists within the Extensions object, the extn pointer will be deleted.
      *
      * @param extn pointer to the certificate extension (Extensions takes ownership)
      * @param critical whether this extension should be marked as critical
      * @return true if the object was added false if the extension was already used
      */
      bool add_new(Certificate_Extension* extn, bool critical = false);

      /**
      * Adds an extension to the list or replaces it.
      * @param extn the certificate extension
      * @param critical whether this extension should be marked as critical
      */
      void replace(Certificate_Extension* extn, bool critical = false);

      /**
      * Remove an extension from the list. Returns true if the
      * extension had been set, false otherwise.
      */
      bool remove(const OID& oid);

      /**
      * Searches for an extension by OID and returns the result.
      * Only the known extensions types declared in this header
      * are searched for by this function.
      * @return Copy of extension with oid, nullptr if not found.
      * Can avoid creating a copy by using get_extension_object function
      */
      std::unique_ptr<Certificate_Extension> get(const OID& oid) const;

      /**
      * Searches for an extension by OID and returns the result decoding
      * it to some arbitrary extension type chosen by the application.
      *
      * Only the unknown extensions, that is, extensions types that
      * are not declared in this header, are searched for by this
      * function.
      *
      * @return Pointer to new extension with oid, nullptr if not found.
      */
      template<typename T>
      std::unique_ptr<T> get_raw(const OID& oid) const
         {
         auto extn_info = m_extension_info.find(oid);

         if(extn_info != m_extension_info.end())
            {
            // Unknown_Extension oid_name is empty
            if(extn_info->second.obj().oid_name() == "")
               {
               std::unique_ptr<T> ext(new T);
               ext->decode_inner(extn_info->second.bits());
               return ext;
               }
            }
         return nullptr;
         }

      /**
      * Returns a copy of the list of extensions together with the corresponding
      * criticality flag. All extensions are encoded as some object, falling back
      * to Unknown_Extension class which simply allows reading the bytes as well
      * as the criticality flag.
      */
      std::vector<std::pair<std::unique_ptr<Certificate_Extension>, bool>> extensions() const;

      /**
      * Returns the list of extensions as raw, encoded bytes
      * together with the corresponding criticality flag.
      * Contains all extensions, including any extensions encoded as Unknown_Extension
      */
      std::map<OID, std::pair<std::vector<uint8_t>, bool>> extensions_raw() const;

      Extensions() {}

      Extensions(const Extensions&) = default;
      Extensions& operator=(const Extensions&) = default;

      Extensions(Extensions&&) = default;
      Extensions& operator=(Extensions&&) = default;

   private:
      static std::unique_ptr<Certificate_Extension>
         create_extn_obj(const OID& oid,
                         bool critical,
                         const std::vector<uint8_t>& body);

      class Extensions_Info
         {
         public:
            Extensions_Info(bool critical,
                            Certificate_Extension* ext) :
               m_obj(ext),
               m_bits(m_obj->encode_inner()),
               m_critical(critical)
               {
               }

            Extensions_Info(bool critical,
                            const std::vector<uint8_t>& encoding,
                            Certificate_Extension* ext) :
               m_obj(ext),
               m_bits(encoding),
               m_critical(critical)
               {
               }

            bool is_critical() const { return m_critical; }
            const std::vector<uint8_t>& bits() const { return m_bits; }
            const Certificate_Extension& obj() const
               {
               BOTAN_ASSERT_NONNULL(m_obj.get());
               return *m_obj.get();
               }

         private:
            std::shared_ptr<Certificate_Extension> m_obj;
            std::vector<uint8_t> m_bits;
            bool m_critical = false;
         };

      std::vector<OID> m_extension_oids;
      std::map<OID, Extensions_Info> m_extension_info;
   };

}

#endif

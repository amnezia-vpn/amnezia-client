/*
* PKCS#11 Object
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_OBJECT_H_
#define BOTAN_P11_OBJECT_H_

#include <botan/p11.h>
#include <botan/p11_types.h>
#include <botan/secmem.h>

#include <vector>
#include <string>
#include <type_traits>
#include <list>
#include <functional>

namespace Botan {
namespace PKCS11 {

class Module;

/// Helper class to build the Attribute / CK_ATTRIBUTE structures
class BOTAN_PUBLIC_API(2,0) AttributeContainer
   {
   public:
      AttributeContainer() = default;

      /// @param object_class the class type of this container
      AttributeContainer(ObjectClass object_class);

      virtual ~AttributeContainer() = default;

      AttributeContainer(AttributeContainer&& other) = default;
      AttributeContainer& operator=(AttributeContainer&& other) = default;

      // Warning when implementing copy/assignment: m_attributes contains pointers to the other members which must be updated after a copy
      AttributeContainer(const AttributeContainer& other) = delete;
      AttributeContainer& operator=(const AttributeContainer& other) = delete;

      /// @return the attributes this container contains
      inline const std::vector<Attribute>& attributes() const
         {
         return m_attributes;
         }

      /// @return raw attribute data
      inline Attribute* data() const
         {
         return const_cast< Attribute* >(m_attributes.data());
         }

      /// @return the number of attributes in this container
      inline size_t count() const
         {
         return m_attributes.size();
         }

      /**
      * Add a class attribute (CKA_CLASS / AttributeType::Class).
      * @param object_class class attribute to add
      */
      void add_class(ObjectClass object_class);

      /**
      * Add a string attribute (e.g. CKA_LABEL / AttributeType::Label).
      * @param attribute attribute type
      * @param value string value to add
      */
      void add_string(AttributeType attribute, const std::string& value);

      /**
      * Add a binary attribute (e.g. CKA_ID / AttributeType::Id).
      * @param attribute attribute type
      * @param value binary attribute value to add
      * @param length size of the binary attribute value in bytes
      */
      void add_binary(AttributeType attribute, const uint8_t* value, size_t length);

      /**
      * Add a binary attribute (e.g. CKA_ID / AttributeType::Id).
      * @param attribute attribute type
      * @param binary binary attribute value to add
      */
      template<typename TAlloc>
      void add_binary(AttributeType attribute, const std::vector<uint8_t, TAlloc>& binary)
         {
         add_binary(attribute, binary.data(), binary.size());
         }

      /**
      * Add a bool attribute (e.g. CKA_SENSITIVE / AttributeType::Sensitive).
      * @param attribute attribute type
      * @param value boolean value to add
      */
      void add_bool(AttributeType attribute, bool value);

      /**
      * Add a numeric attribute (e.g. CKA_MODULUS_BITS / AttributeType::ModulusBits).
      * @param attribute attribute type
      * @param value numeric value to add
      */
      template<typename T>
      void add_numeric(AttributeType attribute, T value)
         {
         static_assert(std::is_integral<T>::value, "Numeric value required.");
         m_numerics.push_back(static_cast< uint64_t >(value));
         add_attribute(attribute, reinterpret_cast< uint8_t* >(&m_numerics.back()), sizeof(T));
         }

   protected:
      /// Add an attribute with the given value and size to the attribute collection `m_attributes`
      void add_attribute(AttributeType attribute, const uint8_t* value, uint32_t size);

   private:
      std::vector<Attribute> m_attributes;
      std::list<uint64_t> m_numerics;
      std::list<std::string> m_strings;
      std::list<secure_vector<uint8_t>> m_vectors;
   };

/// Manages calls to C_FindObjects* functions (C_FindObjectsInit -> C_FindObjects -> C_FindObjectsFinal)
class BOTAN_PUBLIC_API(2,0) ObjectFinder final
   {
   public:
      /**
      * Initializes a search for token and session objects that match a template (calls C_FindObjectsInit)
      * @param session the session to use for the search
      * @param search_template the search_template as a vector of `Attribute`
      */
      ObjectFinder(Session& session, const std::vector<Attribute>& search_template);

      ObjectFinder(const ObjectFinder& other) = default;
      ObjectFinder& operator=(const ObjectFinder& other) = delete;

      ObjectFinder(ObjectFinder&& other) = default;
      ObjectFinder& operator=(ObjectFinder&& other) = delete;

      /// Terminates a search for token and session objects (calls C_FindObjectsFinal)
      ~ObjectFinder() noexcept;

      /**
      * Starts or continues a search for token and session objects that match a template, obtaining additional object handles (calls C_FindObjects)
      * @param max_count maximum amount of object handles to retrieve. Default = 100
      * @return the result of the search as a vector of `ObjectHandle`
      */
      std::vector<ObjectHandle> find(std::uint32_t max_count = 100) const;

      /// Finishes the search operation manually to allow a new ObjectFinder to exist
      void finish();

      /// @return the module this `ObjectFinder` belongs to
      inline Module& module() const
         {
         return m_session.get().module();
         }

   private:
      const std::reference_wrapper<Session> m_session;
      bool m_search_terminated;
   };

/// Common attributes of all objects
class BOTAN_PUBLIC_API(2,0) ObjectProperties : public AttributeContainer
   {
   public:
      /// @param object_class the object class of the object
      ObjectProperties(ObjectClass object_class);

      /// @return the object class of this object
      inline ObjectClass object_class() const
         {
         return m_object_class;
         }

   private:
      const ObjectClass m_object_class;
   };

/// Common attributes of all storage objects
class BOTAN_PUBLIC_API(2,0) StorageObjectProperties : public ObjectProperties
   {
   public:
      /// @param object_class the CK_OBJECT_CLASS this storage object belongs to
      StorageObjectProperties(ObjectClass object_class);

      /// @param label description of the object (RFC2279 string)
      inline void set_label(const std::string& label)
         {
         add_string(AttributeType::Label, label);
         }

      /// @param value if true the object is a token object; otherwise the object is a session object
      inline void set_token(bool value)
         {
         add_bool(AttributeType::Token, value);
         }

      /**
      * @param value if true the object is a private object; otherwise the object is a public object
      * When private, a user may not access the object until the user has been authenticated to the token
      */
      inline void set_private(bool value)
         {
         add_bool(AttributeType::Private, value);
         }

      /// @param value if true the object can be modified, otherwise it is read-only
      void set_modifiable(bool value)
         {
         add_bool(AttributeType::Modifiable, value);
         }

      /// @param value if true the object can be copied using C_CopyObject
      void set_copyable(bool value)
         {
         add_bool(AttributeType::Copyable, value);
         }

      /// @param value if true the object can be destroyed using C_DestroyObject
      void set_destroyable(bool value)
         {
         add_bool(AttributeType::Destroyable, value);
         }
   };

/// Common attributes of all data objects
class BOTAN_PUBLIC_API(2,0) DataObjectProperties final : public StorageObjectProperties
   {
   public:
      DataObjectProperties();

      /// @param value description of the application that manages the object (RFC2279 string)
      inline void set_application(const std::string& value)
         {
         add_string(AttributeType::Application, value);
         }

      /// @param object_id DER-encoding of the object identifier indicating the data object type
      inline void set_object_id(const std::vector<uint8_t>& object_id)
         {
         add_binary(AttributeType::ObjectId, object_id);
         }

      /// @param value value of the object
      inline void set_value(const secure_vector<uint8_t>& value)
         {
         add_binary(AttributeType::Value, value);
         }
   };

/// Common attributes of all certificate objects
class BOTAN_PUBLIC_API(2,0) CertificateProperties : public StorageObjectProperties
   {
   public:
      /// @param cert_type type of certificate
      CertificateProperties(CertificateType cert_type);

      /// @param value the certificate can be trusted for the application that it was created (can only be set to true by SO user)
      inline void set_trusted(bool value)
         {
         add_bool(AttributeType::Trusted, value);
         }

      /// @param category one of `CertificateCategory`
      inline void set_category(CertificateCategory category)
         {
         add_numeric(AttributeType::CertificateCategory, static_cast< CK_CERTIFICATE_CATEGORY >(category));
         }

      /**
      * @param checksum the value of this attribute is derived from the certificate by taking the
      * first three bytes of the SHA - 1 hash of the certificate object's `CKA_VALUE` attribute
      */
      inline void set_check_value(const std::vector<uint8_t>& checksum)
         {
         add_binary(AttributeType::CheckValue, checksum);
         }

      /// @param date start date for the certificate
      inline void set_start_date(Date date)
         {
         add_binary(AttributeType::StartDate, reinterpret_cast<uint8_t*>(&date), sizeof(Date));
         }

      /// @param date end date for the certificate
      inline void set_end_date(Date date)
         {
         add_binary(AttributeType::EndDate, reinterpret_cast<uint8_t*>(&date), sizeof(Date));
         }

      /// @param pubkey_info DER-encoding of the SubjectPublicKeyInfo for the public key contained in this certificate
      inline void set_public_key_info(const std::vector<uint8_t>& pubkey_info)
         {
         add_binary(AttributeType::PublicKeyInfo, pubkey_info);
         }

      /// @return the certificate type of this certificate object
      inline CertificateType cert_type() const
         {
         return m_cert_type;
         }

   private:
      const CertificateType m_cert_type;
   };

/// Common attributes of all key objects
class BOTAN_PUBLIC_API(2,0) KeyProperties : public StorageObjectProperties
   {
   public:
      /**
      * @param object_class the `CK_OBJECT_CLASS` this key object belongs to
      * @param key_type type of key
      */
      KeyProperties(ObjectClass object_class, KeyType key_type);

      /// @param id key identifier for key
      inline void set_id(const std::vector<uint8_t>& id)
         {
         add_binary(AttributeType::Id, id);
         }

      /// @param date start date for the key
      inline void set_start_date(Date date)
         {
         add_binary(AttributeType::StartDate, reinterpret_cast<uint8_t*>(&date), sizeof(Date));
         }

      /// @param date end date for the key
      inline void set_end_date(Date date)
         {
         add_binary(AttributeType::EndDate, reinterpret_cast<uint8_t*>(&date), sizeof(Date));
         }

      /// @param value true if key supports key derivation (i.e., if other keys can be derived from this one)
      inline void set_derive(bool value)
         {
         add_bool(AttributeType::Derive, value);
         }

      /**
      * Sets a list of mechanisms allowed to be used with this key
      * Not implemented
      */
      inline void set_allowed_mechanisms(const std::vector<MechanismType>&)
         {
         throw Not_Implemented("KeyProperties::set_allowed_mechanisms");
         }

      /// @return the key type of this key object
      inline KeyType key_type() const
         {
         return m_key_type;
         }

   private:
      const KeyType m_key_type;
   };

/// Common attributes of all public key objects
class BOTAN_PUBLIC_API(2,0) PublicKeyProperties : public KeyProperties
   {
   public:
      /// @param key_type type of key
      PublicKeyProperties(KeyType key_type);

      /// @param subject DER-encoding of the key subject name
      inline void set_subject(const std::vector<uint8_t>& subject)
         {
         add_binary(AttributeType::Subject, subject);
         }

      /// @param value true if the key supports encryption
      inline void set_encrypt(bool value)
         {
         add_bool(AttributeType::Encrypt, value);
         }

      /// @param value true if the key supports verification where the signature is an appendix to the data
      inline void set_verify(bool value)
         {
         add_bool(AttributeType::Verify, value);
         }

      /// @param value true if the key supports verification where the data is recovered from the signature
      inline void set_verify_recover(bool value)
         {
         add_bool(AttributeType::VerifyRecover, value);
         }

      /// @param value true if the key supports wrapping (i.e., can be used to wrap other keys)
      inline void set_wrap(bool value)
         {
         add_bool(AttributeType::Wrap, value);
         }

      /**
      * @param value true if the key can be trusted for the application that it was created.
      * The wrapping key can be used to wrap keys with `CKA_WRAP_WITH_TRUSTED` set to `CK_TRUE`
      */
      inline void set_trusted(bool value)
         {
         add_bool(AttributeType::Trusted, value);
         }

      /**
      * For wrapping keys
      * The attribute template to match against any keys wrapped using this wrapping key.
      * Keys that do not match cannot be wrapped
      * Not implemented
      */
      inline void set_wrap_template(const AttributeContainer&)
         {
         throw Not_Implemented("PublicKeyProperties::set_wrap_template");
         }

      /// @param pubkey_info DER-encoding of the SubjectPublicKeyInfo for this public key
      inline void set_public_key_info(const std::vector<uint8_t>& pubkey_info)
         {
         add_binary(AttributeType::PublicKeyInfo, pubkey_info);
         }
   };

/// Common attributes of all private keys
class BOTAN_PUBLIC_API(2,0) PrivateKeyProperties : public KeyProperties
   {
   public:
      /// @param key_type type of key
      PrivateKeyProperties(KeyType key_type);

      /// @param subject DER-encoding of the key subject name
      inline void set_subject(const std::vector<uint8_t>& subject)
         {
         add_binary(AttributeType::Subject, subject);
         }

      /// @param value true if the key is sensitive
      inline void set_sensitive(bool value)
         {
         add_bool(AttributeType::Sensitive, value);
         }

      /// @param value true if the key supports decryption
      inline void set_decrypt(bool value)
         {
         add_bool(AttributeType::Decrypt, value);
         }

      /// @param value true if the key supports signatures where the signature is an appendix to the data
      inline void set_sign(bool value)
         {
         add_bool(AttributeType::Sign, value);
         }

      /// @param value true if the key supports signatures where the data can be recovered from the signature
      inline void set_sign_recover(bool value)
         {
         add_bool(AttributeType::SignRecover, value);
         }

      /// @param value true if the key supports unwrapping (i.e., can be used to unwrap other keys)
      inline void set_unwrap(bool value)
         {
         add_bool(AttributeType::Unwrap, value);
         }

      /// @param value true if the key is extractable and can be wrapped
      inline void set_extractable(bool value)
         {
         add_bool(AttributeType::Extractable, value);
         }

      /// @param value true if the key can only be wrapped with a wrapping key that has `CKA_TRUSTED` set to `CK_TRUE`
      inline void set_wrap_with_trusted(bool value)
         {
         add_bool(AttributeType::WrapWithTrusted, value);
         }

      /// @param value If true, the user has to supply the PIN for each use (sign or decrypt) with the key
      inline void set_always_authenticate(bool value)
         {
         add_bool(AttributeType::AlwaysAuthenticate, value);
         }

      /**
      * For wrapping keys
      * The attribute template to apply to any keys unwrapped using this wrapping key.
      * Any user supplied template is applied after this template as if the object has already been created
      * Not implemented
      */
      inline void set_unwrap_template(const AttributeContainer&)
         {
         throw Not_Implemented("PrivateKeyProperties::set_unwrap_template");
         }

      /// @param pubkey_info DER-encoding of the SubjectPublicKeyInfo for this public key
      inline void set_public_key_info(const std::vector<uint8_t>& pubkey_info)
         {
         add_binary(AttributeType::PublicKeyInfo, pubkey_info);
         }
   };

/// Common attributes of all secret (symmetric) keys
class BOTAN_PUBLIC_API(2,0) SecretKeyProperties final : public KeyProperties
   {
   public:
      /// @param key_type type of key
      SecretKeyProperties(KeyType key_type);

      /// @param value true if the key is sensitive
      inline void set_sensitive(bool value)
         {
         add_bool(AttributeType::Sensitive, value);
         }

      /// @param value true if the key supports encryption
      inline void set_encrypt(bool value)
         {
         add_bool(AttributeType::Encrypt, value);
         }

      /// @param value true if the key supports decryption
      inline void set_decrypt(bool value)
         {
         add_bool(AttributeType::Decrypt, value);
         }

      /// @param value true if the key supports signatures where the signature is an appendix to the data
      inline void set_sign(bool value)
         {
         add_bool(AttributeType::Sign, value);
         }

      /// @param value true if the key supports verification where the signature is an appendix to the data
      inline void set_verify(bool value)
         {
         add_bool(AttributeType::Verify, value);
         }

      /// @param value true if the key supports unwrapping (i.e., can be used to unwrap other keys)
      inline void set_unwrap(bool value)
         {
         add_bool(AttributeType::Unwrap, value);
         }

      /// @param value true if the key is extractable and can be wrapped
      inline void set_extractable(bool value)
         {
         add_bool(AttributeType::Extractable, value);
         }

      /// @param value true if the key can only be wrapped with a wrapping key that has `CKA_TRUSTED` set to `CK_TRUE`
      inline void set_wrap_with_trusted(bool value)
         {
         add_bool(AttributeType::WrapWithTrusted, value);
         }

      /// @param value if true, the user has to supply the PIN for each use (sign or decrypt) with the key
      inline void set_always_authenticate(bool value)
         {
         add_bool(AttributeType::AlwaysAuthenticate, value);
         }

      /// @param value true if the key supports wrapping (i.e., can be used to wrap other keys)
      inline void set_wrap(bool value)
         {
         add_bool(AttributeType::Wrap, value);
         }

      /**
      * @param value the key can be trusted for the application that it was created.
      * The wrapping key can be used to wrap keys with `CKA_WRAP_WITH_TRUSTED` set to `CK_TRUE`
      */
      inline void set_trusted(bool value)
         {
         add_bool(AttributeType::Trusted, value);
         }

      /// @param checksum the key check value of this key
      inline void set_check_value(const std::vector<uint8_t>& checksum)
         {
         add_binary(AttributeType::CheckValue, checksum);
         }

      /**
      * For wrapping keys
      * The attribute template to match against any keys wrapped using this wrapping key.
      * Keys that do not match cannot be wrapped
      * Not implemented
      */
      inline void set_wrap_template(const AttributeContainer&)
         {
         throw Not_Implemented("SecretKeyProperties::set_wrap_template");
         }

      /**
      * For wrapping keys
      * The attribute template to apply to any keys unwrapped using this wrapping key
      * Any user supplied template is applied after this template as if the object has already been created
      * Not Implemented
      */
      inline void set_unwrap_template(const AttributeContainer&)
         {
         throw Not_Implemented("SecretKeyProperties::set_unwrap_template");
         }
   };

/// Common attributes of domain parameter
class BOTAN_PUBLIC_API(2,0) DomainParameterProperties final : public StorageObjectProperties
   {
   public:
      /// @param key_type type of key the domain parameters can be used to generate
      DomainParameterProperties(KeyType key_type);

      /// @return the key type
      inline KeyType key_type() const
         {
         return m_key_type;
         }

   private:
      const KeyType m_key_type;
   };

/**
* Represents a PKCS#11 object.
*/
class BOTAN_PUBLIC_API(2,0) Object
   {
   public:
      /**
      * Creates an `Object` from an existing PKCS#11 object
      * @param session the session the object belongs to
      * @param handle handle of the object
      */

      Object(Session& session, ObjectHandle handle);

      /**
      * Creates the object
      * @param session the session in which the object should be created
      * @param obj_props properties of this object
      */
      Object(Session& session, const ObjectProperties& obj_props);

      Object(const Object&) = default;
      Object& operator=(const Object&) = delete;
      virtual ~Object() = default;

      /// Searches for all objects of the given type that match `search_template`
      template<typename T>
      static std::vector<T> search(Session& session, const std::vector<Attribute>& search_template);

      /// Searches for all objects of the given type using the label (`CKA_LABEL`)
      template<typename T>
      static std::vector<T> search(Session& session, const std::string& label);

      /// Searches for all objects of the given type using the id (`CKA_ID`)
      template<typename T>
      static std::vector<T> search(Session& session, const std::vector<uint8_t>& id);

      /// Searches for all objects of the given type using the label (`CKA_LABEL`) and id (`CKA_ID`)
      template<typename T>
      static std::vector<T> search(Session& session, const std::string& label, const std::vector<uint8_t>& id);

      /// Searches for all objects of the given type
      template<typename T>
      static std::vector<T> search(Session& session);

      /// @returns the value of the given attribute (using `C_GetAttributeValue`)
      secure_vector<uint8_t> get_attribute_value(AttributeType attribute) const;

      /// Sets the given value for the attribute (using `C_SetAttributeValue`)
      void set_attribute_value(AttributeType attribute, const secure_vector<uint8_t>& value) const;

      /// Destroys the object
      void destroy() const;

      /**
      * Copies the object
      * @param modified_attributes the attributes of the copied object
      */
      ObjectHandle copy(const AttributeContainer& modified_attributes) const;

      /// @return the handle of this object.
      inline ObjectHandle handle() const
         {
         return m_handle;
         }

      /// @return the session this objects belongs to
      inline Session& session() const
         {
         return m_session;
         }

      /// @return the module this object belongs to
      inline Module& module() const
         {
         return m_session.get().module();
         }
   protected:
      Object(Session& session)
         : m_session(session)
         {}

      void reset_handle(ObjectHandle handle)
         {
         if(m_handle != CK_INVALID_HANDLE)
            throw Invalid_Argument("Cannot reset handle on already valid PKCS11 object");
         m_handle = handle;
         }

   private:
      const std::reference_wrapper<Session> m_session;
      ObjectHandle m_handle = CK_INVALID_HANDLE;
   };

template<typename T>
std::vector<T> Object::search(Session& session, const std::vector<Attribute>& search_template)
   {
   ObjectFinder finder(session, search_template);
   std::vector<ObjectHandle> handles = finder.find();
   std::vector<T> result;
   result.reserve(handles.size());
   for(const auto& handle : handles)
      {
      result.emplace_back(T(session, handle));
      }
   return result;
   }

template<typename T>
std::vector<T> Object::search(Session& session, const std::string& label)
   {
   AttributeContainer search_template(T::Class);
   search_template.add_string(AttributeType::Label, label);
   return search<T>(session, search_template.attributes());
   }

template<typename T>
std::vector<T> Object::search(Session& session, const std::vector<uint8_t>& id)
   {
   AttributeContainer search_template(T::Class);
   search_template.add_binary(AttributeType::Id, id);
   return search<T>(session, search_template.attributes());
   }

template<typename T>
std::vector<T> Object::search(Session& session, const std::string& label, const std::vector<uint8_t>& id)
   {
   AttributeContainer search_template(T::Class);
   search_template.add_string(AttributeType::Label, label);
   search_template.add_binary(AttributeType::Id, id);
   return search<T>(session, search_template.attributes());
   }

template<typename T>
std::vector<T> Object::search(Session& session)
   {
   return search<T>(session, AttributeContainer(T::Class).attributes());
   }

}

}

#endif

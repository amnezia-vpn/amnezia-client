/*
* (C) 1999-2007,2018,2020 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ASN1_OBJECT_TYPES_H_
#define BOTAN_ASN1_OBJECT_TYPES_H_

#include <botan/secmem.h>
#include <botan/exceptn.h>
#include <vector>
#include <string>
#include <chrono>

namespace Botan {

class BER_Decoder;
class DER_Encoder;

/**
* ASN.1 Type and Class Tags
* This will become an enum class in a future major release
*/
enum ASN1_Tag : uint32_t {
   UNIVERSAL        = 0x00,
   APPLICATION      = 0x40,
   CONTEXT_SPECIFIC = 0x80,

   CONSTRUCTED      = 0x20,

   PRIVATE          = CONSTRUCTED | CONTEXT_SPECIFIC,

   EOC              = 0x00,
   BOOLEAN          = 0x01,
   INTEGER          = 0x02,
   BIT_STRING       = 0x03,
   OCTET_STRING     = 0x04,
   NULL_TAG         = 0x05,
   OBJECT_ID        = 0x06,
   ENUMERATED       = 0x0A,
   SEQUENCE         = 0x10,
   SET              = 0x11,

   UTF8_STRING      = 0x0C,
   NUMERIC_STRING   = 0x12,
   PRINTABLE_STRING = 0x13,
   T61_STRING       = 0x14,
   IA5_STRING       = 0x16,
   VISIBLE_STRING   = 0x1A,
   UNIVERSAL_STRING = 0x1C,
   BMP_STRING       = 0x1E,

   UTC_TIME                = 0x17,
   GENERALIZED_TIME        = 0x18,
   UTC_OR_GENERALIZED_TIME = 0x19,

   NO_OBJECT        = 0xFF00,
   DIRECTORY_STRING = 0xFF01
};

std::string BOTAN_UNSTABLE_API asn1_tag_to_string(ASN1_Tag type);
std::string BOTAN_UNSTABLE_API asn1_class_to_string(ASN1_Tag type);

/**
* Basic ASN.1 Object Interface
*/
class BOTAN_PUBLIC_API(2,0) ASN1_Object
   {
   public:
      /**
      * Encode whatever this object is into to
      * @param to the DER_Encoder that will be written to
      */
      virtual void encode_into(DER_Encoder& to) const = 0;

      /**
      * Decode whatever this object is from from
      * @param from the BER_Decoder that will be read from
      */
      virtual void decode_from(BER_Decoder& from) = 0;

      /**
      * Return the encoding of this object. This is a convenience
      * method when just one object needs to be serialized. Use
      * DER_Encoder for complicated encodings.
      */
      std::vector<uint8_t> BER_encode() const;

      ASN1_Object() = default;
      ASN1_Object(const ASN1_Object&) = default;
      ASN1_Object & operator=(const ASN1_Object&) = default;
      virtual ~ASN1_Object() = default;
   };

/**
* BER Encoded Object
*/
class BOTAN_PUBLIC_API(2,0) BER_Object final
   {
   public:
      BER_Object() : type_tag(NO_OBJECT), class_tag(UNIVERSAL) {}

      BER_Object(const BER_Object& other) = default;

      BER_Object& operator=(const BER_Object& other) = default;

      BER_Object(BER_Object&& other) = default;

      BER_Object& operator=(BER_Object&& other) = default;

      bool is_set() const { return type_tag != NO_OBJECT; }

      ASN1_Tag tagging() const { return ASN1_Tag(type() | get_class()); }

      ASN1_Tag type() const { return type_tag; }
      ASN1_Tag get_class() const { return class_tag; }

      const uint8_t* bits() const { return value.data(); }

      size_t length() const { return value.size(); }

      void assert_is_a(ASN1_Tag type_tag, ASN1_Tag class_tag,
                       const std::string& descr = "object") const;

      bool is_a(ASN1_Tag type_tag, ASN1_Tag class_tag) const;

      bool is_a(int type_tag, ASN1_Tag class_tag) const;

   BOTAN_DEPRECATED_PUBLIC_MEMBER_VARIABLES:
      /*
      * The following member variables are public for historical reasons, but
      * will be made private in a future major release. Use the accessor
      * functions above.
      */
      ASN1_Tag type_tag, class_tag;
      secure_vector<uint8_t> value;

   private:

      friend class BER_Decoder;

      void set_tagging(ASN1_Tag type_tag, ASN1_Tag class_tag);

      uint8_t* mutable_bits(size_t length)
         {
         value.resize(length);
         return value.data();
         }
   };

/*
* ASN.1 Utility Functions
*/
class DataSource;

namespace ASN1 {

std::vector<uint8_t> put_in_sequence(const std::vector<uint8_t>& val);
std::vector<uint8_t> put_in_sequence(const uint8_t bits[], size_t len);
std::string to_string(const BER_Object& obj);

/**
* Heuristics tests; is this object possibly BER?
* @param src a data source that will be peeked at but not modified
*/
bool maybe_BER(DataSource& src);

}

/**
* General BER Decoding Error Exception
*/
class BOTAN_PUBLIC_API(2,0) BER_Decoding_Error : public Decoding_Error
   {
   public:
      explicit BER_Decoding_Error(const std::string&);
   };

/**
* Exception For Incorrect BER Taggings
*/
class BOTAN_PUBLIC_API(2,0) BER_Bad_Tag final : public BER_Decoding_Error
   {
   public:
      BER_Bad_Tag(const std::string& msg, ASN1_Tag tag);
      BER_Bad_Tag(const std::string& msg, ASN1_Tag tag1, ASN1_Tag tag2);
   };

/**
* This class represents ASN.1 object identifiers.
*/
class BOTAN_PUBLIC_API(2,0) OID final : public ASN1_Object
   {
   public:

      /**
      * Create an uninitialied OID object
      */
      explicit OID() {}

      /**
      * Construct an OID from a string.
      * @param str a string in the form "a.b.c" etc., where a,b,c are numbers
      */
      explicit OID(const std::string& str);

      /**
      * Initialize an OID from a sequence of integer values
      */
      explicit OID(std::initializer_list<uint32_t> init) : m_id(init) {}

      /**
      * Initialize an OID from a vector of integer values
      */
      explicit OID(std::vector<uint32_t>&& init) : m_id(init) {}

      /**
      * Construct an OID from a string.
      * @param str a string in the form "a.b.c" etc., where a,b,c are numbers
      *        or any known OID name (for example "RSA" or "X509v3.SubjectKeyIdentifier")
      */
      static OID from_string(const std::string& str);

      void encode_into(class DER_Encoder&) const override;
      void decode_from(class BER_Decoder&) override;

      /**
      * Find out whether this OID is empty
      * @return true is no OID value is set
      */
      bool empty() const { return m_id.empty(); }

      /**
      * Find out whether this OID has a value
      * @return true is this OID has a value
      */
      bool has_value() const { return (m_id.empty() == false); }

      /**
      * Get this OID as list (vector) of its components.
      * @return vector representing this OID
      */
      const std::vector<uint32_t>& get_components() const { return m_id; }

      const std::vector<uint32_t>& get_id() const { return get_components(); }

      /**
      * Get this OID as a string
      * @return string representing this OID
      */
      std::string BOTAN_DEPRECATED("Use OID::to_string") as_string() const
         {
         return this->to_string();
         }

      /**
      * Get this OID as a dotted-decimal string
      * @return string representing this OID
      */
      std::string to_string() const;

      /**
      * If there is a known name associated with this OID, return that.
      * Otherwise return the result of to_string
      */
      std::string to_formatted_string() const;

      /**
      * Compare two OIDs.
      * @return true if they are equal, false otherwise
      */
      bool operator==(const OID& other) const
         {
         return m_id == other.m_id;
         }

      /**
      * Reset this instance to an empty OID.
      */
      void BOTAN_DEPRECATED("Avoid mutation of OIDs") clear() { m_id.clear(); }

      /**
      * Add a component to this OID.
      * @param new_comp the new component to add to the end of this OID
      * @return reference to *this
      */
      BOTAN_DEPRECATED("Avoid mutation of OIDs") OID& operator+=(uint32_t new_comp)
         {
         m_id.push_back(new_comp);
         return (*this);
         }

   private:
      std::vector<uint32_t> m_id;
   };

/**
* Append another component onto the OID.
* @param oid the OID to add the new component to
* @param new_comp the new component to add
*/
OID BOTAN_PUBLIC_API(2,0) operator+(const OID& oid, uint32_t new_comp);

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is not equal to b
*/
inline bool operator!=(const OID& a, const OID& b)
   {
   return !(a == b);
   }

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is lexicographically smaller than b
*/
bool BOTAN_PUBLIC_API(2,0) operator<(const OID& a, const OID& b);

/**
* Time (GeneralizedTime/UniversalTime)
*/
class BOTAN_PUBLIC_API(2,0) ASN1_Time final : public ASN1_Object
   {
   public:
      /// DER encode a ASN1_Time
      void encode_into(DER_Encoder&) const override;

      // Decode a BER encoded ASN1_Time
      void decode_from(BER_Decoder&) override;

      /// Return an internal string representation of the time
      std::string to_string() const;

      /// Returns a human friendly string replesentation of no particular formatting
      std::string readable_string() const;

      /// Return if the time has been set somehow
      bool time_is_set() const;

      ///  Compare this time against another
      int32_t cmp(const ASN1_Time& other) const;

      /// Create an invalid ASN1_Time
      ASN1_Time() = default;

      /// Create a ASN1_Time from a time point
      explicit ASN1_Time(const std::chrono::system_clock::time_point& time);

      /// Create an ASN1_Time from string
      ASN1_Time(const std::string& t_spec, ASN1_Tag tag);

      /// Returns a STL timepoint object
      std::chrono::system_clock::time_point to_std_timepoint() const;

      /// Return time since epoch
      uint64_t time_since_epoch() const;

   private:
      void set_to(const std::string& t_spec, ASN1_Tag);
      bool passes_sanity_check() const;

      uint32_t m_year = 0;
      uint32_t m_month = 0;
      uint32_t m_day = 0;
      uint32_t m_hour = 0;
      uint32_t m_minute = 0;
      uint32_t m_second = 0;
      ASN1_Tag m_tag = NO_OBJECT;
   };

/*
* Comparison Operations
*/
bool BOTAN_PUBLIC_API(2,0) operator==(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2,0) operator!=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2,0) operator<=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2,0) operator>=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2,0) operator<(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2,0) operator>(const ASN1_Time&, const ASN1_Time&);

typedef ASN1_Time X509_Time;

/**
* ASN.1 string type
* This class normalizes all inputs to a UTF-8 std::string
*/
class BOTAN_PUBLIC_API(2,0) ASN1_String final : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const override;
      void decode_from(class BER_Decoder&) override;

      ASN1_Tag tagging() const { return m_tag; }

      const std::string& value() const { return m_utf8_str; }

      size_t size() const { return value().size(); }

      bool empty() const { return m_utf8_str.empty(); }

      std::string BOTAN_DEPRECATED("Use value() to get UTF-8 string instead")
         iso_8859() const;

      /**
      * Return true iff this is a tag for a known string type we can handle.
      * This ignores string types that are not supported, eg teletexString
      */
      static bool is_string_type(ASN1_Tag tag);

      bool operator==(const ASN1_String& other) const
         { return value() == other.value(); }

      explicit ASN1_String(const std::string& utf8 = "");
      ASN1_String(const std::string& utf8, ASN1_Tag tag);
   private:
      std::vector<uint8_t> m_data;
      std::string m_utf8_str;
      ASN1_Tag m_tag;
   };

/**
* Algorithm Identifier
*/
class BOTAN_PUBLIC_API(2,0) AlgorithmIdentifier final : public ASN1_Object
   {
   public:
      enum Encoding_Option { USE_NULL_PARAM, USE_EMPTY_PARAM };

      void encode_into(class DER_Encoder&) const override;
      void decode_from(class BER_Decoder&) override;

      AlgorithmIdentifier() = default;

      AlgorithmIdentifier(const OID& oid, Encoding_Option enc);
      AlgorithmIdentifier(const std::string& oid_name, Encoding_Option enc);

      AlgorithmIdentifier(const OID& oid, const std::vector<uint8_t>& params);
      AlgorithmIdentifier(const std::string& oid_name, const std::vector<uint8_t>& params);

      const OID& get_oid() const { return oid; }
      const std::vector<uint8_t>& get_parameters() const { return parameters; }

      bool parameters_are_null() const;
      bool parameters_are_empty() const { return parameters.empty(); }

      bool parameters_are_null_or_empty() const
         {
         return parameters_are_empty() || parameters_are_null();
         }

   BOTAN_DEPRECATED_PUBLIC_MEMBER_VARIABLES:
      /*
      * These values are public for historical reasons, but in a future release
      * they will be made private. Do not access them.
      */
      OID oid;
      std::vector<uint8_t> parameters;
   };

/*
* Comparison Operations
*/
bool BOTAN_PUBLIC_API(2,0) operator==(const AlgorithmIdentifier&,
                                      const AlgorithmIdentifier&);
bool BOTAN_PUBLIC_API(2,0) operator!=(const AlgorithmIdentifier&,
                                      const AlgorithmIdentifier&);

}

#endif

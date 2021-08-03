/*
* BER Decoder
* (C) 1999-2010,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BER_DECODER_H_
#define BOTAN_BER_DECODER_H_

#include <botan/asn1_obj.h>
#include <botan/data_src.h>

namespace Botan {

class BigInt;

/**
* BER Decoding Object
*/
class BOTAN_PUBLIC_API(2,0) BER_Decoder final
   {
   public:
      /**
      * Set up to BER decode the data in buf of length len
      */
      BER_Decoder(const uint8_t buf[], size_t len);

      /**
      * Set up to BER decode the data in vec
      */
      explicit BER_Decoder(const secure_vector<uint8_t>& vec);

      /**
      * Set up to BER decode the data in vec
      */
      explicit BER_Decoder(const std::vector<uint8_t>& vec);

      /**
      * Set up to BER decode the data in src
      */
      explicit BER_Decoder(DataSource& src);

      /**
      * Set up to BER decode the data in obj
      */
      BER_Decoder(const BER_Object& obj) :
         BER_Decoder(obj.bits(), obj.length()) {}

      /**
      * Set up to BER decode the data in obj
      */
      BER_Decoder(BER_Object&& obj) :
         BER_Decoder(std::move(obj), nullptr) {}

      BER_Decoder(const BER_Decoder& other);

      BER_Decoder& operator=(const BER_Decoder&) = delete;

      /**
      * Get the next object in the data stream.
      * If EOF, returns an object with type NO_OBJECT.
      */
      BER_Object get_next_object();

      BER_Decoder& get_next(BER_Object& ber)
         {
         ber = get_next_object();
         return (*this);
         }

      /**
      * Push an object back onto the stream. Throws if another
      * object was previously pushed and has not been subsequently
      * read out.
      */
      void push_back(const BER_Object& obj);

      /**
      * Push an object back onto the stream. Throws if another
      * object was previously pushed and has not been subsequently
      * read out.
      */
      void push_back(BER_Object&& obj);

      /**
      * Return true if there is at least one more item remaining
      */
      bool more_items() const;

      /**
      * Verify the stream is concluded, throws otherwise.
      * Returns (*this)
      */
      BER_Decoder& verify_end();

      /**
      * Verify the stream is concluded, throws otherwise.
      * Returns (*this)
      */
      BER_Decoder& verify_end(const std::string& err_msg);

      /**
      * Discard any data that remains unread
      * Returns (*this)
      */
      BER_Decoder& discard_remaining();

      /**
      * Start decoding a constructed data (sequence or set)
      */
      BER_Decoder start_cons(ASN1_Tag type_tag, ASN1_Tag class_tag = UNIVERSAL);

      /**
      * Finish decoding a constructed data, throws if any data remains.
      * Returns the parent of *this (ie the object on which start_cons was called).
      */
      BER_Decoder& end_cons();

      /**
      * Get next object and copy value to POD type
      * Asserts value length is equal to POD type sizeof.
      * Asserts Type tag and optional Class tag according to parameters.
      * Copy value to POD type (struct, union, C-style array, std::array, etc.).
      * @param out POD type reference where to copy object value
      * @param type_tag ASN1_Tag enum to assert type on object read
      * @param class_tag ASN1_Tag enum to assert class on object read (default: CONTEXT_SPECIFIC)
      * @return this reference
      */
      template <typename T>
         BER_Decoder& get_next_value(T &out,
                                     ASN1_Tag type_tag,
                                     ASN1_Tag class_tag = CONTEXT_SPECIFIC)
         {
         static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "Type must be POD");

         BER_Object obj = get_next_object();
         obj.assert_is_a(type_tag, class_tag);

         if (obj.length() != sizeof(T))
            throw BER_Decoding_Error(
                    "Size mismatch. Object value size is " +
                    std::to_string(obj.length()) +
                    "; Output type size is " +
                    std::to_string(sizeof(T)));

         copy_mem(reinterpret_cast<uint8_t*>(&out), obj.bits(), obj.length());

         return (*this);
         }

      /*
      * Save all the bytes remaining in the source
      */
      template<typename Alloc>
      BER_Decoder& raw_bytes(std::vector<uint8_t, Alloc>& out)
         {
         out.clear();
         uint8_t buf;
         while(m_source->read_byte(buf))
            out.push_back(buf);
         return (*this);
         }

      BER_Decoder& decode_null();

      /**
      * Decode a BER encoded BOOLEAN
      */
      BER_Decoder& decode(bool& out)
         {
         return decode(out, BOOLEAN, UNIVERSAL);
         }

      /*
      * Decode a small BER encoded INTEGER
      */
      BER_Decoder& decode(size_t& out)
         {
         return decode(out, INTEGER, UNIVERSAL);
         }

      /*
      * Decode a BER encoded INTEGER
      */
      BER_Decoder& decode(BigInt& out)
         {
         return decode(out, INTEGER, UNIVERSAL);
         }

      std::vector<uint8_t> get_next_octet_string()
         {
         std::vector<uint8_t> out_vec;
         decode(out_vec, OCTET_STRING);
         return out_vec;
         }

      /*
      * BER decode a BIT STRING or OCTET STRING
      */
      template<typename Alloc>
      BER_Decoder& decode(std::vector<uint8_t, Alloc>& out, ASN1_Tag real_type)
         {
         return decode(out, real_type, real_type, UNIVERSAL);
         }

      BER_Decoder& decode(bool& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(size_t& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(BigInt& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(std::vector<uint8_t>& v,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(secure_vector<uint8_t>& v,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(class ASN1_Object& obj,
                          ASN1_Tag type_tag = NO_OBJECT,
                          ASN1_Tag class_tag = NO_OBJECT);

      /**
      * Decode an integer value which is typed as an octet string
      */
      BER_Decoder& decode_octet_string_bigint(BigInt& b);

      uint64_t decode_constrained_integer(ASN1_Tag type_tag,
                                          ASN1_Tag class_tag,
                                          size_t T_bytes);

      template<typename T> BER_Decoder& decode_integer_type(T& out)
         {
         return decode_integer_type<T>(out, INTEGER, UNIVERSAL);
         }

      template<typename T>
         BER_Decoder& decode_integer_type(T& out,
                                          ASN1_Tag type_tag,
                                          ASN1_Tag class_tag = CONTEXT_SPECIFIC)
         {
         out = static_cast<T>(decode_constrained_integer(type_tag, class_tag, sizeof(out)));
         return (*this);
         }

      template<typename T>
         BER_Decoder& decode_optional(T& out,
                                      ASN1_Tag type_tag,
                                      ASN1_Tag class_tag,
                                      const T& default_value = T());

      template<typename T>
         BER_Decoder& decode_optional_implicit(
            T& out,
            ASN1_Tag type_tag,
            ASN1_Tag class_tag,
            ASN1_Tag real_type,
            ASN1_Tag real_class,
            const T& default_value = T());

      template<typename T>
         BER_Decoder& decode_list(std::vector<T>& out,
                                  ASN1_Tag type_tag = SEQUENCE,
                                  ASN1_Tag class_tag = UNIVERSAL);

      template<typename T>
         BER_Decoder& decode_and_check(const T& expected,
                                       const std::string& error_msg)
         {
         T actual;
         decode(actual);

         if(actual != expected)
            throw Decoding_Error(error_msg);

         return (*this);
         }

      /*
      * Decode an OPTIONAL string type
      */
      template<typename Alloc>
      BER_Decoder& decode_optional_string(std::vector<uint8_t, Alloc>& out,
                                          ASN1_Tag real_type,
                                          uint16_t type_no,
                                          ASN1_Tag class_tag = CONTEXT_SPECIFIC)
         {
         BER_Object obj = get_next_object();

         ASN1_Tag type_tag = static_cast<ASN1_Tag>(type_no);

         if(obj.is_a(type_tag, class_tag))
            {
            if((class_tag & CONSTRUCTED) && (class_tag & CONTEXT_SPECIFIC))
               {
               BER_Decoder(std::move(obj)).decode(out, real_type).verify_end();
               }
            else
               {
               push_back(std::move(obj));
               decode(out, real_type, type_tag, class_tag);
               }
            }
         else
            {
            out.clear();
            push_back(std::move(obj));
            }

         return (*this);
         }

   private:
      BER_Decoder(BER_Object&& obj, BER_Decoder* parent);

      BER_Decoder* m_parent = nullptr;
      BER_Object m_pushed;
      // either m_data_src.get() or an unowned pointer
      DataSource* m_source;
      mutable std::unique_ptr<DataSource> m_data_src;
   };

/*
* Decode an OPTIONAL or DEFAULT element
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_optional(T& out,
                                          ASN1_Tag type_tag,
                                          ASN1_Tag class_tag,
                                          const T& default_value)
   {
   BER_Object obj = get_next_object();

   if(obj.is_a(type_tag, class_tag))
      {
      if((class_tag & CONSTRUCTED) && (class_tag & CONTEXT_SPECIFIC))
         {
         BER_Decoder(std::move(obj)).decode(out).verify_end();
         }
      else
         {
         push_back(std::move(obj));
         decode(out, type_tag, class_tag);
         }
      }
   else
      {
      out = default_value;
      push_back(std::move(obj));
      }

   return (*this);
   }

/*
* Decode an OPTIONAL or DEFAULT element
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_optional_implicit(
   T& out,
   ASN1_Tag type_tag,
   ASN1_Tag class_tag,
   ASN1_Tag real_type,
   ASN1_Tag real_class,
   const T& default_value)
   {
   BER_Object obj = get_next_object();

   if(obj.is_a(type_tag, class_tag))
      {
      obj.set_tagging(real_type, real_class);
      push_back(std::move(obj));
      decode(out, real_type, real_class);
      }
   else
      {
      // Not what we wanted, push it back on the stream
      out = default_value;
      push_back(std::move(obj));
      }

   return (*this);
   }
/*
* Decode a list of homogenously typed values
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_list(std::vector<T>& vec,
                                      ASN1_Tag type_tag,
                                      ASN1_Tag class_tag)
   {
   BER_Decoder list = start_cons(type_tag, class_tag);

   while(list.more_items())
      {
      T value;
      list.decode(value);
      vec.push_back(std::move(value));
      }

   list.end_cons();

   return (*this);
   }

}

#endif

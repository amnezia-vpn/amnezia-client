/*
* DER Encoder
* (C) 1999-2007,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DER_ENCODER_H_
#define BOTAN_DER_ENCODER_H_

#include <botan/asn1_obj.h>
#include <vector>
#include <functional>

namespace Botan {

class BigInt;

/**
* General DER Encoding Object
*/
class BOTAN_PUBLIC_API(2,0) DER_Encoder final
   {
   public:
      typedef std::function<void (const uint8_t[], size_t)> append_fn;

      /**
      * DER encode, writing to an internal buffer
      * Use get_contents or get_contents_unlocked to read the results
      * after all encoding is completed.
      */
      DER_Encoder() = default;

      /**
      * DER encode, writing to @param vec
      * If this constructor is used, get_contents* may not be called.
      */
      DER_Encoder(secure_vector<uint8_t>& vec);

      /**
      * DER encode, writing to @param vec
      * If this constructor is used, get_contents* may not be called.
      */
      DER_Encoder(std::vector<uint8_t>& vec);

      /**
      * DER encode, calling append to write output
      * If this constructor is used, get_contents* may not be called.
      */
      DER_Encoder(append_fn append) : m_append_output(append) {}

      secure_vector<uint8_t> get_contents();

      /**
      * Return the encoded contents as a std::vector
      *
      * If using this function, instead pass a std::vector to the
      * contructor of DER_Encoder where the output will be placed. This
      * avoids several unecessary copies.
      */
      std::vector<uint8_t> BOTAN_DEPRECATED("Use DER_Encoder(vector) instead") get_contents_unlocked();

      DER_Encoder& start_cons(ASN1_Tag type_tag,
                              ASN1_Tag class_tag = UNIVERSAL);
      DER_Encoder& end_cons();

      DER_Encoder& start_explicit(uint16_t type_tag);
      DER_Encoder& end_explicit();

      /**
      * Insert raw bytes directly into the output stream
      */
      DER_Encoder& raw_bytes(const uint8_t val[], size_t len);

      template<typename Alloc>
      DER_Encoder& raw_bytes(const std::vector<uint8_t, Alloc>& val)
         {
         return raw_bytes(val.data(), val.size());
         }

      DER_Encoder& encode_null();
      DER_Encoder& encode(bool b);
      DER_Encoder& encode(size_t s);
      DER_Encoder& encode(const BigInt& n);
      DER_Encoder& encode(const uint8_t val[], size_t len, ASN1_Tag real_type);

      template<typename Alloc>
      DER_Encoder& encode(const std::vector<uint8_t, Alloc>& vec, ASN1_Tag real_type)
         {
         return encode(vec.data(), vec.size(), real_type);
         }

      DER_Encoder& encode(bool b,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(size_t s,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(const BigInt& n,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(const uint8_t v[], size_t len,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      template<typename Alloc>
      DER_Encoder& encode(const std::vector<uint8_t, Alloc>& bytes,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag, ASN1_Tag class_tag)
         {
         return encode(bytes.data(), bytes.size(),
                       real_type, type_tag, class_tag);
         }

      template<typename T>
      DER_Encoder& encode_optional(const T& value, const T& default_value)
         {
         if(value != default_value)
            encode(value);
         return (*this);
         }

      template<typename T>
      DER_Encoder& encode_list(const std::vector<T>& values)
         {
         for(size_t i = 0; i != values.size(); ++i)
            encode(values[i]);
         return (*this);
         }

      /*
      * Request for an object to encode itself to this stream
      */
      DER_Encoder& encode(const ASN1_Object& obj);

      /*
      * Conditionally write some values to the stream
      */
      DER_Encoder& encode_if(bool pred, DER_Encoder& enc)
         {
         if(pred)
            return raw_bytes(enc.get_contents());
         return (*this);
         }

      DER_Encoder& encode_if(bool pred, const ASN1_Object& obj)
         {
         if(pred)
            encode(obj);
         return (*this);
         }

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const uint8_t rep[], size_t length);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const std::vector<uint8_t>& rep)
         {
         return add_object(type_tag, class_tag, rep.data(), rep.size());
         }

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const secure_vector<uint8_t>& rep)
         {
         return add_object(type_tag, class_tag, rep.data(), rep.size());
         }

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const std::string& str);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              uint8_t val);

   private:
      class DER_Sequence final
         {
         public:
            ASN1_Tag tag_of() const;

            void push_contents(DER_Encoder& der);

            void add_bytes(const uint8_t val[], size_t len);

            void add_bytes(const uint8_t hdr[], size_t hdr_len,
                           const uint8_t val[], size_t val_len);

            DER_Sequence(ASN1_Tag, ASN1_Tag);

            DER_Sequence(DER_Sequence&& seq)
               {
               std::swap(m_type_tag, seq.m_type_tag);
               std::swap(m_class_tag, seq.m_class_tag);
               std::swap(m_contents, seq.m_contents);
               std::swap(m_set_contents, seq.m_set_contents);
               }

            DER_Sequence& operator=(DER_Sequence&& seq)
               {
               std::swap(m_type_tag, seq.m_type_tag);
               std::swap(m_class_tag, seq.m_class_tag);
               std::swap(m_contents, seq.m_contents);
               std::swap(m_set_contents, seq.m_set_contents);
               return (*this);
               }

            DER_Sequence(const DER_Sequence& seq) = default;

            DER_Sequence& operator=(const DER_Sequence& seq) = default;

         private:
            ASN1_Tag m_type_tag, m_class_tag;
            secure_vector<uint8_t> m_contents;
            std::vector< secure_vector<uint8_t> > m_set_contents;
         };

      append_fn m_append_output;
      secure_vector<uint8_t> m_default_outbuf;
      std::vector<DER_Sequence> m_subsequences;
   };

}

#endif

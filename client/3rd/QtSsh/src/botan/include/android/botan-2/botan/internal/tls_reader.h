/*
* TLS Data Reader
* (C) 2010-2011,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_READER_H_
#define BOTAN_TLS_READER_H_

#include <botan/exceptn.h>
#include <botan/secmem.h>
#include <botan/loadstor.h>
#include <string>
#include <vector>

namespace Botan {

namespace TLS {

/**
* Helper class for decoding TLS protocol messages
*/
class TLS_Data_Reader final
   {
   public:
      TLS_Data_Reader(const char* type, const std::vector<uint8_t>& buf_in) :
         m_typename(type), m_buf(buf_in), m_offset(0) {}

      void assert_done() const
         {
         if(has_remaining())
            throw decode_error("Extra bytes at end of message");
         }

      size_t read_so_far() const { return m_offset; }

      size_t remaining_bytes() const { return m_buf.size() - m_offset; }

      bool has_remaining() const { return (remaining_bytes() > 0); }

      std::vector<uint8_t> get_remaining()
         {
         return std::vector<uint8_t>(m_buf.begin() + m_offset, m_buf.end());
         }

      void discard_next(size_t bytes)
         {
         assert_at_least(bytes);
         m_offset += bytes;
         }

      uint32_t get_uint32_t()
         {
         assert_at_least(4);
         uint32_t result = make_uint32(m_buf[m_offset  ], m_buf[m_offset+1],
                                     m_buf[m_offset+2], m_buf[m_offset+3]);
         m_offset += 4;
         return result;
         }

      uint16_t get_uint16_t()
         {
         assert_at_least(2);
         uint16_t result = make_uint16(m_buf[m_offset], m_buf[m_offset+1]);
         m_offset += 2;
         return result;
         }

      uint8_t get_byte()
         {
         assert_at_least(1);
         uint8_t result = m_buf[m_offset];
         m_offset += 1;
         return result;
         }

      template<typename T, typename Container>
      Container get_elem(size_t num_elems)
         {
         assert_at_least(num_elems * sizeof(T));

         Container result(num_elems);

         for(size_t i = 0; i != num_elems; ++i)
            result[i] = load_be<T>(&m_buf[m_offset], i);

         m_offset += num_elems * sizeof(T);

         return result;
         }

      template<typename T>
      std::vector<T> get_range(size_t len_bytes,
                               size_t min_elems,
                               size_t max_elems)
         {
         const size_t num_elems =
            get_num_elems(len_bytes, sizeof(T), min_elems, max_elems);

         return get_elem<T, std::vector<T>>(num_elems);
         }

      template<typename T>
      std::vector<T> get_range_vector(size_t len_bytes,
                                      size_t min_elems,
                                      size_t max_elems)
         {
         const size_t num_elems =
            get_num_elems(len_bytes, sizeof(T), min_elems, max_elems);

         return get_elem<T, std::vector<T>>(num_elems);
         }

      std::string get_string(size_t len_bytes,
                             size_t min_bytes,
                             size_t max_bytes)
         {
         std::vector<uint8_t> v =
            get_range_vector<uint8_t>(len_bytes, min_bytes, max_bytes);

         return std::string(cast_uint8_ptr_to_char(v.data()), v.size());
         }

      template<typename T>
      std::vector<T> get_fixed(size_t size)
         {
         return get_elem<T, std::vector<T>>(size);
         }

   private:
      size_t get_length_field(size_t len_bytes)
         {
         assert_at_least(len_bytes);

         if(len_bytes == 1)
            return get_byte();
         else if(len_bytes == 2)
            return get_uint16_t();

         throw decode_error("Bad length size");
         }

      size_t get_num_elems(size_t len_bytes,
                           size_t T_size,
                           size_t min_elems,
                           size_t max_elems)
         {
         const size_t byte_length = get_length_field(len_bytes);

         if(byte_length % T_size != 0)
            throw decode_error("Size isn't multiple of T");

         const size_t num_elems = byte_length / T_size;

         if(num_elems < min_elems || num_elems > max_elems)
            throw decode_error("Length field outside parameters");

         return num_elems;
         }

      void assert_at_least(size_t n) const
         {
         if(m_buf.size() - m_offset < n)
            throw decode_error("Expected " + std::to_string(n) +
                               " bytes remaining, only " +
                               std::to_string(m_buf.size()-m_offset) +
                               " left");
         }

      Decoding_Error decode_error(const std::string& why) const
         {
         return Decoding_Error("Invalid " + std::string(m_typename) + ": " + why);
         }

      const char* m_typename;
      const std::vector<uint8_t>& m_buf;
      size_t m_offset;
   };

/**
* Helper function for encoding length-tagged vectors
*/
template<typename T, typename Alloc>
void append_tls_length_value(std::vector<uint8_t, Alloc>& buf,
                             const T* vals,
                             size_t vals_size,
                             size_t tag_size)
   {
   const size_t T_size = sizeof(T);
   const size_t val_bytes = T_size * vals_size;

   if(tag_size != 1 && tag_size != 2)
      throw Invalid_Argument("append_tls_length_value: invalid tag size");

   if((tag_size == 1 && val_bytes > 255) ||
      (tag_size == 2 && val_bytes > 65535))
      throw Invalid_Argument("append_tls_length_value: value too large");

   for(size_t i = 0; i != tag_size; ++i)
      buf.push_back(get_byte(sizeof(val_bytes)-tag_size+i, val_bytes));

   for(size_t i = 0; i != vals_size; ++i)
      for(size_t j = 0; j != T_size; ++j)
         buf.push_back(get_byte(j, vals[i]));
   }

template<typename T, typename Alloc, typename Alloc2>
void append_tls_length_value(std::vector<uint8_t, Alloc>& buf,
                             const std::vector<T, Alloc2>& vals,
                             size_t tag_size)
   {
   append_tls_length_value(buf, vals.data(), vals.size(), tag_size);
   }

template<typename Alloc>
void append_tls_length_value(std::vector<uint8_t, Alloc>& buf,
                             const std::string& str,
                             size_t tag_size)
   {
   append_tls_length_value(buf,
                           cast_char_ptr_to_uint8(str.data()),
                           str.size(),
                           tag_size);
   }

}

}

#endif

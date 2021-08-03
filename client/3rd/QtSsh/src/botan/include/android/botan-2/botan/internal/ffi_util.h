/*
* (C) 2015,2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FFI_UTILS_H_
#define BOTAN_FFI_UTILS_H_

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <functional>
#include <botan/exceptn.h>
#include <botan/mem_ops.h>

namespace Botan_FFI {

class BOTAN_UNSTABLE_API FFI_Error final : public Botan::Exception
   {
   public:
      FFI_Error(const std::string& what, int err_code) :
         Exception("FFI error", what),
         m_err_code(err_code)
         {}

      int error_code() const noexcept override { return m_err_code; }

      Botan::ErrorType error_type() const noexcept override { return Botan::ErrorType::InvalidArgument; }

   private:
      int m_err_code;
   };

template<typename T, uint32_t MAGIC>
struct botan_struct
   {
   public:
      botan_struct(T* obj) : m_magic(MAGIC), m_obj(obj) {}
      virtual ~botan_struct() { m_magic = 0; m_obj.reset(); }

      bool magic_ok() const { return (m_magic == MAGIC); }

      T* unsafe_get() const
         {
         return m_obj.get();
         }
   private:
      uint32_t m_magic = 0;
      std::unique_ptr<T> m_obj;
   };

#define BOTAN_FFI_DECLARE_STRUCT(NAME, TYPE, MAGIC) \
   struct NAME final : public Botan_FFI::botan_struct<TYPE, MAGIC> { explicit NAME(TYPE* x) : botan_struct(x) {} }

// Declared in ffi.cpp
int ffi_error_exception_thrown(const char* func_name, const char* exn,
                               int rc = BOTAN_FFI_ERROR_EXCEPTION_THROWN);

template<typename T, uint32_t M>
T& safe_get(botan_struct<T,M>* p)
   {
   if(!p)
      throw FFI_Error("Null pointer argument", BOTAN_FFI_ERROR_NULL_POINTER);
   if(p->magic_ok() == false)
      throw FFI_Error("Bad magic in ffi object", BOTAN_FFI_ERROR_INVALID_OBJECT);

   if(T* t = p->unsafe_get())
      return *t;

   throw FFI_Error("Invalid object pointer", BOTAN_FFI_ERROR_INVALID_OBJECT);
   }

int ffi_guard_thunk(const char* func_name, std::function<int ()>);

template<typename T, uint32_t M, typename F>
int apply_fn(botan_struct<T, M>* o, const char* func_name, F func)
   {
   if(!o)
      return BOTAN_FFI_ERROR_NULL_POINTER;

   if(o->magic_ok() == false)
      return BOTAN_FFI_ERROR_INVALID_OBJECT;

   T* p = o->unsafe_get();
   if(p == nullptr)
      return BOTAN_FFI_ERROR_INVALID_OBJECT;

   return ffi_guard_thunk(func_name, [&]() { return func(*p); });
   }

#define BOTAN_FFI_DO(T, obj, param, block)                \
   apply_fn(obj, __func__,                                \
            [=](T& param) -> int { do { block } while(0); return BOTAN_FFI_SUCCESS; })

/*
* Like BOTAN_FFI_DO but with no trailing return with the expectation
* that the block always returns a value. This exists because otherwise
* MSVC warns about the dead return after the block in FFI_DO.
*/
#define BOTAN_FFI_RETURNING(T, obj, param, block)         \
   apply_fn(obj, __func__,                                \
            [=](T& param) -> int { do { block } while(0); })

template<typename T, uint32_t M>
int ffi_delete_object(botan_struct<T, M>* obj, const char* func_name)
   {
   try
      {
      if(obj == nullptr)
         return BOTAN_FFI_SUCCESS; // ignore delete of null objects

      if(obj->magic_ok() == false)
         return BOTAN_FFI_ERROR_INVALID_OBJECT;

      delete obj;
      return BOTAN_FFI_SUCCESS;
      }
   catch(std::exception& e)
      {
      return ffi_error_exception_thrown(func_name, e.what());
      }
   catch(...)
      {
      return ffi_error_exception_thrown(func_name, "unknown exception");
      }
   }

#define BOTAN_FFI_CHECKED_DELETE(o) ffi_delete_object(o, __func__)

inline int write_output(uint8_t out[], size_t* out_len, const uint8_t buf[], size_t buf_len)
   {
   if(out_len == nullptr)
      return BOTAN_FFI_ERROR_NULL_POINTER;

   const size_t avail = *out_len;
   *out_len = buf_len;

   if((avail >= buf_len) && (out != nullptr))
      {
      Botan::copy_mem(out, buf, buf_len);
      return BOTAN_FFI_SUCCESS;
      }
   else
      {
      if(out != nullptr)
         {
         Botan::clear_mem(out, avail);
         }
      return BOTAN_FFI_ERROR_INSUFFICIENT_BUFFER_SPACE;
      }
   }

template<typename Alloc>
int write_vec_output(uint8_t out[], size_t* out_len, const std::vector<uint8_t, Alloc>& buf)
   {
   return write_output(out, out_len, buf.data(), buf.size());
   }

inline int write_str_output(uint8_t out[], size_t* out_len, const std::string& str)
   {
   return write_output(out, out_len,
                       Botan::cast_char_ptr_to_uint8(str.data()),
                       str.size() + 1);
   }

inline int write_str_output(char out[], size_t* out_len, const std::string& str)
   {
   return write_str_output(Botan::cast_char_ptr_to_uint8(out), out_len, str);
   }

inline int write_str_output(char out[], size_t* out_len, const std::vector<uint8_t>& str_vec)
   {
   return write_output(Botan::cast_char_ptr_to_uint8(out),
                       out_len,
                       str_vec.data(),
                       str_vec.size());
   }

}

#endif

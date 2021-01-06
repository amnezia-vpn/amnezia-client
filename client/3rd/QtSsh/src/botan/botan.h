/*
* Botan 1.10.2 Amalgamation
* (C) 1999-2011 Jack Lloyd and others
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_AMALGAMATION_H__
#define BOTAN_AMALGAMATION_H__

#ifdef USE_SYSTEM_BOTAN
#include <botan/auto_rng.h>
#include <botan/cbc.h>
#include <botan/ctr.h>
#include <botan/der_enc.h>
#include <botan/dh.h>
#include <botan/dl_group.h>
#include <botan/dsa.h>
#include <botan/ec_group.h>
#include <botan/ecdh.h>
#include <botan/ecdsa.h>
#include <botan/hmac.h>
#include <botan/init.h>
#include <botan/lookup.h>
#include <botan/pem.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/ui.h>
#else

#include <QtGlobal>

#include <iosfwd>
#include <map>
#include <exception>
#include <string>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <utility>

#define BOTAN_VERSION_MAJOR 1
#define BOTAN_VERSION_MINOR 10
#define BOTAN_VERSION_PATCH 2
#define BOTAN_VERSION_DATESTAMP 0

#define BOTAN_VERSION_VC_REVISION "mtn:2bf8ad2c501213efb4cf9b219330b87666988e91"

#define BOTAN_DISTRIBUTION_INFO "unspecified"

//#ifndef BOTAN_DLL
//#define BOTAN_DLL Q_DECL_IMPORT
//#endif

#define BOTAN_DLL

/* Chunk sizes */
#define BOTAN_DEFAULT_BUFFER_SIZE 4096
#define BOTAN_MEM_POOL_CHUNK_SIZE 64*1024
#define BOTAN_BLOCK_CIPHER_PAR_MULT 4

/* BigInt toggles */
#define BOTAN_MP_WORD_BITS 32
#define BOTAN_KARAT_MUL_THRESHOLD 32
#define BOTAN_KARAT_SQR_THRESHOLD 32

/* PK key consistency checking toggles */
#define BOTAN_PUBLIC_KEY_STRONG_CHECKS_ON_LOAD 1
#define BOTAN_PRIVATE_KEY_STRONG_CHECKS_ON_LOAD 0
#define BOTAN_PRIVATE_KEY_STRONG_CHECKS_ON_GENERATE 1

/* Should we use GCC-style inline assembler? */
#if !defined(BOTAN_USE_GCC_INLINE_ASM) && defined(__GNUG__)
  #define BOTAN_USE_GCC_INLINE_ASM 1
#endif

#if !defined(BOTAN_USE_GCC_INLINE_ASM)
  #define BOTAN_USE_GCC_INLINE_ASM 0
#endif

#ifdef __GNUC__
  #define BOTAN_GCC_VERSION \
     (__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__)
#else
  #define BOTAN_GCC_VERSION 0
#endif

#define BOTAN_TARGET_CPU_IS_LITTLE_ENDIAN
#define BOTAN_TARGET_CPU_IS_X86_FAMILY
#define BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK 1

#if defined(BOTAN_TARGET_CPU_IS_LITTLE_ENDIAN) || \
    defined(BOTAN_TARGET_CPU_IS_BIG_ENDIAN)
  #define BOTAN_TARGET_CPU_HAS_KNOWN_ENDIANNESS
#endif

#if defined(_MSC_VER)
  // 4250: inherits via dominance (diamond inheritence issue)
  // 4251: needs DLL interface (STL DLL exports)
  #pragma warning(disable: 4250 4251)
#endif

/*
* Compile-time deprecatation warnings
*/
#if !defined(BOTAN_NO_DEPRECATED_WARNINGS)

  #if defined(__clang__)
    #define BOTAN_DEPRECATED(msg) __attribute__ ((deprecated))

  #elif defined(_MSC_VER)
    #define BOTAN_DEPRECATED(msg) __declspec(deprecated(msg))

  #elif defined(__GNUG__)

    #if BOTAN_GCC_VERSION >= 450 && !defined(__INTEL_COMPILER)
      #define BOTAN_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
    #else
      #define BOTAN_DEPRECATED(msg) __attribute__ ((deprecated))
    #endif

  #endif

#endif

#if !defined(BOTAN_DEPRECATED)
  #define BOTAN_DEPRECATED(msg)
#endif

/*
* Module availability definitions
*/
#define BOTAN_HAS_ADLER32
#define BOTAN_HAS_AES
#define BOTAN_HAS_ALGORITHM_FACTORY
#define BOTAN_HAS_ANSI_X919_MAC
#define BOTAN_HAS_ARC4
#define BOTAN_HAS_ASN1
#define BOTAN_HAS_AUTO_SEEDING_RNG
#define BOTAN_HAS_BASE64_CODEC
#define BOTAN_HAS_BCRYPT
#define BOTAN_HAS_BIGINT
#define BOTAN_HAS_BIGINT_MATH
#define BOTAN_HAS_BIGINT_MP
#define BOTAN_HAS_BLOCK_CIPHER
#define BOTAN_HAS_BLOWFISH
#define BOTAN_HAS_BMW_512
#define BOTAN_HAS_CAMELLIA
#define BOTAN_HAS_CASCADE
#define BOTAN_HAS_CAST
#define BOTAN_HAS_CBC
#define BOTAN_HAS_CBC_MAC
#define BOTAN_HAS_CERTIFICATE_STORE
#define BOTAN_HAS_CFB
#define BOTAN_HAS_CIPHER_MODE_PADDING
#define BOTAN_HAS_CMAC
#define BOTAN_HAS_CODEC_FILTERS
#define BOTAN_HAS_COMB4P
#define BOTAN_HAS_CORE_ENGINE
#define BOTAN_HAS_CRC24
#define BOTAN_HAS_CRC32
#define BOTAN_HAS_CRYPTO_BOX
#define BOTAN_HAS_CTR_BE
#define BOTAN_HAS_CTS
#define BOTAN_HAS_DES
#define BOTAN_HAS_DIFFIE_HELLMAN
#define BOTAN_HAS_DLIES
#define BOTAN_HAS_DL_GROUP
#define BOTAN_HAS_DL_PUBLIC_KEY_FAMILY
#define BOTAN_HAS_DSA
#define BOTAN_HAS_EAX
#define BOTAN_HAS_ECB
#define BOTAN_HAS_ECC_GROUP
#define BOTAN_HAS_ECC_PUBLIC_KEY_CRYPTO
#define BOTAN_HAS_ECDH
#define BOTAN_HAS_ECDSA
#define BOTAN_HAS_EC_CURVE_GFP
#define BOTAN_HAS_ELGAMAL
#define BOTAN_HAS_EME1
#define BOTAN_HAS_EME_PKCS1v15
#define BOTAN_HAS_EMSA1
#define BOTAN_HAS_EMSA1_BSI
#define BOTAN_HAS_EMSA2
#define BOTAN_HAS_EMSA3
#define BOTAN_HAS_EMSA4
#define BOTAN_HAS_EMSA_RAW
#define BOTAN_HAS_ENGINES
#define BOTAN_HAS_ENGINE_SIMD
#define BOTAN_HAS_ENTROPY_SRC_HIGH_RESOLUTION_TIMER
#define BOTAN_HAS_FILTERS
#define BOTAN_HAS_FPE_FE1
#define BOTAN_HAS_GOST_28147_89
#define BOTAN_HAS_GOST_34_10_2001
#define BOTAN_HAS_GOST_34_11
#define BOTAN_HAS_HASH_ID
#define BOTAN_HAS_HAS_160
#define BOTAN_HAS_HEX_CODEC
#define BOTAN_HAS_HMAC
#define BOTAN_HAS_HMAC_RNG
#define BOTAN_HAS_IDEA
#define BOTAN_HAS_IF_PUBLIC_KEY_FAMILY
#define BOTAN_HAS_KASUMI
#define BOTAN_HAS_KDF1
#define BOTAN_HAS_KDF2
#define BOTAN_HAS_KDF_BASE
#define BOTAN_HAS_KECCAK
#define BOTAN_HAS_KEYPAIR_TESTING
#define BOTAN_HAS_LIBSTATE_MODULE
#define BOTAN_HAS_LION
#define BOTAN_HAS_LUBY_RACKOFF
#define BOTAN_HAS_MARS
#define BOTAN_HAS_MD2
#define BOTAN_HAS_MD4
#define BOTAN_HAS_MD5
#define BOTAN_HAS_MDX_HASH_FUNCTION
#define BOTAN_HAS_MGF1
#define BOTAN_HAS_MISTY1
#define BOTAN_HAS_MUTEX_NOOP
#define BOTAN_HAS_MUTEX_WRAPPERS
#define BOTAN_HAS_NOEKEON
#define BOTAN_HAS_NOEKEON_SIMD
#define BOTAN_HAS_NYBERG_RUEPPEL
#define BOTAN_HAS_OFB
#define BOTAN_HAS_OID_LOOKUP
#define BOTAN_HAS_OPENPGP_CODEC
#define BOTAN_HAS_PACKAGE_TRANSFORM
#define BOTAN_HAS_PARALLEL_HASH
#define BOTAN_HAS_PASSHASH9
#define BOTAN_HAS_PASSWORD_BASED_ENCRYPTION
#define BOTAN_HAS_PBE_PKCS_V15
#define BOTAN_HAS_PBE_PKCS_V20
#define BOTAN_HAS_PBKDF1
#define BOTAN_HAS_PBKDF2
#define BOTAN_HAS_PEM_CODEC
#define BOTAN_HAS_PGPS2K
#define BOTAN_HAS_PKCS10_REQUESTS
#define BOTAN_HAS_PK_PADDING
#define BOTAN_HAS_PUBLIC_KEY_CRYPTO
#define BOTAN_HAS_PUBLIC_KEY_CRYPTO
#define BOTAN_HAS_RANDPOOL
#define BOTAN_HAS_RC2
#define BOTAN_HAS_RC5
#define BOTAN_HAS_RC6
#define BOTAN_HAS_RFC3394_KEYWRAP
#define BOTAN_HAS_RIPEMD_128
#define BOTAN_HAS_RIPEMD_160
#define BOTAN_HAS_RSA
#define BOTAN_HAS_RUNTIME_BENCHMARKING
#define BOTAN_HAS_RW
#define BOTAN_HAS_SAFER
#define BOTAN_HAS_SALSA20
#define BOTAN_HAS_SEED
#define BOTAN_HAS_SELFTESTS
#define BOTAN_HAS_SERPENT
#define BOTAN_HAS_SERPENT_SIMD
#define BOTAN_HAS_SHA1
#define BOTAN_HAS_SHA2_32
#define BOTAN_HAS_SHA2_64
#define BOTAN_HAS_SIMD_32
#define BOTAN_HAS_SIMD_SCALAR
#define BOTAN_HAS_SKEIN_512
#define BOTAN_HAS_SKIPJACK
#define BOTAN_HAS_SQUARE
#define BOTAN_HAS_SRP6
#define BOTAN_HAS_SSL3_MAC
#define BOTAN_HAS_SSL_V3_PRF
#define BOTAN_HAS_STREAM_CIPHER
#define BOTAN_HAS_TEA
#define BOTAN_HAS_THRESHOLD_SECRET_SHARING
#define BOTAN_HAS_TIGER
#define BOTAN_HAS_TLS_V10_PRF
#define BOTAN_HAS_TURING
#define BOTAN_HAS_TWOFISH
#define BOTAN_HAS_UTIL_FUNCTIONS
#define BOTAN_HAS_WHIRLPOOL
#define BOTAN_HAS_WID_WAKE
#define BOTAN_HAS_X509_CA
#define BOTAN_HAS_X509_CERTIFICATES
#define BOTAN_HAS_X509_CRL
#define BOTAN_HAS_X509_SELF_SIGNED
#define BOTAN_HAS_X509_STORE
#define BOTAN_HAS_X931_RNG
#define BOTAN_HAS_X942_PRF
#define BOTAN_HAS_XTEA
#define BOTAN_HAS_XTEA_SIMD
#define BOTAN_HAS_XTS

/*
* Local configuration options (if any) follow
*/


#include <stddef.h>

/**
* The primary namespace for the botan library
*/
namespace Botan {

/**
* Typedef representing an unsigned 8-bit quantity
*/
typedef unsigned char byte;

/**
* Typedef representing an unsigned 16-bit quantity
*/
typedef unsigned short u16bit;

/**
* Typedef representing an unsigned 32-bit quantity
*/
typedef unsigned int u32bit;

/**
* Typedef representing a signed 32-bit quantity
*/
typedef signed int s32bit;

/**
* Typedef representing an unsigned 64-bit quantity
*/
#if defined(_MSC_VER) || defined(__BORLANDC__)
   typedef unsigned __int64 u64bit;
#elif defined(__KCC)
   typedef unsigned __long_long u64bit;
#elif defined(__GNUG__)
   __extension__ typedef unsigned long long u64bit;
#else
   typedef unsigned long long u64bit;
#endif

/**
* A default buffer size; typically a memory page
*/
static const size_t DEFAULT_BUFFERSIZE = BOTAN_DEFAULT_BUFFER_SIZE;

}

namespace Botan_types {

using Botan::byte;
using Botan::u32bit;

}


namespace Botan {

/**
* Allocator Interface
*/
class BOTAN_DLL Allocator
   {
   public:
      /**
      * Acquire a pointer to an allocator
      * @param locking is true if the allocator should attempt to
      *                secure the memory (eg for using to store keys)
      * @return pointer to an allocator; ownership remains with library,
      *                 so do not delete
      */
      static Allocator* get(bool locking);

      /**
      * Allocate a block of memory
      * @param n how many bytes to allocate
      * @return pointer to n bytes of memory
      */
      virtual void* allocate(size_t n) = 0;

      /**
      * Deallocate memory allocated with allocate()
      * @param ptr the pointer returned by allocate()
      * @param n the size of the block pointed to by ptr
      */
      virtual void deallocate(void* ptr, size_t n) = 0;

      /**
      * @return name of this allocator type
      */
      virtual std::string type() const = 0;

      /**
      * Initialize the allocator
      */
      virtual void init() {}

      /**
      * Shutdown the allocator
      */
      virtual void destroy() {}

      virtual ~Allocator() Q_DECL_NOEXCEPT_EXPR(false) {}
   };

}


namespace Botan {

/**
* Copy memory
* @param out the destination array
* @param in the source array
* @param n the number of elements of in/out
*/
template<typename T> inline void copy_mem(T* out, const T* in, size_t n)
   {
   std::memmove(out, in, sizeof(T)*n);
   }

/**
* Zeroize memory
* @param ptr a pointer to an array
* @param n the number of Ts pointed to by ptr
*/
template<typename T> inline void clear_mem(T* ptr, size_t n)
   {
   if(n) // avoid glibc warning if n == 0
      std::memset(ptr, 0, sizeof(T)*n);
   }

/**
* Set memory to a fixed value
* @param ptr a pointer to an array
* @param n the number of Ts pointed to by ptr
* @param val the value to set each byte to
*/
template<typename T>
inline void set_mem(T* ptr, size_t n, byte val)
   {
   std::memset(ptr, val, sizeof(T)*n);
   }

/**
* Memory comparison, input insensitive
* @param p1 a pointer to an array
* @param p2 a pointer to another array
* @param n the number of Ts in p1 and p2
* @return true iff p1[i] == p2[i] forall i in [0...n)
*/
template<typename T> inline bool same_mem(const T* p1, const T* p2, size_t n)
   {
   bool is_same = true;

   for(size_t i = 0; i != n; ++i)
      is_same &= (p1[i] == p2[i]);

   return is_same;
   }

}


namespace Botan {

/**
* This class represents variable length memory buffers.
*/
template<typename T>
class MemoryRegion
   {
   public:
      /**
      * Find out the size of the buffer, i.e. how many objects of type T it
      * contains.
      * @return size of the buffer
      */
      size_t size() const { return used; }

      /**
      * Find out whether this buffer is empty.
      * @return true if the buffer is empty, false otherwise
      */
      bool empty() const { return (used == 0); }

      /**
      * Get a pointer to the first element in the buffer.
      * @return pointer to the first element in the buffer
      */
      operator T* () { return buf; }

      /**
      * Get a constant pointer to the first element in the buffer.
      * @return constant pointer to the first element in the buffer
      */
      operator const T* () const { return buf; }

      /**
      * Get a pointer to the first element in the buffer.
      * @return pointer to the first element in the buffer
      */
      T* begin() { return buf; }

      /**
      * Get a constant pointer to the first element in the buffer.
      * @return constant pointer to the first element in the buffer
      */
      const T* begin() const { return buf; }

      /**
      * Get a pointer to one past the last element in the buffer.
      * @return pointer to one past the last element in the buffer
      */
      T* end() { return (buf + size()); }

      /**
      * Get a const pointer to one past the last element in the buffer.
      * @return const pointer to one past the last element in the buffer
      */
      const T* end() const { return (buf + size()); }

      /**
      * Check two buffers for equality.
      * @return true iff the content of both buffers is byte-wise equal
      */
      bool operator==(const MemoryRegion<T>& other) const
         {
         return (size() == other.size() &&
                 same_mem(buf, other.buf, size()));
         }

      /**
      * Compare two buffers
      * @return true iff this is ordered before other
      */
      bool operator<(const MemoryRegion<T>& other) const;

      /**
      * Check two buffers for inequality.
      * @return false if the content of both buffers is byte-wise equal, true
      * otherwise.
      */
      bool operator!=(const MemoryRegion<T>& other) const
         { return (!(*this == other)); }

      /**
      * Copy the contents of another buffer into this buffer.
      * The former contents of *this are discarded.
      * @param other the buffer to copy the contents from.
      * @return reference to *this
      */
      MemoryRegion<T>& operator=(const MemoryRegion<T>& other)
         {
         if(this != &other)
            {
            this->resize(other.size());
            this->copy(&other[0], other.size());
            }
         return (*this);
         }

      /**
      * Copy the contents of an array of objects of type T into this buffer.
      * The former contents of *this are discarded.
      * The length of *this must be at least n, otherwise memory errors occur.
      * @param in the array to copy the contents from
      * @param n the length of in
      */
      void copy(const T in[], size_t n)
         {
         copy_mem(buf, in, std::min(n, size()));
         }

      /**
      * Copy the contents of an array of objects of type T into this buffer.
      * The former contents of *this are discarded.
      * The length of *this must be at least n, otherwise memory errors occur.
      * @param off the offset position inside this buffer to start inserting
      * the copied bytes
      * @param in the array to copy the contents from
      * @param n the length of in
      */
      void copy(size_t off, const T in[], size_t n)
         {
         copy_mem(buf + off, in, std::min(n, size() - off));
         }

      /**
      * Append a single element.
      * @param x the element to append
      */
      void push_back(T x)
         {
         resize(size() + 1);
         buf[size()-1] = x;
         }

      /**
      * Reset this buffer to an empty buffer with size zero.
      */
      void clear() { resize(0); }

      /**
      * Inserts or erases elements at the end such that the size
      * becomes n, leaving elements in the range 0...n unmodified if
      * set or otherwise zero-initialized
      * @param n length of the new buffer
      */
      void resize(size_t n);

      /**
      * Swap this buffer with another object.
      */
      void swap(MemoryRegion<T>& other);

      virtual ~MemoryRegion() { deallocate(buf, allocated); }
   protected:
      MemoryRegion() : buf(0), used(0), allocated(0), alloc(0) {}

      /**
      * Copy constructor
      * @param other the other region to copy
      */
      MemoryRegion(const MemoryRegion<T>& other) :
         buf(0),
         used(0),
         allocated(0),
         alloc(other.alloc)
         {
         resize(other.size());
         copy(&other[0], other.size());
         }

      /**
      * @param locking should we use a locking allocator
      * @param length the initial length to use
      */
      void init(bool locking, size_t length = 0)
         { alloc = Allocator::get(locking); resize(length); }

   private:
      T* allocate(size_t n)
         {
         return static_cast<T*>(alloc->allocate(sizeof(T)*n));
         }

      void deallocate(T* p, size_t n)
         { if(alloc && p && n) alloc->deallocate(p, sizeof(T)*n); }

      T* buf;
      size_t used;
      size_t allocated;
      Allocator* alloc;
   };

/*
* Change the size of the buffer
*/
template<typename T>
void MemoryRegion<T>::resize(size_t n)
   {
   if(n <= allocated)
      {
      size_t zap = std::min(used, n);
      clear_mem(buf + zap, allocated - zap);
      used = n;
      }
   else
      {
      T* new_buf = allocate(n);
      copy_mem(new_buf, buf, used);
      deallocate(buf, allocated);
      buf = new_buf;
      allocated = used = n;
      }
   }

/*
* Compare this buffer with another one
*/
template<typename T>
bool MemoryRegion<T>::operator<(const MemoryRegion<T>& other) const
   {
   const size_t min_size = std::min(size(), other.size());

   // This should probably be rewritten to run in constant time
   for(size_t i = 0; i != min_size; ++i)
      {
      if(buf[i] < other[i])
         return true;
      if(buf[i] > other[i])
         return false;
      }

   // First min_size bytes are equal, shorter is first
   return (size() < other.size());
   }

/*
* Swap this buffer with another one
*/
template<typename T>
void MemoryRegion<T>::swap(MemoryRegion<T>& x)
   {
   std::swap(buf, x.buf);
   std::swap(used, x.used);
   std::swap(allocated, x.allocated);
   std::swap(alloc, x.alloc);
   }

/**
* This class represents variable length buffers that do not
* make use of memory locking.
*/
template<typename T>
class MemoryVector : public MemoryRegion<T>
   {
   public:
      /**
      * Copy the contents of another buffer into this buffer.
      * @param in the buffer to copy the contents from
      * @return reference to *this
      */
      MemoryVector<T>& operator=(const MemoryRegion<T>& in)
         {
         if(this != &in)
            {
            this->resize(in.size());
            this->copy(&in[0], in.size());
            }
         return (*this);
         }

      /**
      * Create a buffer of the specified length.
      * @param n the length of the buffer to create.
      */
      MemoryVector(size_t n = 0) { this->init(false, n); }

      /**
      * Create a buffer with the specified contents.
      * @param in the array containing the data to be initially copied
      * into the newly created buffer
      * @param n the size of the arry in
      */
      MemoryVector(const T in[], size_t n)
         {
         this->init(false);
         this->resize(n);
         this->copy(in, n);
         }

      /**
      * Copy constructor.
      */
      MemoryVector(const MemoryRegion<T>& in)
         {
         this->init(false);
         this->resize(in.size());
         this->copy(&in[0], in.size());
         }
   };

/**
* This class represents variable length buffers using the operating
* systems capability to lock memory, i.e. keeping it from being
* swapped out to disk. In this way, a security hole allowing attackers
* to find swapped out secret keys is closed.
*/
template<typename T>
class SecureVector : public MemoryRegion<T>
   {
   public:
      /**
      * Copy the contents of another buffer into this buffer.
      * @param other the buffer to copy the contents from
      * @return reference to *this
      */
      SecureVector<T>& operator=(const MemoryRegion<T>& other)
         {
         if(this != &other)
            {
            this->resize(other.size());
            this->copy(&other[0], other.size());
            }
         return (*this);
         }

      /**
      * Create a buffer of the specified length.
      * @param n the length of the buffer to create.
      */
      SecureVector(size_t n = 0) { this->init(true, n); }

      /**
      * Create a buffer with the specified contents.
      * @param in the array containing the data to be initially copied
      * into the newly created buffer
      * @param n the size of the array in
      */
      SecureVector(const T in[], size_t n)
         {
         this->init(true);
         this->resize(n);
         this->copy(&in[0], n);
         }

      /**
      * Create a buffer with contents specified contents.
      * @param in the buffer holding the contents that will be
      * copied into the newly created buffer.
      */
      SecureVector(const MemoryRegion<T>& in)
         {
         this->init(true);
         this->resize(in.size());
         this->copy(&in[0], in.size());
         }
   };

template<typename T>
MemoryRegion<T>& operator+=(MemoryRegion<T>& out,
                            const MemoryRegion<T>& in)
   {
   const size_t copy_offset = out.size();
   out.resize(out.size() + in.size());
   copy_mem(&out[copy_offset], &in[0], in.size());
   return out;
   }

template<typename T>
MemoryRegion<T>& operator+=(MemoryRegion<T>& out,
                            T in)
   {
   out.push_back(in);
   return out;
   }

template<typename T, typename L>
MemoryRegion<T>& operator+=(MemoryRegion<T>& out,
                            const std::pair<const T*, L>& in)
   {
   const size_t copy_offset = out.size();
   out.resize(out.size() + in.second);
   copy_mem(&out[copy_offset], in.first, in.second);
   return out;
   }

template<typename T, typename L>
MemoryRegion<T>& operator+=(MemoryRegion<T>& out,
                            const std::pair<T*, L>& in)
   {
   const size_t copy_offset = out.size();
   out.resize(out.size() + in.second);
   copy_mem(&out[copy_offset], in.first, in.second);
   return out;
   }

/**
* Zeroise the values; length remains unchanged
* @param vec the vector to zeroise
*/
template<typename T>
void zeroise(MemoryRegion<T>& vec)
   {
   clear_mem(&vec[0], vec.size());
   }

}

namespace std {

template<typename T>
inline void swap(Botan::MemoryRegion<T>& x, Botan::MemoryRegion<T>& y)
   {
   x.swap(y);
   }

}


namespace Botan {

/**
* Byte extraction
* @param byte_num which byte to extract, 0 == highest byte
* @param input the value to extract from
* @return byte byte_num of input
*/
template<typename T> inline byte get_byte(size_t byte_num, T input)
   {
   return static_cast<byte>(
      input >> ((sizeof(T)-1-(byte_num&(sizeof(T)-1))) << 3)
      );
   }

}


namespace Botan {

/**
* This class represents any kind of computation which uses an internal
* state, such as hash functions or MACs
*/
class BOTAN_DLL Buffered_Computation
   {
   public:
      /**
      * @return length of the output of this function in bytes
      */
      virtual size_t output_length() const = 0;

      /**
      * Add new input to process.
      * @param in the input to process as a byte array
      * @param length of param in in bytes
      */
      void update(const byte in[], size_t length) { add_data(in, length); }

      /**
      * Add new input to process.
      * @param in the input to process as a MemoryRegion
      */
      void update(const MemoryRegion<byte>& in)
         {
         add_data(&in[0], in.size());
         }

      /**
      * Add an integer in big-endian order
      * @param in the value
      */
      template<typename T> void update_be(const T in)
         {
         for(size_t i = 0; i != sizeof(T); ++i)
            {
            byte b = get_byte(i, in);
            add_data(&b, 1);
            }
         }

      /**
      * Add new input to process.
      * @param str the input to process as a std::string. Will be interpreted
      * as a byte array based on
      * the strings encoding.
      */
      void update(const std::string& str)
         {
         add_data(reinterpret_cast<const byte*>(str.data()), str.size());
         }

      /**
      * Process a single byte.
      * @param in the byte to process
      */
      void update(byte in) { add_data(&in, 1); }

      /**
      * Complete the computation and retrieve the
      * final result.
      * @param out The byte array to be filled with the result.
      * Must be of length output_length()
      */
      void final(byte out[]) { final_result(out); }

      /**
      * Complete the computation and retrieve the
      * final result.
      * @return SecureVector holding the result
      */
      SecureVector<byte> final()
         {
         SecureVector<byte> output(output_length());
         final_result(&output[0]);
         return output;
         }

      /**
      * Update and finalize computation. Does the same as calling update()
      * and final() consecutively.
      * @param in the input to process as a byte array
      * @param length the length of the byte array
      * @result the result of the call to final()
      */
      SecureVector<byte> process(const byte in[], size_t length)
         {
         add_data(in, length);
         return final();
         }

      /**
      * Update and finalize computation. Does the same as calling update()
      * and final() consecutively.
      * @param in the input to process
      * @result the result of the call to final()
      */
      SecureVector<byte> process(const MemoryRegion<byte>& in)
         {
         add_data(&in[0], in.size());
         return final();
         }

      /**
      * Update and finalize computation. Does the same as calling update()
      * and final() consecutively.
      * @param in the input to process as a string
      * @result the result of the call to final()
      */
      SecureVector<byte> process(const std::string& in)
         {
         update(in);
         return final();
         }

      virtual ~Buffered_Computation() {}
   private:
      /**
      * Add more data to the computation
      * @param input is an input buffer
      * @param length is the length of input in bytes
      */
      virtual void add_data(const byte input[], size_t length) = 0;

      /**
      * Write the final output to out
      * @param out is an output buffer of output_length()
      */
      virtual void final_result(byte out[]) = 0;
   };

}


namespace Botan {

/**
* Class used to accumulate the poll results of EntropySources
*/
class BOTAN_DLL Entropy_Accumulator
   {
   public:
      /**
      * Initialize an Entropy_Accumulator
      * @param goal is how many bits we would like to collect
      */
      Entropy_Accumulator(size_t goal) :
         entropy_goal(goal), collected_bits(0) {}

      virtual ~Entropy_Accumulator() {}

      /**
      * Get a cached I/O buffer (purely for minimizing allocation
      * overhead to polls)
      *
      * @param size requested size for the I/O buffer
      * @return cached I/O buffer for repeated polls
      */
      MemoryRegion<byte>& get_io_buffer(size_t size)
         { io_buffer.resize(size); return io_buffer; }

      /**
      * @return number of bits collected so far
      */
      size_t bits_collected() const
         { return static_cast<size_t>(collected_bits); }

      /**
      * @return if our polling goal has been achieved
      */
      bool polling_goal_achieved() const
         { return (collected_bits >= entropy_goal); }

      /**
      * @return how many bits we need to reach our polling goal
      */
      size_t desired_remaining_bits() const
         {
         if(collected_bits >= entropy_goal)
            return 0;
         return static_cast<size_t>(entropy_goal - collected_bits);
         }

      /**
      * Add entropy to the accumulator
      * @param bytes the input bytes
      * @param length specifies how many bytes the input is
      * @param entropy_bits_per_byte is a best guess at how much
      * entropy per byte is in this input
      */
      void add(const void* bytes, size_t length, double entropy_bits_per_byte)
         {
         add_bytes(reinterpret_cast<const byte*>(bytes), length);
         collected_bits += entropy_bits_per_byte * length;
         }

      /**
      * Add entropy to the accumulator
      * @param v is some value
      * @param entropy_bits_per_byte is a best guess at how much
      * entropy per byte is in this input
      */
      template<typename T>
      void add(const T& v, double entropy_bits_per_byte)
         {
         add(&v, sizeof(T), entropy_bits_per_byte);
         }
   private:
      virtual void add_bytes(const byte bytes[], size_t length) = 0;

      SecureVector<byte> io_buffer;
      size_t entropy_goal;
      double collected_bits;
   };

/**
* Entropy accumulator that puts the input into a Buffered_Computation
*/
class BOTAN_DLL Entropy_Accumulator_BufferedComputation :
   public Entropy_Accumulator
   {
   public:
      /**
      * @param sink the hash or MAC we are feeding the poll data into
      * @param goal is how many bits we want to collect in this poll
      */
      Entropy_Accumulator_BufferedComputation(Buffered_Computation& sink,
                                              size_t goal) :
         Entropy_Accumulator(goal), entropy_sink(sink) {}

   private:
      virtual void add_bytes(const byte bytes[], size_t length)
         {
         entropy_sink.update(bytes, length);
         }

      Buffered_Computation& entropy_sink;
   };

/**
* Abstract interface to a source of (hopefully unpredictable) system entropy
*/
class BOTAN_DLL EntropySource
   {
   public:
      /**
      * @return name identifying this entropy source
      */
      virtual std::string name() const = 0;

      /**
      * Perform an entropy gathering poll
      * @param accum is an accumulator object that will be given entropy
      */
      virtual void poll(Entropy_Accumulator& accum) = 0;

      virtual ~EntropySource() {}
   };

}


namespace Botan {

/**
* Parse a SCAN-style algorithm name
* @param scan_name the name
* @return the name components
*/
BOTAN_DLL std::vector<std::string>
parse_algorithm_name(const std::string& scan_name);

/**
* Split a string
* @param str the input string
* @param delim the delimitor
* @return string split by delim
*/
BOTAN_DLL std::vector<std::string> split_on(
   const std::string& str, char delim);

/**
* Parse an ASN.1 OID
* @param oid the OID in string form
* @return OID components
*/
BOTAN_DLL std::vector<u32bit> parse_asn1_oid(const std::string& oid);

/**
* Compare two names using the X.509 comparison algorithm
* @param name1 the first name
* @param name2 the second name
* @return true if name1 is the same as name2 by the X.509 comparison rules
*/
BOTAN_DLL bool x500_name_cmp(const std::string& name1,
                             const std::string& name2);

/**
* Convert a number to a string
* @param n the integer to convert to a string
* @param min_len the min length of the output string
* @return n convert to a string
*/
BOTAN_DLL std::string to_string(u64bit n, size_t min_len = 0);

/**
* Convert a string to a number
* @param str the string to convert
* @return number value of the string
*/
BOTAN_DLL u32bit to_u32bit(const std::string& str);

/**
* Convert a time specification to a number
* @param timespec the time specification
* @return number of seconds represented by timespec
*/
BOTAN_DLL u32bit timespec_to_u32bit(const std::string& timespec);

/**
* Convert a string representation of an IPv4 address to a number
* @param ip_str the string representation
* @return integer IPv4 address
*/
BOTAN_DLL u32bit string_to_ipv4(const std::string& ip_str);

/**
* Convert an IPv4 address to a string
* @param ip_addr the IPv4 address to convert
* @return string representation of the IPv4 address
*/
BOTAN_DLL std::string ipv4_to_string(u32bit ip_addr);

}


namespace Botan {

typedef std::runtime_error Exception;
typedef std::invalid_argument Invalid_Argument;

/**
* Invalid_State Exception
*/
struct BOTAN_DLL Invalid_State : public Exception
   {
   Invalid_State(const std::string& err) :
      Exception(err)
      {}
   };

/**
* Lookup_Error Exception
*/
struct BOTAN_DLL Lookup_Error : public Exception
   {
   Lookup_Error(const std::string& err) :
      Exception(err)
      {}
   };

/**
* Internal_Error Exception
*/
struct BOTAN_DLL Internal_Error : public Exception
   {
   Internal_Error(const std::string& err) :
      Exception("Internal error: " + err)
      {}
   };

/**
* Invalid_Key_Length Exception
*/
struct BOTAN_DLL Invalid_Key_Length : public Invalid_Argument
   {
   Invalid_Key_Length(const std::string& name, size_t length) :
      Invalid_Argument(name + " cannot accept a key of length " +
                       to_string(length))
      {}
   };

/**
* Invalid_Block_Size Exception
*/
struct BOTAN_DLL Invalid_Block_Size : public Invalid_Argument
   {
   Invalid_Block_Size(const std::string& mode,
                      const std::string& pad) :
      Invalid_Argument("Padding method " + pad +
                       " cannot be used with " + mode)
      {}
   };

/**
* Invalid_IV_Length Exception
*/
struct BOTAN_DLL Invalid_IV_Length : public Invalid_Argument
   {
   Invalid_IV_Length(const std::string& mode, size_t bad_len) :
      Invalid_Argument("IV length " + to_string(bad_len) +
                       " is invalid for " + mode)
      {}
   };

/**
* PRNG_Unseeded Exception
*/
struct BOTAN_DLL PRNG_Unseeded : public Invalid_State
   {
   PRNG_Unseeded(const std::string& algo) :
      Invalid_State("PRNG not seeded: " + algo)
      {}
   };

/**
* Policy_Violation Exception
*/
struct BOTAN_DLL Policy_Violation : public Invalid_State
   {
   Policy_Violation(const std::string& err) :
      Invalid_State("Policy violation: " + err)
      {}
   };

/**
* Algorithm_Not_Found Exception
*/
struct BOTAN_DLL Algorithm_Not_Found : public Lookup_Error
   {
   Algorithm_Not_Found(const std::string& name) :
      Lookup_Error("Could not find any algorithm named \"" + name + "\"")
      {}
   };

/**
* Invalid_Algorithm_Name Exception
*/
struct BOTAN_DLL Invalid_Algorithm_Name : public Invalid_Argument
   {
   Invalid_Algorithm_Name(const std::string& name):
      Invalid_Argument("Invalid algorithm name: " + name)
      {}
   };

/**
* Encoding_Error Exception
*/
struct BOTAN_DLL Encoding_Error : public Invalid_Argument
   {
   Encoding_Error(const std::string& name) :
      Invalid_Argument("Encoding error: " + name) {}
   };

/**
* Decoding_Error Exception
*/
struct BOTAN_DLL Decoding_Error : public Invalid_Argument
   {
   Decoding_Error(const std::string& name) :
      Invalid_Argument("Decoding error: " + name) {}
   };

/**
* Integrity_Failure Exception
*/
struct BOTAN_DLL Integrity_Failure : public Exception
   {
   Integrity_Failure(const std::string& msg) :
      Exception("Integrity failure: " + msg) {}
   };

/**
* Invalid_OID Exception
*/
struct BOTAN_DLL Invalid_OID : public Decoding_Error
   {
   Invalid_OID(const std::string& oid) :
      Decoding_Error("Invalid ASN.1 OID: " + oid) {}
   };

/**
* Stream_IO_Error Exception
*/
struct BOTAN_DLL Stream_IO_Error : public Exception
   {
   Stream_IO_Error(const std::string& err) :
      Exception("I/O error: " + err)
      {}
   };

/**
* Self Test Failure Exception
*/
struct BOTAN_DLL Self_Test_Failure : public Internal_Error
   {
   Self_Test_Failure(const std::string& err) :
      Internal_Error("Self test failed: " + err)
      {}
   };

/**
* Memory Allocation Exception
*/
struct BOTAN_DLL Memory_Exhaustion : public std::bad_alloc
   {
   const char* what() const throw()
      { return "Ran out of memory, allocation failed"; }
   };

}


namespace Botan {

/**
* This class represents a random number (RNG) generator object.
*/
class BOTAN_DLL RandomNumberGenerator
   {
   public:
      /**
      * Create a seeded and active RNG object for general application use
      */
      static RandomNumberGenerator* make_rng();

      /**
      * Randomize a byte array.
      * @param output the byte array to hold the random output.
      * @param length the length of the byte array output.
      */
      virtual void randomize(byte output[], size_t length) = 0;

      /**
      * Return a random vector
      * @param bytes number of bytes in the result
      * @return randomized vector of length bytes
      */
      SecureVector<byte> random_vec(size_t bytes)
         {
         SecureVector<byte> output(bytes);
         randomize(&output[0], output.size());
         return output;
         }

      /**
      * Return a random byte
      * @return random byte
      */
      byte next_byte();

      /**
      * Check whether this RNG is seeded.
      * @return true if this RNG was already seeded, false otherwise.
      */
      virtual bool is_seeded() const { return true; }

      /**
      * Clear all internally held values of this RNG.
      */
      virtual void clear() = 0;

      /**
      * Return the name of this object
      */
      virtual std::string name() const = 0;

      /**
      * Seed this RNG using the entropy sources it contains.
      * @param bits_to_collect is the number of bits of entropy to
               attempt to gather from the entropy sources
      */
      virtual void reseed(size_t bits_to_collect) = 0;

      /**
      * Add this entropy source to the RNG object
      * @param source the entropy source which will be retained and used by RNG
      */
      virtual void add_entropy_source(EntropySource* source) = 0;

      /**
      * Add entropy to this RNG.
      * @param in a byte array containg the entropy to be added
      * @param length the length of the byte array in
      */
      virtual void add_entropy(const byte in[], size_t length) = 0;

      RandomNumberGenerator() {}
      virtual ~RandomNumberGenerator() {}
   private:
      RandomNumberGenerator(const RandomNumberGenerator&) {}
      RandomNumberGenerator& operator=(const RandomNumberGenerator&)
         { return (*this); }
   };

/**
* Null/stub RNG - fails if you try to use it for anything
*/
class BOTAN_DLL Null_RNG : public RandomNumberGenerator
   {
   public:
      void randomize(byte[], size_t) { throw PRNG_Unseeded("Null_RNG"); }
      void clear() {}
      std::string name() const { return "Null_RNG"; }

      void reseed(size_t) {}
      bool is_seeded() const { return false; }
      void add_entropy(const byte[], size_t) {}
      void add_entropy_source(EntropySource* es) { delete es; }
   };

}


namespace Botan {

/**
* Encoding Method for Signatures, Appendix
*/
class BOTAN_DLL EMSA
   {
   public:
      /**
      * Add more data to the signature computation
      * @param input some data
      * @param length length of input in bytes
      */
      virtual void update(const byte input[], size_t length) = 0;

      /**
      * @return raw hash
      */
      virtual SecureVector<byte> raw_data() = 0;

      /**
      * Return the encoding of a message
      * @param msg the result of raw_data()
      * @param output_bits the desired output bit size
      * @param rng a random number generator
      * @return encoded signature
      */
      virtual SecureVector<byte> encoding_of(const MemoryRegion<byte>& msg,
                                             size_t output_bits,
                                             RandomNumberGenerator& rng) = 0;

      /**
      * Verify the encoding
      * @param coded the received (coded) message representative
      * @param raw the computed (local, uncoded) message representative
      * @param key_bits the size of the key in bits
      * @return true if coded is a valid encoding of raw, otherwise false
      */
      virtual bool verify(const MemoryRegion<byte>& coded,
                          const MemoryRegion<byte>& raw,
                          size_t key_bits) = 0;
      virtual ~EMSA() {}
   };

}


namespace Botan {

/**
* This class represents an algorithm of some kind
*/
class BOTAN_DLL Algorithm
   {
   public:

      /**
      * Zeroize internal state
      */
      virtual void clear() = 0;

      /**
      * @return name of this algorithm
      */
      virtual std::string name() const = 0;

      Algorithm() {}
      virtual ~Algorithm() {}
   private:
      Algorithm(const Algorithm&) {}
      Algorithm& operator=(const Algorithm&) { return (*this); }
   };

}


namespace Botan {

/**
* This class represents hash function (message digest) objects
*/
class BOTAN_DLL HashFunction : public Buffered_Computation,
                               public Algorithm
   {
   public:
      /**
      * Get a new object representing the same algorithm as *this
      */
      virtual HashFunction* clone() const = 0;

      /**
      * The hash block size as defined for this algorithm
      */
      virtual size_t hash_block_size() const { return 0; }
   };

}


namespace Botan {

/**
* EMSA1 from IEEE 1363
* Essentially, sign the hash directly
*/
class BOTAN_DLL EMSA1 : public EMSA
   {
   public:
      /**
      * @param h the hash object to use
      */
      EMSA1(HashFunction* h) : hash(h) {}
      ~EMSA1() { delete hash; }
   protected:
      /**
      * @return const pointer to the underlying hash
      */
      const HashFunction* hash_ptr() const { return hash; }
   private:
      void update(const byte[], size_t);
      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);

      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);

      HashFunction* hash;
   };

}


namespace Botan {

/**
* Keccak[1600], a SHA-3 candidate
*/
class BOTAN_DLL Keccak_1600 : public HashFunction
   {
   public:

      /**
      * @param output_bits the size of the hash output; must be one of
      *                    224, 256, 384, or 512
      */
      Keccak_1600(size_t output_bits = 512);

      size_t hash_block_size() const { return bitrate / 8; }
      size_t output_length() const { return output_bits / 8; }

      HashFunction* clone() const;
      std::string name() const;
      void clear();
   private:
      void add_data(const byte input[], size_t length);
      void final_result(byte out[]);

      size_t output_bits, bitrate;
      SecureVector<u64bit> S;
      size_t S_pos;
   };

}


namespace Botan {

/**
A class encapsulating a SCAN name (similar to JCE conventions)
http://www.users.zetnet.co.uk/hopwood/crypto/scan/
*/
class BOTAN_DLL SCAN_Name
   {
   public:
      /**
      * @param algo_spec A SCAN-format name
      */
      SCAN_Name(std::string algo_spec);

      /**
      * @return original input string
      */
      std::string as_string() const { return orig_algo_spec; }

      /**
      * @return algorithm name
      */
      std::string algo_name() const { return alg_name; }

      /**
      * @return algorithm name plus any arguments
      */
      std::string algo_name_and_args() const;

      /**
      * @return number of arguments
      */
      size_t arg_count() const { return args.size(); }

      /**
      * @param lower is the lower bound
      * @param upper is the upper bound
      * @return if the number of arguments is between lower and upper
      */
      bool arg_count_between(size_t lower, size_t upper) const
         { return ((arg_count() >= lower) && (arg_count() <= upper)); }

      /**
      * @param i which argument
      * @return ith argument
      */
      std::string arg(size_t i) const;

      /**
      * @param i which argument
      * @param def_value the default value
      * @return ith argument or the default value
      */
      std::string arg(size_t i, const std::string& def_value) const;

      /**
      * @param i which argument
      * @param def_value the default value
      * @return ith argument as an integer, or the default value
      */
      size_t arg_as_integer(size_t i, size_t def_value) const;

      /**
      * @return cipher mode (if any)
      */
      std::string cipher_mode() const
         { return (mode_info.size() >= 1) ? mode_info[0] : ""; }

      /**
      * @return cipher mode padding (if any)
      */
      std::string cipher_mode_pad() const
         { return (mode_info.size() >= 2) ? mode_info[1] : ""; }

   private:
      std::string orig_algo_spec;
      std::string alg_name;
      std::vector<std::string> args;
      std::vector<std::string> mode_info;
   };

}


namespace Botan {

/**
* Represents the length requirements on an algorithm key
*/
class BOTAN_DLL Key_Length_Specification
   {
   public:
      /**
      * Constructor for fixed length keys
      * @param keylen the supported key length
      */
      Key_Length_Specification(size_t keylen) :
         min_keylen(keylen),
         max_keylen(keylen),
         keylen_mod(1)
         {
         }

      /**
      * Constructor for variable length keys
      * @param min_k the smallest supported key length
      * @param max_k the largest supported key length
      * @param k_mod the number of bytes the key must be a multiple of
      */
      Key_Length_Specification(size_t min_k,
                               size_t max_k,
                               size_t k_mod = 1) :
         min_keylen(min_k),
         max_keylen(max_k ? max_k : min_k),
         keylen_mod(k_mod)
         {
         }

      /**
      * @param length is a key length in bytes
      * @return true iff this length is a valid length for this algo
      */
      bool valid_keylength(size_t length) const
         {
         return ((length >= min_keylen) &&
                 (length <= max_keylen) &&
                 (length % keylen_mod == 0));
         }

      /**
      * @return minimum key length in bytes
      */
      size_t minimum_keylength() const
         {
         return min_keylen;
         }

      /**
      * @return maximum key length in bytes
      */
      size_t maximum_keylength() const
         {
         return max_keylen;
         }

      /**
      * @return key length multiple in bytes
      */
      size_t keylength_multiple() const
         {
         return keylen_mod;
         }

   private:
      size_t min_keylen, max_keylen, keylen_mod;
   };

}


namespace Botan {

/**
* Octet String
*/
class BOTAN_DLL OctetString
   {
   public:
      /**
      * @return size of this octet string in bytes
      */
      size_t length() const { return bits.size(); }

      /**
      * @return this object as a SecureVector<byte>
      */
      SecureVector<byte> bits_of() const { return bits; }

      /**
      * @return start of this string
      */
      const byte* begin() const { return &bits[0]; }

      /**
      * @return end of this string
      */
      const byte* end() const   { return &bits[bits.size()]; }

      /**
      * @return this encoded as hex
      */
      std::string as_string() const;

      /**
      * XOR the contents of another octet string into this one
      * @param other octet string
      * @return reference to this
      */
      OctetString& operator^=(const OctetString& other);

      /**
      * Force to have odd parity
      */
      void set_odd_parity();

      /**
      * Change the contents of this octet string
      * @param hex_string a hex encoded bytestring
      */
      void change(const std::string& hex_string);

      /**
      * Change the contents of this octet string
      * @param in the input
      * @param length of in in bytes
      */
      void change(const byte in[], size_t length);

      /**
      * Change the contents of this octet string
      * @param in the input
      */
      void change(const MemoryRegion<byte>& in) { bits = in; }

      /**
      * Create a new random OctetString
      * @param rng is a random number generator
      * @param len is the desired length in bytes
      */
      OctetString(class RandomNumberGenerator& rng, size_t len);

      /**
      * Create a new OctetString
      * @param str is a hex encoded string
      */
      OctetString(const std::string& str = "") { change(str); }

      /**
      * Create a new OctetString
      * @param in is an array
      * @param len is the length of in in bytes
      */
      OctetString(const byte in[], size_t len) { change(in, len); }

      /**
      * Create a new OctetString
      * @param in a bytestring
      */
      OctetString(const MemoryRegion<byte>& in) { change(in); }
   private:
      SecureVector<byte> bits;
   };

/**
* Compare two strings
* @param x an octet string
* @param y an octet string
* @return if x is equal to y
*/
BOTAN_DLL bool operator==(const OctetString& x,
                          const OctetString& y);

/**
* Compare two strings
* @param x an octet string
* @param y an octet string
* @return if x is not equal to y
*/
BOTAN_DLL bool operator!=(const OctetString& x,
                          const OctetString& y);

/**
* Concatenate two strings
* @param x an octet string
* @param y an octet string
* @return x concatenated with y
*/
BOTAN_DLL OctetString operator+(const OctetString& x,
                                const OctetString& y);

/**
* XOR two strings
* @param x an octet string
* @param y an octet string
* @return x XORed with y
*/
BOTAN_DLL OctetString operator^(const OctetString& x,
                                const OctetString& y);


/**
* Alternate name for octet string showing intent to use as a key
*/
typedef OctetString SymmetricKey;

/**
* Alternate name for octet string showing intent to use as an IV
*/
typedef OctetString InitializationVector;

}


namespace Botan {

/**
* This class represents a symmetric algorithm object.
*/
class BOTAN_DLL SymmetricAlgorithm : public Algorithm
   {
   public:
      /**
      * @return object describing limits on key size
      */
      virtual Key_Length_Specification key_spec() const = 0;

      /**
      * @return minimum allowed key length
      */
      size_t maximum_keylength() const
         {
         return key_spec().maximum_keylength();
         }

      /**
      * @return maxmium allowed key length
      */
      size_t minimum_keylength() const
         {
         return key_spec().minimum_keylength();
         }

      /**
      * Check whether a given key length is valid for this algorithm.
      * @param length the key length to be checked.
      * @return true if the key length is valid.
      */
      bool valid_keylength(size_t length) const
         {
         return key_spec().valid_keylength(length);
         }

      /**
      * Set the symmetric key of this object.
      * @param key the SymmetricKey to be set.
      */
      void set_key(const SymmetricKey& key)
         { set_key(key.begin(), key.length()); }

      /**
      * Set the symmetric key of this object.
      * @param key the to be set as a byte array.
      * @param length in bytes of key param
      */
      void set_key(const byte key[], size_t length)
         {
         if(!valid_keylength(length))
            throw Invalid_Key_Length(name(), length);
         key_schedule(key, length);
         }
   private:
      /**
      * Run the key schedule
      * @param key the key
      * @param length of key
      */
      virtual void key_schedule(const byte key[], size_t length) = 0;
   };

/**
* The two possible directions for cipher filters, determining whether they
* actually perform encryption or decryption.
*/
enum Cipher_Dir { ENCRYPTION, DECRYPTION };

}


namespace Botan {

/**
* This class represents a block cipher object.
*/
class BOTAN_DLL BlockCipher : public SymmetricAlgorithm
   {
   public:

      /**
      * @return block size of this algorithm
      */
      virtual size_t block_size() const = 0;

      /**
      * @return native parallelism of this cipher in blocks
      */
      virtual size_t parallelism() const { return 1; }

      /**
      * @return prefererred parallelism of this cipher in bytes
      */
      size_t parallel_bytes() const
         {
         return parallelism() * block_size() * BOTAN_BLOCK_CIPHER_PAR_MULT;
         }

      /**
      * Encrypt a block.
      * @param in The plaintext block to be encrypted as a byte array.
      * Must be of length block_size().
      * @param out The byte array designated to hold the encrypted block.
      * Must be of length block_size().
      */
      void encrypt(const byte in[], byte out[]) const
         { encrypt_n(in, out, 1); }

      /**
      * Decrypt a block.
      * @param in The ciphertext block to be decypted as a byte array.
      * Must be of length block_size().
      * @param out The byte array designated to hold the decrypted block.
      * Must be of length block_size().
      */
      void decrypt(const byte in[], byte out[]) const
         { decrypt_n(in, out, 1); }

      /**
      * Encrypt a block.
      * @param block the plaintext block to be encrypted
      * Must be of length block_size(). Will hold the result when the function
      * has finished.
      */
      void encrypt(byte block[]) const { encrypt_n(block, block, 1); }

      /**
      * Decrypt a block.
      * @param block the ciphertext block to be decrypted
      * Must be of length block_size(). Will hold the result when the function
      * has finished.
      */
      void decrypt(byte block[]) const { decrypt_n(block, block, 1); }

      /**
      * Encrypt one or more blocks
      * @param in the input buffer (multiple of block_size())
      * @param out the output buffer (same size as in)
      * @param blocks the number of blocks to process
      */
      virtual void encrypt_n(const byte in[], byte out[],
                             size_t blocks) const = 0;

      /**
      * Decrypt one or more blocks
      * @param in the input buffer (multiple of block_size())
      * @param out the output buffer (same size as in)
      * @param blocks the number of blocks to process
      */
      virtual void decrypt_n(const byte in[], byte out[],
                             size_t blocks) const = 0;

      /**
      * Get a new object representing the same algorithm as *this
      */
      virtual BlockCipher* clone() const = 0;
   };

/**
* Represents a block cipher with a single fixed block size
*/
template<size_t BS, size_t KMIN, size_t KMAX = 0, size_t KMOD = 1>
class Block_Cipher_Fixed_Params : public BlockCipher
   {
   public:
      enum { BLOCK_SIZE = BS };
      size_t block_size() const { return BS; }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(KMIN, KMAX, KMOD);
         }
   };

}


namespace Botan {

/**
* Base class for all stream ciphers
*/
class BOTAN_DLL StreamCipher : public SymmetricAlgorithm
   {
   public:
      /**
      * Encrypt or decrypt a message
      * @param in the plaintext
      * @param out the byte array to hold the output, i.e. the ciphertext
      * @param len the length of both in and out in bytes
      */
      virtual void cipher(const byte in[], byte out[], size_t len) = 0;

      /**
      * Encrypt or decrypt a message
      * @param buf the plaintext / ciphertext
      * @param len the length of buf in bytes
      */
      void cipher1(byte buf[], size_t len)
         { cipher(buf, buf, len); }

      /**
      * Resync the cipher using the IV
      * @param iv the initialization vector
      * @param iv_len the length of the IV in bytes
      */
      virtual void set_iv(const byte iv[], size_t iv_len);

      /**
      * @param iv_len the length of the IV in bytes
      * @return if the length is valid for this algorithm
      */
      virtual bool valid_iv_length(size_t iv_len) const;

      /**
      * Get a new object representing the same algorithm as *this
      */
      virtual StreamCipher* clone() const = 0;
   };

}


namespace Botan {

/**
* This class represents Message Authentication Code (MAC) objects.
*/
class BOTAN_DLL MessageAuthenticationCode : public Buffered_Computation,
                                            public SymmetricAlgorithm
   {
   public:
      /**
      * Verify a MAC.
      * @param in the MAC to verify as a byte array
      * @param length the length of param in
      * @return true if the MAC is valid, false otherwise
      */
      virtual bool verify_mac(const byte in[], size_t length);

      /**
      * Get a new object representing the same algorithm as *this
      */
      virtual MessageAuthenticationCode* clone() const = 0;

      /**
      * Get the name of this algorithm.
      * @return name of this algorithm
      */
      virtual std::string name() const = 0;
   };

}


namespace Botan {

/**
* Base class for PBKDF (password based key derivation function)
* implementations. Converts a password into a key using a salt
* and iterated hashing to make brute force attacks harder.
*/
class BOTAN_DLL PBKDF : public Algorithm
   {
   public:

      /**
      * @return new instance of this same algorithm
      */
      virtual PBKDF* clone() const = 0;

      void clear() {}

      /**
      * Derive a key from a passphrase
      * @param output_len the desired length of the key to produce
      * @param passphrase the password to derive the key from
      * @param salt a randomly chosen salt
      * @param salt_len length of salt in bytes
      * @param iterations the number of iterations to use (use 10K or more)
      */
      virtual OctetString derive_key(size_t output_len,
                                     const std::string& passphrase,
                                     const byte salt[], size_t salt_len,
                                     size_t iterations) const = 0;
   };

/**
* For compatability with 1.8
*/
typedef PBKDF S2K;

}


namespace Botan {

#if (BOTAN_MP_WORD_BITS == 8)
  typedef byte word;
#elif (BOTAN_MP_WORD_BITS == 16)
  typedef u16bit word;
#elif (BOTAN_MP_WORD_BITS == 32)
  typedef u32bit word;
#elif (BOTAN_MP_WORD_BITS == 64)
  typedef u64bit word;
#else
  #error BOTAN_MP_WORD_BITS must be 8, 16, 32, or 64
#endif

const word MP_WORD_MASK = ~static_cast<word>(0);
const word MP_WORD_TOP_BIT = static_cast<word>(1) << (8*sizeof(word) - 1);
const word MP_WORD_MAX = MP_WORD_MASK;

}


namespace Botan {

/**
* Arbitrary precision integer
*/
class BOTAN_DLL BigInt
   {
   public:
     /**
     * Base enumerator for encoding and decoding
     */
     enum Base { Octal = 8, Decimal = 10, Hexadecimal = 16, Binary = 256 };

     /**
     * Sign symbol definitions for positive and negative numbers
     */
     enum Sign { Negative = 0, Positive = 1 };

     /**
     * Number types (currently only power-of-2 supported)
     */
     enum NumberType { Power2 };

     /**
     * DivideByZero Exception
     */
     struct BOTAN_DLL DivideByZero : public Exception
        { DivideByZero() : Exception("BigInt divide by zero") {} };

     /**
     * += operator
     * @param y the BigInt to add to this
     */
     BigInt& operator+=(const BigInt& y);

     /**
     * -= operator
     * @param y the BigInt to subtract from this
     */
     BigInt& operator-=(const BigInt& y);

     /**
     * *= operator
     * @param y the BigInt to multiply with this
     */
     BigInt& operator*=(const BigInt& y);

     /**
     * /= operator
     * @param y the BigInt to divide this by
     */
     BigInt& operator/=(const BigInt& y);

     /**
     * Modulo operator
     * @param y the modulus to reduce this by
     */
     BigInt& operator%=(const BigInt& y);

     /**
     * Modulo operator
     * @param y the modulus (word) to reduce this by
     */
     word    operator%=(word y);

     /**
     * Left shift operator
     * @param shift the number of bits to shift this left by
     */
     BigInt& operator<<=(size_t shift);

     /**
     * Right shift operator
     * @param shift the number of bits to shift this right by
     */
     BigInt& operator>>=(size_t shift);

     /**
     * Increment operator
     */
     BigInt& operator++() { return (*this += 1); }

     /**
     * Decrement operator
     */
     BigInt& operator--() { return (*this -= 1); }

     /**
     * Postfix increment operator
     */
     BigInt  operator++(int) { BigInt x = (*this); ++(*this); return x; }

     /**
     * Postfix decrement operator
     */
     BigInt  operator--(int) { BigInt x = (*this); --(*this); return x; }

     /**
     * Unary negation operator
     * @return negative this
     */
     BigInt operator-() const;

     /**
     * ! operator
     * @return true iff this is zero, otherwise false
     */
     bool operator !() const { return (!is_nonzero()); }

     /**
     * [] operator (array access)
     * @param i a word index
     * @return the word at index i
     */
     word& operator[](size_t i) { return reg[i]; }

     /**
     * [] operator (array access)
     * @param i a word index
     * @return the word at index i
     */
     const word& operator[](size_t i) const { return reg[i]; }

     /**
     * Zeroize the BigInt
     */
     void clear() { zeroise(reg); }

     /**
     * Compare this to another BigInt
     * @param n the BigInt value to compare with
     * @param check_signs include sign in comparison?
     * @result if (this<n) return -1, if (this>n) return 1, if both
     * values are identical return 0 [like Perl's <=> operator]
     */
     s32bit cmp(const BigInt& n, bool check_signs = true) const;

     /**
     * Test if the integer has an even value
     * @result true if the integer is even, false otherwise
     */
     bool is_even() const { return (get_bit(0) == 0); }

     /**
     * Test if the integer has an odd value
     * @result true if the integer is odd, false otherwise
     */
     bool is_odd()  const { return (get_bit(0) == 1); }

     /**
     * Test if the integer is not zero
     * @result true if the integer is non-zero, false otherwise
     */
     bool is_nonzero() const { return (!is_zero()); }

     /**
     * Test if the integer is zero
     * @result true if the integer is zero, false otherwise
     */
     bool is_zero() const
        {
        const size_t sw = sig_words();

        for(size_t i = 0; i != sw; ++i)
           if(reg[i])
              return false;
        return true;
        }

     /**
     * Set bit at specified position
     * @param n bit position to set
     */
     void set_bit(size_t n);

     /**
     * Clear bit at specified position
     * @param n bit position to clear
     */
     void clear_bit(size_t n);

     /**
     * Clear all but the lowest n bits
     * @param n amount of bits to keep
     */
     void mask_bits(size_t n);

     /**
     * Return bit value at specified position
     * @param n the bit offset to test
     * @result true, if the bit at position n is set, false otherwise
     */
     bool get_bit(size_t n) const;

     /**
     * Return (a maximum of) 32 bits of the complete value
     * @param offset the offset to start extracting
     * @param length amount of bits to extract (starting at offset)
     * @result the integer extracted from the register starting at
     * offset with specified length
     */
     u32bit get_substring(size_t offset, size_t length) const;

     /**
     * Convert this value into a u32bit, if it is in the range
     * [0 ... 2**32-1], or otherwise throw an exception.
     * @result the value as a u32bit if conversion is possible
     */
     u32bit to_u32bit() const;

     /**
     * @param n the offset to get a byte from
     * @result byte at offset n
     */
     byte byte_at(size_t n) const;

     /**
     * Return the word at a specified position of the internal register
     * @param n position in the register
     * @return value at position n
     */
     word word_at(size_t n) const
        { return ((n < size()) ? reg[n] : 0); }

     /**
     * Tests if the sign of the integer is negative
     * @result true, iff the integer has a negative sign
     */
     bool is_negative() const { return (sign() == Negative); }

     /**
     * Tests if the sign of the integer is positive
     * @result true, iff the integer has a positive sign
     */
     bool is_positive() const { return (sign() == Positive); }

     /**
     * Return the sign of the integer
     * @result the sign of the integer
     */
     Sign sign() const { return (signedness); }

     /**
     * @result the opposite sign of the represented integer value
     */
     Sign reverse_sign() const;

     /**
     * Flip the sign of this BigInt
     */
     void flip_sign();

     /**
     * Set sign of the integer
     * @param sign new Sign to set
     */
     void set_sign(Sign sign);

     /**
     * @result absolute (positive) value of this
     */
     BigInt abs() const;

     /**
     * Give size of internal register
     * @result size of internal register in words
     */
     size_t size() const { return get_reg().size(); }

     /**
     * Return how many words we need to hold this value
     * @result significant words of the represented integer value
     */
     size_t sig_words() const
        {
        const word* x = &reg[0];
        size_t sig = reg.size();

        while(sig && (x[sig-1] == 0))
           sig--;
        return sig;
        }

     /**
     * Give byte length of the integer
     * @result byte length of the represented integer value
     */
     size_t bytes() const;

     /**
     * Get the bit length of the integer
     * @result bit length of the represented integer value
     */
     size_t bits() const;

     /**
     * Return a pointer to the big integer word register
     * @result a pointer to the start of the internal register of
     * the integer value
     */
     const word* data() const { return &reg[0]; }

     /**
     * return a reference to the internal register containing the value
     * @result a reference to the word-array (SecureVector<word>)
     * with the internal register value (containing the integer
     * value)
     */
     SecureVector<word>& get_reg() { return reg; }

     /**
     * return a const reference to the internal register containing the value
     * @result a const reference to the word-array (SecureVector<word>)
     * with the internal register value (containing the integer value)
     */
     const SecureVector<word>& get_reg() const { return reg; }

     /**
     * Assign using a plain word array
     */
     void assign(const word x[], size_t length)
        {
        reg.resize(length);
        copy_mem(&reg[0], x, length);
        }

     /**
     * Increase internal register buffer by n words
     * @param n increase by n words
     */
     void grow_reg(size_t n);

     void grow_to(size_t n);

     /**
     * Fill BigInt with a random number with size of bitsize
     * @param rng the random number generator to use
     * @param bitsize number of bits the created random value should have
     */
     void randomize(RandomNumberGenerator& rng, size_t bitsize = 0);

     /**
     * Store BigInt-value in a given byte array
     * @param buf destination byte array for the integer value
     */
     void binary_encode(byte buf[]) const;

     /**
     * Read integer value from a byte array with given size
     * @param buf byte array buffer containing the integer
     * @param length size of buf
     */
     void binary_decode(const byte buf[], size_t length);

     /**
     * Read integer value from a byte array (MemoryRegion<byte>)
     * @param buf the array to load from
     */
     void binary_decode(const MemoryRegion<byte>& buf);

     /**
     * @param base the base to measure the size for
     * @return size of this integer in base base
     */
     size_t encoded_size(Base base = Binary) const;

     /**
     * @param rng a random number generator
     * @param min the minimum value
     * @param max the maximum value
     * @return random integer between min and max
     */
     static BigInt random_integer(RandomNumberGenerator& rng,
                                  const BigInt& min,
                                  const BigInt& max);

     /**
     * Encode the integer value from a BigInt to a SecureVector of bytes
     * @param n the BigInt to use as integer source
     * @param base number-base of resulting byte array representation
     * @result SecureVector of bytes containing the integer with given base
     */
     static SecureVector<byte> encode(const BigInt& n, Base base = Binary);

     /**
     * Encode the integer value from a BigInt to a byte array
     * @param buf destination byte array for the encoded integer
     * value with given base
     * @param n the BigInt to use as integer source
     * @param base number-base of resulting byte array representation
     */
     static void encode(byte buf[], const BigInt& n, Base base = Binary);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @param length size of buf
     * @param base number-base of the integer in buf
     * @result BigInt representing the integer in the byte array
     */
     static BigInt decode(const byte buf[], size_t length,
                          Base base = Binary);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @param base number-base of the integer in buf
     * @result BigInt representing the integer in the byte array
     */
     static BigInt decode(const MemoryRegion<byte>& buf,
                          Base base = Binary);

     /**
     * Encode a BigInt to a byte array according to IEEE 1363
     * @param n the BigInt to encode
     * @param bytes the length of the resulting SecureVector<byte>
     * @result a SecureVector<byte> containing the encoded BigInt
     */
     static SecureVector<byte> encode_1363(const BigInt& n, size_t bytes);

     /**
     * Swap this value with another
     * @param other BigInt to swap values with
     */
     void swap(BigInt& other);

     /**
     * Create empty BigInt
     */
     BigInt() { signedness = Positive; }

     /**
     * Create BigInt from 64 bit integer
     * @param n initial value of this BigInt
     */
     BigInt(u64bit n);

     /**
     * Copy Constructor
     * @param other the BigInt to copy
     */
     BigInt(const BigInt& other);

     /**
     * Create BigInt from a string. If the string starts with 0x the
     * rest of the string will be interpreted as hexadecimal digits.
     * If the string starts with 0 and the second character is NOT an
     * 'x' the string will be interpreted as octal digits. If the
     * string starts with non-zero digit, it will be interpreted as a
     * decimal number.
     *
     * @param str the string to parse for an integer value
     */
     BigInt(const std::string& str);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the byte array holding the value
     * @param length size of buf
     * @param base is the number base of the integer in buf
     */
     BigInt(const byte buf[], size_t length, Base base = Binary);

     /**
     * Create a random BigInt of the specified size
     * @param rng random number generator
     * @param bits size in bits
     */
     BigInt(RandomNumberGenerator& rng, size_t bits);

     /**
     * Create BigInt of specified size, all zeros
     * @param sign the sign
     * @param n size of the internal register in words
     */
     BigInt(Sign sign, size_t n);

     /**
     * Create a number of the specified type and size
     * @param type the type of number to create. For Power2,
     * will create the integer 2^n
     * @param n a size/length parameter, interpretation depends upon
     * the value of type
     */
     BigInt(NumberType type, size_t n);

   private:
      SecureVector<word> reg;
      Sign signedness;
   };

/*
* Arithmetic Operators
*/
BigInt BOTAN_DLL operator+(const BigInt& x, const BigInt& y);
BigInt BOTAN_DLL operator-(const BigInt& x, const BigInt& y);
BigInt BOTAN_DLL operator*(const BigInt& x, const BigInt& y);
BigInt BOTAN_DLL operator/(const BigInt& x, const BigInt& d);
BigInt BOTAN_DLL operator%(const BigInt& x, const BigInt& m);
word   BOTAN_DLL operator%(const BigInt& x, word m);
BigInt BOTAN_DLL operator<<(const BigInt& x, size_t n);
BigInt BOTAN_DLL operator>>(const BigInt& x, size_t n);

/*
* Comparison Operators
*/
inline bool operator==(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) == 0); }
inline bool operator!=(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) != 0); }
inline bool operator<=(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) <= 0); }
inline bool operator>=(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) >= 0); }
inline bool operator<(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) < 0); }
inline bool operator>(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) > 0); }

/*
* I/O Operators
*/
BOTAN_DLL std::ostream& operator<<(std::ostream&, const BigInt&);
BOTAN_DLL std::istream& operator>>(std::istream&, BigInt&);

}

namespace std {

template<>
inline void swap(Botan::BigInt& x, Botan::BigInt& y)
   {
   x.swap(y);
   }

}


namespace Botan {

/**
* Modular Exponentiator Interface
*/
class BOTAN_DLL Modular_Exponentiator
   {
   public:
      virtual void set_base(const BigInt&) = 0;
      virtual void set_exponent(const BigInt&) = 0;
      virtual BigInt execute() const = 0;
      virtual Modular_Exponentiator* copy() const = 0;
      virtual ~Modular_Exponentiator() {}
   };

/**
* Modular Exponentiator Proxy
*/
class BOTAN_DLL Power_Mod
   {
   public:

      enum Usage_Hints {
         NO_HINTS        = 0x0000,

         BASE_IS_FIXED   = 0x0001,
         BASE_IS_SMALL   = 0x0002,
         BASE_IS_LARGE   = 0x0004,
         BASE_IS_2       = 0x0008,

         EXP_IS_FIXED    = 0x0100,
         EXP_IS_SMALL    = 0x0200,
         EXP_IS_LARGE    = 0x0400
      };

      /*
      * Try to choose a good window size
      */
      static size_t window_bits(size_t exp_bits, size_t base_bits,
                                Power_Mod::Usage_Hints hints);

      void set_modulus(const BigInt&, Usage_Hints = NO_HINTS) const;
      void set_base(const BigInt&) const;
      void set_exponent(const BigInt&) const;

      BigInt execute() const;

      Power_Mod& operator=(const Power_Mod&);

      Power_Mod(const BigInt& = 0, Usage_Hints = NO_HINTS);
      Power_Mod(const Power_Mod&);
      virtual ~Power_Mod();
   private:
      mutable Modular_Exponentiator* core;
      Usage_Hints hints;
   };

/**
* Fixed Exponent Modular Exponentiator Proxy
*/
class BOTAN_DLL Fixed_Exponent_Power_Mod : public Power_Mod
   {
   public:
      BigInt operator()(const BigInt& b) const
         { set_base(b); return execute(); }

      Fixed_Exponent_Power_Mod() {}
      Fixed_Exponent_Power_Mod(const BigInt&, const BigInt&,
                               Usage_Hints = NO_HINTS);
   };

/**
* Fixed Base Modular Exponentiator Proxy
*/
class BOTAN_DLL Fixed_Base_Power_Mod : public Power_Mod
   {
   public:
      BigInt operator()(const BigInt& e) const
         { set_exponent(e); return execute(); }

      Fixed_Base_Power_Mod() {}
      Fixed_Base_Power_Mod(const BigInt&, const BigInt&,
                           Usage_Hints = NO_HINTS);
   };

}


namespace Botan {

/**
* ASN.1 Type and Class Tags
*/
enum ASN1_Tag {
   UNIVERSAL        = 0x00,
   APPLICATION      = 0x40,
   CONTEXT_SPECIFIC = 0x80,
   PRIVATE          = 0xC0,

   CONSTRUCTED      = 0x20,

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
   BMP_STRING       = 0x1E,

   UTC_TIME         = 0x17,
   GENERALIZED_TIME = 0x18,

   NO_OBJECT        = 0xFF00,
   DIRECTORY_STRING = 0xFF01
};

/**
* Basic ASN.1 Object Interface
*/
class BOTAN_DLL ASN1_Object
   {
   public:
      /**
      * Encode whatever this object is into to
      * @param to the DER_Encoder that will be written to
      */
      virtual void encode_into(class DER_Encoder& to) const = 0;

      /**
      * Decode whatever this object is from from
      * @param from the BER_Decoder that will be read from
      */
      virtual void decode_from(class BER_Decoder& from) = 0;

      virtual ~ASN1_Object() {}
   };

/**
* BER Encoded Object
*/
class BOTAN_DLL BER_Object
   {
   public:
      void assert_is_a(ASN1_Tag, ASN1_Tag);

      ASN1_Tag type_tag, class_tag;
      SecureVector<byte> value;
   };

/*
* ASN.1 Utility Functions
*/
class DataSource;

namespace ASN1 {

SecureVector<byte> put_in_sequence(const MemoryRegion<byte>& val);
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
struct BOTAN_DLL BER_Decoding_Error : public Decoding_Error
   {
   BER_Decoding_Error(const std::string&);
   };

/**
* Exception For Incorrect BER Taggings
*/
struct BOTAN_DLL BER_Bad_Tag : public BER_Decoding_Error
   {
   BER_Bad_Tag(const std::string& msg, ASN1_Tag tag);
   BER_Bad_Tag(const std::string& msg, ASN1_Tag tag1, ASN1_Tag tag2);
   };

}


namespace Botan {

/**
* This class represents ASN.1 object identifiers.
*/
class BOTAN_DLL OID : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      /**
      * Find out whether this OID is empty
      * @return true is no OID value is set
      */
      bool is_empty() const { return id.size() == 0; }

      /**
      * Get this OID as list (vector) of its components.
      * @return vector representing this OID
      */
      std::vector<u32bit> get_id() const { return id; }

      /**
      * Get this OID as a string
      * @return string representing this OID
      */
      std::string as_string() const;

      /**
      * Compare two OIDs.
      * @return true if they are equal, false otherwise
      */
      bool operator==(const OID&) const;

      /**
      * Reset this instance to an empty OID.
      */
      void clear();

      /**
      * Add a component to this OID.
      * @param new_comp the new component to add to the end of this OID
      * @return reference to *this
      */
      OID& operator+=(u32bit new_comp);

      /**
      * Construct an OID from a string.
      * @param str a string in the form "a.b.c" etc., where a,b,c are numbers
      */
      OID(const std::string& str = "");
   private:
      std::vector<u32bit> id;
   };

/**
* Append another component onto the OID.
* @param oid the OID to add the new component to
* @param new_comp the new component to add
*/
OID operator+(const OID& oid, u32bit new_comp);

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is not equal to b
*/
bool operator!=(const OID& a, const OID& b);

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is lexicographically smaller than b
*/
bool operator<(const OID& a, const OID& b);

}


namespace Botan {

/**
* Algorithm Identifier
*/
class BOTAN_DLL AlgorithmIdentifier : public ASN1_Object
   {
   public:
      enum Encoding_Option { USE_NULL_PARAM };

      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      AlgorithmIdentifier() {}
      AlgorithmIdentifier(const OID&, Encoding_Option);
      AlgorithmIdentifier(const std::string&, Encoding_Option);

      AlgorithmIdentifier(const OID&, const MemoryRegion<byte>&);
      AlgorithmIdentifier(const std::string&, const MemoryRegion<byte>&);

      OID oid;
      SecureVector<byte> parameters;
   };

/*
* Comparison Operations
*/
bool BOTAN_DLL operator==(const AlgorithmIdentifier&,
                          const AlgorithmIdentifier&);
bool BOTAN_DLL operator!=(const AlgorithmIdentifier&,
                          const AlgorithmIdentifier&);

}


namespace Botan {

/**
* Public Key Base Class.
*/
class BOTAN_DLL Public_Key
   {
   public:
      /**
      * Get the name of the underlying public key scheme.
      * @return name of the public key scheme
      */
      virtual std::string algo_name() const = 0;

      /**
      * Get the OID of the underlying public key scheme.
      * @return OID of the public key scheme
      */
      virtual OID get_oid() const;

      /**
      * Test the key values for consistency.
      * @param rng rng to use
      * @param strong whether to perform strong and lengthy version
      * of the test
      * @return true if the test is passed
      */
      virtual bool check_key(RandomNumberGenerator& rng,
                             bool strong) const = 0;

      /**
      * Find out the number of message parts supported by this scheme.
      * @return number of message parts
      */
      virtual size_t message_parts() const { return 1; }

      /**
      * Find out the message part size supported by this scheme/key.
      * @return size of the message parts in bits
      */
      virtual size_t message_part_size() const { return 0; }

      /**
      * Get the maximum message size in bits supported by this public key.
      * @return maximum message size in bits
      */
      virtual size_t max_input_bits() const = 0;

      /**
      * @return X.509 AlgorithmIdentifier for this key
      */
      virtual AlgorithmIdentifier algorithm_identifier() const = 0;

      /**
      * @return X.509 subject key encoding for this key object
      */
      virtual MemoryVector<byte> x509_subject_public_key() const = 0;

      virtual ~Public_Key() {}
   protected:
      /**
      * Self-test after loading a key
      * @param rng a random number generator
      */
      virtual void load_check(RandomNumberGenerator& rng) const;
   };

/**
* Private Key Base Class
*/
class BOTAN_DLL Private_Key : public virtual Public_Key
   {
   public:
      /**
      * @return PKCS #8 private key encoding for this key object
      */
      virtual MemoryVector<byte> pkcs8_private_key() const = 0;

      /**
      * @return PKCS #8 AlgorithmIdentifier for this key
      * Might be different from the X.509 identifier, but normally is not
      */
      virtual AlgorithmIdentifier pkcs8_algorithm_identifier() const
         { return algorithm_identifier(); }

   protected:
      /**
      * Self-test after loading a key
      * @param rng a random number generator
      */
      void load_check(RandomNumberGenerator& rng) const;

      /**
      * Self-test after generating a key
      * @param rng a random number generator
      */
      void gen_check(RandomNumberGenerator& rng) const;
   };

/**
* PK Secret Value Derivation Key
*/
class BOTAN_DLL PK_Key_Agreement_Key : public virtual Private_Key
   {
   public:
      /*
      * @return public component of this key
      */
      virtual MemoryVector<byte> public_value() const = 0;

      virtual ~PK_Key_Agreement_Key() {}
   };

/*
* Typedefs
*/
typedef PK_Key_Agreement_Key PK_KA_Key;
typedef Public_Key X509_PublicKey;
typedef Private_Key PKCS8_PrivateKey;

}


namespace Botan {

namespace PK_Ops {

/**
* Public key encryption interface
*/
class BOTAN_DLL Encryption
   {
   public:
      virtual size_t max_input_bits() const = 0;

      virtual SecureVector<byte> encrypt(const byte msg[], size_t msg_len,
                                         RandomNumberGenerator& rng) = 0;

      virtual ~Encryption() {}
   };

/**
* Public key decryption interface
*/
class BOTAN_DLL Decryption
   {
   public:
      virtual size_t max_input_bits() const = 0;

      virtual SecureVector<byte> decrypt(const byte msg[],
                                         size_t msg_len) = 0;

      virtual ~Decryption() {}
   };

/**
* Public key signature creation interface
*/
class BOTAN_DLL Signature
   {
   public:
      /**
      * Find out the number of message parts supported by this scheme.
      * @return number of message parts
      */
      virtual size_t message_parts() const { return 1; }

      /**
      * Find out the message part size supported by this scheme/key.
      * @return size of the message parts
      */
      virtual size_t message_part_size() const { return 0; }

      /**
      * Get the maximum message size in bits supported by this public key.
      * @return maximum message in bits
      */
      virtual size_t max_input_bits() const = 0;

      /*
      * Perform a signature operation
      * @param msg the message
      * @param msg_len the length of msg in bytes
      * @param rng a random number generator
      */
      virtual SecureVector<byte> sign(const byte msg[], size_t msg_len,
                                      RandomNumberGenerator& rng) = 0;

      virtual ~Signature() {}
   };

/**
* Public key signature verification interface
*/
class BOTAN_DLL Verification
   {
   public:
      /**
      * Get the maximum message size in bits supported by this public key.
      * @return maximum message in bits
      */
      virtual size_t max_input_bits() const = 0;

      /**
      * Find out the number of message parts supported by this scheme.
      * @return number of message parts
      */
      virtual size_t message_parts() const { return 1; }

      /**
      * Find out the message part size supported by this scheme/key.
      * @return size of the message parts
      */
      virtual size_t message_part_size() const { return 0; }

      /**
      * @return boolean specifying if this key type supports message
      * recovery and thus if you need to call verify() or verify_mr()
      */
      virtual bool with_recovery() const = 0;

      /*
      * Perform a signature check operation
      * @param msg the message
      * @param msg_len the length of msg in bytes
      * @param sig the signature
      * @param sig_len the length of sig in bytes
      * @returns if signature is a valid one for message
      */
      virtual bool verify(const byte[], size_t,
                          const byte[], size_t)
         {
         throw Invalid_State("Message recovery required");
         }

      /*
      * Perform a signature operation (with message recovery)
      * Only call this if with_recovery() returns true
      * @param msg the message
      * @param msg_len the length of msg in bytes
      * @returns recovered message
      */
      virtual SecureVector<byte> verify_mr(const byte[],
                                           size_t)
         {
         throw Invalid_State("Message recovery not supported");
         }

      virtual ~Verification() {}
   };

/**
* A generic key agreement Operation (eg DH or ECDH)
*/
class BOTAN_DLL Key_Agreement
   {
   public:
      /*
      * Perform a key agreement operation
      * @param w the other key value
      * @param w_len the length of w in bytes
      * @returns the agreed key
      */
      virtual SecureVector<byte> agree(const byte w[], size_t w_len) = 0;

      virtual ~Key_Agreement() {}
   };

}

}


namespace Botan {

class Algorithm_Factory;
class Keyed_Filter;

/**
* Base class for all engines. All non-pure virtual functions simply
* return NULL, indicating the algorithm in question is not
* supported. Subclasses can reimplement whichever function(s)
* they want to hook in a particular type.
*/
class BOTAN_DLL Engine
   {
   public:
      virtual ~Engine() {}

      /**
      * @return name of this engine
      */
      virtual std::string provider_name() const = 0;

      /**
      * @param algo_spec the algorithm name/specification
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual BlockCipher*
         find_block_cipher(const SCAN_Name& algo_spec,
                           Algorithm_Factory& af) const;

      /**
      * @param algo_spec the algorithm name/specification
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual StreamCipher*
         find_stream_cipher(const SCAN_Name& algo_spec,
                            Algorithm_Factory& af) const;

      /**
      * @param algo_spec the algorithm name/specification
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual HashFunction*
         find_hash(const SCAN_Name& algo_spec,
                   Algorithm_Factory& af) const;

      /**
      * @param algo_spec the algorithm name/specification
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual MessageAuthenticationCode*
         find_mac(const SCAN_Name& algo_spec,
                  Algorithm_Factory& af) const;

      /**
      * @param algo_spec the algorithm name/specification
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual PBKDF* find_pbkdf(const SCAN_Name& algo_spec,
                                Algorithm_Factory& af) const;

      /**
      * @param n the modulus
      * @param hints any use hints
      * @return newly allocated object, or NULL
      */
      virtual Modular_Exponentiator*
         mod_exp(const BigInt& n,
                 Power_Mod::Usage_Hints hints) const;

      /**
      * Return a new cipher object
      * @param algo_spec the algorithm name/specification
      * @param dir specifies if encryption or decryption is desired
      * @param af an algorithm factory object
      * @return newly allocated object, or NULL
      */
      virtual Keyed_Filter* get_cipher(const std::string& algo_spec,
                                       Cipher_Dir dir,
                                       Algorithm_Factory& af);

      /**
      * Return a new operator object for this key, if possible
      * @param key the key we want an operator for
      * @return newly allocated operator object, or NULL
      */
      virtual PK_Ops::Key_Agreement*
         get_key_agreement_op(const Private_Key& key) const;

      /**
      * Return a new operator object for this key, if possible
      * @param key the key we want an operator for
      * @return newly allocated operator object, or NULL
      */
      virtual PK_Ops::Signature*
         get_signature_op(const Private_Key& key) const;

      /**
      * Return a new operator object for this key, if possible
      * @param key the key we want an operator for
      * @return newly allocated operator object, or NULL
      */
      virtual PK_Ops::Verification*
         get_verify_op(const Public_Key& key) const;

      /**
      * Return a new operator object for this key, if possible
      * @param key the key we want an operator for
      * @return newly allocated operator object, or NULL
      */
      virtual PK_Ops::Encryption*
         get_encryption_op(const Public_Key& key) const;

      /**
      * Return a new operator object for this key, if possible
      * @param key the key we want an operator for
      * @return newly allocated operator object, or NULL
      */
      virtual PK_Ops::Decryption*
         get_decryption_op(const Private_Key& key) const;
   };

}


namespace Botan {

/**
* Dynamically_Loaded_Engine just proxies the requests to the underlying
* Engine object, and handles load/unload details
*/
class BOTAN_DLL Dynamically_Loaded_Engine : public Engine
   {
   public:
      /**
      * @param lib_path full pathname to DLL to load
      */
      Dynamically_Loaded_Engine(const std::string& lib_path);

      ~Dynamically_Loaded_Engine();

      std::string provider_name() const { return engine->provider_name(); }

      BlockCipher* find_block_cipher(const SCAN_Name& algo_spec,
                                     Algorithm_Factory& af) const
         {
         return engine->find_block_cipher(algo_spec, af);
         }

      StreamCipher* find_stream_cipher(const SCAN_Name& algo_spec,
                                       Algorithm_Factory& af) const
         {
         return engine->find_stream_cipher(algo_spec, af);
         }

      HashFunction* find_hash(const SCAN_Name& algo_spec,
                              Algorithm_Factory& af) const
         {
         return engine->find_hash(algo_spec, af);
         }

      MessageAuthenticationCode* find_mac(const SCAN_Name& algo_spec,
                                          Algorithm_Factory& af) const
         {
         return engine->find_mac(algo_spec, af);
         }

      PBKDF* find_pbkdf(const SCAN_Name& algo_spec,
                        Algorithm_Factory& af) const
         {
         return engine->find_pbkdf(algo_spec, af);
         }

      Modular_Exponentiator* mod_exp(const BigInt& n,
                                     Power_Mod::Usage_Hints hints) const
         {
         return engine->mod_exp(n, hints);
         }

      Keyed_Filter* get_cipher(const std::string& algo_spec,
                               Cipher_Dir dir,
                               Algorithm_Factory& af)
         {
         return engine->get_cipher(algo_spec, dir, af);
         }

      PK_Ops::Key_Agreement*
         get_key_agreement_op(const Private_Key& key) const
         {
         return engine->get_key_agreement_op(key);
         }

      PK_Ops::Signature*
         get_signature_op(const Private_Key& key) const
         {
         return engine->get_signature_op(key);
         }

      PK_Ops::Verification*
         get_verify_op(const Public_Key& key) const
         {
         return engine->get_verify_op(key);
         }

      PK_Ops::Encryption*
         get_encryption_op(const Public_Key& key) const
         {
         return engine->get_encryption_op(key);
         }

      PK_Ops::Decryption*
         get_decryption_op(const Private_Key& key) const
         {
         return engine->get_decryption_op(key);
         }

   private:
      class Dynamically_Loaded_Library* lib;
      Engine* engine;
   };

}


namespace Botan {

/**
* Simple String
*/
class BOTAN_DLL ASN1_String : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      std::string value() const;
      std::string iso_8859() const;

      ASN1_Tag tagging() const;

      ASN1_String(const std::string& = "");
      ASN1_String(const std::string&, ASN1_Tag);
   private:
      std::string iso_8859_str;
      ASN1_Tag tag;
   };

}


namespace Botan {

/**
* Attribute
*/
class BOTAN_DLL Attribute : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder& to) const;
      void decode_from(class BER_Decoder& from);

      OID oid;
      MemoryVector<byte> parameters;

      Attribute() {}
      Attribute(const OID&, const MemoryRegion<byte>&);
      Attribute(const std::string&, const MemoryRegion<byte>&);
   };

/**
* X.509 Time
*/
class BOTAN_DLL X509_Time : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      std::string as_string() const;
      std::string readable_string() const;
      bool time_is_set() const;

      s32bit cmp(const X509_Time&) const;

      void set_to(const std::string&);
      void set_to(const std::string&, ASN1_Tag);

      X509_Time(u64bit);
      X509_Time(const std::string& = "");
      X509_Time(const std::string&, ASN1_Tag);
   private:
      bool passes_sanity_check() const;
      u32bit year, month, day, hour, minute, second;
      ASN1_Tag tag;
   };

/**
* Alternative Name
*/
class BOTAN_DLL AlternativeName : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      std::multimap<std::string, std::string> contents() const;

      void add_attribute(const std::string&, const std::string&);
      std::multimap<std::string, std::string> get_attributes() const;

      void add_othername(const OID&, const std::string&, ASN1_Tag);
      std::multimap<OID, ASN1_String> get_othernames() const;

      bool has_items() const;

      AlternativeName(const std::string& = "", const std::string& = "",
                      const std::string& = "", const std::string& = "");
   private:
      std::multimap<std::string, std::string> alt_info;
      std::multimap<OID, ASN1_String> othernames;
   };

/*
* Comparison Operations
*/
bool BOTAN_DLL operator==(const X509_Time&, const X509_Time&);
bool BOTAN_DLL operator!=(const X509_Time&, const X509_Time&);
bool BOTAN_DLL operator<=(const X509_Time&, const X509_Time&);
bool BOTAN_DLL operator>=(const X509_Time&, const X509_Time&);
bool BOTAN_DLL operator<(const X509_Time&, const X509_Time&);
bool BOTAN_DLL operator>(const X509_Time&, const X509_Time&);

}


namespace Botan {

/**
* This class represents an abstract data source object.
*/
class BOTAN_DLL DataSource
   {
   public:
      /**
      * Read from the source. Moves the internal offset so that every
      * call to read will return a new portion of the source.
      *
      * @param out the byte array to write the result to
      * @param length the length of the byte array out
      * @return length in bytes that was actually read and put
      * into out
      */
      virtual size_t read(byte out[], size_t length) = 0;

      /**
      * Read from the source but do not modify the internal
      * offset. Consecutive calls to peek() will return portions of
      * the source starting at the same position.
      *
      * @param out the byte array to write the output to
      * @param length the length of the byte array out
      * @param peek_offset the offset into the stream to read at
      * @return length in bytes that was actually read and put
      * into out
      */
      virtual size_t peek(byte out[], size_t length,
                          size_t peek_offset) const = 0;

      /**
      * Test whether the source still has data that can be read.
      * @return true if there is still data to read, false otherwise
      */
      virtual bool end_of_data() const = 0;
      /**
      * return the id of this data source
      * @return std::string representing the id of this data source
      */
      virtual std::string id() const { return ""; }

      /**
      * Read one byte.
      * @param out the byte to read to
      * @return length in bytes that was actually read and put
      * into out
      */
      size_t read_byte(byte& out);

      /**
      * Peek at one byte.
      * @param out an output byte
      * @return length in bytes that was actually read and put
      * into out
      */
      size_t peek_byte(byte& out) const;

      /**
      * Discard the next N bytes of the data
      * @param N the number of bytes to discard
      * @return number of bytes actually discarded
      */
      size_t discard_next(size_t N);

      DataSource() {}
      virtual ~DataSource() {}
   private:
      DataSource& operator=(const DataSource&) { return (*this); }
      DataSource(const DataSource&);
   };

/**
* This class represents a Memory-Based DataSource
*/
class BOTAN_DLL DataSource_Memory : public DataSource
   {
   public:
      size_t read(byte[], size_t);
      size_t peek(byte[], size_t, size_t) const;
      bool end_of_data() const;

      /**
      * Construct a memory source that reads from a string
      * @param in the string to read from
      */
      DataSource_Memory(const std::string& in);

      /**
      * Construct a memory source that reads from a byte array
      * @param in the byte array to read from
      * @param length the length of the byte array
      */
      DataSource_Memory(const byte in[], size_t length);

      /**
      * Construct a memory source that reads from a MemoryRegion
      * @param in the MemoryRegion to read from
      */
      DataSource_Memory(const MemoryRegion<byte>& in);
   private:
      SecureVector<byte> source;
      size_t offset;
   };

/**
* This class represents a Stream-Based DataSource.
*/
class BOTAN_DLL DataSource_Stream : public DataSource
   {
   public:
      size_t read(byte[], size_t);
      size_t peek(byte[], size_t, size_t) const;
      bool end_of_data() const;
      std::string id() const;

      DataSource_Stream(std::istream&,
                        const std::string& id = "<std::istream>");

      /**
      * Construct a Stream-Based DataSource from file
      * @param file the name of the file
      * @param use_binary whether to treat the file as binary or not
      */
      DataSource_Stream(const std::string& file, bool use_binary = false);

      ~DataSource_Stream();
   private:
      const std::string identifier;

      std::istream* source_p;
      std::istream& source;
      size_t total_read;
   };

}


namespace Botan {

/**
* This class represents general abstract filter objects.
*/
class BOTAN_DLL Filter
   {
   public:
      /**
      * @return descriptive name for this filter
      */
      virtual std::string name() const = 0;

      /**
      * Write a portion of a message to this filter.
      * @param input the input as a byte array
      * @param length the length of the byte array input
      */
      virtual void write(const byte input[], size_t length) = 0;

      /**
      * Start a new message. Must be closed by end_msg() before another
      * message can be started.
      */
      virtual void start_msg() {}

      /**
      * Notify that the current message is finished; flush buffers and
      * do end-of-message processing (if any).
      */
      virtual void end_msg() {}

      /**
      * Check whether this filter is an attachable filter.
      * @return true if this filter is attachable, false otherwise
      */
      virtual bool attachable() { return true; }

      virtual ~Filter() {}
   protected:
      /**
      * @param in some input for the filter
      * @param length the length of in
      */
      void send(const byte in[], size_t length);

      /**
      * @param in some input for the filter
      */
      void send(byte in) { send(&in, 1); }

      /**
      * @param in some input for the filter
      */
      void send(const MemoryRegion<byte>& in) { send(&in[0], in.size()); }

      /**
      * @param in some input for the filter
      * @param length the number of bytes of in to send
      */
      void send(const MemoryRegion<byte>& in, size_t length)
         {
         send(&in[0], length);
         }

      Filter();
   private:
      Filter(const Filter&) {}
      Filter& operator=(const Filter&) { return (*this); }

      /**
      * Start a new message in *this and all following filters. Only for
      * internal use, not intended for use in client applications.
      */
      void new_msg();

      /**
      * End a new message in *this and all following filters. Only for
      * internal use, not intended for use in client applications.
      */
      void finish_msg();

      friend class Pipe;
      friend class Fanout_Filter;

      size_t total_ports() const;
      size_t current_port() const { return port_num; }

      /**
      * Set the active port
      * @param new_port the new value
      */
      void set_port(size_t new_port);

      size_t owns() const { return filter_owns; }

      /**
      * Attach another filter to this one
      * @param f filter to attach
      */
      void attach(Filter* f);

      /**
      * @param filters the filters to set
      * @param count number of items in filters
      */
      void set_next(Filter* filters[], size_t count);
      Filter* get_next() const;

      SecureVector<byte> write_queue;
      std::vector<Filter*> next;
      size_t port_num, filter_owns;

      // true if filter belongs to a pipe --> prohibit filter sharing!
      bool owned;
   };

/**
* This is the abstract Fanout_Filter base class.
**/
class BOTAN_DLL Fanout_Filter : public Filter
   {
   protected:
      /**
      * Increment the number of filters past us that we own
      */
      void incr_owns() { ++filter_owns; }

      void set_port(size_t n) { Filter::set_port(n); }

      void set_next(Filter* f[], size_t n) { Filter::set_next(f, n); }

      void attach(Filter* f) { Filter::attach(f); }
   };

/**
* The type of checking to be performed by decoders:
* NONE - no checks, IGNORE_WS - perform checks, but ignore
* whitespaces, FULL_CHECK - perform checks, also complain
* about white spaces.
*/
enum Decoder_Checking { NONE, IGNORE_WS, FULL_CHECK };

}


namespace Botan {

/**
* This class represents pipe objects.
* A set of filters can be placed into a pipe, and information flows
* through the pipe until it reaches the end, where the output is
* collected for retrieval.  If you're familiar with the Unix shell
* environment, this design will sound quite familiar.
*/
class BOTAN_DLL Pipe : public DataSource
   {
   public:
      /**
      * An opaque type that identifies a message in this Pipe
      */
      typedef size_t message_id;

      /**
      * Exception if you use an invalid message as an argument to
      * read, remaining, etc
      */
      struct BOTAN_DLL Invalid_Message_Number : public Invalid_Argument
         {
         /**
         * @param where the error occured
         * @param msg the invalid message id that was used
         */
         Invalid_Message_Number(const std::string& where, message_id msg) :
            Invalid_Argument("Pipe::" + where + ": Invalid message number " +
                             to_string(msg))
            {}
         };

      /**
      * A meta-id for whatever the last message is
      */
      static const message_id LAST_MESSAGE;

      /**
      * A meta-id for the default message (set with set_default_msg)
      */
      static const message_id DEFAULT_MESSAGE;

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the byte array to write
      * @param length the length of the byte array in
      */
      void write(const byte in[], size_t length);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the MemoryRegion containing the data to write
      */
      void write(const MemoryRegion<byte>& in);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the string containing the data to write
      */
      void write(const std::string& in);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the DataSource to read the data from
      */
      void write(DataSource& in);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in a single byte to be written
      */
      void write(byte in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the byte array containing the data to write
      * @param length the length of the byte array to write
      */
      void process_msg(const byte in[], size_t length);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the MemoryRegion containing the data to write
      */
      void process_msg(const MemoryRegion<byte>& in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the string containing the data to write
      */
      void process_msg(const std::string& in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the DataSource providing the data to write
      */
      void process_msg(DataSource& in);

      /**
      * Find out how many bytes are ready to read.
      * @param msg the number identifying the message
      * for which the information is desired
      * @return number of bytes that can still be read
      */
      size_t remaining(message_id msg = DEFAULT_MESSAGE) const;

      /**
      * Read the default message from the pipe. Moves the internal
      * offset so that every call to read will return a new portion of
      * the message.
      *
      * @param output the byte array to write the read bytes to
      * @param length the length of the byte array output
      * @return number of bytes actually read into output
      */
      size_t read(byte output[], size_t length);

      /**
      * Read a specified message from the pipe. Moves the internal
      * offset so that every call to read will return a new portion of
      * the message.
      * @param output the byte array to write the read bytes to
      * @param length the length of the byte array output
      * @param msg the number identifying the message to read from
      * @return number of bytes actually read into output
      */
      size_t read(byte output[], size_t length, message_id msg);

      /**
      * Read a single byte from the pipe. Moves the internal offset so
      * that every call to read will return a new portion of the
      * message.
      *
      * @param output the byte to write the result to
      * @param msg the message to read from
      * @return number of bytes actually read into output
      */
      size_t read(byte& output, message_id msg = DEFAULT_MESSAGE);

      /**
      * Read the full contents of the pipe.
      * @param msg the number identifying the message to read from
      * @return SecureVector holding the contents of the pipe
      */
      SecureVector<byte> read_all(message_id msg = DEFAULT_MESSAGE);

      /**
      * Read the full contents of the pipe.
      * @param msg the number identifying the message to read from
      * @return string holding the contents of the pipe
      */
      std::string read_all_as_string(message_id = DEFAULT_MESSAGE);

      /** Read from the default message but do not modify the internal
      * offset. Consecutive calls to peek() will return portions of
      * the message starting at the same position.
      * @param output the byte array to write the peeked message part to
      * @param length the length of the byte array output
      * @param offset the offset from the current position in message
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(byte output[], size_t length, size_t offset) const;

      /** Read from the specified message but do not modify the
      * internal offset. Consecutive calls to peek() will return
      * portions of the message starting at the same position.
      * @param output the byte array to write the peeked message part to
      * @param length the length of the byte array output
      * @param offset the offset from the current position in message
      * @param msg the number identifying the message to peek from
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(byte output[], size_t length,
                  size_t offset, message_id msg) const;

      /** Read a single byte from the specified message but do not
      * modify the internal offset. Consecutive calls to peek() will
      * return portions of the message starting at the same position.
      * @param output the byte to write the peeked message byte to
      * @param offset the offset from the current position in message
      * @param msg the number identifying the message to peek from
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(byte& output, size_t offset,
                  message_id msg = DEFAULT_MESSAGE) const;

      /**
      * @return currently set default message
      */
      size_t default_msg() const { return default_read; }

      /**
      * Set the default message
      * @param msg the number identifying the message which is going to
      * be the new default message
      */
      void set_default_msg(message_id msg);

      /**
      * Get the number of messages the are in this pipe.
      * @return number of messages the are in this pipe
      */
      message_id message_count() const;

      /**
      * Test whether this pipe has any data that can be read from.
      * @return true if there is more data to read, false otherwise
      */
      bool end_of_data() const;

      /**
      * Start a new message in the pipe. A potential other message in this pipe
      * must be closed with end_msg() before this function may be called.
      */
      void start_msg();

      /**
      * End the current message.
      */
      void end_msg();

      /**
      * Insert a new filter at the front of the pipe
      * @param filt the new filter to insert
      */
      void prepend(Filter* filt);

      /**
      * Insert a new filter at the back of the pipe
      * @param filt the new filter to insert
      */
      void append(Filter* filt);

      /**
      * Remove the first filter at the front of the pipe.
      */
      void pop();

      /**
      * Reset this pipe to an empty pipe.
      */
      void reset();

      /**
      * Construct a Pipe of up to four filters. The filters are set up
      * in the same order as the arguments.
      */
      Pipe(Filter* = 0, Filter* = 0, Filter* = 0, Filter* = 0);

      /**
      * Construct a Pipe from range of filters passed as an array
      * @param filters the set of filters to use
      * @param count the number of elements in filters
      */
      Pipe(Filter* filters[], size_t count);
      ~Pipe();
   private:
      Pipe(const Pipe&) : DataSource() {}
      Pipe& operator=(const Pipe&) { return (*this); }
      void init();
      void destruct(Filter*);
      void find_endpoints(Filter*);
      void clear_endpoints(Filter*);

      message_id get_message_no(const std::string&, message_id) const;

      Filter* pipe;
      class Output_Buffers* outputs;
      message_id default_read;
      bool inside_msg;
   };

/**
* Stream output operator; dumps the results from pipe's default
* message to the output stream.
* @param out an output stream
* @param pipe the pipe
*/
BOTAN_DLL std::ostream& operator<<(std::ostream& out, Pipe& pipe);

/**
* Stream input operator; dumps the remaining bytes of input
* to the (assumed open) pipe message.
* @param in the input stream
* @param pipe the pipe
*/
BOTAN_DLL std::istream& operator>>(std::istream& in, Pipe& pipe);

}

#if defined(BOTAN_HAS_PIPE_UNIXFD_IO)

namespace Botan {

/**
* Stream output operator; dumps the results from pipe's default
* message to the output stream.
* @param out file descriptor for an open output stream
* @param pipe the pipe
*/
int BOTAN_DLL operator<<(int out, Pipe& pipe);

/**
* File descriptor input operator; dumps the remaining bytes of input
* to the (assumed open) pipe message.
* @param in file descriptor for an open input stream
* @param pipe the pipe
*/
int BOTAN_DLL operator>>(int in, Pipe& pipe);

}

#endif


namespace Botan {

/**
* BER Decoding Object
*/
class BOTAN_DLL BER_Decoder
   {
   public:
      BER_Object get_next_object();
      void push_back(const BER_Object&);

      bool more_items() const;
      BER_Decoder& verify_end();
      BER_Decoder& discard_remaining();

      BER_Decoder  start_cons(ASN1_Tag, ASN1_Tag = UNIVERSAL);
      BER_Decoder& end_cons();

      BER_Decoder& raw_bytes(MemoryRegion<byte>&);

      BER_Decoder& decode_null();
      BER_Decoder& decode(bool&);
      BER_Decoder& decode(size_t&);
      BER_Decoder& decode(class BigInt&);
      BER_Decoder& decode(MemoryRegion<byte>&, ASN1_Tag);

      BER_Decoder& decode(bool&, ASN1_Tag, ASN1_Tag = CONTEXT_SPECIFIC);
      BER_Decoder& decode(size_t&, ASN1_Tag, ASN1_Tag = CONTEXT_SPECIFIC);
      BER_Decoder& decode(class BigInt&,
                          ASN1_Tag, ASN1_Tag = CONTEXT_SPECIFIC);
      BER_Decoder& decode(MemoryRegion<byte>&, ASN1_Tag,
                          ASN1_Tag, ASN1_Tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(class ASN1_Object&);

      BER_Decoder& decode_octet_string_bigint(class BigInt&);

      template<typename T>
         BER_Decoder& decode_optional(T& out,
                                      ASN1_Tag type_tag,
                                      ASN1_Tag class_tag,
                                      const T& default_value = T());

      template<typename T>
         BER_Decoder& decode_list(std::vector<T>& out,
                                  bool clear_out = true);

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

      BER_Decoder& decode_optional_string(MemoryRegion<byte>&,
                                          ASN1_Tag, u16bit);

      BER_Decoder(DataSource&);
      BER_Decoder(const byte[], size_t);
      BER_Decoder(const MemoryRegion<byte>&);
      BER_Decoder(const BER_Decoder&);
      ~BER_Decoder();
   private:
      BER_Decoder& operator=(const BER_Decoder&) { return (*this); }

      BER_Decoder* parent;
      DataSource* source;
      BER_Object pushed;
      mutable bool owns;
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

   if(obj.type_tag == type_tag && obj.class_tag == class_tag)
      {
      if(class_tag & CONSTRUCTED)
         BER_Decoder(obj.value).decode(out).verify_end();
      else
         {
         push_back(obj);
         decode(out, type_tag, class_tag);
         }
      }
   else
      {
      out = default_value;
      push_back(obj);
      }

   return (*this);
   }

/*
* Decode a list of homogenously typed values
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_list(std::vector<T>& vec, bool clear_it)
   {
   if(clear_it)
      vec.clear();

   while(more_items())
      {
      T value;
      decode(value);
      vec.push_back(value);
      }
   return (*this);
   }

}


namespace Botan {

/**
* X.509v3 Key Constraints.
*/
enum Key_Constraints {
   NO_CONSTRAINTS     = 0,
   DIGITAL_SIGNATURE  = 32768,
   NON_REPUDIATION    = 16384,
   KEY_ENCIPHERMENT   = 8192,
   DATA_ENCIPHERMENT  = 4096,
   KEY_AGREEMENT      = 2048,
   KEY_CERT_SIGN      = 1024,
   CRL_SIGN           = 512,
   ENCIPHER_ONLY      = 256,
   DECIPHER_ONLY      = 128
};

/**
* BER Decoding Function for key constraints
*/
namespace BER {

void BOTAN_DLL decode(BER_Decoder&, Key_Constraints&);

}

/**
* X.509v2 CRL Reason Code.
*/
enum CRL_Code {
   UNSPECIFIED            = 0,
   KEY_COMPROMISE         = 1,
   CA_COMPROMISE          = 2,
   AFFILIATION_CHANGED    = 3,
   SUPERSEDED             = 4,
   CESSATION_OF_OPERATION = 5,
   CERTIFICATE_HOLD       = 6,
   REMOVE_FROM_CRL        = 8,
   PRIVLEDGE_WITHDRAWN    = 9,
   AA_COMPROMISE          = 10,

   DELETE_CRL_ENTRY       = 0xFF00,
   OCSP_GOOD              = 0xFF01,
   OCSP_UNKNOWN           = 0xFF02
};

/*
* Various Other Enumerations
*/

/**
* The two types of X509 encoding supported by Botan.
*/
enum X509_Encoding { RAW_BER, PEM };

}


namespace Botan {

/**
* This class represents abstract X.509 signed objects as
* in the X.500 SIGNED macro
*/
class BOTAN_DLL X509_Object
   {
   public:
      /**
      * The underlying data that is to be or was signed
      * @return data that is or was signed
      */
      MemoryVector<byte> tbs_data() const;

      /**
      * @return signature on tbs_data()
      */
      MemoryVector<byte> signature() const;

      /**
      * @return signature algorithm that was used to generate signature
      */
      AlgorithmIdentifier signature_algorithm() const;

      /**
      * @return hash algorithm that was used to generate signature
      */
      std::string hash_used_for_signature() const;

      /**
      * Create a signed X509 object.
      * @param signer the signer used to sign the object
      * @param rng the random number generator to use
      * @param alg_id the algorithm identifier of the signature scheme
      * @param tbs the tbs bits to be signed
      * @return signed X509 object
      */
      static MemoryVector<byte> make_signed(class PK_Signer* signer,
                                            RandomNumberGenerator& rng,
                                            const AlgorithmIdentifier& alg_id,
                                            const MemoryRegion<byte>& tbs);

      /**
      * Check the signature on this data
      * @param key the public key purportedly used to sign this data
      * @return true if the signature is valid, otherwise false
      */
      bool check_signature(class Public_Key& key) const;

      /**
      * Check the signature on this data
      * @param key the public key purportedly used to sign this data
      *        the pointer will be deleted after use
      * @return true if the signature is valid, otherwise false
      */
      bool check_signature(class Public_Key* key) const;

      /**
      * @return BER encoding of this
      */
      MemoryVector<byte> BER_encode() const;

      /**
      * @return PEM encoding of this
      */
      std::string PEM_encode() const;

      /**
      * Encode this to a pipe
      * @deprecated use BER_encode or PEM_encode instead
      * @param out the pipe to write to
      * @param encoding the encoding to use
      */
      BOTAN_DEPRECATED("Use BER_encode or PEM_encode")
      void encode(Pipe& out, X509_Encoding encoding = PEM) const;

      virtual ~X509_Object() {}
   protected:
      X509_Object(DataSource& src, const std::string& pem_labels);
      X509_Object(const std::string& file, const std::string& pem_labels);

      void do_decode();
      X509_Object() {}
      AlgorithmIdentifier sig_algo;
      MemoryVector<byte> tbs_bits, sig;
   private:
      virtual void force_decode() = 0;
      void init(DataSource&, const std::string&);
      void decode_info(DataSource&);
      std::vector<std::string> PEM_labels_allowed;
      std::string PEM_label_pref;
   };

}


namespace Botan {

/**
* Distinguished Name
*/
class BOTAN_DLL X509_DN : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      std::multimap<OID, std::string> get_attributes() const;
      std::vector<std::string> get_attribute(const std::string&) const;

      std::multimap<std::string, std::string> contents() const;

      void add_attribute(const std::string&, const std::string&);
      void add_attribute(const OID&, const std::string&);

      static std::string deref_info_field(const std::string&);

      MemoryVector<byte> get_bits() const;

      X509_DN();
      X509_DN(const std::multimap<OID, std::string>&);
      X509_DN(const std::multimap<std::string, std::string>&);
   private:
      std::multimap<OID, ASN1_String> dn_info;
      MemoryVector<byte> dn_bits;
   };

bool BOTAN_DLL operator==(const X509_DN&, const X509_DN&);
bool BOTAN_DLL operator!=(const X509_DN&, const X509_DN&);
bool BOTAN_DLL operator<(const X509_DN&, const X509_DN&);

}


namespace Botan {

/**
* This namespace contains functions for handling X.509 public keys
*/
namespace X509 {

/**
* BER encode a key
* @param key the public key to encode
* @return BER encoding of this key
*/
BOTAN_DLL MemoryVector<byte> BER_encode(const Public_Key& key);

/**
* PEM encode a public key into a string.
* @param key the key to encode
* @return PEM encoded key
*/
BOTAN_DLL std::string PEM_encode(const Public_Key& key);

/**
* Create a public key from a data source.
* @param source the source providing the DER or PEM encoded key
* @return new public key object
*/
BOTAN_DLL Public_Key* load_key(DataSource& source);

/**
* Create a public key from a file
* @param filename pathname to the file to load
* @return new public key object
*/
BOTAN_DLL Public_Key* load_key(const std::string& filename);

/**
* Create a public key from a memory region.
* @param enc the memory region containing the DER or PEM encoded key
* @return new public key object
*/
BOTAN_DLL Public_Key* load_key(const MemoryRegion<byte>& enc);

/**
* Copy a key.
* @param key the public key to copy
* @return new public key object
*/
BOTAN_DLL Public_Key* copy_key(const Public_Key& key);

/**
* Create the key constraints for a specific public key.
* @param pub_key the public key from which the basic set of
* constraints to be placed in the return value is derived
* @param limits additional limits that will be incorporated into the
* return value
* @return combination of key type specific constraints and
* additional limits
*/
BOTAN_DLL Key_Constraints find_constraints(const Public_Key& pub_key,
                                           Key_Constraints limits);

/**
* Encode a key into a pipe.
* @deprecated Use PEM_encode or BER_encode instead
*
* @param key the public key to encode
* @param pipe the pipe to feed the encoded key into
* @param encoding the encoding type to use
*/
BOTAN_DEPRECATED("Use PEM_encode or BER_encode")
inline void encode(const Public_Key& key,
                   Pipe& pipe,
                   X509_Encoding encoding = PEM)
   {
   if(encoding == PEM)
      pipe.write(X509::PEM_encode(key));
   else
      pipe.write(X509::BER_encode(key));
   }

}

}


namespace Botan {

/**
* Data Store
*/
class BOTAN_DLL Data_Store
   {
   public:
      /**
      * A search function
      */
      class BOTAN_DLL Matcher
         {
         public:
            virtual bool operator()(const std::string&,
                                    const std::string&) const = 0;

            virtual std::pair<std::string, std::string>
               transform(const std::string&, const std::string&) const;

            virtual ~Matcher() {}
         };

      bool operator==(const Data_Store&) const;

      std::multimap<std::string, std::string>
         search_with(const Matcher&) const;

      std::vector<std::string> get(const std::string&) const;

      std::string get1(const std::string&) const;

      MemoryVector<byte> get1_memvec(const std::string&) const;
      u32bit get1_u32bit(const std::string&, u32bit = 0) const;

      bool has_value(const std::string&) const;

      void add(const std::multimap<std::string, std::string>&);
      void add(const std::string&, const std::string&);
      void add(const std::string&, u32bit);
      void add(const std::string&, const MemoryRegion<byte>&);
   private:
      std::multimap<std::string, std::string> contents;
   };

}


namespace Botan {

/**
* This class represents X.509 Certificate
*/
class BOTAN_DLL X509_Certificate : public X509_Object
   {
   public:
     /**
     * Get the public key associated with this certificate.
     * @return subject public key of this certificate
     */
      Public_Key* subject_public_key() const;

      /**
      * Get the issuer certificate DN.
      * @return issuer DN of this certificate
      */
      X509_DN issuer_dn() const;

      /**
      * Get the subject certificate DN.
      * @return subject DN of this certificate
      */
      X509_DN subject_dn() const;

      /**
      * Get a value for a specific subject_info parameter name.
      * @param name the name of the paramter to look up. Possible names are
      * "X509.Certificate.version", "X509.Certificate.serial",
      * "X509.Certificate.start", "X509.Certificate.end",
      * "X509.Certificate.v2.key_id", "X509.Certificate.public_key",
      * "X509v3.BasicConstraints.path_constraint",
      * "X509v3.BasicConstraints.is_ca", "X509v3.ExtendedKeyUsage",
      * "X509v3.CertificatePolicies", "X509v3.SubjectKeyIdentifier" or
      * "X509.Certificate.serial".
      * @return value(s) of the specified parameter
      */
      std::vector<std::string> subject_info(const std::string& name) const;

      /**
      * Get a value for a specific subject_info parameter name.
      * @param name the name of the paramter to look up. Possible names are
      * "X509.Certificate.v2.key_id" or "X509v3.AuthorityKeyIdentifier".
      * @return value(s) of the specified parameter
      */
      std::vector<std::string> issuer_info(const std::string& name) const;

      /**
      * Get the notBefore of the certificate.
      * @return notBefore of the certificate
      */
      std::string start_time() const;

      /**
      * Get the notAfter of the certificate.
      * @return notAfter of the certificate
      */
      std::string end_time() const;

      /**
      * Get the X509 version of this certificate object.
      * @return X509 version
      */
      u32bit x509_version() const;

      /**
      * Get the serial number of this certificate.
      * @return certificates serial number
      */
      MemoryVector<byte> serial_number() const;

      /**
      * Get the DER encoded AuthorityKeyIdentifier of this certificate.
      * @return DER encoded AuthorityKeyIdentifier
      */
      MemoryVector<byte> authority_key_id() const;

      /**
      * Get the DER encoded SubjectKeyIdentifier of this certificate.
      * @return DER encoded SubjectKeyIdentifier
      */
      MemoryVector<byte> subject_key_id() const;

      /**
      * Check whether this certificate is self signed.
      * @return true if this certificate is self signed
      */
      bool is_self_signed() const { return self_signed; }

      /**
      * Check whether this certificate is a CA certificate.
      * @return true if this certificate is a CA certificate
      */
      bool is_CA_cert() const;

      /**
      * Get the path limit as defined in the BasicConstraints extension of
      * this certificate.
      * @return path limit
      */
      u32bit path_limit() const;

      /**
      * Get the key constraints as defined in the KeyUsage extension of this
      * certificate.
      * @return key constraints
      */
      Key_Constraints constraints() const;

      /**
      * Get the key constraints as defined in the ExtendedKeyUsage
      * extension of this
      * certificate.
      * @return key constraints
      */
      std::vector<std::string> ex_constraints() const;

      /**
      * Get the policies as defined in the CertificatePolicies extension
      * of this certificate.
      * @return certificate policies
      */
      std::vector<std::string> policies() const;

      /**
      * @return a string describing the certificate
      */
      std::string to_string() const;

      /**
      * Check to certificates for equality.
      * @return true both certificates are (binary) equal
      */
      bool operator==(const X509_Certificate& other) const;

      /**
      * Create a certificate from a data source providing the DER or
      * PEM encoded certificate.
      * @param source the data source
      */
      X509_Certificate(DataSource& source);

      /**
      * Create a certificate from a file containing the DER or PEM
      * encoded certificate.
      * @param filename the name of the certificate file
      */
      X509_Certificate(const std::string& filename);
   private:
      void force_decode();
      friend class X509_CA;
      X509_Certificate() {}

      Data_Store subject, issuer;
      bool self_signed;
   };

/**
* Check two certificates for inequality
* @return true if the arguments represent different certificates,
* false if they are binary identical
*/
BOTAN_DLL bool operator!=(const X509_Certificate&, const X509_Certificate&);

/*
* Data Store Extraction Operations
*/
BOTAN_DLL X509_DN create_dn(const Data_Store&);
BOTAN_DLL AlternativeName create_alt_name(const Data_Store&);

}


namespace Botan {

/**
* This class represents CRL entries
*/
class BOTAN_DLL CRL_Entry : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      /**
      * Get the serial number of the certificate associated with this entry.
      * @return certificate's serial number
      */
      MemoryVector<byte> serial_number() const { return serial; }

      /**
      * Get the revocation date of the certificate associated with this entry
      * @return certificate's revocation date
      */
      X509_Time expire_time() const { return time; }

      /**
      * Get the entries reason code
      * @return reason code
      */
      CRL_Code reason_code() const { return reason; }

      /**
      * Construct an empty CRL entry.
      */
      CRL_Entry(bool throw_on_unknown_critical_extension = false);

      /**
      * Construct an CRL entry.
      * @param cert the certificate to revoke
      * @param reason the reason code to set in the entry
      */
      CRL_Entry(const X509_Certificate& cert,
                CRL_Code reason = UNSPECIFIED);

   private:
      bool throw_on_unknown_critical;
      MemoryVector<byte> serial;
      X509_Time time;
      CRL_Code reason;
   };

/**
* Test two CRL entries for equality in all fields.
*/
BOTAN_DLL bool operator==(const CRL_Entry&, const CRL_Entry&);

/**
* Test two CRL entries for inequality in at least one field.
*/
BOTAN_DLL bool operator!=(const CRL_Entry&, const CRL_Entry&);

}


namespace Botan {

/**
* This class represents X.509 Certificate Revocation Lists (CRLs).
*/
class BOTAN_DLL X509_CRL : public X509_Object
   {
   public:
      /**
      * This class represents CRL related errors.
      */
      struct BOTAN_DLL X509_CRL_Error : public Exception
         {
         X509_CRL_Error(const std::string& error) :
            Exception("X509_CRL: " + error) {}
         };

      /**
      * Get the entries of this CRL in the form of a vector.
      * @return vector containing the entries of this CRL.
      */
      std::vector<CRL_Entry> get_revoked() const;

      /**
      * Get the issuer DN of this CRL.
      * @return CRLs issuer DN
      */
      X509_DN issuer_dn() const;

      /**
      * Get the AuthorityKeyIdentifier of this CRL.
      * @return this CRLs AuthorityKeyIdentifier
      */
      MemoryVector<byte> authority_key_id() const;

      /**
      * Get the serial number of this CRL.
      * @return CRLs serial number
      */
      u32bit crl_number() const;

      /**
      * Get the CRL's thisUpdate value.
      * @return CRLs thisUpdate
      */
      X509_Time this_update() const;

      /**
      * Get the CRL's nextUpdate value.
      * @return CRLs nextdUpdate
      */
      X509_Time next_update() const;

      /**
      * Construct a CRL from a data source.
      * @param source the data source providing the DER or PEM encoded CRL.
      * @param throw_on_unknown_critical should we throw an exception
      * if an unknown CRL extension marked as critical is encountered.
      */
      X509_CRL(DataSource& source, bool throw_on_unknown_critical = false);

      /**
      * Construct a CRL from a file containing the DER or PEM encoded CRL.
      * @param filename the name of the CRL file
      * @param throw_on_unknown_critical should we throw an exception
      * if an unknown CRL extension marked as critical is encountered.
      */
      X509_CRL(const std::string& filename,
               bool throw_on_unknown_critical = false);
   private:
      void force_decode();

      bool throw_on_unknown_critical;
      std::vector<CRL_Entry> revoked;
      Data_Store info;
   };

}


namespace Botan {

/**
* Luby-Rackoff block cipher construction
*/
class BOTAN_DLL LubyRackoff : public BlockCipher
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      size_t block_size() const { return 2 * hash->output_length(); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(2, 32, 2);
         }

      void clear();
      std::string name() const;
      BlockCipher* clone() const;

      /**
      * @param hash function to use to form the block cipher
      */
      LubyRackoff(HashFunction* hash);
      ~LubyRackoff() { delete hash; }
   private:
      void key_schedule(const byte[], size_t);

      HashFunction* hash;
      SecureVector<byte> K1, K2;
   };

}


namespace Botan {

/**
* EMSA2 from IEEE 1363
* Useful for Rabin-Williams
*/
class BOTAN_DLL EMSA2 : public EMSA
   {
   public:
      /**
      * @param hash the hash object to use
      */
      EMSA2(HashFunction* hash);
      ~EMSA2() { delete hash; }
   private:
      void update(const byte[], size_t);
      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);

      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);

      SecureVector<byte> empty_hash;
      HashFunction* hash;
      byte hash_id;
   };

}


namespace Botan {

/**
* Fused multiply-add
* @param a an integer
* @param b an integer
* @param c an integer
* @return (a*b)+c
*/
BigInt BOTAN_DLL mul_add(const BigInt& a,
                         const BigInt& b,
                         const BigInt& c);

/**
* Fused subtract-multiply
* @param a an integer
* @param b an integer
* @param c an integer
* @return (a-b)*c
*/
BigInt BOTAN_DLL sub_mul(const BigInt& a,
                         const BigInt& b,
                         const BigInt& c);

/**
* Return the absolute value
* @param n an integer
* @return absolute value of n
*/
inline BigInt abs(const BigInt& n) { return n.abs(); }

/**
* Compute the greatest common divisor
* @param x a positive integer
* @param y a positive integer
* @return gcd(x,y)
*/
BigInt BOTAN_DLL gcd(const BigInt& x, const BigInt& y);

/**
* Least common multiple
* @param x a positive integer
* @param y a positive integer
* @return z, smallest integer such that z % x == 0 and z % y == 0
*/
BigInt BOTAN_DLL lcm(const BigInt& x, const BigInt& y);

/**
* @param x an integer
* @return (x*x)
*/
BigInt BOTAN_DLL square(const BigInt& x);

/**
* Modular inversion
* @param x a positive integer
* @param modulus a positive integer
* @return y st (x*y) % modulus == 1
*/
BigInt BOTAN_DLL inverse_mod(const BigInt& x,
                             const BigInt& modulus);

/**
* Compute the Jacobi symbol. If n is prime, this is equivalent
* to the Legendre symbol.
* @see http://mathworld.wolfram.com/JacobiSymbol.html
*
* @param a is a non-negative integer
* @param n is an odd integer > 1
* @return (n / m)
*/
s32bit BOTAN_DLL jacobi(const BigInt& a,
                        const BigInt& n);

/**
* Modular exponentation
* @param b an integer base
* @param x a positive exponent
* @param m a positive modulus
* @return (b^x) % m
*/
BigInt BOTAN_DLL power_mod(const BigInt& b,
                           const BigInt& x,
                           const BigInt& m);

/**
* Compute the square root of x modulo a prime using the
* Shanks-Tonnelli algorithm
*
* @param x the input
* @param p the prime
* @return y such that (y*y)%p == x, or -1 if no such integer
*/
BigInt BOTAN_DLL ressol(const BigInt& x, const BigInt& p);

/**
* @param x an integer
* @return count of the zero bits in x, or, equivalently, the largest
* value of n such that 2^n divides x evently
*/
size_t BOTAN_DLL low_zero_bits(const BigInt& x);

/**
* Primality Testing
* @param n a positive integer to test for primality
* @param rng a random number generator
* @param level how hard to test
* @return true if all primality tests passed, otherwise false
*/
bool BOTAN_DLL primality_test(const BigInt& n,
                              RandomNumberGenerator& rng,
                              size_t level = 1);

/**
* Quickly check for primality
* @param n a positive integer to test for primality
* @param rng a random number generator
* @return true if all primality tests passed, otherwise false
*/
inline bool quick_check_prime(const BigInt& n, RandomNumberGenerator& rng)
   { return primality_test(n, rng, 0); }

/**
* Check for primality
* @param n a positive integer to test for primality
* @param rng a random number generator
* @return true if all primality tests passed, otherwise false
*/
inline bool check_prime(const BigInt& n, RandomNumberGenerator& rng)
   { return primality_test(n, rng, 1); }

/**
* Verify primality - this function is slow but useful if you want to
* ensure that a possibly malicious entity did not provide you with
* something that 'looks like' a prime
* @param n a positive integer to test for primality
* @param rng a random number generator
* @return true if all primality tests passed, otherwise false
*/
inline bool verify_prime(const BigInt& n, RandomNumberGenerator& rng)
   { return primality_test(n, rng, 2); }

/**
* Randomly generate a prime
* @param rng a random number generator
* @param bits how large the resulting prime should be in bits
* @param coprime a positive integer the result should be coprime to
* @param equiv a non-negative number that the result should be
               equivalent to modulo equiv_mod
* @param equiv_mod the modulus equiv should be checked against
* @return random prime with the specified criteria
*/
BigInt BOTAN_DLL random_prime(RandomNumberGenerator& rng,
                              size_t bits, const BigInt& coprime = 1,
                              size_t equiv = 1, size_t equiv_mod = 2);

/**
* Return a 'safe' prime, of the form p=2*q+1 with q prime
* @param rng a random number generator
* @param bits is how long the resulting prime should be
* @return prime randomly chosen from safe primes of length bits
*/
BigInt BOTAN_DLL random_safe_prime(RandomNumberGenerator& rng,
                                   size_t bits);

class Algorithm_Factory;

/**
* Generate DSA parameters using the FIPS 186 kosherizer
* @param rng a random number generator
* @param af an algorithm factory
* @param p_out where the prime p will be stored
* @param q_out where the prime q will be stored
* @param pbits how long p will be in bits
* @param qbits how long q will be in bits
* @return random seed used to generate this parameter set
*/
SecureVector<byte> BOTAN_DLL
generate_dsa_primes(RandomNumberGenerator& rng,
                    Algorithm_Factory& af,
                    BigInt& p_out, BigInt& q_out,
                    size_t pbits, size_t qbits);

/**
* Generate DSA parameters using the FIPS 186 kosherizer
* @param rng a random number generator
* @param af an algorithm factory
* @param p_out where the prime p will be stored
* @param q_out where the prime q will be stored
* @param pbits how long p will be in bits
* @param qbits how long q will be in bits
* @param seed the seed used to generate the parameters
* @return true if seed generated a valid DSA parameter set, otherwise
          false. p_out and q_out are only valid if true was returned.
*/
bool BOTAN_DLL
generate_dsa_primes(RandomNumberGenerator& rng,
                    Algorithm_Factory& af,
                    BigInt& p_out, BigInt& q_out,
                    size_t pbits, size_t qbits,
                    const MemoryRegion<byte>& seed);

/**
* The size of the PRIMES[] array
*/
const size_t PRIME_TABLE_SIZE = 6541;

/**
* A const array of all primes less than 65535
*/
extern const u16bit BOTAN_DLL PRIMES[];

}


namespace Botan {

/**
* This class represents an elliptic curve over GF(p)
*/
class BOTAN_DLL CurveGFp
   {
   public:

      /**
      * Create an uninitialized CurveGFp
      */
      CurveGFp() {}

      /**
      * Construct the elliptic curve E: y^2 = x^3 + ax + b over GF(p)
      * @param p prime number of the field
      * @param a first coefficient
      * @param b second coefficient
      */
      CurveGFp(const BigInt& p, const BigInt& a, const BigInt& b) :
         p(p), a(a), b(b), p_words(p.sig_words())
         {
         BigInt r(BigInt::Power2, p_words * BOTAN_MP_WORD_BITS);

         p_dash = (((r * inverse_mod(r, p)) - 1) / p).word_at(0);

         r2  = (r * r) % p;
         a_r = (a * r) % p;
         b_r = (b * r) % p;
         }

      // CurveGFp(const CurveGFp& other) = default;
      // CurveGFp& operator=(const CurveGFp& other) = default;

      /**
      * @return curve coefficient a
      */
      const BigInt& get_a() const { return a; }

      /**
      * @return curve coefficient b
      */
      const BigInt& get_b() const { return b; }

      /**
      * Get prime modulus of the field of the curve
      * @return prime modulus of the field of the curve
      */
      const BigInt& get_p() const { return p; }

      /**
      * @return Montgomery parameter r^2 % p
      */
      const BigInt& get_r2() const { return r2; }

      /**
      * @return a * r mod p
      */
      const BigInt& get_a_r() const { return a_r; }

      /**
      * @return b * r mod p
      */
      const BigInt& get_b_r() const { return b_r; }

      /**
      * @return Montgomery parameter p-dash
      */
      word get_p_dash() const { return p_dash; }

      /**
      * @return p.sig_words()
      */
      size_t get_p_words() const { return p_words; }

      /**
      * swaps the states of *this and other, does not throw
      * @param other curve to swap values with
      */
      void swap(CurveGFp& other)
         {
         std::swap(p, other.p);

         std::swap(a, other.a);
         std::swap(b, other.b);

         std::swap(a_r, other.a_r);
         std::swap(b_r, other.b_r);

         std::swap(p_words, other.p_words);

         std::swap(r2, other.r2);
         std::swap(p_dash, other.p_dash);
         }

      /**
      * Equality operator
      * @param other curve to compare with
      * @return true iff this is the same curve as other
      */
      bool operator==(const CurveGFp& other) const
         {
         /*
         Relies on choice of R, but that is fixed by constructor based
         on size of p
         */
         return (p == other.p && a_r == other.a_r && b_r == other.b_r);
         }

   private:
      // Curve parameters
      BigInt p, a, b;

      size_t p_words; // cache of p.sig_words()

      // Montgomery parameters
      BigInt r2, a_r, b_r;
      word p_dash;
   };

/**
* Equality operator
* @param lhs a curve
* @param rhs a curve
* @return true iff lhs is not the same as rhs
*/
inline bool operator!=(const CurveGFp& lhs, const CurveGFp& rhs)
   {
   return !(lhs == rhs);
   }

}

namespace std {

template<> inline
void swap<Botan::CurveGFp>(Botan::CurveGFp& curve1,
                           Botan::CurveGFp& curve2)
   {
   curve1.swap(curve2);
   }

} // namespace std


namespace Botan {

/**
* Exception thrown if you try to convert a zero point to an affine
* coordinate
*/
struct BOTAN_DLL Illegal_Transformation : public Exception
   {
   Illegal_Transformation(const std::string& err =
                          "Requested transformation is not possible") :
      Exception(err) {}
   };

/**
* Exception thrown if some form of illegal point is decoded
*/
struct BOTAN_DLL Illegal_Point : public Exception
   {
   Illegal_Point(const std::string& err = "Malformed ECP point detected") :
      Exception(err) {}
   };

/**
* This class represents one point on a curve of GF(p)
*/
class BOTAN_DLL PointGFp
   {
   public:
      enum Compression_Type {
         UNCOMPRESSED = 0,
         COMPRESSED   = 1,
         HYBRID       = 2
      };

      /**
      * Construct an uninitialized PointGFp
      */
      PointGFp() {}

      /**
      * Construct the zero point
      * @param curve The base curve
      */
      PointGFp(const CurveGFp& curve);

      /**
      * Construct a point from its affine coordinates
      * @param curve the base curve
      * @param x affine x coordinate
      * @param y affine y coordinate
      */
      PointGFp(const CurveGFp& curve, const BigInt& x, const BigInt& y);

      //PointGFp(const PointGFp& other) = default;
      //PointGFp& operator=(const PointGFp& other) = default;

      /**
      * += Operator
      * @param rhs the PointGFp to add to the local value
      * @result resulting PointGFp
      */
      PointGFp& operator+=(const PointGFp& rhs);

      /**
      * -= Operator
      * @param rhs the PointGFp to subtract from the local value
      * @result resulting PointGFp
      */
      PointGFp& operator-=(const PointGFp& rhs);

      /**
      * *= Operator
      * @param scalar the PointGFp to multiply with *this
      * @result resulting PointGFp
      */
      PointGFp& operator*=(const BigInt& scalar);

      /**
      * Multiplication Operator
      * @param scalar the scalar value
      * @param point the point value
      * @return scalar*point on the curve
      */
      friend BOTAN_DLL PointGFp operator*(const BigInt& scalar, const PointGFp& point);

      /**
      * Multiexponentiation
      * @param p1 a point
      * @param z1 a scalar
      * @param p2 a point
      * @param z2 a scalar
      * @result (p1 * z1 + p2 * z2)
      */
      friend BOTAN_DLL PointGFp multi_exponentiate(
        const PointGFp& p1, const BigInt& z1,
        const PointGFp& p2, const BigInt& z2);

      /**
      * Negate this point
      * @return *this
      */
      PointGFp& negate()
         {
         if(!is_zero())
            coord_y = curve.get_p() - coord_y;
         return *this;
         }

      /**
      * Return base curve of this point
      * @result the curve over GF(p) of this point
      */
      const CurveGFp& get_curve() const { return curve; }

      /**
      * get affine x coordinate
      * @result affine x coordinate
      */
      BigInt get_affine_x() const;

      /**
      * get affine y coordinate
      * @result affine y coordinate
      */
      BigInt get_affine_y() const;

      /**
      * Is this the point at infinity?
      * @result true, if this point is at infinity, false otherwise.
      */
      bool is_zero() const
         { return (coord_x.is_zero() && coord_z.is_zero()); }

      /**
      * Checks whether the point is to be found on the underlying
      * curve; used to prevent fault attacks.
      * @return if the point is on the curve
      */
      bool on_the_curve() const;

      /**
      * swaps the states of *this and other, does not throw!
      * @param other the object to swap values with
      */
      void swap(PointGFp& other);

      /**
      * Equality operator
      */
      bool operator==(const PointGFp& other) const;
   private:

      /**
      * Montgomery multiplication/reduction
      * @param x first multiplicand
      * @param y second multiplicand
      * @param workspace temp space
      */
      BigInt monty_mult(const BigInt& x, const BigInt& y) const
         {
         BigInt result;
         monty_mult(result, x, y);
         return result;
         }

      /**
      * Montgomery multiplication/reduction
      * @warning z cannot alias x or y
      * @param z output
      * @param x first multiplicand
      * @param y second multiplicand
      */
      void monty_mult(BigInt& z, const BigInt& x, const BigInt& y) const;

      /**
      * Montgomery squaring/reduction
      * @param x multiplicand
      */
      BigInt monty_sqr(const BigInt& x) const
         {
         BigInt result;
         monty_sqr(result, x);
         return result;
         }

      /**
      * Montgomery squaring/reduction
      * @warning z cannot alias x
      * @param z output
      * @param x multiplicand
      */
      void monty_sqr(BigInt& z, const BigInt& x) const;

      /**
      * Point addition
      * @param workspace temp space, at least 11 elements
      */
      void add(const PointGFp& other, std::vector<BigInt>& workspace);

      /**
      * Point doubling
      * @param workspace temp space, at least 9 elements
      */
      void mult2(std::vector<BigInt>& workspace);

      CurveGFp curve;
      BigInt coord_x, coord_y, coord_z;
      mutable SecureVector<word> ws; // workspace for Montgomery
   };

// relational operators
inline bool operator!=(const PointGFp& lhs, const PointGFp& rhs)
   {
   return !(rhs == lhs);
   }

// arithmetic operators
inline PointGFp operator-(const PointGFp& lhs)
   {
   return PointGFp(lhs).negate();
   }

inline PointGFp operator+(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp += rhs;
   }

inline PointGFp operator-(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp -= rhs;
   }

inline PointGFp operator*(const PointGFp& point, const BigInt& scalar)
   {
   return scalar * point;
   }

// encoding and decoding
SecureVector<byte> BOTAN_DLL EC2OSP(const PointGFp& point, byte format);

PointGFp BOTAN_DLL OS2ECP(const byte data[], size_t data_len,
                          const CurveGFp& curve);

inline PointGFp OS2ECP(const MemoryRegion<byte>& data, const CurveGFp& curve)
   { return OS2ECP(&data[0], data.size(), curve); }

}

namespace std {

template<>
inline void swap<Botan::PointGFp>(Botan::PointGFp& x, Botan::PointGFp& y)
   { x.swap(y); }

}


namespace Botan {

/**
* This class represents elliptic curce domain parameters
*/
enum EC_Group_Encoding {
   EC_DOMPAR_ENC_EXPLICIT = 0,
   EC_DOMPAR_ENC_IMPLICITCA = 1,
   EC_DOMPAR_ENC_OID = 2
};

/**
* Class representing an elliptic curve
*/
class BOTAN_DLL EC_Group
   {
   public:

      /**
      * Construct Domain paramers from specified parameters
      * @param curve elliptic curve
      * @param base_point a base point
      * @param order the order of the base point
      * @param cofactor the cofactor
      */
      EC_Group(const CurveGFp& curve,
               const PointGFp& base_point,
               const BigInt& order,
               const BigInt& cofactor) :
         curve(curve),
         base_point(base_point),
         order(order),
         cofactor(cofactor),
         oid("")
         {}

      /**
      * Decode a BER encoded ECC domain parameter set
      * @param ber_encoding the bytes of the BER encoding
      */
      EC_Group(const MemoryRegion<byte>& ber_encoding);

      /**
      * Create an EC domain by OID (or throw if unknown)
      * @param oid the OID of the EC domain to create
      */
      EC_Group(const OID& oid);

      /**
      * Create an EC domain from PEM encoding (as from PEM_encode),
      * or from an OID name (eg "secp160r1", or "1.3.132.0.8")
      * @param pem_or_oid PEM-encoded data, or an OID
      */
      EC_Group(const std::string& pem_or_oid = "");

      /**
      * Create the DER encoding of this domain
      * @param form of encoding to use
      * @returns bytes encododed as DER
      */
      SecureVector<byte> DER_encode(EC_Group_Encoding form) const;

      /**
      * Return the PEM encoding (always in explicit form)
      * @return string containing PEM data
      */
      std::string PEM_encode() const;

      /**
      * Return domain parameter curve
      * @result domain parameter curve
      */
      const CurveGFp& get_curve() const { return curve; }

      /**
      * Return domain parameter curve
      * @result domain parameter curve
      */
      const PointGFp& get_base_point() const { return base_point; }

      /**
      * Return the order of the base point
      * @result order of the base point
      */
      const BigInt& get_order() const { return order; }

      /**
      * Return the cofactor
      * @result the cofactor
      */
      const BigInt& get_cofactor() const { return cofactor; }

      bool initialized() const { return !base_point.is_zero(); }

      /**
      * Return the OID of these domain parameters
      * @result the OID
      */
      std::string get_oid() const { return oid; }

      bool operator==(const EC_Group& other) const
         {
         return ((get_curve() == other.get_curve()) &&
                 (get_base_point() == other.get_base_point()) &&
                 (get_order() == other.get_order()) &&
                 (get_cofactor() == other.get_cofactor()));
         }

   private:
      CurveGFp curve;
      PointGFp base_point;
      BigInt order, cofactor;
      std::string oid;
   };

inline bool operator!=(const EC_Group& lhs,
                       const EC_Group& rhs)
   {
   return !(lhs == rhs);
   }

// For compatability with 1.8
typedef EC_Group EC_Domain_Params;

}


namespace Botan {

/**
* User Interface
* Only really used for callbacks for PKCS #8 decryption
*/
class BOTAN_DLL User_Interface
   {
   public:
      enum UI_Result { OK, CANCEL_ACTION };

      virtual std::string get_passphrase(const std::string&,
                                         const std::string&,
                                         UI_Result&) const;
      User_Interface(const std::string& = "");
      virtual ~User_Interface() {}
   protected:
      std::string preset_passphrase;
      mutable bool first_try;
   };

}


namespace Botan {

/**
* PKCS #8 General Exception
*/
struct BOTAN_DLL PKCS8_Exception : public Decoding_Error
   {
   PKCS8_Exception(const std::string& error) :
      Decoding_Error("PKCS #8: " + error) {}
   };

/**
* This namespace contains functions for handling PKCS #8 private keys
*/
namespace PKCS8 {

/**
* BER encode a private key
* @param key the private key to encode
* @return BER encoded key
*/
BOTAN_DLL SecureVector<byte> BER_encode(const Private_Key& key);

/**
* Get a string containing a PEM encoded private key.
* @param key the key to encode
* @return encoded key
*/
BOTAN_DLL std::string PEM_encode(const Private_Key& key);

/**
* Encrypt a key using PKCS #8 encryption
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbe_algo the name of the desired password-based encryption
         algorithm; if empty ("") a reasonable (portable/secure)
         default will be chosen.
* @return encrypted key in binary BER form
*/
BOTAN_DLL SecureVector<byte> BER_encode(const Private_Key& key,
                                        RandomNumberGenerator& rng,
                                        const std::string& pass,
                                        const std::string& pbe_algo = "");

/**
* Get a string containing a PEM encoded private key, encrypting it with a
* password.
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbe_algo the name of the desired password-based encryption
         algorithm; if empty ("") a reasonable (portable/secure)
         default will be chosen.
* @return encrypted key in PEM form
*/
BOTAN_DLL std::string PEM_encode(const Private_Key& key,
                                 RandomNumberGenerator& rng,
                                 const std::string& pass,
                                 const std::string& pbe_algo = "");


/**
* Encode a private key into a pipe.
* @deprecated Use PEM_encode or BER_encode instead
*
* @param key the private key to encode
* @param pipe the pipe to feed the encoded key into
* @param encoding the encoding type to use
*/
BOTAN_DEPRECATED("Use PEM_encode or BER_encode")
inline void encode(const Private_Key& key,
                   Pipe& pipe,
                   X509_Encoding encoding = PEM)
   {
   if(encoding == PEM)
      pipe.write(PKCS8::PEM_encode(key));
   else
      pipe.write(PKCS8::BER_encode(key));
   }

/**
* Encode and encrypt a private key into a pipe.
* @deprecated Use PEM_encode or BER_encode instead
*
* @param key the private key to encode
* @param pipe the pipe to feed the encoded key into
* @param pass the password to use for encryption
* @param rng the rng to use
* @param pbe_algo the name of the desired password-based encryption
         algorithm; if empty ("") a reasonable (portable/secure)
         default will be chosen.
* @param encoding the encoding type to use
*/
BOTAN_DEPRECATED("Use PEM_encode or BER_encode")
inline void encrypt_key(const Private_Key& key,
                        Pipe& pipe,
                        RandomNumberGenerator& rng,
                        const std::string& pass,
                        const std::string& pbe_algo = "",
                        X509_Encoding encoding = PEM)
   {
   if(encoding == PEM)
      pipe.write(PKCS8::PEM_encode(key, rng, pass, pbe_algo));
   else
      pipe.write(PKCS8::BER_encode(key, rng, pass, pbe_algo));
   }

/**
* Load a key from a data source.
* @param source the data source providing the encoded key
* @param rng the rng to use
* @param ui the user interface to be used for passphrase dialog
* @return loaded private key object
*/
BOTAN_DLL Private_Key* load_key(DataSource& source,
                                RandomNumberGenerator& rng,
                                const User_Interface& ui);

/** Load a key from a data source.
* @param source the data source providing the encoded key
* @param rng the rng to use
* @param pass the passphrase to decrypt the key. Provide an empty
* string if the key is not encoded.
* @return loaded private key object
*/
BOTAN_DLL Private_Key* load_key(DataSource& source,
                                RandomNumberGenerator& rng,
                                const std::string& pass = "");

/**
* Load a key from a file.
* @param filename the path to the file containing the encoded key
* @param rng the rng to use
* @param ui the user interface to be used for passphrase dialog
* @return loaded private key object
*/
BOTAN_DLL Private_Key* load_key(const std::string& filename,
                                RandomNumberGenerator& rng,
                                const User_Interface& ui);

/** Load a key from a file.
* @param filename the path to the file containing the encoded key
* @param rng the rng to use
* @param pass the passphrase to decrypt the key. Provide an empty
* string if the key is not encoded.
* @return loaded private key object
*/
BOTAN_DLL Private_Key* load_key(const std::string& filename,
                                RandomNumberGenerator& rng,
                                const std::string& pass = "");

/**
* Copy an existing encoded key object.
* @param key the key to copy
* @param rng the rng to use
* @return new copy of the key
*/
BOTAN_DLL Private_Key* copy_key(const Private_Key& key,
                                RandomNumberGenerator& rng);

}

}


namespace Botan {

/**
* This class represents abstract ECC public keys. When encoding a key
* via an encoder that can be accessed via the corresponding member
* functions, the key will decide upon its internally stored encoding
* information whether to encode itself with or without domain
* parameters, or using the domain parameter oid. Furthermore, a public
* key without domain parameters can be decoded. In that case, it
* cannot be used for verification until its domain parameters are set
* by calling the corresponding member function.
*/
class BOTAN_DLL EC_PublicKey : public virtual Public_Key
   {
   public:
      EC_PublicKey(const EC_Group& dom_par,
                   const PointGFp& pub_point);

      EC_PublicKey(const AlgorithmIdentifier& alg_id,
                   const MemoryRegion<byte>& key_bits);

      /**
      * Get the public point of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the public point of this key
      */
      const PointGFp& public_point() const { return public_key; }

      AlgorithmIdentifier algorithm_identifier() const;

      MemoryVector<byte> x509_subject_public_key() const;

      bool check_key(RandomNumberGenerator& rng,
                     bool strong) const;

      /**
      * Get the domain parameters of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the domain parameters of this key
      */
      const EC_Group& domain() const { return domain_params; }

      /**
      * Set the domain parameter encoding to be used when encoding this key.
      * @param enc the encoding to use
      */
      void set_parameter_encoding(EC_Group_Encoding enc);

      /**
      * Return the DER encoding of this keys domain in whatever format
      * is preset for this particular key
      */
      MemoryVector<byte> DER_domain() const
         { return domain().DER_encode(domain_format()); }

      /**
      * Get the domain parameter encoding to be used when encoding this key.
      * @result the encoding to use
      */
      EC_Group_Encoding domain_format() const
         { return domain_encoding; }
   protected:
      EC_PublicKey() : domain_encoding(EC_DOMPAR_ENC_EXPLICIT) {}

      EC_Group domain_params;
      PointGFp public_key;
      EC_Group_Encoding domain_encoding;
   };

/**
* This abstract class represents ECC private keys
*/
class BOTAN_DLL EC_PrivateKey : public virtual EC_PublicKey,
                                public virtual Private_Key
   {
   public:
     EC_PrivateKey(RandomNumberGenerator& rng,
                   const EC_Group& domain,
                   const BigInt& private_key);

      EC_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits);

      MemoryVector<byte> pkcs8_private_key() const;

      /**
      * Get the private key value of this key object.
      * @result the private key value of this key object
      */
      const BigInt& private_value() const;
   protected:
      EC_PrivateKey() {}

      BigInt private_key;
   };

}


namespace Botan {

/**
* GOST-34.10 Public Key
*/
class BOTAN_DLL GOST_3410_PublicKey : public virtual EC_PublicKey
   {
   public:

      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      GOST_3410_PublicKey(const EC_Group& dom_par,
                          const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      /**
      * Construct from X.509 algorithm id and subject public key bits
      */
      GOST_3410_PublicKey(const AlgorithmIdentifier& alg_id,
                          const MemoryRegion<byte>& key_bits);

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name
      */
      std::string algo_name() const { return "GOST-34.10"; }

      AlgorithmIdentifier algorithm_identifier() const;

      MemoryVector<byte> x509_subject_public_key() const;

      /**
      * Get the maximum number of bits allowed to be fed to this key.
      * This is the bitlength of the order of the base point.

      * @result the maximum number of input bits
      */
      size_t max_input_bits() const { return domain().get_order().bits(); }

      size_t message_parts() const { return 2; }

      size_t message_part_size() const
         { return domain().get_order().bytes(); }

   protected:
      GOST_3410_PublicKey() {}
   };

/**
* GOST-34.10 Private Key
*/
class BOTAN_DLL GOST_3410_PrivateKey : public GOST_3410_PublicKey,
                                       public EC_PrivateKey
   {
   public:

      GOST_3410_PrivateKey(const AlgorithmIdentifier& alg_id,
                           const MemoryRegion<byte>& key_bits) :
         EC_PrivateKey(alg_id, key_bits) {}

      /**
      * Generate a new private key
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key; if zero, a new random key is generated
      */
      GOST_3410_PrivateKey(RandomNumberGenerator& rng,
                           const EC_Group& domain,
                           const BigInt& x = 0) :
         EC_PrivateKey(rng, domain, x) {}

      AlgorithmIdentifier pkcs8_algorithm_identifier() const
         { return EC_PublicKey::algorithm_identifier(); }
   };

/**
* GOST-34.10 signature operation
*/
class BOTAN_DLL GOST_3410_Signature_Operation : public PK_Ops::Signature
   {
   public:
      GOST_3410_Signature_Operation(const GOST_3410_PrivateKey& gost_3410);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return order.bytes(); }
      size_t max_input_bits() const { return order.bits(); }

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);

   private:
      const PointGFp& base_point;
      const BigInt& order;
      const BigInt& x;
   };

/**
* GOST-34.10 verification operation
*/
class BOTAN_DLL GOST_3410_Verification_Operation : public PK_Ops::Verification
   {
   public:
      GOST_3410_Verification_Operation(const GOST_3410_PublicKey& gost);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return order.bytes(); }
      size_t max_input_bits() const { return order.bits(); }

      bool with_recovery() const { return false; }

      bool verify(const byte msg[], size_t msg_len,
                  const byte sig[], size_t sig_len);
   private:
      const PointGFp& base_point;
      const PointGFp& public_point;
      const BigInt& order;
   };

}


namespace Botan {

/**
* MDx Hash Function Base Class
*/
class BOTAN_DLL MDx_HashFunction : public HashFunction
   {
   public:
      /**
      * @param block_length is the number of bytes per block
      * @param big_byte_endian specifies if the hash uses big-endian bytes
      * @param big_bit_endian specifies if the hash uses big-endian bits
      * @param counter_size specifies the size of the counter var in bytes
      */
      MDx_HashFunction(size_t block_length,
                       bool big_byte_endian,
                       bool big_bit_endian,
                       size_t counter_size = 8);

      size_t hash_block_size() const { return buffer.size(); }
   protected:
      void add_data(const byte input[], size_t length);
      void final_result(byte output[]);

      /**
      * Run the hash's compression function over a set of blocks
      * @param blocks the input
      * @param block_n the number of blocks
      */
      virtual void compress_n(const byte blocks[], size_t block_n) = 0;

      void clear();

      /**
      * Copy the output to the buffer
      * @param buffer to put the output into
      */
      virtual void copy_out(byte buffer[]) = 0;

      /**
      * Write the count, if used, to this spot
      * @param out where to write the counter to
      */
      virtual void write_count(byte out[]);
   private:
      SecureVector<byte> buffer;
      u64bit count;
      size_t position;

      const bool BIG_BYTE_ENDIAN, BIG_BIT_ENDIAN;
      const size_t COUNT_SIZE;
   };

}


namespace Botan {

/**
* HAS-160, a Korean hash function standardized in
* TTAS.KO-12.0011/R1. Used in conjuction with KCDSA
*/
class BOTAN_DLL HAS_160 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "HAS-160"; }
      size_t output_length() const { return 20; }
      HashFunction* clone() const { return new HAS_160; }

      void clear();

      HAS_160() : MDx_HashFunction(64, false, true), X(20), digest(5)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u32bit> X, digest;
   };

}


namespace Botan {

/**
* This class represents the Library Initialization/Shutdown Object. It
* has to exceed the lifetime of any Botan object used in an
* application.  You can call initialize/deinitialize or use
* LibraryInitializer in the RAII style.
*/
class BOTAN_DLL LibraryInitializer
   {
   public:
      /**
      * Initialize the library
      * @param options a string listing initialization options
      */
      static void initialize(const std::string& options = "");

      /**
      * Shutdown the library
      */
      static void deinitialize();

      /**
      * Initialize the library
      * @param options a string listing initialization options
      */
      LibraryInitializer(const std::string& options = "")
         { LibraryInitializer::initialize(options); }

      ~LibraryInitializer() { LibraryInitializer::deinitialize(); }
   };

}


namespace Botan {

/*
* Forward declare to avoid recursive dependency between this header
* and libstate.h
*/
class Library_State;

/**
* Namespace for management of the global state
*/
namespace Global_State_Management {

/**
* Access the global library state
* @return reference to the global library state
*/
BOTAN_DLL Library_State& global_state();

/**
* Set the global state object
* @param state the new global state to use
*/
BOTAN_DLL void set_global_state(Library_State* state);

/**
* Set the global state object unless it is already set
* @param state the new global state to use
* @return true if the state parameter is now being used as the global
*         state, or false if one was already set, in which case the
*         parameter was deleted immediately
*/
BOTAN_DLL bool set_global_state_unless_set(Library_State* state);

/**
* Swap the current state for another
* @param new_state the new state object to use
* @return previous state (or NULL if none)
*/
BOTAN_DLL Library_State* swap_global_state(Library_State* new_state);

/**
* Query if the library is currently initialized
* @return true iff the library is initialized
*/
BOTAN_DLL bool global_state_exists();

}

/*
* Insert into Botan ns for convenience/backwards compatability
*/
using Global_State_Management::global_state;

}


namespace Botan {

/**
* Forward declarations (don't need full definitions here)
*/
class BlockCipher;
class StreamCipher;
class HashFunction;
class MessageAuthenticationCode;
class PBKDF;

template<typename T> class Algorithm_Cache;

class Engine;
class Mutex_Factory;

/**
* Algorithm Factory
*/
class BOTAN_DLL Algorithm_Factory
   {
   public:
      /**
      * Constructor
      * @param mf a mutex factory
      */
      Algorithm_Factory(Mutex_Factory& mf);

      /**
      * Destructor
      */
      ~Algorithm_Factory();

      /**
      * @param engine to add (Algorithm_Factory takes ownership)
      */
      void add_engine(Engine* engine);

      /**
      * Clear out any cached objects
      */
      void clear_caches();

      /**
      * @param algo_spec the algorithm we are querying
      * @returns list of providers of this algorithm
      */
      std::vector<std::string> providers_of(const std::string& algo_spec);

      /**
      * @param algo_spec the algorithm we are setting a provider for
      * @param provider the provider we would like to use
      */
      void set_preferred_provider(const std::string& algo_spec,
                                  const std::string& provider);

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to const prototype object, ready to clone(), or NULL
      */
      const BlockCipher*
         prototype_block_cipher(const std::string& algo_spec,
                                const std::string& provider = "");

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to freshly created instance of the request algorithm
      */
      BlockCipher* make_block_cipher(const std::string& algo_spec,
                                     const std::string& provider = "");

      /**
      * @param algo the algorithm to add
      * @param provider the provider of this algorithm
      */
      void add_block_cipher(BlockCipher* algo, const std::string& provider);

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to const prototype object, ready to clone(), or NULL
      */
      const StreamCipher*
         prototype_stream_cipher(const std::string& algo_spec,
                                 const std::string& provider = "");

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to freshly created instance of the request algorithm
      */
      StreamCipher* make_stream_cipher(const std::string& algo_spec,
                                       const std::string& provider = "");

      /**
      * @param algo the algorithm to add
      * @param provider the provider of this algorithm
      */
      void add_stream_cipher(StreamCipher* algo, const std::string& provider);

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to const prototype object, ready to clone(), or NULL
      */
      const HashFunction*
         prototype_hash_function(const std::string& algo_spec,
                                 const std::string& provider = "");

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to freshly created instance of the request algorithm
      */
      HashFunction* make_hash_function(const std::string& algo_spec,
                                       const std::string& provider = "");

      /**
      * @param algo the algorithm to add
      * @param provider the provider of this algorithm
      */
      void add_hash_function(HashFunction* algo, const std::string& provider);

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to const prototype object, ready to clone(), or NULL
      */
      const MessageAuthenticationCode*
         prototype_mac(const std::string& algo_spec,
                       const std::string& provider = "");

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to freshly created instance of the request algorithm
      */
      MessageAuthenticationCode* make_mac(const std::string& algo_spec,
                                          const std::string& provider = "");

      /**
      * @param algo the algorithm to add
      * @param provider the provider of this algorithm
      */
      void add_mac(MessageAuthenticationCode* algo,
                   const std::string& provider);

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to const prototype object, ready to clone(), or NULL
      */
      const PBKDF* prototype_pbkdf(const std::string& algo_spec,
                                   const std::string& provider = "");

      /**
      * @param algo_spec the algorithm we want
      * @param provider the provider we would like to use
      * @returns pointer to freshly created instance of the request algorithm
      */
      PBKDF* make_pbkdf(const std::string& algo_spec,
                        const std::string& provider = "");

      /**
      * @param algo the algorithm to add
      * @param provider the provider of this algorithm
      */
      void add_pbkdf(PBKDF* algo, const std::string& provider);

      /**
      * An iterator for the engines in this factory
      * @deprecated Avoid in new code
      */
      class BOTAN_DLL Engine_Iterator
         {
         public:
            /**
            * @return next engine in the sequence
            */
            Engine* next() { return af.get_engine_n(n++); }

            /**
            * @param a an algorithm factory
            */
            Engine_Iterator(const Algorithm_Factory& a) :
               af(a) { n = 0; }
         private:
            const Algorithm_Factory& af;
            size_t n;
         };
      friend class Engine_Iterator;

   private:
      Algorithm_Factory(const Algorithm_Factory&) {}
      Algorithm_Factory& operator=(const Algorithm_Factory&)
         { return (*this); }

      Engine* get_engine_n(size_t n) const;

      std::vector<Engine*> engines;

      Algorithm_Cache<BlockCipher>* block_cipher_cache;
      Algorithm_Cache<StreamCipher>* stream_cipher_cache;
      Algorithm_Cache<HashFunction>* hash_cache;
      Algorithm_Cache<MessageAuthenticationCode>* mac_cache;
      Algorithm_Cache<PBKDF>* pbkdf_cache;
   };

}



namespace Botan {

class Mutex;

/**
* Global state container aka the buritto at the center of it all
*/
class BOTAN_DLL Library_State
   {
   public:
      Library_State();
      ~Library_State();

      /**
      * @param thread_safe should a mutex be used for serialization
      */
      void initialize(bool thread_safe);

      /**
      * @return global Algorithm_Factory
      */
      Algorithm_Factory& algorithm_factory() const;

      /**
      * @return global RandomNumberGenerator
      */
      RandomNumberGenerator& global_rng();

      /**
      * @param name the name of the allocator
      * @return allocator matching this name, or NULL
      */
      Allocator* get_allocator(const std::string& name = "") const;

      /**
      * Add a new allocator to the list of available ones
      * @param alloc the allocator to add
      */
      void add_allocator(Allocator* alloc);

      /**
      * Set the default allocator
      * @param name the name of the allocator to use as the default
      */
      void set_default_allocator(const std::string& name);

      /**
      * Get a parameter value as std::string.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @result the value of the parameter
      */
      std::string get(const std::string& section,
                      const std::string& key) const;

      /**
      * Check whether a certain parameter is set or not.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @result true if the parameters value is set,
      * false otherwise
      */
      bool is_set(const std::string& section,
                  const std::string& key) const;

      /**
      * Set a configuration parameter.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @param value the new value
      * @param overwrite if set to true, the parameters value
      * will be overwritten even if it is already set, otherwise
      * no existing values will be overwritten.
      */
      void set(const std::string& section,
               const std::string& key,
               const std::string& value,
               bool overwrite = true);

      /**
      * Add a parameter value to the "alias" section.
      * @param key the name of the parameter which shall have a new alias
      * @param value the new alias
      */
      void add_alias(const std::string& key,
                     const std::string& value);

      /**
      * Resolve an alias.
      * @param alias the alias to resolve.
      * @return what the alias stands for
      */
      std::string deref_alias(const std::string& alias) const;

      /**
      * @return newly created Mutex (free with delete)
      */
      Mutex* get_mutex() const;
   private:
      static RandomNumberGenerator* make_global_rng(Algorithm_Factory& af,
                                                    Mutex* mutex);

      void load_default_config();

      Library_State(const Library_State&) {}
      Library_State& operator=(const Library_State&) { return (*this); }

      class Mutex_Factory* mutex_factory;

      Mutex* global_rng_lock;
      RandomNumberGenerator* global_rng_ptr;

      Mutex* config_lock;
      std::map<std::string, std::string> config;

      Mutex* allocator_lock;
      std::string default_allocator_name;
      std::map<std::string, Allocator*> alloc_factory;
      mutable Allocator* cached_default_allocator;
      std::vector<Allocator*> allocators;

      Algorithm_Factory* m_algorithm_factory;
   };

}



namespace Botan {

/**
* BitBucket is a filter which simply discards all inputs
*/
struct BOTAN_DLL BitBucket : public Filter
   {
   void write(const byte[], size_t) {}

   std::string name() const { return "BitBucket"; }
   };

/**
* This class represents Filter chains. A Filter chain is an ordered
* concatenation of Filters, the input to a Chain sequentially passes
* through all the Filters contained in the Chain.
*/

class BOTAN_DLL Chain : public Fanout_Filter
   {
   public:
      void write(const byte input[], size_t length) { send(input, length); }

      std::string name() const;

      /**
      * Construct a chain of up to four filters. The filters are set
      * up in the same order as the arguments.
      */
      Chain(Filter* = 0, Filter* = 0, Filter* = 0, Filter* = 0);

      /**
      * Construct a chain from range of filters
      * @param filter_arr the list of filters
      * @param length how many filters
      */
      Chain(Filter* filter_arr[], size_t length);
   };

/**
* This class represents a fork filter, whose purpose is to fork the
* flow of data. It causes an input message to result in n messages at
* the end of the filter, where n is the number of forks.
*/
class BOTAN_DLL Fork : public Fanout_Filter
   {
   public:
      void write(const byte input[], size_t length) { send(input, length); }
      void set_port(size_t n) { Fanout_Filter::set_port(n); }

      std::string name() const;

      /**
      * Construct a Fork filter with up to four forks.
      */
      Fork(Filter*, Filter*, Filter* = 0, Filter* = 0);

      /**
      * Construct a Fork from range of filters
      * @param filter_arr the list of filters
      * @param length how many filters
      */
      Fork(Filter* filter_arr[], size_t length);
   };

}


namespace Botan {

/**
* This class represents keyed filters, i.e. filters that have to be
* fed with a key in order to function.
*/
class BOTAN_DLL Keyed_Filter : public Filter
   {
   public:
      /**
      * Set the key of this filter
      * @param key the key to use
      */
      virtual void set_key(const SymmetricKey& key) = 0;

      /**
      * Set the initialization vector of this filter. Note: you should
      * call set_iv() only after you have called set_key()
      * @param iv the initialization vector to use
      */
      virtual void set_iv(const InitializationVector& iv);

      /**
      * Check whether a key length is valid for this filter
      * @param length the key length to be checked for validity
      * @return true if the key length is valid, false otherwise
      */
      virtual bool valid_keylength(size_t length) const = 0;

      /**
      * Check whether an IV length is valid for this filter
      * @param length the IV length to be checked for validity
      * @return true if the IV length is valid, false otherwise
      */
      virtual bool valid_iv_length(size_t length) const
         { return (length == 0); }
   };

}


namespace Botan {

/**
* This class represents abstract data sink objects.
*/
class BOTAN_DLL DataSink : public Filter
   {
   public:
      bool attachable() { return false; }
      DataSink() {}
      virtual ~DataSink() {}
   private:
      DataSink& operator=(const DataSink&) { return (*this); }
      DataSink(const DataSink&);
   };

/**
* This class represents a data sink which writes its output to a stream.
*/
class BOTAN_DLL DataSink_Stream : public DataSink
   {
   public:
      std::string name() const { return identifier; }

      void write(const byte[], size_t);

      /**
      * Construct a DataSink_Stream from a stream.
      * @param stream the stream to write to
      * @param name identifier
      */
      DataSink_Stream(std::ostream& stream,
                      const std::string& name = "<std::ostream>");

      /**
      * Construct a DataSink_Stream from a stream.
      * @param pathname the name of the file to open a stream to
      * @param use_binary indicates whether to treat the file
      * as a binary file or not
      */
      DataSink_Stream(const std::string& pathname,
                      bool use_binary = false);

      ~DataSink_Stream();
   private:
      const std::string identifier;

      std::ostream* sink_p;
      std::ostream& sink;
   };

}



#if defined(BOTAN_HAS_CODEC_FILTERS)

namespace Botan {

/**
* This class represents a Base64 encoder.
*/
class BOTAN_DLL Base64_Encoder : public Filter
   {
   public:
      std::string name() const { return "Base64_Encoder"; }

      /**
      * Input a part of a message to the encoder.
      * @param input the message to input as a byte array
      * @param length the length of the byte array input
      */
      void write(const byte input[], size_t length);

      /**
      * Inform the Encoder that the current message shall be closed.
      */
      void end_msg();

      /**
      * Create a base64 encoder.
      * @param breaks whether to use line breaks in the output
      * @param length the length of the lines of the output
      * @param t_n whether to use a trailing newline
      */
      Base64_Encoder(bool breaks = false, size_t length = 72,
                     bool t_n = false);
   private:
      void encode_and_send(const byte input[], size_t length,
                           bool final_inputs = false);
      void do_output(const byte output[], size_t length);

      const size_t line_length;
      const bool trailing_newline;
      MemoryVector<byte> in, out;
      size_t position, out_position;
   };

/**
* This object represents a Base64 decoder.
*/
class BOTAN_DLL Base64_Decoder : public Filter
   {
   public:
      std::string name() const { return "Base64_Decoder"; }

      /**
      * Input a part of a message to the decoder.
      * @param input the message to input as a byte array
      * @param length the length of the byte array input
      */
      void write(const byte input[], size_t length);

      /**
      * Finish up the current message
      */
      void end_msg();

      /**
      * Create a base64 decoder.
      * @param checking the type of checking that shall be performed by
      * the decoder
      */
      Base64_Decoder(Decoder_Checking checking = NONE);
   private:
      const Decoder_Checking checking;
      MemoryVector<byte> in, out;
      size_t position;
   };

}


namespace Botan {

/**
* Converts arbitrary binary data to hex strings, optionally with
* newlines inserted
*/
class BOTAN_DLL Hex_Encoder : public Filter
   {
   public:
      /**
      * Whether to use uppercase or lowercase letters for the encoded string.
      */
      enum Case { Uppercase, Lowercase };

      std::string name() const { return "Hex_Encoder"; }

      void write(const byte in[], size_t length);
      void end_msg();

      /**
      * Create a hex encoder.
      * @param the_case the case to use in the encoded strings.
      */
      Hex_Encoder(Case the_case);

      /**
      * Create a hex encoder.
      * @param newlines should newlines be used
      * @param line_length if newlines are used, how long are lines
      * @param the_case the case to use in the encoded strings
      */
      Hex_Encoder(bool newlines = false,
                  size_t line_length = 72,
                  Case the_case = Uppercase);
   private:
      void encode_and_send(const byte[], size_t);

      const Case casing;
      const size_t line_length;
      MemoryVector<byte> in, out;
      size_t position, counter;
   };

/**
* Converts hex strings to bytes
*/
class BOTAN_DLL Hex_Decoder : public Filter
   {
   public:
      std::string name() const { return "Hex_Decoder"; }

      void write(const byte[], size_t);
      void end_msg();

      /**
      * Construct a Hex Decoder using the specified
      * character checking.
      * @param checking the checking to use during decoding.
      */
      Hex_Decoder(Decoder_Checking checking = NONE);
   private:
      const Decoder_Checking checking;
      MemoryVector<byte> in, out;
      size_t position;
   };

}

#endif

namespace Botan {

/**
* Stream Cipher Filter.
*/
class BOTAN_DLL StreamCipher_Filter : public Keyed_Filter
   {
   public:

      std::string name() const { return cipher->name(); }

      /**
      * Write input data
      * @param input data
      * @param input_len length of input in bytes
      */
      void write(const byte input[], size_t input_len);

      bool valid_iv_length(size_t iv_len) const
         { return cipher->valid_iv_length(iv_len); }

      /**
      * Set the initialization vector for this filter.
      * @param iv the initialization vector to set
      */
      void set_iv(const InitializationVector& iv);

      /**
      * Set the key of this filter.
      * @param key the key to set
      */
      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      /**
      * Check whether a key length is valid for this filter.
      * @param length the key length to be checked for validity
      * @return true if the key length is valid, false otherwise
      */
      bool valid_keylength(size_t length) const
         { return cipher->valid_keylength(length); }

      /**
      * Construct a stream cipher filter.
      * @param cipher_obj a cipher object to use
      */
      StreamCipher_Filter(StreamCipher* cipher_obj);

      /**
      * Construct a stream cipher filter.
      * @param cipher_obj a cipher object to use
      * @param key the key to use inside this filter
      */
      StreamCipher_Filter(StreamCipher* cipher_obj, const SymmetricKey& key);

      /**
      * Construct a stream cipher filter.
      * @param cipher the name of the desired cipher
      */
      StreamCipher_Filter(const std::string& cipher);

      /**
      * Construct a stream cipher filter.
      * @param cipher the name of the desired cipher
      * @param key the key to use inside this filter
      */
      StreamCipher_Filter(const std::string& cipher, const SymmetricKey& key);

      ~StreamCipher_Filter() { delete cipher; }
   private:
      SecureVector<byte> buffer;
      StreamCipher* cipher;
   };

/**
* Hash Filter.
*/
class BOTAN_DLL Hash_Filter : public Filter
   {
   public:
      void write(const byte input[], size_t len) { hash->update(input, len); }
      void end_msg();

      std::string name() const { return hash->name(); }

      /**
      * Construct a hash filter.
      * @param hash_fun the hash function to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the hashfunction
      * hash. Otherwise, specify a smaller value here so that the
      * output of the hash algorithm will be cut off.
      */
      Hash_Filter(HashFunction* hash_fun, size_t len = 0) :
         OUTPUT_LENGTH(len), hash(hash_fun) {}

      /**
      * Construct a hash filter.
      * @param request the name of the hash algorithm to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the hashfunction
      * hash. Otherwise, specify a smaller value here so that the
      * output of the hash algorithm will be cut off.
      */
      Hash_Filter(const std::string& request, size_t len = 0);

      ~Hash_Filter() { delete hash; }
   private:
      const size_t OUTPUT_LENGTH;
      HashFunction* hash;
   };

/**
* MessageAuthenticationCode Filter.
*/
class BOTAN_DLL MAC_Filter : public Keyed_Filter
   {
   public:
      void write(const byte input[], size_t len) { mac->update(input, len); }
      void end_msg();

      std::string name() const { return mac->name(); }

      /**
      * Set the key of this filter.
      * @param key the key to set
      */
      void set_key(const SymmetricKey& key) { mac->set_key(key); }

      /**
      * Check whether a key length is valid for this filter.
      * @param length the key length to be checked for validity
      * @return true if the key length is valid, false otherwise
      */
      bool valid_keylength(size_t length) const
         { return mac->valid_keylength(length); }

      /**
      * Construct a MAC filter. The MAC key will be left empty.
      * @param mac_obj the MAC to use
      * @param out_len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(MessageAuthenticationCode* mac_obj,
                 size_t out_len = 0) : OUTPUT_LENGTH(out_len)
         {
         mac = mac_obj;
         }

      /**
      * Construct a MAC filter.
      * @param mac_obj the MAC to use
      * @param key the MAC key to use
      * @param out_len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(MessageAuthenticationCode* mac_obj,
                 const SymmetricKey& key,
                 size_t out_len = 0) : OUTPUT_LENGTH(out_len)
         {
         mac = mac_obj;
         mac->set_key(key);
         }

      /**
      * Construct a MAC filter. The MAC key will be left empty.
      * @param mac the name of the MAC to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(const std::string& mac, size_t len = 0);

      /**
      * Construct a MAC filter.
      * @param mac the name of the MAC to use
      * @param key the MAC key to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(const std::string& mac, const SymmetricKey& key,
                 size_t len = 0);

      ~MAC_Filter() { delete mac; }
   private:
      const size_t OUTPUT_LENGTH;
      MessageAuthenticationCode* mac;
   };

}


namespace Botan {

/**
* Block Cipher Mode Padding Method
* This class is pretty limited, it cannot deal well with
* randomized padding methods, or any padding method that
* wants to add more than one block. For instance, it should
* be possible to define cipher text stealing mode as simply
* a padding mode for CBC, which happens to consume the last
* two block (and requires use of the block cipher).
*/
class BOTAN_DLL BlockCipherModePaddingMethod
   {
   public:
      /**
      * @param block output buffer
      * @param size of the block
      * @param current_position in the last block
      */
      virtual void pad(byte block[],
                       size_t size,
                       size_t current_position) const = 0;

      /**
      * @param block the last block
      * @param size the of the block
      */
      virtual size_t unpad(const byte block[],
                           size_t size) const = 0;

      /**
      * @param block_size of the cipher
      * @param position in the current block
      * @return number of padding bytes that will be appended
      */
      virtual size_t pad_bytes(size_t block_size,
                               size_t position) const;

      /**
      * @param block_size of the cipher
      * @return valid block size for this padding mode
      */
      virtual bool valid_blocksize(size_t block_size) const = 0;

      /**
      * @return name of the mode
      */
      virtual std::string name() const = 0;

      /**
      * virtual destructor
      */
      virtual ~BlockCipherModePaddingMethod() {}
   };

/**
* PKCS#7 Padding
*/
class BOTAN_DLL PKCS7_Padding : public BlockCipherModePaddingMethod
   {
   public:
      void pad(byte[], size_t, size_t) const;
      size_t unpad(const byte[], size_t) const;
      bool valid_blocksize(size_t) const;
      std::string name() const { return "PKCS7"; }
   };

/**
* ANSI X9.23 Padding
*/
class BOTAN_DLL ANSI_X923_Padding : public BlockCipherModePaddingMethod
   {
   public:
      void pad(byte[], size_t, size_t) const;
      size_t unpad(const byte[], size_t) const;
      bool valid_blocksize(size_t) const;
      std::string name() const { return "X9.23"; }
   };

/**
* One And Zeros Padding
*/
class BOTAN_DLL OneAndZeros_Padding : public BlockCipherModePaddingMethod
   {
   public:
      void pad(byte[], size_t, size_t) const;
      size_t unpad(const byte[], size_t) const;
      bool valid_blocksize(size_t) const;
      std::string name() const { return "OneAndZeros"; }
   };

/**
* Null Padding
*/
class BOTAN_DLL Null_Padding : public BlockCipherModePaddingMethod
   {
   public:
      void pad(byte[], size_t, size_t) const { return; }
      size_t unpad(const byte[], size_t size) const { return size; }
      size_t pad_bytes(size_t, size_t) const { return 0; }
      bool valid_blocksize(size_t) const { return true; }
      std::string name() const { return "NoPadding"; }
   };

}


namespace Botan {

/**
* Key Derivation Function
*/
class BOTAN_DLL KDF : public Algorithm
   {
   public:
      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      */
      SecureVector<byte> derive_key(size_t key_len,
                                    const MemoryRegion<byte>& secret,
                                    const std::string& salt = "") const;

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      */
      SecureVector<byte> derive_key(size_t key_len,
                                    const MemoryRegion<byte>& secret,
                                    const MemoryRegion<byte>& salt) const;

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      * @param salt_len size of salt in bytes
      */
      SecureVector<byte> derive_key(size_t key_len,
                                    const MemoryRegion<byte>& secret,
                                    const byte salt[],
                                    size_t salt_len) const;

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param secret_len size of secret in bytes
      * @param salt a diversifier
      */
      SecureVector<byte> derive_key(size_t key_len,
                                    const byte secret[],
                                    size_t secret_len,
                                    const std::string& salt = "") const;

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param secret_len size of secret in bytes
      * @param salt a diversifier
      * @param salt_len size of salt in bytes
      */
      SecureVector<byte> derive_key(size_t key_len,
                                    const byte secret[],
                                    size_t secret_len,
                                    const byte salt[],
                                    size_t salt_len) const;

      void clear() {}

      virtual KDF* clone() const = 0;
   private:
      virtual SecureVector<byte>
         derive(size_t key_len,
                const byte secret[], size_t secret_len,
                const byte salt[], size_t salt_len) const = 0;
   };

/**
* Mask Generation Function
*/
class BOTAN_DLL MGF
   {
   public:
      virtual void mask(const byte in[], size_t in_len,
                              byte out[], size_t out_len) const = 0;

      virtual ~MGF() {}
   };

}


namespace Botan {

/**
* Encoding Method for Encryption
*/
class BOTAN_DLL EME
   {
   public:
      /**
      * Return the maximum input size in bytes we can support
      * @param keybits the size of the key in bits
      * @return upper bound of input in bytes
      */
      virtual size_t maximum_input_size(size_t keybits) const = 0;

      /**
      * Encode an input
      * @param in the plaintext
      * @param in_length length of plaintext in bytes
      * @param key_length length of the key in bits
      * @param rng a random number generator
      * @return encoded plaintext
      */
      SecureVector<byte> encode(const byte in[],
                                size_t in_length,
                                size_t key_length,
                                RandomNumberGenerator& rng) const;

      /**
      * Encode an input
      * @param in the plaintext
      * @param key_length length of the key in bits
      * @param rng a random number generator
      * @return encoded plaintext
      */
      SecureVector<byte> encode(const MemoryRegion<byte>& in,
                                size_t key_length,
                                RandomNumberGenerator& rng) const;

      /**
      * Decode an input
      * @param in the encoded plaintext
      * @param in_length length of encoded plaintext in bytes
      * @param key_length length of the key in bits
      * @return plaintext
      */
      SecureVector<byte> decode(const byte in[],
                                size_t in_length,
                                size_t key_length) const;

      /**
      * Decode an input
      * @param in the encoded plaintext
      * @param key_length length of the key in bits
      * @return plaintext
      */
      SecureVector<byte> decode(const MemoryRegion<byte>& in,
                                size_t key_length) const;

      virtual ~EME() {}
   private:
      /**
      * Encode an input
      * @param in the plaintext
      * @param in_length length of plaintext in bytes
      * @param key_length length of the key in bits
      * @param rng a random number generator
      * @return encoded plaintext
      */
      virtual SecureVector<byte> pad(const byte in[],
                                     size_t in_length,
                                     size_t key_length,
                                     RandomNumberGenerator& rng) const = 0;

      /**
      * Decode an input
      * @param in the encoded plaintext
      * @param in_length length of encoded plaintext in bytes
      * @param key_length length of the key in bits
      * @return plaintext
      */
      virtual SecureVector<byte> unpad(const byte in[],
                                       size_t in_length,
                                       size_t key_length) const = 0;
   };

}


namespace Botan {

/**
* Retrieve an object prototype from the global factory
* @param algo_spec an algorithm name
* @return constant prototype object (use clone to create usable object),
          library retains ownership
*/
inline const BlockCipher*
retrieve_block_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.prototype_block_cipher(algo_spec);
   }

/**
* Retrieve an object prototype from the global factory
* @param algo_spec an algorithm name
* @return constant prototype object (use clone to create usable object),
          library retains ownership
*/
inline const StreamCipher*
retrieve_stream_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.prototype_stream_cipher(algo_spec);
   }

/**
* Retrieve an object prototype from the global factory
* @param algo_spec an algorithm name
* @return constant prototype object (use clone to create usable object),
          library retains ownership
*/
inline const HashFunction*
retrieve_hash(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.prototype_hash_function(algo_spec);
   }

/**
* Retrieve an object prototype from the global factory
* @param algo_spec an algorithm name
* @return constant prototype object (use clone to create usable object),
          library retains ownership
*/
inline const MessageAuthenticationCode*
retrieve_mac(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.prototype_mac(algo_spec);
   }

/*
* Get an algorithm object
*  NOTE: these functions create and return new objects, letting the
* caller assume ownership of them
*/

/**
* Block cipher factory method.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the desired block cipher
* @return pointer to the block cipher object
*/
inline BlockCipher* get_block_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.make_block_cipher(algo_spec);
   }

/**
* Stream cipher factory method.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the desired stream cipher
* @return pointer to the stream cipher object
*/
inline StreamCipher* get_stream_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.make_stream_cipher(algo_spec);
   }

/**
* Hash function factory method.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the desired hash function
* @return pointer to the hash function object
*/
inline HashFunction* get_hash(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.make_hash_function(algo_spec);
   }

/**
* MAC factory method.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the desired MAC
* @return pointer to the MAC object
*/
inline MessageAuthenticationCode* get_mac(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return af.make_mac(algo_spec);
   }

/**
* Password based key derivation function factory method
* @param algo_spec the name of the desired PBKDF algorithm
* @return pointer to newly allocated object of that type
*/
BOTAN_DLL PBKDF* get_pbkdf(const std::string& algo_spec);

/**
* @deprecated Use get_pbkdf
* @param algo_spec the name of the desired algorithm
* @return pointer to newly allocated object of that type
*/
inline PBKDF* get_s2k(const std::string& algo_spec)
   {
   return get_pbkdf(algo_spec);
   }

/*
* Get an EMSA/EME/KDF/MGF function
*/
// NOTE: these functions create and return new objects, letting the
// caller assume ownership of them

/**
* Factory method for EME (message-encoding methods for encryption) objects
* @param algo_spec the name of the EME to create
* @return pointer to newly allocated object of that type
*/
BOTAN_DLL EME*  get_eme(const std::string& algo_spec);

/**
* Factory method for EMSA (message-encoding methods for signatures
* with appendix) objects
* @param algo_spec the name of the EME to create
* @return pointer to newly allocated object of that type
*/
BOTAN_DLL EMSA* get_emsa(const std::string& algo_spec);

/**
* Factory method for KDF (key derivation function)
* @param algo_spec the name of the KDF to create
* @return pointer to newly allocated object of that type
*/
BOTAN_DLL KDF*  get_kdf(const std::string& algo_spec);

/*
* Get a cipher object
*/

/**
* Factory method for general symmetric cipher filters.
* @param algo_spec the name of the desired cipher
* @param key the key to be used for encryption/decryption performed by
* the filter
* @param iv the initialization vector to be used
* @param direction determines whether the filter will be an encrypting
* or decrypting filter
* @return pointer to newly allocated encryption or decryption filter
*/
BOTAN_DLL Keyed_Filter* get_cipher(const std::string& algo_spec,
                                   const SymmetricKey& key,
                                   const InitializationVector& iv,
                                   Cipher_Dir direction);

/**
* Factory method for general symmetric cipher filters.
* @param algo_spec the name of the desired cipher
* @param key the key to be used for encryption/decryption performed by
* the filter
* @param direction determines whether the filter will be an encrypting
* or decrypting filter
* @return pointer to the encryption or decryption filter
*/
BOTAN_DLL Keyed_Filter* get_cipher(const std::string& algo_spec,
                                   const SymmetricKey& key,
                                   Cipher_Dir direction);

/**
* Factory method for general symmetric cipher filters. No key will be
* set in the filter.
*
* @param algo_spec the name of the desired cipher
* @param direction determines whether the filter will be an encrypting or
* decrypting filter
* @return pointer to the encryption or decryption filter
*/
BOTAN_DLL Keyed_Filter* get_cipher(const std::string& algo_spec,
                                   Cipher_Dir direction);

/**
* Check if an algorithm exists.
* @param algo_spec the name of the algorithm to check for
* @return true if the algorithm exists, false otherwise
*/
BOTAN_DLL bool have_algorithm(const std::string& algo_spec);

/**
* Check if a block cipher algorithm exists.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm to check for
* @return true if the algorithm exists, false otherwise
*/
inline bool have_block_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return (af.prototype_block_cipher(algo_spec) != 0);
   }

/**
* Check if a stream cipher algorithm exists.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm to check for
* @return true if the algorithm exists, false otherwise
*/
inline bool have_stream_cipher(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return (af.prototype_stream_cipher(algo_spec) != 0);
   }

/**
* Check if a hash algorithm exists.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm to check for
* @return true if the algorithm exists, false otherwise
*/
inline bool have_hash(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return (af.prototype_hash_function(algo_spec) != 0);
   }

/**
* Check if a MAC algorithm exists.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm to check for
* @return true if the algorithm exists, false otherwise
*/
inline bool have_mac(const std::string& algo_spec)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   return (af.prototype_mac(algo_spec) != 0);
   }

/*
* Query information about an algorithm
*/

/**
* Find out the block size of a certain symmetric algorithm.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm
* @return block size of the specified algorithm
*/
BOTAN_DLL size_t block_size_of(const std::string& algo_spec);

/**
* Find out the output length of a certain symmetric algorithm.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm
* @return output length of the specified algorithm
*/
BOTAN_DLL size_t output_length_of(const std::string& algo_spec);

/**
* Find out the minimum key size of a certain symmetric algorithm.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm
* @return minimum key length of the specified algorithm
*/
BOTAN_DEPRECATED("Retrieve object you want and then call key_spec")
BOTAN_DLL size_t min_keylength_of(const std::string& algo_spec);

/**
* Find out the maximum key size of a certain symmetric algorithm.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm
* @return maximum key length of the specified algorithm
*/
BOTAN_DEPRECATED("Retrieve object you want and then call key_spec")
BOTAN_DLL size_t max_keylength_of(const std::string& algo_spec);

/**
* Find out the size any valid key is a multiple of for a certain algorithm.
* @deprecated Call algorithm_factory() directly
*
* @param algo_spec the name of the algorithm
* @return size any valid key is a multiple of
*/
BOTAN_DEPRECATED("Retrieve object you want and then call key_spec")
BOTAN_DLL size_t keylength_multiple_of(const std::string& algo_spec);

}


namespace Botan {

/*
* Get information describing the version
*/

/**
* Get a human-readable string identifying the version of Botan.
* No particular format should be assumed.
* @return version string
*/
BOTAN_DLL std::string version_string();

/**
* Return the date this version of botan was released, in an integer of
* the form YYYYMMDD. For instance a version released on May 21, 2013
* would return the integer 20130521. If the currently running version
* is not an official release, this function will return 0 instead.
*
* @return release date, or zero if unreleased
*/
BOTAN_DLL u32bit version_datestamp();

/**
* Get the major version number.
* @return major version number
*/
BOTAN_DLL u32bit version_major();

/**
* Get the minor version number.
* @return minor version number
*/
BOTAN_DLL u32bit version_minor();

/**
* Get the patch number.
* @return patch number
*/
BOTAN_DLL u32bit version_patch();

/*
* Macros for compile-time version checks
*/
#define BOTAN_VERSION_CODE_FOR(a,b,c) ((a << 16) | (b << 8) | (c))

/**
* Compare using BOTAN_VERSION_CODE_FOR, as in
*  # if BOTAN_VERSION_CODE < BOTAN_VERSION_CODE_FOR(1,8,0)
*  #    error "Botan version too old"
*  # endif
*/
#define BOTAN_VERSION_CODE BOTAN_VERSION_CODE_FOR(BOTAN_VERSION_MAJOR, \
                                                  BOTAN_VERSION_MINOR, \
                                                  BOTAN_VERSION_PATCH)

}



#if defined(BOTAN_HAS_AUTO_SEEDING_RNG)

namespace Botan {

/**
* An automatically seeded PRNG
*/
class BOTAN_DLL AutoSeeded_RNG : public RandomNumberGenerator
   {
   public:
      void randomize(byte out[], size_t len)
         { rng->randomize(out, len); }

      bool is_seeded() const { return rng->is_seeded(); }

      void clear() { rng->clear(); }

      std::string name() const { return rng->name(); }

      void reseed(size_t poll_bits = 256) { rng->reseed(poll_bits); }

      void add_entropy_source(EntropySource* es)
         { rng->add_entropy_source(es); }

      void add_entropy(const byte in[], size_t len)
         { rng->add_entropy(in, len); }

      AutoSeeded_RNG() { rng = &global_state().global_rng(); }
   private:
      RandomNumberGenerator* rng;
   };

}

#endif


namespace Botan {

/**
* PKCS #10 Certificate Request.
*/
class BOTAN_DLL PKCS10_Request : public X509_Object
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
      MemoryVector<byte> raw_public_key() const;

      /**
      * Get the subject DN.
      * @return subject DN
      */
      X509_DN subject_dn() const;

      /**
      * Get the subject alternative name.
      * @return subject alternative name.
      */
      AlternativeName subject_alt_name() const;

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
      u32bit path_limit() const;

      /**
      * Get the challenge password for this request
      * @return challenge password for this request
      */
      std::string challenge_password() const;

      /**
      * Create a PKCS#10 Request from a data source.
      * @param source the data source providing the DER encoded request
      */
      PKCS10_Request(DataSource& source);

      /**
      * Create a PKCS#10 Request from a file.
      * @param filename the name of the file containing the DER or PEM
      * encoded request file
      */
      PKCS10_Request(const std::string& filename);
   private:
      void force_decode();
      void handle_attribute(const Attribute&);

      Data_Store info;
   };

}


namespace Botan {

/**
* Options for X.509 certificates.
*/
class BOTAN_DLL X509_Cert_Options
   {
   public:
      /**
      * the subject common name
      */
      std::string common_name;

      /**
      * the subject counry
      */
      std::string country;

      /**
      * the subject organization
      */
      std::string organization;

      /**
      * the subject organizational unit
      */
      std::string org_unit;

      /**
      * the subject locality
      */
      std::string locality;

      /**
      * the subject state
      */
      std::string state;

      /**
      * the subject serial number
      */
      std::string serial_number;

      /**
      * the subject email adress
      */
      std::string email;

      /**
      * the subject URI
      */
      std::string uri;

      /**
      * the subject IPv4 address
      */
      std::string ip;

      /**
      * the subject DNS
      */
      std::string dns;

      /**
      * the subject XMPP
      */
      std::string xmpp;

      /**
      * the subject challenge password
      */
      std::string challenge;

      /**
      * the subject notBefore
      */
      X509_Time start;
      /**
      * the subject notAfter
      */
      X509_Time end;

      /**
      * Indicates whether the certificate request
      */
      bool is_CA;

      /**
      * Indicates the BasicConstraints path limit
      */
      size_t path_limit;

      /**
      * The key constraints for the subject public key
      */
      Key_Constraints constraints;

      /**
      * The key extended constraints for the subject public key
      */
      std::vector<OID> ex_constraints;

      /**
      * Check the options set in this object for validity.
      */
      void sanity_check() const;

      /**
      * Mark the certificate as a CA certificate and set the path limit.
      * @param limit the path limit to be set in the BasicConstraints extension.
      */
      void CA_key(size_t limit = 1);

      /**
      * Set the notBefore of the certificate.
      * @param time the notBefore value of the certificate
      */
      void not_before(const std::string& time);

      /**
      * Set the notAfter of the certificate.
      * @param time the notAfter value of the certificate
      */
      void not_after(const std::string& time);

      /**
      * Add the key constraints of the KeyUsage extension.
      * @param constr the constraints to set
      */
      void add_constraints(Key_Constraints constr);

      /**
      * Add constraints to the ExtendedKeyUsage extension.
      * @param oid the oid to add
      */
      void add_ex_constraint(const OID& oid);

      /**
      * Add constraints to the ExtendedKeyUsage extension.
      * @param name the name to look up the oid to add
      */
      void add_ex_constraint(const std::string& name);

      /**
      * Construct a new options object
      * @param opts define the common name of this object. An example for this
      * parameter would be "common_name/country/organization/organizational_unit".
      * @param expire_time the expiration time (from the current clock in seconds)
      */
      X509_Cert_Options(const std::string& opts = "",
                        u32bit expire_time = 365 * 24 * 60 * 60);
   };

namespace X509 {

/**
* Create a self-signed X.509 certificate.
* @param opts the options defining the certificate to create
* @param key the private key used for signing, i.e. the key
* associated with this self-signed certificate
* @param hash_fn the hash function to use
* @param rng the rng to use
* @return newly created self-signed certificate
*/
BOTAN_DLL X509_Certificate
create_self_signed_cert(const X509_Cert_Options& opts,
                        const Private_Key& key,
                        const std::string& hash_fn,
                        RandomNumberGenerator& rng);

/**
* Create a PKCS#10 certificate request.
* @param opts the options defining the request to create
* @param key the key used to sign this request
* @param rng the rng to use
* @param hash_fn the hash function to use
* @return newly created PKCS#10 request
*/
BOTAN_DLL PKCS10_Request create_cert_req(const X509_Cert_Options& opts,
                                         const Private_Key& key,
                                         const std::string& hash_fn,
                                         RandomNumberGenerator& rng);

}

}


namespace Botan {

class BigInt;
class ASN1_Object;

/**
* General DER Encoding Object
*/
class BOTAN_DLL DER_Encoder
   {
   public:
      SecureVector<byte> get_contents();

      DER_Encoder& start_cons(ASN1_Tag type_tag,
                              ASN1_Tag class_tag = UNIVERSAL);
      DER_Encoder& end_cons();

      DER_Encoder& start_explicit(u16bit type_tag);
      DER_Encoder& end_explicit();

      DER_Encoder& raw_bytes(const byte val[], size_t len);
      DER_Encoder& raw_bytes(const MemoryRegion<byte>& val);

      DER_Encoder& encode_null();
      DER_Encoder& encode(bool b);
      DER_Encoder& encode(size_t s);
      DER_Encoder& encode(const BigInt& n);
      DER_Encoder& encode(const MemoryRegion<byte>& v, ASN1_Tag real_type);
      DER_Encoder& encode(const byte val[], size_t len, ASN1_Tag real_type);

      DER_Encoder& encode(bool b,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(size_t s,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(const BigInt& n,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(const MemoryRegion<byte>& v,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      DER_Encoder& encode(const byte v[], size_t len,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

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

      DER_Encoder& encode(const ASN1_Object& obj);
      DER_Encoder& encode_if(bool pred, DER_Encoder& enc);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const byte rep[], size_t length);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const MemoryRegion<byte>& rep);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              const std::string& str);

      DER_Encoder& add_object(ASN1_Tag type_tag, ASN1_Tag class_tag,
                              byte val);

   private:
      class DER_Sequence
         {
         public:
            ASN1_Tag tag_of() const;
            SecureVector<byte> get_contents();
            void add_bytes(const byte[], size_t);
            DER_Sequence(ASN1_Tag, ASN1_Tag);
         private:
            ASN1_Tag type_tag, class_tag;
            SecureVector<byte> contents;
            std::vector< SecureVector<byte> > set_contents;
         };

      SecureVector<byte> contents;
      std::vector<DER_Sequence> subsequences;
   };

}


namespace Botan {

/**
* EME1, aka OAEP
*/
class BOTAN_DLL EME1 : public EME
   {
   public:
      size_t maximum_input_size(size_t) const;

      /**
      * @param hash object to use for hashing (takes ownership)
      * @param P an optional label. Normally empty.
      */
      EME1(HashFunction* hash, const std::string& P = "");

      ~EME1() { delete mgf; }
   private:
      SecureVector<byte> pad(const byte[], size_t, size_t,
                             RandomNumberGenerator&) const;
      SecureVector<byte> unpad(const byte[], size_t, size_t) const;

      SecureVector<byte> Phash;
      MGF* mgf;
   };

}


namespace Botan {

/**
* The two types of signature format supported by Botan.
*/
enum Signature_Format { IEEE_1363, DER_SEQUENCE };

/**
* Enum marking if protection against fault attacks should be used
*/
enum Fault_Protection {
   ENABLE_FAULT_PROTECTION,
   DISABLE_FAULT_PROTECTION
};

/**
* Public Key Encryptor
*/
class BOTAN_DLL PK_Encryptor
   {
   public:

      /**
      * Encrypt a message.
      * @param in the message as a byte array
      * @param length the length of the above byte array
      * @param rng the random number source to use
      * @return encrypted message
      */
      SecureVector<byte> encrypt(const byte in[], size_t length,
                                 RandomNumberGenerator& rng) const
         {
         return enc(in, length, rng);
         }

      /**
      * Encrypt a message.
      * @param in the message
      * @param rng the random number source to use
      * @return encrypted message
      */
      SecureVector<byte> encrypt(const MemoryRegion<byte>& in,
                                 RandomNumberGenerator& rng) const
         {
         return enc(&in[0], in.size(), rng);
         }

      /**
      * Return the maximum allowed message size in bytes.
      * @return maximum message size in bytes
      */
      virtual size_t maximum_input_size() const = 0;

      PK_Encryptor() {}
      virtual ~PK_Encryptor() {}
   private:
      PK_Encryptor(const PK_Encryptor&) {}
      PK_Encryptor& operator=(const PK_Encryptor&) { return *this; }

      virtual SecureVector<byte> enc(const byte[], size_t,
                                     RandomNumberGenerator&) const = 0;
   };

/**
* Public Key Decryptor
*/
class BOTAN_DLL PK_Decryptor
   {
   public:
      /**
      * Decrypt a ciphertext.
      * @param in the ciphertext as a byte array
      * @param length the length of the above byte array
      * @return decrypted message
      */
      SecureVector<byte> decrypt(const byte in[], size_t length) const
         {
         return dec(in, length);
         }

      /**
      * Decrypt a ciphertext.
      * @param in the ciphertext
      * @return decrypted message
      */
      SecureVector<byte> decrypt(const MemoryRegion<byte>& in) const
         {
         return dec(&in[0], in.size());
         }

      PK_Decryptor() {}
      virtual ~PK_Decryptor() {}
   private:
      PK_Decryptor(const PK_Decryptor&) {}
      PK_Decryptor& operator=(const PK_Decryptor&) { return *this; }

      virtual SecureVector<byte> dec(const byte[], size_t) const = 0;
   };

/**
* Public Key Signer. Use the sign_message() functions for small
* messages. Use multiple calls update() to process large messages and
* generate the signature by finally calling signature().
*/
class BOTAN_DLL PK_Signer
   {
   public:
      /**
      * Sign a message.
      * @param in the message to sign as a byte array
      * @param length the length of the above byte array
      * @param rng the rng to use
      * @return signature
      */
      SecureVector<byte> sign_message(const byte in[], size_t length,
                                      RandomNumberGenerator& rng);

      /**
      * Sign a message.
      * @param in the message to sign
      * @param rng the rng to use
      * @return signature
      */
      SecureVector<byte> sign_message(const MemoryRegion<byte>& in,
                                      RandomNumberGenerator& rng)
         { return sign_message(&in[0], in.size(), rng); }

      /**
      * Add a message part (single byte).
      * @param in the byte to add
      */
      void update(byte in) { update(&in, 1); }

      /**
      * Add a message part.
      * @param in the message part to add as a byte array
      * @param length the length of the above byte array
      */
      void update(const byte in[], size_t length);

      /**
      * Add a message part.
      * @param in the message part to add
      */
      void update(const MemoryRegion<byte>& in) { update(&in[0], in.size()); }

      /**
      * Get the signature of the so far processed message (provided by the
      * calls to update()).
      * @param rng the rng to use
      * @return signature of the total message
      */
      SecureVector<byte> signature(RandomNumberGenerator& rng);

      /**
      * Set the output format of the signature.
      * @param format the signature format to use
      */
      void set_output_format(Signature_Format format) { sig_format = format; }

      /**
      * Construct a PK Signer.
      * @param key the key to use inside this signer
      * @param emsa the EMSA to use
      * An example would be "EMSA1(SHA-224)".
      * @param format the signature format to use
      * @param prot says if fault protection should be enabled
      */
      PK_Signer(const Private_Key& key,
                const std::string& emsa,
                Signature_Format format = IEEE_1363,
                Fault_Protection prot = ENABLE_FAULT_PROTECTION);

      ~PK_Signer() { delete op; delete verify_op; delete emsa; }
   private:
      bool self_test_signature(const MemoryRegion<byte>& msg,
                               const MemoryRegion<byte>& sig) const;

      PK_Signer(const PK_Signer&) {}
      PK_Signer& operator=(const PK_Signer&) { return *this; }

      PK_Ops::Signature* op;
      PK_Ops::Verification* verify_op;
      EMSA* emsa;
      Signature_Format sig_format;
   };

/**
* Public Key Verifier. Use the verify_message() functions for small
* messages. Use multiple calls update() to process large messages and
* verify the signature by finally calling check_signature().
*/
class BOTAN_DLL PK_Verifier
   {
   public:
      /**
      * Verify a signature.
      * @param msg the message that the signature belongs to, as a byte array
      * @param msg_length the length of the above byte array msg
      * @param sig the signature as a byte array
      * @param sig_length the length of the above byte array sig
      * @return true if the signature is valid
      */
      bool verify_message(const byte msg[], size_t msg_length,
                          const byte sig[], size_t sig_length);
      /**
      * Verify a signature.
      * @param msg the message that the signature belongs to
      * @param sig the signature
      * @return true if the signature is valid
      */
      bool verify_message(const MemoryRegion<byte>& msg,
                          const MemoryRegion<byte>& sig)
         {
         return verify_message(&msg[0], msg.size(),
                               &sig[0], sig.size());
         }

      /**
      * Add a message part (single byte) of the message corresponding to the
      * signature to be verified.
      * @param in the byte to add
      */
      void update(byte in) { update(&in, 1); }

      /**
      * Add a message part of the message corresponding to the
      * signature to be verified.
      * @param msg_part the new message part as a byte array
      * @param length the length of the above byte array
      */
      void update(const byte msg_part[], size_t length);

      /**
      * Add a message part of the message corresponding to the
      * signature to be verified.
      * @param in the new message part
      */
      void update(const MemoryRegion<byte>& in)
         { update(&in[0], in.size()); }

      /**
      * Check the signature of the buffered message, i.e. the one build
      * by successive calls to update.
      * @param sig the signature to be verified as a byte array
      * @param length the length of the above byte array
      * @return true if the signature is valid, false otherwise
      */
      bool check_signature(const byte sig[], size_t length);

      /**
      * Check the signature of the buffered message, i.e. the one build
      * by successive calls to update.
      * @param sig the signature to be verified
      * @return true if the signature is valid, false otherwise
      */
      bool check_signature(const MemoryRegion<byte>& sig)
         {
         return check_signature(&sig[0], sig.size());
         }

      /**
      * Set the format of the signatures fed to this verifier.
      * @param format the signature format to use
      */
      void set_input_format(Signature_Format format);

      /**
      * Construct a PK Verifier.
      * @param pub_key the public key to verify against
      * @param emsa the EMSA to use (eg "EMSA3(SHA-1)")
      * @param format the signature format to use
      */
      PK_Verifier(const Public_Key& pub_key,
                  const std::string& emsa,
                  Signature_Format format = IEEE_1363);

      ~PK_Verifier() { delete op; delete emsa; }
   private:
      PK_Verifier(const PK_Verifier&) {}
      PK_Verifier& operator=(const PK_Verifier&) { return *this; }

      bool validate_signature(const MemoryRegion<byte>& msg,
                              const byte sig[], size_t sig_len);

      PK_Ops::Verification* op;
      EMSA* emsa;
      Signature_Format sig_format;
   };

/**
* Key used for key agreement
*/
class BOTAN_DLL PK_Key_Agreement
   {
   public:

      /*
      * Perform Key Agreement Operation
      * @param key_len the desired key output size
      * @param in the other parties key
      * @param in_len the length of in in bytes
      * @param params extra derivation params
      * @param params_len the length of params in bytes
      */
      SymmetricKey derive_key(size_t key_len,
                              const byte in[],
                              size_t in_len,
                              const byte params[],
                              size_t params_len) const;

      /*
      * Perform Key Agreement Operation
      * @param key_len the desired key output size
      * @param in the other parties key
      * @param in_len the length of in in bytes
      * @param params extra derivation params
      * @param params_len the length of params in bytes
      */
      SymmetricKey derive_key(size_t key_len,
                              const MemoryRegion<byte>& in,
                              const byte params[],
                              size_t params_len) const
         {
         return derive_key(key_len, &in[0], in.size(),
                           params, params_len);
         }

      /*
      * Perform Key Agreement Operation
      * @param key_len the desired key output size
      * @param in the other parties key
      * @param in_len the length of in in bytes
      * @param params extra derivation params
      */
      SymmetricKey derive_key(size_t key_len,
                              const byte in[], size_t in_len,
                              const std::string& params = "") const
         {
         return derive_key(key_len, in, in_len,
                           reinterpret_cast<const byte*>(params.data()),
                           params.length());
         }

      /*
      * Perform Key Agreement Operation
      * @param key_len the desired key output size
      * @param in the other parties key
      * @param params extra derivation params
      */
      SymmetricKey derive_key(size_t key_len,
                              const MemoryRegion<byte>& in,
                              const std::string& params = "") const
         {
         return derive_key(key_len, &in[0], in.size(),
                           reinterpret_cast<const byte*>(params.data()),
                           params.length());
         }

      /**
      * Construct a PK Key Agreement.
      * @param key the key to use
      * @param kdf name of the KDF to use (or 'Raw' for no KDF)
      */
      PK_Key_Agreement(const PK_Key_Agreement_Key& key,
                       const std::string& kdf);

      ~PK_Key_Agreement() { delete op; delete kdf; }
   private:
      PK_Key_Agreement(const PK_Key_Agreement_Key&) {}
      PK_Key_Agreement& operator=(const PK_Key_Agreement&) { return *this; }

      PK_Ops::Key_Agreement* op;
      KDF* kdf;
   };

/**
* Encryption with an MR algorithm and an EME.
*/
class BOTAN_DLL PK_Encryptor_EME : public PK_Encryptor
   {
   public:
      size_t maximum_input_size() const;

      /**
      * Construct an instance.
      * @param key the key to use inside the decryptor
      * @param eme the EME to use
      */
      PK_Encryptor_EME(const Public_Key& key,
                       const std::string& eme);

      ~PK_Encryptor_EME() { delete op; delete eme; }
   private:
      SecureVector<byte> enc(const byte[], size_t,
                             RandomNumberGenerator& rng) const;

      PK_Ops::Encryption* op;
      const EME* eme;
   };

/**
* Decryption with an MR algorithm and an EME.
*/
class BOTAN_DLL PK_Decryptor_EME : public PK_Decryptor
   {
   public:
     /**
      * Construct an instance.
      * @param key the key to use inside the encryptor
      * @param eme the EME to use
      */
      PK_Decryptor_EME(const Private_Key& key,
                       const std::string& eme);

      ~PK_Decryptor_EME() { delete op; delete eme; }
   private:
      SecureVector<byte> dec(const byte[], size_t) const;

      PK_Ops::Decryption* op;
      const EME* eme;
   };

/*
* Typedefs for compatability with 1.8
*/
typedef PK_Encryptor_EME PK_Encryptor_MR_with_EME;
typedef PK_Decryptor_EME PK_Decryptor_MR_with_EME;

}


namespace Botan {

/**
* DES
*/
class BOTAN_DLL DES : public Block_Cipher_Fixed_Params<8, 8>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(round_key); }
      std::string name() const { return "DES"; }
      BlockCipher* clone() const { return new DES; }

      DES() : round_key(32) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> round_key;
   };

/**
* Triple DES
*/
class BOTAN_DLL TripleDES : public Block_Cipher_Fixed_Params<8, 16, 24, 8>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(round_key); }
      std::string name() const { return "TripleDES"; }
      BlockCipher* clone() const { return new TripleDES; }

      TripleDES() : round_key(96) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> round_key;
   };

/*
* DES Tables
*/
extern const u32bit DES_SPBOX1[256];
extern const u32bit DES_SPBOX2[256];
extern const u32bit DES_SPBOX3[256];
extern const u32bit DES_SPBOX4[256];
extern const u32bit DES_SPBOX5[256];
extern const u32bit DES_SPBOX6[256];
extern const u32bit DES_SPBOX7[256];
extern const u32bit DES_SPBOX8[256];

extern const u64bit DES_IPTAB1[256];
extern const u64bit DES_IPTAB2[256];
extern const u64bit DES_FPTAB1[256];
extern const u64bit DES_FPTAB2[256];

}


namespace Botan {

/**
* DESX
*/
class BOTAN_DLL DESX : public Block_Cipher_Fixed_Params<8, 24>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { des.clear(); zeroise(K1); zeroise(K2); }
      std::string name() const { return "DESX"; }
      BlockCipher* clone() const { return new DESX; }

      DESX() : K1(8), K2(8) {}
   private:
      void key_schedule(const byte[], size_t);
      SecureVector<byte> K1, K2;
      DES des;
   };

}


namespace Botan {

/**
* The GOST 28147-89 block cipher uses a set of 4 bit Sboxes, however
* the standard does not actually define these Sboxes; they are
* considered a local configuration issue. Several different sets are
* used.
*/
class BOTAN_DLL GOST_28147_89_Params
   {
   public:
      /**
      * @param row the row
      * @param col the column
      * @return sbox entry at this row/column
      */
      byte sbox_entry(size_t row, size_t col) const;

      /**
      * @return name of this parameter set
      */
      std::string param_name() const { return name; }

      /**
      * Default GOST parameters are the ones given in GOST R 34.11 for
      * testing purposes; these sboxes are also used by Crypto++, and,
      * at least according to Wikipedia, the Central Bank of Russian
      * Federation
      * @param name of the parameter set
      */
      GOST_28147_89_Params(const std::string& name = "R3411_94_TestParam");
   private:
      const byte* sboxes;
      std::string name;
   };

/**
* GOST 28147-89
*/
class BOTAN_DLL GOST_28147_89 : public Block_Cipher_Fixed_Params<8, 32>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); }

      std::string name() const;
      BlockCipher* clone() const { return new GOST_28147_89(SBOX); }

      /**
      * @param params the sbox parameters to use
      */
      GOST_28147_89(const GOST_28147_89_Params& params);
   private:
      GOST_28147_89(const SecureVector<u32bit>& other_SBOX) :
         SBOX(other_SBOX), EK(8) {}

      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> SBOX;
      SecureVector<u32bit> EK;
   };

}


namespace Botan {

/**
* GOST 34.11
*/
class BOTAN_DLL GOST_34_11 : public HashFunction
   {
   public:
      std::string name() const { return "GOST-R-34.11-94" ; }
      size_t output_length() const { return 32; }
      size_t hash_block_size() const { return 32; }
      HashFunction* clone() const { return new GOST_34_11; }

      void clear();

      GOST_34_11();
   private:
      void compress_n(const byte input[], size_t blocks);

      void add_data(const byte[], size_t);
      void final_result(byte[]);

      GOST_28147_89 cipher;
      SecureVector<byte> buffer, sum, hash;
      size_t position;
      u64bit count;
   };

}


namespace Botan {

/**
* SHA-384
*/
class BOTAN_DLL SHA_384 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "SHA-384"; }
      size_t output_length() const { return 48; }
      HashFunction* clone() const { return new SHA_384; }

      void clear();

      SHA_384() : MDx_HashFunction(128, true, true, 16), digest(8)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u64bit> digest;
   };

/**
* SHA-512
*/
class BOTAN_DLL SHA_512 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "SHA-512"; }
      size_t output_length() const { return 64; }
      HashFunction* clone() const { return new SHA_512; }

      void clear();

      SHA_512() : MDx_HashFunction(128, true, true, 16), digest(8)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u64bit> digest;
   };

}


namespace Botan {

/**
* Password Based Encryption (PBE) Filter.
*/
class BOTAN_DLL PBE : public Filter
   {
   public:
      /**
      * Set this filter's key.
      * @param pw the password to be used for the encryption
      */
      virtual void set_key(const std::string& pw) = 0;

      /**
      * Create a new random salt value and set the default iterations value.
      * @param rng a random number generator
      */
      virtual void new_params(RandomNumberGenerator& rng) = 0;

      /**
      * DER encode the params (the number of iterations and the salt value)
      * @return encoded params
      */
      virtual MemoryVector<byte> encode_params() const = 0;

      /**
      * Decode params and use them inside this Filter.
      * @param src a data source to read the encoded params from
      */
      virtual void decode_params(DataSource& src) = 0;

      /**
      * Get this PBE's OID.
      * @return object identifier
      */
      virtual OID get_oid() const = 0;
   };

}


namespace Botan {

/**
* KDF1, from IEEE 1363
*/
class BOTAN_DLL KDF1 : public KDF
   {
   public:
      SecureVector<byte> derive(size_t,
                                const byte secret[], size_t secret_len,
                                const byte P[], size_t P_len) const;

      std::string name() const { return "KDF1(" + hash->name() + ")"; }
      KDF* clone() const { return new KDF1(hash->clone()); }

      KDF1(HashFunction* h) : hash(h) {}
      KDF1(const KDF1& other) : KDF(), hash(other.hash->clone()) {}

      ~KDF1() { delete hash; }
   private:
      HashFunction* hash;
   };

}


namespace Botan {

/**
* MISTY1
*/
class BOTAN_DLL MISTY1 : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); zeroise(DK); }
      std::string name() const { return "MISTY1"; }
      BlockCipher* clone() const { return new MISTY1; }

      /**
      * @param rounds the number of rounds. Must be 8 with the current
      * implementation
      */
      MISTY1(size_t rounds = 8);
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u16bit> EK, DK;
   };

}


namespace Botan {

/**
* 32-bit cyclic redundancy check
*/
class BOTAN_DLL CRC32 : public HashFunction
   {
   public:
      std::string name() const { return "CRC32"; }
      size_t output_length() const { return 4; }
      HashFunction* clone() const { return new CRC32; }

      void clear() { crc = 0xFFFFFFFF; }

      CRC32() { clear(); }
      ~CRC32() { clear(); }
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      u32bit crc;
   };

}


namespace Botan {

namespace PEM_Code {

/*
* PEM Encoding/Decoding
*/
BOTAN_DLL std::string encode(const byte[], size_t,
                             const std::string&, size_t = 64);
BOTAN_DLL std::string encode(const MemoryRegion<byte>&,
                             const std::string&, size_t = 64);

BOTAN_DLL SecureVector<byte> decode(DataSource&, std::string&);
BOTAN_DLL SecureVector<byte> decode_check_label(DataSource&,
                                                const std::string&);
BOTAN_DLL bool matches(DataSource&, const std::string& = "",
                       size_t search_range = 4096);

}

}


namespace Botan {

/**
* XTEA
*/
class BOTAN_DLL XTEA : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); }
      std::string name() const { return "XTEA"; }
      BlockCipher* clone() const { return new XTEA; }

      XTEA() : EK(64) {}
   protected:
      /**
      * @return const reference to the key schedule
      */
      const SecureVector<u32bit>& get_EK() const { return EK; }

   private:
      void key_schedule(const byte[], size_t);
      SecureVector<u32bit> EK;
   };

}


namespace Botan {

/**
* KASUMI, the block cipher used in 3G telephony
*/
class BOTAN_DLL KASUMI : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); }
      std::string name() const { return "KASUMI"; }
      BlockCipher* clone() const { return new KASUMI; }

      KASUMI() : EK(64) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u16bit> EK;
   };

}


namespace Botan {

/**
* This class represents discrete logarithm groups. It holds a prime p,
* a prime q = (p-1)/2 and g = x^((p-1)/q) mod p.
*/
class BOTAN_DLL DL_Group
   {
   public:
      /**
      * Get the prime p.
      * @return prime p
      */
      const BigInt& get_p() const;

      /**
      * Get the prime q.
      * @return prime q
      */
      const BigInt& get_q() const;

      /**
      * Get the base g.
      * @return base g
      */
      const BigInt& get_g() const;

      /**
      * The DL group encoding format variants.
      */
      enum Format {
         ANSI_X9_42,
         ANSI_X9_57,
         PKCS_3,

         DSA_PARAMETERS = ANSI_X9_57,
         DH_PARAMETERS = ANSI_X9_42,
         X942_DH_PARAMETERS = ANSI_X9_42,
         PKCS3_DH_PARAMETERS = PKCS_3
      };

      /**
      * Determine the prime creation for DL groups.
      */
      enum PrimeType { Strong, Prime_Subgroup, DSA_Kosherizer };

      /**
      * Perform validity checks on the group.
      * @param rng the rng to use
      * @param strong whether to perform stronger by lengthier tests
      * @return true if the object is consistent, false otherwise
      */
      bool verify_group(RandomNumberGenerator& rng, bool strong) const;

      /**
      * Encode this group into a string using PEM encoding.
      * @param format the encoding format
      * @return string holding the PEM encoded group
      */
      std::string PEM_encode(Format format) const;

      /**
      * Encode this group into a string using DER encoding.
      * @param format the encoding format
      * @return string holding the DER encoded group
      */
      SecureVector<byte> DER_encode(Format format) const;

      /**
      * Decode a DER/BER encoded group into this instance.
      * @param src a DataSource providing the encoded group
      * @param format the format of the encoded group
      */
      void BER_decode(DataSource& src, Format format);

      /**
      * Decode a PEM encoded group into this instance.
      * @param src a DataSource providing the encoded group
      */
      void PEM_decode(DataSource& src);

      /**
      * Construct a DL group with uninitialized internal value.
      * Use this constructor is you wish to set the groups values
      * from a DER or PEM encoded group.
      */
      DL_Group();

      /**
      * Construct a DL group that is registered in the configuration.
      * @param name the name that is configured in the global configuration
      * for the desired group. If no configuration file is specified,
      * the default values from the file policy.cpp will be used. For instance,
      * use "modp/ietf/768" as name.
      */
      DL_Group(const std::string& name);

      /**
      * Create a new group randomly.
      * @param rng the random number generator to use
      * @param type specifies how the creation of primes p and q shall
      * be performed. If type=Strong, then p will be determined as a
      * safe prime, and q will be chosen as (p-1)/2. If
      * type=Prime_Subgroup and qbits = 0, then the size of q will be
      * determined according to the estimated difficulty of the DL
      * problem. If type=DSA_Kosherizer, DSA primes will be created.
      * @param pbits the number of bits of p
      * @param qbits the number of bits of q. Leave it as 0 to have
      * the value determined according to pbits.
      */
      DL_Group(RandomNumberGenerator& rng, PrimeType type,
               size_t pbits, size_t qbits = 0);

      /**
      * Create a DSA group with a given seed.
      * @param rng the random number generator to use
      * @param seed the seed to use to create the random primes
      * @param pbits the desired bit size of the prime p
      * @param qbits the desired bit size of the prime q.
      */
      DL_Group(RandomNumberGenerator& rng, const MemoryRegion<byte>& seed,
               size_t pbits = 1024, size_t qbits = 0);

      /**
      * Create a DL group. The prime q will be determined according to p.
      * @param p the prime p
      * @param g the base g
      */
      DL_Group(const BigInt& p, const BigInt& g);

      /**
      * Create a DL group.
      * @param p the prime p
      * @param q the prime q
      * @param g the base g
      */
      DL_Group(const BigInt& p, const BigInt& q, const BigInt& g);
   private:
      static BigInt make_dsa_generator(const BigInt&, const BigInt&);

      void init_check() const;
      void initialize(const BigInt&, const BigInt&, const BigInt&);
      bool initialized;
      BigInt p, q, g;
   };

}


namespace Botan {

/**
* This class represents discrete logarithm (DL) public keys.
*/
class BOTAN_DLL DL_Scheme_PublicKey : public virtual Public_Key
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const;

      AlgorithmIdentifier algorithm_identifier() const;

      MemoryVector<byte> x509_subject_public_key() const;

      /**
      * Get the DL domain parameters of this key.
      * @return DL domain parameters of this key
      */
      const DL_Group& get_domain() const { return group; }

      /**
      * Get the public value y with y = g^x mod p where x is the secret key.
      */
      const BigInt& get_y() const { return y; }

      /**
      * Get the prime p of the underlying DL group.
      * @return prime p
      */
      const BigInt& group_p() const { return group.get_p(); }

      /**
      * Get the prime q of the underlying DL group.
      * @return prime q
      */
      const BigInt& group_q() const { return group.get_q(); }

      /**
      * Get the generator g of the underlying DL group.
      * @return generator g
      */
      const BigInt& group_g() const { return group.get_g(); }

      /**
      * Get the underlying groups encoding format.
      * @return encoding format
      */
      virtual DL_Group::Format group_format() const = 0;

      DL_Scheme_PublicKey(const AlgorithmIdentifier& alg_id,
                          const MemoryRegion<byte>& key_bits,
                          DL_Group::Format group_format);

   protected:
      DL_Scheme_PublicKey() {}

      /**
      * The DL public key
      */
      BigInt y;

      /**
      * The DL group
      */
      DL_Group group;
   };

/**
* This class represents discrete logarithm (DL) private keys.
*/
class BOTAN_DLL DL_Scheme_PrivateKey : public virtual DL_Scheme_PublicKey,
                                       public virtual Private_Key
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const;

      /**
      * Get the secret key x.
      * @return secret key
      */
      const BigInt& get_x() const { return x; }

      MemoryVector<byte> pkcs8_private_key() const;

      DL_Scheme_PrivateKey(const AlgorithmIdentifier& alg_id,
                           const MemoryRegion<byte>& key_bits,
                           DL_Group::Format group_format);

   protected:
      DL_Scheme_PrivateKey() {}

      /**
      * The DL private key
      */
      BigInt x;
   };

}


namespace Botan {

/**
* Modular Reducer (using Barrett's technique)
*/
class BOTAN_DLL Modular_Reducer
   {
   public:
      const BigInt& get_modulus() const { return modulus; }

      BigInt reduce(const BigInt& x) const;

      /**
      * Multiply mod p
      * @param x
      * @param y
      * @return (x * y) % p
      */
      BigInt multiply(const BigInt& x, const BigInt& y) const
         { return reduce(x * y); }

      /**
      * Square mod p
      * @param x
      * @return (x * x) % p
      */
      BigInt square(const BigInt& x) const
         { return reduce(Botan::square(x)); }

      /**
      * Cube mod p
      * @param x
      * @return (x * x * x) % p
      */
      BigInt cube(const BigInt& x) const
         { return multiply(x, this->square(x)); }

      bool initialized() const { return (mod_words != 0); }

      Modular_Reducer() { mod_words = 0; }
      Modular_Reducer(const BigInt& mod);
   private:
      BigInt modulus, modulus_2, mu;
      size_t mod_words;
   };

}


namespace Botan {

/**
* Blinding Function Object
*/
class BOTAN_DLL Blinder
   {
   public:
      BigInt blind(const BigInt& x) const;
      BigInt unblind(const BigInt& x) const;

      bool initialized() const { return reducer.initialized(); }

      Blinder() {}

      /**
      * Construct a blinder
      * @param mask the forward (blinding) mask
      * @param inverse_mask the inverse of mask (depends on algo)
      * @param modulus of the group operations are performed in
      */
      Blinder(const BigInt& mask,
              const BigInt& inverse_mask,
              const BigInt& modulus);

   private:
      Modular_Reducer reducer;
      mutable BigInt e, d;
   };

}


namespace Botan {

/**
* This class represents Diffie-Hellman public keys.
*/
class BOTAN_DLL DH_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "DH"; }

      MemoryVector<byte> public_value() const;
      size_t max_input_bits() const { return group_p().bits(); }

      DL_Group::Format group_format() const { return DL_Group::ANSI_X9_42; }

      DH_PublicKey(const AlgorithmIdentifier& alg_id,
                   const MemoryRegion<byte>& key_bits) :
         DL_Scheme_PublicKey(alg_id, key_bits, DL_Group::ANSI_X9_42) {}

      /**
      * Construct a public key with the specified parameters.
      * @param grp the DL group to use in the key
      * @param y the public value y
      */
      DH_PublicKey(const DL_Group& grp, const BigInt& y);
   protected:
      DH_PublicKey() {}
   };

/**
* This class represents Diffie-Hellman private keys.
*/
class BOTAN_DLL DH_PrivateKey : public DH_PublicKey,
                                public PK_Key_Agreement_Key,
                                public virtual DL_Scheme_PrivateKey
   {
   public:
      MemoryVector<byte> public_value() const;

      /**
      * Load a DH private key
      * @param alg_id the algorithm id
      * @param key_bits the subject public key
      * @param rng a random number generator
      */
      DH_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits,
                    RandomNumberGenerator& rng);

      /**
      * Construct a private key with predetermined value.
      * @param rng random number generator to use
      * @param grp the group to be used in the key
      * @param x the key's secret value (or if zero, generate a new key)
      */
      DH_PrivateKey(RandomNumberGenerator& rng, const DL_Group& grp,
                    const BigInt& x = 0);
   };

/**
* DH operation
*/
class BOTAN_DLL DH_KA_Operation : public PK_Ops::Key_Agreement
   {
   public:
      DH_KA_Operation(const DH_PrivateKey& key);

      SecureVector<byte> agree(const byte w[], size_t w_len);
   private:
      const BigInt& p;

      Fixed_Exponent_Power_Mod powermod_x_p;
      Blinder blinder;
   };

}


namespace Botan {

/**
* CAST-256
*/
class BOTAN_DLL CAST_256 : public Block_Cipher_Fixed_Params<16, 4, 32, 4>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(MK); zeroise(RK); }
      std::string name() const { return "CAST-256"; }
      BlockCipher* clone() const { return new CAST_256; }

      CAST_256() : MK(48), RK(48) {}
   private:
      void key_schedule(const byte[], size_t);

      static const u32bit KEY_MASK[192];
      static const byte   KEY_ROT[32];

      SecureVector<u32bit> MK;
      SecureVector<byte> RK;
   };

extern const u32bit CAST_SBOX1[256];
extern const u32bit CAST_SBOX2[256];
extern const u32bit CAST_SBOX3[256];
extern const u32bit CAST_SBOX4[256];

}


namespace Botan {

/**
* MD2
*/
class BOTAN_DLL MD2 : public HashFunction
   {
   public:
      std::string name() const { return "MD2"; }
      size_t output_length() const { return 16; }
      size_t hash_block_size() const { return 16; }
      HashFunction* clone() const { return new MD2; }

      void clear();

      MD2() : X(48), checksum(16), buffer(16)
         { clear(); }
   private:
      void add_data(const byte[], size_t);
      void hash(const byte[]);
      void final_result(byte[]);

      SecureVector<byte> X, checksum, buffer;
      size_t position;
   };

}


namespace Botan {

/**
* RC6, Ron Rivest's AES candidate
*/
class BOTAN_DLL RC6 : public Block_Cipher_Fixed_Params<16, 1, 32>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(S); }
      std::string name() const { return "RC6"; }
      BlockCipher* clone() const { return new RC6; }

      RC6() : S(44) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> S;
   };

}


namespace Botan {

/**
* WiderWake4+1-BE
*
* Note: quite old and possibly not safe; use XSalsa20 or a block
* cipher in counter mode.
*/
class BOTAN_DLL WiderWake_41_BE : public StreamCipher
   {
   public:
      void cipher(const byte[], byte[], size_t);
      void set_iv(const byte[], size_t);

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == 8); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(16);
         }

      void clear();
      std::string name() const { return "WiderWake4+1-BE"; }
      StreamCipher* clone() const { return new WiderWake_41_BE; }

      WiderWake_41_BE() : T(256), state(5), t_key(4),
                          buffer(DEFAULT_BUFFERSIZE), position(0)
         {}

   private:
      void key_schedule(const byte[], size_t);

      void generate(size_t);

      SecureVector<u32bit> T;
      SecureVector<u32bit> state;
      SecureVector<u32bit> t_key;
      SecureVector<byte> buffer;
      size_t position;
   };

}


namespace Botan {

/**
* ElGamal Public Key
*/
class BOTAN_DLL ElGamal_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "ElGamal"; }
      DL_Group::Format group_format() const { return DL_Group::ANSI_X9_42; }

      size_t max_input_bits() const { return (group_p().bits() - 1); }

      ElGamal_PublicKey(const AlgorithmIdentifier& alg_id,
                        const MemoryRegion<byte>& key_bits) :
         DL_Scheme_PublicKey(alg_id, key_bits, DL_Group::ANSI_X9_42)
         {}

      ElGamal_PublicKey(const DL_Group& group, const BigInt& y);
   protected:
      ElGamal_PublicKey() {}
   };

/**
* ElGamal Private Key
*/
class BOTAN_DLL ElGamal_PrivateKey : public ElGamal_PublicKey,
                                     public virtual DL_Scheme_PrivateKey
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const;

      ElGamal_PrivateKey(const AlgorithmIdentifier& alg_id,
                         const MemoryRegion<byte>& key_bits,
                         RandomNumberGenerator& rng);

      ElGamal_PrivateKey(RandomNumberGenerator& rng,
                         const DL_Group& group,
                         const BigInt& priv_key = 0);
   };

/**
* ElGamal encryption operation
*/
class BOTAN_DLL ElGamal_Encryption_Operation : public PK_Ops::Encryption
   {
   public:
      size_t max_input_bits() const { return mod_p.get_modulus().bits() - 1; }

      ElGamal_Encryption_Operation(const ElGamal_PublicKey& key);

      SecureVector<byte> encrypt(const byte msg[], size_t msg_len,
                                 RandomNumberGenerator& rng);

   private:
      Fixed_Base_Power_Mod powermod_g_p, powermod_y_p;
      Modular_Reducer mod_p;
   };

/**
* ElGamal decryption operation
*/
class BOTAN_DLL ElGamal_Decryption_Operation : public PK_Ops::Decryption
   {
   public:
      size_t max_input_bits() const { return mod_p.get_modulus().bits() - 1; }

      ElGamal_Decryption_Operation(const ElGamal_PrivateKey& key);

      SecureVector<byte> decrypt(const byte msg[], size_t msg_len);
   private:
      Fixed_Exponent_Power_Mod powermod_x_p;
      Modular_Reducer mod_p;
      Blinder blinder;
   };

}


namespace Botan {

/**
HMAC_RNG - based on the design described in "On Extract-then-Expand
Key Derivation Functions and an HMAC-based KDF" by Hugo Krawczyk
(henceforce, 'E-t-E')

However it actually can be parameterized with any two MAC functions,
not restricted to HMAC (this variation is also described in Krawczyk's
paper), for instance one could use HMAC(SHA-512) as the extractor
and CMAC(AES-256) as the PRF.
*/
class BOTAN_DLL HMAC_RNG : public RandomNumberGenerator
   {
   public:
      void randomize(byte buf[], size_t len);
      bool is_seeded() const { return seeded; }
      void clear();
      std::string name() const;

      void reseed(size_t poll_bits);
      void add_entropy_source(EntropySource* es);
      void add_entropy(const byte[], size_t);

      /**
      * @param extractor a MAC used for extracting the entropy
      * @param prf a MAC used as a PRF using HKDF construction
      */
      HMAC_RNG(MessageAuthenticationCode* extractor,
               MessageAuthenticationCode* prf);

      ~HMAC_RNG();
   private:
      MessageAuthenticationCode* extractor;
      MessageAuthenticationCode* prf;

      std::vector<EntropySource*> entropy_sources;
      bool seeded;

      SecureVector<byte> K, io_buffer;
      size_t user_input_len;
      u32bit counter;
   };

}


namespace Botan {

/**
* A MAC only used in SSLv3. Do not use elsewhere! Use HMAC instead.
*/
class BOTAN_DLL SSL3_MAC : public MessageAuthenticationCode
   {
   public:
      std::string name() const;
      size_t output_length() const { return hash->output_length(); }
      MessageAuthenticationCode* clone() const;

      void clear();

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(hash->output_length());
         }

      /**
      * @param hash the underlying hash to use
      */
      SSL3_MAC(HashFunction* hash);
      ~SSL3_MAC() { delete hash; }
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      void key_schedule(const byte[], size_t);

      HashFunction* hash;
      SecureVector<byte> i_key, o_key;
   };

}


namespace Botan {

/**
EMSA1_BSI is a variant of EMSA1 specified by the BSI. It accepts only
hash values which are less or equal than the maximum key length. The
implementation comes from InSiTo
*/
class BOTAN_DLL EMSA1_BSI : public EMSA1
   {
   public:
      /**
      * @param hash the hash object to use
      */
      EMSA1_BSI(HashFunction* hash) : EMSA1(hash) {}
   private:
      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);
   };

}


namespace Botan {

/**
* Serpent, an AES finalist
*/
class BOTAN_DLL Serpent : public Block_Cipher_Fixed_Params<16, 16, 32, 8>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(round_key); }
      std::string name() const { return "Serpent"; }
      BlockCipher* clone() const { return new Serpent; }

      Serpent() : round_key(132) {}
   protected:
      /**
      * For use by subclasses using SIMD, asm, etc
      * @return const reference to the key schedule
      */
      const SecureVector<u32bit>& get_round_keys() const
         { return round_key; }

      /**
      * For use by subclasses that implement the key schedule
      * @param ks is the new key schedule value to set
      */
      void set_round_keys(const u32bit ks[132])
         {
         copy_mem(&round_key[0], ks, 132);
         }

   private:
      void key_schedule(const byte key[], size_t length);
      SecureVector<u32bit> round_key;
   };

}


namespace Botan {

/**
* NIST's SHA-160
*/
class BOTAN_DLL SHA_160 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "SHA-160"; }
      size_t output_length() const { return 20; }
      HashFunction* clone() const { return new SHA_160; }

      void clear();

      SHA_160() : MDx_HashFunction(64, true, true), digest(5), W(80)
         {
         clear();
         }
   protected:
      /**
      * Set a custom size for the W array. Normally 80, but some
      * subclasses need slightly more for best performance/internal
      * constraints
      * @param W_size how big to make W
      */
      SHA_160(size_t W_size) :
         MDx_HashFunction(64, true, true), digest(5), W(W_size)
         {
         clear();
         }

      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      /**
      * The digest value, exposed for use by subclasses (asm, SSE2)
      */
      SecureVector<u32bit> digest;

      /**
      * The message buffer, exposed for use by subclasses (asm, SSE2)
      */
      SecureVector<u32bit> W;
   };

}


namespace Botan {

/**
* ANSI X9.31 RNG
*/
class BOTAN_DLL ANSI_X931_RNG : public RandomNumberGenerator
   {
   public:
      void randomize(byte[], size_t);
      bool is_seeded() const;
      void clear();
      std::string name() const;

      void reseed(size_t poll_bits);
      void add_entropy_source(EntropySource*);
      void add_entropy(const byte[], size_t);

      /**
      * @param cipher the block cipher to use in this PRNG
      * @param rng the underlying PRNG for generating inputs
      * (eg, an HMAC_RNG)
      */
      ANSI_X931_RNG(BlockCipher* cipher,
                    RandomNumberGenerator* rng);
      ~ANSI_X931_RNG();
   private:
      void rekey();
      void update_buffer();

      BlockCipher* cipher;
      RandomNumberGenerator* prng;
      SecureVector<byte> V, R;
      size_t position;
   };

}


namespace Botan {

/**
* This class represents ECDSA Public Keys.
*/
class BOTAN_DLL ECDSA_PublicKey : public virtual EC_PublicKey
   {
   public:

      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      ECDSA_PublicKey(const EC_Group& dom_par,
                      const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      ECDSA_PublicKey(const AlgorithmIdentifier& alg_id,
                      const MemoryRegion<byte>& key_bits) :
         EC_PublicKey(alg_id, key_bits) {}

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name ("ECDSA")
      */
      std::string algo_name() const { return "ECDSA"; }

      /**
      * Get the maximum number of bits allowed to be fed to this key.
      * This is the bitlength of the order of the base point.
      * @result the maximum number of input bits
      */
      size_t max_input_bits() const { return domain().get_order().bits(); }

      size_t message_parts() const { return 2; }

      size_t message_part_size() const
         { return domain().get_order().bytes(); }

   protected:
      ECDSA_PublicKey() {}
   };

/**
* This class represents ECDSA Private Keys
*/
class BOTAN_DLL ECDSA_PrivateKey : public ECDSA_PublicKey,
                                   public EC_PrivateKey
   {
   public:

      /**
      * Load a private key
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      ECDSA_PrivateKey(const AlgorithmIdentifier& alg_id,
                       const MemoryRegion<byte>& key_bits) :
         EC_PrivateKey(alg_id, key_bits) {}

      /**
      * Generate a new private key
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key (if zero, generate a ney random key)
      */
      ECDSA_PrivateKey(RandomNumberGenerator& rng,
                       const EC_Group& domain,
                       const BigInt& x = 0) :
         EC_PrivateKey(rng, domain, x) {}

      bool check_key(RandomNumberGenerator& rng, bool) const;
   };

/**
* ECDSA signature operation
*/
class BOTAN_DLL ECDSA_Signature_Operation : public PK_Ops::Signature
   {
   public:
      ECDSA_Signature_Operation(const ECDSA_PrivateKey& ecdsa);

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return order.bytes(); }
      size_t max_input_bits() const { return order.bits(); }

   private:
      const PointGFp& base_point;
      const BigInt& order;
      const BigInt& x;
      Modular_Reducer mod_order;
   };

/**
* ECDSA verification operation
*/
class BOTAN_DLL ECDSA_Verification_Operation : public PK_Ops::Verification
   {
   public:
      ECDSA_Verification_Operation(const ECDSA_PublicKey& ecdsa);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return order.bytes(); }
      size_t max_input_bits() const { return order.bits(); }

      bool with_recovery() const { return false; }

      bool verify(const byte msg[], size_t msg_len,
                  const byte sig[], size_t sig_len);
   private:
      const PointGFp& base_point;
      const PointGFp& public_point;
      const BigInt& order;
   };

}


namespace Botan {

/**
* BigInt Division
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_DLL divide(const BigInt& x,
                      const BigInt& y,
                      BigInt& q,
                      BigInt& r);

}


namespace Botan {

/**
* Square
*/
class BOTAN_DLL Square : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();
      std::string name() const { return "Square"; }
      BlockCipher* clone() const { return new Square; }

      Square() : EK(28), DK(28), ME(32), MD(32) {}
   private:
      void key_schedule(const byte[], size_t);

      static void transform(u32bit[4]);

      static const byte SE[256];
      static const byte SD[256];
      static const byte Log[256];
      static const byte ALog[255];

      static const u32bit TE0[256];
      static const u32bit TE1[256];
      static const u32bit TE2[256];
      static const u32bit TE3[256];
      static const u32bit TD0[256];
      static const u32bit TD1[256];
      static const u32bit TD2[256];
      static const u32bit TD3[256];

      SecureVector<u32bit> EK, DK;
      SecureVector<byte> ME, MD;
   };

}


namespace Botan {

/**
* Perform hex encoding
* @param output an array of at least input_length*2 bytes
* @param input is some binary data
* @param input_length length of input in bytes
* @param uppercase should output be upper or lower case?
*/
void BOTAN_DLL hex_encode(char output[],
                          const byte input[],
                          size_t input_length,
                          bool uppercase = true);

/**
* Perform hex encoding
* @param input some input
* @param input_length length of input in bytes
* @param uppercase should output be upper or lower case?
* @return hexadecimal representation of input
*/
std::string BOTAN_DLL hex_encode(const byte input[],
                                 size_t input_length,
                                 bool uppercase = true);

/**
* Perform hex encoding
* @param input some input
* @param uppercase should output be upper or lower case?
* @return hexadecimal representation of input
*/
std::string BOTAN_DLL hex_encode(const MemoryRegion<byte>& input,
                                 bool uppercase = true);

/**
* Perform hex decoding
* @param output an array of at least input_length/2 bytes
* @param input some hex input
* @param input_length length of input in bytes
* @param input_consumed is an output parameter which says how many
*        bytes of input were actually consumed. If less than
*        input_length, then the range input[consumed:length]
*        should be passed in later along with more input.
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL hex_decode(byte output[],
                            const char input[],
                            size_t input_length,
                            size_t& input_consumed,
                            bool ignore_ws = true);

/**
* Perform hex decoding
* @param output an array of at least input_length/2 bytes
* @param input some hex input
* @param input_length length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL hex_decode(byte output[],
                            const char input[],
                            size_t input_length,
                            bool ignore_ws = true);

/**
* Perform hex decoding
* @param output an array of at least input_length/2 bytes
* @param input some hex input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL hex_decode(byte output[],
                            const std::string& input,
                            bool ignore_ws = true);

/**
* Perform hex decoding
* @param input some hex input
* @param input_length the length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded hex output
*/
SecureVector<byte> BOTAN_DLL hex_decode(const char input[],
                                        size_t input_length,
                                        bool ignore_ws = true);

/**
* Perform hex decoding
* @param input some hex input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded hex output
*/
SecureVector<byte> BOTAN_DLL hex_decode(const std::string& input,
                                        bool ignore_ws = true);

}


namespace Botan {

/**
* Block Cipher Cascade
*/
class BOTAN_DLL Cascade_Cipher : public BlockCipher
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      size_t block_size() const { return block; }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(cipher1->maximum_keylength() +
                                         cipher2->maximum_keylength());
         }

      void clear();
      std::string name() const;
      BlockCipher* clone() const;

      /**
      * Create a cascade of two block ciphers
      * @param cipher1 the first cipher
      * @param cipher2 the second cipher
      */
      Cascade_Cipher(BlockCipher* cipher1, BlockCipher* cipher2);

      ~Cascade_Cipher();
   private:
      void key_schedule(const byte[], size_t);

      size_t block;
      BlockCipher* cipher1;
      BlockCipher* cipher2;
   };


}


namespace Botan {

/**
* EME from PKCS #1 v1.5
*/
class BOTAN_DLL EME_PKCS1v15 : public EME
   {
   public:
      size_t maximum_input_size(size_t) const;
   private:
      SecureVector<byte> pad(const byte[], size_t, size_t,
                             RandomNumberGenerator&) const;
      SecureVector<byte> unpad(const byte[], size_t, size_t) const;
   };

}


namespace Botan {

/**
* Bit rotation left
* @param input the input word
* @param rot the number of bits to rotate
* @return input rotated left by rot bits
*/
template<typename T> inline T rotate_left(T input, size_t rot)
   {
   return static_cast<T>((input << rot) | (input >> (8*sizeof(T)-rot)));;
   }

/**
* Bit rotation right
* @param input the input word
* @param rot the number of bits to rotate
* @return input rotated right by rot bits
*/
template<typename T> inline T rotate_right(T input, size_t rot)
   {
   return static_cast<T>((input >> rot) | (input << (8*sizeof(T)-rot)));
   }

}


#if defined(BOTAN_TARGET_CPU_HAS_SSE2) && !defined(BOTAN_NO_SSE_INTRINSICS)
  #include <emmintrin.h>
#endif

namespace Botan {

/**
* Swap a 16 bit integer
*/
inline u16bit reverse_bytes(u16bit val)
   {
   return rotate_left(val, 8);
   }

/**
* Swap a 32 bit integer
*/
inline u32bit reverse_bytes(u32bit val)
   {
#if BOTAN_GCC_VERSION >= 430 && !defined(BOTAN_TARGET_CPU_IS_ARM_FAMILY)
   /*
   GCC intrinsic added in 4.3, works for a number of CPUs

   However avoid under ARM, as it branches to a function in libgcc
   instead of generating inline asm, so slower even than the generic
   rotate version below.
   */
   return __builtin_bswap32(val);

#elif BOTAN_USE_GCC_INLINE_ASM && defined(BOTAN_TARGET_CPU_IS_X86_FAMILY)

   // GCC-style inline assembly for x86 or x86-64
   asm("bswapl %0" : "=r" (val) : "0" (val));
   return val;

#elif BOTAN_USE_GCC_INLINE_ASM && defined(BOTAN_TARGET_CPU_IS_ARM_FAMILY)

   asm ("eor r3, %1, %1, ror #16\n\t"
        "bic r3, r3, #0x00FF0000\n\t"
        "mov %0, %1, ror #8\n\t"
        "eor %0, %0, r3, lsr #8"
        : "=r" (val)
        : "0" (val)
        : "r3", "cc");

   return val;

#else

   // Generic implementation
   return (rotate_right(val, 8) & 0xFF00FF00) |
          (rotate_left (val, 8) & 0x00FF00FF);

#endif
   }

/**
* Swap a 64 bit integer
*/
inline u64bit reverse_bytes(u64bit val)
   {
#if BOTAN_GCC_VERSION >= 430

   // GCC intrinsic added in 4.3, works for a number of CPUs
   return __builtin_bswap64(val);

#elif BOTAN_USE_GCC_INLINE_ASM && defined(BOTAN_TARGET_ARCH_IS_X86_64)
   // GCC-style inline assembly for x86-64
   asm("bswapq %0" : "=r" (val) : "0" (val));
   return val;

#else
   /* Generic implementation. Defined in terms of 32-bit bswap so any
    * optimizations in that version can help here (particularly
    * useful for 32-bit x86).
    */

   u32bit hi = static_cast<u32bit>(val >> 32);
   u32bit lo = static_cast<u32bit>(val);

   hi = reverse_bytes(hi);
   lo = reverse_bytes(lo);

   return (static_cast<u64bit>(lo) << 32) | hi;
#endif
   }

/**
* Swap 4 Ts in an array
*/
template<typename T>
inline void bswap_4(T x[4])
   {
   x[0] = reverse_bytes(x[0]);
   x[1] = reverse_bytes(x[1]);
   x[2] = reverse_bytes(x[2]);
   x[3] = reverse_bytes(x[3]);
   }

#if defined(BOTAN_TARGET_CPU_HAS_SSE2) && !defined(BOTAN_NO_SSE_INTRINSICS)

/**
* Swap 4 u32bits in an array using SSE2 shuffle instructions
*/
template<>
inline void bswap_4(u32bit x[4])
   {
   __m128i T = _mm_loadu_si128(reinterpret_cast<const __m128i*>(x));

   T = _mm_shufflehi_epi16(T, _MM_SHUFFLE(2, 3, 0, 1));
   T = _mm_shufflelo_epi16(T, _MM_SHUFFLE(2, 3, 0, 1));

   T =  _mm_or_si128(_mm_srli_epi16(T, 8), _mm_slli_epi16(T, 8));

   _mm_storeu_si128(reinterpret_cast<__m128i*>(x), T);
   }

#endif

}


namespace Botan {

/**
* MD5
*/
class BOTAN_DLL MD5 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "MD5"; }
      size_t output_length() const { return 16; }
      HashFunction* clone() const { return new MD5; }

      void clear();

      MD5() : MDx_HashFunction(64, false, true), M(16), digest(4)
         { clear(); }
   protected:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      /**
      * The message buffer, exposed for use by subclasses (x86 asm)
      */
      SecureVector<u32bit> M;

      /**
      * The digest value, exposed for use by subclasses (x86 asm)
      */
      SecureVector<u32bit> digest;
   };

}


namespace Botan {

/**
* Filter mixin that breaks input into blocks, useful for
* cipher modes
*/
class BOTAN_DLL Buffered_Filter
   {
   public:
      /**
      * Write bytes into the buffered filter, which will them emit them
      * in calls to buffered_block in the subclass
      * @param in the input bytes
      * @param length of in in bytes
      */
      void write(const byte in[], size_t length);

      /**
      * Finish a message, emitting to buffered_block and buffered_final
      * Will throw an exception if less than final_minimum bytes were
      * written into the filter.
      */
      void end_msg();

      /**
      * Initialize a Buffered_Filter
      * @param block_size the function buffered_block will be called
      *        with inputs which are a multiple of this size
      * @param final_minimum the function buffered_final will be called
      *        with at least this many bytes.
      */
      Buffered_Filter(size_t block_size, size_t final_minimum);

      virtual ~Buffered_Filter() {}
   protected:
      /**
      * The block processor, implemented by subclasses
      * @param input some input bytes
      * @param length the size of input, guaranteed to be a multiple
      *        of block_size
      */
      virtual void buffered_block(const byte input[], size_t length) = 0;

      /**
      * The final block, implemented by subclasses
      * @param input some input bytes
      * @param length the size of input, guaranteed to be at least
      *        final_minimum bytes
      */
      virtual void buffered_final(const byte input[], size_t length) = 0;

      /**
      * @return block size of inputs
      */
      size_t buffered_block_size() const { return main_block_mod; }

      /**
      * @return current position in the buffer
      */
      size_t current_position() const { return buffer_pos; }

      /**
      * Reset the buffer position
      */
      void buffer_reset() { buffer_pos = 0; }
   private:
      size_t main_block_mod, final_minimum;

      SecureVector<byte> buffer;
      size_t buffer_pos;
   };

}


namespace Botan {

/**
* ECB Encryption
*/
class BOTAN_DLL ECB_Encryption : public Keyed_Filter,
                                 private Buffered_Filter
   {
   public:
      std::string name() const;

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      ECB_Encryption(BlockCipher* ciph,
                     BlockCipherModePaddingMethod* pad);

      ECB_Encryption(BlockCipher* ciph,
                     BlockCipherModePaddingMethod* pad,
                     const SymmetricKey& key);

      ~ECB_Encryption();
   private:
      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      void write(const byte input[], size_t input_length);
      void end_msg();

      BlockCipher* cipher;
      BlockCipherModePaddingMethod* padder;
      SecureVector<byte> temp;
   };

/**
* ECB Decryption
*/
class BOTAN_DLL ECB_Decryption : public Keyed_Filter,
                                 public Buffered_Filter
   {
   public:
      std::string name() const;

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      ECB_Decryption(BlockCipher* ciph,
                     BlockCipherModePaddingMethod* pad);

      ECB_Decryption(BlockCipher* ciph,
                     BlockCipherModePaddingMethod* pad,
                     const SymmetricKey& key);

      ~ECB_Decryption();
   private:
      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      void write(const byte input[], size_t input_length);
      void end_msg();

      BlockCipher* cipher;
      BlockCipherModePaddingMethod* padder;
      SecureVector<byte> temp;
   };

}


namespace Botan {

/**
* The different charsets (nominally) supported by Botan.
*/
enum Character_Set {
   LOCAL_CHARSET,
   UCS2_CHARSET,
   UTF8_CHARSET,
   LATIN1_CHARSET
};

namespace Charset {

/*
* Character Set Handling
*/
std::string BOTAN_DLL transcode(const std::string& str,
                                Character_Set to,
                                Character_Set from);

bool BOTAN_DLL is_digit(char c);
bool BOTAN_DLL is_space(char c);
bool BOTAN_DLL caseless_cmp(char x, char y);

byte BOTAN_DLL char2digit(char c);
char BOTAN_DLL digit2char(byte b);

}

}


namespace Botan {

/**
* This class represents public keys
* of integer factorization based (IF) public key schemes.
*/
class BOTAN_DLL IF_Scheme_PublicKey : public virtual Public_Key
   {
   public:
      IF_Scheme_PublicKey(const AlgorithmIdentifier& alg_id,
                          const MemoryRegion<byte>& key_bits);

      IF_Scheme_PublicKey(const BigInt& n, const BigInt& e) :
         n(n), e(e) {}

      bool check_key(RandomNumberGenerator& rng, bool) const;

      AlgorithmIdentifier algorithm_identifier() const;

      MemoryVector<byte> x509_subject_public_key() const;

      /**
      * @return public modulus
      */
      const BigInt& get_n() const { return n; }

      /**
      * @return public exponent
      */
      const BigInt& get_e() const { return e; }

      size_t max_input_bits() const { return (n.bits() - 1); }

   protected:
      IF_Scheme_PublicKey() {}

      BigInt n, e;
   };

/**
* This class represents public keys
* of integer factorization based (IF) public key schemes.
*/
class BOTAN_DLL IF_Scheme_PrivateKey : public virtual IF_Scheme_PublicKey,
                                       public virtual Private_Key
   {
   public:

      IF_Scheme_PrivateKey(RandomNumberGenerator& rng,
                           const BigInt& prime1, const BigInt& prime2,
                           const BigInt& exp, const BigInt& d_exp,
                           const BigInt& mod);

      IF_Scheme_PrivateKey(RandomNumberGenerator& rng,
                           const AlgorithmIdentifier& alg_id,
                           const MemoryRegion<byte>& key_bits);

      bool check_key(RandomNumberGenerator& rng, bool) const;

      /**
      * Get the first prime p.
      * @return prime p
      */
      const BigInt& get_p() const { return p; }

      /**
      * Get the second prime q.
      * @return prime q
      */
      const BigInt& get_q() const { return q; }

      /**
      * Get d with exp * d = 1 mod (p - 1, q - 1).
      * @return d
      */
      const BigInt& get_d() const { return d; }

      const BigInt& get_c() const { return c; }
      const BigInt& get_d1() const { return d1; }
      const BigInt& get_d2() const { return d2; }

      MemoryVector<byte> pkcs8_private_key() const;

   protected:
      IF_Scheme_PrivateKey() {}

      BigInt d, p, q, d1, d2, c;
   };

}


namespace Botan {

/**
* RSA Public Key
*/
class BOTAN_DLL RSA_PublicKey : public virtual IF_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "RSA"; }

      RSA_PublicKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits) :
         IF_Scheme_PublicKey(alg_id, key_bits)
         {}

      /**
      * Create a RSA_PublicKey
      * @arg n the modulus
      * @arg e the exponent
      */
      RSA_PublicKey(const BigInt& n, const BigInt& e) :
         IF_Scheme_PublicKey(n, e)
         {}

   protected:
      RSA_PublicKey() {}
   };

/**
* RSA Private Key
*/
class BOTAN_DLL RSA_PrivateKey : public RSA_PublicKey,
                                 public IF_Scheme_PrivateKey
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const;

      RSA_PrivateKey(const AlgorithmIdentifier& alg_id,
                     const MemoryRegion<byte>& key_bits,
                     RandomNumberGenerator& rng) :
         IF_Scheme_PrivateKey(rng, alg_id, key_bits) {}

      /**
      * Construct a private key from the specified parameters.
      * @param rng a random number generator
      * @param p the first prime
      * @param q the second prime
      * @param e the exponent
      * @param d if specified, this has to be d with
      * exp * d = 1 mod (p - 1, q - 1). Leave it as 0 if you wish to
      * the constructor to calculate it.
      * @param n if specified, this must be n = p * q. Leave it as 0
      * if you wish to the constructor to calculate it.
      */
      RSA_PrivateKey(RandomNumberGenerator& rng,
                     const BigInt& p, const BigInt& q,
                     const BigInt& e, const BigInt& d = 0,
                     const BigInt& n = 0) :
         IF_Scheme_PrivateKey(rng, p, q, e, d, n) {}

      /**
      * Create a new private key with the specified bit length
      * @param rng the random number generator to use
      * @param bits the desired bit length of the private key
      * @param exp the public exponent to be used
      */
      RSA_PrivateKey(RandomNumberGenerator& rng,
                     size_t bits, size_t exp = 65537);
   };

/**
* RSA private (decrypt/sign) operation
*/
class BOTAN_DLL RSA_Private_Operation : public PK_Ops::Signature,
                                        public PK_Ops::Decryption
   {
   public:
      RSA_Private_Operation(const RSA_PrivateKey& rsa);

      size_t max_input_bits() const { return (n.bits() - 1); }

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);

      SecureVector<byte> decrypt(const byte msg[], size_t msg_len);

   private:
      BigInt private_op(const BigInt& m) const;

      const BigInt& n;
      const BigInt& q;
      const BigInt& c;
      Fixed_Exponent_Power_Mod powermod_e_n, powermod_d1_p, powermod_d2_q;
      Modular_Reducer mod_p;
      Blinder blinder;
   };

/**
* RSA public (encrypt/verify) operation
*/
class BOTAN_DLL RSA_Public_Operation : public PK_Ops::Verification,
                                       public PK_Ops::Encryption
   {
   public:
      RSA_Public_Operation(const RSA_PublicKey& rsa) :
         n(rsa.get_n()), powermod_e_n(rsa.get_e(), rsa.get_n())
         {}

      size_t max_input_bits() const { return (n.bits() - 1); }
      bool with_recovery() const { return true; }

      SecureVector<byte> encrypt(const byte msg[], size_t msg_len,
                                 RandomNumberGenerator&)
         {
         BigInt m(msg, msg_len);
         return BigInt::encode_1363(public_op(m), n.bytes());
         }

      SecureVector<byte> verify_mr(const byte msg[], size_t msg_len)
         {
         BigInt m(msg, msg_len);
         return BigInt::encode(public_op(m));
         }

   private:
      BigInt public_op(const BigInt& m) const
         {
         if(m >= n)
            throw Invalid_Argument("RSA public op - input is too large");
         return powermod_e_n(m);
         }

      const BigInt& n;
      Fixed_Exponent_Power_Mod powermod_e_n;
   };

}


namespace Botan {

/**
* RIPEMD-160
*/
class BOTAN_DLL RIPEMD_160 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "RIPEMD-160"; }
      size_t output_length() const { return 20; }
      HashFunction* clone() const { return new RIPEMD_160; }

      void clear();

      RIPEMD_160() : MDx_HashFunction(64, false, true), M(16), digest(5)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u32bit> M, digest;
   };

}


namespace Botan {

/**
* Whirlpool
*/
class BOTAN_DLL Whirlpool : public MDx_HashFunction
   {
   public:
      std::string name() const { return "Whirlpool"; }
      size_t output_length() const { return 64; }
      HashFunction* clone() const { return new Whirlpool; }

      void clear();

      Whirlpool() : MDx_HashFunction(64, true, true, 32), M(8), digest(8)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      static const u64bit C0[256];
      static const u64bit C1[256];
      static const u64bit C2[256];
      static const u64bit C3[256];
      static const u64bit C4[256];
      static const u64bit C5[256];
      static const u64bit C6[256];
      static const u64bit C7[256];

      SecureVector<u64bit> M, digest;
   };

}


namespace Botan {

/**
* Tiger
*/
class BOTAN_DLL Tiger : public MDx_HashFunction
   {
   public:
      std::string name() const;
      size_t output_length() const { return hash_len; }

      HashFunction* clone() const
         {
         return new Tiger(output_length(), passes);
         }

      void clear();

      /**
      * @param out_size specifies the output length; can be 16, 20, or 24
      * @param passes to make in the algorithm
      */
      Tiger(size_t out_size = 24, size_t passes = 3);
   private:
      void compress_n(const byte[], size_t block);
      void copy_out(byte[]);

      static void pass(u64bit& A, u64bit& B, u64bit& C,
                       const MemoryRegion<u64bit>& M,
                       byte mul);

      static const u64bit SBOX1[256];
      static const u64bit SBOX2[256];
      static const u64bit SBOX3[256];
      static const u64bit SBOX4[256];

      SecureVector<u64bit> X, digest;
      const size_t hash_len, passes;
   };

}


namespace Botan {

/**
* SEED, a Korean block cipher
*/
class BOTAN_DLL SEED : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(K); }
      std::string name() const { return "SEED"; }
      BlockCipher* clone() const { return new SEED; }

      SEED() : K(32) {}
   private:
      void key_schedule(const byte[], size_t);

      class G_FUNC
         {
         public:
            u32bit operator()(u32bit) const;
         private:
            static const u32bit S0[256], S1[256], S2[256], S3[256];
         };

      SecureVector<u32bit> K;
   };

}


namespace Botan {

/**
* EMSA3 from IEEE 1363
* aka PKCS #1 v1.5 signature padding
* aka PKCS #1 block type 1
*/
class BOTAN_DLL EMSA3 : public EMSA
   {
   public:
      /**
      * @param hash the hash object to use
      */
      EMSA3(HashFunction* hash);
      ~EMSA3();

      void update(const byte[], size_t);

      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);

      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);
   private:
      HashFunction* hash;
      SecureVector<byte> hash_id;
   };

/**
* EMSA3_Raw which is EMSA3 without a hash or digest id (which
* according to QCA docs is "identical to PKCS#11's CKM_RSA_PKCS
* mechanism", something I have not confirmed)
*/
class BOTAN_DLL EMSA3_Raw : public EMSA
   {
   public:
      void update(const byte[], size_t);

      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);

      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);

   private:
      SecureVector<byte> message;
   };

}


namespace Botan {

/**
* Twofish, an AES finalist
*/
class BOTAN_DLL Twofish : public Block_Cipher_Fixed_Params<16, 16, 32, 8>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();
      std::string name() const { return "Twofish"; }
      BlockCipher* clone() const { return new Twofish; }

      Twofish() : SB(1024), RK(40) {}
   private:
      void key_schedule(const byte[], size_t);

      static void rs_mul(byte[4], byte, size_t);

      static const u32bit MDS0[256];
      static const u32bit MDS1[256];
      static const u32bit MDS2[256];
      static const u32bit MDS3[256];
      static const byte Q0[256];
      static const byte Q1[256];
      static const byte RS[32];
      static const byte EXP_TO_POLY[255];
      static const byte POLY_TO_EXP[255];

      SecureVector<u32bit> SB, RK;
   };

}


namespace Botan {

/**
* Create a password hash using PBKDF2
* @param password the password
* @param rng a random number generator
* @param work_factor how much work to do to slow down guessing attacks
* @param alg_id specifies which PRF to use with PBKDF2
*        0 is HMAC(SHA-1)
*        1 is HMAC(SHA-256)
*        2 is CMAC(Blowfish)
*        all other values are currently undefined
*/
std::string BOTAN_DLL generate_passhash9(const std::string& password,
                                         RandomNumberGenerator& rng,
                                         u16bit work_factor = 10,
                                         byte alg_id = 0);

/**
* Check a previously created password hash
* @param password the password to check against
* @param hash the stored hash to check against
*/
bool BOTAN_DLL check_passhash9(const std::string& password,
                               const std::string& hash);

}


namespace Botan {

/**
* SHA-224
*/
class BOTAN_DLL SHA_224 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "SHA-224"; }
      size_t output_length() const { return 28; }
      HashFunction* clone() const { return new SHA_224; }

      void clear();

      SHA_224() : MDx_HashFunction(64, true, true), digest(8)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u32bit> digest;
   };

/**
* SHA-256
*/
class BOTAN_DLL SHA_256 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "SHA-256"; }
      size_t output_length() const { return 32; }
      HashFunction* clone() const { return new SHA_256; }

      void clear();

      SHA_256() : MDx_HashFunction(64, true, true), digest(8)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u32bit> digest;
   };

}


namespace Botan {

/**
* Blue Midnight Wish 512 (Round 2 tweaked version)
*/
class BOTAN_DLL BMW_512 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "BMW512"; }
      size_t output_length() const { return 64; }
      HashFunction* clone() const { return new BMW_512; }

      void clear();

      BMW_512() : MDx_HashFunction(128, false, true), H(16), M(16), Q(32)
         { clear(); }
   private:
      void compress_n(const byte input[], size_t blocks);
      void copy_out(byte output[]);

      SecureVector<u64bit> H, M, Q;
   };

}


namespace Botan {

/**
* @param input the input data
* @param length length of input in bytes
* @param label the human-readable label
* @param headers a set of key/value pairs included in the header
*/
BOTAN_DLL std::string PGP_encode(
   const byte input[],
   size_t length,
   const std::string& label,
   const std::map<std::string, std::string>& headers);

/**
* @param input the input data
* @param length length of input in bytes
* @param label the human-readable label
*/
BOTAN_DLL std::string PGP_encode(
   const byte input[],
   size_t length,
   const std::string& label);

/**
* @param source the input source
* @param label is set to the human-readable label
* @param headers is set to any headers
* @return decoded output as raw binary
*/
BOTAN_DLL SecureVector<byte> PGP_decode(
   DataSource& source,
   std::string& label,
   std::map<std::string, std::string>& headers);

/**
* @param source the input source
* @param label is set to the human-readable label
* @return decoded output as raw binary
*/
BOTAN_DLL SecureVector<byte> PGP_decode(
   DataSource& source,
   std::string& label);

}


namespace Botan {

/**
* Certificate Store Interface
*/
class BOTAN_DLL Certificate_Store
   {
   public:
      virtual ~Certificate_Store() {}

      virtual Certificate_Store* clone() const = 0;

      /**
      * Add a certificate; this may fail if the store is write-only
      */
      virtual void add_certificate(const X509_Certificate& cert) = 0;

      /**
      * Add a CRL; this may fail if the store is write-only
      */
      virtual void add_crl(const X509_CRL& crl) = 0;

      /**
      * Subject DN and (optionally) key identifier
      */
      virtual std::vector<X509_Certificate>
         find_cert_by_subject_and_key_id(
            const X509_DN& subject_dn,
            const MemoryRegion<byte>& key_id) const = 0;

      /**
      * Find CRLs by the DN and key id of the issuer
      */
      virtual std::vector<X509_CRL>
         find_crl_by_subject_and_key_id(
            const X509_DN& issuer_dn,
            const MemoryRegion<byte>& key_id) const = 0;
   };

/**
* In Memory Certificate Store
*/
class BOTAN_DLL Certificate_Store_Memory : public Certificate_Store
   {
   public:
      Certificate_Store* clone() const;

      void add_certificate(const X509_Certificate& cert);

      void add_crl(const X509_CRL& crl);

      std::vector<X509_Certificate> find_cert_by_subject_and_key_id(
         const X509_DN& subject_dn,
         const MemoryRegion<byte>& key_id) const;

      std::vector<X509_CRL> find_crl_by_subject_and_key_id(
         const X509_DN& issuer_dn,
         const MemoryRegion<byte>& key_id) const;

      Certificate_Store_Memory() {}
   private:
      // TODO: Add indexing on the DN and key id to avoid linear search?
      std::vector<X509_Certificate> certs;
      std::vector<X509_CRL> crls;
   };

// TODO: file-backed store

}


namespace Botan {

/**
* KDF2, from IEEE 1363
*/
class BOTAN_DLL KDF2 : public KDF
   {
   public:
      SecureVector<byte> derive(size_t, const byte[], size_t,
                                const byte[], size_t) const;

      std::string name() const { return "KDF2(" + hash->name() + ")"; }
      KDF* clone() const { return new KDF2(hash->clone()); }

      KDF2(HashFunction* h) : hash(h) {}
      KDF2(const KDF2& other) : KDF(), hash(other.hash->clone()) {}
      ~KDF2() { delete hash; }
   private:
      HashFunction* hash;
   };

}


namespace Botan {

namespace KeyPair {

/**
* Tests whether the key is consistent for encryption; whether
* encrypting and then decrypting gives to the original plaintext.
* @param rng the rng to use
* @param key the key to test
* @param padding the encryption padding method to use
* @return true if consistent otherwise false
*/
BOTAN_DLL bool
encryption_consistency_check(RandomNumberGenerator& rng,
                             const Private_Key& key,
                             const std::string& padding);

/**
* Tests whether the key is consistent for signatures; whether a
* signature can be created and then verified
* @param rng the rng to use
* @param key the key to test
* @param padding the signature padding method to use
* @return true if consistent otherwise false
*/
BOTAN_DLL bool
signature_consistency_check(RandomNumberGenerator& rng,
                            const Private_Key& key,
                            const std::string& padding);

}

}


namespace Botan {

/**
* This namespace holds various high-level crypto functions
*/
namespace CryptoBox {

/**
* Encrypt a message using a passphrase
* @param input the input data
* @param input_len the length of input in bytes
* @param passphrase the passphrase used to encrypt the message
* @param rng a ref to a random number generator, such as AutoSeeded_RNG
*/
BOTAN_DLL std::string encrypt(const byte input[], size_t input_len,
                              const std::string& passphrase,
                              RandomNumberGenerator& rng);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param input_len the length of input in bytes
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DLL std::string decrypt(const byte input[], size_t input_len,
                              const std::string& passphrase);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DLL std::string decrypt(const std::string& input,
                              const std::string& passphrase);

}

}


namespace Botan {

/**
* X.509 Certificate Validation Result
*/
enum X509_Code {
   VERIFIED,
   UNKNOWN_X509_ERROR,
   CANNOT_ESTABLISH_TRUST,
   CERT_CHAIN_TOO_LONG,
   SIGNATURE_ERROR,
   POLICY_ERROR,
   INVALID_USAGE,

   CERT_FORMAT_ERROR,
   CERT_ISSUER_NOT_FOUND,
   CERT_NOT_YET_VALID,
   CERT_HAS_EXPIRED,
   CERT_IS_REVOKED,

   CRL_FORMAT_ERROR,
   CRL_ISSUER_NOT_FOUND,
   CRL_NOT_YET_VALID,
   CRL_HAS_EXPIRED,

   CA_CERT_CANNOT_SIGN,
   CA_CERT_NOT_FOR_CERT_ISSUER,
   CA_CERT_NOT_FOR_CRL_ISSUER
};

/**
* X.509 Certificate Store
*/
class BOTAN_DLL X509_Store
   {
   public:
      enum Cert_Usage {
         ANY              = 0x00,
         TLS_SERVER       = 0x01,
         TLS_CLIENT       = 0x02,
         CODE_SIGNING     = 0x04,
         EMAIL_PROTECTION = 0x08,
         TIME_STAMPING    = 0x10,
         CRL_SIGNING      = 0x20
      };

      X509_Code validate_cert(const X509_Certificate&, Cert_Usage = ANY);

      std::vector<X509_Certificate> get_cert_chain(const X509_Certificate&);
      std::string PEM_encode() const;

      X509_Code add_crl(const X509_CRL&);
      void add_cert(const X509_Certificate&, bool = false);
      void add_certs(DataSource&);
      void add_trusted_certs(DataSource&);

      void add_new_certstore(Certificate_Store*);

      X509_Store(u32bit time_slack = 24*60*60,
                 u32bit cache_results = 30*60);

      X509_Store(const X509_Store&);
      ~X509_Store();
   private:
      X509_Store& operator=(const X509_Store&) { return (*this); }

      class BOTAN_DLL CRL_Data
         {
         public:
            X509_DN issuer;
            MemoryVector<byte> serial, auth_key_id;
            bool operator==(const CRL_Data&) const;
            bool operator!=(const CRL_Data&) const;
            bool operator<(const CRL_Data&) const;
         };

      class BOTAN_DLL Cert_Info
         {
         public:
            bool is_verified(u32bit timeout) const;
            bool is_trusted() const;
            X509_Code verify_result() const;
            void set_result(X509_Code) const;
            Cert_Info(const X509_Certificate&, bool = false);

            X509_Certificate cert;
            bool trusted;
         private:
            mutable bool checked;
            mutable X509_Code result;
            mutable u64bit last_checked;
         };

      static X509_Code check_sig(const X509_Object&, Public_Key*);

      size_t find_cert(const X509_DN&, const MemoryRegion<byte>&) const;
      X509_Code check_sig(const Cert_Info&, const Cert_Info&) const;
      void recompute_revoked_info() const;

      void do_add_certs(DataSource&, bool);
      X509_Code construct_cert_chain(const X509_Certificate&,
                                     std::vector<size_t>&, bool = false);

      size_t find_parent_of(const X509_Certificate&);
      bool is_revoked(const X509_Certificate&) const;

      static const size_t NO_CERT_FOUND = 0xFFFFFFFF;
      std::vector<Cert_Info> certs;
      std::vector<CRL_Data> revoked;
      std::vector<Certificate_Store*> stores;
      u32bit time_slack, validation_cache_timeout;
      mutable bool revoked_info_valid;
   };

}


namespace Botan {

/**
* Noekeon
*/
class BOTAN_DLL Noekeon : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();
      std::string name() const { return "Noekeon"; }
      BlockCipher* clone() const { return new Noekeon; }

      Noekeon() : EK(4), DK(4) {}
   protected:
      /**
      * The Noekeon round constants
      */
      static const byte RC[17];

      /**
      * @return const reference to encryption subkeys
      */
      const SecureVector<u32bit>& get_EK() const { return EK; }

      /**
      * @return const reference to decryption subkeys
      */
      const SecureVector<u32bit>& get_DK() const { return DK; }

   private:
      void key_schedule(const byte[], size_t);
      SecureVector<u32bit> EK, DK;
   };

}


namespace Botan {

/**
* Create a password hash using Bcrypt
* @param password the password
* @param rng a random number generator
* @param work_factor how much work to do to slow down guessing attacks
*
* @see http://www.usenix.org/events/usenix99/provos/provos_html/
*/
std::string BOTAN_DLL generate_bcrypt(const std::string& password,
                                      RandomNumberGenerator& rng,
                                      u16bit work_factor = 10);

/**
* Check a previously created password hash
* @param password the password to check against
* @param hash the stored hash to check against
*/
bool BOTAN_DLL check_bcrypt(const std::string& password,
                            const std::string& hash);

}


namespace Botan {

/**
* CFB Encryption
*/
class BOTAN_DLL CFB_Encryption : public Keyed_Filter
   {
   public:
      std::string name() const { return cipher->name() + "/CFB"; }

      void set_iv(const InitializationVector&);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CFB_Encryption(BlockCipher* cipher, size_t feedback = 0);

      CFB_Encryption(BlockCipher* cipher,
                     const SymmetricKey& key,
                     const InitializationVector& iv,
                     size_t feedback = 0);

      ~CFB_Encryption() { delete cipher; }
   private:
      void write(const byte[], size_t);

      BlockCipher* cipher;
      SecureVector<byte> buffer, state;
      size_t position, feedback;
   };

/**
* CFB Decryption
*/
class BOTAN_DLL CFB_Decryption : public Keyed_Filter
   {
   public:
      std::string name() const { return cipher->name() + "/CFB"; }

      void set_iv(const InitializationVector&);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CFB_Decryption(BlockCipher* cipher, size_t feedback = 0);

      CFB_Decryption(BlockCipher* cipher,
                     const SymmetricKey& key,
                     const InitializationVector& iv,
                     size_t feedback = 0);

      ~CFB_Decryption() { delete cipher; }
   private:
      void write(const byte[], size_t);

      BlockCipher* cipher;
      SecureVector<byte> buffer, state;
      size_t position, feedback;
   };

}


namespace Botan {

/**
* X.509 Certificate Extension
*/
class BOTAN_DLL Certificate_Extension
   {
   public:
      /**
      * @return OID representing this extension
      */
      OID oid_of() const;

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
      * @return short readable name
      */
      virtual std::string config_id() const = 0;

      /*
      * @return specific OID name
      */
      virtual std::string oid_name() const = 0;

      virtual ~Certificate_Extension() {}
   protected:
      friend class Extensions;
      virtual bool should_encode() const { return true; }
      virtual MemoryVector<byte> encode_inner() const = 0;
      virtual void decode_inner(const MemoryRegion<byte>&) = 0;
   };

/**
* X.509 Certificate Extension List
*/
class BOTAN_DLL Extensions : public ASN1_Object
   {
   public:
      void encode_into(class DER_Encoder&) const;
      void decode_from(class BER_Decoder&);

      void contents_to(Data_Store&, Data_Store&) const;

      void add(Certificate_Extension* extn, bool critical = false);

      Extensions& operator=(const Extensions&);

      Extensions(const Extensions&);
      Extensions(bool st = true) : should_throw(st) {}
      ~Extensions();
   private:
      static Certificate_Extension* get_extension(const OID&);

      std::vector<std::pair<Certificate_Extension*, bool> > extensions;
      bool should_throw;
   };

namespace Cert_Extension {

static const size_t NO_CERT_PATH_LIMIT = 0xFFFFFFF0;

/**
* Basic Constraints Extension
*/
class BOTAN_DLL Basic_Constraints : public Certificate_Extension
   {
   public:
      Basic_Constraints* copy() const
         { return new Basic_Constraints(is_ca, path_limit); }

      Basic_Constraints(bool ca = false, size_t limit = 0) :
         is_ca(ca), path_limit(limit) {}

      bool get_is_ca() const { return is_ca; }
      size_t get_path_limit() const;
   private:
      std::string config_id() const { return "basic_constraints"; }
      std::string oid_name() const { return "X509v3.BasicConstraints"; }

      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      bool is_ca;
      size_t path_limit;
   };

/**
* Key Usage Constraints Extension
*/
class BOTAN_DLL Key_Usage : public Certificate_Extension
   {
   public:
      Key_Usage* copy() const { return new Key_Usage(constraints); }

      Key_Usage(Key_Constraints c = NO_CONSTRAINTS) : constraints(c) {}

      Key_Constraints get_constraints() const { return constraints; }
   private:
      std::string config_id() const { return "key_usage"; }
      std::string oid_name() const { return "X509v3.KeyUsage"; }

      bool should_encode() const { return (constraints != NO_CONSTRAINTS); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      Key_Constraints constraints;
   };

/**
* Subject Key Identifier Extension
*/
class BOTAN_DLL Subject_Key_ID : public Certificate_Extension
   {
   public:
      Subject_Key_ID* copy() const { return new Subject_Key_ID(key_id); }

      Subject_Key_ID() {}
      Subject_Key_ID(const MemoryRegion<byte>&);

      MemoryVector<byte> get_key_id() const { return key_id; }
   private:
      std::string config_id() const { return "subject_key_id"; }
      std::string oid_name() const { return "X509v3.SubjectKeyIdentifier"; }

      bool should_encode() const { return (key_id.size() > 0); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      MemoryVector<byte> key_id;
   };

/**
* Authority Key Identifier Extension
*/
class BOTAN_DLL Authority_Key_ID : public Certificate_Extension
   {
   public:
      Authority_Key_ID* copy() const { return new Authority_Key_ID(key_id); }

      Authority_Key_ID() {}
      Authority_Key_ID(const MemoryRegion<byte>& k) : key_id(k) {}

      MemoryVector<byte> get_key_id() const { return key_id; }
   private:
      std::string config_id() const { return "authority_key_id"; }
      std::string oid_name() const { return "X509v3.AuthorityKeyIdentifier"; }

      bool should_encode() const { return (key_id.size() > 0); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      MemoryVector<byte> key_id;
   };

/**
* Alternative Name Extension Base Class
*/
class BOTAN_DLL Alternative_Name : public Certificate_Extension
   {
   public:
      AlternativeName get_alt_name() const { return alt_name; }

   protected:
      Alternative_Name(const AlternativeName&,
                       const std::string&, const std::string&);

      Alternative_Name(const std::string&, const std::string&);
   private:
      std::string config_id() const { return config_name_str; }
      std::string oid_name() const { return oid_name_str; }

      bool should_encode() const { return alt_name.has_items(); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      std::string config_name_str, oid_name_str;
      AlternativeName alt_name;
   };

/**
* Subject Alternative Name Extension
*/
class BOTAN_DLL Subject_Alternative_Name : public Alternative_Name
   {
   public:
      Subject_Alternative_Name* copy() const
         { return new Subject_Alternative_Name(get_alt_name()); }

      Subject_Alternative_Name(const AlternativeName& = AlternativeName());
   };

/**
* Issuer Alternative Name Extension
*/
class BOTAN_DLL Issuer_Alternative_Name : public Alternative_Name
   {
   public:
      Issuer_Alternative_Name* copy() const
         { return new Issuer_Alternative_Name(get_alt_name()); }

      Issuer_Alternative_Name(const AlternativeName& = AlternativeName());
   };

/**
* Extended Key Usage Extension
*/
class BOTAN_DLL Extended_Key_Usage : public Certificate_Extension
   {
   public:
      Extended_Key_Usage* copy() const { return new Extended_Key_Usage(oids); }

      Extended_Key_Usage() {}
      Extended_Key_Usage(const std::vector<OID>& o) : oids(o) {}

      std::vector<OID> get_oids() const { return oids; }
   private:
      std::string config_id() const { return "extended_key_usage"; }
      std::string oid_name() const { return "X509v3.ExtendedKeyUsage"; }

      bool should_encode() const { return (oids.size() > 0); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      std::vector<OID> oids;
   };

/**
* Certificate Policies Extension
*/
class BOTAN_DLL Certificate_Policies : public Certificate_Extension
   {
   public:
      Certificate_Policies* copy() const
         { return new Certificate_Policies(oids); }

      Certificate_Policies() {}
      Certificate_Policies(const std::vector<OID>& o) : oids(o) {}

      std::vector<OID> get_oids() const { return oids; }
   private:
      std::string config_id() const { return "policy_info"; }
      std::string oid_name() const { return "X509v3.CertificatePolicies"; }

      bool should_encode() const { return (oids.size() > 0); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      std::vector<OID> oids;
   };

/**
* CRL Number Extension
*/
class BOTAN_DLL CRL_Number : public Certificate_Extension
   {
   public:
      CRL_Number* copy() const;

      CRL_Number() : has_value(false), crl_number(0) {}
      CRL_Number(size_t n) : has_value(true), crl_number(n) {}

      size_t get_crl_number() const;
   private:
      std::string config_id() const { return "crl_number"; }
      std::string oid_name() const { return "X509v3.CRLNumber"; }

      bool should_encode() const { return has_value; }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      bool has_value;
      size_t crl_number;
   };

/**
* CRL Entry Reason Code Extension
*/
class BOTAN_DLL CRL_ReasonCode : public Certificate_Extension
   {
   public:
      CRL_ReasonCode* copy() const { return new CRL_ReasonCode(reason); }

      CRL_ReasonCode(CRL_Code r = UNSPECIFIED) : reason(r) {}

      CRL_Code get_reason() const { return reason; }
   private:
      std::string config_id() const { return "crl_reason"; }
      std::string oid_name() const { return "X509v3.ReasonCode"; }

      bool should_encode() const { return (reason != UNSPECIFIED); }
      MemoryVector<byte> encode_inner() const;
      void decode_inner(const MemoryRegion<byte>&);
      void contents_to(Data_Store&, Data_Store&) const;

      CRL_Code reason;
   };

}

}


namespace Botan {

/**
* DSA Public Key
*/
class BOTAN_DLL DSA_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "DSA"; }

      DL_Group::Format group_format() const { return DL_Group::ANSI_X9_57; }
      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return group_q().bytes(); }
      size_t max_input_bits() const { return group_q().bits(); }

      DSA_PublicKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits) :
         DL_Scheme_PublicKey(alg_id, key_bits, DL_Group::ANSI_X9_57)
         {
         }

      DSA_PublicKey(const DL_Group& group, const BigInt& y);
   protected:
      DSA_PublicKey() {}
   };

/**
* DSA Private Key
*/
class BOTAN_DLL DSA_PrivateKey : public DSA_PublicKey,
                                 public virtual DL_Scheme_PrivateKey
   {
   public:
      DSA_PrivateKey(const AlgorithmIdentifier& alg_id,
                     const MemoryRegion<byte>& key_bits,
                     RandomNumberGenerator& rng);

      DSA_PrivateKey(RandomNumberGenerator& rng,
                     const DL_Group& group,
                     const BigInt& private_key = 0);

      bool check_key(RandomNumberGenerator& rng, bool strong) const;
   };

/**
* Object that can create a DSA signature
*/
class BOTAN_DLL DSA_Signature_Operation : public PK_Ops::Signature
   {
   public:
      DSA_Signature_Operation(const DSA_PrivateKey& dsa);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return q.bytes(); }
      size_t max_input_bits() const { return q.bits(); }

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);
   private:
      const BigInt& q;
      const BigInt& x;
      Fixed_Base_Power_Mod powermod_g_p;
      Modular_Reducer mod_q;
   };

/**
* Object that can verify a DSA signature
*/
class BOTAN_DLL DSA_Verification_Operation : public PK_Ops::Verification
   {
   public:
      DSA_Verification_Operation(const DSA_PublicKey& dsa);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return q.bytes(); }
      size_t max_input_bits() const { return q.bits(); }

      bool with_recovery() const { return false; }

      bool verify(const byte msg[], size_t msg_len,
                  const byte sig[], size_t sig_len);
   private:
      const BigInt& q;
      const BigInt& y;

      Fixed_Base_Power_Mod powermod_g_p, powermod_y_p;
      Modular_Reducer mod_p, mod_q;
   };

}


namespace Botan {

/**
* PRF from ANSI X9.42
*/
class BOTAN_DLL X942_PRF : public KDF
   {
   public:
      SecureVector<byte> derive(size_t, const byte[], size_t,
                                const byte[], size_t) const;

      std::string name() const { return "X942_PRF(" + key_wrap_oid + ")"; }
      KDF* clone() const { return new X942_PRF(key_wrap_oid); }

      X942_PRF(const std::string& oid);
   private:
      std::string key_wrap_oid;
   };

}


namespace Botan {

/**
* TEA
*/
class BOTAN_DLL TEA : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(K); }
      std::string name() const { return "TEA"; }
      BlockCipher* clone() const { return new TEA; }

      TEA() : K(4) {}
   private:
      void key_schedule(const byte[], size_t);
      SecureVector<u32bit> K;
   };

}


namespace Botan {

/**
* Nyberg-Rueppel Public Key
*/
class BOTAN_DLL NR_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "NR"; }

      DL_Group::Format group_format() const { return DL_Group::ANSI_X9_57; }

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return group_q().bytes(); }
      size_t max_input_bits() const { return (group_q().bits() - 1); }

      NR_PublicKey(const AlgorithmIdentifier& alg_id,
                   const MemoryRegion<byte>& key_bits);

      NR_PublicKey(const DL_Group& group, const BigInt& pub_key);
   protected:
      NR_PublicKey() {}
   };

/**
* Nyberg-Rueppel Private Key
*/
class BOTAN_DLL NR_PrivateKey : public NR_PublicKey,
                                public virtual DL_Scheme_PrivateKey
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool strong) const;

      NR_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits,
                    RandomNumberGenerator& rng);

      NR_PrivateKey(RandomNumberGenerator& rng,
                    const DL_Group& group,
                    const BigInt& x = 0);
   };

/**
* Nyberg-Rueppel signature operation
*/
class BOTAN_DLL NR_Signature_Operation : public PK_Ops::Signature
   {
   public:
      NR_Signature_Operation(const NR_PrivateKey& nr);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return q.bytes(); }
      size_t max_input_bits() const { return (q.bits() - 1); }

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);
   private:
      const BigInt& q;
      const BigInt& x;
      Fixed_Base_Power_Mod powermod_g_p;
      Modular_Reducer mod_q;
   };

/**
* Nyberg-Rueppel verification operation
*/
class BOTAN_DLL NR_Verification_Operation : public PK_Ops::Verification
   {
   public:
      NR_Verification_Operation(const NR_PublicKey& nr);

      size_t message_parts() const { return 2; }
      size_t message_part_size() const { return q.bytes(); }
      size_t max_input_bits() const { return (q.bits() - 1); }

      bool with_recovery() const { return true; }

      SecureVector<byte> verify_mr(const byte msg[], size_t msg_len);
   private:
      const BigInt& q;
      const BigInt& y;

      Fixed_Base_Power_Mod powermod_g_p, powermod_y_p;
      Modular_Reducer mod_p, mod_q;
   };

}


namespace Botan {

/**
* Alleged RC4
*/
class BOTAN_DLL ARC4 : public StreamCipher
   {
   public:
      void cipher(const byte in[], byte out[], size_t length);

      void clear();
      std::string name() const;

      StreamCipher* clone() const { return new ARC4(SKIP); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(1, 256);
         }

      /**
      * @param skip skip this many initial bytes in the keystream
      */
      ARC4(size_t skip = 0);

      ~ARC4() { clear(); }
   private:
      void key_schedule(const byte[], size_t);
      void generate();

      const size_t SKIP;

      byte X, Y;
      SecureVector<byte> state;

      SecureVector<byte> buffer;
      size_t position;
   };

}


namespace Botan {

/**
* Rabin-Williams Public Key
*/
class BOTAN_DLL RW_PublicKey : public virtual IF_Scheme_PublicKey
   {
   public:
      std::string algo_name() const { return "RW"; }

      RW_PublicKey(const AlgorithmIdentifier& alg_id,
                   const MemoryRegion<byte>& key_bits) :
         IF_Scheme_PublicKey(alg_id, key_bits)
         {}

      RW_PublicKey(const BigInt& mod, const BigInt& exponent) :
         IF_Scheme_PublicKey(mod, exponent)
         {}

   protected:
      RW_PublicKey() {}
   };

/**
* Rabin-Williams Private Key
*/
class BOTAN_DLL RW_PrivateKey : public RW_PublicKey,
                                public IF_Scheme_PrivateKey
   {
   public:
      RW_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const MemoryRegion<byte>& key_bits,
                    RandomNumberGenerator& rng) :
         IF_Scheme_PrivateKey(rng, alg_id, key_bits) {}

      RW_PrivateKey(RandomNumberGenerator& rng,
                    const BigInt& p, const BigInt& q,
                    const BigInt& e, const BigInt& d = 0,
                    const BigInt& n = 0) :
         IF_Scheme_PrivateKey(rng, p, q, e, d, n) {}

      RW_PrivateKey(RandomNumberGenerator& rng, size_t bits, size_t = 2);

      bool check_key(RandomNumberGenerator& rng, bool) const;
   };

/**
* Rabin-Williams Signature Operation
*/
class BOTAN_DLL RW_Signature_Operation : public PK_Ops::Signature
   {
   public:
      RW_Signature_Operation(const RW_PrivateKey& rw);

      size_t max_input_bits() const { return (n.bits() - 1); }

      SecureVector<byte> sign(const byte msg[], size_t msg_len,
                              RandomNumberGenerator& rng);
   private:
      const BigInt& n;
      const BigInt& e;
      const BigInt& q;
      const BigInt& c;

      Fixed_Exponent_Power_Mod powermod_d1_p, powermod_d2_q;
      Modular_Reducer mod_p;
      Blinder blinder;
   };

/**
* Rabin-Williams Verification Operation
*/
class BOTAN_DLL RW_Verification_Operation : public PK_Ops::Verification
   {
   public:
      RW_Verification_Operation(const RW_PublicKey& rw) :
         n(rw.get_n()), powermod_e_n(rw.get_e(), rw.get_n())
         {}

      size_t max_input_bits() const { return (n.bits() - 1); }
      bool with_recovery() const { return true; }

      SecureVector<byte> verify_mr(const byte msg[], size_t msg_len);

   private:
      const BigInt& n;
      Fixed_Exponent_Power_Mod powermod_e_n;
   };

}


#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK

#if defined(BOTAN_TARGET_CPU_IS_BIG_ENDIAN)

#define BOTAN_ENDIAN_N2B(x) (x)
#define BOTAN_ENDIAN_B2N(x) (x)

#define BOTAN_ENDIAN_N2L(x) reverse_bytes(x)
#define BOTAN_ENDIAN_L2N(x) reverse_bytes(x)

#elif defined(BOTAN_TARGET_CPU_IS_LITTLE_ENDIAN)

#define BOTAN_ENDIAN_N2L(x) (x)
#define BOTAN_ENDIAN_L2N(x) (x)

#define BOTAN_ENDIAN_N2B(x) reverse_bytes(x)
#define BOTAN_ENDIAN_B2N(x) reverse_bytes(x)

#endif

#endif

namespace Botan {

/**
* Make a u16bit from two bytes
* @param i0 the first byte
* @param i1 the second byte
* @return i0 || i1
*/
inline u16bit make_u16bit(byte i0, byte i1)
   {
   return ((static_cast<u16bit>(i0) << 8) | i1);
   }

/**
* Make a u32bit from four bytes
* @param i0 the first byte
* @param i1 the second byte
* @param i2 the third byte
* @param i3 the fourth byte
* @return i0 || i1 || i2 || i3
*/
inline u32bit make_u32bit(byte i0, byte i1, byte i2, byte i3)
   {
   return ((static_cast<u32bit>(i0) << 24) |
           (static_cast<u32bit>(i1) << 16) |
           (static_cast<u32bit>(i2) <<  8) |
           (static_cast<u32bit>(i3)));
   }

/**
* Make a u32bit from eight bytes
* @param i0 the first byte
* @param i1 the second byte
* @param i2 the third byte
* @param i3 the fourth byte
* @param i4 the fifth byte
* @param i5 the sixth byte
* @param i6 the seventh byte
* @param i7 the eighth byte
* @return i0 || i1 || i2 || i3 || i4 || i5 || i6 || i7
*/
inline u64bit make_u64bit(byte i0, byte i1, byte i2, byte i3,
                          byte i4, byte i5, byte i6, byte i7)
    {
   return ((static_cast<u64bit>(i0) << 56) |
           (static_cast<u64bit>(i1) << 48) |
           (static_cast<u64bit>(i2) << 40) |
           (static_cast<u64bit>(i3) << 32) |
           (static_cast<u64bit>(i4) << 24) |
           (static_cast<u64bit>(i5) << 16) |
           (static_cast<u64bit>(i6) <<  8) |
           (static_cast<u64bit>(i7)));
    }

/**
* Load a big-endian word
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th T of in, as a big-endian value
*/
template<typename T>
inline T load_be(const byte in[], size_t off)
   {
   in += off * sizeof(T);
   T out = 0;
   for(size_t i = 0; i != sizeof(T); ++i)
      out = (out << 8) | in[i];
   return out;
   }

/**
* Load a little-endian word
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th T of in, as a litte-endian value
*/
template<typename T>
inline T load_le(const byte in[], size_t off)
   {
   in += off * sizeof(T);
   T out = 0;
   for(size_t i = 0; i != sizeof(T); ++i)
      out = (out << 8) | in[sizeof(T)-1-i];
   return out;
   }

/**
* Load a big-endian u16bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u16bit of in, as a big-endian value
*/
template<>
inline u16bit load_be<u16bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2B(*(reinterpret_cast<const u16bit*>(in) + off));
#else
   in += off * sizeof(u16bit);
   return make_u16bit(in[0], in[1]);
#endif
   }

/**
* Load a little-endian u16bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u16bit of in, as a little-endian value
*/
template<>
inline u16bit load_le<u16bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2L(*(reinterpret_cast<const u16bit*>(in) + off));
#else
   in += off * sizeof(u16bit);
   return make_u16bit(in[1], in[0]);
#endif
   }

/**
* Load a big-endian u32bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u32bit of in, as a big-endian value
*/
template<>
inline u32bit load_be<u32bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2B(*(reinterpret_cast<const u32bit*>(in) + off));
#else
   in += off * sizeof(u32bit);
   return make_u32bit(in[0], in[1], in[2], in[3]);
#endif
   }

/**
* Load a little-endian u32bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u32bit of in, as a little-endian value
*/
template<>
inline u32bit load_le<u32bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2L(*(reinterpret_cast<const u32bit*>(in) + off));
#else
   in += off * sizeof(u32bit);
   return make_u32bit(in[3], in[2], in[1], in[0]);
#endif
   }

/**
* Load a big-endian u64bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u64bit of in, as a big-endian value
*/
template<>
inline u64bit load_be<u64bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2B(*(reinterpret_cast<const u64bit*>(in) + off));
#else
   in += off * sizeof(u64bit);
   return make_u64bit(in[0], in[1], in[2], in[3],
                      in[4], in[5], in[6], in[7]);
#endif
   }

/**
* Load a little-endian u64bit
* @param in a pointer to some bytes
* @param off an offset into the array
* @return off'th u64bit of in, as a little-endian value
*/
template<>
inline u64bit load_le<u64bit>(const byte in[], size_t off)
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   return BOTAN_ENDIAN_N2L(*(reinterpret_cast<const u64bit*>(in) + off));
#else
   in += off * sizeof(u64bit);
   return make_u64bit(in[7], in[6], in[5], in[4],
                      in[3], in[2], in[1], in[0]);
#endif
   }

/**
* Load two little-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
*/
template<typename T>
inline void load_le(const byte in[], T& x0, T& x1)
   {
   x0 = load_le<T>(in, 0);
   x1 = load_le<T>(in, 1);
   }

/**
* Load four little-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
* @param x2 where the third word will be written
* @param x3 where the fourth word will be written
*/
template<typename T>
inline void load_le(const byte in[],
                    T& x0, T& x1, T& x2, T& x3)
   {
   x0 = load_le<T>(in, 0);
   x1 = load_le<T>(in, 1);
   x2 = load_le<T>(in, 2);
   x3 = load_le<T>(in, 3);
   }

/**
* Load eight little-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
* @param x2 where the third word will be written
* @param x3 where the fourth word will be written
* @param x4 where the fifth word will be written
* @param x5 where the sixth word will be written
* @param x6 where the seventh word will be written
* @param x7 where the eighth word will be written
*/
template<typename T>
inline void load_le(const byte in[],
                    T& x0, T& x1, T& x2, T& x3,
                    T& x4, T& x5, T& x6, T& x7)
   {
   x0 = load_le<T>(in, 0);
   x1 = load_le<T>(in, 1);
   x2 = load_le<T>(in, 2);
   x3 = load_le<T>(in, 3);
   x4 = load_le<T>(in, 4);
   x5 = load_le<T>(in, 5);
   x6 = load_le<T>(in, 6);
   x7 = load_le<T>(in, 7);
   }

/**
* Load a variable number of little-endian words
* @param out the output array of words
* @param in the input array of bytes
* @param count how many words are in in
*/
template<typename T>
inline void load_le(T out[],
                    const byte in[],
                    size_t count)
   {
#if defined(BOTAN_TARGET_CPU_HAS_KNOWN_ENDIANNESS)
   std::memcpy(out, in, sizeof(T)*count);

#if defined(BOTAN_TARGET_CPU_IS_BIG_ENDIAN)
   const size_t blocks = count - (count % 4);
   const size_t left = count - blocks;

   for(size_t i = 0; i != blocks; i += 4)
      bswap_4(out + i);

   for(size_t i = 0; i != left; ++i)
      out[blocks+i] = reverse_bytes(out[blocks+i]);
#endif

#else
   for(size_t i = 0; i != count; ++i)
      out[i] = load_le<T>(in, i);
#endif
   }

/**
* Load two big-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
*/
template<typename T>
inline void load_be(const byte in[], T& x0, T& x1)
   {
   x0 = load_be<T>(in, 0);
   x1 = load_be<T>(in, 1);
   }

/**
* Load four big-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
* @param x2 where the third word will be written
* @param x3 where the fourth word will be written
*/
template<typename T>
inline void load_be(const byte in[],
                    T& x0, T& x1, T& x2, T& x3)
   {
   x0 = load_be<T>(in, 0);
   x1 = load_be<T>(in, 1);
   x2 = load_be<T>(in, 2);
   x3 = load_be<T>(in, 3);
   }

/**
* Load eight big-endian words
* @param in a pointer to some bytes
* @param x0 where the first word will be written
* @param x1 where the second word will be written
* @param x2 where the third word will be written
* @param x3 where the fourth word will be written
* @param x4 where the fifth word will be written
* @param x5 where the sixth word will be written
* @param x6 where the seventh word will be written
* @param x7 where the eighth word will be written
*/
template<typename T>
inline void load_be(const byte in[],
                    T& x0, T& x1, T& x2, T& x3,
                    T& x4, T& x5, T& x6, T& x7)
   {
   x0 = load_be<T>(in, 0);
   x1 = load_be<T>(in, 1);
   x2 = load_be<T>(in, 2);
   x3 = load_be<T>(in, 3);
   x4 = load_be<T>(in, 4);
   x5 = load_be<T>(in, 5);
   x6 = load_be<T>(in, 6);
   x7 = load_be<T>(in, 7);
   }

/**
* Load a variable number of big-endian words
* @param out the output array of words
* @param in the input array of bytes
* @param count how many words are in in
*/
template<typename T>
inline void load_be(T out[],
                    const byte in[],
                    size_t count)
   {
#if defined(BOTAN_TARGET_CPU_HAS_KNOWN_ENDIANNESS)
   std::memcpy(out, in, sizeof(T)*count);

#if defined(BOTAN_TARGET_CPU_IS_LITTLE_ENDIAN)
   const size_t blocks = count - (count % 4);
   const size_t left = count - blocks;

   for(size_t i = 0; i != blocks; i += 4)
      bswap_4(out + i);

   for(size_t i = 0; i != left; ++i)
      out[blocks+i] = reverse_bytes(out[blocks+i]);
#endif

#else
   for(size_t i = 0; i != count; ++i)
      out[i] = load_be<T>(in, i);
#endif
   }

/**
* Store a big-endian u16bit
* @param in the input u16bit
* @param out the byte array to write to
*/
inline void store_be(u16bit in, byte out[2])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u16bit*>(out) = BOTAN_ENDIAN_B2N(in);
#else
   out[0] = get_byte(0, in);
   out[1] = get_byte(1, in);
#endif
   }

/**
* Store a little-endian u16bit
* @param in the input u16bit
* @param out the byte array to write to
*/
inline void store_le(u16bit in, byte out[2])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u16bit*>(out) = BOTAN_ENDIAN_L2N(in);
#else
   out[0] = get_byte(1, in);
   out[1] = get_byte(0, in);
#endif
   }

/**
* Store a big-endian u32bit
* @param in the input u32bit
* @param out the byte array to write to
*/
inline void store_be(u32bit in, byte out[4])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u32bit*>(out) = BOTAN_ENDIAN_B2N(in);
#else
   out[0] = get_byte(0, in);
   out[1] = get_byte(1, in);
   out[2] = get_byte(2, in);
   out[3] = get_byte(3, in);
#endif
   }

/**
* Store a little-endian u32bit
* @param in the input u32bit
* @param out the byte array to write to
*/
inline void store_le(u32bit in, byte out[4])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u32bit*>(out) = BOTAN_ENDIAN_L2N(in);
#else
   out[0] = get_byte(3, in);
   out[1] = get_byte(2, in);
   out[2] = get_byte(1, in);
   out[3] = get_byte(0, in);
#endif
   }

/**
* Store a big-endian u64bit
* @param in the input u64bit
* @param out the byte array to write to
*/
inline void store_be(u64bit in, byte out[8])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u64bit*>(out) = BOTAN_ENDIAN_B2N(in);
#else
   out[0] = get_byte(0, in);
   out[1] = get_byte(1, in);
   out[2] = get_byte(2, in);
   out[3] = get_byte(3, in);
   out[4] = get_byte(4, in);
   out[5] = get_byte(5, in);
   out[6] = get_byte(6, in);
   out[7] = get_byte(7, in);
#endif
   }

/**
* Store a little-endian u64bit
* @param in the input u64bit
* @param out the byte array to write to
*/
inline void store_le(u64bit in, byte out[8])
   {
#if BOTAN_TARGET_UNALIGNED_MEMORY_ACCESS_OK
   *reinterpret_cast<u64bit*>(out) = BOTAN_ENDIAN_L2N(in);
#else
   out[0] = get_byte(7, in);
   out[1] = get_byte(6, in);
   out[2] = get_byte(5, in);
   out[3] = get_byte(4, in);
   out[4] = get_byte(3, in);
   out[5] = get_byte(2, in);
   out[6] = get_byte(1, in);
   out[7] = get_byte(0, in);
#endif
   }

/**
* Store two little-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
*/
template<typename T>
inline void store_le(byte out[], T x0, T x1)
   {
   store_le(x0, out + (0 * sizeof(T)));
   store_le(x1, out + (1 * sizeof(T)));
   }

/**
* Store two big-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
*/
template<typename T>
inline void store_be(byte out[], T x0, T x1)
   {
   store_be(x0, out + (0 * sizeof(T)));
   store_be(x1, out + (1 * sizeof(T)));
   }

/**
* Store four little-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
* @param x2 the third word
* @param x3 the fourth word
*/
template<typename T>
inline void store_le(byte out[], T x0, T x1, T x2, T x3)
   {
   store_le(x0, out + (0 * sizeof(T)));
   store_le(x1, out + (1 * sizeof(T)));
   store_le(x2, out + (2 * sizeof(T)));
   store_le(x3, out + (3 * sizeof(T)));
   }

/**
* Store four big-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
* @param x2 the third word
* @param x3 the fourth word
*/
template<typename T>
inline void store_be(byte out[], T x0, T x1, T x2, T x3)
   {
   store_be(x0, out + (0 * sizeof(T)));
   store_be(x1, out + (1 * sizeof(T)));
   store_be(x2, out + (2 * sizeof(T)));
   store_be(x3, out + (3 * sizeof(T)));
   }

/**
* Store eight little-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
* @param x2 the third word
* @param x3 the fourth word
* @param x4 the fifth word
* @param x5 the sixth word
* @param x6 the seventh word
* @param x7 the eighth word
*/
template<typename T>
inline void store_le(byte out[], T x0, T x1, T x2, T x3,
                                 T x4, T x5, T x6, T x7)
   {
   store_le(x0, out + (0 * sizeof(T)));
   store_le(x1, out + (1 * sizeof(T)));
   store_le(x2, out + (2 * sizeof(T)));
   store_le(x3, out + (3 * sizeof(T)));
   store_le(x4, out + (4 * sizeof(T)));
   store_le(x5, out + (5 * sizeof(T)));
   store_le(x6, out + (6 * sizeof(T)));
   store_le(x7, out + (7 * sizeof(T)));
   }

/**
* Store eight big-endian words
* @param out the output byte array
* @param x0 the first word
* @param x1 the second word
* @param x2 the third word
* @param x3 the fourth word
* @param x4 the fifth word
* @param x5 the sixth word
* @param x6 the seventh word
* @param x7 the eighth word
*/
template<typename T>
inline void store_be(byte out[], T x0, T x1, T x2, T x3,
                                 T x4, T x5, T x6, T x7)
   {
   store_be(x0, out + (0 * sizeof(T)));
   store_be(x1, out + (1 * sizeof(T)));
   store_be(x2, out + (2 * sizeof(T)));
   store_be(x3, out + (3 * sizeof(T)));
   store_be(x4, out + (4 * sizeof(T)));
   store_be(x5, out + (5 * sizeof(T)));
   store_be(x6, out + (6 * sizeof(T)));
   store_be(x7, out + (7 * sizeof(T)));
   }

}


namespace Botan {

/**
* CBC Encryption
*/
class BOTAN_DLL CBC_Encryption : public Keyed_Filter,
                                 private Buffered_Filter
   {
   public:
      std::string name() const;

      void set_iv(const InitializationVector& iv);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CBC_Encryption(BlockCipher* cipher,
                     BlockCipherModePaddingMethod* padding);

      CBC_Encryption(BlockCipher* cipher,
                     BlockCipherModePaddingMethod* padding,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~CBC_Encryption() { delete cipher; delete padder; }
   private:
      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      void write(const byte input[], size_t input_length);
      void end_msg();

      BlockCipher* cipher;
      const BlockCipherModePaddingMethod* padder;
      SecureVector<byte> state;
   };

/**
* CBC Decryption
*/
class BOTAN_DLL CBC_Decryption : public Keyed_Filter,
                                 private Buffered_Filter
   {
   public:
      std::string name() const;

      void set_iv(const InitializationVector& iv);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CBC_Decryption(BlockCipher* cipher,
                     BlockCipherModePaddingMethod* padding);

      CBC_Decryption(BlockCipher* cipher,
                     BlockCipherModePaddingMethod* padding,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~CBC_Decryption() { delete cipher; delete padder; }
   private:
      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      void write(const byte[], size_t);
      void end_msg();

      BlockCipher* cipher;
      const BlockCipherModePaddingMethod* padder;
      SecureVector<byte> state, temp;
   };

}


namespace Botan {

/**
* A class handling runtime CPU feature detection
*/
class BOTAN_DLL CPUID
   {
   public:
      /**
      * Probe the CPU and see what extensions are supported
      */
      static void initialize();

      /**
      * Return a best guess of the cache line size
      */
      static size_t cache_line_size() { return cache_line; }

      /**
      * Check if the processor supports RDTSC
      */
      static bool has_rdtsc()
         { return x86_processor_flags_has(CPUID_RDTSC_BIT); }

      /**
      * Check if the processor supports SSE2
      */
      static bool has_sse2()
         { return x86_processor_flags_has(CPUID_SSE2_BIT); }

      /**
      * Check if the processor supports SSSE3
      */
      static bool has_ssse3()
         { return x86_processor_flags_has(CPUID_SSSE3_BIT); }

      /**
      * Check if the processor supports SSE4.1
      */
      static bool has_sse41()
         { return x86_processor_flags_has(CPUID_SSE41_BIT); }

      /**
      * Check if the processor supports SSE4.2
      */
      static bool has_sse42()
         { return x86_processor_flags_has(CPUID_SSE42_BIT); }

      /**
      * Check if the processor supports extended AVX vector instructions
      */
      static bool has_avx()
         { return x86_processor_flags_has(CPUID_AVX_BIT); }

      /**
      * Check if the processor supports AES-NI
      */
      static bool has_aes_ni()
         { return x86_processor_flags_has(CPUID_AESNI_BIT); }

      /**
      * Check if the processor supports PCMULUDQ
      */
      static bool has_pcmuludq()
         { return x86_processor_flags_has(CPUID_PCMUL_BIT); }

      /**
      * Check if the processor supports MOVBE
      */
      static bool has_movbe()
         { return x86_processor_flags_has(CPUID_MOVBE_BIT); }

      /**
      * Check if the processor supports RDRAND
      */
      static bool has_rdrand()
         { return x86_processor_flags_has(CPUID_RDRAND_BIT); }

      /**
      * Check if the processor supports AltiVec/VMX
      */
      static bool has_altivec() { return altivec_capable; }
   private:
      enum CPUID_bits {
         CPUID_RDTSC_BIT = 4,
         CPUID_SSE2_BIT = 26,
         CPUID_PCMUL_BIT = 33,
         CPUID_SSSE3_BIT = 41,
         CPUID_SSE41_BIT = 51,
         CPUID_SSE42_BIT = 52,
         CPUID_MOVBE_BIT = 54,
         CPUID_AESNI_BIT = 57,
         CPUID_AVX_BIT = 60,
         CPUID_RDRAND_BIT = 62
      };

      static bool x86_processor_flags_has(u64bit bit)
         {
         return ((x86_processor_flags >> bit) & 1);
         }

      static u64bit x86_processor_flags;
      static size_t cache_line;
      static bool altivec_capable;
   };

}


namespace Botan {

/**
* Serpent implementation using SIMD
*/
class BOTAN_DLL Serpent_SIMD : public Serpent
   {
   public:
      size_t parallelism() const { return 4; }

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      BlockCipher* clone() const { return new Serpent_SIMD; }
   };

}


namespace Botan {

/**
* PK_Encryptor Filter
*/
class BOTAN_DLL PK_Encryptor_Filter : public Filter
   {
   public:
      void write(const byte[], size_t);
      void end_msg();
      PK_Encryptor_Filter(PK_Encryptor* c,
                          RandomNumberGenerator& rng_ref) :
         cipher(c), rng(rng_ref) {}
      ~PK_Encryptor_Filter() { delete cipher; }
   private:
      PK_Encryptor* cipher;
      RandomNumberGenerator& rng;
      SecureVector<byte> buffer;
   };

/**
* PK_Decryptor Filter
*/
class BOTAN_DLL PK_Decryptor_Filter : public Filter
   {
   public:
      void write(const byte[], size_t);
      void end_msg();
      PK_Decryptor_Filter(PK_Decryptor* c) : cipher(c) {}
      ~PK_Decryptor_Filter() { delete cipher; }
   private:
      PK_Decryptor* cipher;
      SecureVector<byte> buffer;
   };

/**
* PK_Signer Filter
*/
class BOTAN_DLL PK_Signer_Filter : public Filter
   {
   public:
      void write(const byte[], size_t);
      void end_msg();

      PK_Signer_Filter(PK_Signer* s,
                       RandomNumberGenerator& rng_ref) :
         signer(s), rng(rng_ref) {}

      ~PK_Signer_Filter() { delete signer; }
   private:
      PK_Signer* signer;
      RandomNumberGenerator& rng;
   };

/**
* PK_Verifier Filter
*/
class BOTAN_DLL PK_Verifier_Filter : public Filter
   {
   public:
      void write(const byte[], size_t);
      void end_msg();

      void set_signature(const byte[], size_t);
      void set_signature(const MemoryRegion<byte>&);

      PK_Verifier_Filter(PK_Verifier* v) : verifier(v) {}
      PK_Verifier_Filter(PK_Verifier*, const byte[], size_t);
      PK_Verifier_Filter(PK_Verifier*, const MemoryRegion<byte>&);
      ~PK_Verifier_Filter() { delete verifier; }
   private:
      PK_Verifier* verifier;
      SecureVector<byte> signature;
   };

}


namespace Botan {

/**
* XTEA implemented using SIMD operations
*/
class BOTAN_DLL XTEA_SIMD : public XTEA
   {
   public:
      size_t parallelism() const { return 8; }

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;
      BlockCipher* clone() const { return new XTEA_SIMD; }
   };

}


namespace Botan {

/**
* A queue that knows how to zeroize itself
*/
class BOTAN_DLL SecureQueue : public Fanout_Filter, public DataSource
   {
   public:
      std::string name() const { return "Queue"; }

      void write(const byte[], size_t);

      size_t read(byte[], size_t);
      size_t peek(byte[], size_t, size_t = 0) const;

      bool end_of_data() const;

      /**
      * @return number of bytes available in the queue
      */
      size_t size() const;

      bool attachable() { return false; }

      /**
      * SecureQueue assignment
      * @param other the queue to copy
      */
      SecureQueue& operator=(const SecureQueue& other);

      /**
      * SecureQueue default constructor (creates empty queue)
      */
      SecureQueue();

      /**
      * SecureQueue copy constructor
      * @param other the queue to copy
      */
      SecureQueue(const SecureQueue& other);

      ~SecureQueue() { destroy(); }
   private:
      void destroy();
      class SecureQueueNode* head;
      class SecureQueueNode* tail;
   };

}


namespace Botan {

/**
* HMAC
*/
class BOTAN_DLL HMAC : public MessageAuthenticationCode
   {
   public:
      void clear();
      std::string name() const;
      MessageAuthenticationCode* clone() const;

      size_t output_length() const { return hash->output_length(); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(0, 2*hash->hash_block_size());
         }

      /**
      * @param hash the hash to use for HMACing
      */
      HMAC(HashFunction* hash);
      ~HMAC() { delete hash; }
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      void key_schedule(const byte[], size_t);

      HashFunction* hash;
      SecureVector<byte> i_key, o_key;
   };

}


namespace Botan {

/**
* OpenPGP's S2K
*/
class BOTAN_DLL OpenPGP_S2K : public PBKDF
   {
   public:
      /**
      * @param hash_in the hash function to use
      */
      OpenPGP_S2K(HashFunction* hash_in) : hash(hash_in) {}

      ~OpenPGP_S2K() { delete hash; }

      std::string name() const
         {
         return "OpenPGP-S2K(" + hash->name() + ")";
         }

      PBKDF* clone() const
         {
         return new OpenPGP_S2K(hash->clone());
         }

      OctetString derive_key(size_t output_len,
                             const std::string& passphrase,
                             const byte salt[], size_t salt_len,
                             size_t iterations) const;
   private:
      HashFunction* hash;
   };

}


namespace Botan {

/**
* Lion is a block cipher construction designed by Ross Anderson and
* Eli Biham, described in "Two Practical and Provably Secure Block
* Ciphers: BEAR and LION". It has a variable block size and is
* designed to encrypt very large blocks (up to a megabyte)

* http://www.cl.cam.ac.uk/~rja14/Papers/bear-lion.pdf
*/
class BOTAN_DLL Lion : public BlockCipher
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      size_t block_size() const { return BLOCK_SIZE; }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(2, 2*hash->output_length(), 2);
         }

      void clear();
      std::string name() const;
      BlockCipher* clone() const;

      /**
      * @param hash the hash to use internally
      * @param cipher the stream cipher to use internally
      * @param block_size the size of the block to use
      */
      Lion(HashFunction* hash,
           StreamCipher* cipher,
           size_t block_size);

      ~Lion() { delete hash; delete cipher; }
   private:
      void key_schedule(const byte[], size_t);

      const size_t BLOCK_SIZE, LEFT_SIZE, RIGHT_SIZE;

      HashFunction* hash;
      StreamCipher* cipher;
      SecureVector<byte> key1, key2;
   };

}


namespace Botan {

/**
* Skipjack, a NSA designed cipher used in Fortezza
*/
class BOTAN_DLL Skipjack : public Block_Cipher_Fixed_Params<8, 10>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();
      std::string name() const { return "Skipjack"; }
      BlockCipher* clone() const { return new Skipjack; }

      Skipjack() : FTAB(2560) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<byte> FTAB;
   };

}


namespace Botan {

namespace FPE {

/**
* Encrypt X from and onto the group Z_n using key and tweak
* @param n the modulus
* @param X the plaintext as a BigInt
* @param key a random key
* @param tweak will modify the ciphertext (think of as an IV)
*/
BigInt BOTAN_DLL fe1_encrypt(const BigInt& n, const BigInt& X,
                             const SymmetricKey& key,
                             const MemoryRegion<byte>& tweak);

/**
* Decrypt X from and onto the group Z_n using key and tweak
* @param n the modulus
* @param X the ciphertext as a BigInt
* @param key is the key used for encryption
* @param tweak the same tweak used for encryption
*/
BigInt BOTAN_DLL fe1_decrypt(const BigInt& n, const BigInt& X,
                             const SymmetricKey& key,
                             const MemoryRegion<byte>& tweak);

}

}


namespace Botan {

/**
* This class represents ECDH Public Keys.
*/
class BOTAN_DLL ECDH_PublicKey : public virtual EC_PublicKey
   {
   public:

      ECDH_PublicKey(const AlgorithmIdentifier& alg_id,
                     const MemoryRegion<byte>& key_bits) :
         EC_PublicKey(alg_id, key_bits) {}

      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      ECDH_PublicKey(const EC_Group& dom_par,
                     const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      /**
      * Get this keys algorithm name.
      * @return this keys algorithm name
      */
      std::string algo_name() const { return "ECDH"; }

      /**
      * Get the maximum number of bits allowed to be fed to this key.
      * This is the bitlength of the order of the base point.

      * @return maximum number of input bits
      */
      size_t max_input_bits() const { return domain().get_order().bits(); }

      /**
      * @return public point value
      */
      MemoryVector<byte> public_value() const
         { return EC2OSP(public_point(), PointGFp::UNCOMPRESSED); }

   protected:
      ECDH_PublicKey() {}
   };

/**
* This class represents ECDH Private Keys.
*/
class BOTAN_DLL ECDH_PrivateKey : public ECDH_PublicKey,
                                  public EC_PrivateKey,
                                  public PK_Key_Agreement_Key
   {
   public:

      ECDH_PrivateKey(const AlgorithmIdentifier& alg_id,
                      const MemoryRegion<byte>& key_bits) :
         EC_PrivateKey(alg_id, key_bits) {}

      /**
      * Generate a new private key
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key; if zero, a new random key is generated
      */
      ECDH_PrivateKey(RandomNumberGenerator& rng,
                      const EC_Group& domain,
                      const BigInt& x = 0) :
         EC_PrivateKey(rng, domain, x) {}

      MemoryVector<byte> public_value() const
         { return ECDH_PublicKey::public_value(); }
   };

/**
* ECDH operation
*/
class BOTAN_DLL ECDH_KA_Operation : public PK_Ops::Key_Agreement
   {
   public:
      ECDH_KA_Operation(const ECDH_PrivateKey& key);

      SecureVector<byte> agree(const byte w[], size_t w_len);
   private:
      const CurveGFp& curve;
      const BigInt& cofactor;
      BigInt l_times_priv;
   };

}


namespace Botan {

/**
* AES-128
*/
class BOTAN_DLL AES_128 : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      AES_128() : EK(40), DK(40), ME(16), MD(16) {}

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();

      std::string name() const { return "AES-128"; }
      BlockCipher* clone() const { return new AES_128; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u32bit> EK, DK;
      SecureVector<byte> ME, MD;
   };

/**
* AES-192
*/
class BOTAN_DLL AES_192 : public Block_Cipher_Fixed_Params<16, 24>
   {
   public:
      AES_192() : EK(48), DK(48), ME(16), MD(16) {}

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();

      std::string name() const { return "AES-192"; }
      BlockCipher* clone() const { return new AES_192; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u32bit> EK, DK;
      SecureVector<byte> ME, MD;
   };

/**
* AES-256
*/
class BOTAN_DLL AES_256 : public Block_Cipher_Fixed_Params<16, 32>
   {
   public:
      AES_256() : EK(56), DK(56), ME(16), MD(16) {}

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear();

      std::string name() const { return "AES-256"; }
      BlockCipher* clone() const { return new AES_256; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u32bit> EK, DK;
      SecureVector<byte> ME, MD;
   };

}


namespace Botan {

/**
* Rivest's Package Tranform
* @param rng the random number generator to use
* @param cipher the block cipher to use
* @param input the input data buffer
* @param input_len the length of the input data in bytes
* @param output the output data buffer (must be at least
*        input_len + cipher->BLOCK_SIZE bytes long)
*/
void BOTAN_DLL aont_package(RandomNumberGenerator& rng,
                            BlockCipher* cipher,
                            const byte input[], size_t input_len,
                            byte output[]);

/**
* Rivest's Package Tranform (Inversion)
* @param cipher the block cipher to use
* @param input the input data buffer
* @param input_len the length of the input data in bytes
* @param output the output data buffer (must be at least
*        input_len - cipher->BLOCK_SIZE bytes long)
*/
void BOTAN_DLL aont_unpackage(BlockCipher* cipher,
                              const byte input[], size_t input_len,
                              byte output[]);

}


namespace Botan {

/**
* Combines two hash functions using a Feistel scheme. Described in
* "On the Security of Hash Function Combiners", Anja Lehmann
*/
class BOTAN_DLL Comb4P : public HashFunction
   {
   public:
      /**
      * @param h1 the first hash
      * @param h2 the second hash
      */
      Comb4P(HashFunction* h1, HashFunction* h2);

      ~Comb4P() { delete hash1; delete hash2; }

      size_t hash_block_size() const;

      size_t output_length() const
         {
         return hash1->output_length() + hash2->output_length();
         }

      HashFunction* clone() const
         {
         return new Comb4P(hash1->clone(), hash2->clone());
         }

      std::string name() const
         {
         return "Comb4P(" + hash1->name() + "," + hash2->name() + ")";
         }

      void clear();
   private:
      void add_data(const byte input[], size_t length);
      void final_result(byte out[]);

      HashFunction* hash1;
      HashFunction* hash2;
   };

}


namespace Botan {

/**
* A split secret, using the format from draft-mcgrew-tss-03
*/
class BOTAN_DLL RTSS_Share
   {
   public:
      /**
      * @param M the number of shares needed to reconstruct
      * @param N the number of shares generated
      * @param secret the secret to split
      * @param secret_len the length of the secret
      * @param identifier the 16 byte share identifier
      * @param rng the random number generator to use
      */
      static std::vector<RTSS_Share>
         split(byte M, byte N,
               const byte secret[], u16bit secret_len,
               const byte identifier[16],
               RandomNumberGenerator& rng);

      /**
      * @param shares the list of shares
      */
      static SecureVector<byte>
        reconstruct(const std::vector<RTSS_Share>& shares);

      RTSS_Share() {}

      /**
      * @param hex_input the share encoded in hexadecimal
      */
      RTSS_Share(const std::string& hex_input);

      /**
      * @return hex representation
      */
      std::string to_string() const;

      /**
      * @return share identifier
      */
      byte share_id() const;

      /**
      * @return size of this share in bytes
      */
      size_t size() const { return contents.size(); }

      /**
      * @return if this TSS share was initialized or not
      */
      bool initialized() const { return (contents.size() > 0); }
   private:
      SecureVector<byte> contents;
   };

}


namespace Botan {

/**
* RC2
*/
class BOTAN_DLL RC2 : public Block_Cipher_Fixed_Params<8, 1, 32>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      /**
      * Return the code of the effective key bits
      * @param bits key length
      * @return EKB code
      */
      static byte EKB_code(size_t bits);

      void clear() { zeroise(K); }
      std::string name() const { return "RC2"; }
      BlockCipher* clone() const { return new RC2; }

      RC2() : K(64) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u16bit> K;
   };

}


namespace Botan {

/**
* DJB's Salsa20 (and XSalsa20)
*/
class BOTAN_DLL Salsa20 : public StreamCipher
   {
   public:
      void cipher(const byte in[], byte out[], size_t length);

      void set_iv(const byte iv[], size_t iv_len);

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == 8 || iv_len == 24); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(16, 32, 16);
         }

      void clear();
      std::string name() const;
      StreamCipher* clone() const { return new Salsa20; }

      Salsa20() : state(16), buffer(64), position(0) {}
   private:
      void key_schedule(const byte key[], size_t key_len);

      SecureVector<u32bit> state;
      SecureVector<byte> buffer;
      size_t position;
   };

}


namespace Botan {

/**
* RIPEMD-128
*/
class BOTAN_DLL RIPEMD_128 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "RIPEMD-128"; }
      size_t output_length() const { return 16; }
      HashFunction* clone() const { return new RIPEMD_128; }

      void clear();

      RIPEMD_128() : MDx_HashFunction(64, false, true), M(16), digest(4)
         { clear(); }
   private:
      void compress_n(const byte[], size_t blocks);
      void copy_out(byte[]);

      SecureVector<u32bit> M, digest;
   };

}


namespace Botan {

/**
* 24-bit cyclic redundancy check
*/
class BOTAN_DLL CRC24 : public HashFunction
   {
   public:
      std::string name() const { return "CRC24"; }
      size_t output_length() const { return 3; }
      HashFunction* clone() const { return new CRC24; }

      void clear() { crc = 0xB704CE; }

      CRC24() { clear(); }
      ~CRC24() { clear(); }
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      u32bit crc;
   };

}


namespace Botan {

/**
* PKCS #5 v1 PBKDF, aka PBKDF1
* Can only generate a key up to the size of the hash output.
* Unless needed for backwards compatability, use PKCS5_PBKDF2
*/
class BOTAN_DLL PKCS5_PBKDF1 : public PBKDF
   {
   public:
      /**
      * Create a PKCS #5 instance using the specified hash function.
      * @param hash_in pointer to a hash function object to use
      */
      PKCS5_PBKDF1(HashFunction* hash_in) : hash(hash_in) {}

      /**
      * Copy constructor
      * @param other the object to copy
      */
      PKCS5_PBKDF1(const PKCS5_PBKDF1& other) :
         PBKDF(), hash(other.hash->clone()) {}

      ~PKCS5_PBKDF1() { delete hash; }

      std::string name() const
         {
         return "PBKDF1(" + hash->name() + ")";
         }

      PBKDF* clone() const
         {
         return new PKCS5_PBKDF1(hash->clone());
         }

      OctetString derive_key(size_t output_len,
                             const std::string& passphrase,
                             const byte salt[], size_t salt_len,
                             size_t iterations) const;
   private:
      HashFunction* hash;
   };

}


namespace Botan {

/**
* This class represents X.509 Certificate Authorities (CAs).
*/
class BOTAN_DLL X509_CA
   {
   public:

      /**
      * Sign a PKCS#10 Request.
      * @param req the request to sign
      * @param rng the rng to use
      * @param not_before the starting time for the certificate
      * @param not_after the expiration time for the certificate
      * @return resulting certificate
      */
      X509_Certificate sign_request(const PKCS10_Request& req,
                                    RandomNumberGenerator& rng,
                                    const X509_Time& not_before,
                                    const X509_Time& not_after);

      /**
      * Get the certificate of this CA.
      * @return CA certificate
      */
      X509_Certificate ca_certificate() const;

      /**
      * Create a new and empty CRL for this CA.
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      * @return new CRL
      */
      X509_CRL new_crl(RandomNumberGenerator& rng,
                       u32bit next_update = 0) const;

      /**
      * Create a new CRL by with additional entries.
      * @param last_crl the last CRL of this CA to add the new entries to
      * @param new_entries contains the new CRL entries to be added to the CRL
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      */
      X509_CRL update_crl(const X509_CRL& last_crl,
                          const std::vector<CRL_Entry>& new_entries,
                          RandomNumberGenerator& rng,
                          u32bit next_update = 0) const;

      /**
      * Interface for creating new certificates
      * @param signer a signing object
      * @param rng a random number generator
      * @param sig_algo the signature algorithm identifier
      * @param pub_key the serialized public key
      * @param not_before the start time of the certificate
      * @param not_after the end time of the certificate
      * @param issuer_dn the DN of the issuer
      * @param subject_dn the DN of the subject
      * @param extensions an optional list of certificate extensions
      * @returns newly minted certificate
      */
      static X509_Certificate make_cert(PK_Signer* signer,
                                        RandomNumberGenerator& rng,
                                        const AlgorithmIdentifier& sig_algo,
                                        const MemoryRegion<byte>& pub_key,
                                        const X509_Time& not_before,
                                        const X509_Time& not_after,
                                        const X509_DN& issuer_dn,
                                        const X509_DN& subject_dn,
                                        const Extensions& extensions);

      /**
      * Create a new CA object.
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param hash_fn name of a hash function to use for signing
      */
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              const std::string& hash_fn);

      ~X509_CA();
   private:
      X509_CA(const X509_CA&) {}
      X509_CA& operator=(const X509_CA&) { return (*this); }

      X509_CRL make_crl(const std::vector<CRL_Entry>& entries,
                        u32bit crl_number, u32bit next_update,
                        RandomNumberGenerator& rng) const;

      AlgorithmIdentifier ca_sig_algo;
      X509_Certificate cert;
      PK_Signer* signer;
   };

/**
* Choose the default signature format for a certain public key signature
* scheme.
* @param key will be the key to choose a padding scheme for
* @param hash_fn is the desired hash function
* @param alg_id will be set to the chosen scheme
* @return A PK_Signer object for generating signatures
*/
BOTAN_DLL PK_Signer* choose_sig_format(const Private_Key& key,
                                       const std::string& hash_fn,
                                       AlgorithmIdentifier& alg_id);

}


namespace Botan {

namespace OIDS {

/**
* Register an OID to string mapping.
* @param oid the oid to register
* @param name the name to be associated with the oid
*/
BOTAN_DLL void add_oid(const OID& oid, const std::string& name);

/**
* See if an OID exists in the internal table.
* @param oid the oid to check for
* @return true if the oid is registered
*/
BOTAN_DLL bool have_oid(const std::string& oid);

/**
* Resolve an OID
* @param oid the OID to look up
* @return name associated with this OID
*/
BOTAN_DLL std::string lookup(const OID& oid);

/**
* Find the OID to a name. The lookup will be performed in the
* general OID section of the configuration.
* @param name the name to resolve
* @return OID associated with the specified name
*/
BOTAN_DLL OID lookup(const std::string& name);

/**
* Tests whether the specified OID stands for the specified name.
* @param oid the OID to check
* @param name the name to check
* @return true if the specified OID stands for the specified name
*/
BOTAN_DLL bool name_of(const OID& oid, const std::string& name);

}

}


namespace Botan {

/**
* EAX Base Class
*/
class BOTAN_DLL EAX_Base : public Keyed_Filter
   {
   public:
      void set_key(const SymmetricKey& key);
      void set_iv(const InitializationVector& iv);

      /**
      * Set some additional data that is not included in the
      * ciphertext but that will be authenticated.
      * @param header the header contents
      * @param header_len length of header in bytes
      */
      void set_header(const byte header[], size_t header_len);

      /**
      * @return name of this mode
      */
      std::string name() const;

      bool valid_keylength(size_t key_len) const;

      /**
      * EAX supports arbitrary IV lengths
      */
      bool valid_iv_length(size_t) const { return true; }

      ~EAX_Base() { delete ctr; delete cmac; }
   protected:
      /**
      * @param cipher the cipher to use
      * @param tag_size is how big the auth tag will be
      */
      EAX_Base(BlockCipher* cipher, size_t tag_size);
      void start_msg();

      /**
      * The block size of the underlying cipher
      */
      const size_t BLOCK_SIZE;

      /**
      * The requested tag name
      */
      const size_t TAG_SIZE;

      /**
      * The name of the cipher
      */
      std::string cipher_name;

      /**
      * The stream cipher (CTR mode)
      */
      StreamCipher* ctr;

      /**
      * The MAC (CMAC)
      */
      MessageAuthenticationCode* cmac;

      /**
      * The MAC of the nonce
      */
      SecureVector<byte> nonce_mac;

      /**
      * The MAC of the header
      */
      SecureVector<byte> header_mac;

      /**
      * A buffer for CTR mode encryption
      */
      SecureVector<byte> ctr_buf;
   };

/**
* EAX Encryption
*/
class BOTAN_DLL EAX_Encryption : public EAX_Base
   {
   public:
      /**
      * @param ciph the cipher to use
      * @param tag_size is how big the auth tag will be
      */
      EAX_Encryption(BlockCipher* ciph, size_t tag_size = 0) :
         EAX_Base(ciph, tag_size) {}

      /**
      * @param ciph the cipher to use
      * @param key the key to use
      * @param iv the initially set IV
      * @param tag_size is how big the auth tag will be
      */
      EAX_Encryption(BlockCipher* ciph, const SymmetricKey& key,
                     const InitializationVector& iv,
                     size_t tag_size) : EAX_Base(ciph, tag_size)
         {
         set_key(key);
         set_iv(iv);
         }
   private:
      void write(const byte[], size_t);
      void end_msg();
   };

/**
* EAX Decryption
*/
class BOTAN_DLL EAX_Decryption : public EAX_Base
   {
   public:
      /**
      * @param ciph the cipher to use
      * @param tag_size is how big the auth tag will be
      */
      EAX_Decryption(BlockCipher* ciph, size_t tag_size = 0);

      /**
      * @param ciph the cipher to use
      * @param key the key to use
      * @param iv the initially set IV
      * @param tag_size is how big the auth tag will be
      */
      EAX_Decryption(BlockCipher* ciph, const SymmetricKey& key,
                     const InitializationVector& iv,
                     size_t tag_size = 0);
   private:
      void write(const byte[], size_t);
      void do_write(const byte[], size_t);
      void end_msg();

      SecureVector<byte> queue;
      size_t queue_start, queue_end;
   };

}


namespace Botan {

/**
* CMAC, also known as OMAC1
*/
class BOTAN_DLL CMAC : public MessageAuthenticationCode
   {
   public:
      std::string name() const;
      size_t output_length() const { return e->block_size(); }
      MessageAuthenticationCode* clone() const;

      void clear();

      Key_Length_Specification key_spec() const
         {
         return e->key_spec();
         }

      /**
      * CMAC's polynomial doubling operation
      * @param in the input
      * @param polynomial the byte value of the polynomial
      */
      static SecureVector<byte> poly_double(const MemoryRegion<byte>& in,
                                            byte polynomial);

      /**
      * @param cipher the underlying block cipher to use
      */
      CMAC(BlockCipher* cipher);
      ~CMAC();
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      void key_schedule(const byte[], size_t);

      BlockCipher* e;
      SecureVector<byte> buffer, state, B, P;
      size_t position;
      byte polynomial;
   };

}


namespace Botan {

/**
* Skein-512, a SHA-3 candidate
*/
class BOTAN_DLL Skein_512 : public HashFunction
   {
   public:
      /**
      * @param output_bits the output size of Skein in bits
      * @param personalization is a string that will paramaterize the
      * hash output
      */
      Skein_512(size_t output_bits = 512,
                const std::string& personalization = "");

      size_t hash_block_size() const { return 64; }
      size_t output_length() const { return output_bits / 8; }

      HashFunction* clone() const;
      std::string name() const;
      void clear();
   private:
      void add_data(const byte input[], size_t length);
      void final_result(byte out[]);

      std::string personalization;
      size_t output_bits;

      SecureVector<u64bit> H;
      SecureVector<u64bit> T;
      SecureVector<byte> buffer;
      size_t buf_pos;
   };

}


namespace Botan {

/**
* CBC encryption with ciphertext stealing
*/
class BOTAN_DLL CTS_Encryption : public Keyed_Filter
   {
   public:
      std::string name() const { return cipher->name() + "/CTS"; }

      void set_iv(const InitializationVector&);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CTS_Encryption(BlockCipher* cipher);

      CTS_Encryption(BlockCipher* cipher,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~CTS_Encryption() { delete cipher; }
   private:
      void write(const byte[], size_t);
      void end_msg();
      void encrypt(const byte[]);

      BlockCipher* cipher;
      SecureVector<byte> buffer, state;
      size_t position;
   };

/**
* CBC decryption with ciphertext stealing
*/
class BOTAN_DLL CTS_Decryption : public Keyed_Filter
   {
   public:
      std::string name() const { return cipher->name() + "/CTS"; }

      void set_iv(const InitializationVector&);

      void set_key(const SymmetricKey& key) { cipher->set_key(key); }

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      CTS_Decryption(BlockCipher* cipher);

      CTS_Decryption(BlockCipher* cipher,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~CTS_Decryption() { delete cipher; }
   private:
      void write(const byte[], size_t);
      void end_msg();
      void decrypt(const byte[]);

      BlockCipher* cipher;
      SecureVector<byte> buffer, state, temp;
      size_t position;
   };

}


namespace Botan {

/**
* Factory function for PBEs.
* @param algo_spec the name of the PBE algorithm to retrieve
* @return pointer to a PBE with randomly created parameters
*/
BOTAN_DLL PBE* get_pbe(const std::string& algo_spec);

/**
* Factory function for PBEs.
* @param pbe_oid the oid of the desired PBE
* @param params a DataSource providing the DER encoded parameters to use
* @return pointer to the PBE with the specified parameters
*/
BOTAN_DLL PBE* get_pbe(const OID& pbe_oid,
                       DataSource& params);

}


namespace Botan {

/**
* Public key encryptor factory method.
* @deprecated Instantiate object from pubkey.h directly
*
* @param key the key that will work inside the encryptor
* @param eme determines the algorithm and encoding
* @return public key encryptor object
*/
BOTAN_DEPRECATED("Instantiate object directly")
inline PK_Encryptor* get_pk_encryptor(const Public_Key& key,
                                      const std::string& eme)
   {
   return new PK_Encryptor_EME(key, eme);
   }

/**
* Public key decryptor factory method.
* @deprecated Instantiate object from pubkey.h directly
*
* @param key the key that will work inside the decryptor
* @param eme determines the algorithm and encoding
* @return public key decryptor object
*/
BOTAN_DEPRECATED("Instantiate object directly")
inline PK_Decryptor* get_pk_decryptor(const Private_Key& key,
                                      const std::string& eme)
   {
   return new PK_Decryptor_EME(key, eme);
   }

/**
* Public key signer factory method.
* @deprecated Instantiate object from pubkey.h directly
*
* @param key the key that will work inside the signer
* @param emsa determines the algorithm, encoding and hash algorithm
* @param sig_format the signature format to be used
* @return public key signer object
*/
BOTAN_DEPRECATED("Instantiate object directly")
inline PK_Signer* get_pk_signer(const Private_Key& key,
                                const std::string& emsa,
                                Signature_Format sig_format = IEEE_1363)
   {
   return new PK_Signer(key, emsa, sig_format);
   }

/**
* Public key verifier factory method.
* @deprecated Instantiate object from pubkey.h directly
*
* @param key the key that will work inside the verifier
* @param emsa determines the algorithm, encoding and hash algorithm
* @param sig_format the signature format to be used
* @return public key verifier object
*/
BOTAN_DEPRECATED("Instantiate object directly")
inline PK_Verifier* get_pk_verifier(const Public_Key& key,
                                    const std::string& emsa,
                                    Signature_Format sig_format = IEEE_1363)
   {
   return new PK_Verifier(key, emsa, sig_format);
   }

/**
* Public key key agreement factory method.
* @deprecated Instantiate object from pubkey.h directly
*
* @param key the key that will work inside the key agreement
* @param kdf the kdf algorithm to use
* @return key agreement algorithm
*/
BOTAN_DEPRECATED("Instantiate object directly")
inline PK_Key_Agreement* get_pk_kas(const PK_Key_Agreement_Key& key,
                                    const std::string& kdf)
   {
   return new PK_Key_Agreement(key, kdf);
   }

}


namespace Botan {

/**
* IDEA
*/
class BOTAN_DLL IDEA : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); zeroise(DK); }
      std::string name() const { return "IDEA"; }
      BlockCipher* clone() const { return new IDEA; }

      IDEA() : EK(52), DK(52) {}
   protected:
      /**
      * @return const reference to encryption subkeys
      */
      const SecureVector<u16bit>& get_EK() const { return EK; }

      /**
      * @return const reference to decryption subkeys
      */
      const SecureVector<u16bit>& get_DK() const { return DK; }

   private:
      void key_schedule(const byte[], size_t);
      SecureVector<u16bit> EK, DK;
   };

}


namespace Botan {

/**
* Return the PKCS #1 hash identifier
* @see RFC 3447 section 9.2
* @param hash_name the name of the hash function
* @return byte sequence identifying the hash
* @throw Invalid_Argument if the hash has no known PKCS #1 hash id
*/
BOTAN_DLL MemoryVector<byte> pkcs_hash_id(const std::string& hash_name);

/**
* Return the IEEE 1363 hash identifier
* @param hash_name the name of the hash function
* @return byte code identifying the hash, or 0 if not known
*/
BOTAN_DLL byte ieee1363_hash_id(const std::string& hash_name);

}


namespace Botan {

/**
* MGF1 from PKCS #1 v2.0
*/
class BOTAN_DLL MGF1 : public MGF
   {
   public:
      void mask(const byte[], size_t, byte[], size_t) const;

      /**
      MGF1 constructor: takes ownership of hash
      */
      MGF1(HashFunction* hash);

      ~MGF1();
   private:
      HashFunction* hash;
   };

}


namespace Botan {

/**
* IEEE P1619 XTS Encryption
*/
class BOTAN_DLL XTS_Encryption : public Keyed_Filter,
                                 private Buffered_Filter
   {
   public:
      void set_key(const SymmetricKey& key);
      void set_iv(const InitializationVector& iv);

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      std::string name() const;

      XTS_Encryption(BlockCipher* ciph);

      XTS_Encryption(BlockCipher* ciph,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~XTS_Encryption() { delete cipher; delete cipher2; }
   private:
      void write(const byte[], size_t);
      void end_msg();

      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      BlockCipher* cipher;
      BlockCipher* cipher2;
      SecureVector<byte> tweak;
   };

/**
* IEEE P1619 XTS Encryption
*/
class BOTAN_DLL XTS_Decryption : public Keyed_Filter,
                                 private Buffered_Filter
   {
   public:
      void set_key(const SymmetricKey& key);
      void set_iv(const InitializationVector& iv);

      bool valid_keylength(size_t key_len) const
         { return cipher->valid_keylength(key_len); }

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len == cipher->block_size()); }

      std::string name() const;

      XTS_Decryption(BlockCipher* ciph);

      XTS_Decryption(BlockCipher* ciph,
                     const SymmetricKey& key,
                     const InitializationVector& iv);

      ~XTS_Decryption() { delete cipher; delete cipher2; }
   private:
      void write(const byte[], size_t);
      void end_msg();

      void buffered_block(const byte input[], size_t input_length);
      void buffered_final(const byte input[], size_t input_length);

      BlockCipher* cipher;
      BlockCipher* cipher2;
      SecureVector<byte> tweak;
   };

}


namespace Botan {

/**
* Struct representing a particular date and time
*/
struct BOTAN_DLL calendar_point
   {
   /** The year */
   u32bit year;

   /** The month, 1 through 12 for Jan to Dec */
   byte month;

   /** The day of the month, 1 through 31 (or 28 or 30 based on month */
   byte day;

   /** Hour in 24-hour form, 0 to 23 */
   byte hour;

   /** Minutes in the hour, 0 to 60 */
   byte minutes;

   /** Seconds in the minute, 0 to 60, but might be slightly
       larger to deal with leap seconds on some systems
   */
   byte seconds;

   /**
   * Initialize a calendar_point
   * @param y the year
   * @param mon the month
   * @param d the day
   * @param h the hour
   * @param min the minute
   * @param sec the second
   */
   calendar_point(u32bit y, byte mon, byte d, byte h, byte min, byte sec) :
      year(y), month(mon), day(d), hour(h), minutes(min), seconds(sec) {}
   };

/**
* @param time_point a time point from the system clock
* @return calendar_point object representing this time point
*/
BOTAN_DLL calendar_point calendar_value(u64bit time_point);

/**
* @return seconds resolution timestamp, unknown epoch
*/
BOTAN_DLL u64bit system_time();

/**
* @return nanoseconds resolution timestamp, unknown epoch
*/
BOTAN_DLL u64bit get_nanoseconds_clock();

}


namespace Botan {

/**
* EMSA-Raw - sign inputs directly
* Don't use this unless you know what you are doing.
*/
class BOTAN_DLL EMSA_Raw : public EMSA
   {
   private:
      void update(const byte[], size_t);
      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator&);
      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);

      SecureVector<byte> message;
   };

}


namespace Botan {

/**
* Perform base64 encoding
* @param output an array of at least input_length*4/3 bytes
* @param input is some binary data
* @param input_length length of input in bytes
* @param input_consumed is an output parameter which says how many
*        bytes of input were actually consumed. If less than
*        input_length, then the range input[consumed:length]
*        should be passed in later along with more input.
* @param final_inputs true iff this is the last input, in which case
         padding chars will be applied if needed
* @return number of bytes written to output
*/
size_t BOTAN_DLL base64_encode(char output[],
                               const byte input[],
                               size_t input_length,
                               size_t& input_consumed,
                               bool final_inputs);

/**
* Perform base64 encoding
* @param input some input
* @param input_length length of input in bytes
* @return base64adecimal representation of input
*/
std::string BOTAN_DLL base64_encode(const byte input[],
                                    size_t input_length);

/**
* Perform base64 encoding
* @param input some input
* @return base64adecimal representation of input
*/
std::string BOTAN_DLL base64_encode(const MemoryRegion<byte>& input);

/**
* Perform base64 decoding
* @param output an array of at least input_length*3/4 bytes
* @param input some base64 input
* @param input_length length of input in bytes
* @param input_consumed is an output parameter which says how many
*        bytes of input were actually consumed. If less than
*        input_length, then the range input[consumed:length]
*        should be passed in later along with more input.
* @param final_inputs true iff this is the last input, in which case
         padding is allowed
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL base64_decode(byte output[],
                               const char input[],
                               size_t input_length,
                               size_t& input_consumed,
                               bool final_inputs,
                               bool ignore_ws = true);

/**
* Perform base64 decoding
* @param output an array of at least input_length*3/4 bytes
* @param input some base64 input
* @param input_length length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL base64_decode(byte output[],
                               const char input[],
                               size_t input_length,
                               bool ignore_ws = true);

/**
* Perform base64 decoding
* @param output an array of at least input_length/3*4 bytes
* @param input some base64 input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_DLL base64_decode(byte output[],
                               const std::string& input,
                               bool ignore_ws = true);

/**
* Perform base64 decoding
* @param input some base64 input
* @param input_length the length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded base64 output
*/
SecureVector<byte> BOTAN_DLL base64_decode(const char input[],
                                           size_t input_length,
                                           bool ignore_ws = true);

/**
* Perform base64 decoding
* @param input some base64 input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded base64 output
*/
SecureVector<byte> BOTAN_DLL base64_decode(const std::string& input,
                                           bool ignore_ws = true);

}


namespace Botan {

/**
* PRF used in TLS 1.0/1.1
*/
class BOTAN_DLL TLS_PRF : public KDF
   {
   public:
      SecureVector<byte> derive(size_t key_len,
                                const byte secret[], size_t secret_len,
                                const byte seed[], size_t seed_len) const;

      std::string name() const { return "TLS-PRF"; }
      KDF* clone() const { return new TLS_PRF; }

      TLS_PRF();
      ~TLS_PRF();
   private:
      MessageAuthenticationCode* hmac_md5;
      MessageAuthenticationCode* hmac_sha1;
   };

/**
* PRF used in TLS 1.2
*/
class BOTAN_DLL TLS_12_PRF : public KDF
   {
   public:
      SecureVector<byte> derive(size_t key_len,
                                const byte secret[], size_t secret_len,
                                const byte seed[], size_t seed_len) const;

      std::string name() const { return "TLSv12-PRF(" + hmac->name() + ")"; }
      KDF* clone() const { return new TLS_12_PRF(hmac->clone()); }

      TLS_12_PRF(MessageAuthenticationCode* hmac);
      ~TLS_12_PRF();
   private:
      MessageAuthenticationCode* hmac;
   };

}


namespace Botan {

/**
* EMSA4 aka PSS-R
*/
class BOTAN_DLL EMSA4 : public EMSA
   {
   public:
      /**
      * @param hash the hash object to use
      */
      EMSA4(HashFunction* hash);

      /**
      * @param hash the hash object to use
      * @param salt_size the size of the salt to use in bytes
      */
      EMSA4(HashFunction* hash, size_t salt_size);

      ~EMSA4() { delete hash; delete mgf; }
   private:
      void update(const byte[], size_t);
      SecureVector<byte> raw_data();

      SecureVector<byte> encoding_of(const MemoryRegion<byte>&, size_t,
                                     RandomNumberGenerator& rng);
      bool verify(const MemoryRegion<byte>&, const MemoryRegion<byte>&,
                  size_t);

      size_t SALT_SIZE;
      HashFunction* hash;
      const MGF* mgf;
   };

}


namespace Botan {

/**
* Camellia-128
*/
class BOTAN_DLL Camellia_128 : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { SK.clear(); }
      std::string name() const { return "Camellia-128"; }
      BlockCipher* clone() const { return new Camellia_128; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u64bit> SK;
   };

/**
* Camellia-192
*/
class BOTAN_DLL Camellia_192 : public Block_Cipher_Fixed_Params<16, 24>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { SK.clear(); }
      std::string name() const { return "Camellia-192"; }
      BlockCipher* clone() const { return new Camellia_192; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u64bit> SK;
   };

/**
* Camellia-256
*/
class BOTAN_DLL Camellia_256 : public Block_Cipher_Fixed_Params<16, 32>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { SK.clear(); }
      std::string name() const { return "Camellia-256"; }
      BlockCipher* clone() const { return new Camellia_256; }
   private:
      void key_schedule(const byte key[], size_t length);

      SecureVector<u64bit> SK;
   };

}


namespace Botan {

/**
* MD4
*/
class BOTAN_DLL MD4 : public MDx_HashFunction
   {
   public:
      std::string name() const { return "MD4"; }
      size_t output_length() const { return 16; }
      HashFunction* clone() const { return new MD4; }

      void clear();

      MD4() : MDx_HashFunction(64, false, true), M(16), digest(4)
         { clear(); }
   protected:
      void compress_n(const byte input[], size_t blocks);
      void copy_out(byte[]);

      /**
      * The message buffer, exposed for use by subclasses (x86 asm)
      */
      SecureVector<u32bit> M;

      /**
      * The digest value, exposed for use by subclasses (x86 asm)
      */
      SecureVector<u32bit> digest;
   };

}


namespace Botan {

/**
* Turing
*/
class BOTAN_DLL Turing : public StreamCipher
   {
   public:
      void cipher(const byte in[], byte out[], size_t length);
      void set_iv(const byte iv[], size_t iv_length);

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len % 4 == 0 && iv_len <= 16); }

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(4, 32, 4);
         }

      void clear();
      std::string name() const { return "Turing"; }
      StreamCipher* clone() const { return new Turing; }

      Turing() : S0(256), S1(256), S2(256), S3(256),
                 R(17), buffer(340), position(0) {}

   private:
      void key_schedule(const byte[], size_t);
      void generate();

      static u32bit fixedS(u32bit);

      static const u32bit Q_BOX[256];
      static const byte SBOX[256];

      SecureVector<u32bit> S0, S1, S2, S3;
      SecureVector<u32bit> R;
      SecureVector<u32bit> K;
      SecureVector<byte> buffer;
      size_t position;
   };

}


namespace Botan {

/**
* SRP6a Client side
* @param username the username we are attempting login for
* @param password the password we are attempting to use
* @param group_id specifies the shared SRP group
* @param hash_id specifies a secure hash function
* @param salt is the salt value sent by the server
* @param B is the server's public value
* @param rng is a random number generator
*
* @return (A,K) the client public key and the shared secret key
*/
std::pair<BigInt,SymmetricKey>
BOTAN_DLL srp6_client_agree(const std::string& username,
                            const std::string& password,
                            const std::string& group_id,
                            const std::string& hash_id,
                            const MemoryRegion<byte>& salt,
                            const BigInt& B,
                            RandomNumberGenerator& rng);

/**
* Generate a new SRP-6 verifier
* @param identifier a username or other client identifier
* @param password the secret used to authenticate user
* @param salt a randomly chosen value, at least 128 bits long
*/
BigInt BOTAN_DLL generate_srp6_verifier(const std::string& identifier,
                                        const std::string& password,
                                        const MemoryRegion<byte>& salt,
                                        const std::string& group_id,
                                        const std::string& hash_id);

/**
* Return the group id for this SRP param set, or else thrown an
* exception
*/
std::string BOTAN_DLL srp6_group_identifier(const BigInt& N, const BigInt& g);

/**
* Represents a SRP-6a server session
*/
class BOTAN_DLL SRP6_Server_Session
   {
   public:
      /**
      * Server side step 1
      * @param v the verification value saved from client registration
      */
      BigInt step1(const BigInt& v,
                   const std::string& group_id,
                   const std::string& hash_id,
                   RandomNumberGenerator& rng);

      SymmetricKey step2(const BigInt& A);

   private:
      std::string hash_id;
      BigInt B, b, v, S, p;
      size_t p_bytes;
   };

}


namespace Botan {

/**
* SAFER-SK
*/
class BOTAN_DLL SAFER_SK : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); }
      std::string name() const;
      BlockCipher* clone() const;

      /**
      * @param rounds the number of rounds to use - must be between 1
      * and 13
      */
      SAFER_SK(size_t rounds);
   private:
      size_t get_rounds() const { return (EK.size() - 8) / 16; }
      void key_schedule(const byte[], size_t);

      SecureVector<byte> EK;
   };

}


namespace Botan {

/**
* CAST-128
*/
class BOTAN_DLL CAST_128 : public Block_Cipher_Fixed_Params<8, 11, 16>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(MK); zeroise(RK); }
      std::string name() const { return "CAST-128"; }
      BlockCipher* clone() const { return new CAST_128; }

      CAST_128() : MK(16), RK(16) {}
   private:
      void key_schedule(const byte[], size_t);

      static void cast_ks(MemoryRegion<u32bit>& ks,
                          MemoryRegion<u32bit>& user_key);

      static const u32bit S5[256];
      static const u32bit S6[256];
      static const u32bit S7[256];
      static const u32bit S8[256];

      SecureVector<u32bit> MK, RK;
   };

extern const u32bit CAST_SBOX1[256];
extern const u32bit CAST_SBOX2[256];
extern const u32bit CAST_SBOX3[256];
extern const u32bit CAST_SBOX4[256];

}


namespace Botan {

/**
* PKCS #5 v2.0 PBE
*/
class BOTAN_DLL PBE_PKCS5v20 : public PBE
   {
   public:
      /**
      * @param cipher names a block cipher
      * @return true iff PKCS #5 knows how to use this cipher
      */
      static bool known_cipher(const std::string& cipher);

      std::string name() const;

      void write(const byte[], size_t);
      void start_msg();
      void end_msg();

      /**
      * Load a PKCS #5 v2.0 encrypted stream
      * @param input is the input source
      */
      PBE_PKCS5v20(DataSource& input);

      /**
      * @param cipher the block cipher to use
      * @param hash the hash function to use
      */
      PBE_PKCS5v20(BlockCipher* cipher, HashFunction* hash);

      ~PBE_PKCS5v20();
   private:
      void set_key(const std::string&);
      void new_params(RandomNumberGenerator& rng);
      MemoryVector<byte> encode_params() const;
      void decode_params(DataSource&);
      OID get_oid() const;

      void flush_pipe(bool);

      Cipher_Dir direction;
      BlockCipher* block_cipher;
      HashFunction* hash_function;
      SecureVector<byte> salt, key, iv;
      size_t iterations, key_length;
      Pipe pipe;
   };

}


namespace Botan {

/**
* Algorithm benchmark
* @param name the name of the algorithm to test (cipher, hash, or MAC)
* @param af the algorithm factory used to create objects
* @param rng the rng to use to generate random inputs
* @param milliseconds total time for the benchmark to run
* @param buf_size size of buffer to benchmark against, in KiB
* @return results a map from provider to speed in mebibytes per second
*/
std::map<std::string, double>
BOTAN_DLL algorithm_benchmark(const std::string& name,
                              Algorithm_Factory& af,
                              RandomNumberGenerator& rng,
                              u32bit milliseconds,
                              size_t buf_size);

}


namespace Botan {

/**
* CBC-MAC
*/
class BOTAN_DLL CBC_MAC : public MessageAuthenticationCode
   {
   public:
      std::string name() const;
      MessageAuthenticationCode* clone() const;
      size_t output_length() const { return e->block_size(); }
      void clear();

      Key_Length_Specification key_spec() const
         {
         return e->key_spec();
         }

      /**
      * @param cipher the underlying block cipher to use
      */
      CBC_MAC(BlockCipher* cipher);
      ~CBC_MAC();
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      void key_schedule(const byte[], size_t);

      BlockCipher* e;
      SecureVector<byte> state;
      size_t position;
   };

}


namespace Botan {

/**
* MARS, IBM's candidate for AES
*/
class BOTAN_DLL MARS : public Block_Cipher_Fixed_Params<16, 16, 32, 4>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(EK); }
      std::string name() const { return "MARS"; }
      BlockCipher* clone() const { return new MARS; }

      MARS() : EK(40) {}
   private:
      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> EK;
   };

}


namespace Botan {

/**
* PKCS #5 PBKDF2
*/
class BOTAN_DLL PKCS5_PBKDF2 : public PBKDF
   {
   public:
      std::string name() const
         {
         return "PBKDF2(" + mac->name() + ")";
         }

      PBKDF* clone() const
         {
         return new PKCS5_PBKDF2(mac->clone());
         }

      OctetString derive_key(size_t output_len,
                             const std::string& passphrase,
                             const byte salt[], size_t salt_len,
                             size_t iterations) const;

      /**
      * Create a PKCS #5 instance using the specified message auth code
      * @param mac_fn the MAC to use
      */
      PKCS5_PBKDF2(MessageAuthenticationCode* mac_fn) : mac(mac_fn) {}

      /**
      * Destructor
      */
      ~PKCS5_PBKDF2() { delete mac; }
   private:
      MessageAuthenticationCode* mac;
   };

}


namespace Botan {

class Algorithm_Factory;

/**
* Encrypt a key under a key encryption key using the algorithm
* described in RFC 3394
*
* @param key the plaintext key to encrypt
* @param kek the key encryption key
* @param af an algorithm factory
* @return key encrypted under kek
*/
SecureVector<byte> BOTAN_DLL rfc3394_keywrap(const MemoryRegion<byte>& key,
                                             const SymmetricKey& kek,
                                             Algorithm_Factory& af);

/**
* Decrypt a key under a key encryption key using the algorithm
* described in RFC 3394
*
* @param key the encrypted key to decrypt
* @param kek the key encryption key
* @param af an algorithm factory
* @return key decrypted under kek
*/
SecureVector<byte> BOTAN_DLL rfc3394_keyunwrap(const MemoryRegion<byte>& key,
                                               const SymmetricKey& kek,
                                               Algorithm_Factory& af);

}


namespace Botan {

/**
* DLIES Encryption
*/
class BOTAN_DLL DLIES_Encryptor : public PK_Encryptor
   {
   public:
      DLIES_Encryptor(const PK_Key_Agreement_Key&,
                      KDF* kdf,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      ~DLIES_Encryptor();

      void set_other_key(const MemoryRegion<byte>&);
   private:
      SecureVector<byte> enc(const byte[], size_t,
                             RandomNumberGenerator&) const;
      size_t maximum_input_size() const;

      SecureVector<byte> other_key, my_key;

      PK_Key_Agreement ka;
      KDF* kdf;
      MessageAuthenticationCode* mac;
      size_t mac_keylen;
   };

/**
* DLIES Decryption
*/
class BOTAN_DLL DLIES_Decryptor : public PK_Decryptor
   {
   public:
      DLIES_Decryptor(const PK_Key_Agreement_Key&,
                      KDF* kdf,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      ~DLIES_Decryptor();

   private:
      SecureVector<byte> dec(const byte[], size_t) const;

      SecureVector<byte> my_key;

      PK_Key_Agreement ka;
      KDF* kdf;
      MessageAuthenticationCode* mac;
      size_t mac_keylen;
   };

}


namespace Botan {

/**
* Output Feedback Mode
*/
class BOTAN_DLL OFB : public StreamCipher
   {
   public:
      void cipher(const byte in[], byte out[], size_t length);

      void set_iv(const byte iv[], size_t iv_len);

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len <= permutation->block_size()); }

      Key_Length_Specification key_spec() const
         {
         return permutation->key_spec();
         }

      std::string name() const;

      OFB* clone() const
         { return new OFB(permutation->clone()); }

      void clear();

      /**
      * @param cipher the underlying block cipher to use
      */
      OFB(BlockCipher* cipher);
      ~OFB();
   private:
      void key_schedule(const byte key[], size_t key_len);

      BlockCipher* permutation;
      SecureVector<byte> buffer;
      size_t position;
   };

}


namespace Botan {

/**
* Blowfish
*/
class BOTAN_DLL Blowfish : public Block_Cipher_Fixed_Params<8, 1, 56>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      /**
      * Modified EKSBlowfish key schedule, used for bcrypt password hashing
      */
      void eks_key_schedule(const byte key[], size_t key_length,
                            const byte salt[16], size_t workfactor);

      void clear();
      std::string name() const { return "Blowfish"; }
      BlockCipher* clone() const { return new Blowfish; }

      Blowfish() : S(1024), P(18) {}
   private:
      void key_schedule(const byte key[], size_t length);

      void key_expansion(const byte key[],
                         size_t key_length,
                         const byte salt[16]);

      void generate_sbox(MemoryRegion<u32bit>& box,
                         u32bit& L, u32bit& R,
                         const byte salt[16],
                         size_t salt_off) const;

      static const u32bit P_INIT[18];
      static const u32bit S_INIT[1024];

      SecureVector<u32bit> S;
      SecureVector<u32bit> P;
   };

}


namespace Botan {

/**
* The Adler32 checksum, used in zlib
*/
class BOTAN_DLL Adler32 : public HashFunction
   {
   public:
      std::string name() const { return "Adler32"; }
      size_t output_length() const { return 4; }
      HashFunction* clone() const { return new Adler32; }

      void clear() { S1 = 1; S2 = 0; }

      Adler32() { clear(); }
      ~Adler32() { clear(); }
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      u16bit S1, S2;
   };

}


namespace Botan {

/**
* Parallel Hashes
*/
class BOTAN_DLL Parallel : public HashFunction
   {
   public:
      void clear();
      std::string name() const;
      HashFunction* clone() const;

      size_t output_length() const;

      /**
      * @param hashes a set of hashes to compute in parallel
      */
      Parallel(const std::vector<HashFunction*>& hashes);
      ~Parallel();
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      std::vector<HashFunction*> hashes;
   };

}


namespace Botan {

/**
* RC5
*/
class BOTAN_DLL RC5 : public Block_Cipher_Fixed_Params<8, 1, 32>
   {
   public:
      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      void clear() { zeroise(S); }
      std::string name() const;
      BlockCipher* clone() const { return new RC5(get_rounds()); }

      /**
      * @param rounds the number of RC5 rounds to run. Must be between
      * 8 and 32 and a multiple of 4.
      */
      RC5(size_t rounds);
   private:
      size_t get_rounds() const { return (S.size() - 2) / 2; }

      void key_schedule(const byte[], size_t);

      SecureVector<u32bit> S;
   };

}


namespace Botan {

/**
* Run a set of self tests on some basic algorithms like AES and SHA-1
* @param af an algorithm factory
* @throws Self_Test_Error if a failure occured
*/
BOTAN_DLL void confirm_startup_self_tests(Algorithm_Factory& af);

/**
* Run a set of self tests on some basic algorithms like AES and SHA-1
* @param af an algorithm factory
* @returns false if a failure occured, otherwise true
*/
BOTAN_DLL bool passes_self_tests(Algorithm_Factory& af);

/**
* Run a set of algorithm KATs (known answer tests)
* @param algo_name the algorithm we are testing
* @param vars a set of input variables for this test, all
         hex encoded. Keys used: "input", "output", "key", and "iv"
* @param af an algorithm factory
* @returns map from provider name to test result for that provider
*/
BOTAN_DLL std::map<std::string, bool>
algorithm_kat(const SCAN_Name& algo_name,
              const std::map<std::string, std::string>& vars,
              Algorithm_Factory& af);

}


namespace Botan {

/**
* Noekeon implementation using SIMD operations
*/
class BOTAN_DLL Noekeon_SIMD : public Noekeon
   {
   public:
      size_t parallelism() const { return 4; }

      void encrypt_n(const byte in[], byte out[], size_t blocks) const;
      void decrypt_n(const byte in[], byte out[], size_t blocks) const;

      BlockCipher* clone() const { return new Noekeon_SIMD; }
   };

}


namespace Botan {

/**
* PKCS #5 v1.5 PBE
*/
class BOTAN_DLL PBE_PKCS5v15 : public PBE
   {
   public:
      std::string name() const;

      void write(const byte[], size_t);
      void start_msg();
      void end_msg();

      /**
      * @param cipher the block cipher to use (DES or RC2)
      * @param hash the hash function to use
      * @param direction are we encrypting or decrypting
      */
      PBE_PKCS5v15(BlockCipher* cipher,
                   HashFunction* hash,
                   Cipher_Dir direction);

      ~PBE_PKCS5v15();
   private:
      void set_key(const std::string&);
      void new_params(RandomNumberGenerator& rng);
      MemoryVector<byte> encode_params() const;
      void decode_params(DataSource&);
      OID get_oid() const;

      void flush_pipe(bool);

      Cipher_Dir direction;
      BlockCipher* block_cipher;
      HashFunction* hash_function;

      SecureVector<byte> salt, key, iv;
      size_t iterations;
      Pipe pipe;
   };

}


namespace Botan {

/**
* CTR-BE (Counter mode, big-endian)
*/
class BOTAN_DLL CTR_BE : public StreamCipher
   {
   public:
      void cipher(const byte in[], byte out[], size_t length);

      void set_iv(const byte iv[], size_t iv_len);

      bool valid_iv_length(size_t iv_len) const
         { return (iv_len <= permutation->block_size()); }

      Key_Length_Specification key_spec() const
         {
         return permutation->key_spec();
         }

      std::string name() const;

      CTR_BE* clone() const
         { return new CTR_BE(permutation->clone()); }

      void clear();

      /**
      * @param cipher the underlying block cipher to use
      */
      CTR_BE(BlockCipher* cipher);
      ~CTR_BE();
   private:
      void key_schedule(const byte key[], size_t key_len);
      void increment_counter();

      BlockCipher* permutation;
      SecureVector<byte> counter, buffer;
      size_t position;
   };

}


namespace Botan {

/**
* Randpool
*/
class BOTAN_DLL Randpool : public RandomNumberGenerator
   {
   public:
      void randomize(byte[], size_t);
      bool is_seeded() const { return seeded; }
      void clear();
      std::string name() const;

      void reseed(size_t bits_to_collect);
      void add_entropy_source(EntropySource* es);
      void add_entropy(const byte input[], size_t length);

      /**
      * @param cipher a block cipher to use
      * @param mac a message authentication code to use
      * @param pool_blocks how many cipher blocks to use for the pool
      * @param iterations_before_reseed how many times we'll use the
      * internal state to generate output before reseeding
      */
      Randpool(BlockCipher* cipher,
               MessageAuthenticationCode* mac,
               size_t pool_blocks = 32,
               size_t iterations_before_reseed = 128);

      ~Randpool();
   private:
      void update_buffer();
      void mix_pool();

      size_t ITERATIONS_BEFORE_RESEED, POOL_BLOCKS;
      BlockCipher* cipher;
      MessageAuthenticationCode* mac;

      std::vector<EntropySource*> entropy_sources;
      SecureVector<byte> pool, buffer, counter;
      bool seeded;
   };

}


namespace Botan {

/**
* PRF used in SSLv3
*/
class BOTAN_DLL SSL3_PRF : public KDF
   {
   public:
      SecureVector<byte> derive(size_t, const byte[], size_t,
                                const byte[], size_t) const;

      std::string name() const { return "SSL3-PRF"; }
      KDF* clone() const { return new SSL3_PRF; }
   };

}


namespace Botan {

/**
* DES/3DES-based MAC from ANSI X9.19
*/
class BOTAN_DLL ANSI_X919_MAC : public MessageAuthenticationCode
   {
   public:
      void clear();
      std::string name() const;
      size_t output_length() const { return e->block_size(); }
      MessageAuthenticationCode* clone() const;

      Key_Length_Specification key_spec() const
         {
         return Key_Length_Specification(8, 16, 8);
         }

      /**
      * @param cipher the underlying block cipher to use
      */
      ANSI_X919_MAC(BlockCipher* cipher);
      ~ANSI_X919_MAC();
   private:
      void add_data(const byte[], size_t);
      void final_result(byte[]);
      void key_schedule(const byte[], size_t);

      BlockCipher* e;
      BlockCipher* d;
      SecureVector<byte> state;
      size_t position;
   };

}

#endif // USE_SYSTEM_BOTAN
#endif // BOTAN_AMALGAMATION_H__

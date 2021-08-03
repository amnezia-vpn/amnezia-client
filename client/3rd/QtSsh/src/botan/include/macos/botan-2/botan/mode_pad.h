/*
* CBC Padding Methods
* (C) 1999-2008,2013 Jack Lloyd
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MODE_PADDING_H_
#define BOTAN_MODE_PADDING_H_

#include <botan/secmem.h>
#include <string>

BOTAN_FUTURE_INTERNAL_HEADER(mode_pad.h)

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
class BOTAN_PUBLIC_API(2,0) BlockCipherModePaddingMethod
   {
   public:
      /**
      * Add padding bytes to buffer.
      * @param buffer data to pad
      * @param final_block_bytes size of the final block in bytes
      * @param block_size size of each block in bytes
      */
      virtual void add_padding(secure_vector<uint8_t>& buffer,
                               size_t final_block_bytes,
                               size_t block_size) const = 0;

      /**
      * Remove padding bytes from block
      * @param block the last block
      * @param len the size of the block in bytes
      * @return number of data bytes, or if the padding is invalid returns len
      */
      virtual size_t unpad(const uint8_t block[], size_t len) const = 0;

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
      virtual ~BlockCipherModePaddingMethod() = default;
   };

/**
* PKCS#7 Padding
*/
class BOTAN_PUBLIC_API(2,0) PKCS7_Padding final : public BlockCipherModePaddingMethod
   {
   public:
      void add_padding(secure_vector<uint8_t>& buffer,
                       size_t final_block_bytes,
                       size_t block_size) const override;

      size_t unpad(const uint8_t[], size_t) const override;

      bool valid_blocksize(size_t bs) const override { return (bs > 2 && bs < 256); }

      std::string name() const override { return "PKCS7"; }
   };

/**
* ANSI X9.23 Padding
*/
class BOTAN_PUBLIC_API(2,0) ANSI_X923_Padding final : public BlockCipherModePaddingMethod
   {
   public:
      void add_padding(secure_vector<uint8_t>& buffer,
                       size_t final_block_bytes,
                       size_t block_size) const override;

      size_t unpad(const uint8_t[], size_t) const override;

      bool valid_blocksize(size_t bs) const override { return (bs > 2 && bs < 256); }

      std::string name() const override { return "X9.23"; }
   };

/**
* One And Zeros Padding (ISO/IEC 9797-1, padding method 2)
*/
class BOTAN_PUBLIC_API(2,0) OneAndZeros_Padding final : public BlockCipherModePaddingMethod
   {
   public:
      void add_padding(secure_vector<uint8_t>& buffer,
                       size_t final_block_bytes,
                       size_t block_size) const override;

      size_t unpad(const uint8_t[], size_t) const override;

      bool valid_blocksize(size_t bs) const override { return (bs > 2); }

      std::string name() const override { return "OneAndZeros"; }
   };

/**
* ESP Padding (RFC 4304)
*/
class BOTAN_PUBLIC_API(2,0) ESP_Padding final : public BlockCipherModePaddingMethod
   {
   public:
      void add_padding(secure_vector<uint8_t>& buffer,
                       size_t final_block_bytes,
                       size_t block_size) const override;

      size_t unpad(const uint8_t[], size_t) const override;

      bool valid_blocksize(size_t bs) const override { return (bs > 2 && bs < 256); }

      std::string name() const override { return "ESP"; }
   };

/**
* Null Padding
*/
class BOTAN_PUBLIC_API(2,0) Null_Padding final : public BlockCipherModePaddingMethod
   {
   public:
      void add_padding(secure_vector<uint8_t>&, size_t, size_t) const override
         {
         /* no padding */
         }

      size_t unpad(const uint8_t[], size_t size) const override { return size; }

      bool valid_blocksize(size_t) const override { return true; }

      std::string name() const override { return "NoPadding"; }
   };

/**
* Get a block cipher padding mode by name (eg "NoPadding" or "PKCS7")
* @param algo_spec block cipher padding mode name
*/
BOTAN_PUBLIC_API(2,0) BlockCipherModePaddingMethod* get_bc_pad(const std::string& algo_spec);

}

#endif

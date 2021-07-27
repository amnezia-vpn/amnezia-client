/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PSK_DB_H_
#define BOTAN_PSK_DB_H_

#include <botan/secmem.h>
#include <memory>
#include <string>
#include <set>

namespace Botan {

class BlockCipher;
class MessageAuthenticationCode;

/**
* This is an interface to a generic PSK (pre-shared key) database.
* It might be implemented as a plaintext storage or via some mechanism
* that encrypts the keys and/or values.
*/
class BOTAN_PUBLIC_API(2,4) PSK_Database
   {
   public:
      /**
      * Return the set of names for which get() will return a value.
      */
      virtual std::set<std::string> list_names() const = 0;

      /**
      * Return the value associated with the specified @param name or otherwise
      * throw an exception.
      */
      virtual secure_vector<uint8_t> get(const std::string& name) const = 0;

      /**
      * Set a value that can later be accessed with get().
      * If name already exists in the database, the old value will be overwritten.
      */
      virtual void set(const std::string& name, const uint8_t psk[], size_t psk_len) = 0;

      /**
      * Remove a PSK from the database
      */
      virtual void remove(const std::string& name) = 0;

      /**
      * Returns if the values in the PSK database are encrypted. If
      * false, saved values are being stored in plaintext.
      */
      virtual bool is_encrypted() const = 0;

      /**
      * Get a PSK in the form of a string (eg if the PSK is a password)
      */
      std::string get_str(const std::string& name) const
         {
         secure_vector<uint8_t> psk = get(name);
         return std::string(cast_uint8_ptr_to_char(psk.data()), psk.size());
         }

      void set_str(const std::string& name, const std::string& psk)
         {
         set(name, cast_char_ptr_to_uint8(psk.data()), psk.size());
         }

      template<typename Alloc>
      void set_vec(const std::string& name,
                   const std::vector<uint8_t, Alloc>& psk)

         {
         set(name, psk.data(), psk.size());
         }

      virtual ~PSK_Database() = default;
   };

/**
* A mixin for an encrypted PSK database.
* Both keys and values are encrypted with NIST AES-256 key wrapping.
* Values are padded to obscure their length before encryption, allowing
* it to be used as a password vault.
*
* Subclasses must implement the virtual calls to handle storing and
* getting raw (base64 encoded) values.
*/
class BOTAN_PUBLIC_API(2,4) Encrypted_PSK_Database : public PSK_Database
   {
   public:
      /**
      * @param master_key specifies the master key used to encrypt all
      * keys and value. It can be of any length, but should be at least 256 bits.
      *
      * Subkeys for the cryptographic algorithms used are derived from this
      * master key. No key stretching is performed; if encrypting a PSK database
      * using a password, it is recommended to use PBKDF2 to derive the database
      * master key.
      */
      Encrypted_PSK_Database(const secure_vector<uint8_t>& master_key);

      ~Encrypted_PSK_Database();

      std::set<std::string> list_names() const override;

      secure_vector<uint8_t> get(const std::string& name) const override;

      void set(const std::string& name, const uint8_t psk[], size_t psk_len) override;

      void remove(const std::string& name) override;

      bool is_encrypted() const override { return true; }

   protected:
      /**
      * Save a encrypted (name.value) pair to the database. Both will be base64 encoded strings.
      */
      virtual void kv_set(const std::string& index, const std::string& value) = 0;

      /**
      * Get a value previously saved with set_raw_value. Should return an empty
      * string if index is not found.
      */
      virtual std::string kv_get(const std::string& index) const = 0;

      /**
      * Remove an index
      */
      virtual void kv_del(const std::string& index) = 0;

      /**
      * Return all indexes in the table.
      */
      virtual std::set<std::string> kv_get_all() const = 0;

   private:
      std::unique_ptr<BlockCipher> m_cipher;
      std::unique_ptr<MessageAuthenticationCode> m_hmac;
      secure_vector<uint8_t> m_wrap_key;
   };

class SQL_Database;

class BOTAN_PUBLIC_API(2,4) Encrypted_PSK_Database_SQL : public Encrypted_PSK_Database
   {
   public:
      Encrypted_PSK_Database_SQL(const secure_vector<uint8_t>& master_key,
                                 std::shared_ptr<SQL_Database> db,
                                 const std::string& table_name);

      ~Encrypted_PSK_Database_SQL();
   private:
      void kv_set(const std::string& index, const std::string& value) override;
      std::string kv_get(const std::string& index) const override;
      void kv_del(const std::string& index) override;
      std::set<std::string> kv_get_all() const override;

      std::shared_ptr<SQL_Database> m_db;
      const std::string m_table_name;
   };

}

#endif

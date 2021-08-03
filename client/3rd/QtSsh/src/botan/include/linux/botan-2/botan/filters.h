/*
* Common Filters
* (C) 1999-2007,2015 Jack Lloyd
* (C) 2013 Joel Low
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FILTERS_H_
#define BOTAN_FILTERS_H_

#include <botan/secmem.h>
#include <botan/data_snk.h>
#include <botan/pipe.h>
#include <botan/symkey.h>
#include <botan/cipher_mode.h>

#if defined(BOTAN_TARGET_OS_HAS_THREADS)
  #include <thread>
#endif

#if defined(BOTAN_HAS_STREAM_CIPHER)
   #include <botan/stream_cipher.h>
#endif

#if defined(BOTAN_HAS_HASH)
   #include <botan/hash.h>
#endif

#if defined(BOTAN_HAS_MAC)
   #include <botan/mac.h>
#endif

namespace Botan {

/**
* Filter mixin that breaks input into blocks, useful for
* cipher modes
*/
class BOTAN_PUBLIC_API(2,0) Buffered_Filter
   {
   public:
      /**
      * Write bytes into the buffered filter, which will them emit them
      * in calls to buffered_block in the subclass
      * @param in the input bytes
      * @param length of in in bytes
      */
      void write(const uint8_t in[], size_t length);

      template<typename Alloc>
         void write(const std::vector<uint8_t, Alloc>& in, size_t length)
         {
         write(in.data(), length);
         }

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

      virtual ~Buffered_Filter() = default;
   protected:
      /**
      * The block processor, implemented by subclasses
      * @param input some input bytes
      * @param length the size of input, guaranteed to be a multiple
      *        of block_size
      */
      virtual void buffered_block(const uint8_t input[], size_t length) = 0;

      /**
      * The final block, implemented by subclasses
      * @param input some input bytes
      * @param length the size of input, guaranteed to be at least
      *        final_minimum bytes
      */
      virtual void buffered_final(const uint8_t input[], size_t length) = 0;

      /**
      * @return block size of inputs
      */
      size_t buffered_block_size() const { return m_main_block_mod; }

      /**
      * @return current position in the buffer
      */
      size_t current_position() const { return m_buffer_pos; }

      /**
      * Reset the buffer position
      */
      void buffer_reset() { m_buffer_pos = 0; }
   private:
      size_t m_main_block_mod, m_final_minimum;

      secure_vector<uint8_t> m_buffer;
      size_t m_buffer_pos;
   };

/**
* This class represents keyed filters, i.e. filters that have to be
* fed with a key in order to function.
*/
class BOTAN_PUBLIC_API(2,0) Keyed_Filter : public Filter
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
      virtual void set_iv(const InitializationVector& iv)
         {
         if(iv.length() != 0)
            throw Invalid_IV_Length(name(), iv.length());
         }

      /**
      * Check whether a key length is valid for this filter
      * @param length the key length to be checked for validity
      * @return true if the key length is valid, false otherwise
      */
      bool valid_keylength(size_t length) const
         {
         return key_spec().valid_keylength(length);
         }

      /**
      * @return object describing limits on key size
      */
      virtual Key_Length_Specification key_spec() const = 0;

      /**
      * Check whether an IV length is valid for this filter
      * @param length the IV length to be checked for validity
      * @return true if the IV length is valid, false otherwise
      */
      virtual bool valid_iv_length(size_t length) const
         { return (length == 0); }
   };

/**
* Filter interface for cipher modes
*/
class BOTAN_PUBLIC_API(2,0) Cipher_Mode_Filter final : public Keyed_Filter,
                                     private Buffered_Filter
   {
   public:
      explicit Cipher_Mode_Filter(Cipher_Mode* t);

      explicit Cipher_Mode_Filter(std::unique_ptr<Cipher_Mode> t) :
         Cipher_Mode_Filter(t.release()) {}

      void set_iv(const InitializationVector& iv) override;

      void set_key(const SymmetricKey& key) override;

      Key_Length_Specification key_spec() const override;

      bool valid_iv_length(size_t length) const override;

      std::string name() const override;

   private:
      void write(const uint8_t input[], size_t input_length) override;
      void start_msg() override;
      void end_msg() override;

      void buffered_block(const uint8_t input[], size_t input_length) override;
      void buffered_final(const uint8_t input[], size_t input_length) override;

      std::unique_ptr<Cipher_Mode> m_mode;
      std::vector<uint8_t> m_nonce;
      secure_vector<uint8_t> m_buffer;
   };

// deprecated aliases, will be removed in a future major release
typedef Cipher_Mode_Filter Transform_Filter;
typedef Transform_Filter Transformation_Filter;

/*
* Get a cipher object
*/

/**
* Factory method for general symmetric cipher filters. No key will be
* set in the filter.
*
* @param algo_spec the name of the desired cipher
* @param direction determines whether the filter will be an encrypting or
* decrypting filter
* @return pointer to the encryption or decryption filter
*/
inline Keyed_Filter* get_cipher(const std::string& algo_spec,
                                Cipher_Dir direction)
   {
   std::unique_ptr<Cipher_Mode> c(Cipher_Mode::create_or_throw(algo_spec, direction));
   return new Cipher_Mode_Filter(c.release());
   }

/**
* Factory method for general symmetric cipher filters.
* @param algo_spec the name of the desired cipher
* @param key the key to be used for encryption/decryption performed by
* the filter
* @param direction determines whether the filter will be an encrypting
* or decrypting filter
* @return pointer to the encryption or decryption filter
*/
inline Keyed_Filter* get_cipher(const std::string& algo_spec,
                                const SymmetricKey& key,
                                Cipher_Dir direction)
   {
   Keyed_Filter* cipher = get_cipher(algo_spec, direction);
   cipher->set_key(key);
   return cipher;
   }

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
inline Keyed_Filter* get_cipher(const std::string& algo_spec,
                                const SymmetricKey& key,
                                const InitializationVector& iv,
                                Cipher_Dir direction)
   {
   Keyed_Filter* cipher = get_cipher(algo_spec, key, direction);
   if(iv.length())
      cipher->set_iv(iv);
   return cipher;
   }

#if defined(BOTAN_HAS_STREAM_CIPHER)

/**
* Stream Cipher Filter
*/
class BOTAN_PUBLIC_API(2,0) StreamCipher_Filter final : public Keyed_Filter
   {
   public:

      std::string name() const override { return m_cipher->name(); }

      /**
      * Write input data
      * @param input data
      * @param input_len length of input in bytes
      */
      void write(const uint8_t input[], size_t input_len) override;

      bool valid_iv_length(size_t iv_len) const override
         { return m_cipher->valid_iv_length(iv_len); }

      /**
      * Set the initialization vector for this filter.
      * @param iv the initialization vector to set
      */
      void set_iv(const InitializationVector& iv) override
         {
         m_cipher->set_iv(iv.begin(), iv.length());
         }

      /**
      * Set the key of this filter.
      * @param key the key to set
      */
      void set_key(const SymmetricKey& key) override { m_cipher->set_key(key); }

      Key_Length_Specification key_spec() const override { return m_cipher->key_spec(); }

      /**
      * Construct a stream cipher filter.
      * @param cipher a cipher object to use
      */
      explicit StreamCipher_Filter(StreamCipher* cipher);

      /**
      * Construct a stream cipher filter.
      * @param cipher a cipher object to use
      * @param key the key to use inside this filter
      */
      StreamCipher_Filter(StreamCipher* cipher, const SymmetricKey& key);

      /**
      * Construct a stream cipher filter.
      * @param cipher the name of the desired cipher
      */
      explicit StreamCipher_Filter(const std::string& cipher);

      /**
      * Construct a stream cipher filter.
      * @param cipher the name of the desired cipher
      * @param key the key to use inside this filter
      */
      StreamCipher_Filter(const std::string& cipher, const SymmetricKey& key);
   private:
      secure_vector<uint8_t> m_buffer;
      std::unique_ptr<StreamCipher> m_cipher;
   };
#endif

#if defined(BOTAN_HAS_HASH)

/**
* Hash Filter.
*/
class BOTAN_PUBLIC_API(2,0) Hash_Filter final : public Filter
   {
   public:
      void write(const uint8_t input[], size_t len) override { m_hash->update(input, len); }
      void end_msg() override;

      std::string name() const override { return m_hash->name(); }

      /**
      * Construct a hash filter.
      * @param hash the hash function to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the hashfunction
      * hash. Otherwise, specify a smaller value here so that the
      * output of the hash algorithm will be cut off.
      */
      Hash_Filter(HashFunction* hash, size_t len = 0) :
         m_hash(hash), m_out_len(len) {}

      /**
      * Construct a hash filter.
      * @param request the name of the hash algorithm to use
      * @param len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the hashfunction
      * hash. Otherwise, specify a smaller value here so that the
      * output of the hash algorithm will be cut off.
      */
      Hash_Filter(const std::string& request, size_t len = 0);

   private:
      std::unique_ptr<HashFunction> m_hash;
      const size_t m_out_len;
   };
#endif

#if defined(BOTAN_HAS_MAC)

/**
* MessageAuthenticationCode Filter.
*/
class BOTAN_PUBLIC_API(2,0) MAC_Filter final : public Keyed_Filter
   {
   public:
      void write(const uint8_t input[], size_t len) override { m_mac->update(input, len); }
      void end_msg() override;

      std::string name() const override { return m_mac->name(); }

      /**
      * Set the key of this filter.
      * @param key the key to set
      */
      void set_key(const SymmetricKey& key) override { m_mac->set_key(key); }

      Key_Length_Specification key_spec() const override { return m_mac->key_spec(); }

      /**
      * Construct a MAC filter. The MAC key will be left empty.
      * @param mac the MAC to use
      * @param out_len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(MessageAuthenticationCode* mac,
                 size_t out_len = 0) :
         m_mac(mac),
         m_out_len(out_len)
         {
         }

      /**
      * Construct a MAC filter.
      * @param mac the MAC to use
      * @param key the MAC key to use
      * @param out_len the output length of this filter. Leave the default
      * value 0 if you want to use the full output of the
      * MAC. Otherwise, specify a smaller value here so that the
      * output of the MAC will be cut off.
      */
      MAC_Filter(MessageAuthenticationCode* mac,
                 const SymmetricKey& key,
                 size_t out_len = 0) :
         m_mac(mac),
         m_out_len(out_len)
         {
         m_mac->set_key(key);
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
   private:
      std::unique_ptr<MessageAuthenticationCode> m_mac;
      const size_t m_out_len;
   };
#endif

#if defined(BOTAN_HAS_COMPRESSION)

class Compression_Algorithm;
class Decompression_Algorithm;

/**
* Filter interface for compression
*/
class BOTAN_PUBLIC_API(2,0) Compression_Filter final : public Filter
   {
   public:
      void start_msg() override;
      void write(const uint8_t input[], size_t input_length) override;
      void end_msg() override;

      void flush();

      std::string name() const override;

      Compression_Filter(const std::string& type,
                         size_t compression_level,
                         size_t buffer_size = 4096);

      ~Compression_Filter();
   private:
      std::unique_ptr<Compression_Algorithm> m_comp;
      size_t m_buffersize, m_level;
      secure_vector<uint8_t> m_buffer;
   };

/**
* Filter interface for decompression
*/
class BOTAN_PUBLIC_API(2,0) Decompression_Filter final : public Filter
   {
   public:
      void start_msg() override;
      void write(const uint8_t input[], size_t input_length) override;
      void end_msg() override;

      std::string name() const override;

      Decompression_Filter(const std::string& type,
                           size_t buffer_size = 4096);

      ~Decompression_Filter();
   private:
      std::unique_ptr<Decompression_Algorithm> m_comp;
      std::size_t m_buffersize;
      secure_vector<uint8_t> m_buffer;
   };

#endif

/**
* This class represents a Base64 encoder.
*/
class BOTAN_PUBLIC_API(2,0) Base64_Encoder final : public Filter
   {
   public:
      std::string name() const override { return "Base64_Encoder"; }

      /**
      * Input a part of a message to the encoder.
      * @param input the message to input as a byte array
      * @param length the length of the byte array input
      */
      void write(const uint8_t input[], size_t length) override;

      /**
      * Inform the Encoder that the current message shall be closed.
      */
      void end_msg() override;

      /**
      * Create a base64 encoder.
      * @param breaks whether to use line breaks in the output
      * @param length the length of the lines of the output
      * @param t_n whether to use a trailing newline
      */
      Base64_Encoder(bool breaks = false, size_t length = 72,
                     bool t_n = false);
   private:
      void encode_and_send(const uint8_t input[], size_t length,
                           bool final_inputs = false);
      void do_output(const uint8_t output[], size_t length);

      const size_t m_line_length;
      const bool m_trailing_newline;
      std::vector<uint8_t> m_in, m_out;
      size_t m_position, m_out_position;
   };

/**
* This object represents a Base64 decoder.
*/
class BOTAN_PUBLIC_API(2,0) Base64_Decoder final : public Filter
   {
   public:
      std::string name() const override { return "Base64_Decoder"; }

      /**
      * Input a part of a message to the decoder.
      * @param input the message to input as a byte array
      * @param length the length of the byte array input
      */
      void write(const uint8_t input[], size_t length) override;

      /**
      * Finish up the current message
      */
      void end_msg() override;

      /**
      * Create a base64 decoder.
      * @param checking the type of checking that shall be performed by
      * the decoder
      */
      explicit Base64_Decoder(Decoder_Checking checking = NONE);
   private:
      const Decoder_Checking m_checking;
      std::vector<uint8_t> m_in, m_out;
      size_t m_position;
   };

/**
* Converts arbitrary binary data to hex strings, optionally with
* newlines inserted
*/
class BOTAN_PUBLIC_API(2,0) Hex_Encoder final : public Filter
   {
   public:
      /**
      * Whether to use uppercase or lowercase letters for the encoded string.
      */
      enum Case { Uppercase, Lowercase };

      std::string name() const override { return "Hex_Encoder"; }

      void write(const uint8_t in[], size_t length) override;
      void end_msg() override;

      /**
      * Create a hex encoder.
      * @param the_case the case to use in the encoded strings.
      */
      explicit Hex_Encoder(Case the_case);

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
      void encode_and_send(const uint8_t[], size_t);

      const Case m_casing;
      const size_t m_line_length;
      std::vector<uint8_t> m_in, m_out;
      size_t m_position, m_counter;
   };

/**
* Converts hex strings to bytes
*/
class BOTAN_PUBLIC_API(2,0) Hex_Decoder final : public Filter
   {
   public:
      std::string name() const override { return "Hex_Decoder"; }

      void write(const uint8_t[], size_t) override;
      void end_msg() override;

      /**
      * Construct a Hex Decoder using the specified
      * character checking.
      * @param checking the checking to use during decoding.
      */
      explicit Hex_Decoder(Decoder_Checking checking = NONE);
   private:
      const Decoder_Checking m_checking;
      std::vector<uint8_t> m_in, m_out;
      size_t m_position;
   };

/**
* BitBucket is a filter which simply discards all inputs
*/
class BOTAN_PUBLIC_API(2,0) BitBucket final : public Filter
   {
   public:
      void write(const uint8_t[], size_t) override { /* discard */ }

      std::string name() const override { return "BitBucket"; }
   };

/**
* This class represents Filter chains. A Filter chain is an ordered
* concatenation of Filters, the input to a Chain sequentially passes
* through all the Filters contained in the Chain.
*/

class BOTAN_PUBLIC_API(2,0) Chain final : public Fanout_Filter
   {
   public:
      void write(const uint8_t input[], size_t length) override { send(input, length); }

      std::string name() const override { return "Chain"; }

      /**
      * Construct a chain of up to four filters. The filters are set
      * up in the same order as the arguments.
      */
      Chain(Filter* = nullptr, Filter* = nullptr,
            Filter* = nullptr, Filter* = nullptr);

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
class BOTAN_PUBLIC_API(2,0) Fork : public Fanout_Filter
   {
   public:
      void write(const uint8_t input[], size_t length) override { send(input, length); }
      void set_port(size_t n) { Fanout_Filter::set_port(n); }

      std::string name() const override { return "Fork"; }

      /**
      * Construct a Fork filter with up to four forks.
      */
      Fork(Filter*, Filter*, Filter* = nullptr, Filter* = nullptr);

      /**
      * Construct a Fork from range of filters
      * @param filter_arr the list of filters
      * @param length how many filters
      */
      Fork(Filter* filter_arr[], size_t length);
   };

#if defined(BOTAN_HAS_THREAD_UTILS)

/**
* This class is a threaded version of the Fork filter. While this uses
* threads, the class itself is NOT thread-safe. This is meant as a drop-
* in replacement for Fork where performance gains are possible.
*/
class BOTAN_PUBLIC_API(2,0) Threaded_Fork final : public Fork
   {
   public:
      std::string name() const override;

      /**
      * Construct a Threaded_Fork filter with up to four forks.
      */
      Threaded_Fork(Filter*, Filter*, Filter* = nullptr, Filter* = nullptr);

      /**
      * Construct a Threaded_Fork from range of filters
      * @param filter_arr the list of filters
      * @param length how many filters
      */
      Threaded_Fork(Filter* filter_arr[], size_t length);

      ~Threaded_Fork();

   private:
      void set_next(Filter* f[], size_t n);
      void send(const uint8_t in[], size_t length) override;
      void thread_delegate_work(const uint8_t input[], size_t length);
      void thread_entry(Filter* filter);

      std::vector<std::shared_ptr<std::thread>> m_threads;
      std::unique_ptr<struct Threaded_Fork_Data> m_thread_data;
   };
#endif

}

#endif

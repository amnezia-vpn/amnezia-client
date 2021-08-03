/*
* BigInt
* (C) 1999-2008,2012,2018 Jack Lloyd
*     2007 FlexSecure
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BIGINT_H_
#define BOTAN_BIGINT_H_

#include <botan/types.h>
#include <botan/secmem.h>
#include <botan/exceptn.h>
#include <iosfwd>

namespace Botan {

class RandomNumberGenerator;

/**
* Arbitrary precision integer
*/
class BOTAN_PUBLIC_API(2,0) BigInt final
   {
   public:
     /**
     * Base enumerator for encoding and decoding
     */
     enum Base { Decimal = 10, Hexadecimal = 16, Binary = 256 };

     /**
     * Sign symbol definitions for positive and negative numbers
     */
     enum Sign { Negative = 0, Positive = 1 };

     /**
     * DivideByZero Exception
     *
     * In a future release this exception will be removed and its usage
     * replaced by Invalid_Argument
     */
     class BOTAN_PUBLIC_API(2,0) DivideByZero final : public Invalid_Argument
        {
        public:
           DivideByZero() : Invalid_Argument("BigInt divide by zero") {}
        };

     /**
     * Create empty BigInt
     */
     BigInt() = default;

     /**
     * Create BigInt from 64 bit integer
     * @param n initial value of this BigInt
     */
     BigInt(uint64_t n);

     /**
     * Copy Constructor
     * @param other the BigInt to copy
     */
     BigInt(const BigInt& other) = default;

     /**
     * Create BigInt from a string. If the string starts with 0x the
     * rest of the string will be interpreted as hexadecimal digits.
     * Otherwise, it will be interpreted as a decimal number.
     *
     * @param str the string to parse for an integer value
     */
     explicit BigInt(const std::string& str);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the byte array holding the value
     * @param length size of buf
     */
     BigInt(const uint8_t buf[], size_t length);

     /**
     * Create a BigInt from an integer in a byte array
     * @param vec the byte vector holding the value
     */
     template<typename Alloc>
     explicit BigInt(const std::vector<uint8_t, Alloc>& vec) : BigInt(vec.data(), vec.size()) {}

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the byte array holding the value
     * @param length size of buf
     * @param base is the number base of the integer in buf
     */
     BigInt(const uint8_t buf[], size_t length, Base base);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the byte array holding the value
     * @param length size of buf
     * @param max_bits if the resulting integer is more than max_bits,
     *        it will be shifted so it is at most max_bits in length.
     */
     BigInt(const uint8_t buf[], size_t length, size_t max_bits);

     /**
     * Create a BigInt from an array of words
     * @param words the words
     * @param length number of words
     */
     BigInt(const word words[], size_t length);

     /**
     * \brief Create a random BigInt of the specified size
     *
     * @param rng random number generator
     * @param bits size in bits
     * @param set_high_bit if true, the highest bit is always set
     *
     * @see randomize
     */
     BigInt(RandomNumberGenerator& rng, size_t bits, bool set_high_bit = true);

     /**
     * Create BigInt of specified size, all zeros
     * @param sign the sign
     * @param n size of the internal register in words
     */
     BigInt(Sign sign, size_t n);

     /**
     * Move constructor
     */
     BigInt(BigInt&& other)
        {
        this->swap(other);
        }

     ~BigInt() { const_time_unpoison(); }

     /**
     * Move assignment
     */
     BigInt& operator=(BigInt&& other)
        {
        if(this != &other)
           this->swap(other);

        return (*this);
        }

     /**
     * Copy assignment
     */
     BigInt& operator=(const BigInt&) = default;

     /**
     * Swap this value with another
     * @param other BigInt to swap values with
     */
     void swap(BigInt& other)
        {
        m_data.swap(other.m_data);
        std::swap(m_signedness, other.m_signedness);
        }

     void swap_reg(secure_vector<word>& reg)
        {
        m_data.swap(reg);
        // sign left unchanged
        }

     /**
     * += operator
     * @param y the BigInt to add to this
     */
     BigInt& operator+=(const BigInt& y)
        {
        return add(y.data(), y.sig_words(), y.sign());
        }

     /**
     * += operator
     * @param y the word to add to this
     */
     BigInt& operator+=(word y)
        {
        return add(&y, 1, Positive);
        }

     /**
     * -= operator
     * @param y the BigInt to subtract from this
     */
     BigInt& operator-=(const BigInt& y)
        {
        return sub(y.data(), y.sig_words(), y.sign());
        }

     /**
     * -= operator
     * @param y the word to subtract from this
     */
     BigInt& operator-=(word y)
        {
        return sub(&y, 1, Positive);
        }

     /**
     * *= operator
     * @param y the BigInt to multiply with this
     */
     BigInt& operator*=(const BigInt& y);

     /**
     * *= operator
     * @param y the word to multiply with this
     */
     BigInt& operator*=(word y);

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

     static BigInt add2(const BigInt& x, const word y[], size_t y_words, Sign y_sign);

     BigInt& add(const word y[], size_t y_words, Sign sign);

     BigInt& sub(const word y[], size_t y_words, Sign sign)
        {
        return add(y, y_words, sign == Positive ? Negative : Positive);
        }

     /**
     * Multiply this with y
     * @param y the BigInt to multiply with this
     * @param ws a temp workspace
     */
     BigInt& mul(const BigInt& y, secure_vector<word>& ws);

     /**
     * Square value of *this
     * @param ws a temp workspace
     */
     BigInt& square(secure_vector<word>& ws);

     /**
     * Set *this to y - *this
     * @param y the BigInt to subtract from as a sequence of words
     * @param y_words length of y in words
     * @param ws a temp workspace
     */
     BigInt& rev_sub(const word y[], size_t y_words, secure_vector<word>& ws);

     /**
     * Set *this to (*this + y) % mod
     * This function assumes *this is >= 0 && < mod
     * @param y the BigInt to add - assumed y >= 0 and y < mod
     * @param mod the positive modulus
     * @param ws a temp workspace
     */
     BigInt& mod_add(const BigInt& y, const BigInt& mod, secure_vector<word>& ws);

     /**
     * Set *this to (*this - y) % mod
     * This function assumes *this is >= 0 && < mod
     * @param y the BigInt to subtract - assumed y >= 0 and y < mod
     * @param mod the positive modulus
     * @param ws a temp workspace
     */
     BigInt& mod_sub(const BigInt& y, const BigInt& mod, secure_vector<word>& ws);

     /**
     * Set *this to (*this * y) % mod
     * This function assumes *this is >= 0 && < mod
     * y should be small, less than 16
     * @param y the small integer to multiply by
     * @param mod the positive modulus
     * @param ws a temp workspace
     */
     BigInt& mod_mul(uint8_t y, const BigInt& mod, secure_vector<word>& ws);

     /**
     * Return *this % mod
     *
     * Assumes that *this is (if anything) only slightly larger than
     * mod and performs repeated subtractions. It should not be used if
     * *this is much larger than mod, instead use modulo operator.
     */
     size_t reduce_below(const BigInt& mod, secure_vector<word> &ws);

     /**
     * Return *this % mod
     *
     * Assumes that *this is (if anything) only slightly larger than mod and
     * performs repeated subtractions. It should not be used if *this is much
     * larger than mod, instead use modulo operator.
     *
     * Performs exactly bound subtractions, so if *this is >= bound*mod then the
     * result will not be fully reduced. If bound is zero, nothing happens.
     */
     void ct_reduce_below(const BigInt& mod, secure_vector<word> &ws, size_t bound);

     /**
     * Zeroize the BigInt. The size of the underlying register is not
     * modified.
     */
     void clear() { m_data.set_to_zero(); m_signedness = Positive; }

     /**
     * Compare this to another BigInt
     * @param n the BigInt value to compare with
     * @param check_signs include sign in comparison?
     * @result if (this<n) return -1, if (this>n) return 1, if both
     * values are identical return 0 [like Perl's <=> operator]
     */
     int32_t cmp(const BigInt& n, bool check_signs = true) const;

     /**
     * Compare this to another BigInt
     * @param n the BigInt value to compare with
     * @result true if this == n or false otherwise
     */
     bool is_equal(const BigInt& n) const;

     /**
     * Compare this to another BigInt
     * @param n the BigInt value to compare with
     * @result true if this < n or false otherwise
     */
     bool is_less_than(const BigInt& n) const;

     /**
     * Compare this to an integer
     * @param n the value to compare with
     * @result if (this<n) return -1, if (this>n) return 1, if both
     * values are identical return 0 [like Perl's <=> operator]
     */
     int32_t cmp_word(word n) const;

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
        return (sig_words() == 0);
        }

     /**
     * Set bit at specified position
     * @param n bit position to set
     */
     void set_bit(size_t n)
        {
        conditionally_set_bit(n, true);
        }

     /**
     * Conditionally set bit at specified position. Note if set_it is
     * false, nothing happens, and if the bit is already set, it
     * remains set.
     *
     * @param n bit position to set
     * @param set_it if the bit should be set
     */
     void conditionally_set_bit(size_t n, bool set_it);

     /**
     * Clear bit at specified position
     * @param n bit position to clear
     */
     void clear_bit(size_t n);

     /**
     * Clear all but the lowest n bits
     * @param n amount of bits to keep
     */
     void mask_bits(size_t n)
        {
        m_data.mask_bits(n);
        }

     /**
     * Return bit value at specified position
     * @param n the bit offset to test
     * @result true, if the bit at position n is set, false otherwise
     */
     bool get_bit(size_t n) const
        {
        return ((word_at(n / BOTAN_MP_WORD_BITS) >> (n % BOTAN_MP_WORD_BITS)) & 1);
        }

     /**
     * Return (a maximum of) 32 bits of the complete value
     * @param offset the offset to start extracting
     * @param length amount of bits to extract (starting at offset)
     * @result the integer extracted from the register starting at
     * offset with specified length
     */
     uint32_t get_substring(size_t offset, size_t length) const;

     /**
     * Convert this value into a uint32_t, if it is in the range
     * [0 ... 2**32-1], or otherwise throw an exception.
     * @result the value as a uint32_t if conversion is possible
     */
     uint32_t to_u32bit() const;

     /**
     * Convert this value to a decimal string.
     * Warning: decimal conversions are relatively slow
     */
     std::string to_dec_string() const;

     /**
     * Convert this value to a hexadecimal string.
     */
     std::string to_hex_string() const;

     /**
     * @param n the offset to get a byte from
     * @result byte at offset n
     */
     uint8_t byte_at(size_t n) const;

     /**
     * Return the word at a specified position of the internal register
     * @param n position in the register
     * @return value at position n
     */
     word word_at(size_t n) const
        {
        return m_data.get_word_at(n);
        }

     void set_word_at(size_t i, word w)
        {
        m_data.set_word_at(i, w);
        }

     void set_words(const word w[], size_t len)
        {
        m_data.set_words(w, len);
        }

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
     Sign sign() const { return (m_signedness); }

     /**
     * @result the opposite sign of the represented integer value
     */
     Sign reverse_sign() const
        {
        if(sign() == Positive)
           return Negative;
        return Positive;
        }

     /**
     * Flip the sign of this BigInt
     */
     void flip_sign()
        {
        set_sign(reverse_sign());
        }

     /**
     * Set sign of the integer
     * @param sign new Sign to set
     */
     void set_sign(Sign sign)
        {
        if(sign == Negative && is_zero())
           sign = Positive;

        m_signedness = sign;
        }

     /**
     * @result absolute (positive) value of this
     */
     BigInt abs() const;

     /**
     * Give size of internal register
     * @result size of internal register in words
     */
     size_t size() const { return m_data.size(); }

     /**
     * Return how many words we need to hold this value
     * @result significant words of the represented integer value
     */
     size_t sig_words() const
        {
        return m_data.sig_words();
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
     * Get the number of high bits unset in the top (allocated) word
     * of this integer. Returns BOTAN_MP_WORD_BITS only iff *this is
     * zero. Ignores sign.
     */
     size_t top_bits_free() const;

     /**
     * Return a mutable pointer to the register
     * @result a pointer to the start of the internal register
     */
     word* mutable_data() { return m_data.mutable_data(); }

     /**
     * Return a const pointer to the register
     * @result a pointer to the start of the internal register
     */
     const word* data() const { return m_data.const_data(); }

     /**
     * Don't use this function in application code
     */
     secure_vector<word>& get_word_vector() { return m_data.mutable_vector(); }

     /**
     * Don't use this function in application code
     */
     const secure_vector<word>& get_word_vector() const { return m_data.const_vector(); }

     /**
     * Increase internal register buffer to at least n words
     * @param n new size of register
     */
     void grow_to(size_t n) const { m_data.grow_to(n); }

     /**
     * Resize the vector to the minimum word size to hold the integer, or
     * min_size words, whichever is larger
     */
     void BOTAN_DEPRECATED("Use resize if required") shrink_to_fit(size_t min_size = 0)
        {
        m_data.shrink_to_fit(min_size);
        }

     void resize(size_t s) { m_data.resize(s); }

     /**
     * Fill BigInt with a random number with size of bitsize
     *
     * If \p set_high_bit is true, the highest bit will be set, which causes
     * the entropy to be \a bits-1. Otherwise the highest bit is randomly chosen
     * by the rng, causing the entropy to be \a bits.
     *
     * @param rng the random number generator to use
     * @param bitsize number of bits the created random value should have
     * @param set_high_bit if true, the highest bit is always set
     */
     void randomize(RandomNumberGenerator& rng, size_t bitsize, bool set_high_bit = true);

     /**
     * Store BigInt-value in a given byte array
     * @param buf destination byte array for the integer value
     */
     void binary_encode(uint8_t buf[]) const;

     /**
     * Store BigInt-value in a given byte array. If len is less than
     * the size of the value, then it will be truncated. If len is
     * greater than the size of the value, it will be zero-padded.
     * If len exactly equals this->bytes(), this function behaves identically
     * to binary_encode.
     *
     * @param buf destination byte array for the integer value
     * @param len how many bytes to write
     */
     void binary_encode(uint8_t buf[], size_t len) const;

     /**
     * Read integer value from a byte array with given size
     * @param buf byte array buffer containing the integer
     * @param length size of buf
     */
     void binary_decode(const uint8_t buf[], size_t length);

     /**
     * Read integer value from a byte vector
     * @param buf the vector to load from
     */
     template<typename Alloc>
     void binary_decode(const std::vector<uint8_t, Alloc>& buf)
        {
        binary_decode(buf.data(), buf.size());
        }

     /**
     * @param base the base to measure the size for
     * @return size of this integer in base base
     *
     * Deprecated. This is only needed when using the `encode` and
     * `encode_locked` functions, which are also deprecated.
     */
     BOTAN_DEPRECATED("See comments on declaration")
     size_t encoded_size(Base base = Binary) const;

     /**
     * Place the value into out, zero-padding up to size words
     * Throw if *this cannot be represented in size words
     */
     void encode_words(word out[], size_t size) const;

     /**
     * If predicate is true assign other to *this
     * Uses a masked operation to avoid side channels
     */
     void ct_cond_assign(bool predicate, const BigInt& other);

     /**
     * If predicate is true swap *this and other
     * Uses a masked operation to avoid side channels
     */
     void ct_cond_swap(bool predicate, BigInt& other);

     /**
     * If predicate is true add value to *this
     */
     void ct_cond_add(bool predicate, const BigInt& value);

     /**
     * If predicate is true flip the sign of *this
     */
     void cond_flip_sign(bool predicate);

#if defined(BOTAN_HAS_VALGRIND)
     void const_time_poison() const;
     void const_time_unpoison() const;
#else
     void const_time_poison() const {}
     void const_time_unpoison() const {}
#endif

     /**
     * @param rng a random number generator
     * @param min the minimum value (must be non-negative)
     * @param max the maximum value (must be non-negative and > min)
     * @return random integer in [min,max)
     */
     static BigInt random_integer(RandomNumberGenerator& rng,
                                  const BigInt& min,
                                  const BigInt& max);

     /**
     * Create a power of two
     * @param n the power of two to create
     * @return bigint representing 2^n
     */
     static BigInt power_of_2(size_t n)
        {
        BigInt b;
        b.set_bit(n);
        return b;
        }

     /**
     * Encode the integer value from a BigInt to a std::vector of bytes
     * @param n the BigInt to use as integer source
     * @result secure_vector of bytes containing the bytes of the integer
     */
     static std::vector<uint8_t> encode(const BigInt& n)
        {
        std::vector<uint8_t> output(n.bytes());
        n.binary_encode(output.data());
        return output;
        }

     /**
     * Encode the integer value from a BigInt to a secure_vector of bytes
     * @param n the BigInt to use as integer source
     * @result secure_vector of bytes containing the bytes of the integer
     */
     static secure_vector<uint8_t> encode_locked(const BigInt& n)
        {
        secure_vector<uint8_t> output(n.bytes());
        n.binary_encode(output.data());
        return output;
        }

     /**
     * Encode the integer value from a BigInt to a byte array
     * @param buf destination byte array for the encoded integer
     * @param n the BigInt to use as integer source
     */
     static BOTAN_DEPRECATED("Use n.binary_encode") void encode(uint8_t buf[], const BigInt& n)
        {
        n.binary_encode(buf);
        }

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @param length size of buf
     * @result BigInt representing the integer in the byte array
     */
     static BigInt decode(const uint8_t buf[], size_t length)
        {
        return BigInt(buf, length);
        }

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @result BigInt representing the integer in the byte array
     */
     template<typename Alloc>
     static BigInt decode(const std::vector<uint8_t, Alloc>& buf)
        {
        return BigInt(buf);
        }

     /**
     * Encode the integer value from a BigInt to a std::vector of bytes
     * @param n the BigInt to use as integer source
     * @param base number-base of resulting byte array representation
     * @result secure_vector of bytes containing the integer with given base
     *
     * Deprecated. If you need Binary, call the version of encode that doesn't
     * take a Base. If you need Hex or Decimal output, use to_hex_string or
     * to_dec_string resp.
     */
     BOTAN_DEPRECATED("See comments on declaration")
     static std::vector<uint8_t> encode(const BigInt& n, Base base);

     /**
     * Encode the integer value from a BigInt to a secure_vector of bytes
     * @param n the BigInt to use as integer source
     * @param base number-base of resulting byte array representation
     * @result secure_vector of bytes containing the integer with given base
     *
     * Deprecated. If you need Binary, call the version of encode_locked that
     * doesn't take a Base. If you need Hex or Decimal output, use to_hex_string
     * or to_dec_string resp.
     */
     BOTAN_DEPRECATED("See comments on declaration")
     static secure_vector<uint8_t> encode_locked(const BigInt& n,
                                                 Base base);

     /**
     * Encode the integer value from a BigInt to a byte array
     * @param buf destination byte array for the encoded integer
     * value with given base
     * @param n the BigInt to use as integer source
     * @param base number-base of resulting byte array representation
     *
     * Deprecated. If you need Binary, call binary_encode. If you need
     * Hex or Decimal output, use to_hex_string or to_dec_string resp.
     */
     BOTAN_DEPRECATED("See comments on declaration")
     static void encode(uint8_t buf[], const BigInt& n, Base base);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @param length size of buf
     * @param base number-base of the integer in buf
     * @result BigInt representing the integer in the byte array
     */
     static BigInt decode(const uint8_t buf[], size_t length,
                          Base base);

     /**
     * Create a BigInt from an integer in a byte array
     * @param buf the binary value to load
     * @param base number-base of the integer in buf
     * @result BigInt representing the integer in the byte array
     */
     template<typename Alloc>
     static BigInt decode(const std::vector<uint8_t, Alloc>& buf, Base base)
        {
        if(base == Binary)
           return BigInt(buf);
        return BigInt::decode(buf.data(), buf.size(), base);
        }

     /**
     * Encode a BigInt to a byte array according to IEEE 1363
     * @param n the BigInt to encode
     * @param bytes the length of the resulting secure_vector<uint8_t>
     * @result a secure_vector<uint8_t> containing the encoded BigInt
     */
     static secure_vector<uint8_t> encode_1363(const BigInt& n, size_t bytes);

     static void encode_1363(uint8_t out[], size_t bytes, const BigInt& n);

     /**
     * Encode two BigInt to a byte array according to IEEE 1363
     * @param n1 the first BigInt to encode
     * @param n2 the second BigInt to encode
     * @param bytes the length of the encoding of each single BigInt
     * @result a secure_vector<uint8_t> containing the concatenation of the two encoded BigInt
     */
     static secure_vector<uint8_t> encode_fixed_length_int_pair(const BigInt& n1, const BigInt& n2, size_t bytes);

     /**
     * Set output = vec[idx].m_reg in constant time
     *
     * All elements of vec must have the same size, and output must be
     * pre-allocated with the same size.
     */
     static void BOTAN_DEPRECATED("No longer in use") const_time_lookup(
        secure_vector<word>& output,
        const std::vector<BigInt>& vec,
        size_t idx);

   private:

     class Data
        {
        public:
           word* mutable_data()
              {
              invalidate_sig_words();
              return m_reg.data();
              }

           const word* const_data() const
              {
              return m_reg.data();
              }

           secure_vector<word>& mutable_vector()
              {
              invalidate_sig_words();
              return m_reg;
              }

           const secure_vector<word>& const_vector() const
              {
              return m_reg;
              }

           word get_word_at(size_t n) const
              {
              if(n < m_reg.size())
                 return m_reg[n];
              return 0;
              }

           void set_word_at(size_t i, word w)
              {
              invalidate_sig_words();
              if(i >= m_reg.size())
                 {
                 if(w == 0)
                    return;
                 grow_to(i + 1);
                 }
              m_reg[i] = w;
              }

           void set_words(const word w[], size_t len)
              {
              invalidate_sig_words();
              m_reg.assign(w, w + len);
              }

           void set_to_zero()
              {
              m_reg.resize(m_reg.capacity());
              clear_mem(m_reg.data(), m_reg.size());
              m_sig_words = 0;
              }

           void set_size(size_t s)
              {
              invalidate_sig_words();
              clear_mem(m_reg.data(), m_reg.size());
              m_reg.resize(s + (8 - (s % 8)));
              }

           void mask_bits(size_t n)
              {
              if(n == 0) { return set_to_zero(); }

              const size_t top_word = n / BOTAN_MP_WORD_BITS;

              // if(top_word < sig_words()) ?
              if(top_word < size())
                 {
                 const word mask = (static_cast<word>(1) << (n % BOTAN_MP_WORD_BITS)) - 1;
                 const size_t len = size() - (top_word + 1);
                 if(len > 0)
                    {
                    clear_mem(&m_reg[top_word+1], len);
                    }
                 m_reg[top_word] &= mask;
                 invalidate_sig_words();
                 }
              }

           void grow_to(size_t n) const
              {
              if(n > size())
                 {
                 if(n <= m_reg.capacity())
                    m_reg.resize(n);
                 else
                    m_reg.resize(n + (8 - (n % 8)));
                 }
              }

           size_t size() const { return m_reg.size(); }

           void shrink_to_fit(size_t min_size = 0)
              {
              const size_t words = std::max(min_size, sig_words());
              m_reg.resize(words);
              }

           void resize(size_t s)
              {
              m_reg.resize(s);
              }

           void swap(Data& other)
              {
              m_reg.swap(other.m_reg);
              std::swap(m_sig_words, other.m_sig_words);
              }

           void swap(secure_vector<word>& reg)
              {
              m_reg.swap(reg);
              invalidate_sig_words();
              }

           void invalidate_sig_words() const
              {
              m_sig_words = sig_words_npos;
              }

           size_t sig_words() const
              {
              if(m_sig_words == sig_words_npos)
                 {
                 m_sig_words = calc_sig_words();
                 }
              else
                 {
                 BOTAN_DEBUG_ASSERT(m_sig_words == calc_sig_words());
                 }
              return m_sig_words;
              }
        private:
           static const size_t sig_words_npos = static_cast<size_t>(-1);

           size_t calc_sig_words() const;

           mutable secure_vector<word> m_reg;
           mutable size_t m_sig_words = sig_words_npos;
        };

      Data m_data;
      Sign m_signedness = Positive;
   };

/*
* Arithmetic Operators
*/
inline BigInt operator+(const BigInt& x, const BigInt& y)
   {
   return BigInt::add2(x, y.data(), y.sig_words(), y.sign());
   }

inline BigInt operator+(const BigInt& x, word y)
   {
   return BigInt::add2(x, &y, 1, BigInt::Positive);
   }

inline BigInt operator+(word x, const BigInt& y)
   {
   return y + x;
   }

inline BigInt operator-(const BigInt& x, const BigInt& y)
   {
   return BigInt::add2(x, y.data(), y.sig_words(), y.reverse_sign());
   }

inline BigInt operator-(const BigInt& x, word y)
   {
   return BigInt::add2(x, &y, 1, BigInt::Negative);
   }

BigInt BOTAN_PUBLIC_API(2,0) operator*(const BigInt& x, const BigInt& y);
BigInt BOTAN_PUBLIC_API(2,8) operator*(const BigInt& x, word y);
inline BigInt operator*(word x, const BigInt& y) { return y*x; }

BigInt BOTAN_PUBLIC_API(2,0) operator/(const BigInt& x, const BigInt& d);
BigInt BOTAN_PUBLIC_API(2,0) operator/(const BigInt& x, word m);
BigInt BOTAN_PUBLIC_API(2,0) operator%(const BigInt& x, const BigInt& m);
word   BOTAN_PUBLIC_API(2,0) operator%(const BigInt& x, word m);
BigInt BOTAN_PUBLIC_API(2,0) operator<<(const BigInt& x, size_t n);
BigInt BOTAN_PUBLIC_API(2,0) operator>>(const BigInt& x, size_t n);

/*
* Comparison Operators
*/
inline bool operator==(const BigInt& a, const BigInt& b)
   { return a.is_equal(b); }
inline bool operator!=(const BigInt& a, const BigInt& b)
   { return !a.is_equal(b); }
inline bool operator<=(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) <= 0); }
inline bool operator>=(const BigInt& a, const BigInt& b)
   { return (a.cmp(b) >= 0); }
inline bool operator<(const BigInt& a, const BigInt& b)
   { return a.is_less_than(b); }
inline bool operator>(const BigInt& a, const BigInt& b)
   { return b.is_less_than(a); }

inline bool operator==(const BigInt& a, word b)
   { return (a.cmp_word(b) == 0); }
inline bool operator!=(const BigInt& a, word b)
   { return (a.cmp_word(b) != 0); }
inline bool operator<=(const BigInt& a, word b)
   { return (a.cmp_word(b) <= 0); }
inline bool operator>=(const BigInt& a, word b)
   { return (a.cmp_word(b) >= 0); }
inline bool operator<(const BigInt& a, word b)
   { return (a.cmp_word(b) < 0); }
inline bool operator>(const BigInt& a, word b)
   { return (a.cmp_word(b) > 0); }

/*
* I/O Operators
*/
BOTAN_PUBLIC_API(2,0) std::ostream& operator<<(std::ostream&, const BigInt&);
BOTAN_PUBLIC_API(2,0) std::istream& operator>>(std::istream&, BigInt&);

}

namespace std {

template<>
inline void swap<Botan::BigInt>(Botan::BigInt& x, Botan::BigInt& y)
   {
   x.swap(y);
   }

}

#endif

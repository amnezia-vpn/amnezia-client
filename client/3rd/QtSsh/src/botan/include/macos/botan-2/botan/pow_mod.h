/*
* Modular Exponentiator
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_POWER_MOD_H_
#define BOTAN_POWER_MOD_H_

#include <botan/bigint.h>

BOTAN_FUTURE_INTERNAL_HEADER(pow_mod.h)

namespace Botan {

class Modular_Exponentiator;

/**
* Modular Exponentiator Proxy
*/
class BOTAN_PUBLIC_API(2,0) Power_Mod
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

      /**
      * @param modulus the modulus
      * @param hints Passed to set_modulus if modulus > 0
      * @param disable_montgomery_arith Disables use of Montgomery
      * representation. Likely only useful for testing.
      */
      void set_modulus(const BigInt& modulus,
                       Usage_Hints hints = NO_HINTS,
                       bool disable_montgomery_arith = false) const;

      /**
      * Set the base
      */
      void set_base(const BigInt& base) const;

      /**
      * Set the exponent
      */
      void set_exponent(const BigInt& exponent) const;

      /**
      * All three of the above functions must have already been called.
      * @return result of g^x%p
      */
      BigInt execute() const;

      Power_Mod& operator=(const Power_Mod&);

      /**
      * @param modulus Optionally call set_modulus
      * @param hints Passed to set_modulus if modulus > 0
      * @param disable_montgomery_arith Disables use of Montgomery
      * representation. Likely only useful for testing.
      */
      Power_Mod(const BigInt& modulus = 0,
                Usage_Hints hints = NO_HINTS,
                bool disable_montgomery_arith = false);
      Power_Mod(const Power_Mod&);
      virtual ~Power_Mod();
   private:
      mutable std::unique_ptr<Modular_Exponentiator> m_core;
   };

/**
* Fixed Exponent Modular Exponentiator Proxy
*/
class BOTAN_PUBLIC_API(2,0) Fixed_Exponent_Power_Mod final : public Power_Mod
   {
   public:
      BigInt operator()(const BigInt& b) const
         { set_base(b); return execute(); }

      Fixed_Exponent_Power_Mod() = default;

      Fixed_Exponent_Power_Mod(const BigInt& exponent,
                               const BigInt& modulus,
                               Usage_Hints hints = NO_HINTS);
   };

/**
* Fixed Base Modular Exponentiator Proxy
*/
class BOTAN_PUBLIC_API(2,0) Fixed_Base_Power_Mod final : public Power_Mod
   {
   public:
      BigInt operator()(const BigInt& e) const
         { set_exponent(e); return execute(); }

      Fixed_Base_Power_Mod() = default;

      Fixed_Base_Power_Mod(const BigInt& base,
                           const BigInt& modulus,
                           Usage_Hints hints = NO_HINTS);
   };

}

#endif

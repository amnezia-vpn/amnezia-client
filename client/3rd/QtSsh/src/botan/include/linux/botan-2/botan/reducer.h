/*
* Modular Reducer
* (C) 1999-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MODULAR_REDUCER_H_
#define BOTAN_MODULAR_REDUCER_H_

#include <botan/numthry.h>

namespace Botan {

/**
* Modular Reducer (using Barrett's technique)
*/
class BOTAN_PUBLIC_API(2,0) Modular_Reducer
   {
   public:
      const BigInt& get_modulus() const { return m_modulus; }

      BigInt reduce(const BigInt& x) const;

      /**
      * Multiply mod p
      * @param x the first operand
      * @param y the second operand
      * @return (x * y) % p
      */
      BigInt multiply(const BigInt& x, const BigInt& y) const
         { return reduce(x * y); }

      /**
      * Square mod p
      * @param x the value to square
      * @return (x * x) % p
      */
      BigInt square(const BigInt& x) const
         { return reduce(Botan::square(x)); }

      /**
      * Cube mod p
      * @param x the value to cube
      * @return (x * x * x) % p
      */
      BigInt cube(const BigInt& x) const
         { return multiply(x, this->square(x)); }

      /**
      * Low level reduction function. Mostly for internal use.
      * Sometimes useful for performance by reducing temporaries
      * Reduce x mod p and place the output in out. ** X and out must not reference each other **
      * ws is a temporary workspace.
      */
      void reduce(BigInt& out, const BigInt& x, secure_vector<word>& ws) const;

      bool initialized() const { return (m_mod_words != 0); }

      Modular_Reducer() { m_mod_words = 0; }
      explicit Modular_Reducer(const BigInt& mod);
   private:
      BigInt m_modulus, m_mu;
      size_t m_mod_words;
   };

}

#endif

/*
 * (C) Copyright Projet SECRET, INRIA, Rocquencourt
 * (C) Bhaskar Biswas and  Nicolas Sendrier
 *
 * (C) 2014 cryptosource GmbH
 * (C) 2014 Falko Strenzke fstrenzke@cryptosource.de
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 *
 */

#ifndef BOTAN_GF2M_SMALL_M_H_
#define BOTAN_GF2M_SMALL_M_H_

#include <botan/types.h>
#include <vector>

BOTAN_FUTURE_INTERNAL_HEADER(gf2m_small_m.h)

namespace Botan {

typedef uint16_t gf2m;

/**
* GF(2^m) field for m = [2...16]
*/
class BOTAN_PUBLIC_API(2,0) GF2m_Field
   {
   public:
      explicit GF2m_Field(size_t extdeg);

      gf2m gf_mul(gf2m x, gf2m y) const
         {
         return ((x) ? gf_mul_fast(x, y) : 0);
         }

      gf2m gf_square(gf2m x) const
         {
         return ((x) ? gf_exp(_gf_modq_1(gf_log(x) << 1)) : 0);
         }

      gf2m square_rr(gf2m x) const
         {
         return _gf_modq_1(x << 1);
         }

      gf2m gf_mul_fast(gf2m x, gf2m y) const
         {
         return ((y) ? gf_exp(_gf_modq_1(gf_log(x) + gf_log(y))) : 0);
         }

      /*
      naming convention of GF(2^m) field operations:
        l logarithmic, unreduced
        r logarithmic, reduced
        n normal, non-zero
        z normal, might be zero
      */

      gf2m gf_mul_lll(gf2m a, gf2m b) const
         {
         return (a + b);
         }

      gf2m gf_mul_rrr(gf2m a, gf2m b) const
         {
         return (_gf_modq_1(gf_mul_lll(a, b)));
         }

      gf2m gf_mul_nrr(gf2m a, gf2m b) const
         {
         return (gf_exp(gf_mul_rrr(a, b)));
         }

      gf2m gf_mul_rrn(gf2m a, gf2m y) const
         {
         return _gf_modq_1(gf_mul_lll(a, gf_log(y)));
         }

      gf2m gf_mul_rnr(gf2m y, gf2m a) const
         {
         return gf_mul_rrn(a, y);
         }

      gf2m gf_mul_lnn(gf2m x, gf2m y) const
         {
         return (gf_log(x) + gf_log(y));
         }

      gf2m gf_mul_rnn(gf2m x, gf2m y) const
         {
         return _gf_modq_1(gf_mul_lnn(x, y));
         }

      gf2m gf_mul_nrn(gf2m a, gf2m y) const
         {
         return gf_exp(_gf_modq_1((a) + gf_log(y)));
         }

      /**
      * zero operand allowed
      */
      gf2m gf_mul_zrz(gf2m a, gf2m y) const
         {
         return ( (y == 0) ? 0 : gf_mul_nrn(a, y) );
         }

      gf2m gf_mul_zzr(gf2m a, gf2m y) const
         {
         return gf_mul_zrz(y, a);
         }

      /**
      * non-zero operand
      */
      gf2m gf_mul_nnr(gf2m y, gf2m a) const
         {
         return gf_mul_nrn(a, y);
         }

      gf2m gf_sqrt(gf2m x) const
         {
         return ((x) ? gf_exp(_gf_modq_1(gf_log(x) << (get_extension_degree()-1))) : 0);
         }

      gf2m gf_div_rnn(gf2m x, gf2m y) const
         {
         return _gf_modq_1(gf_log(x) - gf_log(y));
         }

      gf2m gf_div_rnr(gf2m x, gf2m b) const
         {
         return _gf_modq_1(gf_log(x) - b);
         }

      gf2m gf_div_nrr(gf2m a, gf2m b) const
         {
         return gf_exp(_gf_modq_1(a - b));
         }

      gf2m gf_div_zzr(gf2m x, gf2m b) const
         {
         return ((x) ? gf_exp(_gf_modq_1(gf_log(x) - b)) : 0);
         }

      gf2m gf_inv(gf2m x) const
         {
         return gf_exp(gf_ord() - gf_log(x));
         }

      gf2m gf_inv_rn(gf2m x) const
         {
         return (gf_ord() - gf_log(x));
         }

      gf2m gf_square_ln(gf2m x) const
         {
         return gf_log(x) << 1;
         }

      gf2m gf_square_rr(gf2m a) const
         {
         return a << 1;
         }

      gf2m gf_l_from_n(gf2m x) const
         {
         return gf_log(x);
         }

      gf2m gf_div(gf2m x, gf2m y) const;

      gf2m gf_exp(gf2m i) const
         {
         return m_gf_exp_table.at(i); /* alpha^i */
         }

      gf2m gf_log(gf2m i) const
         {
         return m_gf_log_table.at(i); /* return i when x=alpha^i */
         }

      gf2m gf_ord() const
         {
         return m_gf_multiplicative_order;
         }

      size_t get_extension_degree() const
         {
         return m_gf_extension_degree;
         }

      gf2m get_cardinality() const
         {
         return static_cast<gf2m>(1 << get_extension_degree());
         }

   private:
      gf2m _gf_modq_1(int32_t d) const
         {
         /* residual modulo q-1
         when -q < d < 0, we get (q-1+d)
         when 0 <= d < q, we get (d)
         when q <= d < 2q-1, we get (d-q+1)
         */
         return static_cast<gf2m>(((d) & gf_ord()) + ((d) >> get_extension_degree()));
         }

      const size_t m_gf_extension_degree;
      const gf2m m_gf_multiplicative_order;
      const std::vector<gf2m>& m_gf_log_table;
      const std::vector<gf2m>& m_gf_exp_table;
   };

uint32_t encode_gf2m(gf2m to_enc, uint8_t* mem);

gf2m decode_gf2m(const uint8_t* mem);

}

#endif

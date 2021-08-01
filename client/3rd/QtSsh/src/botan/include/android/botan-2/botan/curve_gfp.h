/*
* Elliptic curves over GF(p)
*
* (C) 2007 Martin Doering, Christoph Ludwig, Falko Strenzke
*     2010-2011,2012,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_GFP_CURVE_H_
#define BOTAN_GFP_CURVE_H_

#include <botan/bigint.h>
#include <memory>

// Currently exposed in PointGFp
//BOTAN_FUTURE_INTERNAL_HEADER(curve_gfp.h)

namespace Botan {

class BOTAN_UNSTABLE_API CurveGFp_Repr
   {
   public:
      virtual ~CurveGFp_Repr() = default;

      virtual const BigInt& get_p() const = 0;
      virtual const BigInt& get_a() const = 0;
      virtual const BigInt& get_b() const = 0;

      virtual size_t get_p_words() const = 0;

      virtual size_t get_ws_size() const = 0;

      virtual bool is_one(const BigInt& x) const = 0;

      virtual bool a_is_zero() const = 0;

      virtual bool a_is_minus_3() const = 0;

      /*
      * Returns to_curve_rep(get_a())
      */
      virtual const BigInt& get_a_rep() const = 0;

      /*
      * Returns to_curve_rep(get_b())
      */
      virtual const BigInt& get_b_rep() const = 0;

      /*
      * Returns to_curve_rep(1)
      */
      virtual const BigInt& get_1_rep() const = 0;

      virtual BigInt invert_element(const BigInt& x, secure_vector<word>& ws) const = 0;

      virtual void to_curve_rep(BigInt& x, secure_vector<word>& ws) const = 0;

      virtual void from_curve_rep(BigInt& x, secure_vector<word>& ws) const = 0;

      void curve_mul(BigInt& z, const BigInt& x, const BigInt& y,
                     secure_vector<word>& ws) const
         {
         BOTAN_DEBUG_ASSERT(x.sig_words() <= get_p_words());
         curve_mul_words(z, x.data(), x.size(), y, ws);
         }

      virtual void curve_mul_words(BigInt& z,
                                   const word x_words[],
                                   const size_t x_size,
                                   const BigInt& y,
                                   secure_vector<word>& ws) const = 0;

      void curve_sqr(BigInt& z, const BigInt& x,
                             secure_vector<word>& ws) const
         {
         BOTAN_DEBUG_ASSERT(x.sig_words() <= get_p_words());
         curve_sqr_words(z, x.data(), x.size(), ws);
         }

      virtual void curve_sqr_words(BigInt& z,
                                   const word x_words[],
                                   size_t x_size,
                                   secure_vector<word>& ws) const = 0;
   };

/**
* This class represents an elliptic curve over GF(p)
*
* There should not be any reason for applications to use this type.
* If you need EC primitives use the interfaces EC_Group and PointGFp
*
* It is likely this class will be removed entirely in a future major
* release.
*/
class BOTAN_UNSTABLE_API CurveGFp final
   {
   public:

      /**
      * Create an uninitialized CurveGFp
      */
      CurveGFp() = default;

      /**
      * Construct the elliptic curve E: y^2 = x^3 + ax + b over GF(p)
      * @param p prime number of the field
      * @param a first coefficient
      * @param b second coefficient
      */
      CurveGFp(const BigInt& p, const BigInt& a, const BigInt& b) :
         m_repr(choose_repr(p, a, b))
         {
         }

      CurveGFp(const CurveGFp&) = default;

      CurveGFp& operator=(const CurveGFp&) = default;

      /**
      * @return curve coefficient a
      */
      const BigInt& get_a() const { return m_repr->get_a(); }

      /**
      * @return curve coefficient b
      */
      const BigInt& get_b() const { return m_repr->get_b(); }

      /**
      * Get prime modulus of the field of the curve
      * @return prime modulus of the field of the curve
      */
      const BigInt& get_p() const { return m_repr->get_p(); }

      size_t get_p_words() const { return m_repr->get_p_words(); }

      size_t get_ws_size() const { return m_repr->get_ws_size(); }

      const BigInt& get_a_rep() const { return m_repr->get_a_rep(); }

      const BigInt& get_b_rep() const { return m_repr->get_b_rep(); }

      const BigInt& get_1_rep() const { return m_repr->get_1_rep(); }

      bool a_is_minus_3() const { return m_repr->a_is_minus_3(); }
      bool a_is_zero() const { return m_repr->a_is_zero(); }

      bool is_one(const BigInt& x) const { return m_repr->is_one(x); }

      BigInt invert_element(const BigInt& x, secure_vector<word>& ws) const
         {
         return m_repr->invert_element(x, ws);
         }

      void to_rep(BigInt& x, secure_vector<word>& ws) const
         {
         m_repr->to_curve_rep(x, ws);
         }

      void from_rep(BigInt& x, secure_vector<word>& ws) const
         {
         m_repr->from_curve_rep(x, ws);
         }

      BigInt from_rep_to_tmp(const BigInt& x, secure_vector<word>& ws) const
         {
         BigInt xt(x);
         m_repr->from_curve_rep(xt, ws);
         return xt;
         }

      // TODO: from_rep taking && ref

      void mul(BigInt& z, const BigInt& x, const BigInt& y, secure_vector<word>& ws) const
         {
         m_repr->curve_mul(z, x, y, ws);
         }

      void mul(BigInt& z, const word x_w[], size_t x_size,
               const BigInt& y, secure_vector<word>& ws) const
         {
         m_repr->curve_mul_words(z, x_w, x_size, y, ws);
         }

      void sqr(BigInt& z, const BigInt& x, secure_vector<word>& ws) const
         {
         m_repr->curve_sqr(z, x, ws);
         }

      void sqr(BigInt& z, const word x_w[], size_t x_size, secure_vector<word>& ws) const
         {
         m_repr->curve_sqr_words(z, x_w, x_size, ws);
         }

      BigInt mul(const BigInt& x, const BigInt& y, secure_vector<word>& ws) const
         {
         return mul_to_tmp(x, y, ws);
         }

      BigInt sqr(const BigInt& x, secure_vector<word>& ws) const
         {
         return sqr_to_tmp(x, ws);
         }

      BigInt mul_to_tmp(const BigInt& x, const BigInt& y, secure_vector<word>& ws) const
         {
         BigInt z;
         m_repr->curve_mul(z, x, y, ws);
         return z;
         }

      BigInt sqr_to_tmp(const BigInt& x, secure_vector<word>& ws) const
         {
         BigInt z;
         m_repr->curve_sqr(z, x, ws);
         return z;
         }

      void swap(CurveGFp& other)
         {
         std::swap(m_repr, other.m_repr);
         }

      /**
      * Equality operator
      * @param other a curve
      * @return true iff *this is the same as other
      */
      inline bool operator==(const CurveGFp& other) const
         {
         if(m_repr.get() == other.m_repr.get())
            return true;

         return (get_p() == other.get_p()) &&
                (get_a() == other.get_a()) &&
                (get_b() == other.get_b());
         }

   private:
      static std::shared_ptr<CurveGFp_Repr>
         choose_repr(const BigInt& p, const BigInt& a, const BigInt& b);

      std::shared_ptr<CurveGFp_Repr> m_repr;
   };

inline bool operator!=(const CurveGFp& lhs, const CurveGFp& rhs)
   {
   return !(lhs == rhs);
   }

}

namespace std {

template<> inline
void swap<Botan::CurveGFp>(Botan::CurveGFp& curve1,
                           Botan::CurveGFp& curve2) noexcept
   {
   curve1.swap(curve2);
   }

} // namespace std

#endif

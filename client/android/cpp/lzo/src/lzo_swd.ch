/* lzo_swd.ch -- sliding window dictionary

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


#if (LZO_UINT_MAX < LZO_0xffffffffL)
#  error "LZO_UINT_MAX"
#endif
#if defined(LZO_DEBUG)
#  include <stdio.h>
#endif
#if defined(__LZO_CHECKER)
#  include <stdlib.h>
#endif


/***********************************************************************
//
************************************************************************/

/* unsigned type for dictionary access - don't waste memory here */
#if (0UL + SWD_N + SWD_F + SWD_F < 65535UL)
   typedef lzo_uint16_t     swd_uint;
#  define SWD_UINT_MAX      0xffffu
#else
   typedef lzo_uint32_t     swd_uint;
#  define SWD_UINT_MAX      0xffffffffu
#endif
#define swd_uintp           swd_uint *
#define SWD_UINT(x)         ((swd_uint)(x))


#ifndef SWD_HSIZE
#  define SWD_HSIZE         16384
#endif
#ifndef SWD_MAX_CHAIN
#  define SWD_MAX_CHAIN     2048
#endif

#if !defined(HEAD3)
#if 1
#  define HEAD3(b,p) \
    ((DMUL(0x9f5f,(((((lzo_xint)b[p]<<5)^b[p+1])<<5)^b[p+2]))>>5) & (SWD_HSIZE-1))
#else
#  define HEAD3(b,p) \
    ((DMUL(0x9f5f,(((((lzo_xint)b[p+2]<<5)^b[p+1])<<5)^b[p]))>>5) & (SWD_HSIZE-1))
#endif
#endif

#if !(SWD_NO_HEAD2) && (SWD_THRESHOLD == 1) && !defined(HEAD2)
#  if 1 && (LZO_OPT_UNALIGNED16)
#    define HEAD2(b,p)      UA_GET_NE16((b)+(p))
#  else
#    define HEAD2(b,p)      (b[p] ^ ((unsigned)b[(p)+1]<<8))
#  endif
#  define NIL2              SWD_UINT_MAX
#endif
#ifndef IF_HEAD2
#define IF_HEAD2(s)         /*empty*/
#endif


typedef struct
{
/* public - "built-in" */
    lzo_uint swd_n;
    lzo_uint swd_f;
    lzo_uint swd_threshold;

/* public - configuration */
    lzo_uint max_chain;
    lzo_uint nice_length;
    lzo_bool use_best_off;
    lzo_uint lazy_insert;

/* public - output */
    lzo_uint m_len;
    lzo_uint m_off;
    lzo_uint look;
    int b_char;
#if defined(SWD_BEST_OFF)
    lzo_uint best_off[ SWD_BEST_OFF ];
#endif

/* semi public */
    LZO_COMPRESS_T *c;
    lzo_uint m_pos;
#if defined(SWD_BEST_OFF)
    lzo_uint best_pos[ SWD_BEST_OFF ];
#endif

/* private */
    const lzo_bytep dict;
    const lzo_bytep dict_end;
    lzo_uint dict_len;

/* private */
    lzo_uint ip;                /* input pointer (lookahead) */
    lzo_uint bp;                /* buffer pointer */
    lzo_uint rp;                /* remove pointer */
    lzo_uint b_size;

    lzo_bytep b_wrap;

    lzo_uint node_count;
    lzo_uint first_rp;

#if defined(__LZO_CHECKER)
    /* malloc arrays of the exact size to detect any overrun */
    unsigned char *b;
    swd_uint *head3;
    swd_uint *succ3;
    swd_uint *best3;
    swd_uint *llen3;
# ifdef HEAD2
    swd_uint *head2;
# endif

#else
    unsigned char b [ SWD_N + SWD_F + SWD_F ];
    swd_uint head3 [ SWD_HSIZE ];
    swd_uint succ3 [ SWD_N + SWD_F ];
    swd_uint best3 [ SWD_N + SWD_F ];
    swd_uint llen3 [ SWD_HSIZE ];
# ifdef HEAD2
    swd_uint head2 [ 65536L ];
# endif
#endif
}
lzo_swd_t;
#define lzo_swd_p   lzo_swd_t *


#define s_b(s)      s->b
#define s_head3(s)  s->head3
#define s_succ3(s)  s->succ3
#define s_best3(s)  s->best3
#define s_llen3(s)  s->llen3
#ifdef HEAD2
#define s_head2(s)  s->head2
#endif
#define SIZEOF_LZO_SWD_T    (sizeof(lzo_swd_t))


/* Access macro for head3.
 * head3[key] may be uninitialized if the list is emtpy,
 * but then its value will never be used.
 */
#if 1 || defined(__LZO_CHECKER)
#  define s_get_head3(s,key) \
        ((swd_uint)((s_llen3(s)[key] == 0) ? SWD_UINT_MAX : s_head3(s)[key]))
#else
#  define s_get_head3(s,key)    (s_head3(s)[key])
#endif


/***********************************************************************
//
************************************************************************/

static
void swd_initdict(lzo_swd_p s, const lzo_bytep dict, lzo_uint dict_len)
{
    s->dict = s->dict_end = NULL;
    s->dict_len = 0;

    if (!dict || dict_len == 0)
        return;
    if (dict_len > s->swd_n)
    {
        dict += dict_len - s->swd_n;
        dict_len = s->swd_n;
    }

    s->dict = dict;
    s->dict_len = dict_len;
    s->dict_end = dict + dict_len;
    lzo_memcpy(s_b(s),dict,dict_len);
    s->ip = dict_len;
}


static
void swd_insertdict(lzo_swd_p s, lzo_uint node, lzo_uint len)
{
    lzo_uint key;

    s->node_count = s->swd_n - len;
    s->first_rp = node;

    if (len) do
    {
        key = HEAD3(s_b(s),node);
        s_succ3(s)[node] = s_get_head3(s,key);
        s_head3(s)[key] = SWD_UINT(node);
        s_best3(s)[node] = SWD_UINT(s->swd_f + 1);
        s_llen3(s)[key]++;
        assert(s_llen3(s)[key] <= s->swd_n);

#ifdef HEAD2
        IF_HEAD2(s) {
            key = HEAD2(s_b(s),node);
            s_head2(s)[key] = SWD_UINT(node);
        }
#endif

        node++;
    }
    while (--len != 0);
}


/***********************************************************************
//
************************************************************************/

static void swd_exit(lzo_swd_p s);

static
int swd_init(lzo_swd_p s, const lzo_bytep dict, lzo_uint dict_len)
{
#if defined(__LZO_CHECKER)
    unsigned r = 1;
    s->b = (lzo_bytep) malloc(SWD_N + SWD_F + SWD_F);
    s->head3 = (swd_uintp) malloc(sizeof(swd_uint) * SWD_HSIZE);
    s->succ3 = (swd_uintp) malloc(sizeof(swd_uint) * (SWD_N + SWD_F));
    s->best3 = (swd_uintp) malloc(sizeof(swd_uint) * (SWD_N + SWD_F));
    s->llen3 = (swd_uintp) malloc(sizeof(swd_uint) * SWD_HSIZE);
    r &= s->b != NULL;
    r &= s->head3 != NULL;
    r &= s->succ3 != NULL;
    r &= s->best3 != NULL;
    r &= s->llen3 != NULL;
#ifdef HEAD2
    IF_HEAD2(s) {
        s->head2 = (swd_uintp) malloc(sizeof(swd_uint) * 65536L);
        r &= s->head2 != NULL;
    }
#endif
    if (r != 1) {
        swd_exit(s);
        return LZO_E_OUT_OF_MEMORY;
    }
#endif

    s->m_len = 0;
    s->m_off = 0;
#if defined(SWD_BEST_OFF)
    {
        unsigned i;
        for (i = 0; i < SWD_BEST_OFF; i++)
            s->best_off[i] = s->best_pos[i] = 0;
    }
#endif

    s->swd_n = SWD_N;
    s->swd_f = SWD_F;
    s->swd_threshold = SWD_THRESHOLD;

    /* defaults */
    s->max_chain = SWD_MAX_CHAIN;
    s->nice_length = s->swd_f;
    s->use_best_off = 0;
    s->lazy_insert = 0;

    s->b_size = s->swd_n + s->swd_f;
#if 0
    if (2 * s->swd_f >= s->swd_n || s->b_size + s->swd_f >= SWD_UINT_MAX)
        return LZO_E_ERROR;
#else
    LZO_COMPILE_TIME_ASSERT(!(0ul + 2 * SWD_F >= SWD_N))
    LZO_COMPILE_TIME_ASSERT(!(0ul + SWD_N + SWD_F + SWD_F >= SWD_UINT_MAX))
#endif
    s->b_wrap = s_b(s) + s->b_size;
    s->node_count = s->swd_n;

    lzo_memset(s_llen3(s), 0, (lzo_uint)sizeof(s_llen3(s)[0]) * (lzo_uint)SWD_HSIZE);
#ifdef HEAD2
    IF_HEAD2(s) {
#if 1
        lzo_memset(s_head2(s), 0xff, (lzo_uint)sizeof(s_head2(s)[0]) * 65536L);
        assert(s_head2(s)[0] == NIL2);
#else
        lzo_xint i;
        for (i = 0; i < 65536L; i++)
            s_head2(s)[i] = NIL2;
#endif
    }
#endif

    s->ip = 0;
    swd_initdict(s,dict,dict_len);
    s->bp = s->ip;
    s->first_rp = s->ip;

    assert(s->ip + s->swd_f <= s->b_size);
#if 1
    s->look = (lzo_uint) (s->c->in_end - s->c->ip);
    if (s->look > 0)
    {
        if (s->look > s->swd_f)
            s->look = s->swd_f;
        lzo_memcpy(&s_b(s)[s->ip],s->c->ip,s->look);
        s->c->ip += s->look;
        s->ip += s->look;
    }
#else
    s->look = 0;
    while (s->look < s->swd_f)
    {
        int c;
        if ((c = getbyte(*(s->c))) < 0)
            break;
        s_b(s)[s->ip] = LZO_BYTE(c);
        s->ip++;
        s->look++;
    }
#endif
    if (s->ip == s->b_size)
        s->ip = 0;

    if (s->look >= 2 && s->dict_len > 0)
        swd_insertdict(s,0,s->dict_len);

    s->rp = s->first_rp;
    if (s->rp >= s->node_count)
        s->rp -= s->node_count;
    else
        s->rp += s->b_size - s->node_count;

#if 1 || defined(__LZO_CHECKER)
    /* initialize memory for the first few HEAD3 (if s->ip is not far
     * enough ahead to do this job for us). The value doesn't matter. */
    if (s->look < 3) {
        lzo_bytep p = &s_b(s)[s->bp+s->look];
        p[0] = p[1] = p[2] = 0;
    }
#endif

    return LZO_E_OK;
}


static
void swd_exit(lzo_swd_p s)
{
#if defined(__LZO_CHECKER)
    /* free in reverse order of allocations */
#ifdef HEAD2
    free(s->head2); s->head2 = NULL;
#endif
    free(s->llen3); s->llen3 = NULL;
    free(s->best3); s->best3 = NULL;
    free(s->succ3); s->succ3 = NULL;
    free(s->head3); s->head3 = NULL;
    free(s->b); s->b = NULL;
#else
    LZO_UNUSED(s);
#endif
}


#define swd_pos2off(s,pos) \
    (s->bp > (pos) ? s->bp - (pos) : s->b_size - ((pos) - s->bp))


/***********************************************************************
//
************************************************************************/

static __lzo_inline
void swd_getbyte(lzo_swd_p s)
{
    int c;

    if ((c = getbyte(*(s->c))) < 0)
    {
        if (s->look > 0)
            --s->look;
#if 1 || defined(__LZO_CHECKER)
        /* initialize memory - value doesn't matter */
        s_b(s)[s->ip] = 0;
        if (s->ip < s->swd_f)
            s->b_wrap[s->ip] = 0;
#endif
    }
    else
    {
        s_b(s)[s->ip] = LZO_BYTE(c);
        if (s->ip < s->swd_f)
            s->b_wrap[s->ip] = LZO_BYTE(c);
    }
    if (++s->ip == s->b_size)
        s->ip = 0;
    if (++s->bp == s->b_size)
        s->bp = 0;
    if (++s->rp == s->b_size)
        s->rp = 0;
}


/***********************************************************************
// remove node from lists
************************************************************************/

static __lzo_inline
void swd_remove_node(lzo_swd_p s, lzo_uint node)
{
    if (s->node_count == 0)
    {
        lzo_uint key;

#ifdef LZO_DEBUG
        if (s->first_rp != LZO_UINT_MAX)
        {
            if (node != s->first_rp)
                printf("Remove %5ld: %5ld %5ld %5ld %5ld  %6ld %6ld\n",
                        (long)node, (long)s->rp, (long)s->ip, (long)s->bp,
                        (long)s->first_rp, (long)(s->ip - node),
                        (long)(s->ip - s->bp));
            assert(node == s->first_rp);
            s->first_rp = LZO_UINT_MAX;
        }
#endif

        key = HEAD3(s_b(s),node);
        assert(s_llen3(s)[key] > 0);
        --s_llen3(s)[key];

#ifdef HEAD2
        IF_HEAD2(s) {
            key = HEAD2(s_b(s),node);
            assert(s_head2(s)[key] != NIL2);
            if ((lzo_uint) s_head2(s)[key] == node)
                s_head2(s)[key] = NIL2;
        }
#endif
    }
    else
        --s->node_count;
}


/***********************************************************************
//
************************************************************************/

static
void swd_accept(lzo_swd_p s, lzo_uint n)
{
    assert(n <= s->look);

    if (n) do
    {
        lzo_uint key;

        swd_remove_node(s,s->rp);

        /* add bp into HEAD3 */
        key = HEAD3(s_b(s),s->bp);
        s_succ3(s)[s->bp] = s_get_head3(s,key);
        s_head3(s)[key] = SWD_UINT(s->bp);
        s_best3(s)[s->bp] = SWD_UINT(s->swd_f + 1);
        s_llen3(s)[key]++;
        assert(s_llen3(s)[key] <= s->swd_n);

#ifdef HEAD2
        /* add bp into HEAD2 */
        IF_HEAD2(s) {
            key = HEAD2(s_b(s),s->bp);
            s_head2(s)[key] = SWD_UINT(s->bp);
        }
#endif

        swd_getbyte(s);
    } while (--n != 0);
}


/***********************************************************************
//
************************************************************************/

static
void swd_search(lzo_swd_p s, lzo_uint node, lzo_uint cnt)
{
    const lzo_bytep p1;
    const lzo_bytep p2;
    const lzo_bytep px;
    lzo_uint m_len = s->m_len;
    const lzo_bytep b  = s_b(s);
    const lzo_bytep bp = s_b(s) + s->bp;
    const lzo_bytep bx = s_b(s) + s->bp + s->look;
    swd_uintp succ3 = s_succ3(s);
    unsigned char scan_end1;

    assert(s->m_len > 0);

    scan_end1 = bp[m_len - 1];
    for ( ; cnt-- > 0; node = succ3[node])
    {
        p1 = bp;
        p2 = b + node;
        px = bx;

        assert(m_len < s->look);

        if (
#if 1
            p2[m_len - 1] == scan_end1 &&
            p2[m_len] == p1[m_len] &&
#endif
            p2[0] == p1[0] &&
            p2[1] == p1[1])
        {
            lzo_uint i;
            assert(lzo_memcmp(bp,&b[node],3) == 0);

#if 0 && (LZO_OPT_UNALIGNED32)
            p1 += 3; p2 += 3;
            while (p1 + 4 <= px && UA_GET_NE32(p1) == UA_GET_NE32(p2))
                p1 += 4, p2 += 4;
            while (p1 < px && *p1 == *p2)
                p1 += 1, p2 += 1;
#else
            p1 += 2; p2 += 2;
            do {} while (++p1 < px && *p1 == *++p2);
#endif
            i = pd(p1, bp);

#ifdef LZO_DEBUG
            if (lzo_memcmp(bp,&b[node],i) != 0)
                printf("%5ld %5ld %5ld %02x/%02x %02x/%02x\n",
                        (long)s->bp, (long) node, (long) i,
                        bp[0], bp[1], b[node], b[node+1]);
#endif
            assert(lzo_memcmp(bp,&b[node],i) == 0);

#if defined(SWD_BEST_OFF)
            if (i < SWD_BEST_OFF)
            {
                if (s->best_pos[i] == 0)
                    s->best_pos[i] = node + 1;
            }
#endif
            if (i > m_len)
            {
                s->m_len = m_len = i;
                s->m_pos = node;
                if (m_len == s->look)
                    return;
                if (m_len >= s->nice_length)
                    return;
                if (m_len > (lzo_uint) s_best3(s)[node])
                    return;
                scan_end1 = bp[m_len - 1];
            }
        }
    }
}


/***********************************************************************
//
************************************************************************/

#ifdef HEAD2

static
lzo_bool swd_search2(lzo_swd_p s)
{
    lzo_uint key;

    assert(s->look >= 2);
    assert(s->m_len > 0);

    key = s_head2(s)[ HEAD2(s_b(s),s->bp) ];
    if (key == NIL2)
        return 0;
#ifdef LZO_DEBUG
    if (lzo_memcmp(&s_b(s)[s->bp],&s_b(s)[key],2) != 0)
        printf("%5ld %5ld %02x/%02x %02x/%02x\n", (long)s->bp, (long)key,
                s_b(s)[s->bp], s_b(s)[s->bp+1], s_b(s)[key], s_b(s)[key+1]);
#endif
    assert(lzo_memcmp(&s_b(s)[s->bp],&s_b(s)[key],2) == 0);
#if defined(SWD_BEST_OFF)
    if (s->best_pos[2] == 0)
        s->best_pos[2] = key + 1;
#endif

    if (s->m_len < 2)
    {
        s->m_len = 2;
        s->m_pos = key;
    }
    return 1;
}

#endif


/***********************************************************************
//
************************************************************************/

static
void swd_findbest(lzo_swd_p s)
{
    lzo_uint key;
    lzo_uint cnt, node;
    lzo_uint len;

    assert(s->m_len > 0);

    /* get current head, add bp into HEAD3 */
    key = HEAD3(s_b(s),s->bp);
    node = s_succ3(s)[s->bp] = s_get_head3(s,key);
    cnt = s_llen3(s)[key]++;
    assert(s_llen3(s)[key] <= s->swd_n + s->swd_f);
    if (cnt > s->max_chain && s->max_chain > 0)
        cnt = s->max_chain;
    s_head3(s)[key] = SWD_UINT(s->bp);

    s->b_char = s_b(s)[s->bp];
    len = s->m_len;
    if (s->m_len >= s->look)
    {
        if (s->look == 0)
            s->b_char = -1;
        s->m_off = 0;
        s_best3(s)[s->bp] = SWD_UINT(s->swd_f + 1);
    }
    else
    {
#if defined(HEAD2)
        if (swd_search2(s) && s->look >= 3)
            swd_search(s,node,cnt);
#else
        if (s->look >= 3)
            swd_search(s,node,cnt);
#endif
        if (s->m_len > len)
            s->m_off = swd_pos2off(s,s->m_pos);
        s_best3(s)[s->bp] = SWD_UINT(s->m_len);

#if defined(SWD_BEST_OFF)
        if (s->use_best_off)
        {
            unsigned i;
            for (i = 2; i < SWD_BEST_OFF; i++)
                if (s->best_pos[i] > 0)
                    s->best_off[i] = swd_pos2off(s,s->best_pos[i]-1);
                else
                    s->best_off[i] = 0;
        }
#endif
    }

    swd_remove_node(s,s->rp);

#ifdef HEAD2
    /* add bp into HEAD2 */
    IF_HEAD2(s) {
        key = HEAD2(s_b(s),s->bp);
        s_head2(s)[key] = SWD_UINT(s->bp);
    }
#endif
}


#undef HEAD3
#undef HEAD2
#undef IF_HEAD2
#undef s_get_head3


/* vim:set ts=4 sw=4 et: */

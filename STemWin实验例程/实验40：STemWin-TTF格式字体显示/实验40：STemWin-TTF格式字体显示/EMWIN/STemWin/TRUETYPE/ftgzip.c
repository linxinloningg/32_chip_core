/***************************************************************************/
/*                                                                         */
/*  ftgzip.c                                                               */
/*                                                                         */
/*    FreeType support for .gz compressed files.                           */
/*                                                                         */
/*  This optional component relies on zlib.  It should mainly be used to   */
/*  parse compressed PCF fonts, as found with many X11 server              */
/*  distributions.                                                         */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005, 2006 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H
#include FT_GZIP_H
#include <string.h>


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#define FT_ERR_PREFIX  Gzip_Err_
#define FT_ERR_BASE    FT_Mod_Err_Gzip

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_ZLIB

#ifdef FT_CONFIG_OPTION_SYSTEM_ZLIB

#include <ttzlib.h>

#else /* !FT_CONFIG_OPTION_SYSTEM_ZLIB */

 /* In this case, we include our own modified sources of the ZLib    */
 /* within the "ftgzip" component.  The modifications were necessary */
 /* to #include all files without conflicts, as well as preventing   */
 /* the definition of "extern" functions that may cause linking      */
 /* conflicts when a program is linked with both FreeType and the    */
 /* original ZLib.                                                   */

#define NO_DUMMY_DECL
#define MY_ZCALLOC

#include "ttzlib.h"

#undef  SLOW
#define SLOW  1  /* we can't use asm-optimized sources here! */

  /* Urgh.  `inflate_mask' must not be declared twice -- C++ doesn't like
     this.  We temporarily disable it and load all necessary header files. */
#define NO_INFLATE_MASK
#include "ttzutil.h"
#include "ttinftrees.h"
#include "infblock.h"
#include "infcodes.h"
#include "infutil.h"
#undef  NO_INFLATE_MASK

  /* infutil.c must be included before infcodes.c */
/* zutil.c -- target dependent utility functions for the compression library
 * Copyright (C) 1995-2002 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

/* @(#) $Id: zutil.c,v 1.3 2006/04/29 07:31:16 wl Exp $ */

#include "ttzutil.h"

#ifndef STDC
extern void exit OF((int));
#endif


#ifndef HAVE_MEMCPY

void zmemcpy(dest, source, len)
    Bytef* dest;
    const Bytef* source;
    uInt  len;
{
    if (len == 0) return;
    do {
        *dest++ = *source++; /* ??? to be unrolled */
    } while (--len != 0);
}

int zmemcmp(s1, s2, len)
    const Bytef* s1;
    const Bytef* s2;
    uInt  len;
{
    uInt j;

    for (j = 0; j < len; j++) {
        if (s1[j] != s2[j]) return 2*(s1[j] > s2[j])-1;
    }
    return 0;
}

void zmemzero(dest, len)
    Bytef* dest;
    uInt  len;
{
    if (len == 0) return;
    do {
        *dest++ = 0;  /* ??? to be unrolled */
    } while (--len != 0);
}
#endif

#ifdef __TURBOC__
#if (defined( __BORLANDC__) || !defined(SMALL_MEDIUM)) && !defined(__32BIT__)
/* Small and medium model in Turbo C are for now limited to near allocation
 * with reduced MAX_WBITS and MAX_MEM_LEVEL
 */
#  define MY_ZCALLOC

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

local int next_ptr = 0;

typedef struct ptr_table_s {
    voidpf org_ptr;
    voidpf new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];
/* This table is used to remember the original form of pointers
 * to large buffers (64K). Such pointers are normalized with a zero offset.
 * Since MSDOS is not a preemptive multitasking OS, this table is not
 * protected from concurrent access. This hack doesn't work anyway on
 * a protected system like OS/2. Use Microsoft C instead.
 */

voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
    voidpf buf = opaque; /* just to make some compilers happy */
    ulg bsize = (ulg)items*size;

    /* If we allocate less than 65520 bytes, we assume that farmalloc
     * will return a usable pointer which doesn't have to be normalized.
     */
    if (bsize < 65520L) {
        buf = farmalloc(bsize);
        if (*(ush*)&buf != 0) return buf;
    } else {
        buf = farmalloc(bsize + 16L);
    }
    if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
    table[next_ptr].org_ptr = buf;

    /* Normalize the pointer to seg:0 */
    *((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
    *(ush*)&buf = 0;
    table[next_ptr++].new_ptr = buf;
    return buf;
}

void  zcfree (voidpf opaque, voidpf ptr)
{
    int n;
    if (*(ush*)&ptr != 0) { /* object < 64K */
        farfree(ptr);
        return;
    }
    /* Find the original pointer */
    for (n = 0; n < next_ptr; n++) {
        if (ptr != table[n].new_ptr) continue;

        farfree(table[n].org_ptr);
        while (++n < next_ptr) {
            table[n-1] = table[n];
        }
        next_ptr--;
        return;
    }
    ptr = opaque; /* just to make some compilers happy */
    Assert(0, "zcfree: ptr not found");
}
#endif
#endif /* __TURBOC__ */


#if defined(M_I86) && !defined(__32BIT__)
/* Microsoft C in 16-bit mode */

#  define MY_ZCALLOC

#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
    if (opaque) opaque = 0; /* to make compiler happy */
    return _halloc((long)items, size);
}

void  zcfree (voidpf opaque, voidpf ptr)
{
    if (opaque) opaque = 0; /* to make compiler happy */
    _hfree(ptr);
}

#endif /* MSC */


#ifndef MY_ZCALLOC /* Any system without a special alloc function */

#ifndef STDC
extern voidp  ft_scalloc OF((uInt items, uInt size));
extern void   ft_sfree   OF((voidpf ptr));
#endif

voidpf zcalloc (opaque, items, size)
    voidpf opaque;
    unsigned items;
    unsigned size;
{
    if (opaque) items += size - size; /* make compiler happy */
    return (voidpf)ft_scalloc(items, size);
}

void  zcfree (opaque, ptr)
    voidpf opaque;
    voidpf ptr;
{
    ft_sfree(ptr);
    if (opaque) return; /* make compiler happy */
}

#endif /* MY_ZCALLOC */

/* inftrees.c -- generate Huffman trees for efficient decoding
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

#include "ttzutil.h"
#include "ttinftrees.h"

#if !defined(BUILDFIXED) && !defined(STDC)
#  define BUILDFIXED   /* non ANSI compilers may not accept ttinffixed.h */
#endif


#if 0
local const char inflate_copyright[] =
   " inflate 1.1.4 Copyright 1995-2002 Mark Adler ";
#endif
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits


local int huft_build OF((
    uIntf *,            /* code lengths in bits */
    uInt,               /* number of codes */
    uInt,               /* number of "simple" codes */
    const uIntf *,      /* list of base values for non-simple codes */
    const uIntf *,      /* list of extra bits for non-simple codes */
    inflate_huft * FAR*,/* result: starting table */
    uIntf *,            /* maximum lookup bits (returns actual) */
    inflate_huft *,     /* space for trees */
    uInt *,             /* hufts used in space */
    uIntf * ));         /* space for values */

/* Tables for deflate from PKZIP's appnote.txt. */
local const uInt cplens[31] = { /* Copy lengths for literal codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        /* see note #13 above about 258 */
local const uInt cplext[31] = { /* Extra bits for literal codes 257..285 */
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112}; /* 112==invalid */
local const uInt cpdist[30] = { /* Copy offsets for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
local const uInt cpdext[30] = { /* Extra bits for distance codes */
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */


/* If BMAX needs to be larger than 16, then h and x[] should be uLong. */
#define BMAX 15         /* maximum bit length of any code */

local int huft_build( /* b, n, s, d, e, t, m, hp, hn, v) */
uIntf *b,               /* code lengths in bits (all assumed <= BMAX) */
uInt n,                 /* number of codes (assumed <= 288) */
uInt s,                 /* number of simple-valued codes (0..s-1) */
const uIntf *d,         /* list of base values for non-simple codes */
const uIntf *e,         /* list of extra bits for non-simple codes */
inflate_huft * FAR *t,  /* result: starting table */
uIntf *m,               /* maximum lookup bits, returns actual */
inflate_huft *hp,       /* space for trees */
uInt *hn,               /* hufts used in space */
uIntf *v                /* working area: values in order of bit length */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return Z_OK on success, Z_BUF_ERROR
   if the given code set is incomplete (the tables are still built in this
   case), or Z_DATA_ERROR if the input is invalid. */
)
{

  uInt a;                       /* counter for codes of length k */
  uInt c[BMAX+1];               /* bit length count table */
  uInt f;                       /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  register uInt i;              /* counter, current code */
  register uInt j;              /* counter */
  register int k;               /* number of bits in current code */
  int l;                        /* bits per table (returned in m) */
  uInt mask;                    /* (1 << w) - 1, to avoid cc -O bug on HP */
  register uIntf *p;            /* pointer into c[], b[], or v[] */
  inflate_huft *q;              /* points to current table */
  struct inflate_huft_s r;      /* table entry for structure assignment */
  inflate_huft *u[BMAX];        /* table stack */
  register int w;               /* bits before this table == (l * h) */
  uInt x[BMAX+1];               /* bit offsets, then code stack */
  uIntf *xp;                    /* pointer into x */
  int y;                        /* number of dummy codes added */
  uInt z;                       /* number of entries in current table */


  /* Generate counts for each bit length */
  p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
  C4                            /* clear c[]--assume BMAX+1 is 16 */
  p = b;  i = n;
  do {
    c[*p++]++;                  /* assume all entries <= BMAX */
  } while (--i);
  if (c[0] == n)                /* null input--all zero length codes */
  {
    *t = (inflate_huft *)Z_NULL;
    *m = 0;
    return Z_OK;
  }


  /* Find minimum and maximum length, bound *m by those */
  l = *m;
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  k = j;                        /* minimum code length */
  if ((uInt)l < j)
    l = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;
  g = i;                        /* maximum code length */
  if ((uInt)l > i)
    l = i;
  *m = l;


  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return Z_DATA_ERROR;
  if ((y -= c[i]) < 0)
    return Z_DATA_ERROR;
  c[i] += y;


  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }


  /* Make a table of values in order of bit lengths */
  p = b;  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);
  n = x[g];                     /* set n to length of v */


  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = -l;                       /* bits decoded == (l * h) */
  u[0] = (inflate_huft *)Z_NULL;        /* just to keep compilers happy */
  q = (inflate_huft *)Z_NULL;   /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l)
      {
        h++;
        w += l;                 /* previous table always l bits */

        /* compute minimum size table less than or equal to l bits */
        z = g - w;
        z = z > (uInt)l ? (uInt)l : z;        /* table size upper limit */
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          if (j < z)
            while (++j < z)     /* try smaller tables up to z bits */
            {
              if ((f <<= 1) <= *++xp)
                break;          /* enough codes to use up j bits */
              f -= *xp;         /* else deduct codes from patterns */
            }
        }
        z = 1 << j;             /* table entries for j-bit table */

        /* allocate new table */
        if (*hn + z > MANY)     /* (note: doesn't matter for fixed) */
          return Z_DATA_ERROR;  /* overflow of MANY */
        u[h] = q = hp + *hn;
        *hn += z;

        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.bits = (Byte)l;     /* bits to dump before this table */
          r.exop = (Byte)j;     /* bits in this table */
          j = i >> (w - l);
          r.base = (uInt)(q - u[h-1] - j);   /* offset to this table */
          u[h-1][j] = r;        /* connect to last table */
        }
        else
          *t = q;               /* first table is returned result */
      }

      /* set up table entry in r */
      r.bits = (Byte)(k - w);
      if (p >= v + n)
        r.exop = 128 + 64;      /* out of values--invalid code */
      else if (*p < s)
      {
        r.exop = (Byte)(*p < 256 ? 0 : 32 + 64);     /* 256 is end-of-block */
        r.base = *p++;          /* simple code is just the value */
      }
      else
      {
        r.exop = (Byte)(e[*p - s] + 16 + 64);/* non-simple--look up in lists */
        r.base = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      mask = (1 << w) - 1;      /* needed on HP, cc -O bug */
      while ((i & mask) != x[h])
      {
        h--;                    /* don't need to update q */
        w -= l;
        mask = (1 << w) - 1;
      }
    }
  }


  /* Return Z_BUF_ERROR if we were given an incomplete table */
  return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}


local int inflate_trees_bits( /* c, bb, tb, hp, z) */
uIntf *c,               /* 19 code lengths */
uIntf *bb,              /* bits tree desired/actual depth */
inflate_huft * FAR *tb, /* bits tree result */
inflate_huft *hp,       /* space for trees */
z_streamp z             /* for messages */
)
{
  int r;
  uInt hn = 0;          /* hufts used in space */
  uIntf *v;             /* work area for huft_build */

  if ((v = (uIntf*)ZALLOC(z, 19, sizeof(uInt))) == Z_NULL)
    return Z_MEM_ERROR;
  r = huft_build(c, 19, 19, (uIntf*)Z_NULL, (uIntf*)Z_NULL,
                 tb, bb, hp, &hn, v);
  if (r == Z_DATA_ERROR)
    z->msg = (char*)"oversubscribed dynamic bit lengths tree";
  else if (r == Z_BUF_ERROR || *bb == 0)
  {
    z->msg = (char*)"incomplete dynamic bit lengths tree";
    r = Z_DATA_ERROR;
  }
  ZFREE(z, v);
  return r;
}


local int inflate_trees_dynamic( /* nl, nd, c, bl, bd, tl, td, hp, z) */
uInt nl,                /* number of literal/length codes */
uInt nd,                /* number of distance codes */
uIntf *c,               /* that many (total) code lengths */
uIntf *bl,              /* literal desired/actual bit depth */
uIntf *bd,              /* distance desired/actual bit depth */
inflate_huft * FAR *tl, /* literal/length tree result */
inflate_huft * FAR *td, /* distance tree result */
inflate_huft *hp,       /* space for trees */
z_streamp z             /* for messages */
)
{
  int r;
  uInt hn = 0;          /* hufts used in space */
  uIntf *v;             /* work area for huft_build */

  /* allocate work area */
  if ((v = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
    return Z_MEM_ERROR;

  /* build literal/length tree */
  r = huft_build(c, nl, 257, cplens, cplext, tl, bl, hp, &hn, v);
  if (r != Z_OK || *bl == 0)
  {
    if (r == Z_DATA_ERROR)
      z->msg = (char*)"oversubscribed literal/length tree";
    else if (r != Z_MEM_ERROR)
    {
      z->msg = (char*)"incomplete literal/length tree";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
  }

  /* build distance tree */
  r = huft_build(c + nl, nd, 0, cpdist, cpdext, td, bd, hp, &hn, v);
  if (r != Z_OK || (*bd == 0 && nl > 257))
  {
    if (r == Z_DATA_ERROR)
      z->msg = (char*)"oversubscribed distance tree";
    else if (r == Z_BUF_ERROR) {
#ifdef PKZIP_BUG_WORKAROUND
      r = Z_OK;
    }
#else
      z->msg = (char*)"incomplete distance tree";
      r = Z_DATA_ERROR;
    }
    else if (r != Z_MEM_ERROR)
    {
      z->msg = (char*)"empty distance tree with lengths";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
#endif
  }

  /* done */
  ZFREE(z, v);
  return Z_OK;
}


/* build fixed tables only once--keep them here */
#ifdef BUILDFIXED
local int fixed_built = 0;
#define FIXEDH 544      /* number of hufts used by fixed tables */
local inflate_huft fixed_mem[FIXEDH];
local uInt fixed_bl;
local uInt fixed_bd;
local inflate_huft *fixed_tl;
local inflate_huft *fixed_td;
#else
#include "ttinffixed.h"
#endif


local int inflate_trees_fixed( /* bl, bd, tl, td, z) */
uIntf *bl,                      /* literal desired/actual bit depth */
uIntf *bd,                      /* distance desired/actual bit depth */
const inflate_huft * FAR *tl,   /* literal/length tree result */
const inflate_huft * FAR *td,   /* distance tree result */
z_streamp z                     /* for memory allocation */
)
{
#ifdef BUILDFIXED
  /* build fixed tables if not already */
  if (!fixed_built)
  {
    int k;              /* temporary variable */
    uInt f = 0;         /* number of hufts used in fixed_mem */
    uIntf *c;           /* length list for huft_build */
    uIntf *v;           /* work area for huft_build */

    /* allocate memory */
    if ((c = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
      return Z_MEM_ERROR;
    if ((v = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
    {
      ZFREE(z, c);
      return Z_MEM_ERROR;
    }

    /* literal table */
    for (k = 0; k < 144; k++)
      c[k] = 8;
    for (; k < 256; k++)
      c[k] = 9;
    for (; k < 280; k++)
      c[k] = 7;
    for (; k < 288; k++)
      c[k] = 8;
    fixed_bl = 9;
    huft_build(c, 288, 257, cplens, cplext, &fixed_tl, &fixed_bl,
               fixed_mem, &f, v);

    /* distance table */
    for (k = 0; k < 30; k++)
      c[k] = 5;
    fixed_bd = 5;
    huft_build(c, 30, 0, cpdist, cpdext, &fixed_td, &fixed_bd,
               fixed_mem, &f, v);

    /* done */
    ZFREE(z, v);
    ZFREE(z, c);
    fixed_built = 1;
  }
#else
  FT_UNUSED(z);
#endif
  *bl = fixed_bl;
  *bd = fixed_bd;
  *tl = fixed_tl;
  *td = fixed_td;
  return Z_OK;
}

/* inflate_util.c -- data and routines common to blocks and codes
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

#include "ttzutil.h"
#include "infblock.h"
#include "ttinftrees.h"
#include "infcodes.h"
#include "infutil.h"


/* And'ing with mask[n] masks the lower n bits */
local const uInt inflate_mask[17] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};


/* copy as much as possible from the sliding window to the output area */
local int inflate_flush( /* s, z, r) */
inflate_blocks_statef *s,
z_streamp z,
int r )
{
  uInt n;
  Bytef *p;
  Bytef *q;

  /* local copies of source and destination pointers */
  p = z->next_out;
  q = s->read;

  /* compute number of bytes to copy as far as end of window */
  n = (uInt)((q <= s->write ? s->write : s->end) - q);
  if (n > z->avail_out) n = z->avail_out;
  if (n && r == Z_BUF_ERROR) r = Z_OK;

  /* update counters */
  z->avail_out -= n;
  z->total_out += n;

  /* update check information */
  if (s->checkfn != Z_NULL)
    z->adler = s->check = (*s->checkfn)(s->check, q, n);

  /* copy as far as end of window */
  zmemcpy(p, q, n);
  p += n;
  q += n;

  /* see if more to copy at beginning of window */
  if (q == s->end)
  {
    /* wrap pointers */
    q = s->window;
    if (s->write == s->end)
      s->write = s->window;

    /* compute bytes to copy */
    n = (uInt)(s->write - q);
    if (n > z->avail_out) n = z->avail_out;
    if (n && r == Z_BUF_ERROR) r = Z_OK;

    /* update counters */
    z->avail_out -= n;
    z->total_out += n;

    /* update check information */
    if (s->checkfn != Z_NULL)
      z->adler = s->check = (*s->checkfn)(s->check, q, n);

    /* copy */
    zmemcpy(p, q, n);
    p += n;
    q += n;
  }

  /* update pointers */
  z->next_out = p;
  s->read = q;

  /* done */
  return r;
}

/* infcodes.c -- process literals and length/distance pairs
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

#include "ttzutil.h"
#include "ttinftrees.h"
#include "infblock.h"
#include "infcodes.h"
#include "infutil.h"

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

typedef enum {        /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
      START,    /* x: set up for LEN */
      LEN,      /* i: get length/literal/eob next */
      LENEXT,   /* i: getting length extra (have base) */
      DIST,     /* i: get distance next */
      DISTEXT,  /* i: getting distance extra */
      COPY,     /* o: copying bytes in window, waiting for space */
      LIT,      /* o: got literal, waiting for output space */
      WASH,     /* o: got eob, possibly still output waiting */
      END,      /* x: got eob and all data flushed */
      BADCODE}  /* x: got error */
inflate_codes_mode;

/* inflate codes private state */
struct inflate_codes_state {

  /* mode */
  inflate_codes_mode mode;      /* current inflate_codes mode */

  /* mode dependent information */
  uInt len;
  union {
    struct {
      inflate_huft *tree;       /* pointer into tree */
      uInt need;                /* bits needed */
    } code;             /* if LEN or DIST, where in tree */
    uInt lit;           /* if LIT, literal */
    struct {
      uInt get;                 /* bits to get for extra */
      uInt dist;                /* distance back to copy from */
    } copy;             /* if EXT or COPY, where and how much */
  } sub;                /* submode */

  /* mode independent information */
  Byte lbits;           /* ltree bits decoded per branch */
  Byte dbits;           /* dtree bits decoder per branch */
  inflate_huft *ltree;          /* literal/length/eob tree */
  inflate_huft *dtree;          /* distance tree */

};


local inflate_codes_statef *inflate_codes_new( /* bl, bd, tl, td, z) */
uInt bl, uInt bd,
inflate_huft *tl,
inflate_huft *td, /* need separate declaration for Borland C++ */
z_streamp z )
{
  inflate_codes_statef *c;

  if ((c = (inflate_codes_statef *)
       ZALLOC(z,1,sizeof(struct inflate_codes_state))) != Z_NULL)
  {
    c->mode = START;
    c->lbits = (Byte)bl;
    c->dbits = (Byte)bd;
    c->ltree = tl;
    c->dtree = td;
    Tracev((stderr, "inflate:       codes new\n"));
  }
  return c;
}


local int inflate_codes( /* s, z, r) */
inflate_blocks_statef *s,
z_streamp z,
int r )
{
  uInt j;               /* temporary storage */
  inflate_huft *t;      /* temporary pointer */
  uInt e;               /* extra bits or operation */
  uLong b;              /* bit buffer */
  uInt k;               /* bits in bit buffer */
  Bytef *p;             /* input data pointer */
  uInt n;               /* bytes available there */
  Bytef *q;             /* output window write pointer */
  uInt m;               /* bytes to end of window or read pointer */
  Bytef *f;             /* pointer to copy strings from */
  inflate_codes_statef *c = s->sub.decode.codes;  /* codes state */

  /* copy input/output information to locals (UPDATE macro restores) */
  LOAD

  /* process input and output based on current state */
  while (1) switch (c->mode)
  {             /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
    case START:         /* x: set up for LEN */
#ifndef SLOW
      if (m >= 258 && n >= 10)
      {
        UPDATE
        r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
        LOAD
        if (r != Z_OK)
        {
          c->mode = r == Z_STREAM_END ? WASH : BADCODE;
          break;
        }
      }
#endif /* !SLOW */
      c->sub.code.need = c->lbits;
      c->sub.code.tree = c->ltree;
      c->mode = LEN;
    case LEN:           /* i: get length/literal/eob next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      e = (uInt)(t->exop);
      if (e == 0)               /* literal */
      {
        c->sub.lit = t->base;
        Tracevv((stderr, t->base >= 0x20 && t->base < 0x7f ?
                 "inflate:         literal '%c'\n" :
                 "inflate:         literal 0x%02x\n", t->base));
        c->mode = LIT;
        break;
      }
      if (e & 16)               /* length */
      {
        c->sub.copy.get = e & 15;
        c->len = t->base;
        c->mode = LENEXT;
        break;
      }
      if ((e & 64) == 0)        /* next table */
      {
        c->sub.code.need = e;
        c->sub.code.tree = t + t->base;
        break;
      }
      if (e & 32)               /* end of block */
      {
        Tracevv((stderr, "inflate:         end of block\n"));
        c->mode = WASH;
        break;
      }
      c->mode = BADCODE;        /* invalid code */
      z->msg = (char*)"invalid literal/length code";
      r = Z_DATA_ERROR;
      LEAVE
    case LENEXT:        /* i: getting length extra (have base) */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->len += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      c->sub.code.need = c->dbits;
      c->sub.code.tree = c->dtree;
      Tracevv((stderr, "inflate:         length %u\n", c->len));
      c->mode = DIST;
    case DIST:          /* i: get distance next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      e = (uInt)(t->exop);
      if (e & 16)               /* distance */
      {
        c->sub.copy.get = e & 15;
        c->sub.copy.dist = t->base;
        c->mode = DISTEXT;
        break;
      }
      if ((e & 64) == 0)        /* next table */
      {
        c->sub.code.need = e;
        c->sub.code.tree = t + t->base;
        break;
      }
      c->mode = BADCODE;        /* invalid code */
      z->msg = (char*)"invalid distance code";
      r = Z_DATA_ERROR;
      LEAVE
    case DISTEXT:       /* i: getting distance extra */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->sub.copy.dist += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      Tracevv((stderr, "inflate:         distance %u\n", c->sub.copy.dist));
      c->mode = COPY;
    case COPY:          /* o: copying bytes in window, waiting for space */
      f = q - c->sub.copy.dist;
      while (f < s->window)             /* modulo window size-"while" instead */
        f += s->end - s->window;        /* of "if" handles invalid distances */
      while (c->len)
      {
        NEEDOUT
        OUTBYTE(*f++)
        if (f == s->end)
          f = s->window;
        c->len--;
      }
      c->mode = START;
      break;
    case LIT:           /* o: got literal, waiting for output space */
      NEEDOUT
      OUTBYTE(c->sub.lit)
      c->mode = START;
      break;
    case WASH:          /* o: got eob, possibly more output */
      if (k > 7)        /* return unused byte, if any */
      {
        Assert(k < 16, "inflate_codes grabbed too many bytes")
        k -= 8;
        n++;
        p--;            /* can always return one */
      }
      FLUSH
      if (s->read != s->write)
        LEAVE
      c->mode = END;
    case END:
      r = Z_STREAM_END;
      LEAVE
    case BADCODE:       /* x: got error */
      r = Z_DATA_ERROR;
      LEAVE
    default:
      r = Z_STREAM_ERROR;
      LEAVE
  }
#ifdef NEED_DUMMY_RETURN
  return Z_STREAM_ERROR;  /* Some dumb compilers complain without this */
#endif
}


local void inflate_codes_free( /* c, z) */
inflate_codes_statef *c,
z_streamp z )
{
  ZFREE(z, c);
  Tracev((stderr, "inflate:       codes free\n"));
}

/* infblock.c -- interpret and process block types to last block
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

#include "ttzutil.h"
#include "infblock.h"
#include "ttinftrees.h"
#include "infcodes.h"
#include "infutil.h"


/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

/* Table for deflate from PKZIP's appnote.txt. */
local const uInt border[] = { /* Order of the bit length code lengths */
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.  The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
 */


local void inflate_blocks_reset( /* s, z, c) */
inflate_blocks_statef *s,
z_streamp z,
uLongf *c )
{
  if (c != Z_NULL)
    *c = s->check;
  if (s->mode == BTREE || s->mode == DTREE)
    ZFREE(z, s->sub.trees.blens);
  if (s->mode == CODES)
    inflate_codes_free(s->sub.decode.codes, z);
  s->mode = TYPE;
  s->bitk = 0;
  s->bitb = 0;
  s->read = s->write = s->window;
  if (s->checkfn != Z_NULL)
    z->adler = s->check = (*s->checkfn)(0L, (const Bytef *)Z_NULL, 0);
  Tracev((stderr, "inflate:   blocks reset\n"));
}


local inflate_blocks_statef *inflate_blocks_new( /* z, c, w) */
z_streamp z,
check_func c,
uInt w )
{
  inflate_blocks_statef *s;

  if ((s = (inflate_blocks_statef *)ZALLOC
       (z,1,sizeof(struct inflate_blocks_state))) == Z_NULL)
    return s;
  if ((s->hufts =
       (inflate_huft *)ZALLOC(z, sizeof(inflate_huft), MANY)) == Z_NULL)
  {
    ZFREE(z, s);
    return Z_NULL;
  }
  if ((s->window = (Bytef *)ZALLOC(z, 1, w)) == Z_NULL)
  {
    ZFREE(z, s->hufts);
    ZFREE(z, s);
    return Z_NULL;
  }
  s->end = s->window + w;
  s->checkfn = c;
  s->mode = TYPE;
  Tracev((stderr, "inflate:   blocks allocated\n"));
  inflate_blocks_reset(s, z, Z_NULL);
  return s;
}


local int inflate_blocks( /* s, z, r) */
inflate_blocks_statef *s,
z_streamp z,
int r )
{
  uInt t;               /* temporary storage */
  uLong b;              /* bit buffer */
  uInt k;               /* bits in bit buffer */
  Bytef *p;             /* input data pointer */
  uInt n;               /* bytes available there */
  Bytef *q;             /* output window write pointer */
  uInt m;               /* bytes to end of window or read pointer */

  /* copy input/output information to locals (UPDATE macro restores) */
  LOAD

  /* process input based on current state */
  while (1) switch (s->mode)
  {
    case TYPE:
      NEEDBITS(3)
      t = (uInt)b & 7;
      s->last = t & 1;
      switch (t >> 1)
      {
        case 0:                         /* stored */
          Tracev((stderr, "inflate:     stored block%s\n",
                 s->last ? " (last)" : ""));
          DUMPBITS(3)
          t = k & 7;                    /* go to byte boundary */
          DUMPBITS(t)
          s->mode = LENS;               /* get length of stored block */
          break;
        case 1:                         /* fixed */
          Tracev((stderr, "inflate:     fixed codes block%s\n",
                 s->last ? " (last)" : ""));
          {
            uInt bl, bd;
            inflate_huft *tl, *td;

            inflate_trees_fixed(&bl, &bd, (const inflate_huft**)&tl,
                                          (const inflate_huft**)&td, z);
            s->sub.decode.codes = inflate_codes_new(bl, bd, tl, td, z);
            if (s->sub.decode.codes == Z_NULL)
            {
              r = Z_MEM_ERROR;
              LEAVE
            }
          }
          DUMPBITS(3)
          s->mode = CODES;
          break;
        case 2:                         /* dynamic */
          Tracev((stderr, "inflate:     dynamic codes block%s\n",
                 s->last ? " (last)" : ""));
          DUMPBITS(3)
          s->mode = TABLE;
          break;
        case 3:                         /* illegal */
          DUMPBITS(3)
          s->mode = BAD;
          z->msg = (char*)"invalid block type";
          r = Z_DATA_ERROR;
          LEAVE
      }
      break;
    case LENS:
      NEEDBITS(32)
      if ((((~b) >> 16) & 0xffff) != (b & 0xffff))
      {
        s->mode = BAD;
        z->msg = (char*)"invalid stored block lengths";
        r = Z_DATA_ERROR;
        LEAVE
      }
      s->sub.left = (uInt)b & 0xffff;
      b = k = 0;                      /* dump bits */
      Tracev((stderr, "inflate:       stored length %u\n", s->sub.left));
      s->mode = s->sub.left ? STORED : (s->last ? DRY : TYPE);
      break;
    case STORED:
      if (n == 0)
        LEAVE
      NEEDOUT
      t = s->sub.left;
      if (t > n) t = n;
      if (t > m) t = m;
      zmemcpy(q, p, t);
      p += t;  n -= t;
      q += t;  m -= t;
      if ((s->sub.left -= t) != 0)
        break;
      Tracev((stderr, "inflate:       stored end, %lu total out\n",
              z->total_out + (q >= s->read ? q - s->read :
              (s->end - s->read) + (q - s->window))));
      s->mode = s->last ? DRY : TYPE;
      break;
    case TABLE:
      NEEDBITS(14)
      s->sub.trees.table = t = (uInt)b & 0x3fff;
#ifndef PKZIP_BUG_WORKAROUND
      if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29)
      {
        s->mode = BAD;
        z->msg = (char*)"too many length or distance symbols";
        r = Z_DATA_ERROR;
        LEAVE
      }
#endif
      t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
      if ((s->sub.trees.blens = (uIntf*)ZALLOC(z, t, sizeof(uInt))) == Z_NULL)
      {
        r = Z_MEM_ERROR;
        LEAVE
      }
      DUMPBITS(14)
      s->sub.trees.index = 0;
      Tracev((stderr, "inflate:       table sizes ok\n"));
      s->mode = BTREE;
    case BTREE:
      while (s->sub.trees.index < 4 + (s->sub.trees.table >> 10))
      {
        NEEDBITS(3)
        s->sub.trees.blens[border[s->sub.trees.index++]] = (uInt)b & 7;
        DUMPBITS(3)
      }
      while (s->sub.trees.index < 19)
        s->sub.trees.blens[border[s->sub.trees.index++]] = 0;
      s->sub.trees.bb = 7;
      t = inflate_trees_bits(s->sub.trees.blens, &s->sub.trees.bb,
                             &s->sub.trees.tb, s->hufts, z);
      if (t != Z_OK)
      {
        r = t;
        if (r == Z_DATA_ERROR)
        {
          ZFREE(z, s->sub.trees.blens);
          s->mode = BAD;
        }
        LEAVE
      }
      s->sub.trees.index = 0;
      Tracev((stderr, "inflate:       bits tree ok\n"));
      s->mode = DTREE;
    case DTREE:
      while (t = s->sub.trees.table,
             s->sub.trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))
      {
        inflate_huft *h;
        uInt i, j, c;

        t = s->sub.trees.bb;
        NEEDBITS(t)
        h = s->sub.trees.tb + ((uInt)b & inflate_mask[t]);
        t = h->bits;
        c = h->base;
        if (c < 16)
        {
          DUMPBITS(t)
          s->sub.trees.blens[s->sub.trees.index++] = c;
        }
        else /* c == 16..18 */
        {
          i = c == 18 ? 7 : c - 14;
          j = c == 18 ? 11 : 3;
          NEEDBITS(t + i)
          DUMPBITS(t)
          j += (uInt)b & inflate_mask[i];
          DUMPBITS(i)
          i = s->sub.trees.index;
          t = s->sub.trees.table;
          if (i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) ||
              (c == 16 && i < 1))
          {
            ZFREE(z, s->sub.trees.blens);
            s->mode = BAD;
            z->msg = (char*)"invalid bit length repeat";
            r = Z_DATA_ERROR;
            LEAVE
          }
          c = c == 16 ? s->sub.trees.blens[i - 1] : 0;
          do {
            s->sub.trees.blens[i++] = c;
          } while (--j);
          s->sub.trees.index = i;
        }
      }
      s->sub.trees.tb = Z_NULL;
      {
        uInt bl, bd;
        inflate_huft *tl, *td;
        inflate_codes_statef *c;

        bl = 9;         /* must be <= 9 for lookahead assumptions */
        bd = 6;         /* must be <= 9 for lookahead assumptions */
        t = s->sub.trees.table;
        t = inflate_trees_dynamic(257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f),
                                  s->sub.trees.blens, &bl, &bd, &tl, &td,
                                  s->hufts, z);
        if (t != Z_OK)
        {
          if (t == (uInt)Z_DATA_ERROR)
          {
            ZFREE(z, s->sub.trees.blens);
            s->mode = BAD;
          }
          r = t;
          LEAVE
        }
        Tracev((stderr, "inflate:       trees ok\n"));
        if ((c = inflate_codes_new(bl, bd, tl, td, z)) == Z_NULL)
        {
          r = Z_MEM_ERROR;
          LEAVE
        }
        s->sub.decode.codes = c;
      }
      ZFREE(z, s->sub.trees.blens);
      s->mode = CODES;
    case CODES:
      UPDATE
      if ((r = inflate_codes(s, z, r)) != Z_STREAM_END)
        return inflate_flush(s, z, r);
      r = Z_OK;
      inflate_codes_free(s->sub.decode.codes, z);
      LOAD
      Tracev((stderr, "inflate:       codes end, %lu total out\n",
              z->total_out + (q >= s->read ? q - s->read :
              (s->end - s->read) + (q - s->window))));
      if (!s->last)
      {
        s->mode = TYPE;
        break;
      }
      s->mode = DRY;
    case DRY:
      FLUSH
      if (s->read != s->write)
        LEAVE
      s->mode = DONE;
    case DONE:
      r = Z_STREAM_END;
      LEAVE
    case BAD:
      r = Z_DATA_ERROR;
      LEAVE
    default:
      r = Z_STREAM_ERROR;
      LEAVE
  }
#ifdef NEED_DUMMY_RETURN
  return 0;
#endif
}


local int inflate_blocks_free( /* s, z) */
inflate_blocks_statef *s,
z_streamp z )
{
  inflate_blocks_reset(s, z, Z_NULL);
  ZFREE(z, s->window);
  ZFREE(z, s->hufts);
  ZFREE(z, s);
  Tracev((stderr, "inflate:   blocks freed\n"));
  return Z_OK;
}



/* inflate.c -- zlib interface to inflate modules
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

#include "ttzutil.h"
#include "infblock.h"

#define  DONE  INFLATE_DONE
#define  BAD   INFLATE_BAD

typedef enum {
      METHOD,   /* waiting for method byte */
      FLAG,     /* waiting for flag byte */
      DICT4,    /* four dictionary check bytes to go */
      DICT3,    /* three dictionary check bytes to go */
      DICT2,    /* two dictionary check bytes to go */
      DICT1,    /* one dictionary check byte to go */
      DICT0,    /* waiting for inflateSetDictionary */
      BLOCKS,   /* decompressing blocks */
      CHECK4,   /* four check bytes to go */
      CHECK3,   /* three check bytes to go */
      CHECK2,   /* two check bytes to go */
      CHECK1,   /* one check byte to go */
      DONE,     /* finished check, done */
      BAD}      /* got an error--stay here */
inflate_mode;

/* inflate private state */
struct internal_state {

  /* mode */
  inflate_mode  mode;   /* current inflate mode */

  /* mode dependent information */
  union {
    uInt method;        /* if FLAGS, method byte */
    struct {
      uLong was;                /* computed check value */
      uLong need;               /* stream check value */
    } check;            /* if CHECK, check values to compare */
    uInt marker;        /* if BAD, inflateSync's marker bytes count */
  } sub;        /* submode */

  /* mode independent information */
  int  nowrap;          /* flag for no wrapper */
  uInt wbits;           /* log2(window size)  (8..15, defaults to 15) */
  inflate_blocks_statef
    *blocks;            /* current inflate_blocks state */

};


ZEXPORT(int) inflateReset( /* z) */
z_streamp z )
{
  if (z == Z_NULL || z->state == Z_NULL)
    return Z_STREAM_ERROR;
  z->total_in = z->total_out = 0;
  z->msg = Z_NULL;
  z->state->mode = z->state->nowrap ? BLOCKS : METHOD;
  inflate_blocks_reset(z->state->blocks, z, Z_NULL);
  Tracev((stderr, "inflate: reset\n"));
  return Z_OK;
}


ZEXPORT(int) inflateEnd( /* z) */
z_streamp z )
{
  if (z == Z_NULL || z->state == Z_NULL || z->zfree == Z_NULL)
    return Z_STREAM_ERROR;
  if (z->state->blocks != Z_NULL)
    inflate_blocks_free(z->state->blocks, z);
  ZFREE(z, z->state);
  z->state = Z_NULL;
  Tracev((stderr, "inflate: end\n"));
  return Z_OK;
}


ZEXPORT(int) inflateInit2_( /* z, w, version, stream_size) */
z_streamp z,
int w,
const char *version,
int stream_size )
{
  if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
      stream_size != sizeof(z_stream))
      return Z_VERSION_ERROR;

  /* initialize state */
  if (z == Z_NULL)
    return Z_STREAM_ERROR;
  z->msg = Z_NULL;
  if (z->zalloc == Z_NULL)
  {
    z->zalloc = zcalloc;
    z->opaque = (voidpf)0;
  }
  if (z->zfree == Z_NULL) z->zfree = zcfree;
  if ((z->state = (struct internal_state FAR *)
       ZALLOC(z,1,sizeof(struct internal_state))) == Z_NULL)
    return Z_MEM_ERROR;
  z->state->blocks = Z_NULL;

  /* handle undocumented nowrap option (no zlib header or check) */
  z->state->nowrap = 0;
  if (w < 0)
  {
    w = - w;
    z->state->nowrap = 1;
  }

  /* set window size */
  if (w < 8 || w > 15)
  {
    inflateEnd(z);
    return Z_STREAM_ERROR;
  }
  z->state->wbits = (uInt)w;

  /* create inflate_blocks state */
  if ((z->state->blocks =
      inflate_blocks_new(z, z->state->nowrap ? Z_NULL : adler32, (uInt)1 << w))
      == Z_NULL)
  {
    inflateEnd(z);
    return Z_MEM_ERROR;
  }
  Tracev((stderr, "inflate: allocated\n"));

  /* reset state */
  inflateReset(z);
  return Z_OK;
}



#undef  NEEDBYTE
#define NEEDBYTE {if(z->avail_in==0)return r;r=f;}

#undef  NEXTBYTE
#define NEXTBYTE (z->avail_in--,z->total_in++,*z->next_in++)


ZEXPORT(int) inflate( /* z, f) */
z_streamp z,
int f )
{
  int r;
  uInt b;

  if (z == Z_NULL || z->state == Z_NULL || z->next_in == Z_NULL)
    return Z_STREAM_ERROR;
  f = f == Z_FINISH ? Z_BUF_ERROR : Z_OK;
  r = Z_BUF_ERROR;
  while (1) switch (z->state->mode)
  {
    case METHOD:
      NEEDBYTE
      if (((z->state->sub.method = NEXTBYTE) & 0xf) != Z_DEFLATED)
      {
        z->state->mode = BAD;
        z->msg = (char*)"unknown compression method";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      if ((z->state->sub.method >> 4) + 8 > z->state->wbits)
      {
        z->state->mode = BAD;
        z->msg = (char*)"invalid window size";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      z->state->mode = FLAG;
    case FLAG:
      NEEDBYTE
      b = NEXTBYTE;
      if (((z->state->sub.method << 8) + b) % 31)
      {
        z->state->mode = BAD;
        z->msg = (char*)"incorrect header check";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      Tracev((stderr, "inflate: zlib header ok\n"));
      if (!(b & PRESET_DICT))
      {
        z->state->mode = BLOCKS;
        break;
      }
      z->state->mode = DICT4;
    case DICT4:
      NEEDBYTE
      z->state->sub.check.need = (uLong)NEXTBYTE << 24;
      z->state->mode = DICT3;
    case DICT3:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE << 16;
      z->state->mode = DICT2;
    case DICT2:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE << 8;
      z->state->mode = DICT1;
    case DICT1:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE;
      z->adler = z->state->sub.check.need;
      z->state->mode = DICT0;
      return Z_NEED_DICT;
    case DICT0:
      z->state->mode = BAD;
      z->msg = (char*)"need dictionary";
      z->state->sub.marker = 0;       /* can try inflateSync */
      return Z_STREAM_ERROR;
    case BLOCKS:
      r = inflate_blocks(z->state->blocks, z, r);
      if (r == Z_DATA_ERROR)
      {
        z->state->mode = BAD;
        z->state->sub.marker = 0;       /* can try inflateSync */
        break;
      }
      if (r == Z_OK)
        r = f;
      if (r != Z_STREAM_END)
        return r;
      r = f;
      inflate_blocks_reset(z->state->blocks, z, &z->state->sub.check.was);
      if (z->state->nowrap)
      {
        z->state->mode = DONE;
        break;
      }
      z->state->mode = CHECK4;
    case CHECK4:
      NEEDBYTE
      z->state->sub.check.need = (uLong)NEXTBYTE << 24;
      z->state->mode = CHECK3;
    case CHECK3:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE << 16;
      z->state->mode = CHECK2;
    case CHECK2:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE << 8;
      z->state->mode = CHECK1;
    case CHECK1:
      NEEDBYTE
      z->state->sub.check.need += (uLong)NEXTBYTE;

      if (z->state->sub.check.was != z->state->sub.check.need)
      {
        z->state->mode = BAD;
        z->msg = (char*)"incorrect data check";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      Tracev((stderr, "inflate: zlib check ok\n"));
      z->state->mode = DONE;
    case DONE:
      return Z_STREAM_END;
    case BAD:
      return Z_DATA_ERROR;
    default:
      return Z_STREAM_ERROR;
  }
#ifdef NEED_DUMMY_RETURN
  return Z_STREAM_ERROR;  /* Some dumb compilers complain without this */
#endif
}


/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in ttzlib.h
 */

/* @(#) $Id: adler32.c,v 1.4 2003/01/30 23:24:18 davidT Exp $ */

#include "ttzlib.h"

#define BASE 65521L /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* ========================================================================= */
ZEXPORT(uLong) adler32( /* adler, buf, len) */
    uLong adler,
    const Bytef *buf,
    uInt len )
{
    unsigned long s1 = adler & 0xffff;
    unsigned long s2 = (adler >> 16) & 0xffff;
    int k;

    if (buf == Z_NULL) return 1L;

    while (len > 0) {
        k = len < NMAX ? len : NMAX;
        len -= k;
        while (k >= 16) {
            DO16(buf);
	    buf += 16;
            k -= 16;
        }
        if (k != 0) do {
            s1 += *buf++;
	    s2 += s1;
        } while (--k);
        s1 %= BASE;
        s2 %= BASE;
    }
    return (s2 << 16) | s1;
}


#endif /* !FT_CONFIG_OPTION_SYSTEM_ZLIB */


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****            Z L I B   M E M O R Y   M A N A G E M E N T          *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  /* it is better to use FreeType memory routines instead of raw
     'malloc/free' */

  static voidpf
  ft_gzip_alloc( FT_Memory  memory,
                 uInt       items,
                 uInt       size )
  {
    FT_ULong    sz = (FT_ULong)size * items;
    FT_Error    error;
    FT_Pointer  p;


    (void)FT_ALLOC( p, sz );
    return p;
  }


  static void
  ft_gzip_free( FT_Memory  memory,
                voidpf     address )
  {
    FT_MEM_FREE( address );
  }


#ifndef FT_CONFIG_OPTION_SYSTEM_ZLIB

  local voidpf
  zcalloc ( voidpf    opaque,
            unsigned  items,
            unsigned  size )
  {
    return ft_gzip_alloc( (FT_Memory)opaque, items, size );
  }

  local void
  zcfree( voidpf  opaque,
          voidpf  ptr )
  {
    ft_gzip_free( (FT_Memory)opaque, ptr );
  }

#endif /* !SYSTEM_ZLIB */


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****               Z L I B   F I L E   D E S C R I P T O R           *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

#define FT_GZIP_BUFFER_SIZE  4096

  typedef struct  FT_GZipFileRec_
  {
    FT_Stream  source;         /* parent/source stream        */
    FT_Stream  stream;         /* embedding stream            */
    FT_Memory  memory;         /* memory allocator            */
    z_stream   zstream;        /* zlib input stream           */

    FT_ULong   start;          /* starting position, after .gz header */
    FT_Byte    input[FT_GZIP_BUFFER_SIZE];   /* input read buffer  */

    FT_Byte    buffer[FT_GZIP_BUFFER_SIZE];  /* output buffer      */
    FT_ULong   pos;                          /* position in output */
    FT_Byte*   cursor;
    FT_Byte*   limit;

  } FT_GZipFileRec, *FT_GZipFile;


  /* gzip flag byte */
#define FT_GZIP_ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define FT_GZIP_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define FT_GZIP_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define FT_GZIP_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define FT_GZIP_COMMENT      0x10 /* bit 4 set: file comment present */
#define FT_GZIP_RESERVED     0xE0 /* bits 5..7: reserved */


  /* check and skip .gz header - we don't support `transparent' compression */
  static FT_Error
  ft_gzip_check_header( FT_Stream  stream )
  {
    FT_Error  error;
    FT_Byte   head[4];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 4 ) )
      goto Exit;

    /* head[0] && head[1] are the magic numbers;    */
    /* head[2] is the method, and head[3] the flags */
    if ( head[0] != 0x1f              ||
         head[1] != 0x8b              ||
         head[2] != Z_DEFLATED        ||
        (head[3] & FT_GZIP_RESERVED)  )
    {
      error = Gzip_Err_Invalid_File_Format;
      goto Exit;
    }

    /* skip time, xflags and os code */
    (void)FT_STREAM_SKIP( 6 );

    /* skip the extra field */
    if ( head[3] & FT_GZIP_EXTRA_FIELD )
    {
      FT_UInt  len;


      if ( FT_READ_USHORT_LE( len ) ||
           FT_STREAM_SKIP( len )    )
        goto Exit;
    }

    /* skip original file name */
    if ( head[3] & FT_GZIP_ORIG_NAME )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    /* skip .gz comment */
    if ( head[3] & FT_GZIP_COMMENT )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    /* skip CRC */
    if ( head[3] & FT_GZIP_HEAD_CRC )
      if ( FT_STREAM_SKIP( 2 ) )
        goto Exit;

  Exit:
    return error;
  }


  static FT_Error
  ft_gzip_file_init( FT_GZipFile  zip,
                     FT_Stream    stream,
                     FT_Stream    source )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = Gzip_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    /* check and skip .gz header */
    {
      stream = source;

      error = ft_gzip_check_header( stream );
      if ( error )
        goto Exit;

      zip->start = FT_STREAM_POS();
    }

    /* initialize zlib -- there is no zlib header in the compressed stream */
    zstream->zalloc = (alloc_func)ft_gzip_alloc;
    zstream->zfree  = (free_func) ft_gzip_free;
    zstream->opaque = stream->memory;

    zstream->avail_in = 0;
    zstream->next_in  = zip->buffer;

    if ( inflateInit2( zstream, -MAX_WBITS ) != Z_OK ||
         zstream->next_in == NULL                     )
      error = Gzip_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static void
  ft_gzip_file_done( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;


    inflateEnd( zstream );

    /* clear the rest */
    zstream->zalloc    = NULL;
    zstream->zfree     = NULL;
    zstream->opaque    = NULL;
    zstream->next_in   = NULL;
    zstream->next_out  = NULL;
    zstream->avail_in  = 0;
    zstream->avail_out = 0;

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_gzip_file_reset( FT_GZipFile  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( zip->start ) )
    {
      z_stream*  zstream = &zip->zstream;


      inflateReset( zstream );

      zstream->avail_in  = 0;
      zstream->next_in   = zip->input;
      zstream->avail_out = 0;
      zstream->next_out  = zip->buffer;

      zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;
    }

    return error;
  }


  static FT_Error
  ft_gzip_file_fill_input( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Stream  stream  = zip->source;
    FT_ULong   size;


    if ( stream->read )
    {
      size = stream->read( stream, stream->pos, zip->input,
                           FT_GZIP_BUFFER_SIZE );
      if ( size == 0 )
        return Gzip_Err_Invalid_Stream_Operation;
    }
    else
    {
      size = stream->size - stream->pos;
      if ( size > FT_GZIP_BUFFER_SIZE )
        size = FT_GZIP_BUFFER_SIZE;

      if ( size == 0 )
        return Gzip_Err_Invalid_Stream_Operation;

      FT_MEM_COPY( zip->input, stream->base + stream->pos, size );
    }
    stream->pos += size;

    zstream->next_in  = zip->input;
    zstream->avail_in = size;

    return Gzip_Err_Ok;
  }


  static FT_Error
  ft_gzip_file_fill_output( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = 0;


    zip->cursor        = zip->buffer;
    zstream->next_out  = zip->cursor;
    zstream->avail_out = FT_GZIP_BUFFER_SIZE;

    while ( zstream->avail_out > 0 )
    {
      int  err;


      if ( zstream->avail_in == 0 )
      {
        error = ft_gzip_file_fill_input( zip );
        if ( error )
          break;
      }

      err = inflate( zstream, Z_NO_FLUSH );

      if ( err == Z_STREAM_END )
      {
        zip->limit = zstream->next_out;
        if ( zip->limit == zip->cursor )
          error = Gzip_Err_Invalid_Stream_Operation;
        break;
      }
      else if ( err != Z_OK )
      {
        error = Gzip_Err_Invalid_Stream_Operation;
        break;
      }
    }

    return error;
  }


  /* fill output buffer; `count' must be <= FT_GZIP_BUFFER_SIZE */
  static FT_Error
  ft_gzip_file_skip_output( FT_GZipFile  zip,
                            FT_ULong     count )
  {
    FT_Error  error = Gzip_Err_Ok;
    FT_ULong  delta;


    for (;;)
    {
      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

    return error;
  }


  static FT_ULong
  ft_gzip_file_io( FT_GZipFile  zip,
                   FT_ULong     pos,
                   FT_Byte*     buffer,
                   FT_ULong     count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    /* Reset inflate stream if we're seeking backwards.        */
    /* Yes, that is not too efficient, but it saves memory :-) */
    if ( pos < zip->pos )
    {
      error = ft_gzip_file_reset( zip );
      if ( error )
        goto Exit;
    }

    /* skip unwanted bytes */
    if ( pos > zip->pos )
    {
      error = ft_gzip_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
      if ( error )
        goto Exit;
    }

    if ( count == 0 )
      goto Exit;

    /* now read the data */
    for (;;)
    {
      FT_ULong  delta;


      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      FT_MEM_COPY( buffer, zip->cursor, delta );
      buffer      += delta;
      result      += delta;
      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****               G Z   E M B E D D I N G   S T R E A M             *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  static void
  ft_gzip_stream_close( FT_Stream  stream )
  {
    FT_GZipFile  zip    = (FT_GZipFile)stream->descriptor.pointer;
    FT_Memory    memory = stream->memory;


    if ( zip )
    {
      /* finalize gzip file descriptor */
      ft_gzip_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_gzip_stream_io( FT_Stream  stream,
                     FT_ULong   pos,
                     FT_Byte*   buffer,
                     FT_ULong   count )
  {
    FT_GZipFile  zip = (FT_GZipFile)stream->descriptor.pointer;


    return ft_gzip_file_io( zip, pos, buffer, count );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_Error     error;
    FT_Memory    memory = source->memory;
    FT_GZipFile  zip;


    /*
     *  check the header right now; this prevents allocating un-necessary
     *  objects when we don't need them
     */
    error = ft_gzip_check_header( source );
    if ( error )
      goto Exit;

    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_QNEW( zip ) )
    {
      error = ft_gzip_file_init( zip, stream, source );
      if ( error )
      {
        FT_FREE( zip );
        goto Exit;
      }

      stream->descriptor.pointer = zip;
    }

    stream->size  = 0x7FFFFFFFL;  /* don't know the real size! */
    stream->pos   = 0;
    stream->base  = 0;
    stream->read  = ft_gzip_stream_io;
    stream->close = ft_gzip_stream_close;

  Exit:
    return error;
  }

#else  /* !FT_CONFIG_OPTION_USE_ZLIB */

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return Gzip_Err_Unimplemented_Feature;
  }

#endif /* !FT_CONFIG_OPTION_USE_ZLIB */


/* END */

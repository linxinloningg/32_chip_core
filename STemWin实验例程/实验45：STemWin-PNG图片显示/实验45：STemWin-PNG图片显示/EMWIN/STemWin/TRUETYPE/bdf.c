/*  bdf.c

    FreeType font driver for bdf files

    Copyright (C) 2001, 2002 by
    Francesco Zappa Nardelli 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define FT_MAKE_OPTION_SINGLE_OBJECT

#include "ft2build.h"
/*
 * Copyright 2000 Computing Research Labs, New Mexico State University
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006 Francesco Zappa Nardelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

  /*************************************************************************/
  /*                                                                       */
  /*  This file is based on bdf.c,v 1.22 2000/03/16 20:08:50               */
  /*                                                                       */
  /*  taken from Mark Leisher's xmbdfed package                            */
  /*                                                                       */
  /*************************************************************************/


#include "ft2build.h"

#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "bdf.h"
#include "bdferror.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_bdflib


  /*************************************************************************/
  /*                                                                       */
  /* Default BDF font options.                                             */
  /*                                                                       */
  /*************************************************************************/


  static const bdf_options_t  _bdf_opts =
  {
    1,                /* Correct metrics.               */
    1,                /* Preserve unencoded glyphs.     */
    0,                /* Preserve comments.             */
    BDF_PROPORTIONAL  /* Default spacing.               */
  };


  /*************************************************************************/
  /*                                                                       */
  /* Builtin BDF font properties.                                          */
  /*                                                                       */
  /*************************************************************************/

  /* List of most properties that might appear in a font.  Doesn't include */
  /* the RAW_* and AXIS_* properties in X11R6 polymorphic fonts.           */

  static const bdf_property_t  _bdf_properties[] =
  {
    { (char *)"ADD_STYLE_NAME",          BDF_ATOM,     1, { 0 } },
    { (char *)"AVERAGE_WIDTH",           BDF_INTEGER,  1, { 0 } },
    { (char *)"AVG_CAPITAL_WIDTH",       BDF_INTEGER,  1, { 0 } },
    { (char *)"AVG_LOWERCASE_WIDTH",     BDF_INTEGER,  1, { 0 } },
    { (char *)"CAP_HEIGHT",              BDF_INTEGER,  1, { 0 } },
    { (char *)"CHARSET_COLLECTIONS",     BDF_ATOM,     1, { 0 } },
    { (char *)"CHARSET_ENCODING",        BDF_ATOM,     1, { 0 } },
    { (char *)"CHARSET_REGISTRY",        BDF_ATOM,     1, { 0 } },
    { (char *)"COMMENT",                 BDF_ATOM,     1, { 0 } },
    { (char *)"COPYRIGHT",               BDF_ATOM,     1, { 0 } },
    { (char *)"DEFAULT_CHAR",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"DESTINATION",             BDF_CARDINAL, 1, { 0 } },
    { (char *)"DEVICE_FONT_NAME",        BDF_ATOM,     1, { 0 } },
    { (char *)"END_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"FACE_NAME",               BDF_ATOM,     1, { 0 } },
    { (char *)"FAMILY_NAME",             BDF_ATOM,     1, { 0 } },
    { (char *)"FIGURE_WIDTH",            BDF_INTEGER,  1, { 0 } },
    { (char *)"FONT",                    BDF_ATOM,     1, { 0 } },
    { (char *)"FONTNAME_REGISTRY",       BDF_ATOM,     1, { 0 } },
    { (char *)"FONT_ASCENT",             BDF_INTEGER,  1, { 0 } },
    { (char *)"FONT_DESCENT",            BDF_INTEGER,  1, { 0 } },
    { (char *)"FOUNDRY",                 BDF_ATOM,     1, { 0 } },
    { (char *)"FULL_NAME",               BDF_ATOM,     1, { 0 } },
    { (char *)"ITALIC_ANGLE",            BDF_INTEGER,  1, { 0 } },
    { (char *)"MAX_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"MIN_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"NORM_SPACE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"NOTICE",                  BDF_ATOM,     1, { 0 } },
    { (char *)"PIXEL_SIZE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"POINT_SIZE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"QUAD_WIDTH",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_ASCENT",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVERAGE_WIDTH",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVG_CAPITAL_WIDTH",   BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVG_LOWERCASE_WIDTH", BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_CAP_HEIGHT",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_DESCENT",             BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_END_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_FIGURE_WIDTH",        BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_MAX_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_MIN_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_NORM_SPACE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_PIXEL_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_POINT_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_PIXELSIZE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_POINTSIZE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_QUAD_WIDTH",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SMALL_CAP_SIZE",      BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_STRIKEOUT_ASCENT",    BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_STRIKEOUT_DESCENT",   BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_SIZE",      BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_X",         BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_Y",         BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_SIZE",    BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_X",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_Y",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_UNDERLINE_POSITION",  BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_UNDERLINE_THICKNESS", BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_X_HEIGHT",            BDF_INTEGER,  1, { 0 } },
    { (char *)"RELATIVE_SETWIDTH",       BDF_CARDINAL, 1, { 0 } },
    { (char *)"RELATIVE_WEIGHT",         BDF_CARDINAL, 1, { 0 } },
    { (char *)"RESOLUTION",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RESOLUTION_X",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"RESOLUTION_Y",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"SETWIDTH_NAME",           BDF_ATOM,     1, { 0 } },
    { (char *)"SLANT",                   BDF_ATOM,     1, { 0 } },
    { (char *)"SMALL_CAP_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"SPACING",                 BDF_ATOM,     1, { 0 } },
    { (char *)"STRIKEOUT_ASCENT",        BDF_INTEGER,  1, { 0 } },
    { (char *)"STRIKEOUT_DESCENT",       BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_X",             BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_Y",             BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_SIZE",        BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_X",           BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_Y",           BDF_INTEGER,  1, { 0 } },
    { (char *)"UNDERLINE_POSITION",      BDF_INTEGER,  1, { 0 } },
    { (char *)"UNDERLINE_THICKNESS",     BDF_INTEGER,  1, { 0 } },
    { (char *)"WEIGHT",                  BDF_CARDINAL, 1, { 0 } },
    { (char *)"WEIGHT_NAME",             BDF_ATOM,     1, { 0 } },
    { (char *)"X_HEIGHT",                BDF_INTEGER,  1, { 0 } },
    { (char *)"_MULE_BASELINE_OFFSET",   BDF_INTEGER,  1, { 0 } },
    { (char *)"_MULE_RELATIVE_COMPOSE",  BDF_INTEGER,  1, { 0 } },
  };

  static const unsigned long
  _num_bdf_properties = sizeof ( _bdf_properties ) /
                        sizeof ( _bdf_properties[0] );


  /*************************************************************************/
  /*                                                                       */
  /* Hash table utilities for the properties.                              */
  /*                                                                       */
  /*************************************************************************/

  /* XXX: Replace this with FreeType's hash functions */


#define INITIAL_HT_SIZE  241

  typedef void
  (*hash_free_func)( hashnode  node );

  static hashnode*
  hash_bucket( const char*  key,
               hashtable*   ht )
  {
    const char*    kp  = key;
    unsigned long  res = 0;
    hashnode*      bp  = ht->table, *ndp;


    /* Mocklisp hash function. */
    while ( *kp )
      res = ( res << 5 ) - res + *kp++;

    ndp = bp + ( res % ht->size );
    while ( *ndp )
    {
      kp = (*ndp)->key;
      if ( kp[0] == key[0] && ft_strcmp( kp, key ) == 0 )
        break;
      ndp--;
      if ( ndp < bp )
        ndp = bp + ( ht->size - 1 );
    }

    return ndp;
  }


  static FT_Error
  hash_rehash( hashtable*  ht,
               FT_Memory   memory )
  {
    hashnode*  obp = ht->table, *bp, *nbp;
    int        i, sz = ht->size;
    FT_Error   error = BDF_Err_Ok;


    ht->size <<= 1;
    ht->limit  = ht->size / 3;

    if ( FT_NEW_ARRAY( ht->table, ht->size ) )
      goto Exit;

    for ( i = 0, bp = obp; i < sz; i++, bp++ )
    {
      if ( *bp )
      {
        nbp = hash_bucket( (*bp)->key, ht );
        *nbp = *bp;
      }
    }
    FT_FREE( obp );

  Exit:
    return error;
  }


  static FT_Error
  hash_init( hashtable*  ht,
             FT_Memory   memory )
  {
    int       sz = INITIAL_HT_SIZE;
    FT_Error  error = BDF_Err_Ok;


    ht->size  = sz;
    ht->limit = sz / 3;
    ht->used  = 0;

    if ( FT_NEW_ARRAY( ht->table, sz ) )
      goto Exit;

  Exit:
    return error;
  }


  static void
  hash_free( hashtable*  ht,
             FT_Memory   memory )
  {
    if ( ht != 0 )
    {
      int        i, sz = ht->size;
      hashnode*  bp = ht->table;


      for ( i = 0; i < sz; i++, bp++ )
        FT_FREE( *bp );

      FT_FREE( ht->table );
    }
  }


  static FT_Error
  hash_insert( char*       key,
               void*       data,
               hashtable*  ht,
               FT_Memory   memory )
  {
    hashnode  nn, *bp = hash_bucket( key, ht );
    FT_Error  error = BDF_Err_Ok;


    nn = *bp;
    if ( !nn )
    {
      if ( FT_NEW( nn ) )
        goto Exit;
      *bp = nn;

      nn->key  = key;
      nn->data = data;

      if ( ht->used >= ht->limit )
      {
        error = hash_rehash( ht, memory );
        if ( error )
          goto Exit;
      }
      ht->used++;
    }
    else
      nn->data = data;

  Exit:
    return error;
  }


  static hashnode
  hash_lookup( const char* key,
               hashtable*  ht )
  {
    hashnode *np = hash_bucket( key, ht );


    return *np;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Utility types and functions.                                          */
  /*                                                                       */
  /*************************************************************************/


  /* Function type for parsing lines of a BDF font. */

  typedef FT_Error
  (*_bdf_line_func_t)( char*          line,
                       unsigned long  linelen,
                       unsigned long  lineno,
                       void*          call_data,
                       void*          client_data );


  /* List structure for splitting lines into fields. */

  typedef struct  _bdf_list_t_
  {
    char**         field;
    unsigned long  size;
    unsigned long  used;
    FT_Memory      memory;

  } _bdf_list_t;


  /* Structure used while loading BDF fonts. */

  typedef struct  _bdf_parse_t_
  {
    unsigned long   flags;
    unsigned long   cnt;
    unsigned long   row;

    short           minlb;
    short           maxlb;
    short           maxrb;
    short           maxas;
    short           maxds;

    short           rbearing;

    char*           glyph_name;
    long            glyph_enc;

    bdf_font_t*     font;
    bdf_options_t*  opts;

    unsigned long   have[2048];
    _bdf_list_t     list;

    FT_Memory       memory;

  } _bdf_parse_t;


#define setsbit( m, cc )  ( m[(cc) >> 3] |= (FT_Byte)( 1 << ( (cc) & 7 ) ) )
#define sbitset( m, cc )  ( m[(cc) >> 3]  & ( 1 << ( (cc) & 7 ) ) )


  static void
  _bdf_list_init( _bdf_list_t*  list,
                  FT_Memory     memory )
  {
    FT_ZERO( list );
    list->memory = memory;
  }


  static void
  _bdf_list_done( _bdf_list_t*  list )
  {
    FT_Memory  memory = list->memory;


    if ( memory )
    {
      FT_FREE( list->field );
      FT_ZERO( list );
    }
  }


  static FT_Error
  _bdf_list_ensure( _bdf_list_t*  list,
                    int           num_items )
  {
    FT_Error  error = BDF_Err_Ok;


    if ( num_items > (int)list->size )
    {
      int        oldsize = list->size;
      int        newsize = oldsize + ( oldsize >> 1 ) + 4;
      int        bigsize = FT_INT_MAX / sizeof ( char* );
      FT_Memory  memory  = list->memory;


      if ( oldsize == bigsize )
      {
        error = BDF_Err_Out_Of_Memory;
        goto Exit;
      }
      else if ( newsize < oldsize || newsize > bigsize )
        newsize = bigsize;

      if ( FT_RENEW_ARRAY( list->field, oldsize, newsize ) )
        goto Exit;

      list->size = newsize;
    }

  Exit:
    return error;
  }


  static void
  _bdf_list_shift( _bdf_list_t*   list,
                   unsigned long  n )
  {
    unsigned long  i, u;


    if ( list == 0 || list->used == 0 || n == 0 )
      return;

    if ( n >= list->used )
    {
      list->used = 0;
      return;
    }

    for ( u = n, i = 0; u < list->used; i++, u++ )
      list->field[i] = list->field[u];
    list->used -= n;
  }


  static char *
  _bdf_list_join( _bdf_list_t*    list,
                  int             c,
                  unsigned long  *alen )
  {
    unsigned long  i, j;
    char           *fp, *dp;


    *alen = 0;

    if ( list == 0 || list->used == 0 )
      return 0;

    dp = list->field[0];
    for ( i = j = 0; i < list->used; i++ )
    {
      fp = list->field[i];
      while ( *fp )
        dp[j++] = *fp++;

      if ( i + 1 < list->used )
        dp[j++] = (char)c;
    }
    dp[j] = 0;

    *alen = j;
    return dp;
  }


  /* An empty string for empty fields. */

  static const char  empty[1] = { 0 };      /* XXX eliminate this */


  static FT_Error
  _bdf_list_split( _bdf_list_t*   list,
                   char*          separators,
                   char*          line,
                   unsigned long  linelen )
  {
    int       mult, final_empty;
    char      *sp, *ep, *end;
    char      seps[32];
    FT_Error  error = BDF_Err_Ok;


    /* Initialize the list. */
    list->used = 0;

    /* If the line is empty, then simply return. */
    if ( linelen == 0 || line[0] == 0 )
      goto Exit;

    /* In the original code, if the `separators' parameter is NULL or */
    /* empty, the list is split into individual bytes.  We don't need */
    /* this, so an error is signaled.                                 */
    if ( separators == 0 || *separators == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    /* Prepare the separator bitmap. */
    FT_MEM_ZERO( seps, 32 );

    /* If the very last character of the separator string is a plus, then */
    /* set the `mult' flag to indicate that multiple separators should be */
    /* collapsed into one.                                                */
    for ( mult = 0, sp = separators; sp && *sp; sp++ )
    {
      if ( *sp == '+' && *( sp + 1 ) == 0 )
        mult = 1;
      else
        setsbit( seps, *sp );
    }

    /* Break the line up into fields. */
    for ( final_empty = 0, sp = ep = line, end = sp + linelen;
          sp < end && *sp; )
    {
      /* Collect everything that is not a separator. */
      for ( ; *ep && !sbitset( seps, *ep ); ep++ )
        ;

      /* Resize the list if necessary. */
      if ( list->used == list->size )
      {
        error = _bdf_list_ensure( list, list->used + 1 );
        if ( error )
          goto Exit;
      }

      /* Assign the field appropriately. */
      list->field[list->used++] = ( ep > sp ) ? sp : (char*)empty;

      sp = ep;

      if ( mult )
      {
        /* If multiple separators should be collapsed, do it now by */
        /* setting all the separator characters to 0.               */
        for ( ; *ep && sbitset( seps, *ep ); ep++ )
          *ep = 0;
      }
      else if ( *ep != 0 )
        /* Don't collapse multiple separators by making them 0, so just */
        /* make the one encountered 0.                                  */
        *ep++ = 0;

      final_empty = ( ep > sp && *ep == 0 );
      sp = ep;
    }

    /* Finally, NULL-terminate the list. */
    if ( list->used + final_empty >= list->size )
    {
      error = _bdf_list_ensure( list, list->used + final_empty + 1 );
      if ( error )
        goto Exit;
    }

    if ( final_empty )
      list->field[list->used++] = (char*)empty;

    list->field[list->used] = 0;

  Exit:
    return error;
  }


#define NO_SKIP  256  /* this value cannot be stored in a 'char' */


  static FT_Error
  _bdf_readstream( FT_Stream         stream,
                   _bdf_line_func_t  callback,
                   void*             client_data,
                   unsigned long    *lno )
  {
    _bdf_line_func_t  cb;
    unsigned long     lineno, buf_size;
    int               refill, bytes, hold, to_skip;
    int               start, end, cursor, avail;
    char*             buf = 0;
    FT_Memory         memory = stream->memory;
    FT_Error          error = BDF_Err_Ok;


    if ( callback == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    /* initial size and allocation of the input buffer */
    buf_size = 1024;

    if ( FT_NEW_ARRAY( buf, buf_size ) )
      goto Exit;

    cb      = callback;
    lineno  = 1;
    buf[0]  = 0;
    start   = 0;
    end     = 0;
    avail   = 0;
    cursor  = 0;
    refill  = 1;
    to_skip = NO_SKIP;
    bytes   = 0;        /* make compiler happy */

    for (;;)
    {
      if ( refill )
      {
        bytes  = (int)FT_Stream_TryRead( stream, (FT_Byte*)buf + cursor,
                                         (FT_ULong)(buf_size - cursor) );
        avail  = cursor + bytes;
        cursor = 0;
        refill = 0;
      }

      end = start;

      /* should we skip an optional character like \n or \r? */
      if ( start < avail && buf[start] == to_skip )
      {
        start  += 1;
        to_skip = NO_SKIP;
        continue;
      }

      /* try to find the end of the line */
      while ( end < avail && buf[end] != '\n' && buf[end] != '\r' )
        end++;

      /* if we hit the end of the buffer, try shifting its content */
      /* or even resizing it                                       */
      if ( end >= avail )
      {
        if ( bytes == 0 )  /* last line in file doesn't end in \r or \n */
          break;           /* ignore it then exit                       */

        if ( start == 0 )
        {
          /* this line is definitely too long; try resizing the input */
          /* buffer a bit to handle it.                               */
          FT_ULong  new_size;


          if ( buf_size >= 65536UL )  /* limit ourselves to 64KByte */
          {
            error = BDF_Err_Invalid_Argument;
            goto Exit;
          }

          new_size = buf_size * 2;
          if ( FT_RENEW_ARRAY( buf, buf_size, new_size ) )
            goto Exit;

          cursor   = buf_size;
          buf_size = new_size;
        }
        else
        {
          bytes = avail - start;

          FT_MEM_COPY( buf, buf + start, bytes );

          cursor = bytes;
          avail -= bytes;
          start  = 0;
        }
        refill = 1;
        continue;
      }

      /* Temporarily NUL-terminate the line. */
      hold     = buf[end];
      buf[end] = 0;

      /* XXX: Use encoding independent value for 0x1a */
      if ( buf[start] != '#' && buf[start] != 0x1a && end > start )
      {
        error = (*cb)( buf + start, end - start, lineno,
                       (void*)&cb, client_data );
        if ( error )
          break;
      }

      lineno  += 1;
      buf[end] = (char)hold;
      start    = end + 1;

      if ( hold == '\n' )
        to_skip = '\r';
      else if ( hold == '\r' )
        to_skip = '\n';
      else
        to_skip = NO_SKIP;
    }

    *lno = lineno;

  Exit:
    FT_FREE( buf );
    return error;
  }


  /* XXX: make this work with EBCDIC also */

  static const unsigned char  a2i[128] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  static const unsigned char  odigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  static const unsigned char  ddigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  static const unsigned char  hdigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03,
    0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };


#define isdigok( m, d )  (m[(d) >> 3] & ( 1 << ( (d) & 7 ) ) )


  /* Routine to convert an ASCII string into an unsigned long integer. */
  static unsigned long
  _bdf_atoul( char*   s,
              char**  end,
              int     base )
  {
    unsigned long         v;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    /* Make sure the radix is something recognizable.  Default to 10. */
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    /* Check for the special hex prefix. */
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = v * base + a2i[(int)*s];

    if ( end != 0 )
      *end = s;

    return v;
  }


  /* Routine to convert an ASCII string into an signed long integer. */
  static long
  _bdf_atol( char*   s,
             char**  end,
             int     base )
  {
    long                  v, neg;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    /* Make sure the radix is something recognizable.  Default to 10. */
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    /* Check for a minus sign. */
    neg = 0;
    if ( *s == '-' )
    {
      s++;
      neg = 1;
    }

    /* Check for the special hex prefix. */
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = v * base + a2i[(int)*s];

    if ( end != 0 )
      *end = s;

    return ( !neg ) ? v : -v;
  }


  /* Routine to convert an ASCII string into an signed short integer. */
  static short
  _bdf_atos( char*   s,
             char**  end,
             int     base )
  {
    short                 v, neg;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    /* Make sure the radix is something recognizable.  Default to 10. */
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    /* Check for a minus. */
    neg = 0;
    if ( *s == '-' )
    {
      s++;
      neg = 1;
    }

    /* Check for the special hex prefix. */
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = (short)( v * base + a2i[(int)*s] );

    if ( end != 0 )
      *end = s;

    return (short)( ( !neg ) ? v : -v );
  }


  /* Routine to compare two glyphs by encoding so they can be sorted. */
  static int
  by_encoding( const void*  a,
               const void*  b )
  {
    bdf_glyph_t  *c1, *c2;


    c1 = (bdf_glyph_t *)a;
    c2 = (bdf_glyph_t *)b;

    if ( c1->encoding < c2->encoding )
      return -1;

    if ( c1->encoding > c2->encoding )
      return 1;

    return 0;
  }


  static FT_Error
  bdf_create_property( char*        name,
                       int          format,
                       bdf_font_t*  font )
  {
    unsigned long    n;
    bdf_property_t*  p;
    FT_Memory        memory = font->memory;
    FT_Error         error = BDF_Err_Ok;


    /* First check to see if the property has      */
    /* already been added or not.  If it has, then */
    /* simply ignore it.                           */
    if ( hash_lookup( name, &(font->proptbl) ) )
      goto Exit;

    if ( FT_RENEW_ARRAY( font->user_props,
                         font->nuser_props,
                         font->nuser_props + 1 ) )
      goto Exit;

    p = font->user_props + font->nuser_props;
    FT_ZERO( p );

    n = (unsigned long)( ft_strlen( name ) + 1 );

    if ( FT_NEW_ARRAY( p->name, n ) )
      goto Exit;

    FT_MEM_COPY( (char *)p->name, name, n );

    p->format  = format;
    p->builtin = 0;

    n = _num_bdf_properties + font->nuser_props;

    error = hash_insert( p->name, (void *)n, &(font->proptbl), memory );
    if ( error )
      goto Exit;

    font->nuser_props++;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( bdf_property_t * )
  bdf_get_property( char*        name,
                    bdf_font_t*  font )
  {
    hashnode       hn;
    unsigned long  propid;


    if ( name == 0 || *name == 0 )
      return 0;

    if ( ( hn = hash_lookup( name, &(font->proptbl) ) ) == 0 )
      return 0;

    propid = (unsigned long)hn->data;
    if ( propid >= _num_bdf_properties )
      return font->user_props + ( propid - _num_bdf_properties );

    return (bdf_property_t*)_bdf_properties + propid;
  }


  /*************************************************************************/
  /*                                                                       */
  /* BDF font file parsing flags and functions.                            */
  /*                                                                       */
  /*************************************************************************/


  /* Parse flags. */

#define _BDF_START      0x0001
#define _BDF_FONT_NAME  0x0002
#define _BDF_SIZE       0x0004
#define _BDF_FONT_BBX   0x0008
#define _BDF_PROPS      0x0010
#define _BDF_GLYPHS     0x0020
#define _BDF_GLYPH      0x0040
#define _BDF_ENCODING   0x0080
#define _BDF_SWIDTH     0x0100
#define _BDF_DWIDTH     0x0200
#define _BDF_BBX        0x0400
#define _BDF_BITMAP     0x0800

#define _BDF_SWIDTH_ADJ  0x1000

#define _BDF_GLYPH_BITS ( _BDF_GLYPH    | \
                          _BDF_ENCODING | \
                          _BDF_SWIDTH   | \
                          _BDF_DWIDTH   | \
                          _BDF_BBX      | \
                          _BDF_BITMAP   )

#define _BDF_GLYPH_WIDTH_CHECK   0x40000000UL
#define _BDF_GLYPH_HEIGHT_CHECK  0x80000000UL


  /* Auto correction messages. */
#define ACMSG1   "FONT_ASCENT property missing.  " \
                 "Added \"FONT_ASCENT %hd\".\n"
#define ACMSG2   "FONT_DESCENT property missing.  " \
                 "Added \"FONT_DESCENT %hd\".\n"
#define ACMSG3   "Font width != actual width.  Old: %hd New: %hd.\n"
#define ACMSG4   "Font left bearing != actual left bearing.  " \
                 "Old: %hd New: %hd.\n"
#define ACMSG5   "Font ascent != actual ascent.  Old: %hd New: %hd.\n"
#define ACMSG6   "Font descent != actual descent.  Old: %hd New: %hd.\n"
#define ACMSG7   "Font height != actual height. Old: %hd New: %hd.\n"
#define ACMSG8   "Glyph scalable width (SWIDTH) adjustments made.\n"
#define ACMSG9   "SWIDTH field missing at line %ld.  Set automatically.\n"
#define ACMSG10  "DWIDTH field missing at line %ld.  Set to glyph width.\n"
#define ACMSG11  "SIZE bits per pixel field adjusted to %hd.\n"
#define ACMSG12  "Duplicate encoding %ld (%s) changed to unencoded.\n"
#define ACMSG13  "Glyph %ld extra rows removed.\n"
#define ACMSG14  "Glyph %ld extra columns removed.\n"
#define ACMSG15  "Incorrect glyph count: %ld indicated but %ld found.\n"

  /* Error messages. */
#define ERRMSG1  "[line %ld] Missing \"%s\" line.\n"
#define ERRMSG2  "[line %ld] Font header corrupted or missing fields.\n"
#define ERRMSG3  "[line %ld] Font glyphs corrupted or missing fields.\n"
#define ERRMSG4  "[line %ld] BBX too big.\n"


  static FT_Error
  _bdf_add_comment( bdf_font_t*    font,
                    char*          comment,
                    unsigned long  len )
  {
    char*      cp;
    FT_Memory  memory = font->memory;
    FT_Error   error = BDF_Err_Ok;


    if ( FT_RENEW_ARRAY( font->comments,
                         font->comments_len,
                         font->comments_len + len + 1 ) )
      goto Exit;

    cp = font->comments + font->comments_len;

    FT_MEM_COPY( cp, comment, len );
    cp[len] = '\n';

    font->comments_len += len + 1;

  Exit:
    return error;
  }


  /* Set the spacing from the font name if it exists, or set it to the */
  /* default specified in the options.                                 */
  static FT_Error
  _bdf_set_default_spacing( bdf_font_t*     font,
                            bdf_options_t*  opts )
  {
    unsigned long  len;
    char           name[128];
    _bdf_list_t    list;
    FT_Memory      memory;
    FT_Error       error = BDF_Err_Ok;


    if ( font == 0 || font->name == 0 || font->name[0] == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    memory = font->memory;

    _bdf_list_init( &list, memory );

    font->spacing = opts->font_spacing;

    len = (unsigned long)( ft_strlen( font->name ) + 1 );
    FT_MEM_COPY( name, font->name, len );

    error = _bdf_list_split( &list, (char *)"-", name, len );
    if ( error )
      goto Fail;

    if ( list.used == 15 )
    {
      switch ( list.field[11][0] )
      {
      case 'C':
      case 'c':
        font->spacing = BDF_CHARCELL;
        break;
      case 'M':
      case 'm':
        font->spacing = BDF_MONOWIDTH;
        break;
      case 'P':
      case 'p':
        font->spacing = BDF_PROPORTIONAL;
        break;
      }
    }

  Fail:
    _bdf_list_done( &list );

  Exit:
    return error;
  }


  /* Determine whether the property is an atom or not.  If it is, then */
  /* clean it up so the double quotes are removed if they exist.       */
  static int
  _bdf_is_atom( char*          line,
                unsigned long  linelen,
                char**         name,
                char**         value,
                bdf_font_t*    font )
  {
    int              hold;
    char             *sp, *ep;
    bdf_property_t*  p;


    *name = sp = ep = line;

    while ( *ep && *ep != ' ' && *ep != '\t' )
      ep++;

    hold = -1;
    if ( *ep )
    {
      hold = *ep;
      *ep  = 0;
    }

    p = bdf_get_property( sp, font );

    /* Restore the character that was saved before any return can happen. */
    if ( hold != -1 )
      *ep = (char)hold;

    /* If the property exists and is not an atom, just return here. */
    if ( p && p->format != BDF_ATOM )
      return 0;

    /* The property is an atom.  Trim all leading and trailing whitespace */
    /* and double quotes for the atom value.                              */
    sp = ep;
    ep = line + linelen;

    /* Trim the leading whitespace if it exists. */
    *sp++ = 0;
    while ( *sp                           &&
            ( *sp == ' ' || *sp == '\t' ) )
      sp++;

    /* Trim the leading double quote if it exists. */
    if ( *sp == '"' )
      sp++;
    *value = sp;

    /* Trim the trailing whitespace if it exists. */
    while ( ep > sp                                       &&
            ( *( ep - 1 ) == ' ' || *( ep - 1 ) == '\t' ) )
      *--ep = 0;

    /* Trim the trailing double quote if it exists. */
    if ( ep > sp && *( ep - 1 ) == '"' )
      *--ep = 0;

    return 1;
  }


  static FT_Error
  _bdf_add_property( bdf_font_t*  font,
                     char*        name,
                     char*        value )
  {
    unsigned long   propid;
    hashnode        hn;
    int             len;
    bdf_property_t  *prop, *fp;
    FT_Memory       memory = font->memory;
    FT_Error        error = BDF_Err_Ok;


    /* First, check to see if the property already exists in the font. */
    if ( ( hn = hash_lookup( name, (hashtable *)font->internal ) ) != 0 )
    {
      /* The property already exists in the font, so simply replace */
      /* the value of the property with the current value.          */
      fp = font->props + (unsigned long)hn->data;

      switch ( fp->format )
      {
      case BDF_ATOM:
        /* Delete the current atom if it exists. */
        FT_FREE( fp->value.atom );

        if ( value == 0 )
          len = 1;
        else
          len = ft_strlen( value ) + 1;

        if ( len > 1 )
        {
          if ( FT_NEW_ARRAY( fp->value.atom, len ) )
            goto Exit;
          FT_MEM_COPY( fp->value.atom, value, len );
        }
        else
          fp->value.atom = 0;
        break;

      case BDF_INTEGER:
        fp->value.int32 = _bdf_atol( value, 0, 10 );
        break;

      case BDF_CARDINAL:
        fp->value.card32 = _bdf_atoul( value, 0, 10 );
        break;

      default:
        ;
      }

      goto Exit;
    }

    /* See whether this property type exists yet or not. */
    /* If not, create it.                                */
    hn = hash_lookup( name, &(font->proptbl) );
    if ( hn == 0 )
    {
      error = bdf_create_property( name, BDF_ATOM, font );
      if ( error )
        goto Exit;
      hn = hash_lookup( name, &(font->proptbl) );
    }

    /* Allocate another property if this is overflow. */
    if ( font->props_used == font->props_size )
    {
      if ( font->props_size == 0 )
      {
        if ( FT_NEW_ARRAY( font->props, 1 ) )
          goto Exit;
      }
      else
      {
        if ( FT_RENEW_ARRAY( font->props,
                             font->props_size,
                             font->props_size + 1 ) )
          goto Exit;
      }

      fp = font->props + font->props_size;
      FT_MEM_ZERO( fp, sizeof ( bdf_property_t ) );
      font->props_size++;
    }

    propid = (unsigned long)hn->data;
    if ( propid >= _num_bdf_properties )
      prop = font->user_props + ( propid - _num_bdf_properties );
    else
      prop = (bdf_property_t*)_bdf_properties + propid;

    fp = font->props + font->props_used;

    fp->name    = prop->name;
    fp->format  = prop->format;
    fp->builtin = prop->builtin;

    switch ( prop->format )
    {
    case BDF_ATOM:
      if ( value == 0 )
        len = 1;
      else
        len = ft_strlen( value ) + 1;

      if ( len > 1 )
      {
        if ( FT_NEW_ARRAY( fp->value.atom, len ) )
          goto Exit;
        FT_MEM_COPY( fp->value.atom, value, len );
      }
      else
        fp->value.atom = 0;
      break;

    case BDF_INTEGER:
      fp->value.int32 = _bdf_atol( value, 0, 10 );
      break;

    case BDF_CARDINAL:
      fp->value.card32 = _bdf_atoul( value, 0, 10 );
      break;
    }

    /* If the property happens to be a comment, then it doesn't need */
    /* to be added to the internal hash table.                       */
    if ( ft_memcmp( name, "COMMENT", 7 ) != 0 ) {
      /* Add the property to the font property table. */
      error = hash_insert( fp->name,
                           (void *)font->props_used,
                           (hashtable *)font->internal,
                           memory );
      if ( error )
        goto Exit;
    }

    font->props_used++;

    /* Some special cases need to be handled here.  The DEFAULT_CHAR       */
    /* property needs to be located if it exists in the property list, the */
    /* FONT_ASCENT and FONT_DESCENT need to be assigned if they are        */
    /* present, and the SPACING property should override the default       */
    /* spacing.                                                            */
    if ( ft_memcmp( name, "DEFAULT_CHAR", 12 ) == 0 )
      font->default_char = fp->value.int32;
    else if ( ft_memcmp( name, "FONT_ASCENT", 11 ) == 0 )
      font->font_ascent = fp->value.int32;
    else if ( ft_memcmp( name, "FONT_DESCENT", 12 ) == 0 )
      font->font_descent = fp->value.int32;
    else if ( ft_memcmp( name, "SPACING", 7 ) == 0 )
    {
      if ( fp->value.atom[0] == 'p' || fp->value.atom[0] == 'P' )
        font->spacing = BDF_PROPORTIONAL;
      else if ( fp->value.atom[0] == 'm' || fp->value.atom[0] == 'M' )
        font->spacing = BDF_MONOWIDTH;
      else if ( fp->value.atom[0] == 'c' || fp->value.atom[0] == 'C' )
        font->spacing = BDF_CHARCELL;
    }

  Exit:
    return error;
  }


  static const unsigned char nibble_mask[8] =
  {
    0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE
  };


  /* Actually parse the glyph info and bitmaps. */
  static FT_Error
  _bdf_parse_glyphs( char*          line,
                     unsigned long  linelen,
                     unsigned long  lineno,
                     void*          call_data,
                     void*          client_data )
  {
    int                c, mask_index;
    char*              s;
    unsigned char*     bp;
    unsigned long      i, slen, nibbles;

    _bdf_parse_t*      p;
    bdf_glyph_t*       glyph;
    bdf_font_t*        font;

    FT_Memory          memory;
    FT_Error           error = BDF_Err_Ok;

    FT_UNUSED( call_data );
    FT_UNUSED( lineno );        /* only used in debug mode */


    p = (_bdf_parse_t *)client_data;

    font   = p->font;
    memory = font->memory;

    /* Check for a comment. */
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      linelen -= 7;

      s = line + 7;
      if ( *s != 0 )
      {
        s++;
        linelen--;
      }
      error = _bdf_add_comment( p->font, s, linelen );
      goto Exit;
    }

    /* The very first thing expected is the number of glyphs. */
    if ( !( p->flags & _BDF_GLYPHS ) )
    {
      if ( ft_memcmp( line, "CHARS", 5 ) != 0 )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "CHARS" ));
        error = BDF_Err_Missing_Chars_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      p->cnt = font->glyphs_size = _bdf_atoul( p->list.field[1], 0, 10 );

      /* Make sure the number of glyphs is non-zero. */
      if ( p->cnt == 0 )
        font->glyphs_size = 64;

      if ( FT_NEW_ARRAY( font->glyphs, font->glyphs_size ) )
        goto Exit;

      p->flags |= _BDF_GLYPHS;

      goto Exit;
    }

    /* Check for the ENDFONT field. */
    if ( ft_memcmp( line, "ENDFONT", 7 ) == 0 )
    {
      /* Sort the glyphs by encoding. */
      ft_qsort( (char *)font->glyphs,
                font->glyphs_used,
                sizeof ( bdf_glyph_t ),
                by_encoding );

      p->flags &= ~_BDF_START;

      goto Exit;
    }

    /* Check for the ENDCHAR field. */
    if ( ft_memcmp( line, "ENDCHAR", 7 ) == 0 )
    {
      p->glyph_enc = 0;
      p->flags    &= ~_BDF_GLYPH_BITS;

      goto Exit;
    }

    /* Check to see whether a glyph is being scanned but should be */
    /* ignored because it is an unencoded glyph.                   */
    if ( ( p->flags & _BDF_GLYPH )     &&
         p->glyph_enc            == -1 &&
         p->opts->keep_unencoded == 0  )
      goto Exit;

    /* Check for the STARTCHAR field. */
    if ( ft_memcmp( line, "STARTCHAR", 9 ) == 0 )
    {
      /* Set the character name in the parse info first until the */
      /* encoding can be checked for an unencoded character.      */
      FT_FREE( p->glyph_name );

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      _bdf_list_shift( &p->list, 1 );

      s = _bdf_list_join( &p->list, ' ', &slen );

      if ( FT_NEW_ARRAY( p->glyph_name, slen + 1 ) )
        goto Exit;

      FT_MEM_COPY( p->glyph_name, s, slen + 1 );

      p->flags |= _BDF_GLYPH;

      goto Exit;
    }

    /* Check for the ENCODING field. */
    if ( ft_memcmp( line, "ENCODING", 8 ) == 0 )
    {
      if ( !( p->flags & _BDF_GLYPH ) )
      {
        /* Missing STARTCHAR field. */
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "STARTCHAR" ));
        error = BDF_Err_Missing_Startchar_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->glyph_enc = _bdf_atol( p->list.field[1], 0, 10 );

      /* Check that the encoding is in the range [0,65536] because        */
      /* otherwise p->have (a bitmap with static size) overflows.         */
      if ( (size_t)p->glyph_enc >= sizeof ( p->have ) * 8 )
      {
        error = BDF_Err_Invalid_File_Format;
        goto Exit;
      }

      /* Check to see whether this encoding has already been encountered. */
      /* If it has then change it to unencoded so it gets added if        */
      /* indicated.                                                       */
      if ( p->glyph_enc >= 0 )
      {
        if ( _bdf_glyph_modified( p->have, p->glyph_enc ) )
        {
          /* Emit a message saying a glyph has been moved to the */
          /* unencoded area.                                     */
          FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG12,
                      p->glyph_enc, p->glyph_name ));
          p->glyph_enc = -1;
          font->modified = 1;
        }
        else
          _bdf_set_glyph_modified( p->have, p->glyph_enc );
      }

      if ( p->glyph_enc >= 0 )
      {
        /* Make sure there are enough glyphs allocated in case the */
        /* number of characters happen to be wrong.                */
        if ( font->glyphs_used == font->glyphs_size )
        {
          if ( FT_RENEW_ARRAY( font->glyphs,
                               font->glyphs_size,
                               font->glyphs_size + 64 ) )
            goto Exit;

          font->glyphs_size += 64;
        }

        glyph           = font->glyphs + font->glyphs_used++;
        glyph->name     = p->glyph_name;
        glyph->encoding = p->glyph_enc;

        /* Reset the initial glyph info. */
        p->glyph_name = 0;
      }
      else
      {
        /* Unencoded glyph.  Check to see whether it should */
        /* be added or not.                                 */
        if ( p->opts->keep_unencoded != 0 )
        {
          /* Allocate the next unencoded glyph. */
          if ( font->unencoded_used == font->unencoded_size )
          {
            if ( FT_RENEW_ARRAY( font->unencoded ,
                                 font->unencoded_size,
                                 font->unencoded_size + 4 ) )
              goto Exit;

            font->unencoded_size += 4;
          }

          glyph           = font->unencoded + font->unencoded_used;
          glyph->name     = p->glyph_name;
          glyph->encoding = font->unencoded_used++;
        }
        else
          /* Free up the glyph name if the unencoded shouldn't be */
          /* kept.                                                */
          FT_FREE( p->glyph_name );

        p->glyph_name = 0;
      }

      /* Clear the flags that might be added when width and height are */
      /* checked for consistency.                                      */
      p->flags &= ~( _BDF_GLYPH_WIDTH_CHECK | _BDF_GLYPH_HEIGHT_CHECK );

      p->flags |= _BDF_ENCODING;

      goto Exit;
    }

    /* Point at the glyph being constructed. */
    if ( p->glyph_enc == -1 )
      glyph = font->unencoded + ( font->unencoded_used - 1 );
    else
      glyph = font->glyphs + ( font->glyphs_used - 1 );

    /* Check to see whether a bitmap is being constructed. */
    if ( p->flags & _BDF_BITMAP )
    {
      /* If there are more rows than are specified in the glyph metrics, */
      /* ignore the remaining lines.                                     */
      if ( p->row >= (unsigned long)glyph->bbx.height )
      {
        if ( !( p->flags & _BDF_GLYPH_HEIGHT_CHECK ) )
        {
          FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG13, glyph->encoding ));
          p->flags |= _BDF_GLYPH_HEIGHT_CHECK;
          font->modified = 1;
        }

        goto Exit;
      }

      /* Only collect the number of nibbles indicated by the glyph     */
      /* metrics.  If there are more columns, they are simply ignored. */
      nibbles = glyph->bpr << 1;
      bp      = glyph->bitmap + p->row * glyph->bpr;

      for ( i = 0; i < nibbles; i++ )
      {
        c = line[i];
        *bp = (FT_Byte)( ( *bp << 4 ) + a2i[c] );
        if ( i + 1 < nibbles && ( i & 1 ) )
          *++bp = 0;
      }

      /* Remove possible garbage at the right. */
      mask_index = ( glyph->bbx.width * p->font->bpp ) & 7;
      if ( glyph->bbx.width )
        *bp &= nibble_mask[mask_index];

      /* If any line has extra columns, indicate they have been removed. */
      if ( ( line[nibbles] == '0' || a2i[(int)line[nibbles]] != 0 ) &&
           !( p->flags & _BDF_GLYPH_WIDTH_CHECK )                   )
      {
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG14, glyph->encoding ));
        p->flags       |= _BDF_GLYPH_WIDTH_CHECK;
        font->modified  = 1;
      }

      p->row++;
      goto Exit;
    }

    /* Expect the SWIDTH (scalable width) field next. */
    if ( ft_memcmp( line, "SWIDTH", 6 ) == 0 )
    {
      if ( !( p->flags & _BDF_ENCODING ) )
      {
        /* Missing ENCODING field. */
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "ENCODING" ));
        error = BDF_Err_Missing_Encoding_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->swidth = (unsigned short)_bdf_atoul( p->list.field[1], 0, 10 );
      p->flags |= _BDF_SWIDTH;

      goto Exit;
    }

    /* Expect the DWIDTH (scalable width) field next. */
    if ( ft_memcmp( line, "DWIDTH", 6 ) == 0 )
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->dwidth = (unsigned short)_bdf_atoul( p->list.field[1], 0, 10 );

      if ( !( p->flags & _BDF_SWIDTH ) )
      {
        /* Missing SWIDTH field.  Emit an auto correction message and set */
        /* the scalable width from the device width.                      */
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG9, lineno ));

        glyph->swidth = (unsigned short)FT_MulDiv(
                          glyph->dwidth, 72000L,
                          (FT_Long)( font->point_size *
                                     font->resolution_x ) );
      }

      p->flags |= _BDF_DWIDTH;
      goto Exit;
    }

    /* Expect the BBX field next. */
    if ( ft_memcmp( line, "BBX", 3 ) == 0 )
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->bbx.width    = _bdf_atos( p->list.field[1], 0, 10 );
      glyph->bbx.height   = _bdf_atos( p->list.field[2], 0, 10 );
      glyph->bbx.x_offset = _bdf_atos( p->list.field[3], 0, 10 );
      glyph->bbx.y_offset = _bdf_atos( p->list.field[4], 0, 10 );

      /* Generate the ascent and descent of the character. */
      glyph->bbx.ascent  = (short)( glyph->bbx.height + glyph->bbx.y_offset );
      glyph->bbx.descent = (short)( -glyph->bbx.y_offset );

      /* Determine the overall font bounding box as the characters are */
      /* loaded so corrections can be done later if indicated.         */
      p->maxas    = (short)FT_MAX( glyph->bbx.ascent, p->maxas );
      p->maxds    = (short)FT_MAX( glyph->bbx.descent, p->maxds );

      p->rbearing = (short)( glyph->bbx.width + glyph->bbx.x_offset );

      p->maxrb    = (short)FT_MAX( p->rbearing, p->maxrb );
      p->minlb    = (short)FT_MIN( glyph->bbx.x_offset, p->minlb );
      p->maxlb    = (short)FT_MAX( glyph->bbx.x_offset, p->maxlb );

      if ( !( p->flags & _BDF_DWIDTH ) )
      {
        /* Missing DWIDTH field.  Emit an auto correction message and set */
        /* the device width to the glyph width.                           */
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG10, lineno ));
        glyph->dwidth = glyph->bbx.width;
      }

      /* If the BDF_CORRECT_METRICS flag is set, then adjust the SWIDTH */
      /* value if necessary.                                            */
      if ( p->opts->correct_metrics != 0 )
      {
        /* Determine the point size of the glyph. */
        unsigned short  sw = (unsigned short)FT_MulDiv(
                               glyph->dwidth, 72000L,
                               (FT_Long)( font->point_size *
                                          font->resolution_x ) );


        if ( sw != glyph->swidth )
        {
          glyph->swidth = sw;

          if ( p->glyph_enc == -1 )
            _bdf_set_glyph_modified( font->umod,
                                     font->unencoded_used - 1 );
          else
            _bdf_set_glyph_modified( font->nmod, glyph->encoding );

          p->flags       |= _BDF_SWIDTH_ADJ;
          font->modified  = 1;
        }
      }

      p->flags |= _BDF_BBX;
      goto Exit;
    }

    /* And finally, gather up the bitmap. */
    if ( ft_memcmp( line, "BITMAP", 6 ) == 0 )
    {
      unsigned long  bitmap_size;


      if ( !( p->flags & _BDF_BBX ) )
      {
        /* Missing BBX field. */
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "BBX" ));
        error = BDF_Err_Missing_Bbx_Field;
        goto Exit;
      }

      /* Allocate enough space for the bitmap. */
      glyph->bpr   = ( glyph->bbx.width * p->font->bpp + 7 ) >> 3;

      bitmap_size = glyph->bpr * glyph->bbx.height;
      if ( bitmap_size > 0xFFFFU )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG4, lineno ));
        error = BDF_Err_Bbx_Too_Big;
        goto Exit;
      }
      else
        glyph->bytes = (unsigned short)bitmap_size;

      if ( FT_NEW_ARRAY( glyph->bitmap, glyph->bytes ) )
        goto Exit;

      p->row    = 0;
      p->flags |= _BDF_BITMAP;

      goto Exit;
    }

    error = BDF_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  /* Load the font properties. */
  static FT_Error
  _bdf_parse_properties( char*          line,
                         unsigned long  linelen,
                         unsigned long  lineno,
                         void*          call_data,
                         void*          client_data )
  {
    unsigned long      vlen;
    _bdf_line_func_t*  next;
    _bdf_parse_t*      p;
    char*              name;
    char*              value;
    char               nbuf[128];
    FT_Error           error = BDF_Err_Ok;

    FT_UNUSED( lineno );


    next = (_bdf_line_func_t *)call_data;
    p    = (_bdf_parse_t *)    client_data;

    /* Check for the end of the properties. */
    if ( ft_memcmp( line, "ENDPROPERTIES", 13 ) == 0 )
    {
      /* If the FONT_ASCENT or FONT_DESCENT properties have not been      */
      /* encountered yet, then make sure they are added as properties and */
      /* make sure they are set from the font bounding box info.          */
      /*                                                                  */
      /* This is *always* done regardless of the options, because X11     */
      /* requires these two fields to compile fonts.                      */
      if ( bdf_get_font_property( p->font, "FONT_ASCENT" ) == 0 )
      {
        p->font->font_ascent = p->font->bbx.ascent;
        ft_sprintf( nbuf, "%hd", p->font->bbx.ascent );
        error = _bdf_add_property( p->font, (char *)"FONT_ASCENT", nbuf );
        if ( error )
          goto Exit;

        FT_TRACE2(( "_bdf_parse_properties: " ACMSG1, p->font->bbx.ascent ));
        p->font->modified = 1;
      }

      if ( bdf_get_font_property( p->font, "FONT_DESCENT" ) == 0 )
      {
        p->font->font_descent = p->font->bbx.descent;
        ft_sprintf( nbuf, "%hd", p->font->bbx.descent );
        error = _bdf_add_property( p->font, (char *)"FONT_DESCENT", nbuf );
        if ( error )
          goto Exit;

        FT_TRACE2(( "_bdf_parse_properties: " ACMSG2, p->font->bbx.descent ));
        p->font->modified = 1;
      }

      p->flags &= ~_BDF_PROPS;
      *next     = _bdf_parse_glyphs;

      goto Exit;
    }

    /* Ignore the _XFREE86_GLYPH_RANGES properties. */
    if ( ft_memcmp( line, "_XFREE86_GLYPH_RANGES", 21 ) == 0 )
      goto Exit;

    /* Handle COMMENT fields and properties in a special way to preserve */
    /* the spacing.                                                      */
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      name = value = line;
      value += 7;
      if ( *value )
        *value++ = 0;
      error = _bdf_add_property( p->font, name, value );
      if ( error )
        goto Exit;
    }
    else if ( _bdf_is_atom( line, linelen, &name, &value, p->font ) )
    {
      error = _bdf_add_property( p->font, name, value );
      if ( error )
        goto Exit;
    }
    else
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      name = p->list.field[0];

      _bdf_list_shift( &p->list, 1 );
      value = _bdf_list_join( &p->list, ' ', &vlen );

      error = _bdf_add_property( p->font, name, value );
      if ( error )
        goto Exit;
    }

  Exit:
    return error;
  }


  /* Load the font header. */
  static FT_Error
  _bdf_parse_start( char*          line,
                    unsigned long  linelen,
                    unsigned long  lineno,
                    void*          call_data,
                    void*          client_data )
  {
    unsigned long      slen;
    _bdf_line_func_t*  next;
    _bdf_parse_t*      p;
    bdf_font_t*        font;
    char               *s;

    FT_Memory          memory = NULL;
    FT_Error           error  = BDF_Err_Ok;

    FT_UNUSED( lineno );            /* only used in debug mode */


    next = (_bdf_line_func_t *)call_data;
    p    = (_bdf_parse_t *)    client_data;

    if ( p->font )
      memory = p->font->memory;

    /* Check for a comment.  This is done to handle those fonts that have */
    /* comments before the STARTFONT line for some reason.                */
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      if ( p->opts->keep_comments != 0 && p->font != 0 )
      {
        linelen -= 7;

        s = line + 7;
        if ( *s != 0 )
        {
          s++;
          linelen--;
        }

        error = _bdf_add_comment( p->font, s, linelen );
        if ( error )
          goto Exit;
        /* here font is not defined! */
      }

      goto Exit;
    }

    if ( !( p->flags & _BDF_START ) )
    {
      memory = p->memory;

      if ( ft_memcmp( line, "STARTFONT", 9 ) != 0 )
      {
        /* No STARTFONT field is a good indication of a problem. */
        error = BDF_Err_Missing_Startfont_Field;
        goto Exit;
      }

      p->flags = _BDF_START;
      font = p->font = 0;

      if ( FT_NEW( font ) )
        goto Exit;
      p->font = font;

      font->memory = p->memory;
      p->memory    = 0;

      { /* setup */
        unsigned long    i;
        bdf_property_t*  prop;


        error = hash_init( &(font->proptbl), memory );
        if ( error )
          goto Exit;
        for ( i = 0, prop = (bdf_property_t*)_bdf_properties;
              i < _num_bdf_properties; i++, prop++ )
        {
          error = hash_insert( prop->name, (void *)i,
                               &(font->proptbl), memory );
          if ( error )
            goto Exit;
        }
      }

      if ( FT_ALLOC( p->font->internal, sizeof ( hashtable ) ) )
        goto Exit;
      error = hash_init( (hashtable *)p->font->internal,memory );
      if ( error )
        goto Exit;
      p->font->spacing      = p->opts->font_spacing;
      p->font->default_char = -1;

      goto Exit;
    }

    /* Check for the start of the properties. */
    if ( ft_memcmp( line, "STARTPROPERTIES", 15 ) == 0 )
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      p->cnt = p->font->props_size = _bdf_atoul( p->list.field[1], 0, 10 );

      if ( FT_NEW_ARRAY( p->font->props, p->cnt ) )
        goto Exit;

      p->flags |= _BDF_PROPS;
      *next     = _bdf_parse_properties;

      goto Exit;
    }

    /* Check for the FONTBOUNDINGBOX field. */
    if ( ft_memcmp( line, "FONTBOUNDINGBOX", 15 ) == 0 )
    {
      if ( !(p->flags & _BDF_SIZE ) )
      {
        /* Missing the SIZE field. */
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "SIZE" ));
        error = BDF_Err_Missing_Size_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->font->bbx.width  = _bdf_atos( p->list.field[1], 0, 10 );
      p->font->bbx.height = _bdf_atos( p->list.field[2], 0, 10 );

      p->font->bbx.x_offset = _bdf_atos( p->list.field[3], 0, 10 );
      p->font->bbx.y_offset = _bdf_atos( p->list.field[4], 0, 10 );

      p->font->bbx.ascent  = (short)( p->font->bbx.height +
                                      p->font->bbx.y_offset );

      p->font->bbx.descent = (short)( -p->font->bbx.y_offset );

      p->flags |= _BDF_FONT_BBX;

      goto Exit;
    }

    /* The next thing to check for is the FONT field. */
    if ( ft_memcmp( line, "FONT", 4 ) == 0 )
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      _bdf_list_shift( &p->list, 1 );

      s = _bdf_list_join( &p->list, ' ', &slen );
      if ( FT_NEW_ARRAY( p->font->name, slen + 1 ) )
        goto Exit;
      FT_MEM_COPY( p->font->name, s, slen + 1 );

      /* If the font name is an XLFD name, set the spacing to the one in  */
      /* the font name.  If there is no spacing fall back on the default. */
      error = _bdf_set_default_spacing( p->font, p->opts );
      if ( error )
        goto Exit;

      p->flags |= _BDF_FONT_NAME;

      goto Exit;
    }

    /* Check for the SIZE field. */
    if ( ft_memcmp( line, "SIZE", 4 ) == 0 )
    {
      if ( !( p->flags & _BDF_FONT_NAME ) )
      {
        /* Missing the FONT field. */
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "FONT" ));
        error = BDF_Err_Missing_Font_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->font->point_size   = _bdf_atoul( p->list.field[1], 0, 10 );
      p->font->resolution_x = _bdf_atoul( p->list.field[2], 0, 10 );
      p->font->resolution_y = _bdf_atoul( p->list.field[3], 0, 10 );

      /* Check for the bits per pixel field. */
      if ( p->list.used == 5 )
      {
        unsigned short bitcount, i, shift;


        p->font->bpp = (unsigned short)_bdf_atos( p->list.field[4], 0, 10 );

        /* Only values 1, 2, 4, 8 are allowed. */
        shift = p->font->bpp;
        bitcount = 0;
        for ( i = 0; shift > 0; i++ )
        {
          if ( shift & 1 )
            bitcount = i;
          shift >>= 1;
        }

        shift = (short)( ( bitcount > 3 ) ? 8 : ( 1 << bitcount ) );

        if ( p->font->bpp > shift || p->font->bpp != shift )
        {
          /* select next higher value */
          p->font->bpp = (unsigned short)( shift << 1 );
          FT_TRACE2(( "_bdf_parse_start: " ACMSG11, p->font->bpp ));
        }
      }
      else
        p->font->bpp = 1;

      p->flags |= _BDF_SIZE;

      goto Exit;
    }

    error = BDF_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* API.                                                                  */
  /*                                                                       */
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  bdf_load_font( FT_Stream       stream,
                 FT_Memory       extmemory,
                 bdf_options_t*  opts,
                 bdf_font_t*    *font )
  {
    unsigned long  lineno;
    _bdf_parse_t   *p;

    FT_Memory      memory = extmemory;
    FT_Error       error  = BDF_Err_Ok;


    if ( FT_NEW( p ) )
      goto Exit;

    memory    = NULL;
    p->opts   = (bdf_options_t*)( ( opts != 0 ) ? opts : &_bdf_opts );
    p->minlb  = 32767;
    p->memory = extmemory;  /* only during font creation */

    _bdf_list_init( &p->list, extmemory );

    error = _bdf_readstream( stream, _bdf_parse_start,
                             (void *)p, &lineno );
    if ( error )
      goto Exit;

    if ( p->font != 0 )
    {
      /* If the font is not proportional, set the font's monowidth */
      /* field to the width of the font bounding box.              */
      memory = p->font->memory;

      if ( p->font->spacing != BDF_PROPORTIONAL )
        p->font->monowidth = p->font->bbx.width;

      /* If the number of glyphs loaded is not that of the original count, */
      /* indicate the difference.                                          */
      if ( p->cnt != p->font->glyphs_used + p->font->unencoded_used )
      {
        FT_TRACE2(( "bdf_load_font: " ACMSG15, p->cnt,
                    p->font->glyphs_used + p->font->unencoded_used ));
        p->font->modified = 1;
      }

      /* Once the font has been loaded, adjust the overall font metrics if */
      /* necessary.                                                        */
      if ( p->opts->correct_metrics != 0 &&
           ( p->font->glyphs_used > 0 || p->font->unencoded_used > 0 ) )
      {
        if ( p->maxrb - p->minlb != p->font->bbx.width )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG3,
                      p->font->bbx.width, p->maxrb - p->minlb ));
          p->font->bbx.width = (unsigned short)( p->maxrb - p->minlb );
          p->font->modified  = 1;
        }

        if ( p->font->bbx.x_offset != p->minlb )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG4,
                      p->font->bbx.x_offset, p->minlb ));
          p->font->bbx.x_offset = p->minlb;
          p->font->modified     = 1;
        }

        if ( p->font->bbx.ascent != p->maxas )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG5,
                      p->font->bbx.ascent, p->maxas ));
          p->font->bbx.ascent = p->maxas;
          p->font->modified   = 1;
        }

        if ( p->font->bbx.descent != p->maxds )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG6,
                      p->font->bbx.descent, p->maxds ));
          p->font->bbx.descent  = p->maxds;
          p->font->bbx.y_offset = (short)( -p->maxds );
          p->font->modified     = 1;
        }

        if ( p->maxas + p->maxds != p->font->bbx.height )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG7,
                      p->font->bbx.height, p->maxas + p->maxds ));
          p->font->bbx.height = (unsigned short)( p->maxas + p->maxds );
        }

        if ( p->flags & _BDF_SWIDTH_ADJ )
          FT_TRACE2(( "bdf_load_font: " ACMSG8 ));
      }
    }

    if ( p->flags & _BDF_START )
    {
      {
        /* The ENDFONT field was never reached or did not exist. */
        if ( !( p->flags & _BDF_GLYPHS ) )
          /* Error happened while parsing header. */
          FT_ERROR(( "bdf_load_font: " ERRMSG2, lineno ));
        else
          /* Error happened when parsing glyphs. */
          FT_ERROR(( "bdf_load_font: " ERRMSG3, lineno ));
      }
    }

    if ( p->font != 0 )
    {
      /* Make sure the comments are NULL terminated if they exist. */
      memory = p->font->memory;

      if ( p->font->comments_len > 0 ) {
        if ( FT_RENEW_ARRAY( p->font->comments,
                             p->font->comments_len,
                             p->font->comments_len + 1 ) )
          goto Exit;

        p->font->comments[p->font->comments_len] = 0;
      }
    }
    else if ( error == BDF_Err_Ok )
      error = BDF_Err_Invalid_File_Format;

    *font = p->font;

  Exit:
    if ( p )
    {
      _bdf_list_done( &p->list );

      memory = extmemory;

      FT_FREE( p );
    }

    return error;
  }


  FT_LOCAL_DEF( void )
  bdf_free_font( bdf_font_t*  font )
  {
    bdf_property_t*  prop;
    unsigned long    i;
    bdf_glyph_t*     glyphs;
    FT_Memory        memory;


    if ( font == 0 )
      return;

    memory = font->memory;

    FT_FREE( font->name );

    /* Free up the internal hash table of property names. */
    if ( font->internal )
    {
      hash_free( (hashtable *)font->internal, memory );
      FT_FREE( font->internal );
    }

    /* Free up the comment info. */
    FT_FREE( font->comments );

    /* Free up the properties. */
    for ( i = 0; i < font->props_size; i++ )
    {
      if ( font->props[i].format == BDF_ATOM )
        FT_FREE( font->props[i].value.atom );
    }

    FT_FREE( font->props );

    /* Free up the character info. */
    for ( i = 0, glyphs = font->glyphs;
          i < font->glyphs_used; i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    for ( i = 0, glyphs = font->unencoded; i < font->unencoded_used;
          i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    FT_FREE( font->glyphs );
    FT_FREE( font->unencoded );

    /* Free up the overflow storage if it was used. */
    for ( i = 0, glyphs = font->overflow.glyphs;
          i < font->overflow.glyphs_used; i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    FT_FREE( font->overflow.glyphs );

    /* bdf_cleanup */
    hash_free( &(font->proptbl), memory );

    /* Free up the user defined properties. */
    for (prop = font->user_props, i = 0;
         i < font->nuser_props; i++, prop++ )
    {
      FT_FREE( prop->name );
      if ( prop->format == BDF_ATOM )
        FT_FREE( prop->value.atom );
    }

    FT_FREE( font->user_props );

    /* FREE( font ); */ /* XXX Fixme */
  }


  FT_LOCAL_DEF( bdf_property_t * )
  bdf_get_font_property( bdf_font_t*  font,
                         const char*  name )
  {
    hashnode  hn;


    if ( font == 0 || font->props_size == 0 || name == 0 || *name == 0 )
      return 0;

    hn = hash_lookup( name, (hashtable *)font->internal );

    return hn ? ( font->props + (unsigned long)hn->data ) : 0;
  }


/* END */

/*  bdfdrivr.c

    FreeType font driver for bdf files

    Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 by
    Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "ft2build.h"

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H
#include FT_BDF_H

#include FT_SERVICE_BDF_H
#include FT_SERVICE_XFREE86_NAME_H

#include "bdf.h"
#include "bdfdrivr.h"

#include "bdferror.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_bdfdriver


  typedef struct  BDF_CMapRec_
  {
    FT_CMapRec        cmap;
    FT_UInt           num_encodings;
    BDF_encoding_el*  encodings;

  } BDF_CMapRec, *BDF_CMap;


  FT_CALLBACK_DEF( FT_Error )
  bdf_cmap_init( FT_CMap     bdfcmap,
                 FT_Pointer  init_data )
  {
    BDF_CMap  cmap = (BDF_CMap)bdfcmap;
    BDF_Face  face = (BDF_Face)FT_CMAP_FACE( cmap );
    FT_UNUSED( init_data );


    cmap->num_encodings = face->bdffont->glyphs_used;
    cmap->encodings     = face->en_table;

    return BDF_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  bdf_cmap_done( FT_CMap  bdfcmap )
  {
    BDF_CMap  cmap = (BDF_CMap)bdfcmap;


    cmap->encodings     = NULL;
    cmap->num_encodings = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  bdf_cmap_char_index( FT_CMap    bdfcmap,
                       FT_UInt32  charcode )
  {
    BDF_CMap          cmap      = (BDF_CMap)bdfcmap;
    BDF_encoding_el*  encodings = cmap->encodings;
    FT_UInt           min, max, mid;
    FT_UInt           result    = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        /* increase glyph index by 1 --              */
        /* we reserve slot 0 for the undefined glyph */
        result = encodings[mid].glyph + 1;
        break;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  bdf_cmap_char_next( FT_CMap     bdfcmap,
                      FT_UInt32  *acharcode )
  {
    BDF_CMap          cmap      = (BDF_CMap)bdfcmap;
    BDF_encoding_el*  encodings = cmap->encodings;
    FT_UInt           min, max, mid;
    FT_UInt32         charcode = *acharcode + 1;
    FT_UInt           result   = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        /* increase glyph index by 1 --              */
        /* we reserve slot 0 for the undefined glyph */
        result = encodings[mid].glyph + 1;
        goto Exit;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    charcode = 0;
    if ( min < cmap->num_encodings )
    {
      charcode = encodings[min].enc;
      result   = encodings[min].glyph + 1;
    }

  Exit:
    *acharcode = charcode;
    return result;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_CMap_ClassRec  bdf_cmap_class =
  {
    sizeof ( BDF_CMapRec ),
    bdf_cmap_init,
    bdf_cmap_done,
    bdf_cmap_char_index,
    bdf_cmap_char_next
  };


  static FT_Error
  bdf_interpret_style( BDF_Face  bdf )
  {
    FT_Error         error  = BDF_Err_Ok;
    FT_Face          face   = FT_FACE( bdf );
    FT_Memory        memory = face->memory;
    bdf_font_t*      font   = bdf->bdffont;
    bdf_property_t*  prop;

    char  *istr = NULL, *bstr = NULL;
    char  *sstr = NULL, *astr = NULL;

    int  parts = 0, len = 0;


    face->style_flags = 0;

    prop = bdf_get_font_property( font, (char *)"SLANT" );
    if ( prop && prop->format == BDF_ATOM                             &&
         prop->value.atom                                             &&
         ( *(prop->value.atom) == 'O' || *(prop->value.atom) == 'o' ||
           *(prop->value.atom) == 'I' || *(prop->value.atom) == 'i' ) )
    {
      face->style_flags |= FT_STYLE_FLAG_ITALIC;
      istr = ( *(prop->value.atom) == 'O' || *(prop->value.atom) == 'o' )
               ? (char *)"Oblique"
               : (char *)"Italic";
      len += ft_strlen( istr );
      parts++;
    }

    prop = bdf_get_font_property( font, (char *)"WEIGHT_NAME" );
    if ( prop && prop->format == BDF_ATOM                             &&
         prop->value.atom                                             &&
         ( *(prop->value.atom) == 'B' || *(prop->value.atom) == 'b' ) )
    {
      face->style_flags |= FT_STYLE_FLAG_BOLD;
      bstr = (char *)"Bold";
      len += ft_strlen( bstr );
      parts++;
    }

    prop = bdf_get_font_property( font, (char *)"SETWIDTH_NAME" );
    if ( prop && prop->format == BDF_ATOM                              &&
         prop->value.atom && *(prop->value.atom)                       &&
         !( *(prop->value.atom) == 'N' || *(prop->value.atom) == 'n' ) )
    {
      sstr = (char *)(prop->value.atom);
      len += ft_strlen( sstr );
      parts++;
    }

    prop = bdf_get_font_property( font, (char *)"ADD_STYLE_NAME" );
    if ( prop && prop->format == BDF_ATOM                              &&
         prop->value.atom && *(prop->value.atom)                       &&
         !( *(prop->value.atom) == 'N' || *(prop->value.atom) == 'n' ) )
    {
      astr = (char *)(prop->value.atom);
      len += ft_strlen( astr );
      parts++;
    }

    if ( !parts || !len )
    {
      if ( FT_ALLOC( face->style_name, ft_strlen( "Regular" ) + 1 ) )
        return error;

      ft_strcpy( face->style_name, "Regular" );
    }
    else
    {
      char          *style, *s;
      unsigned int  i;


      if ( FT_ALLOC( style, len + parts ) )
        return error;

      s = style;

      if ( astr )
      {
        ft_strcpy( s, astr );
        for ( i = 0; i < ft_strlen( astr ); i++, s++ )
          if ( *s == ' ' )
            *s = '-';                     /* replace spaces with dashes */
        *(s++) = ' ';
      }
      if ( bstr )
      {
        ft_strcpy( s, bstr );
        s += ft_strlen( bstr );
        *(s++) = ' ';
      }
      if ( istr )
      {
        ft_strcpy( s, istr );
        s += ft_strlen( istr );
        *(s++) = ' ';
      }
      if ( sstr )
      {
        ft_strcpy( s, sstr );
        for ( i = 0; i < ft_strlen( sstr ); i++, s++ )
          if ( *s == ' ' )
            *s = '-';                     /* replace spaces with dashes */
        *(s++) = ' ';
      }
      *(--s) = '\0';        /* overwrite last ' ', terminate the string */

      face->style_name = style;                     /* allocated string */
    }

    return error;
  }


  FT_CALLBACK_DEF( void )
  BDF_Face_Done( FT_Face  bdfface )         /* BDF_Face */
  {
    BDF_Face   face   = (BDF_Face)bdfface;
    FT_Memory  memory = FT_FACE_MEMORY( face );


    bdf_free_font( face->bdffont );

    FT_FREE( face->en_table );

    FT_FREE( face->charset_encoding );
    FT_FREE( face->charset_registry );
    FT_FREE( bdfface->family_name );
    FT_FREE( bdfface->style_name );

    FT_FREE( bdfface->available_sizes );

    FT_FREE( face->bdffont );

    FT_TRACE4(( "BDF_Face_Done: done face\n" ));
  }


  FT_CALLBACK_DEF( FT_Error )
  BDF_Face_Init( FT_Stream      stream,
                 FT_Face        bdfface,        /* BDF_Face */
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error       error  = BDF_Err_Ok;
    BDF_Face       face   = (BDF_Face)bdfface;
    FT_Memory      memory = FT_FACE_MEMORY( face );

    bdf_font_t*    font = NULL;
    bdf_options_t  options;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );


    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    options.correct_metrics = 1;   /* FZ XXX: options semantics */
    options.keep_unencoded  = 1;
    options.keep_comments   = 0;
    options.font_spacing    = BDF_PROPORTIONAL;

    error = bdf_load_font( stream, memory, &options, &font );
    if ( error == BDF_Err_Missing_Startfont_Field )
    {
      FT_TRACE2(( "[not a valid BDF file]\n" ));
      goto Fail;
    }
    else if ( error )
      goto Exit;

    /* we have a bdf font: let's construct the face object */
    face->bdffont = font;
    {
      bdf_property_t*  prop = NULL;


      FT_TRACE4(( "number of glyphs: %d (%d)\n",
                  font->glyphs_size,
                  font->glyphs_used ));
      FT_TRACE4(( "number of unencoded glyphs: %d (%d)\n",
                  font->unencoded_size,
                  font->unencoded_used ));

      bdfface->num_faces  = 1;
      bdfface->face_index = 0;
      bdfface->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                            FT_FACE_FLAG_HORIZONTAL  |
                            FT_FACE_FLAG_FAST_GLYPHS;

      prop = bdf_get_font_property( font, "SPACING" );
      if ( prop && prop->format == BDF_ATOM                             &&
           prop->value.atom                                             &&
           ( *(prop->value.atom) == 'M' || *(prop->value.atom) == 'm' ||
             *(prop->value.atom) == 'C' || *(prop->value.atom) == 'c' ) )
        bdfface->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* FZ XXX: TO DO: FT_FACE_FLAGS_VERTICAL   */
      /* FZ XXX: I need a font to implement this */

      prop = bdf_get_font_property( font, "FAMILY_NAME" );
      if ( prop && prop->value.atom )
      {
        int  l = ft_strlen( prop->value.atom ) + 1;


        if ( FT_NEW_ARRAY( bdfface->family_name, l ) )
          goto Exit;
        ft_strcpy( bdfface->family_name, prop->value.atom );
      }
      else
        bdfface->family_name = 0;

      if ( ( error = bdf_interpret_style( face ) ) != 0 )
        goto Exit;

      /* the number of glyphs (with one slot for the undefined glyph */
      /* at position 0 and all unencoded glyphs)                     */
      bdfface->num_glyphs = font->glyphs_size + 1;

      bdfface->num_fixed_sizes = 1;
      if ( FT_NEW_ARRAY( bdfface->available_sizes, 1 ) )
        goto Exit;

      {
        FT_Bitmap_Size*  bsize = bdfface->available_sizes;
        FT_Short         resolution_x = 0, resolution_y = 0;


        FT_MEM_ZERO( bsize, sizeof ( FT_Bitmap_Size ) );

        bsize->height = (FT_Short)( font->font_ascent + font->font_descent );

        prop = bdf_get_font_property( font, "AVERAGE_WIDTH" );
        if ( prop )
          bsize->width = (FT_Short)( ( prop->value.int32 + 5 ) / 10 );
        else
          bsize->width = (FT_Short)( bsize->height * 2/3 );

        prop = bdf_get_font_property( font, "POINT_SIZE" );
        if ( prop )
          /* convert from 722.7 decipoints to 72 points per inch */
          bsize->size =
            (FT_Pos)( ( prop->value.int32 * 64 * 7200 + 36135L ) / 72270L );

        prop = bdf_get_font_property( font, "PIXEL_SIZE" );
        if ( prop )
          bsize->y_ppem = (FT_Short)prop->value.int32 << 6;

        prop = bdf_get_font_property( font, "RESOLUTION_X" );
        if ( prop )
          resolution_x = (FT_Short)prop->value.int32;

        prop = bdf_get_font_property( font, "RESOLUTION_Y" );
        if ( prop )
          resolution_y = (FT_Short)prop->value.int32;

        if ( bsize->y_ppem == 0 )
        {
          bsize->y_ppem = bsize->size;
          if ( resolution_y )
            bsize->y_ppem = bsize->y_ppem * resolution_y / 72;
        }
        if ( resolution_x && resolution_y )
          bsize->x_ppem = bsize->y_ppem * resolution_x / resolution_y;
        else
          bsize->x_ppem = bsize->y_ppem;
      }

      /* encoding table */
      {
        bdf_glyph_t*   cur = font->glyphs;
        unsigned long  n;


        if ( FT_NEW_ARRAY( face->en_table, font->glyphs_size ) )
          goto Exit;

        face->default_glyph = 0;
        for ( n = 0; n < font->glyphs_size; n++ )
        {
          (face->en_table[n]).enc = cur[n].encoding;
          FT_TRACE4(( "idx %d, val 0x%lX\n", n, cur[n].encoding ));
          (face->en_table[n]).glyph = (FT_Short)n;

          if ( cur[n].encoding == font->default_char )
            face->default_glyph = n;
        }
      }

      /* charmaps */
      {
        bdf_property_t  *charset_registry = 0, *charset_encoding = 0;
        FT_Bool          unicode_charmap  = 0;


        charset_registry =
          bdf_get_font_property( font, "CHARSET_REGISTRY" );
        charset_encoding =
          bdf_get_font_property( font, "CHARSET_ENCODING" );
        if ( charset_registry && charset_encoding )
        {
          if ( charset_registry->format == BDF_ATOM &&
               charset_encoding->format == BDF_ATOM &&
               charset_registry->value.atom         &&
               charset_encoding->value.atom         )
          {
            const char*  s;


            if ( FT_NEW_ARRAY( face->charset_encoding,
                               ft_strlen( charset_encoding->value.atom ) + 1 ) )
              goto Exit;
            if ( FT_NEW_ARRAY( face->charset_registry,
                               ft_strlen( charset_registry->value.atom ) + 1 ) )
              goto Exit;

            ft_strcpy( face->charset_registry, charset_registry->value.atom );
            ft_strcpy( face->charset_encoding, charset_encoding->value.atom );

            /* Uh, oh, compare first letters manually to avoid dependency
               on locales. */
            s = face->charset_registry;
            if ( ( s[0] == 'i' || s[0] == 'I' ) &&
                 ( s[1] == 's' || s[1] == 'S' ) &&
                 ( s[2] == 'o' || s[2] == 'O' ) )
            {
              s += 3;
              if ( !ft_strcmp( s, "10646" )                      ||
                   ( !ft_strcmp( s, "8859" ) &&
                     !ft_strcmp( face->charset_encoding, "1" ) ) )
              unicode_charmap = 1;
            }

            {
              FT_CharMapRec  charmap;


              charmap.face        = FT_FACE( face );
              charmap.encoding    = FT_ENCODING_NONE;
              charmap.platform_id = 0;
              charmap.encoding_id = 0;

              if ( unicode_charmap )
              {
                charmap.encoding    = FT_ENCODING_UNICODE;
                charmap.platform_id = 3;
                charmap.encoding_id = 1;
              }

              error = FT_CMap_New( &bdf_cmap_class, NULL, &charmap, NULL );

#if 0
              /* Select default charmap */
              if ( bdfface->num_charmaps )
                bdfface->charmap = bdfface->charmaps[0];
#endif
            }

            goto Exit;
          }
        }

        /* otherwise assume Adobe standard encoding */

        {
          FT_CharMapRec  charmap;


          charmap.face        = FT_FACE( face );
          charmap.encoding    = FT_ENCODING_ADOBE_STANDARD;
          charmap.platform_id = 7;
          charmap.encoding_id = 0;

          error = FT_CMap_New( &bdf_cmap_class, NULL, &charmap, NULL );

          /* Select default charmap */
          if ( bdfface->num_charmaps )
            bdfface->charmap = bdfface->charmaps[0];
        }
      }
    }

  Exit:
    return error;

  Fail:
    BDF_Face_Done( bdfface );
    return BDF_Err_Unknown_File_Format;
  }


  FT_CALLBACK_DEF( FT_Error )
  BDF_Size_Select( FT_Size   size,
                   FT_ULong  strike_index )
  {
    bdf_font_t*  bdffont = ( (BDF_Face)size->face )->bdffont;


    FT_Select_Metrics( size->face, strike_index );

    size->metrics.ascender    = bdffont->font_ascent << 6;
    size->metrics.descender   = -bdffont->font_descent << 6;
    size->metrics.max_advance = bdffont->bbx.width << 6;

    return BDF_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  BDF_Size_Request( FT_Size          size,
                    FT_Size_Request  req )
  {
    FT_Face          face    = size->face;
    FT_Bitmap_Size*  bsize   = face->available_sizes;
    bdf_font_t*      bdffont = ( (BDF_Face)face )->bdffont;
    FT_Error         error   = BDF_Err_Invalid_Pixel_Size;
    FT_Long          height;


    height = FT_REQUEST_HEIGHT( req );
    height = ( height + 32 ) >> 6;

    switch ( req->type )
    {
    case FT_SIZE_REQUEST_TYPE_NOMINAL:
      if ( height == ( bsize->y_ppem + 32 ) >> 6 )
        error = BDF_Err_Ok;
      break;

    case FT_SIZE_REQUEST_TYPE_REAL_DIM:
      if ( height == ( bdffont->font_ascent +
                       bdffont->font_descent ) )
        error = BDF_Err_Ok;
      break;

    default:
      error = BDF_Err_Unimplemented_Feature;
      break;
    }

    if ( error )
      return error;
    else
      return BDF_Size_Select( size, 0 );
  }



  FT_CALLBACK_DEF( FT_Error )
  BDF_Glyph_Load( FT_GlyphSlot  slot,
                  FT_Size       size,
                  FT_UInt       glyph_index,
                  FT_Int32      load_flags )
  {
    BDF_Face     face   = (BDF_Face)FT_SIZE_FACE( size );
    FT_Error     error  = BDF_Err_Ok;
    FT_Bitmap*   bitmap = &slot->bitmap;
    bdf_glyph_t  glyph;
    int          bpp    = face->bdffont->bpp;

    FT_UNUSED( load_flags );


    if ( !face )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    /* index 0 is the undefined glyph */
    if ( glyph_index == 0 )
      glyph_index = face->default_glyph;
    else
      glyph_index--;

    /* slot, bitmap => freetype, glyph => bdflib */
    glyph = face->bdffont->glyphs[glyph_index];

    bitmap->rows  = glyph.bbx.height;
    bitmap->width = glyph.bbx.width;
    bitmap->pitch = glyph.bpr;

    /* note: we don't allocate a new array to hold the bitmap; */
    /*       we can simply point to it                         */
    ft_glyphslot_set_bitmap( slot, glyph.bitmap );

    switch ( bpp )
    {
    case 1:
      bitmap->pixel_mode = FT_PIXEL_MODE_MONO;
      break;
    case 2:
      bitmap->pixel_mode = FT_PIXEL_MODE_GRAY2;
      break;
    case 4:
      bitmap->pixel_mode = FT_PIXEL_MODE_GRAY4;
      break;
    case 8:
      bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
      bitmap->num_grays  = 256;
      break;
    }

    slot->format      = FT_GLYPH_FORMAT_BITMAP;
    slot->bitmap_left = glyph.bbx.x_offset;
    slot->bitmap_top  = glyph.bbx.ascent;

    slot->metrics.horiAdvance  = glyph.dwidth << 6;
    slot->metrics.horiBearingX = glyph.bbx.x_offset << 6;
    slot->metrics.horiBearingY = glyph.bbx.ascent << 6;
    slot->metrics.width        = bitmap->width << 6;
    slot->metrics.height       = bitmap->rows << 6;

    /*
     * XXX DWIDTH1 and VVECTOR should be parsed and
     * used here, provided such fonts do exist.
     */
    ft_synthesize_vertical_metrics( &slot->metrics,
                                    face->bdffont->bbx.height << 6 );

  Exit:
    return error;
  }


 /*
  *
  *  BDF SERVICE
  *
  */

  static FT_Error
  bdf_get_bdf_property( BDF_Face          face,
                        const char*       prop_name,
                        BDF_PropertyRec  *aproperty )
  {
    bdf_property_t*  prop;


    FT_ASSERT( face && face->bdffont );

    prop = bdf_get_font_property( face->bdffont, prop_name );
    if ( prop )
    {
      switch ( prop->format )
      {
      case BDF_ATOM:
        aproperty->type   = BDF_PROPERTY_TYPE_ATOM;
        aproperty->u.atom = prop->value.atom;
        break;

      case BDF_INTEGER:
        aproperty->type      = BDF_PROPERTY_TYPE_INTEGER;
        aproperty->u.integer = prop->value.int32;
        break;

      case BDF_CARDINAL:
        aproperty->type       = BDF_PROPERTY_TYPE_CARDINAL;
        aproperty->u.cardinal = prop->value.card32;
        break;

      default:
        goto Fail;
      }
      return 0;
    }

  Fail:
    return BDF_Err_Invalid_Argument;
  }


  static FT_Error
  bdf_get_charset_id( BDF_Face      face,
                      const char*  *acharset_encoding,
                      const char*  *acharset_registry )
  {
    *acharset_encoding = face->charset_encoding;
    *acharset_registry = face->charset_registry;

    return 0;
  }


  static const FT_Service_BDFRec  bdf_service_bdf =
  {
    (FT_BDF_GetCharsetIdFunc)bdf_get_charset_id,
    (FT_BDF_GetPropertyFunc) bdf_get_bdf_property
  };


 /*
  *
  *  SERVICES LIST
  *
  */

  static const FT_ServiceDescRec  bdf_services[] =
  {
    { FT_SERVICE_ID_BDF,       &bdf_service_bdf },
    { FT_SERVICE_ID_XF86_NAME, FT_XF86_FORMAT_BDF },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  bdf_driver_requester( FT_Module    module,
                        const char*  name )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( bdf_services, name );
  }



  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  bdf_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER         |
      FT_MODULE_DRIVER_NO_OUTLINES,
      sizeof ( FT_DriverRec ),

      "bdf",
      0x10000L,
      0x20000L,

      0,

      (FT_Module_Constructor)0,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  bdf_driver_requester
    },

    sizeof ( BDF_FaceRec ),
    sizeof ( FT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),

    BDF_Face_Init,
    BDF_Face_Done,
    0,                          /* FT_Size_InitFunc */
    0,                          /* FT_Size_DoneFunc */
    0,                          /* FT_Slot_InitFunc */
    0,                          /* FT_Slot_DoneFunc */

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif
    BDF_Glyph_Load,

    0,                          /* FT_Face_GetKerningFunc   */
    0,                          /* FT_Face_AttachFunc       */
    0,                          /* FT_Face_GetAdvancesFunc  */

    BDF_Size_Request,
    BDF_Size_Select
  };


/* END */



/* END */

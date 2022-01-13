/***************************************************************************/
/*                                                                         */
/*  ftlzw.c                                                                */
/*                                                                         */
/*    FreeType support for .Z compressed files.                            */
/*                                                                         */
/*  This optional component relies on NetBSD's zopen().  It should mainly  */
/*  be used to parse compressed PCF fonts, as found with many X11 server   */
/*  distributions.                                                         */
/*                                                                         */
/*  Copyright 2004, 2005, 2006 by                                          */
/*  Albert Chin-A-Young.                                                   */
/*                                                                         */
/*  Based on code in src/gzip/ftgzip.c, Copyright 2004 by                  */
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
#include FT_LZW_H
#include <string.h>
#include <stdio.h>


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#define FT_ERR_PREFIX  LZW_Err_
#define FT_ERR_BASE    FT_Mod_Err_LZW

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_LZW

#include "ftzopen.h"


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****                  M E M O R Y   M A N A G E M E N T              *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****                   F I L E   D E S C R I P T O R                 *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

#define FT_LZW_BUFFER_SIZE  4096

  typedef struct  FT_LZWFileRec_
  {
    FT_Stream       source;         /* parent/source stream        */
    FT_Stream       stream;         /* embedding stream            */
    FT_Memory       memory;         /* memory allocator            */
    FT_LzwStateRec  lzw;            /* lzw decompressor state      */

    FT_Byte         buffer[FT_LZW_BUFFER_SIZE]; /* output buffer      */
    FT_ULong        pos;                        /* position in output */
    FT_Byte*        cursor;
    FT_Byte*        limit;

  } FT_LZWFileRec, *FT_LZWFile;


  /* check and skip .Z header */
  static FT_Error
  ft_lzw_check_header( FT_Stream  stream )
  {
    FT_Error  error;
    FT_Byte   head[2];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 2 ) )
      goto Exit;

    /* head[0] && head[1] are the magic numbers */
    if ( head[0] != 0x1f ||
         head[1] != 0x9d )
      error = LZW_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static FT_Error
  ft_lzw_file_init( FT_LZWFile  zip,
                    FT_Stream   stream,
                    FT_Stream   source )
  {
    FT_LzwState  lzw   = &zip->lzw;
    FT_Error     error = LZW_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_LZW_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    /* check and skip .Z header */
    {
      stream = source;

      error = ft_lzw_check_header( source );
      if ( error )
        goto Exit;
    }

    /* initialize internal lzw variable */
    ft_lzwstate_init( lzw, source );

  Exit:
    return error;
  }


  static void
  ft_lzw_file_done( FT_LZWFile  zip )
  {
    /* clear the rest */
    ft_lzwstate_done( &zip->lzw );

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_lzw_file_reset( FT_LZWFile  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( 0 ) )
    {
      ft_lzwstate_reset( &zip->lzw );

      zip->limit  = zip->buffer + FT_LZW_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;
    }

    return error;
  }


  static FT_Error
  ft_lzw_file_fill_output( FT_LZWFile  zip )
  {
    FT_LzwState  lzw = &zip->lzw;
    FT_ULong     count;
    FT_Error     error   = 0;


    zip->cursor = zip->buffer;

    count = ft_lzwstate_io( lzw, zip->buffer, FT_LZW_BUFFER_SIZE );

    zip->limit = zip->cursor + count;

    if ( count == 0 )
      error = LZW_Err_Invalid_Stream_Operation;

    return error;
  }


  /* fill output buffer; `count' must be <= FT_LZW_BUFFER_SIZE */
  static FT_Error
  ft_lzw_file_skip_output( FT_LZWFile  zip,
                           FT_ULong    count )
  {
    FT_Error  error = LZW_Err_Ok;


    /* first, we skip what we can from the output buffer */
    {
      FT_ULong  delta = (FT_ULong)( zip->limit - zip->cursor );


      if ( delta >= count )
        delta = count;

      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
    }

    /* next, we skip as many bytes remaining as possible */
    while ( count > 0 )
    {
      FT_ULong  delta = FT_LZW_BUFFER_SIZE;
      FT_ULong  numread;


      if ( delta > count )
        delta = count;

      numread = ft_lzwstate_io( &zip->lzw, NULL, delta );
      if ( numread < delta )
      {
        /* not enough bytes */
        error = LZW_Err_Invalid_Stream_Operation;
        break;
      }

      zip->pos += delta;
      count    -= delta;
    }

    return error;
  }


  static FT_ULong
  ft_lzw_file_io( FT_LZWFile  zip,
                  FT_ULong    pos,
                  FT_Byte*    buffer,
                  FT_ULong    count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    /* seeking backwards. */
    if ( pos < zip->pos )
    {
      /* If the new position is within the output buffer, simply       */
      /* decrement pointers, otherwise we reset the stream completely! */
      if ( ( zip->pos - pos ) <= (FT_ULong)( zip->cursor - zip->buffer ) )
      {
        zip->cursor -= zip->pos - pos;
        zip->pos     = pos;
      }
      else
      {
        error = ft_lzw_file_reset( zip );
        if ( error )
          goto Exit;
      }
    }

    /* skip unwanted bytes */
    if ( pos > zip->pos )
    {
      error = ft_lzw_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
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

      FT_MEM_COPY( buffer + result, zip->cursor, delta );
      result      += delta;
      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_lzw_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****            L Z W   E M B E D D I N G   S T R E A M              *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  static void
  ft_lzw_stream_close( FT_Stream  stream )
  {
    FT_LZWFile  zip    = (FT_LZWFile)stream->descriptor.pointer;
    FT_Memory   memory = stream->memory;


    if ( zip )
    {
      /* finalize lzw file descriptor */
      ft_lzw_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_lzw_stream_io( FT_Stream  stream,
                    FT_ULong   pos,
                    FT_Byte*   buffer,
                    FT_ULong   count )
  {
    FT_LZWFile  zip = (FT_LZWFile)stream->descriptor.pointer;


    return ft_lzw_file_io( zip, pos, buffer, count );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenLZW( FT_Stream  stream,
                     FT_Stream  source )
  {
    FT_Error    error;
    FT_Memory   memory = source->memory;
    FT_LZWFile  zip;


    /*
     *  Check the header right now; this prevents allocation of a huge
     *  LZWFile object (400 KByte of heap memory) if not necessary.
     *
     *  Did I mention that you should never use .Z compressed font
     *  files?
     */
    error = ft_lzw_check_header( source );
    if ( error )
      goto Exit;

    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_NEW( zip ) )
    {
      error = ft_lzw_file_init( zip, stream, source );
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
    stream->read  = ft_lzw_stream_io;
    stream->close = ft_lzw_stream_close;

  Exit:
    return error;
  }


/***************************************************************************/
/*                                                                         */
/*  ftzopen.c                                                              */
/*                                                                         */
/*    FreeType support for .Z compressed files.                            */
/*                                                                         */
/*  This optional component relies on NetBSD's zopen().  It should mainly  */
/*  be used to parse compressed PCF fonts, as found with many X11 server   */
/*  distributions.                                                         */
/*                                                                         */
/*  Copyright 2005, 2006 by David Turner.                                  */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#include "ftzopen.h"
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H

  /* refill input buffer, return 0 on success, or -1 if eof */
  static int
  ft_lzwstate_refill( FT_LzwState  state )
  {
    int  result = -1;


    if ( !state->in_eof )
    {
      FT_ULong  count = FT_Stream_TryRead( state->source,
                                           state->in_buff,
                                           sizeof ( state->in_buff ) );

      state->in_cursor = state->in_buff;
      state->in_limit  = state->in_buff + count;
      state->in_eof    = FT_BOOL( count < sizeof ( state->in_buff ) );

      if ( count > 0 )
        result = 0;
    }
    return result;
  }


  /* return new code of 'num_bits', or -1 if eof */
  static FT_Int32
  ft_lzwstate_get_code( FT_LzwState  state,
                        FT_UInt      num_bits )
  {
    FT_Int32   result   = -1;
    FT_UInt32  pad      = state->pad;
    FT_UInt    pad_bits = state->pad_bits;


    while ( num_bits > pad_bits )
    {
      if ( state->in_cursor >= state->in_limit &&
           ft_lzwstate_refill( state ) < 0     )
        goto Exit;

      pad      |= (FT_UInt32)(*state->in_cursor++) << pad_bits;
      pad_bits += 8;
    }

    result          = (FT_Int32)( pad & LZW_MASK( num_bits ) );
    state->pad_bits = pad_bits - num_bits;
    state->pad      = pad >> num_bits;

  Exit:
    return result;
  }


  /* grow the character stack */
  static int
  ft_lzwstate_stack_grow( FT_LzwState  state )
  {
    if ( state->stack_top >= state->stack_size )
    {
      FT_Memory  memory = state->memory;
      FT_Error   error;
      FT_UInt    old_size = state->stack_size;
      FT_UInt    new_size = old_size;

      new_size = new_size + ( new_size >> 1 ) + 4;

      if ( state->stack == state->stack_0 )
      {
        state->stack = NULL;
        old_size     = 0;
      }

      if ( FT_RENEW_ARRAY( state->stack, old_size, new_size ) )
        return -1;

      state->stack_size = new_size;
    }
    return 0;
  }


  /* grow the prefix/suffix arrays */
  static int
  ft_lzwstate_prefix_grow( FT_LzwState  state )
  {
    FT_UInt    old_size = state->prefix_size;
    FT_UInt    new_size = old_size;
    FT_Memory  memory   = state->memory;
    FT_Error   error;


    if ( new_size == 0 )  /* first allocation -> 9 bits */
      new_size = 512;
    else
      new_size += new_size >> 2;  /* don't grow too fast */

    /*
     *  Note that the `suffix' array is located in the same memory block
     *  pointed to by `prefix'.
     *
     *  I know that sizeof(FT_Byte) == 1 by definition, but it is clearer
     *  to write it literally.
     *
     */
    if ( FT_REALLOC_MULT( state->prefix, old_size, new_size,
                          sizeof ( FT_UShort ) + sizeof ( FT_Byte ) ) )
      return -1;

    /* now adjust `suffix' and move the data accordingly */
    state->suffix = (FT_Byte*)( state->prefix + new_size );

    FT_MEM_MOVE( state->suffix,
                 state->prefix + old_size,
                 old_size * sizeof ( FT_Byte ) );

    state->prefix_size = new_size;
    return 0;
  }


  FT_LOCAL_DEF( void )
  ft_lzwstate_reset( FT_LzwState  state )
  {
    state->in_cursor = state->in_buff;
    state->in_limit  = state->in_buff;
    state->in_eof    = 0;
    state->pad_bits  = 0;
    state->pad       = 0;

    state->stack_top = 0;
    state->num_bits  = LZW_INIT_BITS;
    state->phase     = FT_LZW_PHASE_START;
  }


  FT_LOCAL_DEF( void )
  ft_lzwstate_init( FT_LzwState  state,
                    FT_Stream    source )
  {
    FT_ZERO( state );

    state->source = source;
    state->memory = source->memory;

    state->prefix      = NULL;
    state->suffix      = NULL;
    state->prefix_size = 0;

    state->stack      = state->stack_0;
    state->stack_size = sizeof ( state->stack_0 );

    ft_lzwstate_reset( state );
  }


  FT_LOCAL_DEF( void )
  ft_lzwstate_done( FT_LzwState  state )
  {
    FT_Memory  memory = state->memory;


    ft_lzwstate_reset( state );

    if ( state->stack != state->stack_0 )
      FT_FREE( state->stack );

    FT_FREE( state->prefix );
    state->suffix = NULL;

    FT_ZERO( state );
  }


#define FTLZW_STACK_PUSH( c )                          \
  FT_BEGIN_STMNT                                       \
    if ( state->stack_top >= state->stack_size &&      \
         ft_lzwstate_stack_grow( state ) < 0   )       \
      goto Eof;                                        \
                                                       \
    state->stack[ state->stack_top++ ] = (FT_Byte)(c); \
  FT_END_STMNT


  FT_LOCAL_DEF( FT_ULong )
  ft_lzwstate_io( FT_LzwState  state,
                  FT_Byte*     buffer,
                  FT_ULong     out_size )
  {
    FT_ULong  result = 0;

    FT_UInt  num_bits = state->num_bits;
    FT_UInt  free_ent = state->free_ent;
    FT_UInt  old_char = state->old_char;
    FT_UInt  old_code = state->old_code;
    FT_UInt  in_code  = state->in_code;


    if ( out_size == 0 )
      goto Exit;

    switch ( state->phase )
    {
    case FT_LZW_PHASE_START:
      {
        FT_Byte   max_bits;
        FT_Int32  c;


        /* skip magic bytes, and read max_bits + block_flag */
        if ( FT_Stream_Seek( state->source, 2 ) != 0               ||
             FT_Stream_TryRead( state->source, &max_bits, 1 ) != 1 )
          goto Eof;

        state->max_bits   = max_bits & LZW_BIT_MASK;
        state->block_mode = max_bits & LZW_BLOCK_MASK;
        state->max_free   = (FT_UInt)( ( 1UL << state->max_bits ) - 256 );

        if ( state->max_bits > LZW_MAX_BITS )
          goto Eof;

        num_bits = LZW_INIT_BITS;
        free_ent = ( state->block_mode ? LZW_FIRST : LZW_CLEAR ) - 256;
        in_code  = 0;

        state->free_bits = num_bits < state->max_bits
                           ? (FT_UInt)( ( 1UL << num_bits ) - 256 )
                           : state->max_free + 1;

        c = ft_lzwstate_get_code( state, num_bits );
        if ( c < 0 )
          goto Eof;

        old_code = old_char = (FT_UInt)c;

        if ( buffer )
          buffer[result] = (FT_Byte)old_char;

        if ( ++result >= out_size )
          goto Exit;

        state->phase = FT_LZW_PHASE_CODE;
      }
      /* fall-through */

    case FT_LZW_PHASE_CODE:
      {
        FT_Int32  c;
        FT_UInt   code;


      NextCode:
        c = ft_lzwstate_get_code( state, num_bits );
        if ( c < 0 )
          goto Eof;

        code = (FT_UInt)c;

        if ( code == LZW_CLEAR && state->block_mode )
        {
          free_ent = ( LZW_FIRST - 1 ) - 256; /* why not LZW_FIRST-256 ? */
          num_bits = LZW_INIT_BITS;

          state->free_bits = num_bits < state->max_bits
                             ? (FT_UInt)( ( 1UL << num_bits ) - 256 )
                             : state->max_free + 1;

          c = ft_lzwstate_get_code( state, num_bits );
          if ( c < 0 )
            goto Eof;

          code = (FT_UInt)c;
        }

        in_code = code; /* save code for later */

        if ( code >= 256U )
        {
          /* special case for KwKwKwK */
          if ( code - 256U >= free_ent )
          {
            FTLZW_STACK_PUSH( old_char );
            code = old_code;
          }

          while ( code >= 256U )
          {
            FTLZW_STACK_PUSH( state->suffix[code - 256] );
            code = state->prefix[code - 256];
          }
        }

        old_char = code;
        FTLZW_STACK_PUSH( old_char );

        state->phase = FT_LZW_PHASE_STACK;
      }
      /* fall-through */

    case FT_LZW_PHASE_STACK:
      {
        while ( state->stack_top > 0 )
        {
          --state->stack_top;

          if ( buffer )
            buffer[result] = state->stack[state->stack_top];

          if ( ++result == out_size )
            goto Exit;
        }

        /* now create new entry */
        if ( free_ent < state->max_free )
        {
          if ( free_ent >= state->prefix_size       &&
               ft_lzwstate_prefix_grow( state ) < 0 )
            goto Eof;

          FT_ASSERT( free_ent < state->prefix_size );

          state->prefix[free_ent] = (FT_UShort)old_code;
          state->suffix[free_ent] = (FT_Byte)  old_char;

          if ( ++free_ent == state->free_bits )
          {
            num_bits++;

            state->free_bits = num_bits < state->max_bits
                               ? (FT_UInt)( ( 1UL << num_bits ) - 256 )
                               : state->max_free + 1;
          }
        }

        old_code = in_code;

        state->phase = FT_LZW_PHASE_CODE;
        goto NextCode;
      }

    default:  /* state == EOF */
      ;
    }

  Exit:
    state->num_bits = num_bits;
    state->free_ent = free_ent;
    state->old_code = old_code;
    state->old_char = old_char;
    state->in_code  = in_code;

    return result;

  Eof:
    state->phase = FT_LZW_PHASE_EOF;
    goto Exit;
  }


/* END */



#else  /* !FT_CONFIG_OPTION_USE_LZW */


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenLZW( FT_Stream  stream,
                     FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return LZW_Err_Unimplemented_Feature;
  }


#endif /* !FT_CONFIG_OPTION_USE_LZW */


/* END */

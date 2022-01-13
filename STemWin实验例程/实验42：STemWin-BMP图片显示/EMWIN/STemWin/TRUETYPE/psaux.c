/***************************************************************************/
/*                                                                         */
/*  psaux.c                                                                */
/*                                                                         */
/*    FreeType auxiliary PostScript driver component (body only).          */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2006 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#define FT_MAKE_OPTION_SINGLE_OBJECT

#include "ft2build.h"
/***************************************************************************/
/*                                                                         */
/*  psobjs.c                                                               */
/*                                                                         */
/*    Auxiliary functions for PostScript fonts (body).                     */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "psobjs.h"
#include "psconv.h"

#include "psauxerr.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                             PS_TABLE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_new                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a PS_Table.                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The address of the target table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    count  :: The table size = the maximum number of elements.         */
  /*                                                                       */
  /*    memory :: The memory object to use for all subsequent              */
  /*              reallocations.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  ps_table_new( PS_Table   table,
                FT_Int     count,
                FT_Memory  memory )
  {
    FT_Error  error;


    table->memory = memory;
    if ( FT_NEW_ARRAY( table->elements, count ) ||
         FT_NEW_ARRAY( table->lengths,  count ) )
      goto Exit;

    table->max_elems = count;
    table->init      = 0xDEADBEEFUL;
    table->num_elems = 0;
    table->block     = 0;
    table->capacity  = 0;
    table->cursor    = 0;

    *(PS_Table_FuncsRec*)&table->funcs = ps_table_funcs;

  Exit:
    if ( error )
      FT_FREE( table->elements );

    return error;
  }


  static void
  shift_elements( PS_Table  table,
                  FT_Byte*  old_base )
  {
    FT_PtrDist  delta  = table->block - old_base;
    FT_Byte**   offset = table->elements;
    FT_Byte**   limit  = offset + table->max_elems;


    for ( ; offset < limit; offset++ )
    {
      if ( offset[0] )
        offset[0] += delta;
    }
  }


  static FT_Error
  reallocate_t1_table( PS_Table  table,
                       FT_Long   new_size )
  {
    FT_Memory  memory   = table->memory;
    FT_Byte*   old_base = table->block;
    FT_Error   error;


    /* allocate new base block */
    if ( FT_ALLOC( table->block, new_size ) )
    {
      table->block = old_base;
      return error;
    }

    /* copy elements and shift offsets */
    if (old_base )
    {
      FT_MEM_COPY( table->block, old_base, table->capacity );
      shift_elements( table, old_base );
      FT_FREE( old_base );
    }

    table->capacity = new_size;

    return PSaux_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_add                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds an object to a PS_Table, possibly growing its memory block.   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The target table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    idx    :: The index of the object in the table.                    */
  /*                                                                       */
  /*    object :: The address of the object to copy in memory.             */
  /*                                                                       */
  /*    length :: The length in bytes of the source object.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  An error is returned if a  */
  /*    reallocation fails.                                                */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  ps_table_add( PS_Table    table,
                FT_Int      idx,
                void*       object,
                FT_PtrDist  length )
  {
    if ( idx < 0 || idx > table->max_elems )
    {
      FT_ERROR(( "ps_table_add: invalid index\n" ));
      return PSaux_Err_Invalid_Argument;
    }

    /* grow the base block if needed */
    if ( table->cursor + length > table->capacity )
    {
      FT_Error   error;
      FT_Offset  new_size  = table->capacity;
      FT_Long    in_offset;


      in_offset = (FT_Long)((FT_Byte*)object - table->block);
      if ( (FT_ULong)in_offset >= table->capacity )
        in_offset = -1;

      while ( new_size < table->cursor + length )
      {
        /* increase size by 25% and round up to the nearest multiple
           of 1024 */
        new_size += ( new_size >> 2 ) + 1;
        new_size  = FT_PAD_CEIL( new_size, 1024 );
      }

      error = reallocate_t1_table( table, new_size );
      if ( error )
        return error;

      if ( in_offset >= 0 )
        object = table->block + in_offset;
    }

    /* add the object to the base block and adjust offset */
    table->elements[idx] = table->block + table->cursor;
    table->lengths [idx] = length;
    FT_MEM_COPY( table->block + table->cursor, object, length );

    table->cursor += length;
    return PSaux_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_done                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a PS_TableRec (i.e., reallocate it to its current        */
  /*    cursor).                                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table :: The target table.                                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT release the heap's memory block.  It is up  */
  /*    to the caller to clean it, or reference it in its own structures.  */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  ps_table_done( PS_Table  table )
  {
    FT_Memory  memory = table->memory;
    FT_Error   error;
    FT_Byte*   old_base = table->block;


    /* should never fail, because rec.cursor <= rec.size */
    if ( !old_base )
      return;

    if ( FT_ALLOC( table->block, table->cursor ) )
      return;
    FT_MEM_COPY( table->block, old_base, table->cursor );
    shift_elements( table, old_base );

    table->capacity = table->cursor;
    FT_FREE( old_base );

    FT_UNUSED( error );
  }


  FT_LOCAL_DEF( void )
  ps_table_release( PS_Table  table )
  {
    FT_Memory  memory = table->memory;


    if ( (FT_ULong)table->init == 0xDEADBEEFUL )
    {
      FT_FREE( table->block );
      FT_FREE( table->elements );
      FT_FREE( table->lengths );
      table->init = 0;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            T1 PARSER                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* first character must be already part of the comment */

  static void
  skip_comment( FT_Byte*  *acur,
                FT_Byte*   limit )
  {
    FT_Byte*  cur = *acur;


    while ( cur < limit )
    {
      if ( IS_PS_NEWLINE( *cur ) )
        break;
      cur++;
    }

    *acur = cur;
  }


  static void
  skip_spaces( FT_Byte*  *acur,
               FT_Byte*   limit )
  {
    FT_Byte*  cur = *acur;


    while ( cur < limit )
    {
      if ( !IS_PS_SPACE( *cur ) )
      {
        if ( *cur == '%' )
          /* According to the PLRM, a comment is equal to a space. */
          skip_comment( &cur, limit );
        else
          break;
      }
      cur++;
    }

    *acur = cur;
  }


  /* first character must be `(' */

  static void
  skip_literal_string( FT_Byte*  *acur,
                       FT_Byte*   limit )
  {
    FT_Byte*  cur   = *acur;
    FT_Int    embed = 0;


    while ( cur < limit )
    {
      if ( *cur == '\\' )
        cur++;
      else if ( *cur == '(' )
        embed++;
      else if ( *cur == ')' )
      {
        embed--;
        if ( embed == 0 )
        {
          cur++;
          break;
        }
      }
      cur++;
    }

    *acur = cur;
  }


  /* first character must be `<' */

  static void
  skip_string( PS_Parser  parser )
  {
    FT_Byte*  cur   = parser->cursor;
    FT_Byte*  limit = parser->limit;


    while ( ++cur < limit )
    {
      /* All whitespace characters are ignored. */
      skip_spaces( &cur, limit );
      if ( cur >= limit )
        break;

      if ( !IS_PS_XDIGIT( *cur ) )
        break;
    }

    if ( cur < limit && *cur != '>' )
    {
      FT_ERROR(( "skip_string: missing closing delimiter `>'\n" ));
      parser->error = PSaux_Err_Invalid_File_Format;
    }
    else
      cur++;

    parser->cursor = cur;
  }


  /***********************************************************************/
  /*                                                                     */
  /* All exported parsing routines handle leading whitespace and stop at */
  /* the first character which isn't part of the just handled token.     */
  /*                                                                     */
  /***********************************************************************/


  FT_LOCAL_DEF( void )
  ps_parser_skip_PS_token( PS_Parser  parser )
  {
    /* Note: PostScript allows any non-delimiting, non-whitespace        */
    /*       character in a name (PS Ref Manual, 3rd ed, p31).           */
    /*       PostScript delimiters are (, ), <, >, [, ], {, }, /, and %. */

    FT_Byte*  cur   = parser->cursor;
    FT_Byte*  limit = parser->limit;


    skip_spaces( &cur, limit );             /* this also skips comments */
    if ( cur >= limit )
      goto Exit;

    /* self-delimiting, single-character tokens */
    if ( *cur == '[' || *cur == ']' ||
         *cur == '{' || *cur == '}' )
    {
      cur++;
      goto Exit;
    }

    if ( *cur == '(' )                              /* (...) */
    {
      skip_literal_string( &cur, limit );
      goto Exit;
    }

    if ( *cur == '<' )                              /* <...> */
    {
      if ( cur + 1 < limit && *(cur + 1) == '<' )   /* << */
      {
        cur++;
        cur++;
        goto Exit;
      }
      parser->cursor = cur;
      skip_string( parser );
      return;
    }

    if ( *cur == '>' )
    {
      cur++;
      if ( cur >= limit || *cur != '>' )             /* >> */
      {
        FT_ERROR(( "ps_parser_skip_PS_token: "
                   "unexpected closing delimiter `>'\n" ));
        parser->error = PSaux_Err_Invalid_File_Format;
        goto Exit;
      }
      cur++;
      goto Exit;
    }

    if ( *cur == '/' )
      cur++;

    /* anything else */
    while ( cur < limit )
    {
      if ( *cur == ')' )
      {
        FT_ERROR(( "ps_parser_skip_PS_token: "
                   "unexpected closing delimiter `)'\n" ));
        parser->error = PSaux_Err_Invalid_File_Format;
        goto Exit;
      }
      else if ( IS_PS_DELIM( *cur ) )
        break;

      cur++;
    }

  Exit:
    parser->cursor = cur;
  }


  FT_LOCAL_DEF( void )
  ps_parser_skip_spaces( PS_Parser  parser )
  {
    skip_spaces( &parser->cursor, parser->limit );
  }


  /* `token' here means either something between balanced delimiters */
  /* or the next token; the delimiters are not removed.              */

  FT_LOCAL_DEF( void )
  ps_parser_to_token( PS_Parser  parser,
                      T1_Token   token )
  {
    FT_Byte*  cur;
    FT_Byte*  limit;
    FT_Byte   starter, ender;
    FT_Int    embed;


    token->type  = T1_TOKEN_TYPE_NONE;
    token->start = 0;
    token->limit = 0;

    /* first of all, skip leading whitespace */
    ps_parser_skip_spaces( parser );

    cur   = parser->cursor;
    limit = parser->limit;

    if ( cur >= limit )
      return;

    switch ( *cur )
    {
      /************* check for literal string *****************/
    case '(':
      token->type  = T1_TOKEN_TYPE_STRING;
      token->start = cur;
      skip_literal_string( &cur, limit );
      if ( cur < limit )
        token->limit = cur;
      break;

      /************* check for programs/array *****************/
    case '{':
      token->type = T1_TOKEN_TYPE_ARRAY;
      ender = '}';
      goto Lookup_Ender;

      /************* check for table/array ********************/
    case '[':
      token->type = T1_TOKEN_TYPE_ARRAY;
      ender = ']';
      /* fall through */

    Lookup_Ender:
      embed        = 1;
      starter      = *cur;
      token->start = cur++;

      /* we need this to catch `[ ]' */
      parser->cursor = cur;
      ps_parser_skip_spaces( parser );
      cur = parser->cursor;

      while ( cur < limit && !parser->error )
      {
        if ( *cur == starter )
          embed++;
        else if ( *cur == ender )
        {
          embed--;
          if ( embed <= 0 )
          {
            token->limit = ++cur;
            break;
          }
        }

        parser->cursor = cur;
        ps_parser_skip_PS_token( parser );
        /* we need this to catch `[XXX ]' */
        ps_parser_skip_spaces  ( parser );
        cur = parser->cursor;
      }
      break;

      /* ************ otherwise, it is any token **************/
    default:
      token->start = cur;
      token->type  = T1_TOKEN_TYPE_ANY;
      ps_parser_skip_PS_token( parser );
      cur = parser->cursor;
      if ( !parser->error )
        token->limit = cur;
    }

    if ( !token->limit )
    {
      token->start = 0;
      token->type  = T1_TOKEN_TYPE_NONE;
    }

    parser->cursor = cur;
  }


  FT_LOCAL_DEF( void )
  ps_parser_to_token_array( PS_Parser  parser,
                            T1_Token   tokens,
                            FT_UInt    max_tokens,
                            FT_Int*    pnum_tokens )
  {
    T1_TokenRec  master;


    *pnum_tokens = -1;

    /* this also handles leading whitespace */
    ps_parser_to_token( parser, &master );

    if ( master.type == T1_TOKEN_TYPE_ARRAY )
    {
      FT_Byte*  old_cursor = parser->cursor;
      FT_Byte*  old_limit  = parser->limit;
      T1_Token  cur        = tokens;
      T1_Token  limit      = cur + max_tokens;


      /* don't include outermost delimiters */
      parser->cursor = master.start + 1;
      parser->limit  = master.limit - 1;

      while ( parser->cursor < parser->limit )
      {
        T1_TokenRec  token;


        ps_parser_to_token( parser, &token );
        if ( !token.type )
          break;

        if ( cur < limit )
          *cur = token;

        cur++;
      }

      *pnum_tokens = (FT_Int)( cur - tokens );

      parser->cursor = old_cursor;
      parser->limit  = old_limit;
    }
  }


  /* first character must be a delimiter or a part of a number */

  static FT_Int
  ps_tocoordarray( FT_Byte*  *acur,
                   FT_Byte*   limit,
                   FT_Int     max_coords,
                   FT_Short*  coords )
  {
    FT_Byte*  cur   = *acur;
    FT_Int    count = 0;
    FT_Byte   c, ender;


    if ( cur >= limit )
      goto Exit;

    /* check for the beginning of an array; otherwise, only one number */
    /* will be read                                                    */
    c     = *cur;
    ender = 0;

    if ( c == '[' )
      ender = ']';

    if ( c == '{' )
      ender = '}';

    if ( ender )
      cur++;

    /* now, read the coordinates */
    while ( cur < limit )
    {
      /* skip whitespace in front of data */
      skip_spaces( &cur, limit );
      if ( cur >= limit )
        goto Exit;

      if ( count >= max_coords )
        break;

      if ( c == ender )
      {
        cur++;
        break;
      }

      coords[count] =
        (FT_Short)( PS_Conv_ToFixed( &cur, limit, 0 ) >> 16 );
      count++;

      if ( !ender )
        break;
    }

  Exit:
    *acur = cur;
    return count;
  }


  /* first character must be a delimiter or a part of a number */

  static FT_Int
  ps_tofixedarray( FT_Byte*  *acur,
                   FT_Byte*   limit,
                   FT_Int     max_values,
                   FT_Fixed*  values,
                   FT_Int     power_ten )
  {
    FT_Byte*  cur   = *acur;
    FT_Int    count = 0;
    FT_Byte   c, ender;


    if ( cur >= limit )
      goto Exit;

    /* Check for the beginning of an array.  Otherwise, only one number */
    /* will be read.                                                    */
    c     = *cur;
    ender = 0;

    if ( c == '[' )
      ender = ']';

    if ( c == '{' )
      ender = '}';

    if ( ender )
      cur++;

    /* now, read the values */
    while ( cur < limit )
    {
      /* skip whitespace in front of data */
      skip_spaces( &cur, limit );
      if ( cur >= limit )
        goto Exit;

      if ( count >= max_values )
        break;

      if ( c == ender )
      {
        cur++;
        break;
      }

      values[count] = PS_Conv_ToFixed( &cur, limit, power_ten );
      count++;

      if ( !ender )
        break;
    }

  Exit:
    *acur = cur;
    return count;
  }


#if 0

  static FT_String*
  ps_tostring( FT_Byte**  cursor,
               FT_Byte*   limit,
               FT_Memory  memory )
  {
    FT_Byte*    cur = *cursor;
    FT_PtrDist  len = 0;
    FT_Int      count;
    FT_String*  result;
    FT_Error    error;


    /* XXX: some stupid fonts have a `Notice' or `Copyright' string     */
    /*      that simply doesn't begin with an opening parenthesis, even */
    /*      though they have a closing one!  E.g. "amuncial.pfb"        */
    /*                                                                  */
    /*      We must deal with these ill-fated cases there.  Note that   */
    /*      these fonts didn't work with the old Type 1 driver as the   */
    /*      notice/copyright was not recognized as a valid string token */
    /*      and made the old token parser commit errors.                */

    while ( cur < limit && ( *cur == ' ' || *cur == '\t' ) )
      cur++;
    if ( cur + 1 >= limit )
      return 0;

    if ( *cur == '(' )
      cur++;  /* skip the opening parenthesis, if there is one */

    *cursor = cur;
    count   = 0;

    /* then, count its length */
    for ( ; cur < limit; cur++ )
    {
      if ( *cur == '(' )
        count++;

      else if ( *cur == ')' )
      {
        count--;
        if ( count < 0 )
          break;
      }
    }

    len = cur - *cursor;
    if ( cur >= limit || FT_ALLOC( result, len + 1 ) )
      return 0;

    /* now copy the string */
    FT_MEM_COPY( result, *cursor, len );
    result[len] = '\0';
    *cursor = cur;
    return result;
  }

#endif /* 0 */


  static int
  ps_tobool( FT_Byte*  *acur,
             FT_Byte*   limit )
  {
    FT_Byte*  cur    = *acur;
    FT_Bool   result = 0;


    /* return 1 if we find `true', 0 otherwise */
    if ( cur + 3 < limit &&
         cur[0] == 't'   &&
         cur[1] == 'r'   &&
         cur[2] == 'u'   &&
         cur[3] == 'e'   )
    {
      result = 1;
      cur   += 5;
    }
    else if ( cur + 4 < limit &&
              cur[0] == 'f'   &&
              cur[1] == 'a'   &&
              cur[2] == 'l'   &&
              cur[3] == 's'   &&
              cur[4] == 'e'   )
    {
      result = 0;
      cur   += 6;
    }

    *acur = cur;
    return result;
  }


  /* load a simple field (i.e. non-table) into the current list of objects */

  FT_LOCAL_DEF( FT_Error )
  ps_parser_load_field( PS_Parser       parser,
                        const T1_Field  field,
                        void**          objects,
                        FT_UInt         max_objects,
                        FT_ULong*       pflags )
  {
    T1_TokenRec  token;
    FT_Byte*     cur;
    FT_Byte*     limit;
    FT_UInt      count;
    FT_UInt      idx;
    FT_Error     error;


    /* this also skips leading whitespace */
    ps_parser_to_token( parser, &token );
    if ( !token.type )
      goto Fail;

    count = 1;
    idx   = 0;
    cur   = token.start;
    limit = token.limit;

    /* we must detect arrays in /FontBBox */
    if ( field->type == T1_FIELD_TYPE_BBOX )
    {
      T1_TokenRec  token2;
      FT_Byte*     old_cur   = parser->cursor;
      FT_Byte*     old_limit = parser->limit;


      /* don't include delimiters */
      parser->cursor = token.start + 1;
      parser->limit  = token.limit - 1;

      ps_parser_to_token( parser, &token2 );
      parser->cursor = old_cur;
      parser->limit  = old_limit;

      if ( token2.type == T1_TOKEN_TYPE_ARRAY )
        goto FieldArray;
    }
    else if ( token.type == T1_TOKEN_TYPE_ARRAY )
    {
    FieldArray:
      /* if this is an array and we have no blend, an error occurs */
      if ( max_objects == 0 )
        goto Fail;

      count = max_objects;
      idx   = 1;

      /* don't include delimiters */
      cur++;
      limit--;
    }

    for ( ; count > 0; count--, idx++ )
    {
      FT_Byte*    q = (FT_Byte*)objects[idx] + field->offset;
      FT_Long     val;
      FT_String*  string;


      skip_spaces( &cur, limit );

      switch ( field->type )
      {
      case T1_FIELD_TYPE_BOOL:
        val = ps_tobool( &cur, limit );
        goto Store_Integer;

      case T1_FIELD_TYPE_FIXED:
        val = PS_Conv_ToFixed( &cur, limit, 0 );
        goto Store_Integer;

      case T1_FIELD_TYPE_FIXED_1000:
        val = PS_Conv_ToFixed( &cur, limit, 3 );
        goto Store_Integer;

      case T1_FIELD_TYPE_INTEGER:
        val = PS_Conv_ToInt( &cur, limit );
        /* fall through */

      Store_Integer:
        switch ( field->size )
        {
        case (8 / FT_CHAR_BIT):
          *(FT_Byte*)q = (FT_Byte)val;
          break;

        case (16 / FT_CHAR_BIT):
          *(FT_UShort*)q = (FT_UShort)val;
          break;

        case (32 / FT_CHAR_BIT):
          *(FT_UInt32*)q = (FT_UInt32)val;
          break;

        default:                /* for 64-bit systems */
          *(FT_Long*)q = val;
        }
        break;

      case T1_FIELD_TYPE_STRING:
      case T1_FIELD_TYPE_KEY:
        {
          FT_Memory  memory = parser->memory;
          FT_UInt    len    = (FT_UInt)( limit - cur );


          if ( cur >= limit )
            break;

          if ( field->type == T1_FIELD_TYPE_KEY )
          {
            /* don't include leading `/' */
            len--;
            cur++;
          }
          else
          {
            /* don't include delimiting parentheses */
            cur++;
            len -= 2;
          }

          if ( FT_ALLOC( string, len + 1 ) )
            goto Exit;

          FT_MEM_COPY( string, cur, len );
          string[len] = 0;

          *(FT_String**)q = string;
        }
        break;

      case T1_FIELD_TYPE_BBOX:
        {
          FT_Fixed  temp[4];
          FT_BBox*  bbox = (FT_BBox*)q;


          (void)ps_tofixedarray( &token.start, token.limit, 4, temp, 0 );

          bbox->xMin = FT_RoundFix( temp[0] );
          bbox->yMin = FT_RoundFix( temp[1] );
          bbox->xMax = FT_RoundFix( temp[2] );
          bbox->yMax = FT_RoundFix( temp[3] );
        }
        break;

      default:
        /* an error occured */
        goto Fail;
      }
    }

#if 0  /* obsolete -- keep for reference */
    if ( pflags )
      *pflags |= 1L << field->flag_bit;
#else
    FT_UNUSED( pflags );
#endif

    error = PSaux_Err_Ok;

  Exit:
    return error;

  Fail:
    error = PSaux_Err_Invalid_File_Format;
    goto Exit;
  }


#define T1_MAX_TABLE_ELEMENTS  32


  FT_LOCAL_DEF( FT_Error )
  ps_parser_load_field_table( PS_Parser       parser,
                              const T1_Field  field,
                              void**          objects,
                              FT_UInt         max_objects,
                              FT_ULong*       pflags )
  {
    T1_TokenRec  elements[T1_MAX_TABLE_ELEMENTS];
    T1_Token     token;
    FT_Int       num_elements;
    FT_Error     error = PSaux_Err_Ok;
    FT_Byte*     old_cursor;
    FT_Byte*     old_limit;
    T1_FieldRec  fieldrec = *(T1_Field)field;


#if 1
    fieldrec.type = T1_FIELD_TYPE_INTEGER;
    if ( field->type == T1_FIELD_TYPE_FIXED_ARRAY )
      fieldrec.type = T1_FIELD_TYPE_FIXED;
#endif

    ps_parser_to_token_array( parser, elements,
                              T1_MAX_TABLE_ELEMENTS, &num_elements );
    if ( num_elements < 0 )
    {
      error = PSaux_Err_Ignore;
      goto Exit;
    }
    if ( num_elements > T1_MAX_TABLE_ELEMENTS )
      num_elements = T1_MAX_TABLE_ELEMENTS;

    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    /* we store the elements count */
    *(FT_Byte*)( (FT_Byte*)objects[0] + field->count_offset ) =
      (FT_Byte)num_elements;

    /* we now load each element, adjusting the field.offset on each one */
    token = elements;
    for ( ; num_elements > 0; num_elements--, token++ )
    {
      parser->cursor = token->start;
      parser->limit  = token->limit;
      ps_parser_load_field( parser, &fieldrec, objects, max_objects, 0 );
      fieldrec.offset += fieldrec.size;
    }

#if 0  /* obsolete -- keep for reference */
    if ( pflags )
      *pflags |= 1L << field->flag_bit;
#else
    FT_UNUSED( pflags );
#endif

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Long )
  ps_parser_to_int( PS_Parser  parser )
  {
    ps_parser_skip_spaces( parser );
    return PS_Conv_ToInt( &parser->cursor, parser->limit );
  }


  /* first character must be `<' if `delimiters' is non-zero */

  FT_LOCAL_DEF( FT_Error )
  ps_parser_to_bytes( PS_Parser  parser,
                      FT_Byte*   bytes,
                      FT_Long    max_bytes,
                      FT_Long*   pnum_bytes,
                      FT_Bool    delimiters )
  {
    FT_Error  error = PSaux_Err_Ok;
    FT_Byte*  cur;
    
    
    ps_parser_skip_spaces( parser );
    cur = parser->cursor;

    if ( cur >= parser->limit )
      goto Exit;

    if ( delimiters )
    {
      if ( *cur != '<' )
      {
        FT_ERROR(( "ps_parser_to_bytes: Missing starting delimiter `<'\n" ));
        error = PSaux_Err_Invalid_File_Format;
        goto Exit;
      }

      cur++;
    }

    *pnum_bytes = PS_Conv_ASCIIHexDecode( &cur,
                                          parser->limit,
                                          bytes,
                                          max_bytes );

    if ( delimiters )
    {
      if ( cur < parser->limit && *cur != '>' )
      {
        FT_ERROR(( "ps_tobytes: Missing closing delimiter `>'\n" ));
        error = PSaux_Err_Invalid_File_Format;
        goto Exit;
      }

      cur++;
    }

    parser->cursor = cur;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Fixed )
  ps_parser_to_fixed( PS_Parser  parser,
                      FT_Int     power_ten )
  {
    ps_parser_skip_spaces( parser );
    return PS_Conv_ToFixed( &parser->cursor, parser->limit, power_ten );
  }


  FT_LOCAL_DEF( FT_Int )
  ps_parser_to_coord_array( PS_Parser  parser,
                            FT_Int     max_coords,
                            FT_Short*  coords )
  {
    ps_parser_skip_spaces( parser );
    return ps_tocoordarray( &parser->cursor, parser->limit,
                            max_coords, coords );
  }


  FT_LOCAL_DEF( FT_Int )
  ps_parser_to_fixed_array( PS_Parser  parser,
                            FT_Int     max_values,
                            FT_Fixed*  values,
                            FT_Int     power_ten )
  {
    ps_parser_skip_spaces( parser );
    return ps_tofixedarray( &parser->cursor, parser->limit,
                            max_values, values, power_ten );
  }


#if 0

  FT_LOCAL_DEF( FT_String* )
  T1_ToString( PS_Parser  parser )
  {
    return ps_tostring( &parser->cursor, parser->limit, parser->memory );
  }


  FT_LOCAL_DEF( FT_Bool )
  T1_ToBool( PS_Parser  parser )
  {
    return ps_tobool( &parser->cursor, parser->limit );
  }

#endif /* 0 */


  FT_LOCAL_DEF( void )
  ps_parser_init( PS_Parser  parser,
                  FT_Byte*   base,
                  FT_Byte*   limit,
                  FT_Memory  memory )
  {
    parser->error  = PSaux_Err_Ok;
    parser->base   = base;
    parser->limit  = limit;
    parser->cursor = base;
    parser->memory = memory;
    parser->funcs  = ps_parser_funcs;
  }


  FT_LOCAL_DEF( void )
  ps_parser_done( PS_Parser  parser )
  {
    FT_UNUSED( parser );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            T1 BUILDER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_builder_init                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph builder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    builder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    glyph   :: The current glyph object.                               */
  /*                                                                       */
  /*    hinting :: Whether hinting should be applied.                      */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  t1_builder_init( T1_Builder    builder,
                   FT_Face       face,
                   FT_Size       size,
                   FT_GlyphSlot  glyph,
                   FT_Bool       hinting )
  {
    builder->parse_state = T1_Parse_Start;
    builder->load_points = 1;

    builder->face   = face;
    builder->glyph  = glyph;
    builder->memory = face->memory;

    if ( glyph )
    {
      FT_GlyphLoader  loader = glyph->internal->loader;


      builder->loader  = loader;
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
      FT_GlyphLoader_Rewind( loader );

      builder->hints_globals = size->internal;
      builder->hints_funcs   = 0;

      if ( hinting )
        builder->hints_funcs = glyph->internal->glyph_hints;
    }

    if ( size )
    {
      builder->scale_x = size->metrics.x_scale;
      builder->scale_y = size->metrics.y_scale;
    }

    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;

    builder->funcs = t1_builder_funcs;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_builder_done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  t1_builder_done( T1_Builder  builder )
  {
    FT_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->outline = *builder->base;
  }


  /* check that there is enough space for `count' more points */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_check_points( T1_Builder  builder,
                           FT_Int      count )
  {
    return FT_GLYPHLOADER_CHECK_POINTS( builder->loader, count, 0 );
  }


  /* add a new point, do not check space */
  FT_LOCAL_DEF( void )
  t1_builder_add_point( T1_Builder  builder,
                        FT_Pos      x,
                        FT_Pos      y,
                        FT_Byte     flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      if ( builder->shift )
      {
        x >>= 16;
        y >>= 16;
      }
      point->x = x;
      point->y = y;
      *control = (FT_Byte)( flag ? FT_CURVE_TAG_ON : FT_CURVE_TAG_CUBIC );

      builder->last = *point;
    }
    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_add_point1( T1_Builder  builder,
                         FT_Pos      x,
                         FT_Pos      y )
  {
    FT_Error  error;


    error = t1_builder_check_points( builder, 1 );
    if ( !error )
      t1_builder_add_point( builder, x, y, 1 );

    return error;
  }


  /* check space for a new contour, then add it */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_add_contour( T1_Builder  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return PSaux_Err_Ok;
    }

    error = FT_GLYPHLOADER_CHECK_POINTS( builder->loader, 0, 1 );
    if ( !error )
    {
      if ( outline->n_contours > 0 )
        outline->contours[outline->n_contours - 1] =
          (short)( outline->n_points - 1 );

      outline->n_contours++;
    }

    return error;
  }


  /* if a path was begun, add its first on-curve point */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_start_point( T1_Builder  builder,
                          FT_Pos      x,
                          FT_Pos      y )
  {
    FT_Error  error = PSaux_Err_Invalid_File_Format;


    /* test whether we are building a new contour */

    if ( builder->parse_state == T1_Parse_Have_Path )
      error = PSaux_Err_Ok;
    else if ( builder->parse_state == T1_Parse_Have_Moveto )
    {
      builder->parse_state = T1_Parse_Have_Path;
      error = t1_builder_add_contour( builder );
      if ( !error )
        error = t1_builder_add_point1( builder, x, y );
    }

    return error;
  }


  /* close the current contour */
  FT_LOCAL_DEF( void )
  t1_builder_close_contour( T1_Builder  builder )
  {
    FT_Outline*  outline = builder->current;


    if ( !outline )
      return;

    /* XXXX: We must not include the last point in the path if it */
    /*       is located on the first point.                       */
    if ( outline->n_points > 1 )
    {
      FT_Int      first   = 0;
      FT_Vector*  p1      = outline->points + first;
      FT_Vector*  p2      = outline->points + outline->n_points - 1;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points - 1;


      if ( outline->n_contours > 1 )
      {
        first = outline->contours[outline->n_contours - 2] + 1;
        p1    = outline->points + first;
      }

      /* `delete' last point only if it coincides with the first */
      /* point and it is not a control point (which can happen). */
      if ( p1->x == p2->x && p1->y == p2->y )
        if ( *control == FT_CURVE_TAG_ON )
          outline->n_points--;
    }

    if ( outline->n_contours > 0 )
      outline->contours[outline->n_contours - 1] =
        (short)( outline->n_points - 1 );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            OTHER                              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  t1_decrypt( FT_Byte*   buffer,
              FT_Offset  length,
              FT_UShort  seed )
  {
    PS_Conv_EexecDecode( &buffer,
                         buffer + length,
                         buffer,
                         length,
                         &seed );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  psauxmod.c                                                             */
/*                                                                         */
/*    FreeType auxiliary PostScript module implementation (body).          */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2006 by                               */
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
#include "psauxmod.h"
#include "psobjs.h"
#include "t1decode.h"
#include "t1cmap.h"

#ifndef T1_CONFIG_OPTION_NO_AFM
#include "afmparse.h"
#endif


  FT_CALLBACK_TABLE_DEF
  const PS_Table_FuncsRec  ps_table_funcs =
  {
    ps_table_new,
    ps_table_done,
    ps_table_add,
    ps_table_release
  };


  FT_CALLBACK_TABLE_DEF
  const PS_Parser_FuncsRec  ps_parser_funcs =
  {
    ps_parser_init,
    ps_parser_done,
    ps_parser_skip_spaces,
    ps_parser_skip_PS_token,
    ps_parser_to_int,
    ps_parser_to_fixed,
    ps_parser_to_bytes,
    ps_parser_to_coord_array,
    ps_parser_to_fixed_array,
    ps_parser_to_token,
    ps_parser_to_token_array,
    ps_parser_load_field,
    ps_parser_load_field_table
  };


  FT_CALLBACK_TABLE_DEF
  const T1_Builder_FuncsRec  t1_builder_funcs =
  {
    t1_builder_init,
    t1_builder_done,
    t1_builder_check_points,
    t1_builder_add_point,
    t1_builder_add_point1,
    t1_builder_add_contour,
    t1_builder_start_point,
    t1_builder_close_contour
  };


  FT_CALLBACK_TABLE_DEF
  const T1_Decoder_FuncsRec  t1_decoder_funcs =
  {
    t1_decoder_init,
    t1_decoder_done,
    t1_decoder_parse_charstrings
  };


#ifndef T1_CONFIG_OPTION_NO_AFM
  FT_CALLBACK_TABLE_DEF
  const AFM_Parser_FuncsRec  afm_parser_funcs =
  {
    afm_parser_init,
    afm_parser_done,
    afm_parser_parse
  };
#endif


  FT_CALLBACK_TABLE_DEF
  const T1_CMap_ClassesRec  t1_cmap_classes =
  {
    &t1_cmap_standard_class_rec,
    &t1_cmap_expert_class_rec,
    &t1_cmap_custom_class_rec,
    &t1_cmap_unicode_class_rec
  };


  static
  const PSAux_Interface  psaux_interface =
  {
    &ps_table_funcs,
    &ps_parser_funcs,
    &t1_builder_funcs,
    &t1_decoder_funcs,
    t1_decrypt,

    (const T1_CMap_ClassesRec*) &t1_cmap_classes,

#ifndef T1_CONFIG_OPTION_NO_AFM
    &afm_parser_funcs,
#else
    0,
#endif
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  psaux_module_class =
  {
    0,
    sizeof( FT_ModuleRec ),
    "psaux",
    0x20000L,
    0x20000L,

    &psaux_interface,  /* module-specific interface */

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  0
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  t1decode.c                                                             */
/*                                                                         */
/*    PostScript Type 1 decoding routines (body).                          */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_OUTLINE_H

#include "t1decode.h"
#include "psobjs.h"

#include "psauxerr.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1decode


  typedef enum  T1_Operator_
  {
    op_none = 0,
    op_endchar,
    op_hsbw,
    op_seac,
    op_sbw,
    op_closepath,
    op_hlineto,
    op_hmoveto,
    op_hvcurveto,
    op_rlineto,
    op_rmoveto,
    op_rrcurveto,
    op_vhcurveto,
    op_vlineto,
    op_vmoveto,
    op_dotsection,
    op_hstem,
    op_hstem3,
    op_vstem,
    op_vstem3,
    op_div,
    op_callothersubr,
    op_callsubr,
    op_pop,
    op_return,
    op_setcurrentpoint,

    op_max    /* never remove this one */

  } T1_Operator;


  static
  const FT_Int  t1_args_count[op_max] =
  {
    0, /* none */
    0, /* endchar */
    2, /* hsbw */
    5, /* seac */
    4, /* sbw */
    0, /* closepath */
    1, /* hlineto */
    1, /* hmoveto */
    4, /* hvcurveto */
    2, /* rlineto */
    2, /* rmoveto */
    6, /* rrcurveto */
    4, /* vhcurveto */
    1, /* vlineto */
    1, /* vmoveto */
    0, /* dotsection */
    2, /* hstem */
    6, /* hstem3 */
    2, /* vstem */
    6, /* vstem3 */
    2, /* div */
   -1, /* callothersubr */
    1, /* callsubr */
    0, /* pop */
    0, /* return */
    2  /* setcurrentpoint */
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_lookup_glyph_by_stdcharcode                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks up a given glyph by its StandardEncoding charcode.  Used to  */
  /*    implement the SEAC Type 1 operator.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: The current face object.                               */
  /*                                                                       */
  /*    charcode :: The character code to look for.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A glyph index in the font face.  Returns -1 if the corresponding   */
  /*    glyph wasn't found.                                                */
  /*                                                                       */
  static FT_Int
  t1_lookup_glyph_by_stdcharcode( T1_Decoder  decoder,
                                  FT_Int      charcode )
  {
    FT_UInt             n;
    const FT_String*    glyph_name;
    FT_Service_PsCMaps  psnames = decoder->psnames;


    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    glyph_name = psnames->adobe_std_strings(
                   psnames->adobe_std_encoding[charcode]);

    for ( n = 0; n < decoder->num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)decoder->glyph_names[n];


      if ( name && name[0] == glyph_name[0]  &&
           ft_strcmp( name, glyph_name ) == 0 )
        return n;
    }

    return -1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1operator_seac                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Implements the `seac' Type 1 operator for a Type 1 decoder.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder :: The current CID decoder.                                */
  /*                                                                       */
  /*    asb     :: The accent's side bearing.                              */
  /*                                                                       */
  /*    adx     :: The horizontal offset of the accent.                    */
  /*                                                                       */
  /*    ady     :: The vertical offset of the accent.                      */
  /*                                                                       */
  /*    bchar   :: The base character's StandardEncoding charcode.         */
  /*                                                                       */
  /*    achar   :: The accent character's StandardEncoding charcode.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  t1operator_seac( T1_Decoder  decoder,
                   FT_Pos      asb,
                   FT_Pos      adx,
                   FT_Pos      ady,
                   FT_Int      bchar,
                   FT_Int      achar )
  {
    FT_Error     error;
    FT_Int       bchar_index, achar_index;
#if 0
    FT_Int       n_base_points;
    FT_Outline*  base = decoder->builder.base;
#endif
    FT_Vector    left_bearing, advance;


    /* seac weirdness */
    adx += decoder->builder.left_bearing.x;

    /* `glyph_names' is set to 0 for CID fonts which do not */
    /* include an encoding.  How can we deal with these?    */
    if ( decoder->glyph_names == 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " glyph names table not available in this font!\n" ));
      return PSaux_Err_Syntax_Error;
    }

    bchar_index = t1_lookup_glyph_by_stdcharcode( decoder, bchar );
    achar_index = t1_lookup_glyph_by_stdcharcode( decoder, achar );

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return PSaux_Err_Syntax_Error;
    }

    /* if we are trying to load a composite glyph, do not load the */
    /* accent character and return the array of subglyphs.         */
    if ( decoder->builder.no_recurse )
    {
      FT_GlyphSlot    glyph  = (FT_GlyphSlot)decoder->builder.glyph;
      FT_GlyphLoader  loader = glyph->internal->loader;
      FT_SubGlyph     subg;


      /* reallocate subglyph array if necessary */
      error = FT_GlyphLoader_CheckSubGlyphs( loader, 2 );
      if ( error )
        goto Exit;

      subg = loader->current.subglyphs;

      /* subglyph 0 = base character */
      subg->index = bchar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES |
                    FT_SUBGLYPH_FLAG_USE_MY_METRICS;
      subg->arg1  = 0;
      subg->arg2  = 0;
      subg++;

      /* subglyph 1 = accent character */
      subg->index = achar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES;
      subg->arg1  = (FT_Int)( adx - asb );
      subg->arg2  = (FT_Int)ady;

      /* set up remaining glyph fields */
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = FT_GLYPH_FORMAT_COMPOSITE;

      loader->current.num_subglyphs = 2;
      goto Exit;
    }

    /* First load `bchar' in builder */
    /* now load the unscaled outline */

    FT_GlyphLoader_Prepare( decoder->builder.loader );  /* prepare loader */

    error = t1_decoder_parse_glyph( decoder, bchar_index );
    if ( error )
      goto Exit;

    /* save the left bearing and width of the base character */
    /* as they will be erased by the next load.              */

    left_bearing = decoder->builder.left_bearing;
    advance      = decoder->builder.advance;

    decoder->builder.left_bearing.x = 0;
    decoder->builder.left_bearing.y = 0;

    decoder->builder.pos_x = adx - asb;
    decoder->builder.pos_y = ady;

    /* Now load `achar' on top of */
    /* the base outline           */
    error = t1_decoder_parse_glyph( decoder, achar_index );
    if ( error )
      goto Exit;

    /* restore the left side bearing and   */
    /* advance width of the base character */

    decoder->builder.left_bearing = left_bearing;
    decoder->builder.advance      = advance;

    decoder->builder.pos_x = 0;
    decoder->builder.pos_y = 0;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_decoder_parse_charstrings                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 1 charstrings program.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /*    charstring_base :: The base address of the charstring stream.      */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_charstrings( T1_Decoder  decoder,
                                FT_Byte*    charstring_base,
                                FT_UInt     charstring_len )
  {
    FT_Error         error;
    T1_Decoder_Zone  zone;
    FT_Byte*         ip;
    FT_Byte*         limit;
    T1_Builder       builder = &decoder->builder;
    FT_Pos           x, y, orig_x, orig_y;

    T1_Hints_Funcs   hinter;


    /* we don't want to touch the source code -- use macro trick */
#define start_point    t1_builder_start_point
#define check_points   t1_builder_check_points
#define add_point      t1_builder_add_point
#define add_point1     t1_builder_add_point1
#define add_contour    t1_builder_add_contour
#define close_contour  t1_builder_close_contour

    /* First of all, initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->parse_state = T1_Parse_Start;

    hinter = (T1_Hints_Funcs)builder->hints_funcs;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = PSaux_Err_Ok;

    x = orig_x = builder->pos_x;
    y = orig_y = builder->pos_y;

    /* begin hints recording session, if any */
    if ( hinter )
      hinter->open( hinter->hints );

    /* now, execute loop */
    while ( ip < limit )
    {
      FT_Long*     top   = decoder->top;
      T1_Operator  op    = op_none;
      FT_Long      value = 0;


      /*********************************************************************/
      /*                                                                   */
      /* Decode operator or operand                                        */
      /*                                                                   */
      /*                                                                   */

      /* first of all, decompress operator or value */
      switch ( *ip++ )
      {
      case 1:
        op = op_hstem;
        break;

      case 3:
        op = op_vstem;
        break;
      case 4:
        op = op_vmoveto;
        break;
      case 5:
        op = op_rlineto;
        break;
      case 6:
        op = op_hlineto;
        break;
      case 7:
        op = op_vlineto;
        break;
      case 8:
        op = op_rrcurveto;
        break;
      case 9:
        op = op_closepath;
        break;
      case 10:
        op = op_callsubr;
        break;
      case 11:
        op = op_return;
        break;

      case 13:
        op = op_hsbw;
        break;
      case 14:
        op = op_endchar;
        break;

      case 15:          /* undocumented, obsolete operator */
        op = op_none;
        break;

      case 21:
        op = op_rmoveto;
        break;
      case 22:
        op = op_hmoveto;
        break;

      case 30:
        op = op_vhcurveto;
        break;
      case 31:
        op = op_hvcurveto;
        break;

      case 12:
        if ( ip > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+EOF)\n" ));
          goto Syntax_Error;
        }

        switch ( *ip++ )
        {
        case 0:
          op = op_dotsection;
          break;
        case 1:
          op = op_vstem3;
          break;
        case 2:
          op = op_hstem3;
          break;
        case 6:
          op = op_seac;
          break;
        case 7:
          op = op_sbw;
          break;
        case 12:
          op = op_div;
          break;
        case 16:
          op = op_callothersubr;
          break;
        case 17:
          op = op_pop;
          break;
        case 33:
          op = op_setcurrentpoint;
          break;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
        break;

      case 255:    /* four bytes integer */
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unexpected EOF in integer\n" ));
          goto Syntax_Error;
        }

        value = (FT_Int32)( ((FT_Long)ip[0] << 24) |
                            ((FT_Long)ip[1] << 16) |
                            ((FT_Long)ip[2] << 8 ) |
                                      ip[3] );
        ip += 4;
        break;

      default:
        if ( ip[-1] >= 32 )
        {
          if ( ip[-1] < 247 )
            value = (FT_Long)ip[-1] - 139;
          else
          {
            if ( ++ip > limit )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

            if ( ip[-2] < 251 )
              value =  ( ( (FT_Long)ip[-2] - 247 ) << 8 ) + ip[-1] + 108;
            else
              value = -( ( ( (FT_Long)ip[-2] - 251 ) << 8 ) + ip[-1] + 108 );
          }
        }
        else
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid byte (%d)\n", ip[-1] ));
          goto Syntax_Error;
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Push value on stack, or process operator                         */
      /*                                                                   */
      /*                                                                   */
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: stack overflow!\n" ));
          goto Syntax_Error;
        }

        FT_TRACE4(( " %ld", value ));

        *top++       = value;
        decoder->top = top;
      }
      else if ( op == op_callothersubr )  /* callothersubr */
      {
        FT_TRACE4(( " callothersubr" ));

        if ( top - decoder->stack < 2 )
          goto Stack_Underflow;

        top -= 2;
        switch ( (FT_Int)top[1] )
        {
        case 1:                     /* start flex feature */
          if ( top[0] != 0 )
            goto Unexpected_OtherSubr;

          decoder->flex_state        = 1;
          decoder->num_flex_vectors  = 0;
          if ( start_point( builder, x, y ) ||
               check_points( builder, 6 )   )
            goto Fail;
          break;

        case 2:                     /* add flex vectors */
          {
            FT_Int  idx;


            if ( top[0] != 0 )
              goto Unexpected_OtherSubr;

            /* note that we should not add a point for index 0; */
            /* this will move our current position to the flex  */
            /* point without adding any point to the outline    */
            idx = decoder->num_flex_vectors++;
            if ( idx > 0 && idx < 7 )
              add_point( builder,
                         x,
                         y,
                         (FT_Byte)( idx == 3 || idx == 6 ) );
          }
          break;

        case 0:                     /* end flex feature */
          if ( top[0] != 3 )
            goto Unexpected_OtherSubr;

          if ( decoder->flex_state       == 0 ||
               decoder->num_flex_vectors != 7 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "unexpected flex end\n" ));
            goto Syntax_Error;
          }

          /* now consume the remaining `pop pop setcurpoint' */
          if ( ip + 6 > limit ||
               ip[0] != 12 || ip[1] != 17 || /* pop */
               ip[2] != 12 || ip[3] != 17 || /* pop */
               ip[4] != 12 || ip[5] != 33 )  /* setcurpoint */
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "invalid flex charstring\n" ));
            goto Syntax_Error;
          }

          ip += 6;
          decoder->flex_state = 0;
          break;

        case 3:                     /* change hints */
          if ( top[0] != 1 )
            goto Unexpected_OtherSubr;

          /* eat the following `pop' */
          if ( ip + 2 > limit )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "invalid escape (12+%d)\n", ip[-1] ));
            goto Syntax_Error;
          }

          if ( ip[0] != 12 || ip[1] != 17 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: " ));
            FT_ERROR(( "`pop' expected, found (%d %d)\n", ip[0], ip[1] ));
            goto Syntax_Error;
          }
          ip += 2;

          if ( hinter )
            hinter->reset( hinter->hints, builder->current->n_points );

          break;

        case 12:
        case 13:
          /* counter control hints, clear stack */
          top = decoder->stack;
          break;

        case 14:
        case 15:
        case 16:
        case 17:
        case 18:                    /* multiple masters */
          {
            PS_Blend  blend = decoder->blend;
            FT_UInt   num_points, nn, mm;
            FT_Long*  delta;
            FT_Long*  values;


            if ( !blend )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected multiple masters operator!\n" ));
              goto Syntax_Error;
            }

            num_points = (FT_UInt)top[1] - 13 + ( top[1] == 18 );
            if ( top[0] != (FT_Int)( num_points * blend->num_designs ) )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "incorrect number of mm arguments\n" ));
              goto Syntax_Error;
            }

            top -= blend->num_designs * num_points;
            if ( top < decoder->stack )
              goto Stack_Underflow;

            /* we want to compute:                                   */
            /*                                                       */
            /*  a0*w0 + a1*w1 + ... + ak*wk                          */
            /*                                                       */
            /* but we only have the a0, a1-a0, a2-a0, .. ak-a0       */
            /* however, given that w0 + w1 + ... + wk == 1, we can   */
            /* rewrite it easily as:                                 */
            /*                                                       */
            /*  a0 + (a1-a0)*w1 + (a2-a0)*w2 + .. + (ak-a0)*wk       */
            /*                                                       */
            /* where k == num_designs-1                              */
            /*                                                       */
            /* I guess that's why it's written in this `compact'     */
            /* form.                                                 */
            /*                                                       */
            delta  = top + num_points;
            values = top;
            for ( nn = 0; nn < num_points; nn++ )
            {
              FT_Long  tmp = values[0];


              for ( mm = 1; mm < blend->num_designs; mm++ )
                tmp += FT_MulFix( *delta++, blend->weight_vector[mm] );

              *values++ = tmp;
            }
            /* note that `top' will be incremented later by calls to `pop' */
            break;
          }

        default:
        Unexpected_OtherSubr:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid othersubr [%d %d]!\n", top[0], top[1] ));
          goto Syntax_Error;
        }
        decoder->top = top;
      }
      else  /* general operator */
      {
        FT_Int  num_args = t1_args_count[op];


        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        top -= num_args;

        switch ( op )
        {
        case op_endchar:
          FT_TRACE4(( " endchar" ));

          close_contour( builder );

          /* close hints recording session */
          if ( hinter )
          {
            if (hinter->close( hinter->hints, builder->current->n_points ))
              goto Syntax_Error;

            /* apply hints to the loaded glyph outline now */
            hinter->apply( hinter->hints,
                           builder->current,
                           (PSH_Globals) builder->hints_globals,
                           decoder->hint_mode );
          }

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return PSaux_Err_Ok;

        case op_hsbw:
          FT_TRACE4(( " hsbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->advance.x       = top[1];
          builder->advance.y       = 0;

          orig_x = builder->last.x = x = builder->pos_x + top[0];
          orig_y = builder->last.y = y = builder->pos_y;

          FT_UNUSED( orig_y );

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_seac:
          /* return immediately after the processing */
          return t1operator_seac( decoder, top[0], top[1], top[2],
                                           (FT_Int)top[3], (FT_Int)top[4] );

        case op_sbw:
          FT_TRACE4(( " sbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->left_bearing.y += top[1];
          builder->advance.x       = top[2];
          builder->advance.y       = top[3];

          builder->last.x = x = builder->pos_x + top[0];
          builder->last.y = y = builder->pos_y + top[1];

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_closepath:
          FT_TRACE4(( " closepath" ));

          close_contour( builder );
          if ( !( builder->parse_state == T1_Parse_Have_Path   ||
                  builder->parse_state == T1_Parse_Have_Moveto ) )
            goto Syntax_Error;
          builder->parse_state = T1_Parse_Have_Width;
          break;

        case op_hlineto:
          FT_TRACE4(( " hlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          goto Add_Line;

        case op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          x += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_hvcurveto:
          FT_TRACE4(( " hvcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          y += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          y += top[1];

        Add_Line:
          if ( add_point1( builder, x, y ) )
            goto Fail;
          break;

        case op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          x += top[0];
          y += top[1];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_rrcurveto:
          FT_TRACE4(( " rcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          y += top[1];
          add_point( builder, x, y, 0 );

          x += top[2];
          y += top[3];
          add_point( builder, x, y, 0 );

          x += top[4];
          y += top[5];
          add_point( builder, x, y, 1 );
          break;

        case op_vhcurveto:
          FT_TRACE4(( " vhcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          y += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          x += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_vlineto:
          FT_TRACE4(( " vlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          y += top[0];
          goto Add_Line;

        case op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          y += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_div:
          FT_TRACE4(( " div" ));

          if ( top[1] )
          {
            *top = top[0] / top[1];
            ++top;
          }
          else
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: division by 0\n" ));
            goto Syntax_Error;
          }
          break;

        case op_callsubr:
          {
            FT_Int  idx;


            FT_TRACE4(( " callsubr" ));

            idx = (FT_Int)top[0];
            if ( idx < 0 || idx >= (FT_Int)decoder->num_subrs )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invalid subrs index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T1_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;

            /* The Type 1 driver stores subroutines without the seed bytes. */
            /* The CID driver stores subroutines with seed bytes.  This     */
            /* case is taken care of when decoder->subrs_len == 0.          */
            zone->base = decoder->subrs[idx];

            if ( decoder->subrs_len )
              zone->limit = zone->base + decoder->subrs_len[idx];
            else
            {
              /* We are using subroutines from a CID font.  We must adjust */
              /* for the seed bytes.                                       */
              zone->base  += ( decoder->lenIV >= 0 ? decoder->lenIV : 0 );
              zone->limit  = decoder->subrs[idx + 1];
            }

            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
            break;
          }

        case op_pop:
          FT_TRACE4(( " pop" ));

          /* theoretically, the arguments are already on the stack */
          top++;
          break;

        case op_return:
          FT_TRACE4(( " return" ));

          if ( zone <= decoder->zones )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          zone--;
          ip            = zone->cursor;
          limit         = zone->limit;
          decoder->zone = zone;
          break;

        case op_dotsection:
          FT_TRACE4(( " dotsection" ));

          break;

        case op_hstem:
          FT_TRACE4(( " hstem" ));

          /* record horizontal hint */
          if ( hinter )
          {
            /* top[0] += builder->left_bearing.y; */
            hinter->stem( hinter->hints, 1, top );
          }

          break;

        case op_hstem3:
          FT_TRACE4(( " hstem3" ));

          /* record horizontal counter-controlled hints */
          if ( hinter )
            hinter->stem3( hinter->hints, 1, top );

          break;

        case op_vstem:
          FT_TRACE4(( " vstem" ));

          /* record vertical  hint */
          if ( hinter )
          {
            top[0] += orig_x;
            hinter->stem( hinter->hints, 0, top );
          }

          break;

        case op_vstem3:
          FT_TRACE4(( " vstem3" ));

          /* record vertical counter-controlled hints */
          if ( hinter )
          {
            FT_Pos  dx = orig_x;


            top[0] += dx;
            top[2] += dx;
            top[4] += dx;
            hinter->stem3( hinter->hints, 0, top );
          }
          break;

        case op_setcurrentpoint:
          FT_TRACE4(( " setcurrentpoint" ));

          FT_ERROR(( "t1_decoder_parse_charstrings: " ));
          FT_ERROR(( "unexpected `setcurrentpoint'\n" ));
          goto Syntax_Error;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

        decoder->top = top;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

  Fail:
    return error;

  Syntax_Error:
    return PSaux_Err_Syntax_Error;

  Stack_Underflow:
    return PSaux_Err_Stack_Underflow;
  }


  /* parse a single Type 1 glyph */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_glyph( T1_Decoder  decoder,
                          FT_UInt     glyph )
  {
    return decoder->parse_callback( decoder, glyph );
  }


  /* initialize T1 decoder */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_init( T1_Decoder           decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   PS_Blend             blend,
                   FT_Bool              hinting,
                   FT_Render_Mode       hint_mode,
                   T1_Decoder_Callback  parse_callback )
  {
    FT_MEM_ZERO( decoder, sizeof ( *decoder ) );

    /* retrieve PSNames interface from list of current modules */
    {
      FT_Service_PsCMaps  psnames = 0;


      FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
      if ( !psnames )
      {
        FT_ERROR(( "t1_decoder_init: " ));
        FT_ERROR(( "the `psnames' module is not available\n" ));
        return PSaux_Err_Unimplemented_Feature;
      }

      decoder->psnames = psnames;
    }

    t1_builder_init( &decoder->builder, face, size, slot, hinting );

    decoder->num_glyphs     = (FT_UInt)face->num_glyphs;
    decoder->glyph_names    = glyph_names;
    decoder->hint_mode      = hint_mode;
    decoder->blend          = blend;
    decoder->parse_callback = parse_callback;

    decoder->funcs          = t1_decoder_funcs;

    return 0;
  }


  /* finalize T1 decoder */
  FT_LOCAL_DEF( void )
  t1_decoder_done( T1_Decoder  decoder )
  {
    t1_builder_done( &decoder->builder );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  t1cmap.c                                                               */
/*                                                                         */
/*    Type 1 character map support (body).                                 */
/*                                                                         */
/*  Copyright 2002, 2003, 2006 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "t1cmap.h"

#include FT_INTERNAL_DEBUG_H

#include "psauxerr.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****          TYPE1 STANDARD (AND EXPERT) ENCODING CMAPS           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  t1_cmap_std_init( T1_CMapStd  cmap,
                    FT_Int      is_expert )
  {
    T1_Face             face    = (T1_Face)FT_CMAP_FACE( cmap );
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)face->psnames;


    cmap->num_glyphs    = face->type1.num_glyphs;
    cmap->glyph_names   = (const char* const*)face->type1.glyph_names;
    cmap->sid_to_string = psnames->adobe_std_strings;
    cmap->code_to_sid   = is_expert ? psnames->adobe_expert_encoding
                                    : psnames->adobe_std_encoding;

    FT_ASSERT( cmap->code_to_sid != NULL );
  }


  FT_CALLBACK_DEF( void )
  t1_cmap_std_done( T1_CMapStd  cmap )
  {
    cmap->num_glyphs    = 0;
    cmap->glyph_names   = NULL;
    cmap->sid_to_string = NULL;
    cmap->code_to_sid   = NULL;
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_std_char_index( T1_CMapStd  cmap,
                          FT_UInt32   char_code )
  {
    FT_UInt  result = 0;


    if ( char_code < 256 )
    {
      FT_UInt      code, n;
      const char*  glyph_name;


      /* convert character code to Adobe SID string */
      code       = cmap->code_to_sid[char_code];
      glyph_name = cmap->sid_to_string( code );

      /* look for the corresponding glyph name */
      for ( n = 0; n < cmap->num_glyphs; n++ )
      {
        const char* gname = cmap->glyph_names[n];


        if ( gname && gname[0] == glyph_name[0]  &&
             ft_strcmp( gname, glyph_name ) == 0 )
        {
          result = n;
          break;
        }
      }
    }

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_std_char_next( T1_CMapStd   cmap,
                         FT_UInt32   *pchar_code )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code + 1;


    while ( char_code < 256 )
    {
      result = t1_cmap_std_char_index( cmap, char_code );
      if ( result != 0 )
        goto Exit;

      char_code++;
    }
    char_code = 0;

  Exit:
    *pchar_code = char_code;
    return result;
  }


  FT_CALLBACK_DEF( FT_Error )
  t1_cmap_standard_init( T1_CMapStd  cmap )
  {
    t1_cmap_std_init( cmap, 0 );
    return 0;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  t1_cmap_standard_class_rec =
  {
    sizeof ( T1_CMapStdRec ),

    (FT_CMap_InitFunc)     t1_cmap_standard_init,
    (FT_CMap_DoneFunc)     t1_cmap_std_done,
    (FT_CMap_CharIndexFunc)t1_cmap_std_char_index,
    (FT_CMap_CharNextFunc) t1_cmap_std_char_next
  };


  FT_CALLBACK_DEF( FT_Error )
  t1_cmap_expert_init( T1_CMapStd  cmap )
  {
    t1_cmap_std_init( cmap, 1 );
    return 0;
  }

  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  t1_cmap_expert_class_rec =
  {
    sizeof ( T1_CMapStdRec ),

    (FT_CMap_InitFunc)     t1_cmap_expert_init,
    (FT_CMap_DoneFunc)     t1_cmap_std_done,
    (FT_CMap_CharIndexFunc)t1_cmap_std_char_index,
    (FT_CMap_CharNextFunc) t1_cmap_std_char_next
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    TYPE1 CUSTOM ENCODING CMAP                 *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_Error )
  t1_cmap_custom_init( T1_CMapCustom  cmap )
  {
    T1_Face      face     = (T1_Face)FT_CMAP_FACE( cmap );
    T1_Encoding  encoding = &face->type1.encoding;


    cmap->first   = encoding->code_first;
    cmap->count   = (FT_UInt)( encoding->code_last - cmap->first + 1 );
    cmap->indices = encoding->char_index;

    FT_ASSERT( cmap->indices != NULL );
    FT_ASSERT( encoding->code_first <= encoding->code_last );

    return 0;
  }


  FT_CALLBACK_DEF( void )
  t1_cmap_custom_done( T1_CMapCustom  cmap )
  {
    cmap->indices = NULL;
    cmap->first   = 0;
    cmap->count   = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_custom_char_index( T1_CMapCustom  cmap,
                             FT_UInt32      char_code )
  {
    FT_UInt    result = 0;


    if ( ( char_code >= cmap->first )                  &&
         ( char_code < ( cmap->first + cmap->count ) ) )
      result = cmap->indices[char_code];

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_custom_char_next( T1_CMapCustom  cmap,
                            FT_UInt32     *pchar_code )
  {
    FT_UInt    result = 0;
    FT_UInt32  char_code = *pchar_code;


    ++char_code;

    if ( char_code < cmap->first )
      char_code = cmap->first;

    for ( ; char_code < ( cmap->first + cmap->count ); char_code++ )
    {
      result = cmap->indices[char_code];
      if ( result != 0 )
        goto Exit;
    }

    char_code = 0;

  Exit:
    *pchar_code = char_code;
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  t1_cmap_custom_class_rec =
  {
    sizeof ( T1_CMapCustomRec ),

    (FT_CMap_InitFunc)     t1_cmap_custom_init,
    (FT_CMap_DoneFunc)     t1_cmap_custom_done,
    (FT_CMap_CharIndexFunc)t1_cmap_custom_char_index,
    (FT_CMap_CharNextFunc) t1_cmap_custom_char_next
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****            TYPE1 SYNTHETIC UNICODE ENCODING CMAP              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( const char * )
  t1_get_glyph_name( T1_Face  face,
                     FT_UInt  idx )
  {
    return face->type1.glyph_names[idx];
  }


  FT_CALLBACK_DEF( FT_Error )
  t1_cmap_unicode_init( PS_Unicodes  unicodes )
  {
    T1_Face             face    = (T1_Face)FT_CMAP_FACE( unicodes );
    FT_Memory           memory  = FT_FACE_MEMORY( face );
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)face->psnames;


    return psnames->unicodes_init( memory,
                                   unicodes,
                                   face->type1.num_glyphs,
                                   (PS_Glyph_NameFunc)&t1_get_glyph_name,
                                   (FT_Pointer)face );
  }


  FT_CALLBACK_DEF( void )
  t1_cmap_unicode_done( PS_Unicodes  unicodes )
  {
    FT_Face    face   = FT_CMAP_FACE( unicodes );
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( unicodes->maps );
    unicodes->num_maps = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_unicode_char_index( PS_Unicodes  unicodes,
                              FT_UInt32    char_code )
  {
    T1_Face             face    = (T1_Face)FT_CMAP_FACE( unicodes );
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)face->psnames;


    return psnames->unicodes_char_index( unicodes, char_code );
  }


  FT_CALLBACK_DEF( FT_UInt )
  t1_cmap_unicode_char_next( PS_Unicodes  unicodes,
                             FT_UInt32   *pchar_code )
  {
    T1_Face             face    = (T1_Face)FT_CMAP_FACE( unicodes );
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)face->psnames;


    return psnames->unicodes_char_next( unicodes, pchar_code );
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  t1_cmap_unicode_class_rec =
  {
    sizeof ( PS_UnicodesRec ),

    (FT_CMap_InitFunc)     t1_cmap_unicode_init,
    (FT_CMap_DoneFunc)     t1_cmap_unicode_done,
    (FT_CMap_CharIndexFunc)t1_cmap_unicode_char_index,
    (FT_CMap_CharNextFunc) t1_cmap_unicode_char_next
  };


/* END */


#ifndef T1_CONFIG_OPTION_NO_AFM
/***************************************************************************/
/*                                                                         */
/*  afmparse.c                                                             */
/*                                                                         */
/*    AFM parser (body).                                                   */
/*                                                                         */
/*  Copyright 2006 by                                                      */
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
#include FT_FREETYPE_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "afmparse.h"
#include "psconv.h"

#include "psauxerr.h"


/***************************************************************************/
/*                                                                         */
/*    AFM_Stream                                                           */
/*                                                                         */
/* The use of AFM_Stream is largely inspired by parseAFM.[ch] from t1lib.  */
/*                                                                         */
/*                                                                         */

  enum
  {
    AFM_STREAM_STATUS_NORMAL,
    AFM_STREAM_STATUS_EOC,
    AFM_STREAM_STATUS_EOL,
    AFM_STREAM_STATUS_EOF
  };


  typedef struct  AFM_StreamRec_
  {
    FT_Byte*  cursor;
    FT_Byte*  base;
    FT_Byte*  limit;

    FT_Int    status;

  } AFM_StreamRec;


#ifndef EOF
#define EOF -1
#endif


  /* this works because empty lines are ignored */
#define AFM_IS_NEWLINE( ch )  ( (ch) == '\r' || (ch) == '\n' )

#define AFM_IS_EOF( ch )      ( (ch) == EOF  || (ch) == '\x1a' )
#define AFM_IS_SPACE( ch )    ( (ch) == ' '  || (ch) == '\t' )

  /* column separator; there is no `column' in the spec actually */
#define AFM_IS_SEP( ch )      ( (ch) == ';' )

#define AFM_GETC()                                                       \
          ( ( (stream)->cursor < (stream)->limit ) ? *(stream)->cursor++ \
                                                   : EOF )

#define AFM_STREAM_KEY_BEGIN( stream )    \
          (char*)( (stream)->cursor - 1 )

#define AFM_STREAM_KEY_LEN( stream, key )       \
          ( (char*)(stream)->cursor - key - 1 )

#define AFM_STATUS_EOC( stream ) \
          ( (stream)->status >= AFM_STREAM_STATUS_EOC )

#define AFM_STATUS_EOL( stream ) \
          ( (stream)->status >= AFM_STREAM_STATUS_EOL )

#define AFM_STATUS_EOF( stream ) \
          ( (stream)->status >= AFM_STREAM_STATUS_EOF )


  static int
  afm_stream_skip_spaces( AFM_Stream  stream )
  {
    int  ch = 0;  /* make stupid compiler happy */


    if ( AFM_STATUS_EOC( stream ) )
      return ';';

    while ( 1 )
    {
      ch = AFM_GETC();
      if ( !AFM_IS_SPACE( ch ) )
        break;
    }

    if ( AFM_IS_NEWLINE( ch ) )
      stream->status = AFM_STREAM_STATUS_EOL;
    else if ( AFM_IS_SEP( ch ) )
      stream->status = AFM_STREAM_STATUS_EOC;
    else if ( AFM_IS_EOF( ch ) )
      stream->status = AFM_STREAM_STATUS_EOF;

    return ch;
  }


  /* read a key or value in current column */
  static char*
  afm_stream_read_one( AFM_Stream  stream )
  {
    char*  str;
    int    ch;


    afm_stream_skip_spaces( stream );
    if ( AFM_STATUS_EOC( stream ) )
      return NULL;

    str = AFM_STREAM_KEY_BEGIN( stream );

    while ( 1 )
    {
      ch = AFM_GETC();
      if ( AFM_IS_SPACE( ch ) )
        break;
      else if ( AFM_IS_NEWLINE( ch ) )
      {
        stream->status = AFM_STREAM_STATUS_EOL;
        break;
      }
      else if ( AFM_IS_SEP( ch ) )
      {
        stream->status = AFM_STREAM_STATUS_EOC;
        break;
      }
      else if ( AFM_IS_EOF( ch ) )
      {
        stream->status = AFM_STREAM_STATUS_EOF;
        break;
      }
    }

    return str;
  }


  /* read a string (i.e., read to EOL) */
  static char*
  afm_stream_read_string( AFM_Stream  stream )
  {
    char*  str;
    int    ch;


    afm_stream_skip_spaces( stream );
    if ( AFM_STATUS_EOL( stream ) )
      return NULL;

    str = AFM_STREAM_KEY_BEGIN( stream );

    /* scan to eol */
    while ( 1 )
    {
      ch = AFM_GETC();
      if ( AFM_IS_NEWLINE( ch ) )
      {
        stream->status = AFM_STREAM_STATUS_EOL;
        break;
      }
      else if ( AFM_IS_EOF( ch ) )
      {
        stream->status = AFM_STREAM_STATUS_EOF;
        break;
      }
    }

    return str;
  }


  /*************************************************************************/
  /*                                                                       */
  /*    AFM_Parser                                                         */
  /*                                                                       */
  /*                                                                       */

  /* all keys defined in Ch. 7-10 of 5004.AFM_Spec.pdf */
  typedef enum  AFM_Token_
  {
    AFM_TOKEN_ASCENDER,
    AFM_TOKEN_AXISLABEL,
    AFM_TOKEN_AXISTYPE,
    AFM_TOKEN_B,
    AFM_TOKEN_BLENDAXISTYPES,
    AFM_TOKEN_BLENDDESIGNMAP,
    AFM_TOKEN_BLENDDESIGNPOSITIONS,
    AFM_TOKEN_C,
    AFM_TOKEN_CC,
    AFM_TOKEN_CH,
    AFM_TOKEN_CAPHEIGHT,
    AFM_TOKEN_CHARWIDTH,
    AFM_TOKEN_CHARACTERSET,
    AFM_TOKEN_CHARACTERS,
    AFM_TOKEN_DESCENDER,
    AFM_TOKEN_ENCODINGSCHEME,
    AFM_TOKEN_ENDAXIS,
    AFM_TOKEN_ENDCHARMETRICS,
    AFM_TOKEN_ENDCOMPOSITES,
    AFM_TOKEN_ENDDIRECTION,
    AFM_TOKEN_ENDFONTMETRICS,
    AFM_TOKEN_ENDKERNDATA,
    AFM_TOKEN_ENDKERNPAIRS,
    AFM_TOKEN_ENDTRACKKERN,
    AFM_TOKEN_ESCCHAR,
    AFM_TOKEN_FAMILYNAME,
    AFM_TOKEN_FONTBBOX,
    AFM_TOKEN_FONTNAME,
    AFM_TOKEN_FULLNAME,
    AFM_TOKEN_ISBASEFONT,
    AFM_TOKEN_ISCIDFONT,
    AFM_TOKEN_ISFIXEDPITCH,
    AFM_TOKEN_ISFIXEDV,
    AFM_TOKEN_ITALICANGLE,
    AFM_TOKEN_KP,
    AFM_TOKEN_KPH,
    AFM_TOKEN_KPX,
    AFM_TOKEN_KPY,
    AFM_TOKEN_L,
    AFM_TOKEN_MAPPINGSCHEME,
    AFM_TOKEN_METRICSSETS,
    AFM_TOKEN_N,
    AFM_TOKEN_NOTICE,
    AFM_TOKEN_PCC,
    AFM_TOKEN_STARTAXIS,
    AFM_TOKEN_STARTCHARMETRICS,
    AFM_TOKEN_STARTCOMPOSITES,
    AFM_TOKEN_STARTDIRECTION,
    AFM_TOKEN_STARTFONTMETRICS,
    AFM_TOKEN_STARTKERNDATA,
    AFM_TOKEN_STARTKERNPAIRS,
    AFM_TOKEN_STARTKERNPAIRS0,
    AFM_TOKEN_STARTKERNPAIRS1,
    AFM_TOKEN_STARTTRACKKERN,
    AFM_TOKEN_STDHW,
    AFM_TOKEN_STDVW,
    AFM_TOKEN_TRACKKERN,
    AFM_TOKEN_UNDERLINEPOSITION,
    AFM_TOKEN_UNDERLINETHICKNESS,
    AFM_TOKEN_VV,
    AFM_TOKEN_VVECTOR,
    AFM_TOKEN_VERSION,
    AFM_TOKEN_W,
    AFM_TOKEN_W0,
    AFM_TOKEN_W0X,
    AFM_TOKEN_W0Y,
    AFM_TOKEN_W1,
    AFM_TOKEN_W1X,
    AFM_TOKEN_W1Y,
    AFM_TOKEN_WX,
    AFM_TOKEN_WY,
    AFM_TOKEN_WEIGHT,
    AFM_TOKEN_WEIGHTVECTOR,
    AFM_TOKEN_XHEIGHT,
    N_AFM_TOKENS,
    AFM_TOKEN_UNKNOWN

  } AFM_Token;


  static const char*  const afm_key_table[N_AFM_TOKENS] =
  {
    "Ascender",
    "AxisLabel",
    "AxisType",
    "B",
    "BlendAxisTypes",
    "BlendDesignMap",
    "BlendDesignPositions",
    "C",
    "CC",
    "CH",
    "CapHeight",
    "CharWidth",
    "CharacterSet",
    "Characters",
    "Descender",
    "EncodingScheme",
    "EndAxis",
    "EndCharMetrics",
    "EndComposites",
    "EndDirection",
    "EndFontMetrics",
    "EndKernData",
    "EndKernPairs",
    "EndTrackKern",
    "EscChar",
    "FamilyName",
    "FontBBox",
    "FontName",
    "FullName",
    "IsBaseFont",
    "IsCIDFont",
    "IsFixedPitch",
    "IsFixedV",
    "ItalicAngle",
    "KP",
    "KPH",
    "KPX",
    "KPY",
    "L",
    "MappingScheme",
    "MetricsSets",
    "N",
    "Notice",
    "PCC",
    "StartAxis",
    "StartCharMetrics",
    "StartComposites",
    "StartDirection",
    "StartFontMetrics",
    "StartKernData",
    "StartKernPairs",
    "StartKernPairs0",
    "StartKernPairs1",
    "StartTrackKern",
    "StdHW",
    "StdVW",
    "TrackKern",
    "UnderlinePosition",
    "UnderlineThickness",
    "VV",
    "VVector",
    "Version",
    "W",
    "W0",
    "W0X",
    "W0Y",
    "W1",
    "W1X",
    "W1Y",
    "WX",
    "WY",
    "Weight",
    "WeightVector",
    "XHeight"
  };


  /*
   * `afm_parser_read_vals' and `afm_parser_next_key' provide
   * high-level operations to an AFM_Stream.  The rest of the
   * parser functions should use them without accessing the
   * AFM_Stream directly.
   */

  FT_LOCAL_DEF( FT_Int )
  afm_parser_read_vals( AFM_Parser  parser,
                        AFM_Value   vals,
                        FT_Int      n )
  {
    AFM_Stream  stream = parser->stream;
    char*       str;
    FT_Int      i;


    if ( n > AFM_MAX_ARGUMENTS )
      return 0;

    for ( i = 0; i < n; i++ )
    {
      FT_UInt    len;
      AFM_Value  val = vals + i;


      if ( val->type == AFM_VALUE_TYPE_STRING )
        str = afm_stream_read_string( stream );
      else
        str = afm_stream_read_one( stream );

      if ( !str )
        break;

      len = AFM_STREAM_KEY_LEN( stream, str );

      switch ( val->type )
      {
      case AFM_VALUE_TYPE_STRING:
      case AFM_VALUE_TYPE_NAME:
        {
          FT_Memory  memory = parser->memory;
          FT_Error   error;


          if ( !FT_QALLOC( val->u.s, len + 1 ) )
          {
            ft_memcpy( val->u.s, str, len );
            val->u.s[len] = '\0';
          }
        }
        break;

      case AFM_VALUE_TYPE_FIXED:
        val->u.f = PS_Conv_ToFixed( (FT_Byte**)(void*)&str,
                                    (FT_Byte*)str + len, 0 );
        break;

      case AFM_VALUE_TYPE_INTEGER:
        val->u.i = PS_Conv_ToInt( (FT_Byte**)(void*)&str,
                                  (FT_Byte*)str + len );
        break;

      case AFM_VALUE_TYPE_BOOL:
        val->u.b = FT_BOOL( len == 4                      &&
                            !ft_strncmp( str, "true", 4 ) );
        break;

      case AFM_VALUE_TYPE_INDEX:
        if ( parser->get_index )
          val->u.i = parser->get_index( str, len, parser->user_data );
        else
          val->u.i = 0;
        break;
      }
    }

    return i;
  }


  FT_LOCAL_DEF( char* )
  afm_parser_next_key( AFM_Parser  parser,
                       FT_Bool     line,
                       FT_UInt*    len )
  {
    AFM_Stream  stream = parser->stream;
    char*       key    = 0;  /* make stupid compiler happy */


    if ( line )
    {
      while ( 1 )
      {
        /* skip current line */
        if ( !AFM_STATUS_EOL( stream ) )
          afm_stream_read_string( stream );

        stream->status = AFM_STREAM_STATUS_NORMAL;
        key = afm_stream_read_one( stream );

        /* skip empty line */
        if ( !key                      &&
             !AFM_STATUS_EOF( stream ) &&
             AFM_STATUS_EOL( stream )  )
          continue;

        break;
      }
    }
    else
    {
      while ( 1 )
      {
        /* skip current column */
        while ( !AFM_STATUS_EOC( stream ) )
          afm_stream_read_one( stream );

        stream->status = AFM_STREAM_STATUS_NORMAL;
        key = afm_stream_read_one( stream );

        /* skip empty column */
        if ( !key                      &&
             !AFM_STATUS_EOF( stream ) &&
             AFM_STATUS_EOC( stream )  )
          continue;

        break;
      }
    }

    if ( len )
      *len = ( key ) ? AFM_STREAM_KEY_LEN( stream, key )
                     : 0;

    return key;
  }


  static AFM_Token
  afm_tokenize( const char*  key,
                FT_UInt      len )
  {
    int  n;


    for ( n = 0; n < N_AFM_TOKENS; n++ )
    {
      if ( *( afm_key_table[n] ) == *key )
      {
        for ( ; n < N_AFM_TOKENS; n++ )
        {
          if ( *( afm_key_table[n] ) != *key )
            return AFM_TOKEN_UNKNOWN;

          if ( ft_strncmp( afm_key_table[n], key, len ) == 0 )
            return (AFM_Token) n;
        }
      }
    }

    return AFM_TOKEN_UNKNOWN;
  }


  FT_LOCAL_DEF( FT_Error )
  afm_parser_init( AFM_Parser  parser,
                   FT_Memory   memory,
                   FT_Byte*    base,
                   FT_Byte*    limit )
  {
    AFM_Stream  stream;
    FT_Error    error;


    if ( FT_NEW( stream ) )
      return error;

    stream->cursor = stream->base = base;
    stream->limit  = limit;

    /* don't skip the first line during the first call */
    stream->status = AFM_STREAM_STATUS_EOL;

    parser->memory    = memory;
    parser->stream    = stream;
    parser->FontInfo  = NULL;
    parser->get_index = NULL;

    return PSaux_Err_Ok;
  }


  FT_LOCAL( void )
  afm_parser_done( AFM_Parser  parser )
  {
    FT_Memory  memory = parser->memory;


    FT_FREE( parser->stream );
  }


  FT_LOCAL_DEF( FT_Error )
  afm_parser_read_int( AFM_Parser  parser,
                       FT_Int*     aint )
  {
    AFM_ValueRec  val;


    val.type = AFM_VALUE_TYPE_INTEGER;

    if ( afm_parser_read_vals( parser, &val, 1 ) == 1 )
    {
      *aint = val.u.i;

      return PSaux_Err_Ok;
    }
    else
      return PSaux_Err_Syntax_Error;
  }


  static FT_Error
  afm_parse_track_kern( AFM_Parser  parser )
  {
    AFM_FontInfo   fi = parser->FontInfo;
    AFM_TrackKern  tk;
    char*          key;
    FT_UInt        len;
    int            n = -1;


    if ( afm_parser_read_int( parser, &fi->NumTrackKern ) )
        goto Fail;

    if ( fi->NumTrackKern )
    {
      FT_Memory  memory = parser->memory;
      FT_Error   error;


      if ( FT_QNEW_ARRAY( fi->TrackKerns, fi->NumTrackKern ) )
        return error;
    }

    while ( ( key = afm_parser_next_key( parser, 1, &len ) ) != 0 )
    {
      AFM_ValueRec  shared_vals[5];


      switch ( afm_tokenize( key, len ) )
      {
      case AFM_TOKEN_TRACKKERN:
        n++;

        if ( n >= fi->NumTrackKern )
          goto Fail;

        tk = fi->TrackKerns + n;

        shared_vals[0].type = AFM_VALUE_TYPE_INTEGER;
        shared_vals[1].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[2].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[3].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[4].type = AFM_VALUE_TYPE_FIXED;
        if ( afm_parser_read_vals( parser, shared_vals, 5 ) != 5 )
          goto Fail;

        tk->degree     = shared_vals[0].u.i;
        tk->min_ptsize = shared_vals[1].u.f;
        tk->min_kern   = shared_vals[2].u.f;
        tk->max_ptsize = shared_vals[3].u.f;
        tk->max_kern   = shared_vals[4].u.f;

        /* is this correct? */
        if ( tk->degree < 0 && tk->min_kern > 0 )
          tk->min_kern = -tk->min_kern;
        break;

      case AFM_TOKEN_ENDTRACKKERN:
      case AFM_TOKEN_ENDKERNDATA:
      case AFM_TOKEN_ENDFONTMETRICS:
        fi->NumTrackKern = n + 1;
        return PSaux_Err_Ok;
        /*break;*/

      case AFM_TOKEN_UNKNOWN:
        break;

      default:
        goto Fail;
        /*break;*/
      }
    }

  Fail:
    return PSaux_Err_Syntax_Error;
  }


#undef  KERN_INDEX
#define KERN_INDEX( g1, g2 )  ( ( (FT_ULong)g1 << 16 ) | g2 )


  /* compare two kerning pairs */
  FT_CALLBACK_DEF( int )
  afm_compare_kern_pairs( const void*  a,
                          const void*  b )
  {
    AFM_KernPair  kp1 = (AFM_KernPair)a;
    AFM_KernPair  kp2 = (AFM_KernPair)b;

    FT_ULong  index1 = KERN_INDEX( kp1->index1, kp1->index2 );
    FT_ULong  index2 = KERN_INDEX( kp2->index1, kp2->index2 );


    return (int)( index1 - index2 );
  }


  static FT_Error
  afm_parse_kern_pairs( AFM_Parser  parser )
  {
    AFM_FontInfo  fi = parser->FontInfo;
    AFM_KernPair  kp;
    char*         key;
    FT_UInt       len;
    int           n = -1;


    if ( afm_parser_read_int( parser, &fi->NumKernPair ) )
      goto Fail;

    if ( fi->NumKernPair )
    {
      FT_Memory  memory = parser->memory;
      FT_Error   error;


      if ( FT_QNEW_ARRAY( fi->KernPairs, fi->NumKernPair ) )
        return error;
    }

    while ( ( key = afm_parser_next_key( parser, 1, &len ) ) != 0 )
    {
      AFM_Token  token = afm_tokenize( key, len );


      switch ( token )
      {
      case AFM_TOKEN_KP:
      case AFM_TOKEN_KPX:
      case AFM_TOKEN_KPY:
        {
          FT_Int        r;
          AFM_ValueRec  shared_vals[4];


          n++;

          if ( n >= fi->NumKernPair )
            goto Fail;

          kp = fi->KernPairs + n;

          shared_vals[0].type = AFM_VALUE_TYPE_INDEX;
          shared_vals[1].type = AFM_VALUE_TYPE_INDEX;
          shared_vals[2].type = AFM_VALUE_TYPE_INTEGER;
          shared_vals[3].type = AFM_VALUE_TYPE_INTEGER;
          r = afm_parser_read_vals( parser, shared_vals, 4 );
          if ( r < 3 )
            goto Fail;

          kp->index1 = shared_vals[0].u.i;
          kp->index2 = shared_vals[1].u.i;
          if ( token == AFM_TOKEN_KPY )
          {
            kp->x = 0;
            kp->y = shared_vals[2].u.i;
          }
          else
          {
            kp->x = shared_vals[2].u.i;
            kp->y = ( token == AFM_TOKEN_KP && r == 4 )
                      ? shared_vals[3].u.i : 0;
          }
        }
        break;

      case AFM_TOKEN_ENDKERNPAIRS:
      case AFM_TOKEN_ENDKERNDATA:
      case AFM_TOKEN_ENDFONTMETRICS:
        fi->NumKernPair = n + 1;
        ft_qsort( fi->KernPairs, fi->NumKernPair,
                  sizeof( AFM_KernPairRec ),
                  afm_compare_kern_pairs );
        return PSaux_Err_Ok;

      case AFM_TOKEN_UNKNOWN:
        break;

      default:
        goto Fail;
        /*break;*/
      }
    }

  Fail:
    return PSaux_Err_Syntax_Error;
  }


  static FT_Error
  afm_parse_kern_data( AFM_Parser  parser )
  {
    FT_Error  error;
    char*     key;
    FT_UInt   len;


    while ( ( key = afm_parser_next_key( parser, 1, &len ) ) != 0 )
    {
      switch ( afm_tokenize( key, len ) )
      {
      case AFM_TOKEN_STARTTRACKKERN:
        error = afm_parse_track_kern( parser );
        if ( error )
          return error;
        break;

      case AFM_TOKEN_STARTKERNPAIRS:
      case AFM_TOKEN_STARTKERNPAIRS0:
        error = afm_parse_kern_pairs( parser );
        if ( error )
          return error;
        break;

      case AFM_TOKEN_ENDKERNDATA:
      case AFM_TOKEN_ENDFONTMETRICS:
        return PSaux_Err_Ok;

      case AFM_TOKEN_UNKNOWN:
        break;

      default:
        goto Fail;
        /*break;*/
      }
    }

  Fail:
    return PSaux_Err_Syntax_Error;
  }


  static FT_Error
  afm_parser_skip_section( AFM_Parser  parser,
                           FT_UInt     n,
                           AFM_Token   end_section )
  {
    char*    key;
    FT_UInt  len;


    while ( n-- > 0 )
    {
      key = afm_parser_next_key( parser, 1, NULL );
      if ( !key )
        goto Fail;
    }

    while ( ( key = afm_parser_next_key( parser, 1, &len ) ) != 0 )
    {
      AFM_Token  token = afm_tokenize( key, len );


      if ( token == end_section || token == AFM_TOKEN_ENDFONTMETRICS )
        return PSaux_Err_Ok;
    }

  Fail:
    return PSaux_Err_Syntax_Error;
  }


  FT_LOCAL_DEF( FT_Error )
  afm_parser_parse( AFM_Parser  parser )
  {
    FT_Memory     memory = parser->memory;
    AFM_FontInfo  fi     = parser->FontInfo;
    FT_Error      error  = PSaux_Err_Syntax_Error;
    char*         key;
    FT_UInt       len;
    FT_Int        metrics_sets = 0;


    if ( !fi )
      return PSaux_Err_Invalid_Argument;

    key = afm_parser_next_key( parser, 1, &len );
    if ( !key || len != 16                              ||
         ft_strncmp( key, "StartFontMetrics", 16 ) != 0 )
      return PSaux_Err_Unknown_File_Format;

    while ( ( key = afm_parser_next_key( parser, 1, &len ) ) != 0 )
    {
      AFM_ValueRec  shared_vals[4];


      switch ( afm_tokenize( key, len ) )
      {
      case AFM_TOKEN_METRICSSETS:
        if ( afm_parser_read_int( parser, &metrics_sets ) )
          goto Fail;

        if ( metrics_sets != 0 && metrics_sets != 2 )
        {
          error = PSaux_Err_Unimplemented_Feature;

          goto Fail;
        }
        break;

      case AFM_TOKEN_ISCIDFONT:
        shared_vals[0].type = AFM_VALUE_TYPE_BOOL;
        if ( afm_parser_read_vals( parser, shared_vals, 1 ) != 1 )
          goto Fail;

        fi->IsCIDFont = shared_vals[0].u.b;
        break;

      case AFM_TOKEN_FONTBBOX:
        shared_vals[0].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[1].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[2].type = AFM_VALUE_TYPE_FIXED;
        shared_vals[3].type = AFM_VALUE_TYPE_FIXED;
        if ( afm_parser_read_vals( parser, shared_vals, 4 ) != 4 )
          goto Fail;

        fi->FontBBox.xMin = shared_vals[0].u.f;
        fi->FontBBox.yMin = shared_vals[1].u.f;
        fi->FontBBox.xMax = shared_vals[2].u.f;
        fi->FontBBox.yMax = shared_vals[3].u.f;
        break;

      case AFM_TOKEN_ASCENDER:
        shared_vals[0].type = AFM_VALUE_TYPE_FIXED;
        if ( afm_parser_read_vals( parser, shared_vals, 1 ) != 1 )
          goto Fail;

        fi->Ascender = shared_vals[0].u.f;
        break;

      case AFM_TOKEN_DESCENDER:
        shared_vals[0].type = AFM_VALUE_TYPE_FIXED;
        if ( afm_parser_read_vals( parser, shared_vals, 1 ) != 1 )
          goto Fail;

        fi->Descender = shared_vals[0].u.f;
        break;

      case AFM_TOKEN_STARTCHARMETRICS:
        {
          FT_Int  n;


          if ( afm_parser_read_int( parser, &n ) )
            goto Fail;

          error = afm_parser_skip_section( parser, n,
                                           AFM_TOKEN_ENDCHARMETRICS );
          if ( error )
            return error;
        }
        break;

      case AFM_TOKEN_STARTKERNDATA:
        error = afm_parse_kern_data( parser );
        if ( error )
          goto Fail;
        /* fall through since we only support kern data */

      case AFM_TOKEN_ENDFONTMETRICS:
        return PSaux_Err_Ok;
        /*break;*/

      default:
        break;
      }
    }

  Fail:
    FT_FREE( fi->TrackKerns );
    fi->NumTrackKern = 0;

    FT_FREE( fi->KernPairs );
    fi->NumKernPair = 0;

    fi->IsCIDFont = 0;

    return error;
  }


/* END */

#endif

/***************************************************************************/
/*                                                                         */
/*  psconv.c                                                               */
/*                                                                         */
/*    Some convenience conversions (body).                                 */
/*                                                                         */
/*  Copyright 2006 by                                                      */
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
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "psconv.h"
#include "psobjs.h"
#include "psauxerr.h"


  /* The following array is used by various functions to quickly convert */
  /* digits (both decimal and non-decimal) into numbers.                 */

#if 'A' == 65
  /* ASCII */

  static const FT_Char  ft_char_table[128] =
  {
    /* 0x00 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  };

  /* no character >= 0x80 can represent a valid number */
#define OP  >=

#endif /* 'A' == 65 */

#if 'A' == 193
  /* EBCDIC */

  static const FT_Char  ft_char_table[128] =
  {
    /* 0x80 */
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  };

  /* no character < 0x80 can represent a valid number */
#define OP  <

#endif /* 'A' == 193 */


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_Strtol( FT_Byte**  cursor,
                  FT_Byte*   limit,
                  FT_Int     base )
  {
    FT_Byte*  p = *cursor;
    FT_Int    num = 0;
    FT_Bool   sign = 0;


    if ( p == limit || base < 2 || base > 36 )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    for ( ; p < limit; p++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( c < 0 || c >= base )
        break;

      num = num * base + c;
    }

    if ( sign )
      num = -num;

    *cursor = p;

    return num;
  }


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_ToInt( FT_Byte**  cursor,
                 FT_Byte*   limit )

  {
    FT_Byte*  p;
    FT_Int    num;


    num = PS_Conv_Strtol( cursor, limit, 10 );
    p   = *cursor;

    if ( p < limit && *p == '#' )
    {
      *cursor = p + 1;

      return PS_Conv_Strtol( cursor, limit, num );
    }
    else
      return num;
  }


  FT_LOCAL_DEF( FT_Fixed )
  PS_Conv_ToFixed( FT_Byte**  cursor,
                   FT_Byte*   limit,
                   FT_Int     power_ten )
  {
    FT_Byte*  p = *cursor;
    FT_Fixed  integral;
    FT_Long   decimal = 0, divider = 1;
    FT_Bool   sign = 0;


    if ( p == limit )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    if ( *p != '.' )
      integral = (FT_Fixed)PS_Conv_ToInt( &p, limit ) << 16;
    else
      integral = 0;

    /* read the decimal part */
    if ( p < limit && *p == '.' )
    {
      p++;

      for ( ; p < limit; p++ )
      {
        FT_Char  c;


        if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
          break;

        c = ft_char_table[*p & 0x7f];

        if ( c < 0 || c >= 10 )
          break;

        if ( divider < 10000000L )
        {
          decimal = decimal * 10 + c;
          divider *= 10;
        }
      }
    }

    /* read exponent, if any */
    if ( p + 1 < limit && ( *p == 'e' || *p == 'E' ) )
    {
      p++;
      power_ten += PS_Conv_ToInt( &p, limit );
    }

    while ( power_ten > 0 )
    {
      integral *= 10;
      decimal  *= 10;
      power_ten--;
    }

    while ( power_ten < 0 )
    {
      integral /= 10;
      divider  *= 10;
      power_ten++;
    }

    if ( decimal )
      integral += FT_DivFix( decimal, divider );

    if ( sign )
      integral = -integral;

    *cursor = p;

    return integral;
  }


#if 0
  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_StringDecode( FT_Byte**  cursor,
                        FT_Byte*   limit,
                        FT_Byte*   buffer,
                        FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r = 0;


    for ( p = *cursor; r < n && p < limit; p++ )
    {
      FT_Byte  b;


      if ( *p != '\\' )
      {
        buffer[r++] = *p;

        continue;
      }

      p++;

      switch ( *p )
      {
      case 'n':
        b = '\n';
        break;
      case 'r':
        b = '\r';
        break;
      case 't':
        b = '\t';
        break;
      case 'b':
        b = '\b';
        break;
      case 'f':
        b = '\f';
        break;
      case '\r':
        p++;
        if ( *p != '\n' )
        {
          b = *p;

          break;
        }
        /* no break */
      case '\n':
        continue;
        break;
      default:
        if ( IS_PS_DIGIT( *p ) )
        {
          b = *p - '0';

          p++;

          if ( IS_PS_DIGIT( *p ) )
          {
            b = b * 8 + *p - '0';

            p++;

            if ( IS_PS_DIGIT( *p ) )
              b = b * 8 + *p - '0';
            else
            {
              buffer[r++] = b;
              b = *p;
            }
          }
          else
          {
            buffer[r++] = b;
            b = *p;
          }
        }
        else
          b = *p;
        break;
      }

      buffer[r++] = b;
    }

    *cursor = p;

    return r;
  }
#endif /* 0 */


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_ASCIIHexDecode( FT_Byte**  cursor,
                          FT_Byte*   limit,
                          FT_Byte*   buffer,
                          FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r = 0;


    n *= 2;
    for ( p = *cursor; r < n && p < limit; p++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) )
        continue;

      if ( *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( c < 0 || c >= 16 )
        break;

      if ( r % 2 )
      {
        *buffer = (FT_Byte)(*buffer + c);
        buffer++;
      }
      else
        *buffer = (FT_Byte)(c << 4);

      r++;
    }

    *cursor = p;

    return ( r + 1 ) / 2;
  }


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_EexecDecode( FT_Byte**   cursor,
                       FT_Byte*    limit,
                       FT_Byte*    buffer,
                       FT_UInt     n,
                       FT_UShort*  seed )
  {
    FT_Byte*   p;
    FT_UInt    r;
    FT_UShort  s = *seed;


    for ( r = 0, p = *cursor; r < n && p < limit; r++, p++ )
    {
      FT_Byte  b = (FT_Byte)( *p ^ ( s >> 8 ) );


      s = (FT_UShort)( ( *p + s ) * 52845U + 22719 );
      *buffer++ = b;
    }

    *cursor = p;
    *seed   = s;

    return r;
  }


/* END */



/* END */

/***************************************************************************/
/*                                                                         */
/*  type1cid.c                                                             */
/*                                                                         */
/*    FreeType OpenType driver component (body only).                      */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
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
/*  cidparse.c                                                             */
/*                                                                         */
/*    CID-keyed Type1 parser (body).                                       */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H

#include "cidparse.h"

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidparse


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    INPUT STREAM PARSER                        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  cid_parser_new( CID_Parser*    parser,
                  FT_Stream      stream,
                  FT_Memory      memory,
                  PSAux_Service  psaux )
  {
    FT_Error  error;
    FT_ULong  base_offset, offset, ps_len;
    FT_Byte   *cur, *limit;
    FT_Byte   *arg1, *arg2;


    FT_MEM_ZERO( parser, sizeof ( *parser ) );
    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream = stream;

    base_offset = FT_STREAM_POS();

    /* first of all, check the font format in the header */
    if ( FT_FRAME_ENTER( 31 ) )
      goto Exit;

    if ( ft_strncmp( (char *)stream->cursor,
                     "%!PS-Adobe-3.0 Resource-CIDFont", 31 ) )
    {
      FT_TRACE2(( "[not a valid CID-keyed font]\n" ));
      error = CID_Err_Unknown_File_Format;
    }

    FT_FRAME_EXIT();
    if ( error )
      goto Exit;

  Again:
    /* now, read the rest of the file until we find a `StartData' */
    {
      FT_Byte   buffer[256 + 10];
      FT_Int    read_len = 256 + 10;
      FT_Byte*  p        = buffer;


      for ( offset = (FT_ULong)FT_STREAM_POS(); ; offset += 256 )
      {
        FT_Int    stream_len;


        stream_len = stream->size - FT_STREAM_POS();
        if ( stream_len == 0 )
          goto Exit;

        read_len = FT_MIN( read_len, stream_len );
        if ( FT_STREAM_READ( p, read_len ) )
          goto Exit;

        if ( read_len < 256 )
          p[read_len]  = '\0';

        limit = p + read_len - 10;

        for ( p = buffer; p < limit; p++ )
        {
          if ( p[0] == 'S' && ft_strncmp( (char*)p, "StartData", 9 ) == 0 )
          {
            /* save offset of binary data after `StartData' */
            offset += p - buffer + 10;
            goto Found;
          }
        }

        FT_MEM_MOVE( buffer, p, 10 );
        read_len = 256;
        p = buffer + 10;
      }
    }

  Found:
    /* We have found the start of the binary data.  Now rewind and */
    /* extract the frame corresponding to the PostScript section.  */

    ps_len = offset - base_offset;
    if ( FT_STREAM_SEEK( base_offset )                  ||
         FT_FRAME_EXTRACT( ps_len, parser->postscript ) )
      goto Exit;

    parser->data_offset    = offset;
    parser->postscript_len = ps_len;
    parser->root.base      = parser->postscript;
    parser->root.cursor    = parser->postscript;
    parser->root.limit     = parser->root.cursor + ps_len;
    parser->num_dict       = -1;

    /* Finally, we check whether `StartData' was real -- it could be  */
    /* in a comment or string.  We also get its arguments to find out */
    /* whether the data is represented in binary or hex format.       */

    arg1 = parser->root.cursor;
    cid_parser_skip_PS_token( parser );
    cid_parser_skip_spaces  ( parser );
    arg2 = parser->root.cursor;
    cid_parser_skip_PS_token( parser );
    cid_parser_skip_spaces  ( parser );

    limit = parser->root.limit;
    cur   = parser->root.cursor;

    while ( cur < limit )
    {
      if ( parser->root.error )
        break;

      if ( *cur == 'S' && ft_strncmp( (char*)cur, "StartData", 9 ) == 0 )
      {
        if ( ft_strncmp( (char*)arg1, "(Hex)", 5 ) == 0 )
          parser->binary_length = ft_atol( (const char *)arg2 );

        limit = parser->root.limit;
        cur   = parser->root.cursor;
        goto Exit;
      }

      cid_parser_skip_PS_token( parser );
      cid_parser_skip_spaces  ( parser );
      arg1 = arg2;
      arg2 = cur;
      cur  = parser->root.cursor;
    }

    /* we haven't found the correct `StartData'; go back and continue */
    /* searching                                                      */
    FT_FRAME_RELEASE( parser->postscript );
    if ( !FT_STREAM_SEEK( offset ) )
      goto Again;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  cid_parser_done( CID_Parser*  parser )
  {
    /* always free the private dictionary */
    if ( parser->postscript )
    {
      FT_Stream  stream = parser->stream;


      FT_FRAME_RELEASE( parser->postscript );
    }
    parser->root.funcs.done( &parser->root );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cidload.c                                                              */
/*                                                                         */
/*    CID-keyed Type1 font loader (body).                                  */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_CONFIG_CONFIG_H
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_TYPE1_TYPES_H

#include "cidload.h"

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidload


  /* read a single offset */
  FT_LOCAL_DEF( FT_Long )
  cid_get_offset( FT_Byte*  *start,
                  FT_Byte    offsize )
  {
    FT_Long   result;
    FT_Byte*  p = *start;


    for ( result = 0; offsize > 0; offsize-- )
    {
      result <<= 8;
      result  |= *p++;
    }

    *start = p;
    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    TYPE 1 SYMBOL PARSING                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  cid_load_keyword( CID_Face        face,
                    CID_Loader*     loader,
                    const T1_Field  keyword )
  {
    FT_Error      error;
    CID_Parser*   parser = &loader->parser;
    FT_Byte*      object;
    void*         dummy_object;
    CID_FaceInfo  cid = &face->cid;


    /* if the keyword has a dedicated callback, call it */
    if ( keyword->type == T1_FIELD_TYPE_CALLBACK )
    {
      keyword->reader( (FT_Face)face, parser );
      error = parser->root.error;
      goto Exit;
    }

    /* we must now compute the address of our target object */
    switch ( keyword->location )
    {
    case T1_FIELD_LOCATION_CID_INFO:
      object = (FT_Byte*)cid;
      break;

    case T1_FIELD_LOCATION_FONT_INFO:
      object = (FT_Byte*)&cid->font_info;
      break;

    case T1_FIELD_LOCATION_BBOX:
      object = (FT_Byte*)&cid->font_bbox;
      break;

    default:
      {
        CID_FaceDict  dict;


        if ( parser->num_dict < 0 )
        {
          FT_ERROR(( "cid_load_keyword: invalid use of `%s'!\n",
                     keyword->ident ));
          error = CID_Err_Syntax_Error;
          goto Exit;
        }

        dict = cid->font_dicts + parser->num_dict;
        switch ( keyword->location )
        {
        case T1_FIELD_LOCATION_PRIVATE:
          object = (FT_Byte*)&dict->private_dict;
          break;

        default:
          object = (FT_Byte*)dict;
        }
      }
    }

    dummy_object = object;

    /* now, load the keyword data in the object's field(s) */
    if ( keyword->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         keyword->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = cid_parser_load_field_table( &loader->parser, keyword,
                                           &dummy_object );
    else
      error = cid_parser_load_field( &loader->parser,
                                     keyword, &dummy_object );
  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  parse_font_matrix( CID_Face     face,
                     CID_Parser*  parser )
  {
    FT_Matrix*    matrix;
    FT_Vector*    offset;
    CID_FaceDict  dict;
    FT_Face       root = (FT_Face)&face->root;
    FT_Fixed      temp[6];
    FT_Fixed      temp_scale;


    if ( parser->num_dict >= 0 )
    {
      dict   = face->cid.font_dicts + parser->num_dict;
      matrix = &dict->font_matrix;
      offset = &dict->font_offset;

      (void)cid_parser_to_fixed_array( parser, 6, temp, 3 );

      temp_scale = FT_ABS( temp[3] );

      /* Set units per EM based on FontMatrix values.  We set the value to */
      /* `1000/temp_scale', because temp_scale was already multiplied by   */
      /* 1000 (in `t1_tofixed', from psobjs.c).                            */
      root->units_per_EM = (FT_UShort)( FT_DivFix( 0x10000L,
                                        FT_DivFix( temp_scale, 1000 ) ) );

      /* we need to scale the values by 1.0/temp[3] */
      if ( temp_scale != 0x10000L )
      {
        temp[0] = FT_DivFix( temp[0], temp_scale );
        temp[1] = FT_DivFix( temp[1], temp_scale );
        temp[2] = FT_DivFix( temp[2], temp_scale );
        temp[4] = FT_DivFix( temp[4], temp_scale );
        temp[5] = FT_DivFix( temp[5], temp_scale );
        temp[3] = 0x10000L;
      }

      matrix->xx = temp[0];
      matrix->yx = temp[1];
      matrix->xy = temp[2];
      matrix->yy = temp[3];

      /* note that the font offsets are expressed in integer font units */
      offset->x  = temp[4] >> 16;
      offset->y  = temp[5] >> 16;
    }

    return CID_Err_Ok;      /* this is a callback function; */
                            /* we must return an error code */
  }


  FT_CALLBACK_DEF( FT_Error )
  parse_fd_array( CID_Face     face,
                  CID_Parser*  parser )
  {
    CID_FaceInfo  cid    = &face->cid;
    FT_Memory     memory = face->root.memory;
    FT_Error      error  = CID_Err_Ok;
    FT_Long       num_dicts;


    num_dicts = cid_parser_to_int( parser );

    if ( !cid->font_dicts )
    {
      FT_Int  n;


      if ( FT_NEW_ARRAY( cid->font_dicts, num_dicts ) )
        goto Exit;

      cid->num_dicts = (FT_UInt)num_dicts;

      /* don't forget to set a few defaults */
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        CID_FaceDict  dict = cid->font_dicts + n;


        /* default value for lenIV */
        dict->private_dict.lenIV = 4;
      }
    }

  Exit:
    return error;
  }


  static
  const T1_FieldRec  cid_field_records[] =
  {

#include "cidtoken.h"

    T1_FIELD_CALLBACK( "FDArray",    parse_fd_array )
    T1_FIELD_CALLBACK( "FontMatrix", parse_font_matrix )

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0 }
  };


  static FT_Error
  cid_parse_dict( CID_Face     face,
                  CID_Loader*  loader,
                  FT_Byte*     base,
                  FT_Long      size )
  {
    CID_Parser*  parser = &loader->parser;


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = CID_Err_Ok;

    {
      FT_Byte*  cur   = base;
      FT_Byte*  limit = cur + size;


      for (;;)
      {
        FT_Byte*  newlimit;


        parser->root.cursor = cur;
        cid_parser_skip_spaces( parser );

        if ( parser->root.cursor >= limit )
          newlimit = limit - 1 - 17;
        else
          newlimit = parser->root.cursor - 17;

        /* look for `%ADOBeginFontDict' */
        for ( ; cur < newlimit; cur++ )
        {
          if ( *cur == '%'                                            &&
               ft_strncmp( (char*)cur, "%ADOBeginFontDict", 17 ) == 0 )
          {
            /* if /FDArray was found, then cid->num_dicts is > 0, and */
            /* we can start increasing parser->num_dict               */
            if ( face->cid.num_dicts > 0 )
              parser->num_dict++;
          }
        }

        cur = parser->root.cursor;
        /* no error can occur in cid_parser_skip_spaces */
        if ( cur >= limit )
          break;

        cid_parser_skip_PS_token( parser );
        if ( parser->root.cursor >= limit || parser->root.error )
          break;

        /* look for immediates */
        if ( *cur == '/' && cur + 2 < limit )
        {
          FT_PtrDist  len;


          cur++;
          len = parser->root.cursor - cur;

          if ( len > 0 && len < 22 )
          {
            /* now compare the immediate name to the keyword table */
            T1_Field  keyword = (T1_Field)cid_field_records;


            for (;;)
            {
              FT_Byte*  name;


              name = (FT_Byte*)keyword->ident;
              if ( !name )
                break;

              if ( cur[0] == name[0]                                 &&
                   len == (FT_PtrDist)ft_strlen( (const char*)name ) )
              {
                FT_PtrDist  n;


                for ( n = 1; n < len; n++ )
                  if ( cur[n] != name[n] )
                    break;

                if ( n >= len )
                {
                  /* we found it - run the parsing callback */
                  parser->root.error = cid_load_keyword( face,
                                                         loader,
                                                         keyword );
                  if ( parser->root.error )
                    return parser->root.error;
                  break;
                }
              }
              keyword++;
            }
          }
        }

        cur = parser->root.cursor;
      }
    }
    return parser->root.error;
  }


  /* read the subrmap and the subrs of each font dict */
  static FT_Error
  cid_read_subrs( CID_Face  face )
  {
    CID_FaceInfo   cid    = &face->cid;
    FT_Memory      memory = face->root.memory;
    FT_Stream      stream = face->cid_stream;
    FT_Error       error;
    FT_Int         n;
    CID_Subrs      subr;
    FT_UInt        max_offsets = 0;
    FT_ULong*      offsets = 0;
    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    if ( FT_NEW_ARRAY( face->subrs, cid->num_dicts ) )
      goto Exit;

    subr = face->subrs;
    for ( n = 0; n < cid->num_dicts; n++, subr++ )
    {
      CID_FaceDict  dict  = cid->font_dicts + n;
      FT_Int        lenIV = dict->private_dict.lenIV;
      FT_UInt       count, num_subrs = dict->num_subrs;
      FT_ULong      data_len;
      FT_Byte*      p;


      /* reallocate offsets array if needed */
      if ( num_subrs + 1 > max_offsets )
      {
        FT_UInt  new_max = FT_PAD_CEIL( num_subrs + 1, 4 );


        if ( FT_RENEW_ARRAY( offsets, max_offsets, new_max ) )
          goto Fail;

        max_offsets = new_max;
      }

      /* read the subrmap's offsets */
      if ( FT_STREAM_SEEK( cid->data_offset + dict->subrmap_offset ) ||
           FT_FRAME_ENTER( ( num_subrs + 1 ) * dict->sd_bytes )      )
        goto Fail;

      p = (FT_Byte*)stream->cursor;
      for ( count = 0; count <= num_subrs; count++ )
        offsets[count] = cid_get_offset( &p, (FT_Byte)dict->sd_bytes );

      FT_FRAME_EXIT();

      /* now, compute the size of subrs charstrings, */
      /* allocate, and read them                     */
      data_len = offsets[num_subrs] - offsets[0];

      if ( FT_NEW_ARRAY( subr->code, num_subrs + 1 ) ||
               FT_ALLOC( subr->code[0], data_len )   )
        goto Fail;

      if ( FT_STREAM_SEEK( cid->data_offset + offsets[0] ) ||
           FT_STREAM_READ( subr->code[0], data_len )  )
        goto Fail;

      /* set up pointers */
      for ( count = 1; count <= num_subrs; count++ )
      {
        FT_ULong  len;


        len               = offsets[count] - offsets[count - 1];
        subr->code[count] = subr->code[count - 1] + len;
      }

      /* decrypt subroutines, but only if lenIV >= 0 */
      if ( lenIV >= 0 )
      {
        for ( count = 0; count < num_subrs; count++ )
        {
          FT_ULong  len;


          len = offsets[count + 1] - offsets[count];
          psaux->t1_decrypt( subr->code[count], len, 4330 );
        }
      }

      subr->num_subrs = num_subrs;
    }

  Exit:
    FT_FREE( offsets );
    return error;

  Fail:
    if ( face->subrs )
    {
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        if ( face->subrs[n].code )
          FT_FREE( face->subrs[n].code[0] );

        FT_FREE( face->subrs[n].code );
      }
      FT_FREE( face->subrs );
    }
    goto Exit;
  }


  static void
  t1_init_loader( CID_Loader*  loader,
                  CID_Face     face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
  }


  static void
  t1_done_loader( CID_Loader*  loader )
  {
    CID_Parser*  parser = &loader->parser;


    /* finalize parser */
    cid_parser_done( parser );
  }


  static FT_Error
  cid_hex_to_binary( FT_Byte*  data,
                     FT_Long   data_len,
                     FT_ULong  offset,
                     CID_Face  face )
  {
    FT_Stream  stream = face->root.stream;
    FT_Error   error;

    FT_Byte    buffer[256];
    FT_Byte   *p, *plimit;
    FT_Byte   *d, *dlimit;
    FT_Byte    val;

    FT_Bool    upper_nibble, done;


    if ( FT_STREAM_SEEK( offset ) )
      goto Exit;

    d      = data;
    dlimit = d + data_len;
    p      = buffer;
    plimit = p;

    upper_nibble = 1;
    done         = 0;

    while ( d < dlimit )
    {
      if ( p >= plimit )
      {
        FT_ULong  oldpos = FT_STREAM_POS();
        FT_ULong  size   = stream->size - oldpos;


        if ( size == 0 )
        {
          error = CID_Err_Syntax_Error;
          goto Exit;
        }

        if ( FT_STREAM_READ( buffer, 256 > size ? size : 256 ) )
          goto Exit;
        p      = buffer;
        plimit = p + FT_STREAM_POS() - oldpos;
      }

      if ( ft_isdigit( *p ) )
        val = (FT_Byte)( *p - '0' );
      else if ( *p >= 'a' && *p <= 'f' )
        val = (FT_Byte)( *p - 'a' );
      else if ( *p >= 'A' && *p <= 'F' )
        val = (FT_Byte)( *p - 'A' + 10 );
      else if ( *p == ' '  ||
                *p == '\t' ||
                *p == '\r' ||
                *p == '\n' ||
                *p == '\f' ||
                *p == '\0' )
      {
        p++;
        continue;
      }
      else if ( *p == '>' )
      {
        val  = 0;
        done = 1;
      }
      else
      {
        error = CID_Err_Syntax_Error;
        goto Exit;
      }

      if ( upper_nibble )
        *d = (FT_Byte)( val << 4 );
      else
      {
        *d = (FT_Byte)( *d + val );
        d++;
      }

      upper_nibble = (FT_Byte)( 1 - upper_nibble );

      if ( done )
        break;

      p++;
    }

    error = CID_Err_Ok;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  cid_face_open( CID_Face  face,
                 FT_Int    face_index )
  {
    CID_Loader   loader;
    CID_Parser*  parser;
    FT_Memory    memory = face->root.memory;
    FT_Error     error;


    t1_init_loader( &loader, face );

    parser = &loader.parser;
    error = cid_parser_new( parser, face->root.stream, face->root.memory,
                            (PSAux_Service)face->psaux );
    if ( error )
      goto Exit;

    error = cid_parse_dict( face, &loader,
                            parser->postscript,
                            parser->postscript_len );
    if ( error )
      goto Exit;

    if ( face_index < 0 )
      goto Exit;

    if ( FT_NEW( face->cid_stream ) )
      goto Exit;

    if ( parser->binary_length )
    {
      /* we must convert the data section from hexadecimal to binary */
      if ( FT_ALLOC( face->binary_data, parser->binary_length )         ||
           cid_hex_to_binary( face->binary_data, parser->binary_length,
                              parser->data_offset, face )               )
        goto Exit;

      FT_Stream_OpenMemory( face->cid_stream,
                            face->binary_data, parser->binary_length );
      face->cid.data_offset = 0;
    }
    else
    {
      *face->cid_stream     = *face->root.stream;
      face->cid.data_offset = loader.parser.data_offset;
    }

    error = cid_read_subrs( face );

  Exit:
    t1_done_loader( &loader );
    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cidobjs.c                                                              */
/*                                                                         */
/*    CID objects manager (body).                                          */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "cidgload.h"
#include "cidload.h"

#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidobjs


  /*************************************************************************/
  /*                                                                       */
  /*                            SLOT  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  cid_slot_done( FT_GlyphSlot  slot )
  {
    slot->internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  cid_slot_init( FT_GlyphSlot  slot )
  {
    CID_Face          face;
    PSHinter_Service  pshinter;


    face     = (CID_Face)slot->face;
    pshinter = (PSHinter_Service)face->pshinter;

    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T1_Hints_Funcs  funcs;


        funcs = pshinter->get_t1_funcs( module );
        slot->internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           SIZE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/


  static PSH_Globals_Funcs
  cid_size_get_globals_funcs( CID_Size  size )
  {
    CID_Face          face     = (CID_Face)size->root.face;
    PSHinter_Service  pshinter = (PSHinter_Service)face->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF( void )
  cid_size_done( FT_Size  cidsize )         /* CID_Size */
  {
    CID_Size  size = (CID_Size)cidsize;


    if ( cidsize->internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = cid_size_get_globals_funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)cidsize->internal );

      cidsize->internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cid_size_init( FT_Size  cidsize )     /* CID_Size */
  {
    CID_Size           size  = (CID_Size)cidsize;
    FT_Error           error = 0;
    PSH_Globals_Funcs  funcs = cid_size_get_globals_funcs( size );


    if ( funcs )
    {
      PSH_Globals   globals;
      CID_Face      face = (CID_Face)cidsize->face;
      CID_FaceDict  dict = face->cid.font_dicts + face->root.face_index;
      PS_Private    priv = &dict->private_dict;


      error = funcs->create( cidsize->face->memory, priv, &globals );
      if ( !error )
        cidsize->internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL( FT_Error )
  cid_size_request( FT_Size          size,
                    FT_Size_Request  req )
  {
    PSH_Globals_Funcs  funcs;


    FT_Request_Metrics( size->face, req );

    funcs = cid_size_get_globals_funcs( (CID_Size)size );

    if ( funcs )
      funcs->set_scale( (PSH_Globals)size->internal,
                        size->metrics.x_scale,
                        size->metrics.y_scale,
                        0, 0 );

    return CID_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_face_done                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given face object.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cid_face_done( FT_Face  cidface )         /* CID_Face */
  {
    CID_Face   face = (CID_Face)cidface;
    FT_Memory  memory;


    if ( face )
    {
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      memory = cidface->memory;

      /* release subrs */
      if ( face->subrs )
      {
        FT_Int  n;


        for ( n = 0; n < cid->num_dicts; n++ )
        {
          CID_Subrs  subr = face->subrs + n;


          if ( subr->code )
          {
            FT_FREE( subr->code[0] );
            FT_FREE( subr->code );
          }
        }

        FT_FREE( face->subrs );
      }

      /* release FontInfo strings */
      FT_FREE( info->version );
      FT_FREE( info->notice );
      FT_FREE( info->full_name );
      FT_FREE( info->family_name );
      FT_FREE( info->weight );

      /* release font dictionaries */
      FT_FREE( cid->font_dicts );
      cid->num_dicts = 0;

      /* release other strings */
      FT_FREE( cid->cid_font_name );
      FT_FREE( cid->registry );
      FT_FREE( cid->ordering );

      cidface->family_name = 0;
      cidface->style_name  = 0;

      FT_FREE( face->binary_data );
      FT_FREE( face->cid_stream );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_face_init                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given CID face object.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The source font stream.                              */
  /*                                                                       */
  /*    face_index :: The index of the font face in the resource.          */
  /*                                                                       */
  /*    num_params :: Number of additional generic parameters.  Ignored.   */
  /*                                                                       */
  /*    params     :: Additional generic parameters.  Ignored.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The newly built face object.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  cid_face_init( FT_Stream      stream,
                 FT_Face        cidface,        /* CID_Face */
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    CID_Face          face = (CID_Face)cidface;
    FT_Error          error;
    PSAux_Service     psaux;
    PSHinter_Service  pshinter;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( stream );


    cidface->num_faces = 1;

    psaux = (PSAux_Service)face->psaux;
    if ( !psaux )
    {
      psaux = (PSAux_Service)FT_Get_Module_Interface(
                FT_FACE_LIBRARY( face ), "psaux" );

      face->psaux = psaux;
    }

    pshinter = (PSHinter_Service)face->pshinter;
    if ( !pshinter )
    {
      pshinter = (PSHinter_Service)FT_Get_Module_Interface(
                   FT_FACE_LIBRARY( face ), "pshinter" );

      face->pshinter = pshinter;
    }

    /* open the tokenizer; this will also check the font format */
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    error = cid_face_open( face, face_index );
    if ( error )
      goto Exit;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Exit;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "cid_face_init: invalid face index\n" ));
      error = CID_Err_Invalid_Argument;
      goto Exit;
    }

    /* now load the font program into the face object */

    /* initialize the face object fields */

    /* set up root face fields */
    {
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      cidface->num_glyphs   = cid->cid_count;
      cidface->num_charmaps = 0;

      cidface->face_index = face_index;
      cidface->face_flags = FT_FACE_FLAG_SCALABLE   | /* scalable outlines */
                            FT_FACE_FLAG_HORIZONTAL | /* horizontal data   */
                            FT_FACE_FLAG_HINTER;      /* has native hinter */

      if ( info->is_fixed_pitch )
        cidface->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* XXX: TODO: add kerning with .afm support */

      /* get style name -- be careful, some broken fonts only */
      /* have a /FontName dictionary entry!                   */
      cidface->family_name = info->family_name;
      /* assume "Regular" style if we don't know better */
      cidface->style_name = (char *)"Regular";
      if ( cidface->family_name )
      {
        char*  full   = info->full_name;
        char*  family = cidface->family_name;


        if ( full )
        {
          while ( *full )
          {
            if ( *full == *family )
            {
              family++;
              full++;
            }
            else
            {
              if ( *full == ' ' || *full == '-' )
                full++;
              else if ( *family == ' ' || *family == '-' )
                family++;
              else
              {
                if ( !*family )
                  cidface->style_name = full;
                break;
              }
            }
          }
        }
      }
      else
      {
        /* do we have a `/FontName'? */
        if ( cid->cid_font_name )
          cidface->family_name = cid->cid_font_name;
      }

      /* compute style flags */
      cidface->style_flags = 0;
      if ( info->italic_angle )
        cidface->style_flags |= FT_STYLE_FLAG_ITALIC;
      if ( info->weight )
      {
        if ( !ft_strcmp( info->weight, "Bold"  ) ||
             !ft_strcmp( info->weight, "Black" ) )
          cidface->style_flags |= FT_STYLE_FLAG_BOLD;
      }

      /* no embedded bitmap support */
      cidface->num_fixed_sizes = 0;
      cidface->available_sizes = 0;

      cidface->bbox.xMin =   cid->font_bbox.xMin             >> 16;
      cidface->bbox.yMin =   cid->font_bbox.yMin             >> 16;
      cidface->bbox.xMax = ( cid->font_bbox.xMax + 0xFFFFU ) >> 16;
      cidface->bbox.yMax = ( cid->font_bbox.yMax + 0xFFFFU ) >> 16;

      if ( !cidface->units_per_EM )
        cidface->units_per_EM = 1000;

      cidface->ascender  = (FT_Short)( cidface->bbox.yMax );
      cidface->descender = (FT_Short)( cidface->bbox.yMin );

      cidface->height = (FT_Short)( ( cidface->units_per_EM * 12 ) / 10 );
      if ( cidface->height < cidface->ascender - cidface->descender )
        cidface->height = (FT_Short)( cidface->ascender - cidface->descender );

      cidface->underline_position  = (FT_Short)info->underline_position;
      cidface->underline_thickness = (FT_Short)info->underline_thickness;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_driver_init                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given CID driver object.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  cid_driver_init( FT_Module  driver )
  {
    FT_UNUSED( driver );

    return CID_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_driver_done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given CID driver.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target CID driver.                       */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cid_driver_done( FT_Module  driver )
  {
    FT_UNUSED( driver );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cidriver.c                                                             */
/*                                                                         */
/*    CID driver interface (body).                                         */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2006 by                         */
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
#include "cidriver.h"
#include "cidgload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "ciderrs.h"

#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_POSTSCRIPT_INFO_H

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ciddriver


 /*
  *  POSTSCRIPT NAME SERVICE
  *
  */

  static const char*
  cid_get_postscript_name( CID_Face  face )
  {
    const char*  result = face->cid.cid_font_name;


    if ( result && result[0] == '/' )
      result++;

    return result;
  }


  static const FT_Service_PsFontNameRec  cid_service_ps_name =
  {
    (FT_PsName_GetFunc) cid_get_postscript_name
  };


 /*
  *  POSTSCRIPT INFO SERVICE
  *
  */

  static FT_Error
  cid_ps_get_font_info( FT_Face          face,
                        PS_FontInfoRec*  afont_info )
  {
    *afont_info = ((CID_Face)face)->cid.font_info;
    return 0;
  }


  static const FT_Service_PsInfoRec  cid_service_ps_info =
  {
    (PS_GetFontInfoFunc)   cid_ps_get_font_info,
    (PS_HasGlyphNamesFunc) NULL,        /* unsupported with CID fonts */
    (PS_GetFontPrivateFunc)NULL         /* unsupported                */
  };


 /*
  *  SERVICE LIST
  *
  */

  static const FT_ServiceDescRec  cid_services[] =
  {
    { FT_SERVICE_ID_POSTSCRIPT_FONT_NAME, &cid_service_ps_name },
    { FT_SERVICE_ID_XF86_NAME,            FT_XF86_FORMAT_CID },
    { FT_SERVICE_ID_POSTSCRIPT_INFO,      &cid_service_ps_info },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cid_get_interface( FT_Module    module,
                     const char*  cid_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( cid_services, cid_interface );
  }



  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  t1cid_driver_class =
  {
    /* first of all, the FT_Module_Class fields */
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof( FT_DriverRec ),
      "t1cid",   /* module name           */
      0x10000L,  /* version 1.0 of driver */
      0x20000L,  /* requires FreeType 2.0 */

      0,

      cid_driver_init,
      cid_driver_done,
      cid_get_interface
    },

    /* then the other font drivers fields */
    sizeof( CID_FaceRec ),
    sizeof( CID_SizeRec ),
    sizeof( CID_GlyphSlotRec ),

    cid_face_init,
    cid_face_done,

    cid_size_init,
    cid_size_done,
    cid_slot_init,
    cid_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif

    cid_slot_load_glyph,

    0,                      /* FT_Face_GetKerningFunc  */
    0,                      /* FT_Face_AttachFunc      */

    0,                      /* FT_Face_GetAdvancesFunc */

    cid_size_request,
    0                       /* FT_Size_SelectFunc      */
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  cidgload.c                                                             */
/*                                                                         */
/*    CID-keyed Type1 Glyph Loader (body).                                 */
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
#include "cidload.h"
#include "cidgload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_OUTLINE_H

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidgload


  FT_CALLBACK_DEF( FT_Error )
  cid_load_glyph( T1_Decoder  decoder,
                  FT_UInt     glyph_index )
  {
    CID_Face       face = (CID_Face)decoder->builder.face;
    CID_FaceInfo   cid  = &face->cid;
    FT_Byte*       p;
    FT_UInt        fd_select;
    FT_Stream      stream = face->cid_stream;
    FT_Error       error  = 0;
    FT_Byte*       charstring = 0;
    FT_Memory      memory = face->root.memory;
    FT_ULong       glyph_length = 0;
    PSAux_Service  psaux = (PSAux_Service)face->psaux;


#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* For incremental fonts get the character data using */
    /* the callback function.                             */
    if ( face->root.internal->incremental_interface )
    {
      FT_Data  glyph_data;


      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index,
                &glyph_data );
      if ( error )
        goto Exit;

      p         = (FT_Byte*)glyph_data.pointer;
      fd_select = (FT_UInt)cid_get_offset( &p, (FT_Byte)cid->fd_bytes );

      if ( glyph_data.length != 0 )
      {
        glyph_length = glyph_data.length - cid->fd_bytes;
        FT_ALLOC( charstring, glyph_length );
        if ( !error )
          ft_memcpy( charstring, glyph_data.pointer + cid->fd_bytes,
                     glyph_length );
      }

      face->root.internal->incremental_interface->funcs->free_glyph_data(
                face->root.internal->incremental_interface->object,
                &glyph_data );

      if ( error )
        goto Exit;
    }

    else

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    /* For ordinary fonts read the CID font dictionary index */
    /* and charstring offset from the CIDMap.                */
    {
      FT_UInt   entry_len = cid->fd_bytes + cid->gd_bytes;
      FT_ULong  off1;


      if ( FT_STREAM_SEEK( cid->data_offset + cid->cidmap_offset +
                           glyph_index * entry_len )               ||
           FT_FRAME_ENTER( 2 * entry_len )                         )
        goto Exit;

      p            = (FT_Byte*)stream->cursor;
      fd_select    = (FT_UInt) cid_get_offset( &p, (FT_Byte)cid->fd_bytes );
      off1         = (FT_ULong)cid_get_offset( &p, (FT_Byte)cid->gd_bytes );
      p           += cid->fd_bytes;
      glyph_length = cid_get_offset( &p, (FT_Byte)cid->gd_bytes ) - off1;
      FT_FRAME_EXIT();

      if ( glyph_length == 0 )
        goto Exit;
      if ( FT_ALLOC( charstring, glyph_length ) )
        goto Exit;
      if ( FT_STREAM_READ_AT( cid->data_offset + off1,
                              charstring, glyph_length ) )
        goto Exit;
    }

    /* Now set up the subrs array and parse the charstrings. */
    {
      CID_FaceDict  dict;
      CID_Subrs     cid_subrs = face->subrs + fd_select;
      FT_Int        cs_offset;


      /* Set up subrs */
      decoder->num_subrs = cid_subrs->num_subrs;
      decoder->subrs     = cid_subrs->code;
      decoder->subrs_len = 0;

      /* Set up font matrix */
      dict                 = cid->font_dicts + fd_select;

      decoder->font_matrix = dict->font_matrix;
      decoder->font_offset = dict->font_offset;
      decoder->lenIV       = dict->private_dict.lenIV;

      /* Decode the charstring. */

      /* Adjustment for seed bytes. */
      cs_offset = ( decoder->lenIV >= 0 ? decoder->lenIV : 0 );

      /* Decrypt only if lenIV >= 0. */
      if ( decoder->lenIV >= 0 )
        psaux->t1_decrypt( charstring, glyph_length, 4330 );

      error = decoder->funcs.parse_charstrings(
                decoder, charstring + cs_offset,
                (FT_Int)glyph_length - cs_offset  );
    }

    FT_FREE( charstring );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* Incremental fonts can optionally override the metrics. */
    if ( !error                                                              &&
         face->root.internal->incremental_interface                          &&
         face->root.internal->incremental_interface->funcs->get_glyph_metrics )
    {
      FT_Incremental_MetricsRec  metrics;


      metrics.bearing_x = decoder->builder.left_bearing.x;
      metrics.bearing_y = decoder->builder.left_bearing.y;
      metrics.advance   = decoder->builder.advance.x;
      error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                face->root.internal->incremental_interface->object,
                glyph_index, FALSE, &metrics );
      decoder->builder.left_bearing.x = metrics.bearing_x;
      decoder->builder.left_bearing.y = metrics.bearing_y;
      decoder->builder.advance.x      = metrics.advance;
      decoder->builder.advance.y      = 0;
    }

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

  Exit:
    return error;
  }


#if 0


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********            COMPUTE THE MAXIMUM ADVANCE WIDTH         *********/
  /**********                                                      *********/
  /**********    The following code is in charge of computing      *********/
  /**********    the maximum advance width of the font.  It        *********/
  /**********    quickly processes each glyph charstring to        *********/
  /**********    extract the value from either a `sbw' or `seac'   *********/
  /**********    operator.                                         *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  cid_face_compute_max_advance( CID_Face  face,
                                FT_Int*   max_advance )
  {
    FT_Error       error;
    T1_DecoderRec  decoder;
    FT_Int         glyph_index;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    *max_advance = 0;

    /* Initialize load decoder */
    error = psaux->t1_decoder_funcs->init( &decoder,
                                           (FT_Face)face,
                                           0, /* size       */
                                           0, /* glyph slot */
                                           0, /* glyph names! XXX */
                                           0, /* blend == 0 */
                                           0, /* hinting == 0 */
                                           cid_load_glyph );
    if ( error )
      return error;

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* for each glyph, parse the glyph charstring and extract */
    /* the advance width                                      */
    for ( glyph_index = 0; glyph_index < face->root.num_glyphs;
          glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = cid_load_glyph( &decoder, glyph_index );
      /* ignore the error if one occurred - skip to next glyph */
    }

    *max_advance = decoder.builder.advance.x;

    return CID_Err_Ok;
  }


#endif /* 0 */


  FT_LOCAL_DEF( FT_Error )
  cid_slot_load_glyph( FT_GlyphSlot  cidglyph,      /* CID_GlyphSlot */
                       FT_Size       cidsize,       /* CID_Size      */
                       FT_UInt       glyph_index,
                       FT_Int32      load_flags )
  {
    CID_GlyphSlot  glyph = (CID_GlyphSlot)cidglyph;
    CID_Size       size  = (CID_Size)cidsize;
    FT_Error       error;
    T1_DecoderRec  decoder;
    CID_Face       face = (CID_Face)cidglyph->face;
    FT_Bool        hinting;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;
    FT_Matrix      font_matrix;
    FT_Vector      font_offset;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = cidsize->metrics.x_scale;
    glyph->y_scale = cidsize->metrics.y_scale;

    cidglyph->outline.n_points   = 0;
    cidglyph->outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    cidglyph->format = FT_GLYPH_FORMAT_OUTLINE;

    {
      error = psaux->t1_decoder_funcs->init( &decoder,
                                             cidglyph->face,
                                             cidsize,
                                             cidglyph,
                                             0, /* glyph names -- XXX */
                                             0, /* blend == 0 */
                                             hinting,
                                             FT_LOAD_TARGET_MODE( load_flags ),
                                             cid_load_glyph );

      /* set up the decoder */
      decoder.builder.no_recurse = FT_BOOL(
        ( ( load_flags & FT_LOAD_NO_RECURSE ) != 0 ) );

      error = cid_load_glyph( &decoder, glyph_index );

      font_matrix = decoder.font_matrix;
      font_offset = decoder.font_offset;

      /* save new glyph tables */
      psaux->t1_decoder_funcs->done( &decoder );
    }

    /* now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax                                    */
    if ( !error )
    {
      cidglyph->outline.flags &= FT_OUTLINE_OWNER;
      cidglyph->outline.flags |= FT_OUTLINE_REVERSE_FILL;

      /* for composite glyphs, return only left side bearing and */
      /* advance width                                           */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_Slot_Internal  internal = cidglyph->internal;


        cidglyph->metrics.horiBearingX = decoder.builder.left_bearing.x;
        cidglyph->metrics.horiAdvance  = decoder.builder.advance.x;

        internal->glyph_matrix         = font_matrix;
        internal->glyph_delta          = font_offset;
        internal->glyph_transformed    = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &cidglyph->metrics;
        FT_Vector          advance;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance                  = decoder.builder.advance.x;
        cidglyph->linearHoriAdvance           = decoder.builder.advance.x;
        cidglyph->internal->glyph_transformed = 0;

        /* make up vertical ones */
        metrics->vertAdvance        = ( face->cid.font_bbox.yMax -
                                        face->cid.font_bbox.yMin ) >> 16;
        cidglyph->linearVertAdvance = metrics->vertAdvance;

        cidglyph->format            = FT_GLYPH_FORMAT_OUTLINE;

        if ( size && cidsize->metrics.y_ppem < 24 )
          cidglyph->outline.flags |= FT_OUTLINE_HIGH_PRECISION;

        /* apply the font matrix */
        FT_Outline_Transform( &cidglyph->outline, &font_matrix );

        FT_Outline_Translate( &cidglyph->outline,
                              font_offset.x,
                              font_offset.y );

        advance.x = metrics->horiAdvance;
        advance.y = 0;
        FT_Vector_Transform( &advance, &font_matrix );
        metrics->horiAdvance = advance.x + font_offset.x;
        advance.x = 0;
        advance.y = metrics->vertAdvance;
        FT_Vector_Transform( &advance, &font_matrix );
        metrics->vertAdvance = advance.y + font_offset.y;

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur = decoder.builder.base;
          FT_Vector*   vec = cur->points;
          FT_Fixed     x_scale = glyph->x_scale;
          FT_Fixed     y_scale = glyph->y_scale;


          /* First of all, scale the points */
          if ( !hinting || !decoder.builder.hints_funcs )
            for ( n = cur->n_points; n > 0; n--, vec++ )
            {
              vec->x = FT_MulFix( vec->x, x_scale );
              vec->y = FT_MulFix( vec->y, y_scale );
            }

          /* Then scale the metrics */
          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,  x_scale );
          metrics->vertAdvance  = FT_MulFix( metrics->vertAdvance,  y_scale );
        }

        /* compute the other metrics */
        FT_Outline_Get_CBox( &cidglyph->outline, &cbox );

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;

        /* make up vertical ones */
        ft_synthesize_vertical_metrics( metrics,
                                        metrics->vertAdvance );
      }
    }

    return error;
  }


/* END */



/* END */

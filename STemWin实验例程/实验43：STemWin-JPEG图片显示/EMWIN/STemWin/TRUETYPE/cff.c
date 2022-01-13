/***************************************************************************/
/*                                                                         */
/*  cff.c                                                                  */
/*                                                                         */
/*    FreeType OpenType driver component (body only).                      */
/*                                                                         */
/*  Copyright 1996-2001, 2002 by                                           */
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
/*  cffdrivr.c                                                             */
/*                                                                         */
/*    OpenType font driver implementation (body).                          */
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
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_TT_CMAP_H

#include "cffdrivr.h"
#include "cffgload.h"
#include "cffload.h"
#include "cffcmap.h"

#include "cfferrs.h"

#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_GLYPH_DICT_H

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdriver


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                          F A C E S                              ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_get_kerning                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to return the kerning vector between two      */
  /*    glyphs of the same face.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the source face object.                 */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    kerning     :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this function.  Other layouts, or more sophisticated  */
  /*    kernings, are out of scope of this method (the basic driver        */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  cff_get_kerning( FT_Face     ttface,          /* TT_Face */
                   FT_UInt     left_glyph,
                   FT_UInt     right_glyph,
                   FT_Vector*  kerning )
  {
    TT_Face       face = (TT_Face)ttface;
    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;


    kerning->x = 0;
    kerning->y = 0;

    if ( sfnt )
      kerning->x = sfnt->get_kerning( face, left_glyph, right_glyph );

    return CFF_Err_Ok;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Load_Glyph                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to load a glyph within a given glyph slot.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the target slot object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled, loaded, etc.                        */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_??? constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  Load_Glyph( FT_GlyphSlot  cffslot,        /* CFF_GlyphSlot */
              FT_Size       cffsize,        /* CFF_Size      */
              FT_UInt       glyph_index,
              FT_Int32      load_flags )
  {
    FT_Error  error;
    CFF_GlyphSlot  slot = (CFF_GlyphSlot)cffslot;
    CFF_Size       size = (CFF_Size)cffsize;


    if ( !slot )
      return CFF_Err_Invalid_Slot_Handle;

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    /* reset the size object if necessary */
    if ( size )
    {
      /* these two objects must have the same parent */
      if ( cffsize->face != cffslot->face )
        return CFF_Err_Invalid_Face_Handle;
    }

    /* now load the glyph outline if necessary */
    error = cff_slot_load( slot, size, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


 /*
  *  GLYPH DICT SERVICE
  *
  */

  static FT_Error
  cff_get_glyph_name( CFF_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    CFF_Font            font   = (CFF_Font)face->extra.data;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          gname;
    FT_UShort           sid;
    FT_Service_PsCMaps  psnames;
    FT_Error            error;


    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
    {
      FT_ERROR(( "cff_get_glyph_name:" ));
      FT_ERROR(( " cannot get glyph name from CFF & CEF fonts\n" ));
      FT_ERROR(( "                   " ));
      FT_ERROR(( " without the `PSNames' module\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    /* first, locate the sid in the charset table */
    sid = font->charset.sids[glyph_index];

    /* now, lookup the name itself */
    gname = cff_index_get_sid_string( &font->string_index, sid, psnames );

    if ( gname && buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)ft_strlen( gname );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    FT_FREE( gname );
    error = CFF_Err_Ok;

    Exit:
      return error;
  }


  static FT_UInt
  cff_get_name_index( CFF_Face    face,
                      FT_String*  glyph_name )
  {
    CFF_Font            cff;
    CFF_Charset         charset;
    FT_Service_PsCMaps  psnames;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          name;
    FT_UShort           sid;
    FT_UInt             i;
    FT_Int              result;


    cff     = (CFF_FontRec *)face->extra.data;
    charset = &cff->charset;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
      return 0;

    for ( i = 0; i < cff->num_glyphs; i++ )
    {
      sid = charset->sids[i];

      if ( sid > 390 )
        name = cff_index_get_name( &cff->string_index, sid - 391 );
      else
        name = (FT_String *)psnames->adobe_std_strings( sid );

      result = ft_strcmp( glyph_name, name );

      if ( sid > 390 )
        FT_FREE( name );

      if ( !result )
        return i;
    }

    return 0;
  }


  static const FT_Service_GlyphDictRec  cff_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  cff_get_glyph_name,
    (FT_GlyphDict_NameIndexFunc)cff_get_name_index,
  };


 /*
  *  POSTSCRIPT INFO SERVICE
  *
  */

  static FT_Int
  cff_ps_has_glyph_names( FT_Face  face )
  {
    return ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES ) > 0;
  }


  static const FT_Service_PsInfoRec  cff_service_ps_info =
  {
    (PS_GetFontInfoFunc)   NULL,        /* unsupported with CFF fonts */
    (PS_HasGlyphNamesFunc) cff_ps_has_glyph_names,
    (PS_GetFontPrivateFunc)NULL         /* unsupported with CFF fonts */
  };


  /*
   * TT CMAP INFO
   *
   * If the charmap is a synthetic Unicode encoding cmap or
   * a Type 1 standard (or expert) encoding cmap, hide TT CMAP INFO
   * service defined in SFNT module.
   *
   * Otherwise call the service function in the sfnt module.
   *
   */
  static FT_Error
  cff_get_cmap_info( FT_CharMap    charmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_CMap   cmap  = FT_CMAP( charmap );
    FT_Error  error = CFF_Err_Ok;


    cmap_info->language = 0;

    if ( cmap->clazz != &cff_cmap_encoding_class_rec &&
         cmap->clazz != &cff_cmap_unicode_class_rec  )
    {
      FT_Face             face    = FT_CMAP_FACE( cmap );
      FT_Library          library = FT_FACE_LIBRARY( face );
      FT_Module           sfnt    = FT_Get_Module( library, "sfnt" );
      FT_Service_TTCMaps  service =
        (FT_Service_TTCMaps)ft_module_get_service( sfnt,
                                                   FT_SERVICE_ID_TT_CMAP );


      if ( service && service->get_cmap_info )
        error = service->get_cmap_info( charmap, cmap_info );
    }

    return error;
  }


  static const FT_Service_TTCMapsRec  cff_service_get_cmap_info =
  {
    (TT_CMap_Info_GetFunc)cff_get_cmap_info
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                D R I V E R  I N T E R F A C E                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static const FT_ServiceDescRec  cff_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME,       FT_XF86_FORMAT_CFF },
    { FT_SERVICE_ID_POSTSCRIPT_INFO, &cff_service_ps_info },
#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
    { FT_SERVICE_ID_GLYPH_DICT,      &cff_service_glyph_dict },
#endif
    { FT_SERVICE_ID_TT_CMAP,         &cff_service_get_cmap_info },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cff_get_interface( FT_Module    driver,       /* CFF_Driver */
                     const char*  module_interface )
  {
    FT_Module            sfnt;
    FT_Module_Interface  result;


    result = ft_service_list_lookup( cff_services, module_interface );
    if ( result != NULL )
      return  result;

    /* we pass our request to the `sfnt' module */
    sfnt = FT_Get_Module( driver->library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, module_interface ) : 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  cff_driver_class =
  {
    /* begin with the FT_Module_Class fields */
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof( CFF_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      0,   /* module-specific interface */

      cff_driver_init,
      cff_driver_done,
      cff_get_interface,
    },

    /* now the specific driver fields */
    sizeof( TT_FaceRec ),
    sizeof( CFF_SizeRec ),
    sizeof( CFF_GlyphSlotRec ),

    cff_face_init,
    cff_face_done,
    cff_size_init,
    cff_size_done,
    cff_slot_init,
    cff_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif

    Load_Glyph,

    cff_get_kerning,
    0,                      /* FT_Face_AttachFunc      */
    0,                      /* FT_Face_GetAdvancesFunc */

    cff_size_request,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    cff_size_select
#else
    0                       /* FT_Size_SelectFunc      */
#endif
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  cffparse.c                                                             */
/*                                                                         */
/*    CFF token stream parser (body)                                       */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004 by                               */
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
#include "cffparse.h"
#include FT_INTERNAL_STREAM_H

#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffparse


  enum
  {
    cff_kind_none = 0,
    cff_kind_num,
    cff_kind_fixed,
    cff_kind_fixed_thousand,
    cff_kind_string,
    cff_kind_bool,
    cff_kind_delta,
    cff_kind_callback,

    cff_kind_max  /* do not remove */
  };


  /* now generate handlers for the most simple fields */
  typedef FT_Error  (*CFF_Field_Reader)( CFF_Parser  parser );

  typedef struct  CFF_Field_Handler_
  {
    int               kind;
    int               code;
    FT_UInt           offset;
    FT_Byte           size;
    CFF_Field_Reader  reader;
    FT_UInt           array_max;
    FT_UInt           count_offset;

  } CFF_Field_Handler;


  FT_LOCAL_DEF( void )
  cff_parser_init( CFF_Parser  parser,
                   FT_UInt     code,
                   void*       object )
  {
    FT_MEM_ZERO( parser, sizeof ( *parser ) );

    parser->top         = parser->stack;
    parser->object_code = code;
    parser->object      = object;
  }


  /* read an integer */
  static FT_Long
  cff_parse_integer( FT_Byte*  start,
                     FT_Byte*  limit )
  {
    FT_Byte*  p   = start;
    FT_Int    v   = *p++;
    FT_Long   val = 0;


    if ( v == 28 )
    {
      if ( p + 2 > limit )
        goto Bad;

      val = (FT_Short)( ( (FT_Int)p[0] << 8 ) | p[1] );
      p  += 2;
    }
    else if ( v == 29 )
    {
      if ( p + 4 > limit )
        goto Bad;

      val = ( (FT_Long)p[0] << 24 ) |
            ( (FT_Long)p[1] << 16 ) |
            ( (FT_Long)p[2] <<  8 ) |
                       p[3];
      p += 4;
    }
    else if ( v < 247 )
    {
      val = v - 139;
    }
    else if ( v < 251 )
    {
      if ( p + 1 > limit )
        goto Bad;

      val = ( v - 247 ) * 256 + p[0] + 108;
      p++;
    }
    else
    {
      if ( p + 1 > limit )
        goto Bad;

      val = -( v - 251 ) * 256 - p[0] - 108;
      p++;
    }

  Exit:
    return val;

  Bad:
    val = 0;
    goto Exit;
  }


  /* read a real */
  static FT_Fixed
  cff_parse_real( FT_Byte*  start,
                  FT_Byte*  limit,
                  FT_Int    power_ten )
  {
    FT_Byte*  p    = start;
    FT_Long   num, divider, result, exponent;
    FT_Int    sign = 0, exponent_sign = 0;
    FT_UInt   nib;
    FT_UInt   phase;


    result  = 0;
    num     = 0;
    divider = 1;

    /* first of all, read the integer part */
    phase = 4;

    for (;;)
    {
      /* If we entered this iteration with phase == 4, we need to */
      /* read a new byte.  This also skips past the intial 0x1E.  */
      if ( phase )
      {
        p++;

        /* Make sure we don't read past the end. */
        if ( p >= limit )
          goto Bad;
      }

      /* Get the nibble. */
      nib   = ( p[0] >> phase ) & 0xF;
      phase = 4 - phase;

      if ( nib == 0xE )
        sign = 1;
      else if ( nib > 9 )
        break;
      else
        result = result * 10 + nib;
    }

    /* read decimal part, if any */
    if ( nib == 0xa )
      for (;;)
      {
        /* If we entered this iteration with phase == 4, we need */
        /* to read a new byte.                                   */
        if ( phase )
        {
          p++;

          /* Make sure we don't read past the end. */
          if ( p >= limit )
            goto Bad;
        }

        /* Get the nibble. */
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        if ( divider < 10000000L )
        {
          num      = num * 10 + nib;
          divider *= 10;
        }
      }

    /* read exponent, if any */
    if ( nib == 12 )
    {
      exponent_sign = 1;
      nib           = 11;
    }

    if ( nib == 11 )
    {
      exponent = 0;

      for (;;)
      {
        /* If we entered this iteration with phase == 4, we need */
        /* to read a new byte.                                   */
        if ( phase )
        {
          p++;

          /* Make sure we don't read past the end. */
          if ( p >= limit )
            goto Bad;
        }

        /* Get the nibble. */
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        exponent = exponent * 10 + nib;
      }

      if ( exponent_sign )
        exponent = -exponent;

      power_ten += (FT_Int)exponent;
    }

    /* raise to power of ten if needed */
    while ( power_ten > 0 )
    {
      result = result * 10;
      num    = num * 10;

      power_ten--;
    }

    while ( power_ten < 0 )
    {
      result  = result / 10;
      divider = divider * 10;

      power_ten++;
    }

    /* Move the integer part into the high 16 bits. */
    result <<= 16;

    /* Place the decimal part into the low 16 bits. */
    if ( num )
      result |= FT_DivFix( num, divider );

    if ( sign )
      result = -result;

  Exit:
    return result;

  Bad:
    result = 0;
    goto Exit;
  }


  /* read a number, either integer or real */
  static FT_Long
  cff_parse_num( FT_Byte**  d )
  {
    return ( **d == 30 ? ( cff_parse_real   ( d[0], d[1], 0 ) >> 16 )
                       :   cff_parse_integer( d[0], d[1] ) );
  }


  /* read a floating point number, either integer or real */
  static FT_Fixed
  cff_parse_fixed( FT_Byte**  d )
  {
    return ( **d == 30 ? cff_parse_real   ( d[0], d[1], 0 )
                       : cff_parse_integer( d[0], d[1] ) << 16 );
  }

  /* read a floating point number, either integer or real, */
  /* but return 1000 times the number read in.             */
  static FT_Fixed
  cff_parse_fixed_thousand( FT_Byte**  d )
  {
    return **d ==
      30 ? cff_parse_real     ( d[0], d[1], 3 )
         : (FT_Fixed)FT_MulFix( cff_parse_integer( d[0], d[1] ) << 16, 1000 );
  }

  static FT_Error
  cff_parse_font_matrix( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict   = (CFF_FontRecDict)parser->object;
    FT_Matrix*       matrix = &dict->font_matrix;
    FT_Vector*       offset = &dict->font_offset;
    FT_UShort*       upm    = &dict->units_per_em;
    FT_Byte**        data   = parser->stack;
    FT_Error         error;
    FT_Fixed         temp;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 6 )
    {
      matrix->xx = cff_parse_fixed_thousand( data++ );
      matrix->yx = cff_parse_fixed_thousand( data++ );
      matrix->xy = cff_parse_fixed_thousand( data++ );
      matrix->yy = cff_parse_fixed_thousand( data++ );
      offset->x  = cff_parse_fixed_thousand( data++ );
      offset->y  = cff_parse_fixed_thousand( data   );

      temp = FT_ABS( matrix->yy );

      *upm = (FT_UShort)FT_DivFix( 0x10000L, FT_DivFix( temp, 1000 ) );

      if ( temp != 0x10000L )
      {
        matrix->xx = FT_DivFix( matrix->xx, temp );
        matrix->yx = FT_DivFix( matrix->yx, temp );
        matrix->xy = FT_DivFix( matrix->xy, temp );
        matrix->yy = FT_DivFix( matrix->yy, temp );
        offset->x  = FT_DivFix( offset->x,  temp );
        offset->y  = FT_DivFix( offset->y,  temp );
      }

      /* note that the offsets must be expressed in integer font units */
      offset->x >>= 16;
      offset->y >>= 16;

      error = CFF_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_font_bbox( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_BBox*         bbox = &dict->font_bbox;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 4 )
    {
      bbox->xMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->xMax = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMax = FT_RoundFix( cff_parse_fixed( data   ) );
      error = CFF_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_private_dict( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 2 )
    {
      dict->private_size   = cff_parse_num( data++ );
      dict->private_offset = cff_parse_num( data   );
      error = CFF_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_cid_ros( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 3 )
    {
      dict->cid_registry   = (FT_UInt)cff_parse_num ( data++ );
      dict->cid_ordering   = (FT_UInt)cff_parse_num ( data++ );
      dict->cid_supplement = (FT_ULong)cff_parse_num( data );
      error = CFF_Err_Ok;
    }

    return error;
  }


#define CFF_FIELD_NUM( code, name ) \
          CFF_FIELD( code, name, cff_kind_num )
#define CFF_FIELD_FIXED( code, name ) \
          CFF_FIELD( code, name, cff_kind_fixed )
#define CFF_FIELD_FIXED_1000( code, name ) \
          CFF_FIELD( code, name, cff_kind_fixed_thousand )
#define CFF_FIELD_STRING( code, name ) \
          CFF_FIELD( code, name, cff_kind_string )
#define CFF_FIELD_BOOL( code, name ) \
          CFF_FIELD( code, name, cff_kind_bool )
#define CFF_FIELD_DELTA( code, name, max ) \
          CFF_FIELD( code, name, cff_kind_delta )

#define CFF_FIELD_CALLBACK( code, name ) \
          {                              \
            cff_kind_callback,           \
            code | CFFCODE,              \
            0, 0,                        \
            cff_parse_ ## name,          \
            0, 0                         \
          },

#undef  CFF_FIELD
#define CFF_FIELD( code, name, kind ) \
          {                          \
            kind,                    \
            code | CFFCODE,          \
            FT_FIELD_OFFSET( name ), \
            FT_FIELD_SIZE( name ),   \
            0, 0, 0                  \
          },

#undef  CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code, name, max ) \
        {                                  \
          cff_kind_delta,                  \
          code | CFFCODE,                  \
          FT_FIELD_OFFSET( name ),         \
          FT_FIELD_SIZE_DELTA( name ),     \
          0,                               \
          max,                             \
          FT_FIELD_OFFSET( num_ ## name )  \
        },

#define CFFCODE_TOPDICT  0x1000
#define CFFCODE_PRIVATE  0x2000

  static const CFF_Field_Handler  cff_field_handlers[] =
  {

#include "cfftoken.h"

    { 0, 0, 0, 0, 0, 0, 0 }
  };


  FT_LOCAL_DEF( FT_Error )
  cff_parser_run( CFF_Parser  parser,
                  FT_Byte*    start,
                  FT_Byte*    limit )
  {
    FT_Byte*  p     = start;
    FT_Error  error = CFF_Err_Ok;


    parser->top    = parser->stack;
    parser->start  = start;
    parser->limit  = limit;
    parser->cursor = start;

    while ( p < limit )
    {
      FT_UInt  v = *p;


      if ( v >= 27 && v != 31 )
      {
        /* it's a number; we will push its position on the stack */
        if ( parser->top - parser->stack >= CFF_MAX_STACK_DEPTH )
          goto Stack_Overflow;

        *parser->top ++ = p;

        /* now, skip it */
        if ( v == 30 )
        {
          /* skip real number */
          p++;
          for (;;)
          {
            if ( p >= limit )
              goto Syntax_Error;
            v = p[0] >> 4;
            if ( v == 15 )
              break;
            v = p[0] & 0xF;
            if ( v == 15 )
              break;
            p++;
          }
        }
        else if ( v == 28 )
          p += 2;
        else if ( v == 29 )
          p += 4;
        else if ( v > 246 )
          p += 1;
      }
      else
      {
        /* This is not a number, hence it's an operator.  Compute its code */
        /* and look for it in our current list.                            */

        FT_UInt                   code;
        FT_UInt                   num_args = (FT_UInt)
                                             ( parser->top - parser->stack );
        const CFF_Field_Handler*  field;


        *parser->top = p;
        code = v;
        if ( v == 12 )
        {
          /* two byte operator */
          p++;
          if ( p >= limit )
            goto Syntax_Error;

          code = 0x100 | p[0];
        }
        code = code | parser->object_code;

        for ( field = cff_field_handlers; field->kind; field++ )
        {
          if ( field->code == (FT_Int)code )
          {
            /* we found our field's handler; read it */
            FT_Long   val;
            FT_Byte*  q = (FT_Byte*)parser->object + field->offset;


            /* check that we have enough arguments -- except for */
            /* delta encoded arrays, which can be empty          */
            if ( field->kind != cff_kind_delta && num_args < 1 )
              goto Stack_Underflow;

            switch ( field->kind )
            {
            case cff_kind_bool:
            case cff_kind_string:
            case cff_kind_num:
              val = cff_parse_num( parser->stack );
              goto Store_Number;

            case cff_kind_fixed:
              val = cff_parse_fixed( parser->stack );
              goto Store_Number;

            case cff_kind_fixed_thousand:
              val = cff_parse_fixed_thousand( parser->stack );

            Store_Number:
              switch ( field->size )
              {
              case (8 / FT_CHAR_BIT):
                *(FT_Byte*)q = (FT_Byte)val;
                break;

              case (16 / FT_CHAR_BIT):
                *(FT_Short*)q = (FT_Short)val;
                break;

              case (32 / FT_CHAR_BIT):
                *(FT_Int32*)q = (FT_Int)val;
                break;

              default:  /* for 64-bit systems */
                *(FT_Long*)q = val;
              }
              break;

            case cff_kind_delta:
              {
                FT_Byte*   qcount = (FT_Byte*)parser->object +
                                      field->count_offset;

                FT_Byte**  data = parser->stack;


                if ( num_args > field->array_max )
                  num_args = field->array_max;

                /* store count */
                *qcount = (FT_Byte)num_args;

                val = 0;
                while ( num_args > 0 )
                {
                  val += cff_parse_num( data++ );
                  switch ( field->size )
                  {
                  case (8 / FT_CHAR_BIT):
                    *(FT_Byte*)q = (FT_Byte)val;
                    break;

                  case (16 / FT_CHAR_BIT):
                    *(FT_Short*)q = (FT_Short)val;
                    break;

                  case (32 / FT_CHAR_BIT):
                    *(FT_Int32*)q = (FT_Int)val;
                    break;

                  default:  /* for 64-bit systems */
                    *(FT_Long*)q = val;
                  }

                  q += field->size;
                  num_args--;
                }
              }
              break;

            default:  /* callback */
              error = field->reader( parser );
              if ( error )
                goto Exit;
            }
            goto Found;
          }
        }

        /* this is an unknown operator, or it is unsupported; */
        /* we will ignore it for now.                         */

      Found:
        /* clear stack */
        parser->top = parser->stack;
      }
      p++;
    }

  Exit:
    return error;

  Stack_Overflow:
    error = CFF_Err_Invalid_Argument;
    goto Exit;

  Stack_Underflow:
    error = CFF_Err_Invalid_Argument;
    goto Exit;

  Syntax_Error:
    error = CFF_Err_Invalid_Argument;
    goto Exit;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cffload.c                                                              */
/*                                                                         */
/*    OpenType and CFF data/program tables loader (body).                  */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_TRUETYPE_TAGS_H

#include "cffload.h"
#include "cffparse.h"

#include "cfferrs.h"


#if 1
  static const FT_UShort  cff_isoadobe_charset[229] =
  {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    78,
    79,
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    90,
    91,
    92,
    93,
    94,
    95,
    96,
    97,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
    117,
    118,
    119,
    120,
    121,
    122,
    123,
    124,
    125,
    126,
    127,
    128,
    129,
    130,
    131,
    132,
    133,
    134,
    135,
    136,
    137,
    138,
    139,
    140,
    141,
    142,
    143,
    144,
    145,
    146,
    147,
    148,
    149,
    150,
    151,
    152,
    153,
    154,
    155,
    156,
    157,
    158,
    159,
    160,
    161,
    162,
    163,
    164,
    165,
    166,
    167,
    168,
    169,
    170,
    171,
    172,
    173,
    174,
    175,
    176,
    177,
    178,
    179,
    180,
    181,
    182,
    183,
    184,
    185,
    186,
    187,
    188,
    189,
    190,
    191,
    192,
    193,
    194,
    195,
    196,
    197,
    198,
    199,
    200,
    201,
    202,
    203,
    204,
    205,
    206,
    207,
    208,
    209,
    210,
    211,
    212,
    213,
    214,
    215,
    216,
    217,
    218,
    219,
    220,
    221,
    222,
    223,
    224,
    225,
    226,
    227,
    228
  };

  static const FT_UShort  cff_expert_charset[166] =
  {
    0,
    1,
    229,
    230,
    231,
    232,
    233,
    234,
    235,
    236,
    237,
    238,
    13,
    14,
    15,
    99,
    239,
    240,
    241,
    242,
    243,
    244,
    245,
    246,
    247,
    248,
    27,
    28,
    249,
    250,
    251,
    252,
    253,
    254,
    255,
    256,
    257,
    258,
    259,
    260,
    261,
    262,
    263,
    264,
    265,
    266,
    109,
    110,
    267,
    268,
    269,
    270,
    271,
    272,
    273,
    274,
    275,
    276,
    277,
    278,
    279,
    280,
    281,
    282,
    283,
    284,
    285,
    286,
    287,
    288,
    289,
    290,
    291,
    292,
    293,
    294,
    295,
    296,
    297,
    298,
    299,
    300,
    301,
    302,
    303,
    304,
    305,
    306,
    307,
    308,
    309,
    310,
    311,
    312,
    313,
    314,
    315,
    316,
    317,
    318,
    158,
    155,
    163,
    319,
    320,
    321,
    322,
    323,
    324,
    325,
    326,
    150,
    164,
    169,
    327,
    328,
    329,
    330,
    331,
    332,
    333,
    334,
    335,
    336,
    337,
    338,
    339,
    340,
    341,
    342,
    343,
    344,
    345,
    346,
    347,
    348,
    349,
    350,
    351,
    352,
    353,
    354,
    355,
    356,
    357,
    358,
    359,
    360,
    361,
    362,
    363,
    364,
    365,
    366,
    367,
    368,
    369,
    370,
    371,
    372,
    373,
    374,
    375,
    376,
    377,
    378
  };

  static const FT_UShort  cff_expertsubset_charset[87] =
  {
    0,
    1,
    231,
    232,
    235,
    236,
    237,
    238,
    13,
    14,
    15,
    99,
    239,
    240,
    241,
    242,
    243,
    244,
    245,
    246,
    247,
    248,
    27,
    28,
    249,
    250,
    251,
    253,
    254,
    255,
    256,
    257,
    258,
    259,
    260,
    261,
    262,
    263,
    264,
    265,
    266,
    109,
    110,
    267,
    268,
    269,
    270,
    272,
    300,
    301,
    302,
    305,
    314,
    315,
    158,
    155,
    163,
    320,
    321,
    322,
    323,
    324,
    325,
    326,
    150,
    164,
    169,
    327,
    328,
    329,
    330,
    331,
    332,
    333,
    334,
    335,
    336,
    337,
    338,
    339,
    340,
    341,
    342,
    343,
    344,
    345,
    346
  };

  static const FT_UShort  cff_standard_encoding[256] =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    78,
    79,
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    90,
    91,
    92,
    93,
    94,
    95,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    96,
    97,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
    0,
    111,
    112,
    113,
    114,
    0,
    115,
    116,
    117,
    118,
    119,
    120,
    121,
    122,
    0,
    123,
    0,
    124,
    125,
    126,
    127,
    128,
    129,
    130,
    131,
    0,
    132,
    133,
    0,
    134,
    135,
    136,
    137,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    138,
    0,
    139,
    0,
    0,
    0,
    0,
    140,
    141,
    142,
    143,
    0,
    0,
    0,
    0,
    0,
    144,
    0,
    0,
    0,
    145,
    0,
    0,
    146,
    147,
    148,
    149,
    0,
    0,
    0,
    0
  };

  static const FT_UShort  cff_expert_encoding[256] =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    229,
    230,
    0,
    231,
    232,
    233,
    234,
    235,
    236,
    237,
    238,
    13,
    14,
    15,
    99,
    239,
    240,
    241,
    242,
    243,
    244,
    245,
    246,
    247,
    248,
    27,
    28,
    249,
    250,
    251,
    252,
    0,
    253,
    254,
    255,
    256,
    257,
    0,
    0,
    0,
    258,
    0,
    0,
    259,
    260,
    261,
    262,
    0,
    0,
    263,
    264,
    265,
    0,
    266,
    109,
    110,
    267,
    268,
    269,
    0,
    270,
    271,
    272,
    273,
    274,
    275,
    276,
    277,
    278,
    279,
    280,
    281,
    282,
    283,
    284,
    285,
    286,
    287,
    288,
    289,
    290,
    291,
    292,
    293,
    294,
    295,
    296,
    297,
    298,
    299,
    300,
    301,
    302,
    303,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    304,
    305,
    306,
    0,
    0,
    307,
    308,
    309,
    310,
    311,
    0,
    312,
    0,
    0,
    312,
    0,
    0,
    314,
    315,
    0,
    0,
    316,
    317,
    318,
    0,
    0,
    0,
    158,
    155,
    163,
    319,
    320,
    321,
    322,
    323,
    324,
    325,
    0,
    0,
    326,
    150,
    164,
    169,
    327,
    328,
    329,
    330,
    331,
    332,
    333,
    334,
    335,
    336,
    337,
    338,
    339,
    340,
    341,
    342,
    343,
    344,
    345,
    346,
    347,
    348,
    349,
    350,
    351,
    352,
    353,
    354,
    355,
    356,
    357,
    358,
    359,
    360,
    361,
    362,
    363,
    364,
    365,
    366,
    367,
    368,
    369,
    370,
    371,
    372,
    373,
    374,
    375,
    376,
    377,
    378
  };
#endif


  FT_LOCAL_DEF( FT_UShort )
  cff_get_standard_encoding( FT_UInt  charcode )
  {
    return  (FT_UShort)(charcode < 256 ? cff_standard_encoding[charcode] : 0);
  }


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffload


  /* read a CFF offset from memory */
  static FT_ULong
  cff_get_offset( FT_Byte*  p,
                  FT_Byte   off_size )
  {
    FT_ULong  result;


    for ( result = 0; off_size > 0; off_size-- )
    {
      result <<= 8;
      result  |= *p++;
    }

    return result;
  }


  static FT_Error
  cff_new_index( CFF_Index  idx,
                 FT_Stream  stream,
                 FT_Bool    load )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_UShort  count;


    FT_MEM_ZERO( idx, sizeof ( *idx ) );

    idx->stream = stream;
    if ( !FT_READ_USHORT( count ) &&
         count > 0                )
    {
      FT_Byte*   p;
      FT_Byte    offsize;
      FT_ULong   data_size;
      FT_ULong*  poff;


      /* there is at least one element; read the offset size,           */
      /* then access the offset table to compute the index's total size */
      if ( FT_READ_BYTE( offsize ) )
        goto Exit;

      idx->stream   = stream;
      idx->count    = count;
      idx->off_size = offsize;
      data_size     = (FT_ULong)( count + 1 ) * offsize;

      if ( FT_NEW_ARRAY( idx->offsets, count + 1 ) ||
           FT_FRAME_ENTER( data_size )             )
        goto Exit;

      poff = idx->offsets;
      p    = (FT_Byte*)stream->cursor;

      for ( ; (FT_Short)count >= 0; count-- )
      {
        poff[0] = cff_get_offset( p, offsize );
        poff++;
        p += offsize;
      }

      FT_FRAME_EXIT();

      idx->data_offset = FT_STREAM_POS();
      data_size        = poff[-1] - 1;

      if ( load )
      {
        /* load the data */
        if ( FT_FRAME_EXTRACT( data_size, idx->bytes ) )
          goto Exit;
      }
      else
      {
        /* skip the data */
        if ( FT_STREAM_SKIP( data_size ) )
          goto Exit;
      }
    }

  Exit:
    if ( error )
      FT_FREE( idx->offsets );

    return error;
  }


  static void
  cff_done_index( CFF_Index  idx )
  {
    if ( idx->stream )
    {
      FT_Stream  stream = idx->stream;
      FT_Memory  memory = stream->memory;


      if ( idx->bytes )
        FT_FRAME_RELEASE( idx->bytes );

      FT_FREE( idx->offsets );
      FT_MEM_ZERO( idx, sizeof ( *idx ) );
    }
  }


  /* allocate a table containing pointers to an index's elements */
  static FT_Error
  cff_index_get_pointers( CFF_Index   idx,
                          FT_Byte***  table )
  {
    FT_Error   error  = CFF_Err_Ok;
    FT_Memory  memory = idx->stream->memory;
    FT_ULong   n, offset, old_offset;
    FT_Byte**  t;


    *table = 0;

    if ( idx->count > 0 && !FT_NEW_ARRAY( t, idx->count + 1 ) )
    {
      old_offset = 1;
      for ( n = 0; n <= idx->count; n++ )
      {
        offset = idx->offsets[n];
        if ( !offset )
          offset = old_offset;

        t[n] = idx->bytes + offset - 1;

        old_offset = offset;
      }
      *table = t;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_index_access_element( CFF_Index  idx,
                            FT_UInt    element,
                            FT_Byte**  pbytes,
                            FT_ULong*  pbyte_len )
  {
    FT_Error  error = CFF_Err_Ok;


    if ( idx && idx->count > element )
    {
      /* compute start and end offsets */
      FT_ULong  off1, off2 = 0;


      off1 = idx->offsets[element];
      if ( off1 )
      {
        do
        {
          element++;
          off2 = idx->offsets[element];

        } while ( off2 == 0 && element < idx->count );

        if ( !off2 )
          off1 = 0;
      }

      /* access element */
      if ( off1 && off2 > off1 )
      {
        *pbyte_len = off2 - off1;

        if ( idx->bytes )
        {
          /* this index was completely loaded in memory, that's easy */
          *pbytes = idx->bytes + off1 - 1;
        }
        else
        {
          /* this index is still on disk/file, access it through a frame */
          FT_Stream  stream = idx->stream;


          if ( FT_STREAM_SEEK( idx->data_offset + off1 - 1 ) ||
               FT_FRAME_EXTRACT( off2 - off1, *pbytes )      )
            goto Exit;
        }
      }
      else
      {
        /* empty index element */
        *pbytes    = 0;
        *pbyte_len = 0;
      }
    }
    else
      error = CFF_Err_Invalid_Argument;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  cff_index_forget_element( CFF_Index  idx,
                            FT_Byte**  pbytes )
  {
    if ( idx->bytes == 0 )
    {
      FT_Stream  stream = idx->stream;


      FT_FRAME_RELEASE( *pbytes );
    }
  }


  FT_LOCAL_DEF( FT_String* )
  cff_index_get_name( CFF_Index  idx,
                      FT_UInt    element )
  {
    FT_Memory   memory = idx->stream->memory;
    FT_Byte*    bytes;
    FT_ULong    byte_len;
    FT_Error    error;
    FT_String*  name = 0;


    error = cff_index_access_element( idx, element, &bytes, &byte_len );
    if ( error )
      goto Exit;

    if ( !FT_ALLOC( name, byte_len + 1 ) )
    {
      FT_MEM_COPY( name, bytes, byte_len );
      name[byte_len] = 0;
    }
    cff_index_forget_element( idx, &bytes );

  Exit:
    return name;
  }


  FT_LOCAL_DEF( FT_String* )
  cff_index_get_sid_string( CFF_Index           idx,
                            FT_UInt             sid,
                            FT_Service_PsCMaps  psnames )
  {
    /* value 0xFFFFU indicates a missing dictionary entry */
    if ( sid == 0xFFFFU )
      return 0;

    /* if it is not a standard string, return it */
    if ( sid > 390 )
      return cff_index_get_name( idx, sid - 391 );

    /* CID-keyed CFF fonts don't have glyph names */
    if ( !psnames )
      return 0;

    /* that's a standard string, fetch a copy from the PSName module */
    {
      FT_String*   name       = 0;
      const char*  adobe_name = psnames->adobe_std_strings( sid );
      FT_UInt      len;


      if ( adobe_name )
      {
        FT_Memory  memory = idx->stream->memory;
        FT_Error   error;


        len = (FT_UInt)ft_strlen( adobe_name );
        if ( !FT_ALLOC( name, len + 1 ) )
        {
          FT_MEM_COPY( name, adobe_name, len );
          name[len] = 0;
        }

        FT_UNUSED( error );
      }

      return name;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***   FD Select table support                                         ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/


  static void
  CFF_Done_FD_Select( CFF_FDSelect  fdselect,
                      FT_Stream     stream )
  {
    if ( fdselect->data )
      FT_FRAME_RELEASE( fdselect->data );

    fdselect->data_size   = 0;
    fdselect->format      = 0;
    fdselect->range_count = 0;
  }


  static FT_Error
  CFF_Load_FD_Select( CFF_FDSelect  fdselect,
                      FT_UInt       num_glyphs,
                      FT_Stream     stream,
                      FT_ULong      offset )
  {
    FT_Error  error;
    FT_Byte   format;
    FT_UInt   num_ranges;


    /* read format */
    if ( FT_STREAM_SEEK( offset ) || FT_READ_BYTE( format ) )
      goto Exit;

    fdselect->format      = format;
    fdselect->cache_count = 0;   /* clear cache */

    switch ( format )
    {
    case 0:     /* format 0, that's simple */
      fdselect->data_size = num_glyphs;
      goto Load_Data;

    case 3:     /* format 3, a tad more complex */
      if ( FT_READ_USHORT( num_ranges ) )
        goto Exit;

      fdselect->data_size = num_ranges * 3 + 2;

    Load_Data:
      if ( FT_FRAME_EXTRACT( fdselect->data_size, fdselect->data ) )
        goto Exit;
      break;

    default:    /* hmm... that's wrong */
      error = CFF_Err_Invalid_File_Format;
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Byte )
  cff_fd_select_get( CFF_FDSelect  fdselect,
                     FT_UInt       glyph_index )
  {
    FT_Byte  fd = 0;


    switch ( fdselect->format )
    {
    case 0:
      fd = fdselect->data[glyph_index];
      break;

    case 3:
      /* first, compare to cache */
      if ( (FT_UInt)( glyph_index - fdselect->cache_first ) <
                        fdselect->cache_count )
      {
        fd = fdselect->cache_fd;
        break;
      }

      /* then, lookup the ranges array */
      {
        FT_Byte*  p       = fdselect->data;
        FT_Byte*  p_limit = p + fdselect->data_size;
        FT_Byte   fd2;
        FT_UInt   first, limit;


        first = FT_NEXT_USHORT( p );
        do
        {
          if ( glyph_index < first )
            break;

          fd2   = *p++;
          limit = FT_NEXT_USHORT( p );

          if ( glyph_index < limit )
          {
            fd = fd2;

            /* update cache */
            fdselect->cache_first = first;
            fdselect->cache_count = limit-first;
            fdselect->cache_fd    = fd2;
            break;
          }
          first = limit;

        } while ( p < p_limit );
      }
      break;

    default:
      ;
    }

    return fd;
  }


  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***   CFF font support                                                ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/

  static void
  cff_charset_done( CFF_Charset  charset,
                    FT_Stream    stream )
  {
    FT_Memory  memory = stream->memory;


    FT_FREE( charset->sids );
    FT_FREE( charset->cids );
    charset->format = 0;
    charset->offset = 0;
  }


  static FT_Error
  cff_charset_load( CFF_Charset  charset,
                    FT_UInt      num_glyphs,
                    FT_Stream    stream,
                    FT_ULong     base_offset,
                    FT_ULong     offset,
                    FT_Bool      invert )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error  = CFF_Err_Ok;
    FT_UShort  glyph_sid;


    /* If the the offset is greater than 2, we have to parse the */
    /* charset table.                                            */
    if ( offset > 2 )
    {
      FT_UInt  j;


      charset->offset = base_offset + offset;

      /* Get the format of the table. */
      if ( FT_STREAM_SEEK( charset->offset ) ||
           FT_READ_BYTE( charset->format )   )
        goto Exit;

      /* Allocate memory for sids. */
      if ( FT_NEW_ARRAY( charset->sids, num_glyphs ) )
        goto Exit;

      /* assign the .notdef glyph */
      charset->sids[0] = 0;

      switch ( charset->format )
      {
      case 0:
        if ( num_glyphs > 0 )
        {
          if ( FT_FRAME_ENTER( ( num_glyphs - 1 ) * 2 ) )
            goto Exit;

          for ( j = 1; j < num_glyphs; j++ )
            charset->sids[j] = FT_GET_USHORT();

          FT_FRAME_EXIT();
        }
        break;

      case 1:
      case 2:
        {
          FT_UInt  nleft;
          FT_UInt  i;


          j = 1;

          while ( j < num_glyphs )
          {
            /* Read the first glyph sid of the range. */
            if ( FT_READ_USHORT( glyph_sid ) )
              goto Exit;

            /* Read the number of glyphs in the range.  */
            if ( charset->format == 2 )
            {
              if ( FT_READ_USHORT( nleft ) )
                goto Exit;
            }
            else
            {
              if ( FT_READ_BYTE( nleft ) )
                goto Exit;
            }

            /* Fill in the range of sids -- `nleft + 1' glyphs. */
            for ( i = 0; j < num_glyphs && i <= nleft; i++, j++, glyph_sid++ )
              charset->sids[j] = glyph_sid;
          }
        }
        break;

      default:
        FT_ERROR(( "cff_charset_load: invalid table format!\n" ));
        error = CFF_Err_Invalid_File_Format;
        goto Exit;
      }
    }
    else
    {
      /* Parse default tables corresponding to offset == 0, 1, or 2.  */
      /* CFF specification intimates the following:                   */
      /*                                                              */
      /* In order to use a predefined charset, the following must be  */
      /* true: The charset constructed for the glyphs in the font's   */
      /* charstrings dictionary must match the predefined charset in  */
      /* the first num_glyphs.                                        */

      charset->offset = offset;  /* record charset type */

      switch ( (FT_UInt)offset )
      {
      case 0:
        if ( num_glyphs > 229 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n"
                     "predefined charset (Adobe ISO-Latin)!\n" ));
          error = CFF_Err_Invalid_File_Format;
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_NEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory. */
        FT_ARRAY_COPY( charset->sids, cff_isoadobe_charset, num_glyphs );

        break;

      case 1:
        if ( num_glyphs > 166 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n"
                     "predefined charset (Adobe Expert)!\n" ));
          error = CFF_Err_Invalid_File_Format;
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_NEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory.     */
        FT_ARRAY_COPY( charset->sids, cff_expert_charset, num_glyphs );

        break;

      case 2:
        if ( num_glyphs > 87 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n"
                     "predefined charset (Adobe Expert Subset)!\n" ));
          error = CFF_Err_Invalid_File_Format;
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_NEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory.     */
        FT_ARRAY_COPY( charset->sids, cff_expertsubset_charset, num_glyphs );

        break;

      default:
        error = CFF_Err_Invalid_File_Format;
        goto Exit;
      }
    }

    /* we have to invert the `sids' array for subsetted CID-keyed fonts */
    if ( invert )
    {
      FT_UInt    i;
      FT_UShort  max_cid = 0;


      for ( i = 0; i < num_glyphs; i++ )
        if ( charset->sids[i] > max_cid )
          max_cid = charset->sids[i];
      max_cid++;

      if ( FT_NEW_ARRAY( charset->cids, max_cid ) )
        goto Exit;
      FT_MEM_ZERO( charset->cids, sizeof ( FT_UShort ) * max_cid );

      for ( i = 0; i < num_glyphs; i++ )
        charset->cids[charset->sids[i]] = (FT_UShort)i;

      charset->max_cid = max_cid;
    }

  Exit:
    /* Clean up if there was an error. */
    if ( error )
    {
      FT_FREE( charset->sids );
      FT_FREE( charset->cids );
      charset->format = 0;
      charset->offset = 0;
      charset->sids   = 0;
    }

    return error;
  }


  static void
  cff_encoding_done( CFF_Encoding  encoding )
  {
    encoding->format = 0;
    encoding->offset = 0;
    encoding->count  = 0;
  }


  static FT_Error
  cff_encoding_load( CFF_Encoding  encoding,
                     CFF_Charset   charset,
                     FT_UInt       num_glyphs,
                     FT_Stream     stream,
                     FT_ULong      base_offset,
                     FT_ULong      offset )
  {
    FT_Error   error = CFF_Err_Ok;
    FT_UInt    count;
    FT_UInt    j;
    FT_UShort  glyph_sid;
    FT_UInt    glyph_code;


    /* Check for charset->sids.  If we do not have this, we fail. */
    if ( !charset->sids )
    {
      error = CFF_Err_Invalid_File_Format;
      goto Exit;
    }

    /* Zero out the code to gid/sid mappings. */
    for ( j = 0; j < 256; j++ )
    {
      encoding->sids [j] = 0;
      encoding->codes[j] = 0;
    }

    /* Note: The encoding table in a CFF font is indexed by glyph index;  */
    /* the first encoded glyph index is 1.  Hence, we read the character  */
    /* code (`glyph_code') at index j and make the assignment:            */
    /*                                                                    */
    /*    encoding->codes[glyph_code] = j + 1                             */
    /*                                                                    */
    /* We also make the assignment:                                       */
    /*                                                                    */
    /*    encoding->sids[glyph_code] = charset->sids[j + 1]               */
    /*                                                                    */
    /* This gives us both a code to GID and a code to SID mapping.        */

    if ( offset > 1 )
    {
      encoding->offset = base_offset + offset;

      /* we need to parse the table to determine its size */
      if ( FT_STREAM_SEEK( encoding->offset ) ||
           FT_READ_BYTE( encoding->format )   ||
           FT_READ_BYTE( count )              )
        goto Exit;

      switch ( encoding->format & 0x7F )
      {
      case 0:
        {
          FT_Byte*  p;


          /* By convention, GID 0 is always ".notdef" and is never */
          /* coded in the font.  Hence, the number of codes found  */
          /* in the table is `count+1'.                            */
          /*                                                       */
          encoding->count = count + 1;

          if ( FT_FRAME_ENTER( count ) )
            goto Exit;

          p = (FT_Byte*)stream->cursor;

          for ( j = 1; j <= count; j++ )
          {
            glyph_code = *p++;

            /* Make sure j is not too big. */
            if ( j < num_glyphs )
            {
              /* Assign code to GID mapping. */
              encoding->codes[glyph_code] = (FT_UShort)j;

              /* Assign code to SID mapping. */
              encoding->sids[glyph_code] = charset->sids[j];
            }
          }

          FT_FRAME_EXIT();
        }
        break;

      case 1:
        {
          FT_UInt  nleft;
          FT_UInt  i = 1;
          FT_UInt  k;


          encoding->count = 0;

          /* Parse the Format1 ranges. */
          for ( j = 0;  j < count; j++, i += nleft )
          {
            /* Read the first glyph code of the range. */
            if ( FT_READ_BYTE( glyph_code ) )
              goto Exit;

            /* Read the number of codes in the range. */
            if ( FT_READ_BYTE( nleft ) )
              goto Exit;

            /* Increment nleft, so we read `nleft + 1' codes/sids. */
            nleft++;

            /* compute max number of character codes */
            if ( (FT_UInt)nleft > encoding->count )
              encoding->count = nleft;

            /* Fill in the range of codes/sids. */
            for ( k = i; k < nleft + i; k++, glyph_code++ )
            {
              /* Make sure k is not too big. */
              if ( k < num_glyphs && glyph_code < 256 )
              {
                /* Assign code to GID mapping. */
                encoding->codes[glyph_code] = (FT_UShort)k;

                /* Assign code to SID mapping. */
                encoding->sids[glyph_code] = charset->sids[k];
              }
            }
          }

          /* simple check; one never knows what can be found in a font */
          if ( encoding->count > 256 )
            encoding->count = 256;
        }
        break;

      default:
        FT_ERROR(( "cff_encoding_load: invalid table format!\n" ));
        error = CFF_Err_Invalid_File_Format;
        goto Exit;
      }

      /* Parse supplemental encodings, if any. */
      if ( encoding->format & 0x80 )
      {
        FT_UInt  gindex;


        /* count supplements */
        if ( FT_READ_BYTE( count ) )
          goto Exit;

        for ( j = 0; j < count; j++ )
        {
          /* Read supplemental glyph code. */
          if ( FT_READ_BYTE( glyph_code ) )
            goto Exit;

          /* Read the SID associated with this glyph code. */
          if ( FT_READ_USHORT( glyph_sid ) )
            goto Exit;

          /* Assign code to SID mapping. */
          encoding->sids[glyph_code] = glyph_sid;

          /* First, look up GID which has been assigned to */
          /* SID glyph_sid.                                */
          for ( gindex = 0; gindex < num_glyphs; gindex++ )
          {
            if ( charset->sids[gindex] == glyph_sid )
            {
              encoding->codes[glyph_code] = (FT_UShort)gindex;
              break;
            }
          }
        }
      }
    }
    else
    {
      FT_UInt i;


      /* We take into account the fact a CFF font can use a predefined */
      /* encoding without containing all of the glyphs encoded by this */
      /* encoding (see the note at the end of section 12 in the CFF    */
      /* specification).                                               */

      switch ( (FT_UInt)offset )
      {
      case 0:
        /* First, copy the code to SID mapping. */
        FT_ARRAY_COPY( encoding->sids, cff_standard_encoding, 256 );
        goto Populate;

      case 1:
        /* First, copy the code to SID mapping. */
        FT_ARRAY_COPY( encoding->sids, cff_expert_encoding, 256 );

      Populate:
        /* Construct code to GID mapping from code to SID mapping */
        /* and charset.                                           */

        encoding->count = 0;

        for ( j = 0; j < 256; j++ )
        {
          /* If j is encoded, find the GID for it. */
          if ( encoding->sids[j] )
          {
            for ( i = 1; i < num_glyphs; i++ )
              /* We matched, so break. */
              if ( charset->sids[i] == encoding->sids[j] )
                break;

            /* i will be equal to num_glyphs if we exited the above */
            /* loop without a match.  In this case, we also have to */
            /* fix the code to SID mapping.                         */
            if ( i == num_glyphs )
            {
              encoding->codes[j] = 0;
              encoding->sids [j] = 0;
            }
            else
            {
              encoding->codes[j] = (FT_UShort)i;

              /* update encoding count */
              if ( encoding->count < j + 1 )
                encoding->count = j + 1;
            }
          }
        }
        break;

      default:
        FT_ERROR(( "cff_encoding_load: invalid table format!\n" ));
        error = CFF_Err_Invalid_File_Format;
        goto Exit;
      }
    }

  Exit:

    /* Clean up if there was an error. */
    return error;
  }


  static FT_Error
  cff_subfont_load( CFF_SubFont  font,
                    CFF_Index    idx,
                    FT_UInt      font_index,
                    FT_Stream    stream,
                    FT_ULong     base_offset )
  {
    FT_Error         error;
    CFF_ParserRec    parser;
    FT_Byte*         dict = NULL;
    FT_ULong         dict_len;
    CFF_FontRecDict  top  = &font->font_dict;
    CFF_Private      priv = &font->private_dict;


    cff_parser_init( &parser, CFF_CODE_TOPDICT, &font->font_dict );

    /* set defaults */
    FT_MEM_ZERO( top, sizeof ( *top ) );

    top->underline_position  = -100L << 16;
    top->underline_thickness = 50L << 16;
    top->charstring_type     = 2;
    top->font_matrix.xx      = 0x10000L;
    top->font_matrix.yy      = 0x10000L;
    top->cid_count           = 8720;

    /* we use the implementation specific SID value 0xFFFF to indicate */
    /* missing entries                                                 */
    top->version             = 0xFFFFU;
    top->notice              = 0xFFFFU;
    top->copyright           = 0xFFFFU;
    top->full_name           = 0xFFFFU;
    top->family_name         = 0xFFFFU;
    top->weight              = 0xFFFFU;
    top->embedded_postscript = 0xFFFFU;

    top->cid_registry        = 0xFFFFU;
    top->cid_ordering        = 0xFFFFU;
    top->cid_font_name       = 0xFFFFU;

    error = cff_index_access_element( idx, font_index, &dict, &dict_len ) ||
            cff_parser_run( &parser, dict, dict + dict_len );

    cff_index_forget_element( idx, &dict );

    if ( error )
      goto Exit;
 
    /* if it is a CID font, we stop there */
    if ( top->cid_registry != 0xFFFFU )
      goto Exit;

    /* parse the private dictionary, if any */
    if ( top->private_offset && top->private_size )
    {
      /* set defaults */
      FT_MEM_ZERO( priv, sizeof ( *priv ) );

      priv->blue_shift       = 7;
      priv->blue_fuzz        = 1;
      priv->lenIV            = -1;
      priv->expansion_factor = (FT_Fixed)( 0.06 * 0x10000L );
      priv->blue_scale       = (FT_Fixed)( 0.039625 * 0x10000L * 1000 );

      cff_parser_init( &parser, CFF_CODE_PRIVATE, priv );

      if ( FT_STREAM_SEEK( base_offset + font->font_dict.private_offset ) ||
           FT_FRAME_ENTER( font->font_dict.private_size )                 )
        goto Exit;

      error = cff_parser_run( &parser,
                              (FT_Byte*)stream->cursor,
                              (FT_Byte*)stream->limit );
      FT_FRAME_EXIT();
      if ( error )
        goto Exit;

      /* ensure that `num_blue_values' is even */
      priv->num_blue_values &= ~1;
    }

    /* read the local subrs, if any */
    if ( priv->local_subrs_offset )
    {
      if ( FT_STREAM_SEEK( base_offset + top->private_offset +
                           priv->local_subrs_offset ) )
        goto Exit;

      error = cff_new_index( &font->local_subrs_index, stream, 1 );
      if ( error )
        goto Exit;

      font->num_local_subrs = font->local_subrs_index.count;
      error = cff_index_get_pointers( &font->local_subrs_index,
                                      &font->local_subrs );
      if ( error )
        goto Exit;
    }

  Exit:
    return error;
  }


  static void
  cff_subfont_done( FT_Memory    memory,
                    CFF_SubFont  subfont )
  {
    if ( subfont )
    {
      cff_done_index( &subfont->local_subrs_index );
      FT_FREE( subfont->local_subrs );
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cff_font_load( FT_Stream  stream,
                 FT_Int     face_index,
                 CFF_Font   font )
  {
    static const FT_Frame_Field  cff_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  CFF_FontRec

      FT_FRAME_START( 4 ),
        FT_FRAME_BYTE( version_major ),
        FT_FRAME_BYTE( version_minor ),
        FT_FRAME_BYTE( header_size ),
        FT_FRAME_BYTE( absolute_offsize ),
      FT_FRAME_END
    };

    FT_Error         error;
    FT_Memory        memory = stream->memory;
    FT_ULong         base_offset;
    CFF_FontRecDict  dict;


    FT_ZERO( font );

    font->stream = stream;
    font->memory = memory;
    dict         = &font->top_font.font_dict;
    base_offset  = FT_STREAM_POS();

    /* read CFF font header */
    if ( FT_STREAM_READ_FIELDS( cff_header_fields, font ) )
      goto Exit;

    /* check format */
    if ( font->version_major   != 1 ||
         font->header_size      < 4 ||
         font->absolute_offsize > 4 )
    {
      FT_TRACE2(( "[not a CFF font header!]\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    /* skip the rest of the header */
    if ( FT_STREAM_SKIP( font->header_size - 4 ) )
      goto Exit;

    /* read the name, top dict, string and global subrs index */
    if ( FT_SET_ERROR( cff_new_index( &font->name_index,         stream, 0 )) ||
         FT_SET_ERROR( cff_new_index( &font->font_dict_index,    stream, 0 )) ||
         FT_SET_ERROR( cff_new_index( &font->string_index,       stream, 0 )) ||
         FT_SET_ERROR( cff_new_index( &font->global_subrs_index, stream, 1 )) )
      goto Exit;

    /* well, we don't really forget the `disabled' fonts... */
    font->num_faces = font->name_index.count;
    if ( face_index >= (FT_Int)font->num_faces )
    {
      FT_ERROR(( "cff_font_load: incorrect face index = %d\n",
                 face_index ));
      error = CFF_Err_Invalid_Argument;
    }

    /* in case of a font format check, simply exit now */
    if ( face_index < 0 )
      goto Exit;

    /* now, parse the top-level font dictionary */
    error = cff_subfont_load( &font->top_font,
                              &font->font_dict_index,
                              face_index,
                              stream,
                              base_offset );
    if ( error )
      goto Exit;

    if ( FT_STREAM_SEEK( base_offset + dict->charstrings_offset ) )
      goto Exit;

    error = cff_new_index( &font->charstrings_index, stream, 0 );
    if ( error )
      goto Exit;

    /* now, check for a CID font */
    if ( dict->cid_registry != 0xFFFFU )
    {
      CFF_IndexRec  fd_index;
      CFF_SubFont   sub;
      FT_UInt       idx;


      /* this is a CID-keyed font, we must now allocate a table of */
      /* sub-fonts, then load each of them separately              */
      if ( FT_STREAM_SEEK( base_offset + dict->cid_fd_array_offset ) )
        goto Exit;

      error = cff_new_index( &fd_index, stream, 0 );
      if ( error )
        goto Exit;

      if ( fd_index.count > CFF_MAX_CID_FONTS )
      {
        FT_ERROR(( "cff_font_load: FD array too large in CID font\n" ));
        goto Fail_CID;
      }

      /* allocate & read each font dict independently */
      font->num_subfonts = fd_index.count;
      if ( FT_NEW_ARRAY( sub, fd_index.count ) )
        goto Fail_CID;

      /* set up pointer table */
      for ( idx = 0; idx < fd_index.count; idx++ )
        font->subfonts[idx] = sub + idx;

      /* now load each subfont independently */
      for ( idx = 0; idx < fd_index.count; idx++ )
      {
        sub = font->subfonts[idx];
        error = cff_subfont_load( sub, &fd_index, idx,
                                  stream, base_offset );
        if ( error )
          goto Fail_CID;
      }

      /* now load the FD Select array */
      error = CFF_Load_FD_Select( &font->fd_select,
                                  font->charstrings_index.count,
                                  stream,
                                  base_offset + dict->cid_fd_select_offset );

    Fail_CID:
      cff_done_index( &fd_index );

      if ( error )
        goto Exit;
    }
    else
      font->num_subfonts = 0;

    /* read the charstrings index now */
    if ( dict->charstrings_offset == 0 )
    {
      FT_ERROR(( "cff_font_load: no charstrings offset!\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    /* explicit the global subrs */
    font->num_global_subrs = font->global_subrs_index.count;
    font->num_glyphs       = font->charstrings_index.count;

    error = cff_index_get_pointers( &font->global_subrs_index,
                                    &font->global_subrs ) ;

    if ( error )
      goto Exit;

    /* read the Charset and Encoding tables if available */
    if ( font->num_glyphs > 0 )
    {
      FT_Bool  invert = FT_BOOL( dict->cid_registry != 0xFFFFU );


      error = cff_charset_load( &font->charset, font->num_glyphs, stream,
                                base_offset, dict->charset_offset, invert );
      if ( error )
        goto Exit;

      /* CID-keyed CFFs don't have an encoding */
      if ( dict->cid_registry == 0xFFFFU )
      {
        error = cff_encoding_load( &font->encoding,
                                   &font->charset,
                                   font->num_glyphs,
                                   stream,
                                   base_offset,
                                   dict->encoding_offset );
        if ( error )
          goto Exit;
      }
      else
        /* CID-keyed fonts only need CIDs */
        FT_FREE( font->charset.sids );
    }

    /* get the font name (/CIDFontName for CID-keyed fonts, */
    /* /FontName otherwise)                                 */
    font->font_name = cff_index_get_name( &font->name_index, face_index );

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  cff_font_done( CFF_Font  font )
  {
    FT_Memory  memory = font->memory;
    FT_UInt    idx;


    cff_done_index( &font->global_subrs_index );
    cff_done_index( &font->string_index );
    cff_done_index( &font->font_dict_index );
    cff_done_index( &font->name_index );
    cff_done_index( &font->charstrings_index );

    /* release font dictionaries, but only if working with */
    /* a CID keyed CFF font                                */
    if ( font->num_subfonts > 0 )
    {
      for ( idx = 0; idx < font->num_subfonts; idx++ )
        cff_subfont_done( memory, font->subfonts[idx] );
    }

    cff_encoding_done( &font->encoding );
    cff_charset_done( &font->charset, font->stream );

    cff_subfont_done( memory, &font->top_font );

    CFF_Done_FD_Select( &font->fd_select, font->stream );

    FT_FREE( font->global_subrs );
    FT_FREE( font->font_name );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cffobjs.c                                                              */
/*                                                                         */
/*    OpenType objects manager (body).                                     */
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
#include FT_INTERNAL_STREAM_H
#include FT_ERRORS_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_SFNT_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include "cffobjs.h"
#include "cffload.h"
#include "cffcmap.h"
#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffobjs


  /*************************************************************************/
  /*                                                                       */
  /*                            SIZE FUNCTIONS                             */
  /*                                                                       */
  /*  Note that we store the global hints in the size's `internal' root    */
  /*  field.                                                               */
  /*                                                                       */
  /*************************************************************************/


  static PSH_Globals_Funcs
  cff_size_get_globals_funcs( CFF_Size  size )
  {
    CFF_Face          face     = (CFF_Face)size->root.face;
    CFF_Font          font     = (CFF_FontRec *)face->extra.data;
    PSHinter_Service  pshinter = (PSHinter_Service)font->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF( void )
  cff_size_done( FT_Size  cffsize )        /* CFF_Size */
  {
    CFF_Size  size = (CFF_Size)cffsize;


    if ( cffsize->internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = cff_size_get_globals_funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)cffsize->internal );

      cffsize->internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cff_size_init( FT_Size  cffsize )         /* CFF_Size */
  {
    CFF_Size           size  = (CFF_Size)cffsize;
    FT_Error           error = CFF_Err_Ok;
    PSH_Globals_Funcs  funcs = cff_size_get_globals_funcs( size );


    if ( funcs )
    {
      PSH_Globals    globals;
      CFF_Face       face    = (CFF_Face)cffsize->face;
      CFF_Font       font    = (CFF_FontRec *)face->extra.data;
      CFF_SubFont    subfont = &font->top_font;

      CFF_Private    cpriv   = &subfont->private_dict;
      PS_PrivateRec  priv;


      /* IMPORTANT: The CFF and Type1 private dictionaries have    */
      /*            slightly different structures; we need to      */
      /*            synthetize a type1 dictionary on the fly here. */

      {
        FT_UInt  n, count;


        FT_MEM_ZERO( &priv, sizeof ( priv ) );

        count = priv.num_blue_values = cpriv->num_blue_values;
        for ( n = 0; n < count; n++ )
          priv.blue_values[n] = (FT_Short)cpriv->blue_values[n];

        count = priv.num_other_blues = cpriv->num_other_blues;
        for ( n = 0; n < count; n++ )
          priv.other_blues[n] = (FT_Short)cpriv->other_blues[n];

        count = priv.num_family_blues = cpriv->num_family_blues;
        for ( n = 0; n < count; n++ )
          priv.family_blues[n] = (FT_Short)cpriv->family_blues[n];

        count = priv.num_family_other_blues = cpriv->num_family_other_blues;
        for ( n = 0; n < count; n++ )
          priv.family_other_blues[n] = (FT_Short)cpriv->family_other_blues[n];

        priv.blue_scale = cpriv->blue_scale;
        priv.blue_shift = (FT_Int)cpriv->blue_shift;
        priv.blue_fuzz  = (FT_Int)cpriv->blue_fuzz;

        priv.standard_width[0]  = (FT_UShort)cpriv->standard_width;
        priv.standard_height[0] = (FT_UShort)cpriv->standard_height;

        count = priv.num_snap_widths = cpriv->num_snap_widths;
        for ( n = 0; n < count; n++ )
          priv.snap_widths[n] = (FT_Short)cpriv->snap_widths[n];

        count = priv.num_snap_heights = cpriv->num_snap_heights;
        for ( n = 0; n < count; n++ )
          priv.snap_heights[n] = (FT_Short)cpriv->snap_heights[n];

        priv.force_bold     = cpriv->force_bold;
        priv.language_group = cpriv->language_group;
        priv.lenIV          = cpriv->lenIV;
      }

      error = funcs->create( cffsize->face->memory, &priv, &globals );
      if ( !error )
        cffsize->internal = (FT_Size_Internal)(void*)globals;
    }

    size->strike_index = 0xFFFFFFFFUL;

    return error;
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL_DEF( FT_Error )
  cff_size_select( FT_Size   size,
                   FT_ULong  strike_index )
  {
    CFF_Size           cffsize = (CFF_Size)size;
    PSH_Globals_Funcs  funcs;


    cffsize->strike_index = strike_index;

    FT_Select_Metrics( size->face, strike_index );

    funcs = cff_size_get_globals_funcs( cffsize );

    if ( funcs )
      funcs->set_scale( (PSH_Globals)size->internal,
                        size->metrics.x_scale,
                        size->metrics.y_scale,
                        0, 0 );

    return CFF_Err_Ok;
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  FT_LOCAL_DEF( FT_Error )
  cff_size_request( FT_Size          size,
                    FT_Size_Request  req )
  {
    CFF_Size           cffsize = (CFF_Size)size;
    PSH_Globals_Funcs  funcs;


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    if ( FT_HAS_FIXED_SIZES( size->face ) )
    {
      CFF_Face      cffface = (CFF_Face)size->face;
      SFNT_Service  sfnt    = (SFNT_Service)cffface->sfnt;
      FT_ULong      index;


      if ( sfnt->set_sbit_strike( cffface, req, &index ) )
        cffsize->strike_index = 0xFFFFFFFFUL;
      else
        return cff_size_select( size, index );
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    FT_Request_Metrics( size->face, req );

    funcs = cff_size_get_globals_funcs( cffsize );

    if ( funcs )
      funcs->set_scale( (PSH_Globals)size->internal,
                        size->metrics.x_scale,
                        size->metrics.y_scale,
                        0, 0 );

    return CFF_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                            SLOT  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  cff_slot_done( FT_GlyphSlot  slot )
  {
    slot->internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_slot_init( FT_GlyphSlot  slot )
  {
    CFF_Face          face     = (CFF_Face)slot->face;
    CFF_Font          font     = (CFF_FontRec *)face->extra.data;
    PSHinter_Service  pshinter = (PSHinter_Service)font->pshinter;


    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T2_Hints_Funcs  funcs;


        funcs = pshinter->get_t2_funcs( module );
        slot->internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  static FT_String*
  cff_strcpy( FT_Memory         memory,
              const FT_String*  source )
  {
    FT_Error    error;
    FT_String*  result = 0;
    FT_Int      len = (FT_Int)ft_strlen( source );


    if ( !FT_ALLOC( result, len + 1 ) )
    {
      FT_MEM_COPY( result, source, len );
      result[len] = 0;
    }

    FT_UNUSED( error );

    return result;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_face_init( FT_Stream      stream,
                 FT_Face        cffface,        /* CFF_Face */
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    CFF_Face            face = (CFF_Face)cffface;
    FT_Error            error;
    SFNT_Service        sfnt;
    FT_Service_PsCMaps  psnames;
    PSHinter_Service    pshinter;
    FT_Bool             pure_cff    = 1;
    FT_Bool             sfnt_format = 0;


#if 0
    FT_FACE_FIND_GLOBAL_SERVICE( face, sfnt,     SFNT );
    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames,  POSTSCRIPT_NAMES );
    FT_FACE_FIND_GLOBAL_SERVICE( face, pshinter, POSTSCRIPT_HINTER );

    if ( !sfnt )
      goto Bad_Format;
#else
    sfnt = (SFNT_Service)FT_Get_Module_Interface(
             cffface->driver->root.library, "sfnt" );
    if ( !sfnt )
      goto Bad_Format;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );

    pshinter = (PSHinter_Service)FT_Get_Module_Interface(
                 cffface->driver->root.library, "pshinter" );
#endif

    /* create input stream from resource */
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    /* check whether we have a valid OpenType file */
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( !error )
    {
      if ( face->format_tag != 0x4F54544FL )  /* `OTTO'; OpenType/CFF font */
      {
        FT_TRACE2(( "[not a valid OpenType/CFF font]\n" ));
        goto Bad_Format;
      }

      /* if we are performing a simple font format check, exit immediately */
      if ( face_index < 0 )
        return CFF_Err_Ok;

      /* UNDOCUMENTED!  A CFF in an SFNT can have only a single font. */
      if ( face_index > 0 )
      {
        FT_ERROR(( "cff_face_init: invalid face index\n" ));
        error = CFF_Err_Invalid_Argument;
        goto Exit;
      }

      sfnt_format = 1;

      /* now, the font can be either an OpenType/CFF font, or an SVG CEF */
      /* font; in the latter case it doesn't have a `head' table         */
      error = face->goto_table( face, TTAG_head, stream, 0 );
      if ( !error )
      {
        pure_cff = 0;

        /* load font directory */
        error = sfnt->load_face( stream, face,
                                 face_index, num_params, params );
        if ( error )
          goto Exit;
      }
      else
      {
        /* load the `cmap' table explicitly */
        error = sfnt->load_cmap( face, stream );
        if ( error )
          goto Exit;

        /* XXX: we don't load the GPOS table, as OpenType Layout     */
        /* support will be added later to a layout library on top of */
        /* FreeType 2                                                */
      }

      /* now load the CFF part of the file */
      error = face->goto_table( face, TTAG_CFF, stream, 0 );
      if ( error )
        goto Exit;
    }
    else
    {
      /* rewind to start of file; we are going to load a pure-CFF font */
      if ( FT_STREAM_SEEK( 0 ) )
        goto Exit;
      error = CFF_Err_Ok;
    }

    /* now load and parse the CFF table in the file */
    {
      CFF_Font         cff;
      CFF_FontRecDict  dict;
      FT_Memory        memory = cffface->memory;
      FT_Int32         flags;
      FT_UInt          i;


      if ( FT_NEW( cff ) )
        goto Exit;

      face->extra.data = cff;
      error = cff_font_load( stream, face_index, cff );
      if ( error )
        goto Exit;

      cff->pshinter = pshinter;
      cff->psnames  = (void*)psnames;

      /* Complement the root flags with some interesting information. */
      /* Note that this is only necessary for pure CFF and CEF fonts; */
      /* SFNT based fonts use the `name' table instead.               */

      cffface->num_glyphs = cff->num_glyphs;

      dict = &cff->top_font.font_dict;

      /* we need the `PSNames' module for CFF and CEF formats */
      /* which aren't CID-keyed                               */
      if ( dict->cid_registry == 0xFFFFU && !psnames )
      {
        FT_ERROR(( "cff_face_init:" ));
        FT_ERROR(( " cannot open CFF & CEF fonts\n" ));
        FT_ERROR(( "              " ));
        FT_ERROR(( " without the `PSNames' module\n" ));
        goto Bad_Format;
      }

      if ( pure_cff )
      {
        char*  style_name = NULL;


        /* set up num_faces */
        cffface->num_faces = cff->num_faces;

        /* compute number of glyphs */
        if ( dict->cid_registry != 0xFFFFU )
          cffface->num_glyphs = dict->cid_count;
        else
          cffface->num_glyphs = cff->charstrings_index.count;

        /* set global bbox, as well as EM size */
        cffface->bbox.xMin =   dict->font_bbox.xMin             >> 16;
        cffface->bbox.yMin =   dict->font_bbox.yMin             >> 16;
        cffface->bbox.xMax = ( dict->font_bbox.xMax + 0xFFFFU ) >> 16;
        cffface->bbox.yMax = ( dict->font_bbox.yMax + 0xFFFFU ) >> 16;

        if ( !dict->units_per_em )
          dict->units_per_em = 1000;

        cffface->units_per_EM = dict->units_per_em;

        cffface->ascender  = (FT_Short)( cffface->bbox.yMax );
        cffface->descender = (FT_Short)( cffface->bbox.yMin );

        cffface->height = (FT_Short)( ( cffface->units_per_EM * 12 ) / 10 );
        if ( cffface->height < cffface->ascender - cffface->descender )
          cffface->height = (FT_Short)( cffface->ascender - cffface->descender );

        cffface->underline_position  =
          (FT_Short)( dict->underline_position >> 16 );
        cffface->underline_thickness =
          (FT_Short)( dict->underline_thickness >> 16 );

        /* retrieve font family & style name */
        cffface->family_name = cff_index_get_name( &cff->name_index,
                                                   face_index );

        if ( cffface->family_name )
        {
          char*  full   = cff_index_get_sid_string( &cff->string_index,
                                                    dict->full_name,
                                                    psnames );
          char*  fullp  = full;
          char*  family = cffface->family_name;
          char*  family_name = 0;


          if ( dict->family_name )
          {
            family_name = cff_index_get_sid_string( &cff->string_index,
                                                    dict->family_name,
                                                    psnames);
            if ( family_name )
              family = family_name;
          }

          /* We try to extract the style name from the full name.   */
          /* We need to ignore spaces and dashes during the search. */
          if ( full && family )
          {
            while ( *fullp )
            {
              /* skip common characters at the start of both strings */
              if ( *fullp == *family )
              {
                family++;
                fullp++;
                continue;
              }

              /* ignore spaces and dashes in full name during comparison */
              if ( *fullp == ' ' || *fullp == '-' )
              {
                fullp++;
                continue;
              }

              /* ignore spaces and dashes in family name during comparison */
              if ( *family == ' ' || *family == '-' )
              {
                family++;
                continue;
              }

              if ( !*family && *fullp )
              {
                /* The full name begins with the same characters as the  */
                /* family name, with spaces and dashes removed.  In this */
                /* case, the remaining string in `fullp' will be used as */
                /* the style name.                                       */
                style_name = cff_strcpy( memory, fullp );
              }
              break;
            }

            if ( family_name )
              FT_FREE( family_name );
            FT_FREE( full );
          }
        }
        else
        {
          char  *cid_font_name =
                   cff_index_get_sid_string( &cff->string_index,
                                             dict->cid_font_name,
                                             psnames );


          /* do we have a `/FontName' for a CID-keyed font? */
          if ( cid_font_name )
            cffface->family_name = cid_font_name;
        }

        if ( style_name )
          cffface->style_name = style_name;
        else
          /* assume "Regular" style if we don't know better */
          cffface->style_name = cff_strcpy( memory, (char *)"Regular" );

        /*******************************************************************/
        /*                                                                 */
        /* Compute face flags.                                             */
        /*                                                                 */
        flags = FT_FACE_FLAG_SCALABLE   |       /* scalable outlines */
                FT_FACE_FLAG_HORIZONTAL |       /* horizontal data   */
                FT_FACE_FLAG_HINTER;            /* has native hinter */

        if ( sfnt_format )
          flags |= FT_FACE_FLAG_SFNT;

        /* fixed width font? */
        if ( dict->is_fixed_pitch )
          flags |= FT_FACE_FLAG_FIXED_WIDTH;

  /* XXX: WE DO NOT SUPPORT KERNING METRICS IN THE GPOS TABLE FOR NOW */
#if 0
        /* kerning available? */
        if ( face->kern_pairs )
          flags |= FT_FACE_FLAG_KERNING;
#endif

        cffface->face_flags = flags;

        /*******************************************************************/
        /*                                                                 */
        /* Compute style flags.                                            */
        /*                                                                 */
        flags = 0;

        if ( dict->italic_angle )
          flags |= FT_STYLE_FLAG_ITALIC;

        {
          char  *weight = cff_index_get_sid_string( &cff->string_index,
                                                    dict->weight,
                                                    psnames );


          if ( weight )
            if ( !ft_strcmp( weight, "Bold"  ) ||
                 !ft_strcmp( weight, "Black" ) )
              flags |= FT_STYLE_FLAG_BOLD;
          FT_FREE( weight );
        }

        /* double check */
        if ( !(flags & FT_STYLE_FLAG_BOLD) && cffface->style_name )
          if ( !ft_strncmp( cffface->style_name, "Bold", 4 )  ||
               !ft_strncmp( cffface->style_name, "Black", 5 ) )
            flags |= FT_STYLE_FLAG_BOLD;

        cffface->style_flags = flags;
      }
      else
      {
        if ( !dict->units_per_em )
          dict->units_per_em = face->root.units_per_EM;
      }

      /* handle font matrix settings in subfonts (if any) */
      for ( i = cff->num_subfonts; i > 0; i-- )
      {
        CFF_FontRecDict  sub = &cff->subfonts[i - 1]->font_dict;
        CFF_FontRecDict  top = &cff->top_font.font_dict;


        if ( sub->units_per_em )
        {
          FT_Matrix  scale;


          scale.xx = scale.yy = (FT_Fixed)FT_DivFix( top->units_per_em,
                                                     sub->units_per_em );
          scale.xy = scale.yx = 0;

          FT_Matrix_Multiply( &scale, &sub->font_matrix );
          FT_Vector_Transform( &sub->font_offset, &scale );
        }
        else
        {
          sub->font_matrix = top->font_matrix;
          sub->font_offset = top->font_offset;
        }
      }

#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
      /* CID-keyed CFF fonts don't have glyph names -- the SFNT loader */
      /* has unset this flag because of the 3.0 `post' table           */
      if ( dict->cid_registry == 0xFFFFU )
        cffface->face_flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      /*******************************************************************/
      /*                                                                 */
      /* Compute char maps.                                              */
      /*                                                                 */

      /* Try to synthetize a Unicode charmap if there is none available */
      /* already.  If an OpenType font contains a Unicode "cmap", we    */
      /* will use it, whatever be in the CFF part of the file.          */
      {
        FT_CharMapRec  cmaprec;
        FT_CharMap     cmap;
        FT_UInt        nn;
        CFF_Encoding   encoding = &cff->encoding;


        for ( nn = 0; nn < (FT_UInt)cffface->num_charmaps; nn++ )
        {
          cmap = cffface->charmaps[nn];

          /* Windows Unicode (3,1)? */
          if ( cmap->platform_id == 3 && cmap->encoding_id == 1 )
            goto Skip_Unicode;

          /* Deprecated Unicode platform id? */
          if ( cmap->platform_id == 0 )
            goto Skip_Unicode; /* Standard Unicode (deprecated) */
        }

        /* since CID-keyed fonts don't contain glyph names, we can't */
        /* construct a cmap                                          */
        if ( pure_cff && cff->top_font.font_dict.cid_registry != 0xFFFFU )
          goto Exit;

        /* we didn't find a Unicode charmap -- synthetize one */
        cmaprec.face        = cffface;
        cmaprec.platform_id = 3;
        cmaprec.encoding_id = 1;
        cmaprec.encoding    = FT_ENCODING_UNICODE;

        nn = (FT_UInt)cffface->num_charmaps;

        FT_CMap_New( &cff_cmap_unicode_class_rec, NULL, &cmaprec, NULL );

        /* if no Unicode charmap was previously selected, select this one */
        if ( cffface->charmap == NULL && nn != (FT_UInt)cffface->num_charmaps )
          cffface->charmap = cffface->charmaps[nn];

      Skip_Unicode:
        if ( encoding->count > 0 )
        {
          FT_CMap_Class  clazz;


          cmaprec.face        = cffface;
          cmaprec.platform_id = 7;  /* Adobe platform id */

          if ( encoding->offset == 0 )
          {
            cmaprec.encoding_id = TT_ADOBE_ID_STANDARD;
            cmaprec.encoding    = FT_ENCODING_ADOBE_STANDARD;
            clazz               = &cff_cmap_encoding_class_rec;
          }
          else if ( encoding->offset == 1 )
          {
            cmaprec.encoding_id = TT_ADOBE_ID_EXPERT;
            cmaprec.encoding    = FT_ENCODING_ADOBE_EXPERT;
            clazz               = &cff_cmap_encoding_class_rec;
          }
          else
          {
            cmaprec.encoding_id = TT_ADOBE_ID_CUSTOM;
            cmaprec.encoding    = FT_ENCODING_ADOBE_CUSTOM;
            clazz               = &cff_cmap_encoding_class_rec;
          }

          FT_CMap_New( clazz, NULL, &cmaprec, NULL );
        }
      }
    }

  Exit:
    return error;

  Bad_Format:
    error = CFF_Err_Unknown_File_Format;
    goto Exit;
  }


  FT_LOCAL_DEF( void )
  cff_face_done( FT_Face  cffface )         /* CFF_Face */
  {
    CFF_Face      face   = (CFF_Face)cffface;
    FT_Memory     memory = cffface->memory;
    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    if ( sfnt )
      sfnt->done_face( face );

    {
      CFF_Font  cff = (CFF_Font)face->extra.data;


      if ( cff )
      {
        cff_font_done( cff );
        FT_FREE( face->extra.data );
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cff_driver_init( FT_Module  module )
  {
    FT_UNUSED( module );

    return CFF_Err_Ok;
  }


  FT_LOCAL_DEF( void )
  cff_driver_done( FT_Module  module )
  {
    FT_UNUSED( module );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cffgload.c                                                             */
/*                                                                         */
/*    OpenType Glyph Loader (body).                                        */
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
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H

#include "cffobjs.h"
#include "cffload.h"
#include "cffgload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffgload


  typedef enum  CFF_Operator_
  {
    cff_op_unknown = 0,

    cff_op_rmoveto,
    cff_op_hmoveto,
    cff_op_vmoveto,

    cff_op_rlineto,
    cff_op_hlineto,
    cff_op_vlineto,

    cff_op_rrcurveto,
    cff_op_hhcurveto,
    cff_op_hvcurveto,
    cff_op_rcurveline,
    cff_op_rlinecurve,
    cff_op_vhcurveto,
    cff_op_vvcurveto,

    cff_op_flex,
    cff_op_hflex,
    cff_op_hflex1,
    cff_op_flex1,

    cff_op_endchar,

    cff_op_hstem,
    cff_op_vstem,
    cff_op_hstemhm,
    cff_op_vstemhm,

    cff_op_hintmask,
    cff_op_cntrmask,
    cff_op_dotsection,  /* deprecated, acts as no-op */

    cff_op_abs,
    cff_op_add,
    cff_op_sub,
    cff_op_div,
    cff_op_neg,
    cff_op_random,
    cff_op_mul,
    cff_op_sqrt,

    cff_op_blend,

    cff_op_drop,
    cff_op_exch,
    cff_op_index,
    cff_op_roll,
    cff_op_dup,

    cff_op_put,
    cff_op_get,
    cff_op_store,
    cff_op_load,

    cff_op_and,
    cff_op_or,
    cff_op_not,
    cff_op_eq,
    cff_op_ifelse,

    cff_op_callsubr,
    cff_op_callgsubr,
    cff_op_return,

    /* do not remove */
    cff_op_max

  } CFF_Operator;


#define CFF_COUNT_CHECK_WIDTH  0x80
#define CFF_COUNT_EXACT        0x40
#define CFF_COUNT_CLEAR_STACK  0x20


  static const FT_Byte  cff_argument_counts[] =
  {
    0,  /* unknown */

    2 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT, /* rmoveto */
    1 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT,
    1 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT,

    0 | CFF_COUNT_CLEAR_STACK, /* rlineto */
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,

    0 | CFF_COUNT_CLEAR_STACK, /* rrcurveto */
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,

    13, /* flex */
    7,
    9,
    11,

    0 | CFF_COUNT_CHECK_WIDTH, /* endchar */

    2 | CFF_COUNT_CHECK_WIDTH, /* hstem */
    2 | CFF_COUNT_CHECK_WIDTH,
    2 | CFF_COUNT_CHECK_WIDTH,
    2 | CFF_COUNT_CHECK_WIDTH,

    0 | CFF_COUNT_CHECK_WIDTH, /* hintmask */
    0 | CFF_COUNT_CHECK_WIDTH, /* cntrmask */
    0, /* dotsection */

    1, /* abs */
    2,
    2,
    2,
    1,
    0,
    2,
    1,

    1, /* blend */

    1, /* drop */
    2,
    1,
    2,
    1,

    2, /* put */
    1,
    4,
    3,

    2, /* and */
    2,
    1,
    2,
    4,

    1, /* callsubr */
    1,
    0
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********             GENERIC CHARSTRING PARSING               *********/
  /**********                                                      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_builder_init                                                   */
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
  static void
  cff_builder_init( CFF_Builder*   builder,
                    TT_Face        face,
                    CFF_Size       size,
                    CFF_GlyphSlot  glyph,
                    FT_Bool        hinting )
  {
    builder->path_begun  = 0;
    builder->load_points = 1;

    builder->face   = face;
    builder->glyph  = glyph;
    builder->memory = face->root.memory;

    if ( glyph )
    {
      FT_GlyphLoader  loader = glyph->root.internal->loader;


      builder->loader  = loader;
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
      FT_GlyphLoader_Rewind( loader );

      builder->hints_globals = 0;
      builder->hints_funcs   = 0;

      if ( hinting && size )
      {
        builder->hints_globals = size->root.internal;
        builder->hints_funcs   = glyph->root.internal->glyph_hints;
      }
    }

    if ( size )
    {
      builder->scale_x = size->root.metrics.x_scale;
      builder->scale_y = size->root.metrics.y_scale;
    }

    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_builder_done                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  static void
  cff_builder_done( CFF_Builder*  builder )
  {
    CFF_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->root.outline = *builder->base;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_compute_bias                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the bias value in dependence of the number of glyph       */
  /*    subroutines.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_subrs :: The number of glyph subroutines.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The bias value.                                                    */
  static FT_Int
  cff_compute_bias( FT_UInt  num_subrs )
  {
    FT_Int  result;


    if ( num_subrs < 1240 )
      result = 107;
    else if ( num_subrs < 33900U )
      result = 1131;
    else
      result = 32768U;

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_decoder_init                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    slot    :: The current glyph object.                               */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cff_decoder_init( CFF_Decoder*    decoder,
                    TT_Face         face,
                    CFF_Size        size,
                    CFF_GlyphSlot   slot,
                    FT_Bool         hinting,
                    FT_Render_Mode  hint_mode )
  {
    CFF_Font  cff = (CFF_Font)face->extra.data;


    /* clear everything */
    FT_MEM_ZERO( decoder, sizeof ( *decoder ) );

    /* initialize builder */
    cff_builder_init( &decoder->builder, face, size, slot, hinting );

    /* initialize Type2 decoder */
    decoder->num_globals  = cff->num_global_subrs;
    decoder->globals      = cff->global_subrs;
    decoder->globals_bias = cff_compute_bias( decoder->num_globals );

    decoder->hint_mode    = hint_mode;
  }


  /* this function is used to select the locals subrs array */
  FT_LOCAL_DEF( void )
  cff_decoder_prepare( CFF_Decoder*  decoder,
                       FT_UInt       glyph_index )
  {
    CFF_Font     cff = (CFF_Font)decoder->builder.face->extra.data;
    CFF_SubFont  sub = &cff->top_font;


    /* manage CID fonts */
    if ( cff->num_subfonts >= 1 )
    {
      FT_Byte  fd_index = cff_fd_select_get( &cff->fd_select, glyph_index );


      sub = cff->subfonts[fd_index];
    }

    decoder->num_locals    = sub->num_local_subrs;
    decoder->locals        = sub->local_subrs;
    decoder->locals_bias   = cff_compute_bias( decoder->num_locals );

    decoder->glyph_width   = sub->private_dict.default_width;
    decoder->nominal_width = sub->private_dict.nominal_width;
  }


  /* check that there is enough space for `count' more points */
  static FT_Error
  check_points( CFF_Builder*  builder,
                FT_Int        count )
  {
    return FT_GLYPHLOADER_CHECK_POINTS( builder->loader, count, 0 );
  }


  /* add a new point, do not check space */
  static void
  cff_builder_add_point( CFF_Builder*  builder,
                         FT_Pos        x,
                         FT_Pos        y,
                         FT_Byte       flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      point->x = x >> 16;
      point->y = y >> 16;
      *control = (FT_Byte)( flag ? FT_CURVE_TAG_ON : FT_CURVE_TAG_CUBIC );

      builder->last = *point;
    }

    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  static FT_Error
  cff_builder_add_point1( CFF_Builder*  builder,
                          FT_Pos        x,
                          FT_Pos        y )
  {
    FT_Error  error;


    error = check_points( builder, 1 );
    if ( !error )
      cff_builder_add_point( builder, x, y, 1 );

    return error;
  }


  /* check space for a new contour, then add it */
  static FT_Error
  cff_builder_add_contour( CFF_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return CFF_Err_Ok;
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
  static FT_Error
  cff_builder_start_point( CFF_Builder*  builder,
                           FT_Pos        x,
                           FT_Pos        y )
  {
    FT_Error  error = CFF_Err_Ok;


    /* test whether we are building a new contour */
    if ( !builder->path_begun )
    {
      builder->path_begun = 1;
      error = cff_builder_add_contour( builder );
      if ( !error )
        error = cff_builder_add_point1( builder, x, y );
    }

    return error;
  }


  /* close the current contour */
  static void
  cff_builder_close_contour( CFF_Builder*  builder )
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

      /* `delete' last point only if it coincides with the first    */
      /* point and if it is not a control point (which can happen). */
      if ( p1->x == p2->x && p1->y == p2->y )
        if ( *control == FT_CURVE_TAG_ON )
          outline->n_points--;
    }

    if ( outline->n_contours > 0 )
      outline->contours[outline->n_contours - 1] =
        (short)( outline->n_points - 1 );
  }


  static FT_Int
  cff_lookup_glyph_by_stdcharcode( CFF_Font  cff,
                                   FT_Int    charcode )
  {
    FT_UInt    n;
    FT_UShort  glyph_sid;


    /* CID-keyed fonts don't have glyph names */
    if ( !cff->charset.sids )
      return -1;

    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    /* Get code to SID mapping from `cff_standard_encoding'. */
    glyph_sid = cff_get_standard_encoding( (FT_UInt)charcode );

    for ( n = 0; n < cff->num_glyphs; n++ )
    {
      if ( cff->charset.sids[n] == glyph_sid )
        return n;
    }

    return -1;
  }


  static FT_Error
  cff_get_glyph_data( TT_Face    face,
                      FT_UInt    glyph_index,
                      FT_Byte**  pointer,
                      FT_ULong*  length )
  {
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    /* For incremental fonts get the character data using the */
    /* callback function.                                     */
    if ( face->root.internal->incremental_interface )
    {
      FT_Data   data;
      FT_Error  error =
                  face->root.internal->incremental_interface->funcs->get_glyph_data(
                    face->root.internal->incremental_interface->object,
                    glyph_index, &data );


      *pointer = (FT_Byte*)data.pointer;
      *length = data.length;

      return error;
    }
    else
#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    {
      CFF_Font  cff  = (CFF_Font)(face->extra.data);


      return cff_index_access_element( &cff->charstrings_index, glyph_index,
                                       pointer, length );
    }
  }


  static void
  cff_free_glyph_data( TT_Face    face,
                       FT_Byte**  pointer,
                       FT_ULong   length )
  {
#ifndef FT_CONFIG_OPTION_INCREMENTAL
    FT_UNUSED( length );
#endif

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    /* For incremental fonts get the character data using the */
    /* callback function.                                     */
    if ( face->root.internal->incremental_interface )
    {
      FT_Data data;


      data.pointer = *pointer;
      data.length  = length;

      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,&data );
    }
    else
#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    {
      CFF_Font  cff = (CFF_Font)(face->extra.data);


      cff_index_forget_element( &cff->charstrings_index, pointer );
    }
  }


  static FT_Error
  cff_operator_seac( CFF_Decoder*  decoder,
                     FT_Pos        adx,
                     FT_Pos        ady,
                     FT_Int        bchar,
                     FT_Int        achar )
  {
    FT_Error      error;
    CFF_Builder*  builder = &decoder->builder;
    FT_Int        bchar_index, achar_index;
    TT_Face       face = decoder->builder.face;
    FT_Vector     left_bearing, advance;
    FT_Byte*      charstring;
    FT_ULong      charstring_len;


#ifdef FT_CONFIG_OPTION_INCREMENTAL
    /* Incremental fonts don't necessarily have valid charsets.        */
    /* They use the character code, not the glyph index, in this case. */
    if ( face->root.internal->incremental_interface )
    {
      bchar_index = bchar;
      achar_index = achar;
    }
    else
#endif /* FT_CONFIG_OPTION_INCREMENTAL */
    {
      CFF_Font cff = (CFF_Font)(face->extra.data);


      bchar_index = cff_lookup_glyph_by_stdcharcode( cff, bchar );
      achar_index = cff_lookup_glyph_by_stdcharcode( cff, achar );
    }

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "cff_operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return CFF_Err_Syntax_Error;
    }

    /* If we are trying to load a composite glyph, do not load the */
    /* accent character and return the array of subglyphs.         */
    if ( builder->no_recurse )
    {
      FT_GlyphSlot    glyph  = (FT_GlyphSlot)builder->glyph;
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
      subg->arg1  = (FT_Int)( adx >> 16 );
      subg->arg2  = (FT_Int)( ady >> 16 );

      /* set up remaining glyph fields */
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = FT_GLYPH_FORMAT_COMPOSITE;

      loader->current.num_subglyphs = 2;
    }

    FT_GlyphLoader_Prepare( builder->loader );

    /* First load `bchar' in builder */
    error = cff_get_glyph_data( face, bchar_index,
                                &charstring, &charstring_len );
    if ( !error )
    {
      error = cff_decoder_parse_charstrings( decoder, charstring,
                                             charstring_len );

      if ( error )
        goto Exit;

      cff_free_glyph_data( face, &charstring, charstring_len );
    }

    /* Save the left bearing and width of the base character */
    /* as they will be erased by the next load.              */

    left_bearing = builder->left_bearing;
    advance      = builder->advance;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;

    builder->pos_x = adx;
    builder->pos_y = ady;

    /* Now load `achar' on top of the base outline. */
    error = cff_get_glyph_data( face, achar_index,
                                &charstring, &charstring_len );
    if ( !error )
    {
      error = cff_decoder_parse_charstrings( decoder, charstring,
                                             charstring_len );

      if ( error )
        goto Exit;

      cff_free_glyph_data( face, &charstring, charstring_len );
    }

    /* Restore the left side bearing and advance width */
    /* of the base character.                          */
    builder->left_bearing = left_bearing;
    builder->advance      = advance;

    builder->pos_x = 0;
    builder->pos_y = 0;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_decoder_parse_charstrings                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 2 charstrings program.                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charstring_base :: The base of the charstring stream.              */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  cff_decoder_parse_charstrings( CFF_Decoder*  decoder,
                                 FT_Byte*      charstring_base,
                                 FT_ULong      charstring_len )
  {
    FT_Error           error;
    CFF_Decoder_Zone*  zone;
    FT_Byte*           ip;
    FT_Byte*           limit;
    CFF_Builder*       builder = &decoder->builder;
    FT_Pos             x, y;
    FT_Fixed           seed;
    FT_Fixed*          stack;

    T2_Hints_Funcs     hinter;


    /* set default width */
    decoder->num_hints  = 0;
    decoder->read_width = 1;

    /* compute random seed from stack address of parameter */
    seed = (FT_Fixed)(char*)&seed           ^
           (FT_Fixed)(char*)&decoder        ^
           (FT_Fixed)(char*)&charstring_base;
    seed = ( seed ^ ( seed >> 10 ) ^ ( seed >> 20 ) ) & 0xFFFFL;
    if ( seed == 0 )
      seed = 0x7384;

    /* initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;
    stack         = decoder->top;

    hinter = (T2_Hints_Funcs)builder->hints_funcs;

    builder->path_begun = 0;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = CFF_Err_Ok;

    x = builder->pos_x;
    y = builder->pos_y;

    /* begin hints recording session, if any */
    if ( hinter )
      hinter->open( hinter->hints );

    /* now execute loop */
    while ( ip < limit )
    {
      CFF_Operator  op;
      FT_Byte       v;


      /********************************************************************/
      /*                                                                  */
      /* Decode operator or operand                                       */
      /*                                                                  */
      v = *ip++;
      if ( v >= 32 || v == 28 )
      {
        FT_Int    shift = 16;
        FT_Int32  val;


        /* this is an operand, push it on the stack */
        if ( v == 28 )
        {
          if ( ip + 1 >= limit )
            goto Syntax_Error;
          val = (FT_Short)( ( (FT_Short)ip[0] << 8 ) | ip[1] );
          ip += 2;
        }
        else if ( v < 247 )
          val = (FT_Long)v - 139;
        else if ( v < 251 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = ( (FT_Long)v - 247 ) * 256 + *ip++ + 108;
        }
        else if ( v < 255 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = -( (FT_Long)v - 251 ) * 256 - *ip++ - 108;
        }
        else
        {
          if ( ip + 3 >= limit )
            goto Syntax_Error;
          val = ( (FT_Int32)ip[0] << 24 ) |
                ( (FT_Int32)ip[1] << 16 ) |
                ( (FT_Int32)ip[2] <<  8 ) |
                            ip[3];
          ip    += 4;
          shift  = 0;
        }
        if ( decoder->top - stack >= CFF_MAX_OPERANDS )
          goto Stack_Overflow;

        val           <<= shift;
        *decoder->top++ = val;

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( !( val & 0xFFFFL ) )
          FT_TRACE4(( " %ld", (FT_Int32)( val >> 16 ) ));
        else
          FT_TRACE4(( " %.2f", val / 65536.0 ));
#endif

      }
      else
      {
        FT_Fixed*  args     = decoder->top;
        FT_Int     num_args = (FT_Int)( args - decoder->stack );
        FT_Int     req_args;


        /* find operator */
        op = cff_op_unknown;

        switch ( v )
        {
        case 1:
          op = cff_op_hstem;
          break;
        case 3:
          op = cff_op_vstem;
          break;
        case 4:
          op = cff_op_vmoveto;
          break;
        case 5:
          op = cff_op_rlineto;
          break;
        case 6:
          op = cff_op_hlineto;
          break;
        case 7:
          op = cff_op_vlineto;
          break;
        case 8:
          op = cff_op_rrcurveto;
          break;
        case 10:
          op = cff_op_callsubr;
          break;
        case 11:
          op = cff_op_return;
          break;
        case 12:
          {
            if ( ip >= limit )
              goto Syntax_Error;
            v = *ip++;

            switch ( v )
            {
            case 0:
              op = cff_op_dotsection;
              break;
            case 3:
              op = cff_op_and;
              break;
            case 4:
              op = cff_op_or;
              break;
            case 5:
              op = cff_op_not;
              break;
            case 8:
              op = cff_op_store;
              break;
            case 9:
              op = cff_op_abs;
              break;
            case 10:
              op = cff_op_add;
              break;
            case 11:
              op = cff_op_sub;
              break;
            case 12:
              op = cff_op_div;
              break;
            case 13:
              op = cff_op_load;
              break;
            case 14:
              op = cff_op_neg;
              break;
            case 15:
              op = cff_op_eq;
              break;
            case 18:
              op = cff_op_drop;
              break;
            case 20:
              op = cff_op_put;
              break;
            case 21:
              op = cff_op_get;
              break;
            case 22:
              op = cff_op_ifelse;
              break;
            case 23:
              op = cff_op_random;
              break;
            case 24:
              op = cff_op_mul;
              break;
            case 26:
              op = cff_op_sqrt;
              break;
            case 27:
              op = cff_op_dup;
              break;
            case 28:
              op = cff_op_exch;
              break;
            case 29:
              op = cff_op_index;
              break;
            case 30:
              op = cff_op_roll;
              break;
            case 34:
              op = cff_op_hflex;
              break;
            case 35:
              op = cff_op_flex;
              break;
            case 36:
              op = cff_op_hflex1;
              break;
            case 37:
              op = cff_op_flex1;
              break;
            default:
              /* decrement ip for syntax error message */
              ip--;
            }
          }
          break;
        case 14:
          op = cff_op_endchar;
          break;
        case 16:
          op = cff_op_blend;
          break;
        case 18:
          op = cff_op_hstemhm;
          break;
        case 19:
          op = cff_op_hintmask;
          break;
        case 20:
          op = cff_op_cntrmask;
          break;
        case 21:
          op = cff_op_rmoveto;
          break;
        case 22:
          op = cff_op_hmoveto;
          break;
        case 23:
          op = cff_op_vstemhm;
          break;
        case 24:
          op = cff_op_rcurveline;
          break;
        case 25:
          op = cff_op_rlinecurve;
          break;
        case 26:
          op = cff_op_vvcurveto;
          break;
        case 27:
          op = cff_op_hhcurveto;
          break;
        case 29:
          op = cff_op_callgsubr;
          break;
        case 30:
          op = cff_op_vhcurveto;
          break;
        case 31:
          op = cff_op_hvcurveto;
          break;
        default:
          ;
        }
        if ( op == cff_op_unknown )
          goto Syntax_Error;

        /* check arguments */
        req_args = cff_argument_counts[op];
        if ( req_args & CFF_COUNT_CHECK_WIDTH )
        {
          args = stack;

          if ( num_args > 0 && decoder->read_width )
          {
            /* If `nominal_width' is non-zero, the number is really a      */
            /* difference against `nominal_width'.  Else, the number here  */
            /* is truly a width, not a difference against `nominal_width'. */
            /* If the font does not set `nominal_width', then              */
            /* `nominal_width' defaults to zero, and so we can set         */
            /* `glyph_width' to `nominal_width' plus number on the stack   */
            /* -- for either case.                                         */

            FT_Int  set_width_ok;


            switch ( op )
            {
            case cff_op_hmoveto:
            case cff_op_vmoveto:
              set_width_ok = num_args & 2;
              break;

            case cff_op_hstem:
            case cff_op_vstem:
            case cff_op_hstemhm:
            case cff_op_vstemhm:
            case cff_op_rmoveto:
            case cff_op_hintmask:
            case cff_op_cntrmask:
              set_width_ok = num_args & 1;
              break;

            case cff_op_endchar:
              /* If there is a width specified for endchar, we either have */
              /* 1 argument or 5 arguments.  We like to argue.             */
              set_width_ok = ( ( num_args == 5 ) || ( num_args == 1 ) );
              break;

            default:
              set_width_ok = 0;
              break;
            }

            if ( set_width_ok )
            {
              decoder->glyph_width = decoder->nominal_width +
                                       ( stack[0] >> 16 );

              /* Consumed an argument. */
              num_args--;
              args++;
            }
          }

          decoder->read_width = 0;
          req_args            = 0;
        }

        req_args &= 15;
        if ( num_args < req_args )
          goto Stack_Underflow;
        args     -= req_args;
        num_args -= req_args;

        switch ( op )
        {
        case cff_op_hstem:
        case cff_op_vstem:
        case cff_op_hstemhm:
        case cff_op_vstemhm:
          /* the number of arguments is always even here */
          FT_TRACE4(( op == cff_op_hstem   ? " hstem"   :
                    ( op == cff_op_vstem   ? " vstem"   :
                    ( op == cff_op_hstemhm ? " hstemhm" : " vstemhm" ) ) ));

          if ( hinter )
            hinter->stems( hinter->hints,
                           ( op == cff_op_hstem || op == cff_op_hstemhm ),
                           num_args / 2,
                           args );

          decoder->num_hints += num_args / 2;
          args = stack;
          break;

        case cff_op_hintmask:
        case cff_op_cntrmask:
          FT_TRACE4(( op == cff_op_hintmask ? " hintmask" : " cntrmask" ));

          /* implement vstem when needed --                        */
          /* the specification doesn't say it, but this also works */
          /* with the 'cntrmask' operator                          */
          /*                                                       */
          if ( num_args > 0 )
          {
            if ( hinter )
              hinter->stems( hinter->hints,
                             0,
                             num_args / 2,
                             args );

            decoder->num_hints += num_args / 2;
          }

          if ( hinter )
          {
            if ( op == cff_op_hintmask )
              hinter->hintmask( hinter->hints,
                                builder->current->n_points,
                                decoder->num_hints,
                                ip );
            else
              hinter->counter( hinter->hints,
                               decoder->num_hints,
                               ip );
          }

#ifdef FT_DEBUG_LEVEL_TRACE
          {
            FT_UInt maskbyte;


            FT_TRACE4(( " " ));

            for ( maskbyte = 0;
                  maskbyte < (FT_UInt)(( decoder->num_hints + 7 ) >> 3);
                  maskbyte++, ip++ )
              FT_TRACE4(( "0x%02X", *ip ));
          }
#else
          ip += ( decoder->num_hints + 7 ) >> 3;
#endif
          if ( ip >= limit )
            goto Syntax_Error;
          args = stack;
          break;

        case cff_op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          y   += args[1];
          args = stack;
          break;

        case cff_op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          y   += args[0];
          args = stack;
          break;

        case cff_op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          args = stack;
          break;

        case cff_op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( cff_builder_start_point ( builder, x, y ) ||
               check_points( builder, num_args / 2 )     )
            goto Fail;

          if ( num_args < 2 || num_args & 1 )
            goto Stack_Underflow;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            cff_builder_add_point( builder, x, y, 1 );
            args += 2;
          }
          args = stack;
          break;

        case cff_op_hlineto:
        case cff_op_vlineto:
          {
            FT_Int  phase = ( op == cff_op_hlineto );


            FT_TRACE4(( op == cff_op_hlineto ? " hlineto"
                                             : " vlineto" ));

            if ( cff_builder_start_point ( builder, x, y ) ||
                 check_points( builder, num_args )         )
              goto Fail;

            args = stack;
            while ( args < decoder->top )
            {
              if ( phase )
                x += args[0];
              else
                y += args[0];

              if ( cff_builder_add_point1( builder, x, y ) )
                goto Fail;

              args++;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case cff_op_rrcurveto:
          FT_TRACE4(( " rrcurveto" ));

          /* check number of arguments; must be a multiple of 6 */
          if ( num_args % 6 != 0 )
            goto Stack_Underflow;

          if ( cff_builder_start_point ( builder, x, y ) ||
               check_points( builder, num_args / 2 )     )
            goto Fail;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            cff_builder_add_point( builder, x, y, 1 );
            args += 6;
          }
          args = stack;
          break;

        case cff_op_vvcurveto:
          FT_TRACE4(( " vvcurveto" ));

          if ( cff_builder_start_point( builder, x, y ) )
            goto Fail;

          args = stack;
          if ( num_args & 1 )
          {
            x += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Fail;

          while ( args < decoder->top )
          {
            y += args[0];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            cff_builder_add_point( builder, x, y, 0 );
            y += args[3];
            cff_builder_add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case cff_op_hhcurveto:
          FT_TRACE4(( " hhcurveto" ));

          if ( cff_builder_start_point( builder, x, y ) )
            goto Fail;

          args = stack;
          if ( num_args & 1 )
          {
            y += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Fail;

          while ( args < decoder->top )
          {
            x += args[0];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[3];
            cff_builder_add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case cff_op_vhcurveto:
        case cff_op_hvcurveto:
          {
            FT_Int  phase;


            FT_TRACE4(( op == cff_op_vhcurveto ? " vhcurveto"
                                               : " hvcurveto" ));

            if ( cff_builder_start_point( builder, x, y ) )
              goto Fail;

            args = stack;
            if ( num_args < 4 || ( num_args % 4 ) > 1 )
              goto Stack_Underflow;

            if ( check_points( builder, ( num_args / 4 ) * 3 ) )
              goto Stack_Underflow;

            phase = ( op == cff_op_hvcurveto );

            while ( num_args >= 4 )
            {
              num_args -= 4;
              if ( phase )
              {
                x += args[0];
                cff_builder_add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                cff_builder_add_point( builder, x, y, 0 );
                y += args[3];
                if ( num_args == 1 )
                  x += args[4];
                cff_builder_add_point( builder, x, y, 1 );
              }
              else
              {
                y += args[0];
                cff_builder_add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                cff_builder_add_point( builder, x, y, 0 );
                x += args[3];
                if ( num_args == 1 )
                  y += args[4];
                cff_builder_add_point( builder, x, y, 1 );
              }
              args  += 4;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case cff_op_rlinecurve:
          {
            FT_Int  num_lines = ( num_args - 6 ) / 2;


            FT_TRACE4(( " rlinecurve" ));

            if ( num_args < 8 || ( num_args - 6 ) & 1 )
              goto Stack_Underflow;

            if ( cff_builder_start_point( builder, x, y ) ||
                 check_points( builder, num_lines + 3 )   )
              goto Fail;

            args = stack;

            /* first, add the line segments */
            while ( num_lines > 0 )
            {
              x += args[0];
              y += args[1];
              cff_builder_add_point( builder, x, y, 1 );
              args += 2;
              num_lines--;
            }

            /* then the curve */
            x += args[0];
            y += args[1];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            cff_builder_add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            cff_builder_add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case cff_op_rcurveline:
          {
            FT_Int  num_curves = ( num_args - 2 ) / 6;


            FT_TRACE4(( " rcurveline" ));

            if ( num_args < 8 || ( num_args - 2 ) % 6 )
              goto Stack_Underflow;

            if ( cff_builder_start_point ( builder, x, y ) ||
                 check_points( builder, num_curves*3 + 2 ) )
              goto Fail;

            args = stack;

            /* first, add the curves */
            while ( num_curves > 0 )
            {
              x += args[0];
              y += args[1];
              cff_builder_add_point( builder, x, y, 0 );
              x += args[2];
              y += args[3];
              cff_builder_add_point( builder, x, y, 0 );
              x += args[4];
              y += args[5];
              cff_builder_add_point( builder, x, y, 1 );
              args += 6;
              num_curves--;
            }

            /* then the final line */
            x += args[0];
            y += args[1];
            cff_builder_add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case cff_op_hflex1:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex1" ));

            args = stack;

            /* adding five more points; 4 control points, 1 on-curve point */
            /* make sure we have enough space for the start point if it    */
            /* needs to be added                                           */
            if ( cff_builder_start_point( builder, x, y ) ||
                 check_points( builder, 6 )               )
              goto Fail;

            /* Record the starting point's y postion for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            y += args[1];
            cff_builder_add_point( builder, x, y, 0 );

            /* second control point */
            x += args[2];
            y += args[3];
            cff_builder_add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[4];
            cff_builder_add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[5];
            cff_builder_add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[6];
            y += args[7];
            cff_builder_add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start   */
            x += args[8];
            y  = start_y;
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_hflex:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex" ));

            args = stack;

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( cff_builder_start_point( builder, x, y ) ||
                 check_points( builder, 6 )               )
              goto Fail;

            /* record the starting point's y-position for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            cff_builder_add_point( builder, x, y, 0 );

            /* second control point */
            x += args[1];
            y += args[2];
            cff_builder_add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[3];
            cff_builder_add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[4];
            cff_builder_add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[5];
            y  = start_y;
            cff_builder_add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start point's */
            /* y-value -- we don't add this point, though               */
            x += args[6];
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_flex1:
          {
            FT_Pos    start_x, start_y; /* record start x, y values for */
                                        /* alter use                                */
            FT_Fixed  dx = 0, dy = 0;   /* used in horizontal/vertical  */
                                        /* algorithm below              */
            FT_Int    horizontal, count;


            FT_TRACE4(( " flex1" ));

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( cff_builder_start_point( builder, x, y ) ||
                 check_points( builder, 6 )               )
              goto Fail;

            /* record the starting point's x, y postion for later use */
            start_x = x;
            start_y = y;

            /* XXX: figure out whether this is supposed to be a horizontal */
            /*      or vertical flex; the Type 2 specification is vague... */

            args = stack;

            /* grab up to the last argument */
            for ( count = 5; count > 0; count-- )
            {
              dx += args[0];
              dy += args[1];
              args += 2;
            }

            /* rewind */
            args = stack;

            if ( dx < 0 ) dx = -dx;
            if ( dy < 0 ) dy = -dy;

            /* strange test, but here it is... */
            horizontal = ( dx > dy );

            for ( count = 5; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              cff_builder_add_point( builder, x, y, (FT_Bool)( count == 3 ) );
              args += 2;
            }

            /* is last operand an x- or y-delta? */
            if ( horizontal )
            {
              x += args[0];
              y  = start_y;
            }
            else
            {
              x  = start_x;
              y += args[0];
            }

            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
           }

        case cff_op_flex:
          {
            FT_UInt  count;


            FT_TRACE4(( " flex" ));

            if ( cff_builder_start_point( builder, x, y ) ||
                 check_points( builder, 6 )               )
              goto Fail;

            args = stack;
            for ( count = 6; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              cff_builder_add_point( builder, x, y,
                                     (FT_Bool)( count == 4 || count == 1 ) );
              args += 2;
            }

            args = stack;
          }
          break;

        case cff_op_endchar:
          FT_TRACE4(( " endchar" ));

          /* We are going to emulate the seac operator. */
          if ( num_args == 4 )
          {
            /* Save glyph width so that the subglyphs don't overwrite it. */
            FT_Pos  glyph_width = decoder->glyph_width;


            error = cff_operator_seac( decoder,
                                       args[0],
                                       args[1],
                                       (FT_Int)( args[2] >> 16 ),
                                       (FT_Int)( args[3] >> 16 ) );
            args += 4;

            decoder->glyph_width = glyph_width;
          }
          else
          {
            if ( !error )
              error = CFF_Err_Ok;

            cff_builder_close_contour( builder );

            /* close hints recording session */
            if ( hinter )
            {
              if ( hinter->close( hinter->hints,
                                  builder->current->n_points ) )
                goto Syntax_Error;

              /* apply hints to the loaded glyph outline now */
              hinter->apply( hinter->hints,
                             builder->current,
                             (PSH_Globals)builder->hints_globals,
                             decoder->hint_mode );
            }

            /* add current outline to the glyph slot */
            FT_GlyphLoader_Add( builder->loader );
          }

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return error;

        case cff_op_abs:
          FT_TRACE4(( " abs" ));

          if ( args[0] < 0 )
            args[0] = -args[0];
          args++;
          break;

        case cff_op_add:
          FT_TRACE4(( " add" ));

          args[0] += args[1];
          args++;
          break;

        case cff_op_sub:
          FT_TRACE4(( " sub" ));

          args[0] -= args[1];
          args++;
          break;

        case cff_op_div:
          FT_TRACE4(( " div" ));

          args[0] = FT_DivFix( args[0], args[1] );
          args++;
          break;

        case cff_op_neg:
          FT_TRACE4(( " neg" ));

          args[0] = -args[0];
          args++;
          break;

        case cff_op_random:
          {
            FT_Fixed  Rand;


            FT_TRACE4(( " rand" ));

            Rand = seed;
            if ( Rand >= 0x8000L )
              Rand++;

            args[0] = Rand;
            seed    = FT_MulFix( seed, 0x10000L - seed );
            if ( seed == 0 )
              seed += 0x2873;
            args++;
          }
          break;

        case cff_op_mul:
          FT_TRACE4(( " mul" ));

          args[0] = FT_MulFix( args[0], args[1] );
          args++;
          break;

        case cff_op_sqrt:
          FT_TRACE4(( " sqrt" ));

          if ( args[0] > 0 )
          {
            FT_Int    count = 9;
            FT_Fixed  root  = args[0];
            FT_Fixed  new_root;


            for (;;)
            {
              new_root = ( root + FT_DivFix( args[0], root ) + 1 ) >> 1;
              if ( new_root == root || count <= 0 )
                break;
              root = new_root;
            }
            args[0] = new_root;
          }
          else
            args[0] = 0;
          args++;
          break;

        case cff_op_drop:
          /* nothing */
          FT_TRACE4(( " drop" ));

          break;

        case cff_op_exch:
          {
            FT_Fixed  tmp;


            FT_TRACE4(( " exch" ));

            tmp     = args[0];
            args[0] = args[1];
            args[1] = tmp;
            args   += 2;
          }
          break;

        case cff_op_index:
          {
            FT_Int  idx = (FT_Int)( args[0] >> 16 );


            FT_TRACE4(( " index" ));

            if ( idx < 0 )
              idx = 0;
            else if ( idx > num_args - 2 )
              idx = num_args - 2;
            args[0] = args[-( idx + 1 )];
            args++;
          }
          break;

        case cff_op_roll:
          {
            FT_Int  count = (FT_Int)( args[0] >> 16 );
            FT_Int  idx   = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " roll" ));

            if ( count <= 0 )
              count = 1;

            args -= count;
            if ( args < stack )
              goto Stack_Underflow;

            if ( idx >= 0 )
            {
              while ( idx > 0 )
              {
                FT_Fixed  tmp = args[count - 1];
                FT_Int    i;


                for ( i = count - 2; i >= 0; i-- )
                  args[i + 1] = args[i];
                args[0] = tmp;
                idx--;
              }
            }
            else
            {
              while ( idx < 0 )
              {
                FT_Fixed  tmp = args[0];
                FT_Int    i;


                for ( i = 0; i < count - 1; i++ )
                  args[i] = args[i + 1];
                args[count - 1] = tmp;
                idx++;
              }
            }
            args += count;
          }
          break;

        case cff_op_dup:
          FT_TRACE4(( " dup" ));

          args[1] = args[0];
          args++;
          break;

        case cff_op_put:
          {
            FT_Fixed  val = args[0];
            FT_Int    idx = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " put" ));

            if ( idx >= 0 && idx < decoder->len_buildchar )
              decoder->buildchar[idx] = val;
          }
          break;

        case cff_op_get:
          {
            FT_Int    idx = (FT_Int)( args[0] >> 16 );
            FT_Fixed  val = 0;


            FT_TRACE4(( " get" ));

            if ( idx >= 0 && idx < decoder->len_buildchar )
              val = decoder->buildchar[idx];

            args[0] = val;
            args++;
          }
          break;

        case cff_op_store:
          FT_TRACE4(( " store "));

          goto Unimplemented;

        case cff_op_load:
          FT_TRACE4(( " load" ));

          goto Unimplemented;

        case cff_op_dotsection:
          /* this operator is deprecated and ignored by the parser */
          FT_TRACE4(( " dotsection" ));
          break;

        case cff_op_and:
          {
            FT_Fixed  cond = args[0] && args[1];


            FT_TRACE4(( " and" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_or:
          {
            FT_Fixed  cond = args[0] || args[1];


            FT_TRACE4(( " or" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_eq:
          {
            FT_Fixed  cond = !args[0];


            FT_TRACE4(( " eq" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_ifelse:
          {
            FT_Fixed  cond = ( args[2] <= args[3] );


            FT_TRACE4(( " ifelse" ));

            if ( !cond )
              args[0] = args[1];
            args++;
          }
          break;

        case cff_op_callsubr:
          {
            FT_UInt  idx = (FT_UInt)( ( args[0] >> 16 ) +
                                      decoder->locals_bias );


            FT_TRACE4(( " callsubr(%d)", idx ));

            if ( idx >= decoder->num_locals )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:" ));
              FT_ERROR(( "  invalid local subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= CFF_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:"
                         " too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->locals[idx];
            zone->limit  = decoder->locals[idx + 1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:"
                         " invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case cff_op_callgsubr:
          {
            FT_UInt  idx = (FT_UInt)( ( args[0] >> 16 ) +
                                      decoder->globals_bias );


            FT_TRACE4(( " callgsubr(%d)", idx ));

            if ( idx >= decoder->num_globals )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:" ));
              FT_ERROR(( " invalid global subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= CFF_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:"
                         " too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->globals[idx];
            zone->limit  = decoder->globals[idx + 1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "cff_decoder_parse_charstrings:"
                         " invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case cff_op_return:
          FT_TRACE4(( " return" ));

          if ( decoder->zone <= decoder->zones )
          {
            FT_ERROR(( "cff_decoder_parse_charstrings:"
                       " unexpected return\n" ));
            goto Syntax_Error;
          }

          decoder->zone--;
          zone  = decoder->zone;
          ip    = zone->cursor;
          limit = zone->limit;
          break;

        default:
        Unimplemented:
          FT_ERROR(( "Unimplemented opcode: %d", ip[-1] ));

          if ( ip[-1] == 12 )
            FT_ERROR(( " %d", ip[0] ));
          FT_ERROR(( "\n" ));

          return CFF_Err_Unimplemented_Feature;
        }

      decoder->top = args;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

  Fail:
    return error;

  Syntax_Error:
    FT_TRACE4(( "cff_decoder_parse_charstrings: syntax error!" ));
    return CFF_Err_Invalid_File_Format;

  Stack_Underflow:
    FT_TRACE4(( "cff_decoder_parse_charstrings: stack underflow!" ));
    return CFF_Err_Too_Few_Arguments;

  Stack_Overflow:
    FT_TRACE4(( "cff_decoder_parse_charstrings: stack overflow!" ));
    return CFF_Err_Stack_Overflow;
  }


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


#if 0 /* unused until we support pure CFF fonts */


  FT_LOCAL_DEF( FT_Error )
  cff_compute_max_advance( TT_Face  face,
                           FT_Int*  max_advance )
  {
    FT_Error     error = CFF_Err_Ok;
    CFF_Decoder  decoder;
    FT_Int       glyph_index;
    CFF_Font     cff = (CFF_Font)face->other;


    *max_advance = 0;

    /* Initialize load decoder */
    cff_decoder_init( &decoder, face, 0, 0, 0, 0 );

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* For each glyph, parse the glyph charstring and extract */
    /* the advance width.                                     */
    for ( glyph_index = 0; glyph_index < face->root.num_glyphs;
          glyph_index++ )
    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      /* now get load the unscaled outline */
      error = cff_get_glyph_data( face, glyph_index,
                                  &charstring, &charstring_len );
      if ( !error )
      {
        cff_decoder_prepare( &decoder, glyph_index );
        error = cff_decoder_parse_charstrings( &decoder,
                                               charstring, charstring_len );

        cff_free_glyph_data( face, &charstring, &charstring_len );
      }

      /* ignore the error if one has occurred -- skip to next glyph */
      error = CFF_Err_Ok;
    }

    *max_advance = decoder.builder.advance.x;

    return CFF_Err_Ok;
  }


#endif /* 0 */


  FT_LOCAL_DEF( FT_Error )
  cff_slot_load( CFF_GlyphSlot  glyph,
                 CFF_Size       size,
                 FT_UInt        glyph_index,
                 FT_Int32       load_flags )
  {
    FT_Error      error;
    CFF_Decoder   decoder;
    TT_Face       face     = (TT_Face)glyph->root.face;
    FT_Bool       hinting;
    CFF_Font      cff      = (CFF_Font)face->extra.data;

    FT_Matrix     font_matrix;
    FT_Vector     font_offset;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = 0x10000L;
    glyph->y_scale = 0x10000L;
    if ( size )
    {
      glyph->x_scale = size->root.metrics.x_scale;
      glyph->y_scale = size->root.metrics.y_scale;
    }

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* try to load embedded bitmap if any              */
    /*                                                 */
    /* XXX: The convention should be emphasized in     */
    /*      the documents because it can be confusing. */
    if ( size )
    {
      CFF_Face      cff_face = (CFF_Face)size->root.face;
      SFNT_Service  sfnt     = (SFNT_Service)cff_face->sfnt;
      FT_Stream     stream   = cff_face->root.stream;


      if ( size->strike_index != 0xFFFFFFFFUL      &&
           sfnt->load_eblc                         &&
           ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )
      {
        TT_SBit_MetricsRec  metrics;


        error = sfnt->load_sbit_image( face,
                                       size->strike_index,
                                       glyph_index,
                                       (FT_Int)load_flags,
                                       stream,
                                       &glyph->root.bitmap,
                                       &metrics );

        if ( !error )
        {
          glyph->root.outline.n_points   = 0;
          glyph->root.outline.n_contours = 0;

          glyph->root.metrics.width  = (FT_Pos)metrics.width  << 6;
          glyph->root.metrics.height = (FT_Pos)metrics.height << 6;

          glyph->root.metrics.horiBearingX = (FT_Pos)metrics.horiBearingX << 6;
          glyph->root.metrics.horiBearingY = (FT_Pos)metrics.horiBearingY << 6;
          glyph->root.metrics.horiAdvance  = (FT_Pos)metrics.horiAdvance  << 6;

          glyph->root.metrics.vertBearingX = (FT_Pos)metrics.vertBearingX << 6;
          glyph->root.metrics.vertBearingY = (FT_Pos)metrics.vertBearingY << 6;
          glyph->root.metrics.vertAdvance  = (FT_Pos)metrics.vertAdvance  << 6;

          glyph->root.format = FT_GLYPH_FORMAT_BITMAP;

          if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
          {
            glyph->root.bitmap_left = metrics.vertBearingX;
            glyph->root.bitmap_top  = metrics.vertBearingY;
          }
          else
          {
            glyph->root.bitmap_left = metrics.horiBearingX;
            glyph->root.bitmap_top  = metrics.horiBearingY;
          }
          return error;
        }
      }
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* return immediately if we only want the embedded bitmaps */
    if ( load_flags & FT_LOAD_SBITS_ONLY )
      return CFF_Err_Invalid_Argument;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    glyph->root.format = FT_GLYPH_FORMAT_OUTLINE;  /* by default */

    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      /* in a CID-keyed font, consider `glyph_index' as a CID and map */
      /* it immediately to the real glyph_index -- if it isn't a      */
      /* subsetted font, glyph_indices and CIDs are identical, though */
      if ( cff->top_font.font_dict.cid_registry != 0xFFFFU &&
           cff->charset.cids )
      {
        if ( glyph_index < cff->charset.max_cid )
          glyph_index = cff->charset.cids[glyph_index];
        else
          glyph_index = 0;
      }

      cff_decoder_init( &decoder, face, size, glyph, hinting,
                        FT_LOAD_TARGET_MODE( load_flags ) );

      decoder.builder.no_recurse =
        (FT_Bool)( ( load_flags & FT_LOAD_NO_RECURSE ) != 0 );

      /* now load the unscaled outline */
      error = cff_get_glyph_data( face, glyph_index,
                                  &charstring, &charstring_len );
      if ( !error )
      {
        cff_decoder_prepare( &decoder, glyph_index );
        error = cff_decoder_parse_charstrings( &decoder,
                                               charstring, charstring_len );

        cff_free_glyph_data( face, &charstring, charstring_len );


#ifdef FT_CONFIG_OPTION_INCREMENTAL
        /* Control data and length may not be available for incremental   */
        /* fonts.                                                         */
        if ( face->root.internal->incremental_interface )
        {
          glyph->root.control_data = 0;
          glyph->root.control_len = 0;
        }
        else
#endif /* FT_CONFIG_OPTION_INCREMENTAL */

        /* We set control_data and control_len if charstrings is loaded.  */
        /* See how charstring loads at cff_index_access_element() in      */
        /* cffload.c.                                                     */
        {
          CFF_IndexRec csindex = cff->charstrings_index;


          glyph->root.control_data =
            csindex.bytes + csindex.offsets[glyph_index] - 1;
          glyph->root.control_len =
            charstring_len;
        }
      }

      /* save new glyph tables */
      cff_builder_done( &decoder.builder );
    }

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* Incremental fonts can optionally override the metrics. */
    if ( !error                                                              &&
         face->root.internal->incremental_interface                          &&
         face->root.internal->incremental_interface->funcs->get_glyph_metrics )
    {
      FT_Incremental_MetricsRec  metrics;


      metrics.bearing_x = decoder.builder.left_bearing.x;
      metrics.bearing_y = decoder.builder.left_bearing.y;
      metrics.advance   = decoder.builder.advance.x;
      error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                face->root.internal->incremental_interface->object,
                glyph_index, FALSE, &metrics );
      decoder.builder.left_bearing.x = metrics.bearing_x;
      decoder.builder.left_bearing.y = metrics.bearing_y;
      decoder.builder.advance.x      = metrics.advance;
      decoder.builder.advance.y      = 0;
    }

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    if ( cff->num_subfonts >= 1 )
    {
      FT_Byte  fd_index = cff_fd_select_get( &cff->fd_select, glyph_index );


      font_matrix = cff->subfonts[fd_index]->font_dict.font_matrix;
      font_offset = cff->subfonts[fd_index]->font_dict.font_offset;
    }
    else
    {
      font_matrix = cff->top_font.font_dict.font_matrix;
      font_offset = cff->top_font.font_dict.font_offset;
    }

    /* Now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax.                                   */
    if ( !error )
    {
      /* For composite glyphs, return only left side bearing and */
      /* advance width.                                          */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_Slot_Internal  internal = glyph->root.internal;


        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.glyph_width;
        internal->glyph_matrix           = font_matrix;
        internal->glyph_delta            = font_offset;
        internal->glyph_transformed      = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;
        FT_Vector          advance;
        FT_Bool            has_vertical_info;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance                    = decoder.glyph_width;
        glyph->root.linearHoriAdvance           = decoder.glyph_width;
        glyph->root.internal->glyph_transformed = 0;

        has_vertical_info = FT_BOOL( face->vertical_info                   &&
                                     face->vertical.number_Of_VMetrics > 0 &&
                                     face->vertical.long_metrics != 0 );

        /* get the vertical metrics from the vtmx table if we have one */
        if ( has_vertical_info )
        {
          FT_Short   vertBearingY = 0;
          FT_UShort  vertAdvance  = 0;


          ( (SFNT_Service)face->sfnt )->get_metrics( face, 1,
                                                     glyph_index,
                                                     &vertBearingY,
                                                     &vertAdvance );
          metrics->vertBearingY = vertBearingY;
          metrics->vertAdvance  = vertAdvance;
        }
        else
        {
          /* make up vertical ones */
          if ( face->os2.version != 0xFFFFU )
            metrics->vertAdvance = (FT_Pos)( face->os2.sTypoAscender -
                                             face->os2.sTypoDescender );
          else
            metrics->vertAdvance = (FT_Pos)( face->horizontal.Ascender -
                                             face->horizontal.Descender );
        }

        glyph->root.linearVertAdvance = metrics->vertAdvance;

        glyph->root.format = FT_GLYPH_FORMAT_OUTLINE;

        glyph->root.outline.flags = 0;
        if ( size && size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= FT_OUTLINE_HIGH_PRECISION;

        glyph->root.outline.flags |= FT_OUTLINE_REVERSE_FILL;

        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline, &font_matrix );

        FT_Outline_Translate( &glyph->root.outline,
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
          FT_Outline*  cur     = &glyph->root.outline;
          FT_Vector*   vec     = cur->points;
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
          metrics->horiAdvance = FT_MulFix( metrics->horiAdvance, x_scale );
          metrics->vertAdvance = FT_MulFix( metrics->vertAdvance, y_scale );
        }

        /* compute the other metrics */
        FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;

        if ( has_vertical_info )
          metrics->vertBearingX = -metrics->width / 2;
        else
          ft_synthesize_vertical_metrics( metrics,
                                          metrics->vertAdvance );
      }
    }

    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  cffcmap.c                                                              */
/*                                                                         */
/*    CFF character mapping table (cmap) support (body).                   */
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


#include "cffcmap.h"
#include "cffload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           CFF STANDARD (AND EXPERT) ENCODING CMAPS            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_encoding_init( CFF_CMapStd  cmap )
  {
    TT_Face       face     = (TT_Face)FT_CMAP_FACE( cmap );
    CFF_Font      cff      = (CFF_Font)face->extra.data;
    CFF_Encoding  encoding = &cff->encoding;


    cmap->gids  = encoding->codes;

    return 0;
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_encoding_done( CFF_CMapStd  cmap )
  {
    cmap->gids  = NULL;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_encoding_char_index( CFF_CMapStd  cmap,
                                FT_UInt32    char_code )
  {
    FT_UInt  result = 0;


    if ( char_code < 256 )
      result = cmap->gids[char_code];

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_encoding_char_next( CFF_CMapStd   cmap,
                               FT_UInt32    *pchar_code )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code;


    *pchar_code = 0;

    if ( char_code < 255 )
    {
      FT_UInt  code = (FT_UInt)(char_code + 1);


      for (;;)
      {
        if ( code >= 256 )
          break;

        result = cmap->gids[code];
        if ( result != 0 )
        {
          *pchar_code = code;
          break;
        }

        code++;
      }
    }
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  cff_cmap_encoding_class_rec =
  {
    sizeof ( CFF_CMapStdRec ),

    (FT_CMap_InitFunc)     cff_cmap_encoding_init,
    (FT_CMap_DoneFunc)     cff_cmap_encoding_done,
    (FT_CMap_CharIndexFunc)cff_cmap_encoding_char_index,
    (FT_CMap_CharNextFunc) cff_cmap_encoding_char_next
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              CFF SYNTHETIC UNICODE ENCODING CMAP              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( const char* )
  cff_sid_to_glyph_name( CFF_Font  cff,
                         FT_UInt   idx )
  {
    CFF_Charset         charset = &cff->charset;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;
    FT_UInt             sid     = charset->sids[idx];


    return cff_index_get_sid_string( &cff->string_index, sid, psnames );
  }


  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_unicode_init( PS_Unicodes  unicodes )
  {
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    FT_Memory           memory  = FT_FACE_MEMORY( face );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    CFF_Charset         charset = &cff->charset;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    /* can't build Unicode map for CID-keyed font */
    if ( !charset->sids )
      return CFF_Err_Invalid_Argument;

    return psnames->unicodes_init( memory,
                                   unicodes,
                                   cff->num_glyphs,
                                   (PS_Glyph_NameFunc)&cff_sid_to_glyph_name,
                                   (FT_Pointer)cff );
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_unicode_done( PS_Unicodes  unicodes )
  {
    FT_Face    face   = FT_CMAP_FACE( unicodes );
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( unicodes->maps );
    unicodes->num_maps = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_unicode_char_index( PS_Unicodes  unicodes,
                               FT_UInt32    char_code )
  {
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    return psnames->unicodes_char_index( unicodes, char_code );
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_unicode_char_next( PS_Unicodes  unicodes,
                              FT_UInt32   *pchar_code )
  {
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    return psnames->unicodes_char_next( unicodes, pchar_code );
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  cff_cmap_unicode_class_rec =
  {
    sizeof ( PS_UnicodesRec ),

    (FT_CMap_InitFunc)     cff_cmap_unicode_init,
    (FT_CMap_DoneFunc)     cff_cmap_unicode_done,
    (FT_CMap_CharIndexFunc)cff_cmap_unicode_char_index,
    (FT_CMap_CharNextFunc) cff_cmap_unicode_char_next
  };


/* END */


/* END */

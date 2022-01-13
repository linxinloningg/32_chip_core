/***************************************************************************/
/*                                                                         */
/*  truetype.c                                                             */
/*                                                                         */
/*    FreeType TrueType driver component (body only).                      */
/*                                                                         */
/*  Copyright 1996-2001, 2004 by                                           */
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
/*  ttdriver.c                                                             */
/*                                                                         */
/*    TrueType font driver implementation (body).                          */
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
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_SERVICE_XFREE86_NAME_H

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include FT_MULTIPLE_MASTERS_H
#include FT_SERVICE_MULTIPLE_MASTERS_H
#endif

#include FT_SERVICE_TRUETYPE_ENGINE_H

#include "ttdriver.h"
#include "ttgload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttdriver


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
  /*    tt_get_kerning                                                     */
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
  static FT_Error
  tt_get_kerning( FT_Face     ttface,          /* TT_Face */
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

    return 0;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           S I Z E S                             ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  static FT_Error
  tt_size_select( FT_Size   size,
                  FT_ULong  strike_index )
  {
    TT_Face   ttface = (TT_Face)size->face;
    TT_Size   ttsize = (TT_Size)size;
    FT_Error  error  = TT_Err_Ok;


    ttsize->strike_index = strike_index;

    if ( FT_IS_SCALABLE( size->face ) )
    {
      /* use the scaled metrics, even when tt_size_reset fails */
      FT_Select_Metrics( size->face, strike_index );

      tt_size_reset( ttsize );
    }
    else
    {
      SFNT_Service      sfnt    = (SFNT_Service) ttface->sfnt;
      FT_Size_Metrics*  metrics = &size->metrics;


      error = sfnt->load_strike_metrics( ttface, strike_index, metrics );
      if ( error )
        ttsize->strike_index = 0xFFFFFFFFUL;
    }

    return error;
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  static FT_Error
  tt_size_request( FT_Size          size,
                   FT_Size_Request  req )
  {
    TT_Size   ttsize = (TT_Size)size;
    FT_Error  error  = TT_Err_Ok;


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    if ( FT_HAS_FIXED_SIZES( size->face ) )
    {
      TT_Face       ttface = (TT_Face)size->face;
      SFNT_Service  sfnt   = (SFNT_Service) ttface->sfnt;
      FT_ULong      index;


      error = sfnt->set_sbit_strike( ttface, req, &index );

      if ( error )
        ttsize->strike_index = 0xFFFFFFFFUL;
      else
        return tt_size_select( size, index );
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    FT_Request_Metrics( size->face, req );

    if ( FT_IS_SCALABLE( size->face ) )
      error = tt_size_reset( ttsize );

    return error;
  }


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
  /*                   FTLOAD_??? constants can be used to control the     */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_Glyph( FT_GlyphSlot  ttslot,         /* TT_GlyphSlot */
              FT_Size       ttsize,         /* TT_Size      */
              FT_UInt       glyph_index,
              FT_Int32      load_flags )
  {
    TT_GlyphSlot  slot = (TT_GlyphSlot)ttslot;
    TT_Size       size = (TT_Size)ttsize;
    FT_Error      error;


    if ( !slot )
      return TT_Err_Invalid_Slot_Handle;

    if ( !size )
      return TT_Err_Invalid_Size_Handle;

    if ( load_flags & ( FT_LOAD_NO_RECURSE | FT_LOAD_NO_SCALE ) )
    {
      load_flags |= FT_LOAD_NO_HINTING |
                    FT_LOAD_NO_BITMAP  |
                    FT_LOAD_NO_SCALE;
    }

    /* now load the glyph outline if necessary */
    error = TT_Load_Glyph( size, slot, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


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

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
  static const FT_Service_MultiMastersRec  tt_service_gx_multi_masters =
  {
    (FT_Get_MM_Func)        NULL,
    (FT_Set_MM_Design_Func) NULL,
    (FT_Set_MM_Blend_Func)  TT_Set_MM_Blend,
    (FT_Get_MM_Var_Func)    TT_Get_MM_Var,
    (FT_Set_Var_Design_Func)TT_Set_Var_Design
  };
#endif

  static const FT_Service_TrueTypeEngineRec  tt_service_truetype_engine =
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_TRUETYPE_ENGINE_TYPE_UNPATENTED
#else
    FT_TRUETYPE_ENGINE_TYPE_PATENTED
#endif

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_TRUETYPE_ENGINE_TYPE_NONE

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
  };

  static const FT_ServiceDescRec  tt_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME,       FT_XF86_FORMAT_TRUETYPE },
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    { FT_SERVICE_ID_MULTI_MASTERS,   &tt_service_gx_multi_masters },
#endif
    { FT_SERVICE_ID_TRUETYPE_ENGINE, &tt_service_truetype_engine },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  tt_get_interface( FT_Module    driver,    /* TT_Driver */
                    const char*  tt_interface )
  {
    FT_Module_Interface  result;
    FT_Module            sfntd;
    SFNT_Service         sfnt;


    result = ft_service_list_lookup( tt_services, tt_interface );
    if ( result != NULL )
      return result;

    /* only return the default interface from the SFNT module */
    sfntd = FT_Get_Module( driver->library, "sfnt" );
    if ( sfntd )
    {
      sfnt = (SFNT_Service)( sfntd->clazz->module_interface );
      if ( sfnt )
        return sfnt->get_interface( driver, tt_interface );
    }

    return 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  tt_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER        |
      FT_MODULE_DRIVER_SCALABLE    |
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      FT_MODULE_DRIVER_HAS_HINTER,
#else
      0,
#endif

      sizeof ( TT_DriverRec ),

      "truetype",      /* driver name                           */
      0x10000L,        /* driver version == 1.0                 */
      0x20000L,        /* driver requires FreeType 2.0 or above */

      (void*)0,        /* driver specific interface */

      tt_driver_init,
      tt_driver_done,
      tt_get_interface,
    },

    sizeof ( TT_FaceRec ),
    sizeof ( TT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),

    tt_face_init,
    tt_face_done,
    tt_size_init,
    tt_size_done,
    tt_slot_init,
    0,                      /* FT_Slot_DoneFunc */

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif
    Load_Glyph,

    tt_get_kerning,
    0,                      /* FT_Face_AttachFunc      */
    0,                      /* FT_Face_GetAdvancesFunc */

    tt_size_request,
#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    tt_size_select
#else
    0                       /* FT_Size_SelectFunc      */
#endif
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttpload.c                                                              */
/*                                                                         */
/*    TrueType-specific tables loader (body).                              */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2004, 2005, 2006 by                         */
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
#include FT_TRUETYPE_TAGS_H

#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttpload


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_loca                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the locations table.                                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_loca( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error  error;
    FT_ULong  table_len;


    /* we need the size of the `glyf' table for malformed `loca' tables */
    error = face->goto_table( face, TTAG_glyf, stream, &face->glyf_len );
    if ( error )
      goto Exit;

    FT_TRACE2(( "Locations " ));
    error = face->goto_table( face, TTAG_loca, stream, &table_len );
    if ( error )
    {
      error = TT_Err_Locations_Missing;
      goto Exit;
    }

    if ( face->header.Index_To_Loc_Format != 0 )
    {
      if ( table_len >= 0x40000L )
      {
        FT_TRACE2(( "table too large!\n" ));
        error = TT_Err_Invalid_Table;
        goto Exit;
      }
      face->num_locations = (FT_UInt)( table_len >> 2 );
    }
    else
    {
      if ( table_len >= 0x20000L )
      {
        FT_TRACE2(( "table too large!\n" ));
        error = TT_Err_Invalid_Table;
        goto Exit;
      }
      face->num_locations = (FT_UInt)( table_len >> 1 );
    }

    /*
     * Extract the frame.  We don't need to decompress it since
     * we are able to parse it directly.
     */
    if ( FT_FRAME_EXTRACT( table_len, face->glyph_locations ) )
      goto Exit;

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_ULong )
  tt_face_get_location( TT_Face   face,
                        FT_UInt   gindex,
                        FT_UInt  *asize )
  {
    FT_ULong  pos1, pos2;
    FT_Byte*  p;
    FT_Byte*  p_limit;


    pos1 = pos2 = 0;

    if ( gindex < face->num_locations )
    {
      if ( face->header.Index_To_Loc_Format != 0 )
      {
        p       = face->glyph_locations + gindex * 4;
        p_limit = face->glyph_locations + face->num_locations * 4;

        pos1 = FT_NEXT_ULONG( p );
        pos2 = pos1;

        if ( p + 4 <= p_limit )
          pos2 = FT_NEXT_ULONG( p );
      }
      else
      {
        p       = face->glyph_locations + gindex * 2;
        p_limit = face->glyph_locations + face->num_locations * 2;

        pos1 = FT_NEXT_USHORT( p );
        pos2 = pos1;

        if ( p + 2 <= p_limit )
          pos2 = FT_NEXT_USHORT( p );

        pos1 <<= 1;
        pos2 <<= 1;
      }
    }

    /* It isn't mentioned explicitly that the `loca' table must be  */
    /* ordered, but implicitly it refers to the length of an entry  */
    /* as the difference between the current and the next position. */
    /* Anyway, there do exist (malformed) fonts which don't obey    */
    /* this rule, so we are only able to provide an upper bound for */
    /* the size.                                                    */
    if ( pos2 >= pos1 )
      *asize = (FT_UInt)( pos2 - pos1 );
    else
      *asize = (FT_UInt)( face->glyf_len - pos1 );

    return pos1;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_loca( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;


    FT_FRAME_RELEASE( face->glyph_locations );
    face->num_locations = 0;
  }


#else /* !FT_OPTIMIZE_MEMORY */


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_loca( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_Short   LongOffsets;
    FT_ULong   table_len;


    /* we need the size of the `glyf' table for malformed `loca' tables */
    error = face->goto_table( face, TTAG_glyf, stream, &face->glyf_len );
    if ( error )
      goto Exit;

    FT_TRACE2(( "Locations " ));
    LongOffsets = face->header.Index_To_Loc_Format;

    error = face->goto_table( face, TTAG_loca, stream, &table_len );
    if ( error )
    {
      error = TT_Err_Locations_Missing;
      goto Exit;
    }

    if ( LongOffsets != 0 )
    {
      face->num_locations = (FT_UShort)( table_len >> 2 );

      FT_TRACE2(( "(32bit offsets): %12d ", face->num_locations ));

      if ( FT_NEW_ARRAY( face->glyph_locations, face->num_locations ) )
        goto Exit;

      if ( FT_FRAME_ENTER( face->num_locations * 4L ) )
        goto Exit;

      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = FT_GET_LONG();
      }

      FT_FRAME_EXIT();
    }
    else
    {
      face->num_locations = (FT_UShort)( table_len >> 1 );

      FT_TRACE2(( "(16bit offsets): %12d ", face->num_locations ));

      if ( FT_NEW_ARRAY( face->glyph_locations, face->num_locations ) )
        goto Exit;

      if ( FT_FRAME_ENTER( face->num_locations * 2L ) )
        goto Exit;

      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = (FT_Long)( (FT_ULong)FT_GET_USHORT() * 2 );
      }

      FT_FRAME_EXIT();
    }

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_ULong )
  tt_face_get_location( TT_Face   face,
                        FT_UInt   gindex,
                        FT_UInt  *asize )
  {
    FT_ULong  offset;
    FT_UInt   count;


    offset = face->glyph_locations[gindex];
    count  = 0;

    if ( gindex < (FT_UInt)face->num_locations - 1 )
    {
      FT_ULong  offset1 = face->glyph_locations[gindex + 1];


      /* It isn't mentioned explicitly that the `loca' table must be  */
      /* ordered, but implicitly it refers to the length of an entry  */
      /* as the difference between the current and the next position. */
      /* Anyway, there do exist (malformed) fonts which don't obey    */
      /* this rule, so we are only able to provide an upper bound for */
      /* the size.                                                    */
      if ( offset1 >= offset )
        count = (FT_UInt)( offset1 - offset );
      else
        count = (FT_UInt)( face->glyf_len - offset );
    }

    *asize = count;
    return offset;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_loca( TT_Face  face )
  {
    FT_Memory  memory = face->root.memory;


    FT_FREE( face->glyph_locations );
    face->num_locations = 0;
  }


#endif /* !FT_OPTIMIZE_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_cvt                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the control value table into a face object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_cvt( TT_Face    face,
                    FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_ULong   table_len;


    FT_TRACE2(( "CVT " ));

    error = face->goto_table( face, TTAG_cvt, stream, &table_len );
    if ( error )
    {
      FT_TRACE2(( "is missing!\n" ));

      face->cvt_size = 0;
      face->cvt      = NULL;
      error          = TT_Err_Ok;

      goto Exit;
    }

    face->cvt_size = table_len / 2;

    if ( FT_NEW_ARRAY( face->cvt, face->cvt_size ) )
      goto Exit;

    if ( FT_FRAME_ENTER( face->cvt_size * 2L ) )
      goto Exit;

    {
      FT_Short*  cur   = face->cvt;
      FT_Short*  limit = cur + face->cvt_size;


      for ( ; cur <  limit; cur++ )
        *cur = FT_GET_SHORT();
    }

    FT_FRAME_EXIT();
    FT_TRACE2(( "loaded\n" ));

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    if ( face->doblend )
      error = tt_face_vary_cvt( face, stream );
#endif

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_fpgm                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the font program.                                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_fpgm( TT_Face    face,
                     FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error  error;
    FT_ULong  table_len;


    FT_TRACE2(( "Font program " ));

    /* The font program is optional */
    error = face->goto_table( face, TTAG_fpgm, stream, &table_len );
    if ( error )
    {
      face->font_program      = NULL;
      face->font_program_size = 0;
      error                   = TT_Err_Ok;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->font_program_size = table_len;
      if ( FT_FRAME_EXTRACT( table_len, face->font_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->font_program_size ));
    }

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_prep                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the cvt program.                                              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_prep( TT_Face    face,
                     FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error  error;
    FT_ULong  table_len;


    FT_TRACE2(( "Prep program " ));

    error = face->goto_table( face, TTAG_prep, stream, &table_len );
    if ( error )
    {
      face->cvt_program      = NULL;
      face->cvt_program_size = 0;
      error                  = TT_Err_Ok;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->cvt_program_size = table_len;
      if ( FT_FRAME_EXTRACT( table_len, face->cvt_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->cvt_program_size ));
    }

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hdmx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hdmx' table into the face object.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_UInt    version, nn, num_records;
    FT_ULong   table_size, record_size;
    FT_Byte*   p;
    FT_Byte*   limit;


    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, &table_size );
    if ( error || table_size < 8 )
      return TT_Err_Ok;

    if ( FT_FRAME_EXTRACT( table_size, face->hdmx_table ) )
      goto Exit;

    p     = face->hdmx_table;
    limit = p + table_size;

    version     = FT_NEXT_USHORT( p );
    num_records = FT_NEXT_USHORT( p );
    record_size = FT_NEXT_ULONG( p );

    if ( version != 0 || num_records > 255 || record_size > 0x40000 )
    {
      error = TT_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( FT_NEW_ARRAY( face->hdmx_record_sizes, num_records ) )
      goto Fail;

    for ( nn = 0; nn < num_records; nn++ )
    {
      if ( p + record_size > limit )
        break;

      face->hdmx_record_sizes[nn] = p[0];
      p                          += record_size;
    }

    face->hdmx_record_count = nn;
    face->hdmx_table_size   = table_size;
    face->hdmx_record_size  = record_size;

  Exit:
    return error;

  Fail:
    FT_FRAME_RELEASE( face->hdmx_table );
    face->hdmx_table_size = 0;
    goto Exit;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_hdmx( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;
    FT_Memory  memory = stream->memory;


    FT_FREE( face->hdmx_record_sizes );
    FT_FRAME_RELEASE( face->hdmx_table );
  }

#else /* !FT_OPTIMIZE_MEMORY */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    TT_Hdmx    hdmx = &face->hdmx;
    FT_Short   num_records;
    FT_Long    num_glyphs;
    FT_Long    record_size;


    hdmx->version     = 0;
    hdmx->num_records = 0;
    hdmx->records     = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, 0 );
    if ( error )
      return TT_Err_Ok;

    if ( FT_FRAME_ENTER( 8L ) )
      goto Exit;

    hdmx->version = FT_GET_USHORT();
    num_records   = FT_GET_SHORT();
    record_size   = FT_GET_LONG();

    FT_FRAME_EXIT();

    if ( record_size < 0 || num_records < 0 )
      return TT_Err_Invalid_File_Format;

    /* Only recognize format 0 */
    if ( hdmx->version != 0 )
      goto Exit;

    /* we can't use FT_QNEW_ARRAY here; otherwise tt_face_free_hdmx */
    /* could fail during deallocation                               */
    if ( FT_NEW_ARRAY( hdmx->records, num_records ) )
      goto Exit;

    hdmx->num_records = num_records;
    num_glyphs        = face->root.num_glyphs;
    record_size      -= num_glyphs + 2;

    {
      TT_HdmxEntry  cur   = hdmx->records;
      TT_HdmxEntry  limit = cur + hdmx->num_records;


      for ( ; cur < limit; cur++ )
      {
        /* read record */
        if ( FT_READ_BYTE( cur->ppem      ) ||
             FT_READ_BYTE( cur->max_width ) )
          goto Exit;

        if ( FT_QALLOC( cur->widths, num_glyphs )      ||
             FT_STREAM_READ( cur->widths, num_glyphs ) )
          goto Exit;

        /* skip padding bytes */
        if ( record_size > 0 && FT_STREAM_SKIP( record_size ) )
          goto Exit;
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_hdmx( TT_Face  face )
  {
    if ( face )
    {
      FT_Int     n;
      FT_Memory  memory = face->root.driver->root.memory;


      for ( n = 0; n < face->hdmx.num_records; n++ )
        FT_FREE( face->hdmx.records[n].widths );

      FT_FREE( face->hdmx.records );
      face->hdmx.num_records = 0;
    }
  }

#endif /* !OPTIMIZE_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* Return the advance width table for a given pixel size if it is found  */
  /* in the font's `hdmx' table (if any).                                  */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Byte* )
  tt_face_get_device_metrics( TT_Face  face,
                              FT_UInt  ppem,
                              FT_UInt  gindex )
  {
#ifdef FT_OPTIMIZE_MEMORY

    FT_UInt   nn;
    FT_Byte*  result      = NULL;
    FT_ULong  record_size = face->hdmx_record_size;
    FT_Byte*  record      = face->hdmx_table + 8;


    for ( nn = 0; nn < face->hdmx_record_count; nn++ )
      if ( face->hdmx_record_sizes[nn] == ppem )
      {
        gindex += 2;
        if ( gindex < record_size )
          result = record + nn * record_size + gindex;
        break;
      }

    return result;

#else

    FT_UShort  n;


    for ( n = 0; n < face->hdmx.num_records; n++ )
      if ( face->hdmx.records[n].ppem == ppem )
        return &face->hdmx.records[n].widths[gindex];

    return NULL;

#endif
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttgload.c                                                              */
/*                                                                         */
/*    TrueType Glyph Loader (body).                                        */
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
#include FT_TRUETYPE_TAGS_H
#include FT_OUTLINE_H

#include "ttgload.h"
#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttgload


  /*************************************************************************/
  /*                                                                       */
  /* Composite font flags.                                                 */
  /*                                                                       */
#define ARGS_ARE_WORDS             0x0001
#define ARGS_ARE_XY_VALUES         0x0002
#define ROUND_XY_TO_GRID           0x0004
#define WE_HAVE_A_SCALE            0x0008
/* reserved                        0x0010 */
#define MORE_COMPONENTS            0x0020
#define WE_HAVE_AN_XY_SCALE        0x0040
#define WE_HAVE_A_2X2              0x0080
#define WE_HAVE_INSTR              0x0100
#define USE_MY_METRICS             0x0200
#define OVERLAP_COMPOUND           0x0400
#define SCALED_COMPONENT_OFFSET    0x0800
#define UNSCALED_COMPONENT_OFFSET  0x1000


  /*************************************************************************/
  /*                                                                       */
  /* Returns the horizontal metrics in font units for a given glyph.  If   */
  /* `check' is true, take care of monospaced fonts by returning the       */
  /* advance width maximum.                                                */
  /*                                                                       */
  static void
  Get_HMetrics( TT_Face     face,
                FT_UInt     idx,
                FT_Bool     check,
                FT_Short*   lsb,
                FT_UShort*  aw )
  {
    ( (SFNT_Service)face->sfnt )->get_metrics( face, 0, idx, lsb, aw );

    if ( check && face->postscript.isFixedPitch )
      *aw = face->horizontal.advance_Width_Max;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Returns the vertical metrics in font units for a given glyph.         */
  /* Greg Hitchcock from Microsoft told us that if there were no `vmtx'    */
  /* table, typoAscender/Descender from the `OS/2' table would be used     */
  /* instead, and if there were no `OS/2' table, use ascender/descender    */
  /* from the `hhea' table.  But that is not what Microsoft's rasterizer   */
  /* apparently does: It uses the ppem value as the advance height, and    */
  /* sets the top side bearing to be zero.                                 */
  /*                                                                       */
  /* The monospace `check' is probably not meaningful here, but we leave   */
  /* it in for a consistent interface.                                     */
  /*                                                                       */
  static void
  Get_VMetrics( TT_Face     face,
                FT_UInt     idx,
                FT_Bool     check,
                FT_Short*   tsb,
                FT_UShort*  ah )
  {
    FT_UNUSED( check );

    if ( face->vertical_info )
      ( (SFNT_Service)face->sfnt )->get_metrics( face, 1, idx, tsb, ah );

#if 1             /* Emperically determined, at variance with what MS said */

    else
    {
      *tsb = 0;
      *ah  = face->root.units_per_EM;
    }

#else      /* This is what MS said to do.  It isn't what they do, however. */

    else if ( face->os2.version != 0xFFFFU )
    {
      *tsb = face->os2.sTypoAscender;
      *ah  = face->os2.sTypoAscender - face->os2.sTypoDescender;
    }
    else
    {
      *tsb = face->horizontal.Ascender;
      *ah  = face->horizontal.Ascender - face->horizontal.Descender;
    }

#endif

  }


  /*************************************************************************/
  /*                                                                       */
  /* Translates an array of coordinates.                                   */
  /*                                                                       */
  static void
  translate_array( FT_UInt     n,
                   FT_Vector*  coords,
                   FT_Pos      delta_x,
                   FT_Pos      delta_y )
  {
    FT_UInt  k;


    if ( delta_x )
      for ( k = 0; k < n; k++ )
        coords[k].x += delta_x;

    if ( delta_y )
      for ( k = 0; k < n; k++ )
        coords[k].y += delta_y;
  }


#undef  IS_HINTED
#define IS_HINTED( flags )  ( ( flags & FT_LOAD_NO_HINTING ) == 0 )


  /*************************************************************************/
  /*                                                                       */
  /* The following functions are used by default with TrueType fonts.      */
  /* However, they can be replaced by alternatives if we need to support   */
  /* TrueType-compressed formats (like MicroType) in the future.           */
  /*                                                                       */
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  TT_Access_Glyph_Frame( TT_Loader  loader,
                         FT_UInt    glyph_index,
                         FT_ULong   offset,
                         FT_UInt    byte_count )
  {
    FT_Error   error;
    FT_Stream  stream = loader->stream;

    /* for non-debug mode */
    FT_UNUSED( glyph_index );


    FT_TRACE5(( "Glyph %ld\n", glyph_index ));

    /* the following line sets the `error' variable through macros! */
    if ( FT_STREAM_SEEK( offset ) || FT_FRAME_ENTER( byte_count ) )
      return error;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  TT_Forget_Glyph_Frame( TT_Loader  loader )
  {
    FT_Stream  stream = loader->stream;


    FT_FRAME_EXIT();
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Glyph_Header( TT_Loader  loader )
  {
    FT_Stream  stream   = loader->stream;
    FT_Int     byte_len = loader->byte_len - 10;


    if ( byte_len < 0 )
      return TT_Err_Invalid_Outline;

    loader->n_contours = FT_GET_SHORT();

    loader->bbox.xMin = FT_GET_SHORT();
    loader->bbox.yMin = FT_GET_SHORT();
    loader->bbox.xMax = FT_GET_SHORT();
    loader->bbox.yMax = FT_GET_SHORT();

    FT_TRACE5(( "  # of contours: %d\n", loader->n_contours ));
    FT_TRACE5(( "  xMin: %4d  xMax: %4d\n", loader->bbox.xMin,
                                            loader->bbox.xMax ));
    FT_TRACE5(( "  yMin: %4d  yMax: %4d\n", loader->bbox.yMin,
                                            loader->bbox.yMax ));
    loader->byte_len = byte_len;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Simple_Glyph( TT_Loader  load )
  {
    FT_Error        error;
    FT_Stream       stream     = load->stream;
    FT_GlyphLoader  gloader    = load->gloader;
    FT_Int          n_contours = load->n_contours;
    FT_Outline*     outline;
    TT_Face         face       = (TT_Face)load->face;
    FT_UShort       n_ins;
    FT_Int          n, n_points;
    FT_Int          byte_len   = load->byte_len;

    FT_Byte         *flag, *flag_limit;
    FT_Byte         c, count;
    FT_Vector       *vec, *vec_limit;
    FT_Pos          x;
    FT_Short        *cont, *cont_limit;


    /* check that we can add the contours to the glyph */
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, 0, n_contours );
    if ( error )
      goto Fail;

    /* reading the contours' endpoints & number of points */
    cont       = gloader->current.outline.contours;
    cont_limit = cont + n_contours;

    /* check space for contours array + instructions count */
    byte_len -= 2 * ( n_contours + 1 );
    if ( byte_len < 0 )
      goto Invalid_Outline;

    for ( ; cont < cont_limit; cont++ )
      cont[0] = FT_GET_USHORT();

    n_points = 0;
    if ( n_contours > 0 )
      n_points = cont[-1] + 1;

    /* note that we will add four phantom points later */
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, n_points + 4, 0 );
    if ( error )
      goto Fail;

    /* we'd better check the contours table right now */
    outline = &gloader->current.outline;

    for ( cont = outline->contours + 1; cont < cont_limit; cont++ )
      if ( cont[-1] >= cont[0] )
        goto Invalid_Outline;

    /* reading the bytecode instructions */
    load->glyph->control_len  = 0;
    load->glyph->control_data = 0;

    n_ins = FT_GET_USHORT();

    FT_TRACE5(( "  Instructions size: %u\n", n_ins ));

    if ( n_ins > face->max_profile.maxSizeOfInstructions )
    {
      FT_TRACE0(( "TT_Load_Simple_Glyph: Too many instructions!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

    byte_len -= (FT_Int)n_ins;
    if ( byte_len < 0 )
    {
      FT_TRACE0(( "TT_Load_Simple_Glyph: Instruction count mismatch!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( IS_HINTED( load->load_flags ) )
    {
      load->glyph->control_len  = n_ins;
      load->glyph->control_data = load->exec->glyphIns;

      FT_MEM_COPY( load->exec->glyphIns, stream->cursor, (FT_Long)n_ins );
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    stream->cursor += (FT_Int)n_ins;

    /* reading the point tags */
    flag       = (FT_Byte*)outline->tags;
    flag_limit = flag + n_points;

    FT_ASSERT( flag != NULL );

    while ( flag < flag_limit )
    {
      if ( --byte_len < 0 )
        goto Invalid_Outline;

      *flag++ = c = FT_GET_BYTE();
      if ( c & 8 )
      {
        if ( --byte_len < 0 )
          goto Invalid_Outline;

        count = FT_GET_BYTE();
        if ( flag + (FT_Int)count > flag_limit )
          goto Invalid_Outline;

        for ( ; count > 0; count-- )
          *flag++ = c;
      }
    }

    /* check that there is enough room to load the coordinates */
    for ( flag = (FT_Byte*)outline->tags; flag < flag_limit; flag++ )
    {
      if ( *flag & 2 )
        byte_len -= 1;
      else if ( ( *flag & 16 ) == 0 )
        byte_len -= 2;

      if ( *flag & 4 )
        byte_len -= 1;
      else if ( ( *flag & 32 ) == 0 )
        byte_len -= 2;
    }

    if ( byte_len < 0 )
      goto Invalid_Outline;

    /* reading the X coordinates */

    vec       = outline->points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos  y = 0;


      if ( *flag & 2 )
      {
        y = (FT_Pos)FT_GET_BYTE();
        if ( ( *flag & 16 ) == 0 )
          y = -y;
      }
      else if ( ( *flag & 16 ) == 0 )
        y = (FT_Pos)FT_GET_SHORT();

      x     += y;
      vec->x = x;
    }

    /* reading the Y coordinates */

    vec       = gloader->current.outline.points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos  y = 0;


      if ( *flag & 4 )
      {
        y = (FT_Pos)FT_GET_BYTE();
        if ( ( *flag & 32 ) == 0 )
          y = -y;
      }
      else if ( ( *flag & 32 ) == 0 )
        y = (FT_Pos)FT_GET_SHORT();

      x     += y;
      vec->y = x;
    }

    /* clear the touch tags */
    for ( n = 0; n < n_points; n++ )
      outline->tags[n] &= FT_CURVE_TAG_ON;

    outline->n_points   = (FT_UShort)n_points;
    outline->n_contours = (FT_Short) n_contours;

    load->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Outline:
    error = TT_Err_Invalid_Outline;
    goto Fail;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Composite_Glyph( TT_Loader  loader )
  {
    FT_Error        error;
    FT_Stream       stream  = loader->stream;
    FT_GlyphLoader  gloader = loader->gloader;
    FT_SubGlyph     subglyph;
    FT_UInt         num_subglyphs;
    FT_Int          byte_len = loader->byte_len;


    num_subglyphs = 0;

    do
    {
      FT_Fixed  xx, xy, yy, yx;


      /* check that we can load a new subglyph */
      error = FT_GlyphLoader_CheckSubGlyphs( gloader, num_subglyphs + 1 );
      if ( error )
        goto Fail;

      /* check space */
      byte_len -= 4;
      if ( byte_len < 0 )
        goto Invalid_Composite;

      subglyph = gloader->current.subglyphs + num_subglyphs;

      subglyph->arg1 = subglyph->arg2 = 0;

      subglyph->flags = FT_GET_USHORT();
      subglyph->index = FT_GET_USHORT();

      /* check space */
      byte_len -= 2;
      if ( subglyph->flags & ARGS_ARE_WORDS )
        byte_len -= 2;
      if ( subglyph->flags & WE_HAVE_A_SCALE )
        byte_len -= 2;
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
        byte_len -= 4;
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
        byte_len -= 8;

      if ( byte_len < 0 )
        goto Invalid_Composite;

      /* read arguments */
      if ( subglyph->flags & ARGS_ARE_WORDS )
      {
        subglyph->arg1 = FT_GET_SHORT();
        subglyph->arg2 = FT_GET_SHORT();
      }
      else
      {
        subglyph->arg1 = FT_GET_CHAR();
        subglyph->arg2 = FT_GET_CHAR();
      }

      /* read transform */
      xx = yy = 0x10000L;
      xy = yx = 0;

      if ( subglyph->flags & WE_HAVE_A_SCALE )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = xx;
      }
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = (FT_Fixed)FT_GET_SHORT() << 2;
      }
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yx = (FT_Fixed)FT_GET_SHORT() << 2;
        xy = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = (FT_Fixed)FT_GET_SHORT() << 2;
      }

      subglyph->transform.xx = xx;
      subglyph->transform.xy = xy;
      subglyph->transform.yx = yx;
      subglyph->transform.yy = yy;

      num_subglyphs++;

    } while ( subglyph->flags & MORE_COMPONENTS );

    gloader->current.num_subglyphs = num_subglyphs;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    {
      /* we must undo the FT_FRAME_ENTER in order to point to the */
      /* composite instructions, if we find some.               */
      /* we will process them later...                          */
      /*                                                        */
      loader->ins_pos = (FT_ULong)( FT_STREAM_POS() +
                                    stream->cursor - stream->limit );
    }

#endif

    loader->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Composite:
    error = TT_Err_Invalid_Composite;
    goto Fail;
  }


  FT_LOCAL_DEF( void )
  TT_Init_Glyph_Loading( TT_Face  face )
  {
    face->access_glyph_frame   = TT_Access_Glyph_Frame;
    face->read_glyph_header    = TT_Load_Glyph_Header;
    face->read_simple_glyph    = TT_Load_Simple_Glyph;
    face->read_composite_glyph = TT_Load_Composite_Glyph;
    face->forget_glyph_frame   = TT_Forget_Glyph_Frame;
  }


  static void
  tt_prepare_zone( TT_GlyphZone  zone,
                   FT_GlyphLoad  load,
                   FT_UInt       start_point,
                   FT_UInt       start_contour )
  {
    zone->n_points   = (FT_UShort)( load->outline.n_points - start_point );
    zone->n_contours = (FT_Short) ( load->outline.n_contours - start_contour );
    zone->org        = load->extra_points + start_point;
    zone->cur        = load->outline.points + start_point;
    zone->tags       = (FT_Byte*)load->outline.tags + start_point;
    zone->contours   = (FT_UShort*)load->outline.contours + start_contour;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Hint_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Hint the glyph using the zone prepared by the caller.  Note that   */
  /*    the zone is supposed to include four phantom points.               */
  /*                                                                       */
#define cur_to_org( n, zone ) \
          FT_ARRAY_COPY( (zone)->org, (zone)->cur, (n) )

  static FT_Error
  TT_Hint_Glyph( TT_Loader  loader,
                 FT_Bool    is_composite )
  {
    TT_GlyphZone  zone = &loader->zone;
    FT_Pos        origin;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FT_UInt       n_ins;
#else
    FT_UNUSED( is_composite );
#endif


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    n_ins = loader->glyph->control_len;
#endif

    origin = zone->cur[zone->n_points - 4].x;
    origin = FT_PIX_ROUND( origin ) - origin;
    if ( origin )
      translate_array( zone->n_points, zone->cur, origin, 0 );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    /* save original point positioin in org */
    if ( n_ins > 0 )
      cur_to_org( zone->n_points, zone );
#endif

    /* round pp2 and pp4 */
    zone->cur[zone->n_points - 3].x =
      FT_PIX_ROUND( zone->cur[zone->n_points - 3].x );
    zone->cur[zone->n_points - 1].y =
      FT_PIX_ROUND( zone->cur[zone->n_points - 1].y );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( n_ins > 0 )
    {
      FT_Bool   debug;
      FT_Error  error;


      error = TT_Set_CodeRange( loader->exec, tt_coderange_glyph,
                                loader->exec->glyphIns, n_ins );
      if ( error )
        return error;

      loader->exec->is_composite = is_composite;
      loader->exec->pts          = *zone;

      debug = !( loader->load_flags & FT_LOAD_NO_SCALE ) &&
              ( (TT_Size)loader->size )->debug;

      error = TT_Run_Context( loader->exec, debug );
      if ( error && loader->exec->pedantic_hinting )
        return error;
    }

#endif

    /* save glyph phantom points */
    if ( !loader->preserve_pps )
    {
      loader->pp1 = zone->cur[zone->n_points - 4];
      loader->pp2 = zone->cur[zone->n_points - 3];
      loader->pp3 = zone->cur[zone->n_points - 2];
      loader->pp4 = zone->cur[zone->n_points - 1];
    }

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Process_Simple_Glyph                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Once a simple glyph has been loaded, it needs to be processed.     */
  /*    Usually, this means scaling and hinting through bytecode           */
  /*    interpretation.                                                    */
  /*                                                                       */
  static FT_Error
  TT_Process_Simple_Glyph( TT_Loader  loader )
  {
    FT_GlyphLoader  gloader = loader->gloader;
    FT_Error        error   = TT_Err_Ok;
    FT_Outline*     outline;
    FT_UInt         n_points;


    outline  = &gloader->current.outline;
    n_points = outline->n_points;

    /* set phantom points */

    outline->points[n_points    ] = loader->pp1;
    outline->points[n_points + 1] = loader->pp2;
    outline->points[n_points + 2] = loader->pp3;
    outline->points[n_points + 3] = loader->pp4;

    outline->tags[n_points    ] = 0;
    outline->tags[n_points + 1] = 0;
    outline->tags[n_points + 2] = 0;
    outline->tags[n_points + 3] = 0;

    n_points += 4;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

    if ( ((TT_Face)loader->face)->doblend )
    {
      /* Deltas apply to the unscaled data. */
      FT_Vector*  deltas;
      FT_Memory   memory = loader->face->memory;
      FT_UInt     i;


      error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(loader->face),
                                        loader->glyph_index,
                                        &deltas,
                                        n_points );
      if ( error )
        return error;

      for ( i = 0; i < n_points; ++i )
      {
        outline->points[i].x += deltas[i].x;
        outline->points[i].y += deltas[i].y;
      }

      FT_FREE( deltas );
    }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */

    /* scale the glyph */
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      FT_Vector*  vec     = outline->points;
      FT_Vector*  limit   = outline->points + n_points;
      FT_Fixed    x_scale = ((TT_Size)loader->size)->metrics.x_scale;
      FT_Fixed    y_scale = ((TT_Size)loader->size)->metrics.y_scale;


      for ( ; vec < limit; vec++ )
      {
        vec->x = FT_MulFix( vec->x, x_scale );
        vec->y = FT_MulFix( vec->y, y_scale );
      }

      loader->pp1 = outline->points[n_points - 4];
      loader->pp2 = outline->points[n_points - 3];
      loader->pp3 = outline->points[n_points - 2];
      loader->pp4 = outline->points[n_points - 1];
    }

    if ( IS_HINTED( loader->load_flags ) )
    {
      tt_prepare_zone( &loader->zone, &gloader->current, 0, 0 );
      loader->zone.n_points += 4;

      error = TT_Hint_Glyph( loader, 0 );
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Process_Composite_Component                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Once a composite component has been loaded, it needs to be         */
  /*    processed.  Usually, this means transforming and translating.      */
  /*                                                                       */
  static FT_Error
  TT_Process_Composite_Component( TT_Loader    loader,
                                  FT_SubGlyph  subglyph,
                                  FT_UInt      start_point,
                                  FT_UInt      num_base_points )
  {
    FT_GlyphLoader  gloader    = loader->gloader;
    FT_Vector*      base_vec   = gloader->base.outline.points;
    FT_UInt         num_points = gloader->base.outline.n_points;
    FT_Bool         have_scale;
    FT_Pos          x, y;


    have_scale = FT_BOOL( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                              WE_HAVE_AN_XY_SCALE |
                                              WE_HAVE_A_2X2       ) );

    /* perform the transform required for this subglyph */
    if ( have_scale )
    {
      FT_UInt  i;


      for ( i = num_base_points; i < num_points; i++ )
        FT_Vector_Transform( base_vec + i, &subglyph->transform );
    }

    /* get offset */
    if ( !( subglyph->flags & ARGS_ARE_XY_VALUES ) )
    {
      FT_UInt     k = subglyph->arg1;
      FT_UInt     l = subglyph->arg2;
      FT_Vector*  p1;
      FT_Vector*  p2;


      /* match l-th point of the newly loaded component to the k-th point */
      /* of the previously loaded components.                             */

      /* change to the point numbers used by our outline */
      k += start_point;
      l += num_base_points;
      if ( k >= num_base_points ||
           l >= num_points      )
        return TT_Err_Invalid_Composite;

      p1 = gloader->base.outline.points + k;
      p2 = gloader->base.outline.points + l;

      x = p1->x - p2->x;
      y = p1->y - p2->y;
    }
    else
    {
      x = subglyph->arg1;
      y = subglyph->arg2;

      if ( !x && !y )
        return TT_Err_Ok;

  /* Use a default value dependent on                                     */
  /* TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED.  This is useful for old TT */
  /* fonts which don't set the xxx_COMPONENT_OFFSET bit.                  */

      if ( have_scale &&
#ifdef TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED
           !( subglyph->flags & UNSCALED_COMPONENT_OFFSET ) )
#else
            ( subglyph->flags & SCALED_COMPONENT_OFFSET ) )
#endif
      {

#if 0

  /*************************************************************************/
  /*                                                                       */
  /* This algorithm is what Apple documents.  But it doesn't work.         */
  /*                                                                       */
        int  a = subglyph->transform.xx > 0 ?  subglyph->transform.xx
                                            : -subglyph->transform.xx;
        int  b = subglyph->transform.yx > 0 ?  subglyph->transform.yx
                                            : -subglyph->transform.yx;
        int  c = subglyph->transform.xy > 0 ?  subglyph->transform.xy
                                            : -subglyph->transform.xy;
        int  d = subglyph->transform.yy > 0 ? subglyph->transform.yy
                                            : -subglyph->transform.yy;
        int  m = a > b ? a : b;
        int  n = c > d ? c : d;


        if ( a - b <= 33 && a - b >= -33 )
          m *= 2;
        if ( c - d <= 33 && c - d >= -33 )
          n *= 2;
        x = FT_MulFix( x, m );
        y = FT_MulFix( y, n );

#else /* 0 */

  /*************************************************************************/
  /*                                                                       */
  /* This algorithm is a guess and works much better than the above.       */
  /*                                                                       */
        FT_Fixed  mac_xscale = FT_SqrtFixed(
                                 FT_MulFix( subglyph->transform.xx,
                                            subglyph->transform.xx ) +
                                 FT_MulFix( subglyph->transform.xy,
                                            subglyph->transform.xy ) );
        FT_Fixed  mac_yscale = FT_SqrtFixed(
                                 FT_MulFix( subglyph->transform.yy,
                                            subglyph->transform.yy ) +
                                 FT_MulFix( subglyph->transform.yx,
                                            subglyph->transform.yx ) );


        x = FT_MulFix( x, mac_xscale );
        y = FT_MulFix( y, mac_yscale );

#endif /* 0 */

      }

      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        FT_Fixed  x_scale = ((TT_Size)loader->size)->metrics.x_scale;
        FT_Fixed  y_scale = ((TT_Size)loader->size)->metrics.y_scale;


        x = FT_MulFix( x, x_scale );
        y = FT_MulFix( y, y_scale );

        if ( subglyph->flags & ROUND_XY_TO_GRID )
        {
          x = FT_PIX_ROUND( x );
          y = FT_PIX_ROUND( y );
        }
      }
    }

    if ( x || y )
      translate_array( num_points - num_base_points,
                       base_vec + num_base_points,
                       x, y );

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Process_Composite_Glyph                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This is slightly different from TT_Process_Simple_Glyph, in that   */
  /*    it's sole purpose is to hint the glyph.  Thus this function is     */
  /*    only available when bytecode interpreter is enabled.               */
  /*                                                                       */
  static FT_Error
  TT_Process_Composite_Glyph( TT_Loader  loader,
                              FT_UInt    start_point,
                              FT_UInt    start_contour )
  {
    FT_Error     error;
    FT_Outline*  outline;


    outline = &loader->gloader->base.outline;

    /* make room for phantom points */
    error = FT_GLYPHLOADER_CHECK_POINTS( loader->gloader,
                                         outline->n_points + 4,
                                         0 );
    if ( error )
      return error;

    outline->points[outline->n_points    ] = loader->pp1;
    outline->points[outline->n_points + 1] = loader->pp2;
    outline->points[outline->n_points + 2] = loader->pp3;
    outline->points[outline->n_points + 3] = loader->pp4;

    outline->tags[outline->n_points    ] = 0;
    outline->tags[outline->n_points + 1] = 0;
    outline->tags[outline->n_points + 2] = 0;
    outline->tags[outline->n_points + 3] = 0;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    {
      FT_Stream  stream = loader->stream;
      FT_UShort  n_ins;


      /* TT_Load_Composite_Glyph only gives us the offset of instructions */
      /* so we read them here                                             */
      if ( FT_STREAM_SEEK( loader->ins_pos ) ||
           FT_READ_USHORT( n_ins )           )
        return error;

      FT_TRACE5(( "  Instructions size = %d\n", n_ins ));

      /* check it */
      if ( n_ins > ((TT_Face)loader->face)->max_profile.maxSizeOfInstructions )
      {
        FT_TRACE0(( "Too many instructions (%d)\n", n_ins ));

        return TT_Err_Too_Many_Hints;
      }
      else if ( n_ins == 0 )
        return TT_Err_Ok;

      if ( FT_STREAM_READ( loader->exec->glyphIns, n_ins ) )
        return error;

      loader->glyph->control_data = loader->exec->glyphIns;
      loader->glyph->control_len  = n_ins;
    }

#endif

    tt_prepare_zone( &loader->zone, &loader->gloader->base,
                     start_point, start_contour );
    loader->zone.n_points += 4;

    return TT_Hint_Glyph( loader, 1 );
  }


  /* Calculate the four phantom points.                     */
  /* The first two stand for horizontal origin and advance. */
  /* The last two stand for vertical origin and advance.    */
#define TT_LOADER_SET_PP( loader )                                          \
          do {                                                              \
            (loader)->pp1.x = (loader)->bbox.xMin - (loader)->left_bearing; \
            (loader)->pp1.y = 0;                                            \
            (loader)->pp2.x = (loader)->pp1.x + (loader)->advance;          \
            (loader)->pp2.y = 0;                                            \
            (loader)->pp3.x = 0;                                            \
            (loader)->pp3.y = (loader)->top_bearing + (loader)->bbox.yMax;  \
            (loader)->pp4.x = 0;                                            \
            (loader)->pp4.y = (loader)->pp3.y - (loader)->vadvance;         \
          } while ( 0 )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    load_truetype_glyph                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given truetype glyph.  Handles composites and uses a       */
  /*    TT_Loader object.                                                  */
  /*                                                                       */
  static FT_Error
  load_truetype_glyph( TT_Loader  loader,
                       FT_UInt    glyph_index,
                       FT_UInt    recurse_count )
  {
    FT_Error        error;
    FT_Fixed        x_scale, y_scale;
    FT_ULong        offset;
    TT_Face         face         = (TT_Face)loader->face;
    FT_GlyphLoader  gloader      = loader->gloader;
    FT_Bool         opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Vector*      deltas       = NULL;
#endif

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_StreamRec    inc_stream;
    FT_Data         glyph_data;
    FT_Bool         glyph_data_loaded = 0;
#endif


    if ( recurse_count > face->max_profile.maxComponentDepth )
    {
      error = TT_Err_Invalid_Composite;
      goto Exit;
    }

    /* check glyph index */
    if ( glyph_index >= (FT_UInt)face->root.num_glyphs )
    {
      error = TT_Err_Invalid_Glyph_Index;
      goto Exit;
    }

    loader->glyph_index = glyph_index;

    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      x_scale = ((TT_Size)loader->size)->metrics.x_scale;
      y_scale = ((TT_Size)loader->size)->metrics.y_scale;
    }
    else
    {
      x_scale = 0x10000L;
      y_scale = 0x10000L;
    }

    /* get metrics, horizontal and vertical */
    {
      FT_Short   left_bearing = 0, top_bearing = 0;
      FT_UShort  advance_width = 0, advance_height = 0;


      Get_HMetrics( face, glyph_index,
                    (FT_Bool)!( loader->load_flags &
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ),
                    &left_bearing,
                    &advance_width );
      Get_VMetrics( face, glyph_index,
                    (FT_Bool)!( loader->load_flags &
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ),
                    &top_bearing,
                    &advance_height );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

      /* If this is an incrementally loaded font see if there are */
      /* overriding metrics for this glyph.                       */
      if ( face->root.internal->incremental_interface &&
           face->root.internal->incremental_interface->funcs->get_glyph_metrics )
      {
        FT_Incremental_MetricsRec  metrics;


        metrics.bearing_x = left_bearing;
        metrics.bearing_y = 0;
        metrics.advance = advance_width;
        error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                  face->root.internal->incremental_interface->object,
                  glyph_index, FALSE, &metrics );
        if ( error )
          goto Exit;
        left_bearing  = (FT_Short)metrics.bearing_x;
        advance_width = (FT_UShort)metrics.advance;

#if 0

        /* GWW: Do I do the same for vertical metrics? */
        metrics.bearing_x = 0;
        metrics.bearing_y = top_bearing;
        metrics.advance = advance_height;
        error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                  face->root.internal->incremental_interface->object,
                  glyph_index, TRUE, &metrics );
        if ( error )
          goto Exit;
        top_bearing  = (FT_Short)metrics.bearing_y;
        advance_height = (FT_UShort)metrics.advance;

#endif /* 0 */

      }

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

      loader->left_bearing = left_bearing;
      loader->advance      = advance_width;
      loader->top_bearing  = top_bearing;
      loader->vadvance     = advance_height;

      if ( !loader->linear_def )
      {
        loader->linear_def = 1;
        loader->linear     = advance_width;
      }
    }

    /* Set `offset' to the start of the glyph relative to the start of */
    /* the `glyf' table, and `byte_len' to the length of the glyph in  */
    /* bytes.                                                          */

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* If we are loading glyph data via the incremental interface, set */
    /* the loader stream to a memory stream reading the data returned  */
    /* by the interface.                                               */
    if ( face->root.internal->incremental_interface )
    {
      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index, &glyph_data );
      if ( error )
        goto Exit;

      glyph_data_loaded = 1;
      offset            = 0;
      loader->byte_len  = glyph_data.length;

      FT_MEM_ZERO( &inc_stream, sizeof ( inc_stream ) );
      FT_Stream_OpenMemory( &inc_stream,
                            glyph_data.pointer, glyph_data.length );

      loader->stream = &inc_stream;
    }
    else

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

      offset = tt_face_get_location( face, glyph_index,
                                     (FT_UInt*)&loader->byte_len );

    if ( loader->byte_len == 0 )
    {
      /* as described by Frederic Loyer, these are spaces or */
      /* the unknown glyph.                                  */
      loader->bbox.xMin = 0;
      loader->bbox.xMax = 0;
      loader->bbox.yMin = 0;
      loader->bbox.yMax = 0;

      TT_LOADER_SET_PP( loader );

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( ((TT_Face)(loader->face))->doblend )
      {
        /* this must be done before scaling */
        FT_Memory  memory = loader->face->memory;


        error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(loader->face),
                                          glyph_index, &deltas, 4 );
        if ( error )
          goto Exit;

        loader->pp1.x += deltas[0].x; loader->pp1.y += deltas[0].y;
        loader->pp2.x += deltas[1].x; loader->pp2.y += deltas[1].y;
        loader->pp3.x += deltas[2].x; loader->pp3.y += deltas[2].y;
        loader->pp4.x += deltas[3].x; loader->pp4.y += deltas[3].y;

        FT_FREE( deltas );
      }

#endif

      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      error = TT_Err_Ok;
      goto Exit;
    }

    error = face->access_glyph_frame( loader, glyph_index,
                                      loader->glyf_offset + offset,
                                      loader->byte_len );
    if ( error )
      goto Exit;

    opened_frame = 1;

    /* read first glyph header */
    error = face->read_glyph_header( loader );
    if ( error )
      goto Exit;

    TT_LOADER_SET_PP( loader );

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* if it is a simple glyph, load it */

    if ( loader->n_contours >= 0 )
    {
      error = face->read_simple_glyph( loader );
      if ( error )
        goto Exit;

      /* all data have been read */
      face->forget_glyph_frame( loader );
      opened_frame = 0;

      error = TT_Process_Simple_Glyph( loader );
      if ( error )
        goto Exit;

      FT_GlyphLoader_Add( gloader );
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* otherwise, load a composite! */
    else if ( loader->n_contours == -1 )
    {
      FT_UInt       start_point;
      FT_UInt       start_contour;
      FT_ULong      ins_pos;  /* position of composite instructions, if any */


      start_point   = gloader->base.outline.n_points;
      start_contour = gloader->base.outline.n_contours;

      /* for each subglyph, read composite header */
      error = face->read_composite_glyph( loader );
      if ( error )
        goto Exit;

      /* store the offset of instructions */
      ins_pos = loader->ins_pos;

      /* all data we need are read */
      face->forget_glyph_frame( loader );
      opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( face->doblend )
      {
        FT_Int       i, limit;
        FT_SubGlyph  subglyph;
        FT_Memory    memory = face->root.memory;


        /* this provides additional offsets */
        /* for each component's translation */

        if ( (error = TT_Vary_Get_Glyph_Deltas(
                        face,
                        glyph_index,
                        &deltas,
                        gloader->current.num_subglyphs + 4 )) != 0 )
          goto Exit;

        subglyph = gloader->current.subglyphs + gloader->base.num_subglyphs;
        limit    = gloader->current.num_subglyphs;

        for ( i = 0; i < limit; ++i, ++subglyph )
        {
          if ( subglyph->flags & ARGS_ARE_XY_VALUES )
          {
            subglyph->arg1 += deltas[i].x;
            subglyph->arg2 += deltas[i].y;
          }
        }

        loader->pp1.x += deltas[i + 0].x; loader->pp1.y += deltas[i + 0].y;
        loader->pp2.x += deltas[i + 1].x; loader->pp2.y += deltas[i + 1].y;
        loader->pp3.x += deltas[i + 2].x; loader->pp3.y += deltas[i + 2].y;
        loader->pp4.x += deltas[i + 3].x; loader->pp4.y += deltas[i + 3].y;

        FT_FREE( deltas );
      }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */

      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      /* if the flag FT_LOAD_NO_RECURSE is set, we return the subglyph */
      /* `as is' in the glyph slot (the client application will be     */
      /* responsible for interpreting these data)...                   */
      /*                                                               */
      if ( loader->load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_GlyphLoader_Add( gloader );
        loader->glyph->format = FT_GLYPH_FORMAT_COMPOSITE;

        goto Exit;
      }

      /*********************************************************************/
      /*********************************************************************/
      /*********************************************************************/

      {
        FT_UInt      n, num_base_points;
        FT_SubGlyph  subglyph       = 0;

        FT_UInt      num_points     = start_point;
        FT_UInt      num_subglyphs  = gloader->current.num_subglyphs;
        FT_UInt      num_base_subgs = gloader->base.num_subglyphs;


        FT_GlyphLoader_Add( gloader );

        /* read each subglyph independently */
        for ( n = 0; n < num_subglyphs; n++ )
        {
          FT_Vector  pp[4];


          /* Each time we call load_truetype_glyph in this loop, the   */
          /* value of `gloader.base.subglyphs' can change due to table */
          /* reallocations.  We thus need to recompute the subglyph    */
          /* pointer on each iteration.                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          pp[0] = loader->pp1;
          pp[1] = loader->pp2;
          pp[2] = loader->pp3;
          pp[3] = loader->pp4;

          num_base_points = gloader->base.outline.n_points;

          error = load_truetype_glyph( loader, subglyph->index,
                                       recurse_count + 1 );
          if ( error )
            goto Exit;

          /* restore subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          if ( !( subglyph->flags & USE_MY_METRICS ) )
          {
            loader->pp1 = pp[0];
            loader->pp2 = pp[1];
            loader->pp3 = pp[2];
            loader->pp4 = pp[3];
          }

          num_points = gloader->base.outline.n_points;

          if ( num_points == num_base_points )
            continue;

          /* gloader->base.outline consists of three part:                  */
          /* 0 -(1)-> start_point -(2)-> num_base_points -(3)-> n_points.   */
          /*                                                                */
          /* (1): exist from the beginning                                  */
          /* (2): components that have been loaded so far                   */
          /* (3): the newly loaded component                                */
          TT_Process_Composite_Component( loader, subglyph, start_point,
                                          num_base_points );
        }


        /* process the glyph */
        loader->ins_pos = ins_pos;
        if ( IS_HINTED( loader->load_flags ) &&

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

             subglyph->flags & WE_HAVE_INSTR &&

#endif

             num_points > start_point )
          TT_Process_Composite_Glyph( loader, start_point, start_contour );

      }
    }
    else
    {
      /* invalid composite count ( negative but not -1 ) */
      error = TT_Err_Invalid_Outline;
      goto Exit;
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

  Exit:

    if ( opened_frame )
      face->forget_glyph_frame( loader );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( glyph_data_loaded )
      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,
        &glyph_data );

#endif

    return error;
  }


  static FT_Error
  compute_glyph_metrics( TT_Loader   loader,
                         FT_UInt     glyph_index )
  {
    FT_BBox       bbox;
    TT_Face       face = (TT_Face)loader->face;
    FT_Fixed      y_scale;
    TT_GlyphSlot  glyph = loader->glyph;
    TT_Size       size = (TT_Size)loader->size;


    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      y_scale = size->root.metrics.y_scale;

    if ( glyph->format != FT_GLYPH_FORMAT_COMPOSITE )
      FT_Outline_Get_CBox( &glyph->outline, &bbox );
    else
      bbox = loader->bbox;

    /* get the device-independent horizontal advance.  It is scaled later */
    /* by the base layer.                                                 */
    {
      FT_Pos  advance = loader->linear;


      /* the flag FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH was introduced to */
      /* correctly support DynaLab fonts, which have an incorrect       */
      /* `advance_Width_Max' field!  It is used, to my knowledge,       */
      /* exclusively in the X-TrueType font server.                     */
      /*                                                                */
      if ( face->postscript.isFixedPitch                                     &&
           ( loader->load_flags & FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ) == 0 )
        advance = face->horizontal.advance_Width_Max;

      /* we need to return the advance in font units in linearHoriAdvance, */
      /* it will be scaled later by the base layer.                        */
      glyph->linearHoriAdvance = advance;
    }

    glyph->metrics.horiBearingX = bbox.xMin;
    glyph->metrics.horiBearingY = bbox.yMax;
    glyph->metrics.horiAdvance  = loader->pp2.x - loader->pp1.x;

    /* Now take care of vertical metrics.  In the case where there is    */
    /* no vertical information within the font (relatively common), make */
    /* up some metrics by `hand'...                                      */

    {
      FT_Pos  top;      /* scaled vertical top side bearing  */
      FT_Pos  advance;  /* scaled vertical advance height    */


      /* Get the unscaled top bearing and advance height. */
      if ( face->vertical_info &&
           face->vertical.number_Of_VMetrics > 0 )
      {
        top = (FT_Short)FT_DivFix( loader->pp3.y - bbox.yMax,
                                   y_scale );

        if ( loader->pp3.y <= loader->pp4.y )
          advance = 0;
        else
          advance = (FT_UShort)FT_DivFix( loader->pp3.y - loader->pp4.y,
                                          y_scale );
      }
      else
      {
        FT_Pos  height;


        /* XXX Compute top side bearing and advance height in  */
        /*     Get_VMetrics instead of here.                   */

        /* NOTE: The OS/2 values are the only `portable' ones, */
        /*       which is why we use them, if there is an OS/2 */
        /*       table in the font.  Otherwise, we use the     */
        /*       values defined in the horizontal header.      */

        height = (FT_Short)FT_DivFix( bbox.yMax - bbox.yMin,
                                      y_scale );
        if ( face->os2.version != 0xFFFFU )
          advance = (FT_Pos)( face->os2.sTypoAscender -
                              face->os2.sTypoDescender );
        else
          advance = (FT_Pos)( face->horizontal.Ascender -
                              face->horizontal.Descender );

        top = ( advance - height ) / 2;
      }

#ifdef FT_CONFIG_OPTION_INCREMENTAL
      {
        FT_Incremental_InterfaceRec*  incr;
        FT_Incremental_MetricsRec     metrics;
        FT_Error                      error;


        incr = face->root.internal->incremental_interface;

        /* If this is an incrementally loaded font see if there are */
        /* overriding metrics for this glyph.                       */
        if ( incr && incr->funcs->get_glyph_metrics )
        {
          metrics.bearing_x = 0;
          metrics.bearing_y = top;
          metrics.advance   = advance;

          error = incr->funcs->get_glyph_metrics( incr->object,
                                                  glyph_index,
                                                  TRUE,
                                                  &metrics );
          if ( error )
            return error;

          top     = metrics.bearing_y;
          advance = metrics.advance;
        }
      }

      /* GWW: Do vertical metrics get loaded incrementally too? */

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

      glyph->linearVertAdvance = advance;

      /* scale the metrics */
      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        top     = FT_MulFix( top, y_scale );
        advance = FT_MulFix( advance, y_scale );
      }

      /* XXX: for now, we have no better algorithm for the lsb, but it */
      /*      should work fine.                                        */
      /*                                                               */
      glyph->metrics.vertBearingX = ( bbox.xMin - bbox.xMax ) / 2;
      glyph->metrics.vertBearingY = top;
      glyph->metrics.vertAdvance  = advance;
    }

    /* adjust advance width to the value contained in the hdmx table */
    if ( !face->postscript.isFixedPitch &&
         IS_HINTED( loader->load_flags )        )
    {
      FT_Byte*  widthp;


      widthp = tt_face_get_device_metrics( face,
                                           size->root.metrics.x_ppem,
                                           glyph_index );

      if ( widthp )
        glyph->metrics.horiAdvance = *widthp << 6;
    }

    /* set glyph dimensions */
    glyph->metrics.width  = bbox.xMax - bbox.xMin;
    glyph->metrics.height = bbox.yMax - bbox.yMin;

    return 0;
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  static FT_Error
  load_sbit_image( TT_Size       size,
                   TT_GlyphSlot  glyph,
                   FT_UInt       glyph_index,
                   FT_Int32      load_flags )
  {
    TT_Face             face;
    SFNT_Service        sfnt;
    FT_Stream           stream;
    FT_Error            error;
    TT_SBit_MetricsRec  metrics;


    face   = (TT_Face)glyph->face;
    sfnt   = (SFNT_Service)face->sfnt;
    stream = face->root.stream;

    error = sfnt->load_sbit_image( face,
                                   size->strike_index,
                                   glyph_index,
                                   (FT_Int)load_flags,
                                   stream,
                                   &glyph->bitmap,
                                   &metrics );
    if ( !error )
    {
      glyph->outline.n_points   = 0;
      glyph->outline.n_contours = 0;

      glyph->metrics.width  = (FT_Pos)metrics.width  << 6;
      glyph->metrics.height = (FT_Pos)metrics.height << 6;

      glyph->metrics.horiBearingX = (FT_Pos)metrics.horiBearingX << 6;
      glyph->metrics.horiBearingY = (FT_Pos)metrics.horiBearingY << 6;
      glyph->metrics.horiAdvance  = (FT_Pos)metrics.horiAdvance  << 6;

      glyph->metrics.vertBearingX = (FT_Pos)metrics.vertBearingX << 6;
      glyph->metrics.vertBearingY = (FT_Pos)metrics.vertBearingY << 6;
      glyph->metrics.vertAdvance  = (FT_Pos)metrics.vertAdvance  << 6;

      glyph->format = FT_GLYPH_FORMAT_BITMAP;
      if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
      {
        glyph->bitmap_left = metrics.vertBearingX;
        glyph->bitmap_top  = metrics.vertBearingY;
      }
      else
      {
        glyph->bitmap_left = metrics.horiBearingX;
        glyph->bitmap_top  = metrics.horiBearingY;
      }
    }

    return error;
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  static FT_Error
  tt_loader_init( TT_Loader     loader,
                  TT_Size       size,
                  TT_GlyphSlot  glyph,
                  FT_Int32      load_flags )
  {
    TT_Face    face;
    FT_Stream  stream;


    face   = (TT_Face)glyph->face;
    stream = face->root.stream;

    FT_MEM_ZERO( loader, sizeof ( TT_LoaderRec ) );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    /* load execution context */
    {
      TT_ExecContext  exec;


      /* query new execution context */
      exec = size->debug ? size->context
                         : ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;
      if ( !exec )
        return TT_Err_Could_Not_Find_Context;

      TT_Load_Context( exec, face, size );

      /* see if the cvt program has disabled hinting */
      if ( exec->GS.instruct_control & 1 )
        load_flags |= FT_LOAD_NO_HINTING;

      /* load default graphics state - if needed */
      if ( exec->GS.instruct_control & 2 )
        exec->GS = tt_default_graphics_state;

      exec->pedantic_hinting = FT_BOOL( load_flags & FT_LOAD_PEDANTIC );
      exec->grayscale =
        FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) != FT_LOAD_TARGET_MONO );

      loader->exec = exec;
      loader->instructions = exec->glyphIns;
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* seek to the beginning of the glyph table.  For Type 42 fonts      */
    /* the table might be accessed from a Postscript stream or something */
    /* else...                                                           */

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( face->root.internal->incremental_interface )
      loader->glyf_offset = 0;
    else

#endif

    {
      FT_Error  error = face->goto_table( face, TTAG_glyf, stream, 0 );


      if ( error )
      {
        FT_ERROR(( "TT_Load_Glyph: could not access glyph table\n" ));
        return error;
      }
      loader->glyf_offset = FT_STREAM_POS();
    }

    /* get face's glyph loader */
    {
      FT_GlyphLoader  gloader = glyph->internal->loader;


      FT_GlyphLoader_Rewind( gloader );
      loader->gloader = gloader;
    }

    loader->load_flags    = load_flags;

    loader->face   = (FT_Face)face;
    loader->size   = (FT_Size)size;
    loader->glyph  = (FT_GlyphSlot)glyph;
    loader->stream = stream;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph       :: A handle to a target slot object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled/loaded.                              */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Load_Glyph( TT_Size       size,
                 TT_GlyphSlot  glyph,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags )
  {
    TT_Face       face;
    /*FT_Stream     stream;*/
    FT_Error      error;
    TT_LoaderRec  loader;


    face   = (TT_Face)glyph->face;
    /*stream = face->root.stream;*/
    error  = TT_Err_Ok;

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* try to load embedded bitmap if any              */
    /*                                                 */
    /* XXX: The convention should be emphasized in     */
    /*      the documents because it can be confusing. */
    if ( size->strike_index != 0xFFFFFFFFUL      &&
         ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )
    {
      error = load_sbit_image( size, glyph, glyph_index, load_flags );
      if ( !error )
        return TT_Err_Ok;
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* if FT_LOAD_NO_SCALE is not set, `ttmetrics' must be valid */
    if ( !( load_flags & FT_LOAD_NO_SCALE ) && !size->ttmetrics.valid )
      return TT_Err_Invalid_Size_Handle;

    if ( load_flags & FT_LOAD_SBITS_ONLY )
      return TT_Err_Invalid_Argument;

    error = tt_loader_init( &loader, size, glyph, load_flags );
    if ( error )
      return error;

    glyph->format        = FT_GLYPH_FORMAT_OUTLINE;
    glyph->num_subglyphs = 0;
    glyph->outline.flags = 0;

    /* Main loading loop */
    error = load_truetype_glyph( &loader, glyph_index, 0 );
    if ( !error )
    {
      if ( glyph->format == FT_GLYPH_FORMAT_COMPOSITE )
      {
        glyph->num_subglyphs = loader.gloader->base.num_subglyphs;
        glyph->subglyphs     = loader.gloader->base.subglyphs;
      }
      else
      {
        glyph->outline        = loader.gloader->base.outline;
        glyph->outline.flags &= ~FT_OUTLINE_SINGLE_PASS;

        /* In case bit 1 of the `flags' field in the `head' table isn't */
        /* set, translate array so that (0,0) is the glyph's origin.    */
        if ( ( face->header.Flags & 2 ) == 0 && loader.pp1.x )
          FT_Outline_Translate( &glyph->outline, -loader.pp1.x, 0 );
      }

      compute_glyph_metrics( &loader, glyph_index );
    }

    /* Set the `high precision' bit flag.                           */
    /* This is _critical_ to get correct output for monochrome      */
    /* TrueType glyphs at all sizes using the bytecode interpreter. */
    /*                                                              */
    if ( !( load_flags & FT_LOAD_NO_SCALE ) &&
         size->root.metrics.y_ppem < 24     )
      glyph->outline.flags |= FT_OUTLINE_HIGH_PRECISION;

    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttobjs.c                                                               */
/*                                                                         */
/*    Objects manager (body).                                              */
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
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_SFNT_H

#include "ttgload.h"
#include "ttpload.h"

#include "tterrors.h"

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#include "ttinterp.h"
#endif

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#include FT_TRUETYPE_UNPATENTED_H
#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttobjs


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

  /*************************************************************************/
  /*                                                                       */
  /*                       GLYPH ZONE FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_glyphzone_done                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Deallocate a glyph zone.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    zone :: A pointer to the target glyph zone.                        */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_glyphzone_done( TT_GlyphZone  zone )
  {
    FT_Memory  memory = zone->memory;


    if ( memory )
    {
      FT_FREE( zone->contours );
      FT_FREE( zone->tags );
      FT_FREE( zone->cur );
      FT_FREE( zone->org );

      zone->max_points   = zone->n_points   = 0;
      zone->max_contours = zone->n_contours = 0;
      zone->memory       = NULL;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_glyphzone_new                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocate a new glyph zone.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory      :: A handle to the current memory object.              */
  /*                                                                       */
  /*    maxPoints   :: The capacity of glyph zone in points.               */
  /*                                                                       */
  /*    maxContours :: The capacity of glyph zone in contours.             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    zone        :: A pointer to the target glyph zone record.          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_glyphzone_new( FT_Memory     memory,
                    FT_UShort     maxPoints,
                    FT_Short      maxContours,
                    TT_GlyphZone  zone )
  {
    FT_Error  error;


    FT_MEM_ZERO( zone, sizeof ( *zone ) );
    zone->memory = memory;

    if ( FT_NEW_ARRAY( zone->org,      maxPoints   ) ||
         FT_NEW_ARRAY( zone->cur,      maxPoints   ) ||
         FT_NEW_ARRAY( zone->tags,     maxPoints   ) ||
         FT_NEW_ARRAY( zone->contours, maxContours ) )
    {
      tt_glyphzone_done( zone );
    }
    else
    {
      zone->max_points   = maxPoints;
      zone->max_contours = maxContours;
    }

    return error;
  }
#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_init                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initialize a given TrueType face object.                           */
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
  tt_face_init( FT_Stream      stream,
                FT_Face        ttface,      /* TT_Face */
                FT_Int         face_index,
                FT_Int         num_params,
                FT_Parameter*  params )
  {
    FT_Error      error;
    FT_Library    library;
    SFNT_Service  sfnt;
    TT_Face       face = (TT_Face)ttface;


    library = face->root.driver->root.library;
    sfnt    = (SFNT_Service)FT_Get_Module_Interface( library, "sfnt" );
    if ( !sfnt )
      goto Bad_Format;

    /* create input stream from resource */
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    /* check that we have a valid TrueType file */
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( error )
      goto Exit;

    /* We must also be able to accept Mac/GX fonts, as well as OT ones. */
    /* The 0x00020000 tag is completely undocumented; some fonts from   */
    /* Arphic made for Chinese Windows 3.1 have this.                   */
    if ( face->format_tag != 0x00010000L &&    /* MS fonts  */
         face->format_tag != 0x00020000L &&    /* CJK fonts for Win 3.1 */
         face->format_tag != TTAG_true   )     /* Mac fonts */
    {
      FT_TRACE2(( "[not a valid TTF font]\n" ));
      goto Bad_Format;
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    face->root.face_flags |= FT_FACE_FLAG_HINTER;
#endif

    /* If we are performing a simple font format check, exit immediately. */
    if ( face_index < 0 )
      return TT_Err_Ok;

    /* Load font directory */
    error = sfnt->load_face( stream, face, face_index, num_params, params );
    if ( error )
      goto Exit;

    error = tt_face_load_hdmx( face, stream );
    if ( error )
      goto Exit;

    if ( face->root.face_flags & FT_FACE_FLAG_SCALABLE )
    {

#ifdef FT_CONFIG_OPTION_INCREMENTAL

      if ( !face->root.internal->incremental_interface )
        error = tt_face_load_loca( face, stream );
      if ( !error )
        error = tt_face_load_cvt( face, stream )  ||
                tt_face_load_fpgm( face, stream ) ||
                tt_face_load_prep( face, stream );

#else

      if ( !error )
        error = tt_face_load_loca( face, stream ) ||
                tt_face_load_cvt( face, stream )  ||
                tt_face_load_fpgm( face, stream ) ||
                tt_face_load_prep( face, stream );

#endif

    }

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING

    /* Determine whether unpatented hinting is to be used for this face. */
    face->unpatented_hinting = FT_BOOL
       ( library->debug_hooks[ FT_DEBUG_HOOK_UNPATENTED_HINTING ] != NULL );

    {
      int  i;


      for ( i = 0; i < num_params && !face->unpatented_hinting; i++ )
        if ( params[i].tag == FT_PARAM_TAG_UNPATENTED_HINTING )
          face->unpatented_hinting = TRUE;
    }

#endif /* TT_CONFIG_OPTION_UNPATENTED_HINTING */

    /* initialize standard glyph loading routines */
    TT_Init_Glyph_Loading( face );

  Exit:
    return error;

  Bad_Format:
    error = TT_Err_Unknown_File_Format;
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_done                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalize a given face object.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_face_done( FT_Face  ttface )           /* TT_Face */
  {
    TT_Face       face   = (TT_Face)ttface;
    FT_Memory     memory = face->root.memory;
    FT_Stream     stream = face->root.stream;

    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    /* for `extended TrueType formats' (i.e. compressed versions) */
    if ( face->extra.finalizer )
      face->extra.finalizer( face->extra.data );

    if ( sfnt )
      sfnt->done_face( face );

    /* freeing the locations table */
    tt_face_done_loca( face );

    tt_face_free_hdmx( face );

    /* freeing the CVT */
    FT_FREE( face->cvt );
    face->cvt_size = 0;

    /* freeing the programs */
    FT_FRAME_RELEASE( face->font_program );
    FT_FRAME_RELEASE( face->cvt_program );
    face->font_program_size = 0;
    face->cvt_program_size  = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    tt_done_blend( memory, face->blend );
    face->blend = NULL;
#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           SIZE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_size_run_fpgm                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Run the font program.                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the size object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_size_run_fpgm( TT_Size  size )
  {
    TT_Face         face = (TT_Face)size->root.face;
    TT_ExecContext  exec;
    FT_Error        error;


    /* debugging instances have their own context */
    if ( size->debug )
      exec = size->context;
    else
      exec = ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    TT_Load_Context( exec, face, size );

    exec->callTop   = 0;
    exec->top       = 0;

    exec->period    = 64;
    exec->phase     = 0;
    exec->threshold = 0;

    exec->instruction_trap = FALSE;
    exec->F_dot_P = 0x10000L;

    {
      FT_Size_Metrics*  metrics    = &exec->metrics;
      TT_Size_Metrics*  tt_metrics = &exec->tt_metrics;


      metrics->x_ppem   = 0;
      metrics->y_ppem   = 0;
      metrics->x_scale  = 0;
      metrics->y_scale  = 0;

      tt_metrics->ppem  = 0;
      tt_metrics->scale = 0;
      tt_metrics->ratio = 0x10000L;
    }

    /* allow font program execution */
    TT_Set_CodeRange( exec,
                      tt_coderange_font,
                      face->font_program,
                      face->font_program_size );

    /* disable CVT and glyph programs coderange */
    TT_Clear_CodeRange( exec, tt_coderange_cvt );
    TT_Clear_CodeRange( exec, tt_coderange_glyph );

    if ( face->font_program_size > 0 )
    {
      error = TT_Goto_CodeRange( exec, tt_coderange_font, 0 );

      if ( !error )
        error = face->interpreter( exec );
    }
    else
      error = TT_Err_Ok;

    if ( !error )
      TT_Save_Context( exec, size );

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_size_run_prep                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Run the control value program.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the size object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_size_run_prep( TT_Size  size )
  {
    TT_Face         face = (TT_Face)size->root.face;
    TT_ExecContext  exec;
    FT_Error        error;


    /* debugging instances have their own context */
    if ( size->debug )
      exec = size->context;
    else
      exec = ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    TT_Load_Context( exec, face, size );

    exec->callTop = 0;
    exec->top     = 0;

    exec->instruction_trap = FALSE;

    TT_Set_CodeRange( exec,
                      tt_coderange_cvt,
                      face->cvt_program,
                      face->cvt_program_size );

    TT_Clear_CodeRange( exec, tt_coderange_glyph );

    if ( face->cvt_program_size > 0 )
    {
      error = TT_Goto_CodeRange( exec, tt_coderange_cvt, 0 );

      if ( !error && !size->debug )
        error = face->interpreter( exec );
    }
    else
      error = TT_Err_Ok;

    /* save as default graphics state */
    size->GS = exec->GS;

    TT_Save_Context( exec, size );

    return error;
  }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_size_init                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initialize a new TrueType size object.                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the size object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_size_init( FT_Size  ttsize )           /* TT_Size */
  {
    TT_Size   size  = (TT_Size)ttsize;
    FT_Error  error = TT_Err_Ok;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    TT_Face    face   = (TT_Face)size->root.face;
    FT_Memory  memory = face->root.memory;
    FT_Int     i;

    FT_UShort       n_twilight;
    TT_MaxProfile*  maxp = &face->max_profile;


    size->max_function_defs    = maxp->maxFunctionDefs;
    size->max_instruction_defs = maxp->maxInstructionDefs;

    size->num_function_defs    = 0;
    size->num_instruction_defs = 0;

    size->max_func = 0;
    size->max_ins  = 0;

    size->cvt_size     = face->cvt_size;
    size->storage_size = maxp->maxStorage;

    /* Set default metrics */
    {
      FT_Size_Metrics*  metrics  = &size->root.metrics;
      TT_Size_Metrics*  metrics2 = &size->ttmetrics;


      metrics->x_ppem = 0;
      metrics->y_ppem = 0;

      metrics2->rotated   = FALSE;
      metrics2->stretched = FALSE;

      /* set default compensation (all 0) */
      for ( i = 0; i < 4; i++ )
        metrics2->compensations[i] = 0;
    }

    /* allocate function defs, instruction defs, cvt, and storage area */
    if ( FT_NEW_ARRAY( size->function_defs,    size->max_function_defs    ) ||
         FT_NEW_ARRAY( size->instruction_defs, size->max_instruction_defs ) ||
         FT_NEW_ARRAY( size->cvt,              size->cvt_size             ) ||
         FT_NEW_ARRAY( size->storage,          size->storage_size         ) )
    {
      tt_size_done( ttsize );

      return error;
    }

    /* reserve twilight zone */
    n_twilight = maxp->maxTwilightPoints;

    /* there are 4 phantom points (do we need this?) */
    n_twilight += 4;

    error = tt_glyphzone_new( memory, n_twilight, 0, &size->twilight );
    if ( error )
    {
      tt_size_done( ttsize );

      return error;
    }

    size->twilight.n_points = n_twilight;

    size->GS = tt_default_graphics_state;

    /* set `face->interpreter' according to the debug hook present */
    {
      FT_Library  library = face->root.driver->root.library;


      face->interpreter = (TT_Interpreter)
                            library->debug_hooks[FT_DEBUG_HOOK_TRUETYPE];
      if ( !face->interpreter )
        face->interpreter = (TT_Interpreter)TT_RunIns;
    }

    /* Fine, now run the font program! */
    error = tt_size_run_fpgm( size );

    if ( error )
      tt_size_done( ttsize );

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    size->ttmetrics.valid = FALSE;
    size->strike_index    = 0xFFFFFFFFUL;

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_size_done                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The TrueType size object finalizer.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_size_done( FT_Size  ttsize )           /* TT_Size */
  {
    TT_Size    size = (TT_Size)ttsize;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Memory  memory = size->root.face->memory;


    if ( size->debug )
    {
      /* the debug context must be deleted by the debugger itself */
      size->context = NULL;
      size->debug   = FALSE;
    }

    FT_FREE( size->cvt );
    size->cvt_size = 0;

    /* free storage area */
    FT_FREE( size->storage );
    size->storage_size = 0;

    /* twilight zone */
    tt_glyphzone_done( &size->twilight );

    FT_FREE( size->function_defs );
    FT_FREE( size->instruction_defs );

    size->num_function_defs    = 0;
    size->max_function_defs    = 0;
    size->num_instruction_defs = 0;
    size->max_instruction_defs = 0;

    size->max_func = 0;
    size->max_ins  = 0;

#endif

    size->ttmetrics.valid = FALSE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_size_reset                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reset a TrueType size when resolutions and character dimensions    */
  /*    have been changed.                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_size_reset( TT_Size  size )
  {
    TT_Face           face;
    FT_Error          error = TT_Err_Ok;
    FT_Size_Metrics*  metrics;


    size->ttmetrics.valid = FALSE;

    face = (TT_Face)size->root.face;

    metrics = &size->metrics;

    /* copy the result from base layer */
    *metrics = size->root.metrics;

    if ( metrics->x_ppem < 1 || metrics->y_ppem < 1 )
      return TT_Err_Invalid_PPem;

    /* This bit flag, if set, indicates that the ppems must be       */
    /* rounded to integers.  Nearly all TrueType fonts have this bit */
    /* set, as hinting won't work really well otherwise.             */
    /*                                                               */
    if ( face->header.Flags & 8 )
    {
      metrics->x_scale = FT_DivFix( metrics->x_ppem << 6,
                                    face->root.units_per_EM );
      metrics->y_scale = FT_DivFix( metrics->y_ppem << 6,
                                    face->root.units_per_EM );

      metrics->ascender =
        FT_PIX_ROUND( FT_MulFix( face->root.ascender, metrics->y_scale ) );
      metrics->descender =
        FT_PIX_ROUND( FT_MulFix( face->root.descender, metrics->y_scale ) );
      metrics->height =
        FT_PIX_ROUND( FT_MulFix( face->root.height, metrics->y_scale ) );
      metrics->max_advance =
        FT_PIX_ROUND( FT_MulFix( face->root.max_advance_width,
                                 metrics->x_scale ) );
    }

    /* compute new transformation */
    if ( metrics->x_ppem >= metrics->y_ppem )
    {
      size->ttmetrics.scale   = metrics->x_scale;
      size->ttmetrics.ppem    = metrics->x_ppem;
      size->ttmetrics.x_ratio = 0x10000L;
      size->ttmetrics.y_ratio = FT_MulDiv( metrics->y_ppem,
                                           0x10000L,
                                           metrics->x_ppem );
    }
    else
    {
      size->ttmetrics.scale   = metrics->y_scale;
      size->ttmetrics.ppem    = metrics->y_ppem;
      size->ttmetrics.x_ratio = FT_MulDiv( metrics->x_ppem,
                                           0x10000L,
                                           metrics->y_ppem );
      size->ttmetrics.y_ratio = 0x10000L;
    }


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    {
      FT_UInt  i;


      /* Scale the cvt values to the new ppem.          */
      /* We use by default the y ppem to scale the CVT. */
      for ( i = 0; i < size->cvt_size; i++ )
        size->cvt[i] = FT_MulFix( face->cvt[i], size->ttmetrics.scale );

      /* All twilight points are originally zero */
      for ( i = 0; i < (FT_UInt)size->twilight.n_points; i++ )
      {
        size->twilight.org[i].x = 0;
        size->twilight.org[i].y = 0;
        size->twilight.cur[i].x = 0;
        size->twilight.cur[i].y = 0;
      }

      /* clear storage area */
      for ( i = 0; i < (FT_UInt)size->storage_size; i++ )
        size->storage[i] = 0;

      size->GS = tt_default_graphics_state;

      error = tt_size_run_prep( size );
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    if ( !error )
      size->ttmetrics.valid = TRUE;

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_driver_init                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initialize a given TrueType driver object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_driver_init( FT_Module  ttdriver )     /* TT_Driver */
  {

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    TT_Driver  driver = (TT_Driver)ttdriver;


    if ( !TT_New_Context( driver ) )
      return TT_Err_Could_Not_Find_Context;

#else

    FT_UNUSED( ttdriver );

#endif

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_driver_done                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalize a given TrueType driver.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target TrueType driver.                  */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_driver_done( FT_Module  ttdriver )     /* TT_Driver */
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    TT_Driver  driver = (TT_Driver)ttdriver;


    /* destroy the execution context */
    if ( driver->context )
    {
      TT_Done_Context( driver->context );
      driver->context = NULL;
    }
#else
    FT_UNUSED( ttdriver );
#endif

  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_slot_init                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initialize a new slot object.                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    slot :: A handle to the slot object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_slot_init( FT_GlyphSlot  slot )
  {
    return FT_GlyphLoader_CreateExtra( slot->internal->loader );
  }


/* END */


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/***************************************************************************/
/*                                                                         */
/*  ttinterp.c                                                             */
/*                                                                         */
/*    TrueType bytecode interpreter (body).                                */
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
#include FT_TRIGONOMETRY_H
#include FT_SYSTEM_H

#include "ttinterp.h"

#include "tterrors.h"


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER


#define TT_MULFIX           FT_MulFix
#define TT_MULDIV           FT_MulDiv
#define TT_MULDIV_NO_ROUND  FT_MulDiv_No_Round


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttinterp

  /*************************************************************************/
  /*                                                                       */
  /* In order to detect infinite loops in the code, we set up a counter    */
  /* within the run loop.  A single stroke of interpretation is now        */
  /* limitet to a maximal number of opcodes defined below.                 */
  /*                                                                       */
#define MAX_RUNNABLE_OPCODES  1000000L


  /*************************************************************************/
  /*                                                                       */
  /* There are two kinds of implementations:                               */
  /*                                                                       */
  /* a. static implementation                                              */
  /*                                                                       */
  /*    The current execution context is a static variable, which fields   */
  /*    are accessed directly by the interpreter during execution.  The    */
  /*    context is named `cur'.                                            */
  /*                                                                       */
  /*    This version is non-reentrant, of course.                          */
  /*                                                                       */
  /* b. indirect implementation                                            */
  /*                                                                       */
  /*    The current execution context is passed to _each_ function as its  */
  /*    first argument, and each field is thus accessed indirectly.        */
  /*                                                                       */
  /*    This version is fully re-entrant.                                  */
  /*                                                                       */
  /* The idea is that an indirect implementation may be slower to execute  */
  /* on low-end processors that are used in some systems (like 386s or     */
  /* even 486s).                                                           */
  /*                                                                       */
  /* As a consequence, the indirect implementation is now the default, as  */
  /* its performance costs can be considered negligible in our context.    */
  /* Note, however, that we kept the same source with macros because:      */
  /*                                                                       */
  /* - The code is kept very close in design to the Pascal code used for   */
  /*   development.                                                        */
  /*                                                                       */
  /* - It's much more readable that way!                                   */
  /*                                                                       */
  /* - It's still open to experimentation and tuning.                      */
  /*                                                                       */
  /*************************************************************************/


#ifndef TT_CONFIG_OPTION_STATIC_INTERPRETER     /* indirect implementation */

#define CUR  (*exc)                             /* see ttobjs.h */

  /*************************************************************************/
  /*                                                                       */
  /* This macro is used whenever `exec' is unused in a function, to avoid  */
  /* stupid warnings from pedantic compilers.                              */
  /*                                                                       */
#define FT_UNUSED_EXEC  FT_UNUSED( exc )

#else                                           /* static implementation */

#define CUR  cur

#define FT_UNUSED_EXEC  int  __dummy = __dummy

  static
  TT_ExecContextRec  cur;   /* static exec. context variable */

  /* apparently, we have a _lot_ of direct indexing when accessing  */
  /* the static `cur', which makes the code bigger (due to all the  */
  /* four bytes addresses).                                         */

#endif /* TT_CONFIG_OPTION_STATIC_INTERPRETER */


  /*************************************************************************/
  /*                                                                       */
  /* The instruction argument stack.                                       */
  /*                                                                       */
#define INS_ARG  EXEC_OP_ FT_Long*  args    /* see ttobjs.h for EXEC_OP_ */


  /*************************************************************************/
  /*                                                                       */
  /* This macro is used whenever `args' is unused in a function, to avoid  */
  /* stupid warnings from pedantic compilers.                              */
  /*                                                                       */
#define FT_UNUSED_ARG  FT_UNUSED_EXEC; FT_UNUSED( args )


  /*************************************************************************/
  /*                                                                       */
  /* The following macros hide the use of EXEC_ARG and EXEC_ARG_ to        */
  /* increase readabilty of the code.                                      */
  /*                                                                       */
  /*************************************************************************/


#define SKIP_Code() \
          SkipCode( EXEC_ARG )

#define GET_ShortIns() \
          GetShortIns( EXEC_ARG )

#define NORMalize( x, y, v ) \
          Normalize( EXEC_ARG_ x, y, v )

#define SET_SuperRound( scale, flags ) \
          SetSuperRound( EXEC_ARG_ scale, flags )

#define ROUND_None( d, c ) \
          Round_None( EXEC_ARG_ d, c )

#define INS_Goto_CodeRange( range, ip ) \
          Ins_Goto_CodeRange( EXEC_ARG_ range, ip )

#define CUR_Func_project( x, y ) \
          CUR.func_project( EXEC_ARG_ x, y )

#define CUR_Func_move( z, p, d ) \
          CUR.func_move( EXEC_ARG_ z, p, d )

#define CUR_Func_move_orig( z, p, d ) \
          CUR.func_move_orig( EXEC_ARG_ z, p, d )

#define CUR_Func_dualproj( x, y ) \
          CUR.func_dualproj( EXEC_ARG_ x, y )

#define CUR_Func_round( d, c ) \
          CUR.func_round( EXEC_ARG_ d, c )

#define CUR_Func_read_cvt( index ) \
          CUR.func_read_cvt( EXEC_ARG_ index )

#define CUR_Func_write_cvt( index, val ) \
          CUR.func_write_cvt( EXEC_ARG_ index, val )

#define CUR_Func_move_cvt( index, val ) \
          CUR.func_move_cvt( EXEC_ARG_ index, val )

#define CURRENT_Ratio() \
          Current_Ratio( EXEC_ARG )

#define CURRENT_Ppem() \
          Current_Ppem( EXEC_ARG )

#define CUR_Ppem() \
          Cur_PPEM( EXEC_ARG )

#define INS_SxVTL( a, b, c, d ) \
          Ins_SxVTL( EXEC_ARG_ a, b, c, d )

#define COMPUTE_Funcs() \
          Compute_Funcs( EXEC_ARG )

#define COMPUTE_Round( a ) \
          Compute_Round( EXEC_ARG_ a )

#define COMPUTE_Point_Displacement( a, b, c, d ) \
          Compute_Point_Displacement( EXEC_ARG_ a, b, c, d )

#define MOVE_Zp2_Point( a, b, c, t ) \
          Move_Zp2_Point( EXEC_ARG_ a, b, c, t )


  /*************************************************************************/
  /*                                                                       */
  /* Instruction dispatch function, as used by the interpreter.            */
  /*                                                                       */
  typedef void  (*TInstruction_Function)( INS_ARG );


  /*************************************************************************/
  /*                                                                       */
  /* A simple bounds-checking macro.                                       */
  /*                                                                       */
#define BOUNDS( x, n )  ( (FT_UInt)(x) >= (FT_UInt)(n) )

#undef  SUCCESS
#define SUCCESS  0

#undef  FAILURE
#define FAILURE  1

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define GUESS_VECTOR( V )                                         \
  if ( CUR.face->unpatented_hinting )                             \
  {                                                               \
    CUR.GS.V.x = (FT_F2Dot14)( CUR.GS.both_x_axis ? 0x4000 : 0 ); \
    CUR.GS.V.y = (FT_F2Dot14)( CUR.GS.both_x_axis ? 0 : 0x4000 ); \
  }
#else
#define GUESS_VECTOR( V )
#endif

  /*************************************************************************/
  /*                                                                       */
  /*                        CODERANGE FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Goto_CodeRange                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Switches to a new code range (updates the code related elements in */
  /*    `exec', and `IP').                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range :: The new execution code range.                             */
  /*                                                                       */
  /*    IP    :: The new IP in the new code range.                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec  :: The target execution context.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Goto_CodeRange( TT_ExecContext  exec,
                     FT_Int          range,
                     FT_Long         IP )
  {
    TT_CodeRange*  coderange;


    FT_ASSERT( range >= 1 && range <= 3 );

    coderange = &exec->codeRangeTable[range - 1];

    FT_ASSERT( coderange->base != NULL );

    /* NOTE: Because the last instruction of a program may be a CALL */
    /*       which will return to the first byte *after* the code    */
    /*       range, we test for IP <= Size instead of IP < Size.     */
    /*                                                               */
    FT_ASSERT( (FT_ULong)IP <= coderange->size );

    exec->code     = coderange->base;
    exec->codeSize = coderange->size;
    exec->IP       = IP;
    exec->curRange = range;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Set_CodeRange                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets a code range.                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The code range index.                                    */
  /*                                                                       */
  /*    base   :: The new code base.                                       */
  /*                                                                       */
  /*    length :: The range size in bytes.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec   :: The target execution context.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Set_CodeRange( TT_ExecContext  exec,
                    FT_Int          range,
                    void*           base,
                    FT_Long         length )
  {
    FT_ASSERT( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = (FT_Byte*)base;
    exec->codeRangeTable[range - 1].size = length;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Clear_CodeRange                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Clears a code range.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range :: The code range index.                                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec  :: The target execution context.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Does not set the Error variable.                                   */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Clear_CodeRange( TT_ExecContext  exec,
                      FT_Int          range )
  {
    FT_ASSERT( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = NULL;
    exec->codeRangeTable[range - 1].size = 0;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                   EXECUTION CONTEXT ROUTINES                          */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Done_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given context.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec   :: A handle to the target execution context.                */
  /*                                                                       */
  /*    memory :: A handle to the parent memory object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Done_Context( TT_ExecContext  exec )
  {
    FT_Memory  memory = exec->memory;


    /* points zone */
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    /* free stack */
    FT_FREE( exec->stack );
    exec->stackSize = 0;

    /* free call stack */
    FT_FREE( exec->callStack );
    exec->callSize = 0;
    exec->callTop  = 0;

    /* free glyph code range */
    FT_FREE( exec->glyphIns );
    exec->glyphSize = 0;

    exec->size = NULL;
    exec->face = NULL;

    FT_FREE( exec );

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Init_Context                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a context object.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to the parent memory object.                    */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec   :: A handle to the target execution context.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Init_Context( TT_ExecContext  exec,
                FT_Memory       memory )
  {
    FT_Error  error;


    FT_TRACE1(( "Init_Context: new object at 0x%08p\n", exec ));

    exec->memory   = memory;
    exec->callSize = 32;

    if ( FT_NEW_ARRAY( exec->callStack, exec->callSize ) )
      goto Fail_Memory;

    /* all values in the context are set to 0 already, but this is */
    /* here as a remainder                                         */
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    exec->stackSize = 0;
    exec->glyphSize = 0;

    exec->stack     = NULL;
    exec->glyphIns  = NULL;

    exec->face = NULL;
    exec->size = NULL;

    return TT_Err_Ok;

  Fail_Memory:
    FT_ERROR(( "Init_Context: not enough memory for 0x%08lx\n",
               (FT_Long)exec ));
    TT_Done_Context( exec );

    return error;
 }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Update_Max                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks the size of a buffer and reallocates it if necessary.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory     :: A handle to the parent memory object.                */
  /*                                                                       */
  /*    multiplier :: The size in bytes of each element in the buffer.     */
  /*                                                                       */
  /*    new_max    :: The new capacity (size) of the buffer.               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size       :: The address of the buffer's current size expressed   */
  /*                  in elements.                                         */
  /*                                                                       */
  /*    buff       :: The address of the buffer base pointer.              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Update_Max( FT_Memory  memory,
              FT_ULong*  size,
              FT_Long    multiplier,
              void**     buff,
              FT_ULong   new_max )
  {
    FT_Error  error;


    if ( *size < new_max )
    {
      if ( FT_REALLOC( *buff, *size * multiplier, new_max * multiplier ) )
        return error;
      *size = new_max;
    }

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Prepare an execution context for glyph hinting.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*                                                                       */
  /*    size :: A handle to the source size object.                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec :: A handle to the target execution context.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Load_Context( TT_ExecContext  exec,
                   TT_Face         face,
                   TT_Size         size )
  {
    FT_Int          i;
    FT_ULong        tmp;
    TT_MaxProfile*  maxp;
    FT_Error        error;


    exec->face = face;
    maxp       = &face->max_profile;
    exec->size = size;

    if ( size )
    {
      exec->numFDefs   = size->num_function_defs;
      exec->maxFDefs   = size->max_function_defs;
      exec->numIDefs   = size->num_instruction_defs;
      exec->maxIDefs   = size->max_instruction_defs;
      exec->FDefs      = size->function_defs;
      exec->IDefs      = size->instruction_defs;
      exec->tt_metrics = size->ttmetrics;
      exec->metrics    = size->metrics;

      exec->maxFunc    = size->max_func;
      exec->maxIns     = size->max_ins;

      for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
        exec->codeRangeTable[i] = size->codeRangeTable[i];

      /* set graphics state */
      exec->GS = size->GS;

      exec->cvtSize = size->cvt_size;
      exec->cvt     = size->cvt;

      exec->storeSize = size->storage_size;
      exec->storage   = size->storage;

      exec->twilight  = size->twilight;
    }

    /* XXX: We reserve a little more elements on the stack to deal safely */
    /*      with broken fonts like arialbs, courbs, timesbs, etc.         */
    tmp = exec->stackSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_F26Dot6 ),
                        (void**)&exec->stack,
                        maxp->maxStackElements + 32 );
    exec->stackSize = (FT_UInt)tmp;
    if ( error )
      return error;

    tmp = exec->glyphSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_Byte ),
                        (void**)&exec->glyphIns,
                        maxp->maxSizeOfInstructions );
    exec->glyphSize = (FT_UShort)tmp;
    if ( error )
      return error;

    exec->pts.n_points   = 0;
    exec->pts.n_contours = 0;

    exec->instruction_trap = FALSE;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Save_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Saves the code ranges in a `size' object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec :: A handle to the source execution context.                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Save_Context( TT_ExecContext  exec,
                   TT_Size         size )
  {
    FT_Int  i;


    /* XXXX: Will probably disappear soon with all the code range */
    /*       management, which is now rather obsolete.            */
    /*                                                            */
    size->num_function_defs    = exec->numFDefs;
    size->num_instruction_defs = exec->numIDefs;

    size->max_func = exec->maxFunc;
    size->max_ins  = exec->maxIns;

    for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
      size->codeRangeTable[i] = exec->codeRangeTable[i];

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Run_Context                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Executes one or more instructions in the execution context.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    debug :: A Boolean flag.  If set, the function sets some internal  */
  /*             variables and returns immediately, otherwise TT_RunIns()  */
  /*             is called.                                                */
  /*                                                                       */
  /*             This is commented out currently.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec  :: A handle to the target execution context.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    TrueTyoe error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Run_Context( TT_ExecContext  exec,
                  FT_Bool         debug )
  {
    FT_Error  error;


    if ( ( error = TT_Goto_CodeRange( exec, tt_coderange_glyph, 0  ) )
           != TT_Err_Ok )
      return error;

    exec->zp0 = exec->pts;
    exec->zp1 = exec->pts;
    exec->zp2 = exec->pts;

    exec->GS.gep0 = 1;
    exec->GS.gep1 = 1;
    exec->GS.gep2 = 1;

    exec->GS.projVector.x = 0x4000;
    exec->GS.projVector.y = 0x0000;

    exec->GS.freeVector = exec->GS.projVector;
    exec->GS.dualVector = exec->GS.projVector;

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    exec->GS.both_x_axis = TRUE;
#endif

    exec->GS.round_state = 1;
    exec->GS.loop        = 1;

    /* some glyphs leave something on the stack. so we clean it */
    /* before a new execution.                                  */
    exec->top     = 0;
    exec->callTop = 0;

#if 1
    FT_UNUSED( debug );

    return exec->face->interpreter( exec );
#else
    if ( !debug )
      return TT_RunIns( exec );
    else
      return TT_Err_Ok;
#endif
  }


  const TT_GraphicsState  tt_default_graphics_state =
  {
    0, 0, 0,
    { 0x4000, 0 },
    { 0x4000, 0 },
    { 0x4000, 0 },

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    TRUE,
#endif

    1, 64, 1,
    TRUE, 68, 0, 0, 9, 3,
    0, FALSE, 2, 1, 1, 1
  };


  /* documentation is in ttinterp.h */

  FT_EXPORT_DEF( TT_ExecContext )
  TT_New_Context( TT_Driver  driver )
  {
    TT_ExecContext  exec;
    FT_Memory       memory;


    memory = driver->root.root.memory;
    exec   = driver->context;

    if ( !driver->context )
    {
      FT_Error  error;


      /* allocate object */
      if ( FT_NEW( exec ) )
        goto Exit;

      /* initialize it */
      error = Init_Context( exec, memory );
      if ( error )
        goto Fail;

      /* store it into the driver */
      driver->context = exec;
    }

  Exit:
    return driver->context;

  Fail:
    FT_FREE( exec );

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Before an opcode is executed, the interpreter verifies that there are */
  /* enough arguments on the stack, with the help of the `Pop_Push_Count'  */
  /* table.                                                                */
  /*                                                                       */
  /* For each opcode, the first column gives the number of arguments that  */
  /* are popped from the stack; the second one gives the number of those   */
  /* that are pushed in result.                                            */
  /*                                                                       */
  /* Opcodes which have a varying number of parameters in the data stream  */
  /* (NPUSHB, NPUSHW) are handled specially; they have a negative value in */
  /* the `opcode_length' table, and the value in `Pop_Push_Count' is set   */
  /* to zero.                                                              */
  /*                                                                       */
  /*************************************************************************/


#undef  PACK
#define PACK( x, y )  ( ( x << 4 ) | y )


  static
  const FT_Byte  Pop_Push_Count[256] =
  {
    /* opcodes are gathered in groups of 16 */
    /* please keep the spaces as they are   */

    /*  SVTCA  y  */  PACK( 0, 0 ),
    /*  SVTCA  x  */  PACK( 0, 0 ),
    /*  SPvTCA y  */  PACK( 0, 0 ),
    /*  SPvTCA x  */  PACK( 0, 0 ),
    /*  SFvTCA y  */  PACK( 0, 0 ),
    /*  SFvTCA x  */  PACK( 0, 0 ),
    /*  SPvTL //  */  PACK( 2, 0 ),
    /*  SPvTL +   */  PACK( 2, 0 ),
    /*  SFvTL //  */  PACK( 2, 0 ),
    /*  SFvTL +   */  PACK( 2, 0 ),
    /*  SPvFS     */  PACK( 2, 0 ),
    /*  SFvFS     */  PACK( 2, 0 ),
    /*  GPV       */  PACK( 0, 2 ),
    /*  GFV       */  PACK( 0, 2 ),
    /*  SFvTPv    */  PACK( 0, 0 ),
    /*  ISECT     */  PACK( 5, 0 ),

    /*  SRP0      */  PACK( 1, 0 ),
    /*  SRP1      */  PACK( 1, 0 ),
    /*  SRP2      */  PACK( 1, 0 ),
    /*  SZP0      */  PACK( 1, 0 ),
    /*  SZP1      */  PACK( 1, 0 ),
    /*  SZP2      */  PACK( 1, 0 ),
    /*  SZPS      */  PACK( 1, 0 ),
    /*  SLOOP     */  PACK( 1, 0 ),
    /*  RTG       */  PACK( 0, 0 ),
    /*  RTHG      */  PACK( 0, 0 ),
    /*  SMD       */  PACK( 1, 0 ),
    /*  ELSE      */  PACK( 0, 0 ),
    /*  JMPR      */  PACK( 1, 0 ),
    /*  SCvTCi    */  PACK( 1, 0 ),
    /*  SSwCi     */  PACK( 1, 0 ),
    /*  SSW       */  PACK( 1, 0 ),

    /*  DUP       */  PACK( 1, 2 ),
    /*  POP       */  PACK( 1, 0 ),
    /*  CLEAR     */  PACK( 0, 0 ),
    /*  SWAP      */  PACK( 2, 2 ),
    /*  DEPTH     */  PACK( 0, 1 ),
    /*  CINDEX    */  PACK( 1, 1 ),
    /*  MINDEX    */  PACK( 1, 0 ),
    /*  AlignPTS  */  PACK( 2, 0 ),
    /*  INS_$28   */  PACK( 0, 0 ),
    /*  UTP       */  PACK( 1, 0 ),
    /*  LOOPCALL  */  PACK( 2, 0 ),
    /*  CALL      */  PACK( 1, 0 ),
    /*  FDEF      */  PACK( 1, 0 ),
    /*  ENDF      */  PACK( 0, 0 ),
    /*  MDAP[0]   */  PACK( 1, 0 ),
    /*  MDAP[1]   */  PACK( 1, 0 ),

    /*  IUP[0]    */  PACK( 0, 0 ),
    /*  IUP[1]    */  PACK( 0, 0 ),
    /*  SHP[0]    */  PACK( 0, 0 ),
    /*  SHP[1]    */  PACK( 0, 0 ),
    /*  SHC[0]    */  PACK( 1, 0 ),
    /*  SHC[1]    */  PACK( 1, 0 ),
    /*  SHZ[0]    */  PACK( 1, 0 ),
    /*  SHZ[1]    */  PACK( 1, 0 ),
    /*  SHPIX     */  PACK( 1, 0 ),
    /*  IP        */  PACK( 0, 0 ),
    /*  MSIRP[0]  */  PACK( 2, 0 ),
    /*  MSIRP[1]  */  PACK( 2, 0 ),
    /*  AlignRP   */  PACK( 0, 0 ),
    /*  RTDG      */  PACK( 0, 0 ),
    /*  MIAP[0]   */  PACK( 2, 0 ),
    /*  MIAP[1]   */  PACK( 2, 0 ),

    /*  NPushB    */  PACK( 0, 0 ),
    /*  NPushW    */  PACK( 0, 0 ),
    /*  WS        */  PACK( 2, 0 ),
    /*  RS        */  PACK( 1, 1 ),
    /*  WCvtP     */  PACK( 2, 0 ),
    /*  RCvt      */  PACK( 1, 1 ),
    /*  GC[0]     */  PACK( 1, 1 ),
    /*  GC[1]     */  PACK( 1, 1 ),
    /*  SCFS      */  PACK( 2, 0 ),
    /*  MD[0]     */  PACK( 2, 1 ),
    /*  MD[1]     */  PACK( 2, 1 ),
    /*  MPPEM     */  PACK( 0, 1 ),
    /*  MPS       */  PACK( 0, 1 ),
    /*  FlipON    */  PACK( 0, 0 ),
    /*  FlipOFF   */  PACK( 0, 0 ),
    /*  DEBUG     */  PACK( 1, 0 ),

    /*  LT        */  PACK( 2, 1 ),
    /*  LTEQ      */  PACK( 2, 1 ),
    /*  GT        */  PACK( 2, 1 ),
    /*  GTEQ      */  PACK( 2, 1 ),
    /*  EQ        */  PACK( 2, 1 ),
    /*  NEQ       */  PACK( 2, 1 ),
    /*  ODD       */  PACK( 1, 1 ),
    /*  EVEN      */  PACK( 1, 1 ),
    /*  IF        */  PACK( 1, 0 ),
    /*  EIF       */  PACK( 0, 0 ),
    /*  AND       */  PACK( 2, 1 ),
    /*  OR        */  PACK( 2, 1 ),
    /*  NOT       */  PACK( 1, 1 ),
    /*  DeltaP1   */  PACK( 1, 0 ),
    /*  SDB       */  PACK( 1, 0 ),
    /*  SDS       */  PACK( 1, 0 ),

    /*  ADD       */  PACK( 2, 1 ),
    /*  SUB       */  PACK( 2, 1 ),
    /*  DIV       */  PACK( 2, 1 ),
    /*  MUL       */  PACK( 2, 1 ),
    /*  ABS       */  PACK( 1, 1 ),
    /*  NEG       */  PACK( 1, 1 ),
    /*  FLOOR     */  PACK( 1, 1 ),
    /*  CEILING   */  PACK( 1, 1 ),
    /*  ROUND[0]  */  PACK( 1, 1 ),
    /*  ROUND[1]  */  PACK( 1, 1 ),
    /*  ROUND[2]  */  PACK( 1, 1 ),
    /*  ROUND[3]  */  PACK( 1, 1 ),
    /*  NROUND[0] */  PACK( 1, 1 ),
    /*  NROUND[1] */  PACK( 1, 1 ),
    /*  NROUND[2] */  PACK( 1, 1 ),
    /*  NROUND[3] */  PACK( 1, 1 ),

    /*  WCvtF     */  PACK( 2, 0 ),
    /*  DeltaP2   */  PACK( 1, 0 ),
    /*  DeltaP3   */  PACK( 1, 0 ),
    /*  DeltaCn[0] */ PACK( 1, 0 ),
    /*  DeltaCn[1] */ PACK( 1, 0 ),
    /*  DeltaCn[2] */ PACK( 1, 0 ),
    /*  SROUND    */  PACK( 1, 0 ),
    /*  S45Round  */  PACK( 1, 0 ),
    /*  JROT      */  PACK( 2, 0 ),
    /*  JROF      */  PACK( 2, 0 ),
    /*  ROFF      */  PACK( 0, 0 ),
    /*  INS_$7B   */  PACK( 0, 0 ),
    /*  RUTG      */  PACK( 0, 0 ),
    /*  RDTG      */  PACK( 0, 0 ),
    /*  SANGW     */  PACK( 1, 0 ),
    /*  AA        */  PACK( 1, 0 ),

    /*  FlipPT    */  PACK( 0, 0 ),
    /*  FlipRgON  */  PACK( 2, 0 ),
    /*  FlipRgOFF */  PACK( 2, 0 ),
    /*  INS_$83   */  PACK( 0, 0 ),
    /*  INS_$84   */  PACK( 0, 0 ),
    /*  ScanCTRL  */  PACK( 1, 0 ),
    /*  SDVPTL[0] */  PACK( 2, 0 ),
    /*  SDVPTL[1] */  PACK( 2, 0 ),
    /*  GetINFO   */  PACK( 1, 1 ),
    /*  IDEF      */  PACK( 1, 0 ),
    /*  ROLL      */  PACK( 3, 3 ),
    /*  MAX       */  PACK( 2, 1 ),
    /*  MIN       */  PACK( 2, 1 ),
    /*  ScanTYPE  */  PACK( 1, 0 ),
    /*  InstCTRL  */  PACK( 2, 0 ),
    /*  INS_$8F   */  PACK( 0, 0 ),

    /*  INS_$90  */   PACK( 0, 0 ),
    /*  INS_$91  */   PACK( 0, 0 ),
    /*  INS_$92  */   PACK( 0, 0 ),
    /*  INS_$93  */   PACK( 0, 0 ),
    /*  INS_$94  */   PACK( 0, 0 ),
    /*  INS_$95  */   PACK( 0, 0 ),
    /*  INS_$96  */   PACK( 0, 0 ),
    /*  INS_$97  */   PACK( 0, 0 ),
    /*  INS_$98  */   PACK( 0, 0 ),
    /*  INS_$99  */   PACK( 0, 0 ),
    /*  INS_$9A  */   PACK( 0, 0 ),
    /*  INS_$9B  */   PACK( 0, 0 ),
    /*  INS_$9C  */   PACK( 0, 0 ),
    /*  INS_$9D  */   PACK( 0, 0 ),
    /*  INS_$9E  */   PACK( 0, 0 ),
    /*  INS_$9F  */   PACK( 0, 0 ),

    /*  INS_$A0  */   PACK( 0, 0 ),
    /*  INS_$A1  */   PACK( 0, 0 ),
    /*  INS_$A2  */   PACK( 0, 0 ),
    /*  INS_$A3  */   PACK( 0, 0 ),
    /*  INS_$A4  */   PACK( 0, 0 ),
    /*  INS_$A5  */   PACK( 0, 0 ),
    /*  INS_$A6  */   PACK( 0, 0 ),
    /*  INS_$A7  */   PACK( 0, 0 ),
    /*  INS_$A8  */   PACK( 0, 0 ),
    /*  INS_$A9  */   PACK( 0, 0 ),
    /*  INS_$AA  */   PACK( 0, 0 ),
    /*  INS_$AB  */   PACK( 0, 0 ),
    /*  INS_$AC  */   PACK( 0, 0 ),
    /*  INS_$AD  */   PACK( 0, 0 ),
    /*  INS_$AE  */   PACK( 0, 0 ),
    /*  INS_$AF  */   PACK( 0, 0 ),

    /*  PushB[0]  */  PACK( 0, 1 ),
    /*  PushB[1]  */  PACK( 0, 2 ),
    /*  PushB[2]  */  PACK( 0, 3 ),
    /*  PushB[3]  */  PACK( 0, 4 ),
    /*  PushB[4]  */  PACK( 0, 5 ),
    /*  PushB[5]  */  PACK( 0, 6 ),
    /*  PushB[6]  */  PACK( 0, 7 ),
    /*  PushB[7]  */  PACK( 0, 8 ),
    /*  PushW[0]  */  PACK( 0, 1 ),
    /*  PushW[1]  */  PACK( 0, 2 ),
    /*  PushW[2]  */  PACK( 0, 3 ),
    /*  PushW[3]  */  PACK( 0, 4 ),
    /*  PushW[4]  */  PACK( 0, 5 ),
    /*  PushW[5]  */  PACK( 0, 6 ),
    /*  PushW[6]  */  PACK( 0, 7 ),
    /*  PushW[7]  */  PACK( 0, 8 ),

    /*  MDRP[00]  */  PACK( 1, 0 ),
    /*  MDRP[01]  */  PACK( 1, 0 ),
    /*  MDRP[02]  */  PACK( 1, 0 ),
    /*  MDRP[03]  */  PACK( 1, 0 ),
    /*  MDRP[04]  */  PACK( 1, 0 ),
    /*  MDRP[05]  */  PACK( 1, 0 ),
    /*  MDRP[06]  */  PACK( 1, 0 ),
    /*  MDRP[07]  */  PACK( 1, 0 ),
    /*  MDRP[08]  */  PACK( 1, 0 ),
    /*  MDRP[09]  */  PACK( 1, 0 ),
    /*  MDRP[10]  */  PACK( 1, 0 ),
    /*  MDRP[11]  */  PACK( 1, 0 ),
    /*  MDRP[12]  */  PACK( 1, 0 ),
    /*  MDRP[13]  */  PACK( 1, 0 ),
    /*  MDRP[14]  */  PACK( 1, 0 ),
    /*  MDRP[15]  */  PACK( 1, 0 ),

    /*  MDRP[16]  */  PACK( 1, 0 ),
    /*  MDRP[17]  */  PACK( 1, 0 ),
    /*  MDRP[18]  */  PACK( 1, 0 ),
    /*  MDRP[19]  */  PACK( 1, 0 ),
    /*  MDRP[20]  */  PACK( 1, 0 ),
    /*  MDRP[21]  */  PACK( 1, 0 ),
    /*  MDRP[22]  */  PACK( 1, 0 ),
    /*  MDRP[23]  */  PACK( 1, 0 ),
    /*  MDRP[24]  */  PACK( 1, 0 ),
    /*  MDRP[25]  */  PACK( 1, 0 ),
    /*  MDRP[26]  */  PACK( 1, 0 ),
    /*  MDRP[27]  */  PACK( 1, 0 ),
    /*  MDRP[28]  */  PACK( 1, 0 ),
    /*  MDRP[29]  */  PACK( 1, 0 ),
    /*  MDRP[30]  */  PACK( 1, 0 ),
    /*  MDRP[31]  */  PACK( 1, 0 ),

    /*  MIRP[00]  */  PACK( 2, 0 ),
    /*  MIRP[01]  */  PACK( 2, 0 ),
    /*  MIRP[02]  */  PACK( 2, 0 ),
    /*  MIRP[03]  */  PACK( 2, 0 ),
    /*  MIRP[04]  */  PACK( 2, 0 ),
    /*  MIRP[05]  */  PACK( 2, 0 ),
    /*  MIRP[06]  */  PACK( 2, 0 ),
    /*  MIRP[07]  */  PACK( 2, 0 ),
    /*  MIRP[08]  */  PACK( 2, 0 ),
    /*  MIRP[09]  */  PACK( 2, 0 ),
    /*  MIRP[10]  */  PACK( 2, 0 ),
    /*  MIRP[11]  */  PACK( 2, 0 ),
    /*  MIRP[12]  */  PACK( 2, 0 ),
    /*  MIRP[13]  */  PACK( 2, 0 ),
    /*  MIRP[14]  */  PACK( 2, 0 ),
    /*  MIRP[15]  */  PACK( 2, 0 ),

    /*  MIRP[16]  */  PACK( 2, 0 ),
    /*  MIRP[17]  */  PACK( 2, 0 ),
    /*  MIRP[18]  */  PACK( 2, 0 ),
    /*  MIRP[19]  */  PACK( 2, 0 ),
    /*  MIRP[20]  */  PACK( 2, 0 ),
    /*  MIRP[21]  */  PACK( 2, 0 ),
    /*  MIRP[22]  */  PACK( 2, 0 ),
    /*  MIRP[23]  */  PACK( 2, 0 ),
    /*  MIRP[24]  */  PACK( 2, 0 ),
    /*  MIRP[25]  */  PACK( 2, 0 ),
    /*  MIRP[26]  */  PACK( 2, 0 ),
    /*  MIRP[27]  */  PACK( 2, 0 ),
    /*  MIRP[28]  */  PACK( 2, 0 ),
    /*  MIRP[29]  */  PACK( 2, 0 ),
    /*  MIRP[30]  */  PACK( 2, 0 ),
    /*  MIRP[31]  */  PACK( 2, 0 )
  };


  static
  const FT_Char  opcode_length[256] =
  {
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

   -1,-2, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    2, 3, 4, 5,  6, 7, 8, 9,  3, 5, 7, 9, 11,13,15,17,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1
  };

  static
  const FT_Vector  Null_Vector = {0,0};


#undef PACK


#undef  NULL_Vector
#define NULL_Vector  (FT_Vector*)&Null_Vector


  /* compute (a*b)/2^14 with maximal accuracy and rounding */
  static FT_Int32
  TT_MulFix14( FT_Int32  a,
               FT_Int    b )
  {
    FT_Int32   m, s, hi;
    FT_UInt32  l, lo;


    /* compute ax*bx as 64-bit value */
    l  = (FT_UInt32)( ( a & 0xFFFFU ) * b );
    m  = ( a >> 16 ) * b;

    lo = l + (FT_UInt32)( m << 16 );
    hi = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo < l );

    /* divide the result by 2^14 with rounding */
    s   = hi >> 31;
    l   = lo + (FT_UInt32)s;
    hi += s + ( l < lo );
    lo  = l;

    l   = lo + 0x2000U;
    hi += (l < lo);

    return ( hi << 18 ) | ( l >> 14 );
  }


  /* compute (ax*bx+ay*by)/2^14 with maximal accuracy and rounding */
  static FT_Int32
  TT_DotFix14( FT_Int32  ax,
               FT_Int32  ay,
               FT_Int    bx,
               FT_Int    by )
  {
    FT_Int32   m, s, hi1, hi2, hi;
    FT_UInt32  l, lo1, lo2, lo;


    /* compute ax*bx as 64-bit value */
    l = (FT_UInt32)( ( ax & 0xFFFFU ) * bx );
    m = ( ax >> 16 ) * bx;

    lo1 = l + (FT_UInt32)( m << 16 );
    hi1 = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo1 < l );

    /* compute ay*by as 64-bit value */
    l = (FT_UInt32)( ( ay & 0xFFFFU ) * by );
    m = ( ay >> 16 ) * by;

    lo2 = l + (FT_UInt32)( m << 16 );
    hi2 = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo2 < l );

    /* add them */
    lo = lo1 + lo2;
    hi = hi1 + hi2 + ( lo < lo1 );

    /* divide the result by 2^14 with rounding */
    s   = hi >> 31;
    l   = lo + (FT_UInt32)s;
    hi += s + ( l < lo );
    lo  = l;

    l   = lo + 0x2000U;
    hi += ( l < lo );

    return ( hi << 18 ) | ( l >> 14 );
  }


  /* return length of given vector */

#if 0

  static FT_Int32
  TT_VecLen( FT_Int32  x,
             FT_Int32  y )
  {
    FT_Int32   m, hi1, hi2, hi;
    FT_UInt32  l, lo1, lo2, lo;


    /* compute x*x as 64-bit value */
    lo = (FT_UInt32)( x & 0xFFFFU );
    hi = x >> 16;

    l  = lo * lo;
    m  = hi * lo;
    hi = hi * hi;

    lo1 = l + (FT_UInt32)( m << 17 );
    hi1 = hi + ( m >> 15 ) + ( lo1 < l );

    /* compute y*y as 64-bit value */
    lo = (FT_UInt32)( y & 0xFFFFU );
    hi = y >> 16;

    l  = lo * lo;
    m  = hi * lo;
    hi = hi * hi;

    lo2 = l + (FT_UInt32)( m << 17 );
    hi2 = hi + ( m >> 15 ) + ( lo2 < l );

    /* add them to get 'x*x+y*y' as 64-bit value */
    lo = lo1 + lo2;
    hi = hi1 + hi2 + ( lo < lo1 );

    /* compute the square root of this value */
    {
      FT_UInt32  root, rem, test_div;
      FT_Int     count;


      root = 0;

      {
        rem   = 0;
        count = 32;
        do
        {
          rem      = ( rem << 2 ) | ( (FT_UInt32)hi >> 30 );
          hi       = (  hi << 2 ) | (            lo >> 30 );
          lo     <<= 2;
          root   <<= 1;
          test_div = ( root << 1 ) + 1;

          if ( rem >= test_div )
          {
            rem  -= test_div;
            root += 1;
          }
        } while ( --count );
      }

      return (FT_Int32)root;
    }
  }

#else

  /* this version uses FT_Vector_Length which computes the same value */
  /* much, much faster..                                              */
  /*                                                                  */
  static FT_F26Dot6
  TT_VecLen( FT_F26Dot6  X,
             FT_F26Dot6  Y )
  {
    FT_Vector  v;


    v.x = X;
    v.y = Y;

    return FT_Vector_Length( &v );
  }

#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Current_Ratio                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the current aspect ratio scaling factor depending on the   */
  /*    projection vector's state and device resolutions.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The aspect ratio in 16.16 format, always <= 1.0 .                  */
  /*                                                                       */
  static FT_Long
  Current_Ratio( EXEC_OP )
  {
    if ( !CUR.tt_metrics.ratio )
    {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
      if ( CUR.face->unpatented_hinting )
      {
        if ( CUR.GS.both_x_axis )
          CUR.tt_metrics.ratio = CUR.tt_metrics.x_ratio;
        else
          CUR.tt_metrics.ratio = CUR.tt_metrics.y_ratio;
      }
      else
#endif
      {
        if ( CUR.GS.projVector.y == 0 )
          CUR.tt_metrics.ratio = CUR.tt_metrics.x_ratio;

        else if ( CUR.GS.projVector.x == 0 )
          CUR.tt_metrics.ratio = CUR.tt_metrics.y_ratio;

        else
        {
          FT_Long  x, y;


          x = TT_MULDIV( CUR.GS.projVector.x,
                         CUR.tt_metrics.x_ratio, 0x4000 );
          y = TT_MULDIV( CUR.GS.projVector.y,
                         CUR.tt_metrics.y_ratio, 0x4000 );
          CUR.tt_metrics.ratio = TT_VecLen( x, y );
        }
      }
    }
    return CUR.tt_metrics.ratio;
  }


  static FT_Long
  Current_Ppem( EXEC_OP )
  {
    return TT_MULFIX( CUR.tt_metrics.ppem, CURRENT_Ratio() );
  }


  /*************************************************************************/
  /*                                                                       */
  /* Functions related to the control value table (CVT).                   */
  /*                                                                       */
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT( EXEC_OP_ FT_ULong  idx )
  {
    return CUR.cvt[idx];
  }


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT_Stretched( EXEC_OP_ FT_ULong  idx )
  {
    return TT_MULFIX( CUR.cvt[idx], CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Write_CVT( EXEC_OP_ FT_ULong    idx,
                      FT_F26Dot6  value )
  {
    CUR.cvt[idx] = value;
  }


  FT_CALLBACK_DEF( void )
  Write_CVT_Stretched( EXEC_OP_ FT_ULong    idx,
                                FT_F26Dot6  value )
  {
    CUR.cvt[idx] = FT_DivFix( value, CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Move_CVT( EXEC_OP_ FT_ULong    idx,
                     FT_F26Dot6  value )
  {
    CUR.cvt[idx] += value;
  }


  FT_CALLBACK_DEF( void )
  Move_CVT_Stretched( EXEC_OP_ FT_ULong    idx,
                               FT_F26Dot6  value )
  {
    CUR.cvt[idx] += FT_DivFix( value, CURRENT_Ratio() );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    GetShortIns                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a short integer taken from the instruction stream at       */
  /*    address IP.                                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Short read at code[IP].                                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This one could become a macro.                                     */
  /*                                                                       */
  static FT_Short
  GetShortIns( EXEC_OP )
  {
    /* Reading a byte stream so there is no endianess (DaveP) */
    CUR.IP += 2;
    return (FT_Short)( ( CUR.code[CUR.IP - 2] << 8 ) +
                         CUR.code[CUR.IP - 1]      );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Ins_Goto_CodeRange                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Goes to a certain code range in the instruction stream.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    aRange :: The index of the code range.                             */
  /*                                                                       */
  /*    aIP    :: The new IP address in the code range.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS or FAILURE.                                                */
  /*                                                                       */
  static FT_Bool
  Ins_Goto_CodeRange( EXEC_OP_ FT_Int    aRange,
                               FT_ULong  aIP )
  {
    TT_CodeRange*  range;


    if ( aRange < 1 || aRange > 3 )
    {
      CUR.error = TT_Err_Bad_Argument;
      return FAILURE;
    }

    range = &CUR.codeRangeTable[aRange - 1];

    if ( range->base == NULL )     /* invalid coderange */
    {
      CUR.error = TT_Err_Invalid_CodeRange;
      return FAILURE;
    }

    /* NOTE: Because the last instruction of a program may be a CALL */
    /*       which will return to the first byte *after* the code    */
    /*       range, we test for AIP <= Size, instead of AIP < Size.  */

    if ( aIP > range->size )
    {
      CUR.error = TT_Err_Code_Overflow;
      return FAILURE;
    }

    CUR.code     = range->base;
    CUR.codeSize = range->size;
    CUR.IP       = aIP;
    CUR.curRange = aRange;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Direct_Move                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Moves a point by a given distance along the freedom vector.  The   */
  /*    point will be `touched'.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    point    :: The index of the point to move.                        */
  /*                                                                       */
  /*    distance :: The distance to apply.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    zone     :: The affected glyph zone.                               */
  /*                                                                       */
  static void
  Direct_Move( EXEC_OP_ TT_GlyphZone  zone,
                        FT_UShort     point,
                        FT_F26Dot6    distance )
  {
    FT_F26Dot6  v;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    v = CUR.GS.freeVector.x;

    if ( v != 0 )
    {
      zone->cur[point].x += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

      zone->tags[point] |= FT_CURVE_TAG_TOUCH_X;
    }

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
    {
      zone->cur[point].y += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

      zone->tags[point] |= FT_CURVE_TAG_TOUCH_Y;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Direct_Move_Orig                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Moves the *original* position of a point by a given distance along */
  /*    the freedom vector.  Obviously, the point will not be `touched'.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    point    :: The index of the point to move.                        */
  /*                                                                       */
  /*    distance :: The distance to apply.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    zone     :: The affected glyph zone.                               */
  /*                                                                       */
  static void
  Direct_Move_Orig( EXEC_OP_ TT_GlyphZone  zone,
                             FT_UShort     point,
                             FT_F26Dot6    distance )
  {
    FT_F26Dot6  v;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    v = CUR.GS.freeVector.x;

    if ( v != 0 )
      zone->org[point].x += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
      zone->org[point].y += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );
  }


  /*************************************************************************/
  /*                                                                       */
  /* Special versions of Direct_Move()                                     */
  /*                                                                       */
  /*   The following versions are used whenever both vectors are both      */
  /*   along one of the coordinate unit vectors, i.e. in 90% of the cases. */
  /*                                                                       */
  /*************************************************************************/


  static void
  Direct_Move_X( EXEC_OP_ TT_GlyphZone  zone,
                          FT_UShort     point,
                          FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].x += distance;
    zone->tags[point]  |= FT_CURVE_TAG_TOUCH_X;
  }


  static void
  Direct_Move_Y( EXEC_OP_ TT_GlyphZone  zone,
                          FT_UShort     point,
                          FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].y += distance;
    zone->tags[point]  |= FT_CURVE_TAG_TOUCH_Y;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Special versions of Direct_Move_Orig()                                */
  /*                                                                       */
  /*   The following versions are used whenever both vectors are both      */
  /*   along one of the coordinate unit vectors, i.e. in 90% of the cases. */
  /*                                                                       */
  /*************************************************************************/


  static void
  Direct_Move_Orig_X( EXEC_OP_ TT_GlyphZone  zone,
                               FT_UShort     point,
                               FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->org[point].x += distance;
  }


  static void
  Direct_Move_Orig_Y( EXEC_OP_ TT_GlyphZone  zone,
                               FT_UShort     point,
                               FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->org[point].y += distance;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_None                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Does not round, but adds engine compensation.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance (not) to round.                       */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The compensated distance.                                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The TrueType specification says very few about the relationship    */
  /*    between rounding and engine compensation.  However, it seems from  */
  /*    the description of super round that we should add the compensation */
  /*    before rounding.                                                   */
  /*                                                                       */
  static FT_F26Dot6
  Round_None( EXEC_OP_ FT_F26Dot6  distance,
                       FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( distance && val < 0 )
        val = 0;
    }
    else {
      val = distance - compensation;
      if ( val > 0 )
        val = 0;
    }
    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Grid                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to grid after adding engine compensation.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                          FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 32;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -FT_PIX_ROUND( compensation - distance );
      if ( val > 0 )
        val = 0;
    }

    return  val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Half_Grid                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to half grid after adding engine compensation.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Half_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = FT_PIX_FLOOR( distance + compensation ) + 32;
      if ( distance && val < 0 )
        val = 0;
    }
    else
    {
      val = -( FT_PIX_FLOOR( compensation - distance ) + 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Down_To_Grid                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value down to grid after adding engine compensation.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_Down_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance ) & -64 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Up_To_Grid                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value up to grid after adding engine compensation.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_Up_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                             FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    FT_UNUSED_EXEC;

    if ( distance >= 0 )
    {
      val = distance + compensation + 63;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = - FT_PIX_CEIL( compensation - distance );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Double_Grid                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to double grid after adding engine compensation.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Double_Grid( EXEC_OP_ FT_F26Dot6  distance,
                                 FT_F26Dot6  compensation )
  {
    FT_F26Dot6 val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 16;
      if ( distance && val > 0 )
        val &= ~31;
      else
        val = 0;
    }
    else
    {
      val = -FT_PAD_ROUND( compensation - distance, 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Super                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Super-rounds value to grid after adding engine compensation.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The TrueType specification says very few about the relationship    */
  /*    between rounding and engine compensation.  However, it seems from  */
  /*    the description of super round that we should add the compensation */
  /*    before rounding.                                                   */
  /*                                                                       */
  static FT_F26Dot6
  Round_Super( EXEC_OP_ FT_F26Dot6  distance,
                        FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( distance - CUR.phase + CUR.threshold + compensation ) &
              -CUR.period;
      if ( distance && val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( CUR.threshold - CUR.phase - distance + compensation ) &
               -CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Super_45                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Super-rounds value to grid after adding engine compensation.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    There is a separate function for Round_Super_45() as we may need   */
  /*    greater precision.                                                 */
  /*                                                                       */
  static FT_F26Dot6
  Round_Super_45( EXEC_OP_ FT_F26Dot6  distance,
                           FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( ( distance - CUR.phase + CUR.threshold + compensation ) /
                CUR.period ) * CUR.period;
      if ( distance && val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( ( CUR.threshold - CUR.phase - distance + compensation ) /
                   CUR.period ) * CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Compute_Round                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the rounding mode.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    round_mode :: The rounding mode to be used.                        */
  /*                                                                       */
  static void
  Compute_Round( EXEC_OP_ FT_Byte  round_mode )
  {
    switch ( round_mode )
    {
    case TT_Round_Off:
      CUR.func_round = (TT_Round_Func)Round_None;
      break;

    case TT_Round_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Grid;
      break;

    case TT_Round_Up_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;
      break;

    case TT_Round_Down_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;
      break;

    case TT_Round_To_Half_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;
      break;

    case TT_Round_To_Double_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;
      break;

    case TT_Round_Super:
      CUR.func_round = (TT_Round_Func)Round_Super;
      break;

    case TT_Round_Super_45:
      CUR.func_round = (TT_Round_Func)Round_Super_45;
      break;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    SetSuperRound                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets Super Round parameters.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    GridPeriod :: Grid period                                          */
  /*    selector   :: SROUND opcode                                        */
  /*                                                                       */
  static void
  SetSuperRound( EXEC_OP_ FT_F26Dot6  GridPeriod,
                          FT_Long     selector )
  {
    switch ( (FT_Int)( selector & 0xC0 ) )
    {
      case 0:
        CUR.period = GridPeriod / 2;
        break;

      case 0x40:
        CUR.period = GridPeriod;
        break;

      case 0x80:
        CUR.period = GridPeriod * 2;
        break;

      /* This opcode is reserved, but... */

      case 0xC0:
        CUR.period = GridPeriod;
        break;
    }

    switch ( (FT_Int)( selector & 0x30 ) )
    {
    case 0:
      CUR.phase = 0;
      break;

    case 0x10:
      CUR.phase = CUR.period / 4;
      break;

    case 0x20:
      CUR.phase = CUR.period / 2;
      break;

    case 0x30:
      CUR.phase = CUR.period * 3 / 4;
      break;
    }

    if ( (selector & 0x0F) == 0 )
      CUR.threshold = CUR.period - 1;
    else
      CUR.threshold = ( (FT_Int)( selector & 0x0F ) - 4 ) * CUR.period / 8;

    CUR.period    /= 256;
    CUR.phase     /= 256;
    CUR.threshold /= 256;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of vector given by (v2-v1) along the       */
  /*    current projection vector.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project( EXEC_OP_ FT_Vector*  v1,
                    FT_Vector*  v2 )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    return TT_DotFix14( v1->x - v2->x,
                        v1->y - v2->y,
                        CUR.GS.projVector.x,
                        CUR.GS.projVector.y );
  }

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Dual_Project                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    current dual vector.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Dual_Project( EXEC_OP_ FT_Vector*  v1,
                         FT_Vector*  v2 )
  {
    return TT_DotFix14( v1->x - v2->x,
                        v1->y - v2->y,
                        CUR.GS.dualVector.x,
                        CUR.GS.dualVector.y );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project_x                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    horizontal axis.                                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project_x( EXEC_OP_ FT_Vector*  v1,
                      FT_Vector*  v2 )
  {
    FT_UNUSED_EXEC;

    return ( v1->x - v2->x );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project_y                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    vertical axis.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project_y( EXEC_OP_ FT_Vector*  v1,
                      FT_Vector*  v2 )
  {
    FT_UNUSED_EXEC;

   return ( v1->y - v2->y );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Compute_Funcs                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection and movement function pointers according   */
  /*    to the current graphics state.                                     */
  /*                                                                       */
  static void
  Compute_Funcs( EXEC_OP )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      /* If both vectors point rightwards along the x axis, set             */
      /* `both-x-axis' true, otherwise set it false.  The x values only     */
      /* need be tested because the vector has been normalised to a unit    */
      /* vector of length 0x4000 = unity.                                   */
      CUR.GS.both_x_axis = (FT_Bool)( CUR.GS.projVector.x == 0x4000 &&
                                      CUR.GS.freeVector.x == 0x4000 );

      /* Throw away projection and freedom vector information */
      /* because the patents don't allow them to be stored.   */
      /* The relevant US Patents are 5155805 and 5325479.     */
      CUR.GS.projVector.x = 0;
      CUR.GS.projVector.y = 0;
      CUR.GS.freeVector.x = 0;
      CUR.GS.freeVector.y = 0;

      if ( CUR.GS.both_x_axis )
      {
        CUR.func_project   = Project_x;
        CUR.func_move      = Direct_Move_X;
        CUR.func_move_orig = Direct_Move_Orig_X;
      }
      else
      {
        CUR.func_project   = Project_y;
        CUR.func_move      = Direct_Move_Y;
        CUR.func_move_orig = Direct_Move_Orig_Y;
      }

      if ( CUR.GS.dualVector.x == 0x4000 )
        CUR.func_dualproj = Project_x;
      else
      {
        if ( CUR.GS.dualVector.y == 0x4000 )
          CUR.func_dualproj = Project_y;
        else
          CUR.func_dualproj = Dual_Project;
      }

      /* Force recalculation of cached aspect ratio */
      CUR.tt_metrics.ratio = 0;

      return;
    }
#endif /* TT_CONFIG_OPTION_UNPATENTED_HINTING */

    if ( CUR.GS.freeVector.x == 0x4000 )
      CUR.F_dot_P       = CUR.GS.projVector.x * 0x10000L;
    else
    {
      if ( CUR.GS.freeVector.y == 0x4000 )
        CUR.F_dot_P       = CUR.GS.projVector.y * 0x10000L;
      else
        CUR.F_dot_P = (FT_Long)CUR.GS.projVector.x * CUR.GS.freeVector.x * 4 +
                      (FT_Long)CUR.GS.projVector.y * CUR.GS.freeVector.y * 4;
    }

    if ( CUR.GS.projVector.x == 0x4000 )
      CUR.func_project = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.projVector.y == 0x4000 )
        CUR.func_project = (TT_Project_Func)Project_y;
      else
        CUR.func_project = (TT_Project_Func)Project;
    }

    if ( CUR.GS.dualVector.x == 0x4000 )
      CUR.func_dualproj = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.dualVector.y == 0x4000 )
        CUR.func_dualproj = (TT_Project_Func)Project_y;
      else
        CUR.func_dualproj = (TT_Project_Func)Dual_Project;
    }

    CUR.func_move      = (TT_Move_Func)Direct_Move;
    CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig;

    if ( CUR.F_dot_P == 0x40000000L )
    {
      if ( CUR.GS.freeVector.x == 0x4000 )
      {
        CUR.func_move      = (TT_Move_Func)Direct_Move_X;
        CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig_X;
      }
      else
      {
        if ( CUR.GS.freeVector.y == 0x4000 )
        {
          CUR.func_move      = (TT_Move_Func)Direct_Move_Y;
          CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig_Y;
        }
      }
    }

    /* at small sizes, F_dot_P can become too small, resulting   */
    /* in overflows and `spikes' in a number of glyphs like `w'. */

    if ( FT_ABS( CUR.F_dot_P ) < 0x4000000L )
      CUR.F_dot_P = 0x40000000L;

    /* Disable cached aspect ratio */
    CUR.tt_metrics.ratio = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Normalize                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Norms a vector.                                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    Vx :: The horizontal input vector coordinate.                      */
  /*    Vy :: The vertical input vector coordinate.                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    R  :: The normed unit vector.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Returns FAILURE if a vector parameter is zero.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In case Vx and Vy are both zero, Normalize() returns SUCCESS, and  */
  /*    R is undefined.                                                    */
  /*                                                                       */


  static FT_Bool
  Normalize( EXEC_OP_ FT_F26Dot6      Vx,
                      FT_F26Dot6      Vy,
                      FT_UnitVector*  R )
  {
    FT_F26Dot6  W;
    FT_Bool     S1, S2;

    FT_UNUSED_EXEC;


    if ( FT_ABS( Vx ) < 0x10000L && FT_ABS( Vy ) < 0x10000L )
    {
      Vx *= 0x100;
      Vy *= 0x100;

      W = TT_VecLen( Vx, Vy );

      if ( W == 0 )
      {
        /* XXX: UNDOCUMENTED! It seems that it is possible to try   */
        /*      to normalize the vector (0,0).  Return immediately. */
        return SUCCESS;
      }

      R->x = (FT_F2Dot14)FT_MulDiv( Vx, 0x4000L, W );
      R->y = (FT_F2Dot14)FT_MulDiv( Vy, 0x4000L, W );

      return SUCCESS;
    }

    W = TT_VecLen( Vx, Vy );

    Vx = FT_MulDiv( Vx, 0x4000L, W );
    Vy = FT_MulDiv( Vy, 0x4000L, W );

    W = Vx * Vx + Vy * Vy;

    /* Now, we want that Sqrt( W ) = 0x4000 */
    /* Or 0x10000000 <= W < 0x10004000        */

    if ( Vx < 0 )
    {
      Vx = -Vx;
      S1 = TRUE;
    }
    else
      S1 = FALSE;

    if ( Vy < 0 )
    {
      Vy = -Vy;
      S2 = TRUE;
    }
    else
      S2 = FALSE;

    while ( W < 0x10000000L )
    {
      /* We need to increase W by a minimal amount */
      if ( Vx < Vy )
        Vx++;
      else
        Vy++;

      W = Vx * Vx + Vy * Vy;
    }

    while ( W >= 0x10004000L )
    {
      /* We need to decrease W by a minimal amount */
      if ( Vx < Vy )
        Vx--;
      else
        Vy--;

      W = Vx * Vx + Vy * Vy;
    }

    /* Note that in various cases, we can only  */
    /* compute a Sqrt(W) of 0x3FFF, eg. Vx = Vy */

    if ( S1 )
      Vx = -Vx;

    if ( S2 )
      Vy = -Vy;

    R->x = (FT_F2Dot14)Vx;   /* Type conversion */
    R->y = (FT_F2Dot14)Vy;   /* Type conversion */

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Here we start with the implementation of the various opcodes.         */
  /*                                                                       */
  /*************************************************************************/


  static FT_Bool
  Ins_SxVTL( EXEC_OP_ FT_UShort       aIdx1,
                      FT_UShort       aIdx2,
                      FT_Int          aOpc,
                      FT_UnitVector*  Vec )
  {
    FT_Long     A, B, C;
    FT_Vector*  p1;
    FT_Vector*  p2;


    if ( BOUNDS( aIdx1, CUR.zp2.n_points ) ||
         BOUNDS( aIdx2, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    p1 = CUR.zp1.cur + aIdx2;
    p2 = CUR.zp2.cur + aIdx1;

    A = p1->x - p2->x;
    B = p1->y - p2->y;

    if ( ( aOpc & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, Vec );

    return SUCCESS;
  }


  /* When not using the big switch statements, the interpreter uses a */
  /* call table defined later below in this source.  Each opcode must */
  /* thus have a corresponding function, even trivial ones.           */
  /*                                                                  */
  /* They are all defined there.                                      */

#define DO_SVTCA                            \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.freeVector.y = B;                \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    GUESS_VECTOR( freeVector );             \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SFVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.freeVector.y = B;                \
                                            \
    GUESS_VECTOR( projVector );             \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.projVector ) == SUCCESS ) \
    {                                                 \
      CUR.GS.dualVector = CUR.GS.projVector;          \
      GUESS_VECTOR( freeVector );                     \
      COMPUTE_Funcs();                                \
    }


#define DO_SFVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.freeVector ) == SUCCESS ) \
    {                                                 \
      GUESS_VECTOR( projVector );                     \
      COMPUTE_Funcs();                                \
    }


#define DO_SFVTPV                          \
    GUESS_VECTOR( projVector );            \
    CUR.GS.freeVector = CUR.GS.projVector; \
    COMPUTE_Funcs();


#define DO_SPVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = (FT_Long)S;                             \
                                                \
    NORMalize( X, Y, &CUR.GS.projVector );      \
                                                \
    CUR.GS.dualVector = CUR.GS.projVector;      \
    GUESS_VECTOR( freeVector );                 \
    COMPUTE_Funcs();                            \
  }


#define DO_SFVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = S;                                      \
                                                \
    NORMalize( X, Y, &CUR.GS.freeVector );      \
    GUESS_VECTOR( projVector );                 \
    COMPUTE_Funcs();                            \
  }


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define DO_GPV                                   \
    if ( CUR.face->unpatented_hinting )          \
    {                                            \
      args[0] = CUR.GS.both_x_axis ? 0x4000 : 0; \
      args[1] = CUR.GS.both_x_axis ? 0 : 0x4000; \
    }                                            \
    else                                         \
    {                                            \
      args[0] = CUR.GS.projVector.x;             \
      args[1] = CUR.GS.projVector.y;             \
    }
#else
#define DO_GPV                                   \
    args[0] = CUR.GS.projVector.x;               \
    args[1] = CUR.GS.projVector.y;
#endif


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define DO_GFV                                   \
    if ( CUR.face->unpatented_hinting )          \
    {                                            \
      args[0] = CUR.GS.both_x_axis ? 0x4000 : 0; \
      args[1] = CUR.GS.both_x_axis ? 0 : 0x4000; \
    }                                            \
    else                                         \
    {                                            \
      args[0] = CUR.GS.freeVector.x;             \
      args[1] = CUR.GS.freeVector.y;             \
    }
#else
#define DO_GFV                                   \
    args[0] = CUR.GS.freeVector.x;               \
    args[1] = CUR.GS.freeVector.y;
#endif


#define DO_SRP0                      \
    CUR.GS.rp0 = (FT_UShort)args[0];


#define DO_SRP1                      \
    CUR.GS.rp1 = (FT_UShort)args[0];


#define DO_SRP2                      \
    CUR.GS.rp2 = (FT_UShort)args[0];


#define DO_RTHG                                         \
    CUR.GS.round_state = TT_Round_To_Half_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;


#define DO_RTG                                     \
    CUR.GS.round_state = TT_Round_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Grid;


#define DO_RTDG                                           \
    CUR.GS.round_state = TT_Round_To_Double_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;


#define DO_RUTG                                       \
    CUR.GS.round_state = TT_Round_Up_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;


#define DO_RDTG                                         \
    CUR.GS.round_state = TT_Round_Down_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;


#define DO_ROFF                                 \
    CUR.GS.round_state = TT_Round_Off;          \
    CUR.func_round = (TT_Round_Func)Round_None;


#define DO_SROUND                                \
    SET_SuperRound( 0x4000, args[0] );           \
    CUR.GS.round_state = TT_Round_Super;         \
    CUR.func_round = (TT_Round_Func)Round_Super;


#define DO_S45ROUND                                 \
    SET_SuperRound( 0x2D41, args[0] );              \
    CUR.GS.round_state = TT_Round_Super_45;         \
    CUR.func_round = (TT_Round_Func)Round_Super_45;


#define DO_SLOOP                       \
    if ( args[0] < 0 )                 \
      CUR.error = TT_Err_Bad_Argument; \
    else                               \
      CUR.GS.loop = args[0];


#define DO_SMD                         \
    CUR.GS.minimum_distance = args[0];


#define DO_SCVTCI                                     \
    CUR.GS.control_value_cutin = (FT_F26Dot6)args[0];


#define DO_SSWCI                                     \
    CUR.GS.single_width_cutin = (FT_F26Dot6)args[0];


    /* XXX: UNDOCUMENTED! or bug in the Windows engine? */
    /*                                                  */
    /* It seems that the value that is read here is     */
    /* expressed in 16.16 format rather than in font    */
    /* units.                                           */
    /*                                                  */
#define DO_SSW                                                 \
    CUR.GS.single_width_value = (FT_F26Dot6)( args[0] >> 10 );


#define DO_FLIPON            \
    CUR.GS.auto_flip = TRUE;


#define DO_FLIPOFF            \
    CUR.GS.auto_flip = FALSE;


#define DO_SDB                             \
    CUR.GS.delta_base = (FT_Short)args[0];


#define DO_SDS                              \
    CUR.GS.delta_shift = (FT_Short)args[0];


#define DO_MD  /* nothing */


#define DO_MPPEM              \
    args[0] = CURRENT_Ppem();


  /* Note: The pointSize should be irrelevant in a given font program; */
  /*       we thus decide to return only the ppem.                     */
#if 0

#define DO_MPS                       \
    args[0] = CUR.metrics.pointSize;

#else

#define DO_MPS                \
    args[0] = CURRENT_Ppem();

#endif /* 0 */


#define DO_DUP         \
    args[1] = args[0];


#define DO_CLEAR     \
    CUR.new_top = 0;


#define DO_SWAP        \
  {                    \
    FT_Long  L;        \
                       \
                       \
    L       = args[0]; \
    args[0] = args[1]; \
    args[1] = L;       \
  }


#define DO_DEPTH       \
    args[0] = CUR.top;


#define DO_CINDEX                           \
  {                                         \
    FT_Long  L;                             \
                                            \
                                            \
    L = args[0];                            \
                                            \
    if ( L <= 0 || L > CUR.args )           \
      CUR.error = TT_Err_Invalid_Reference; \
    else                                    \
      args[0] = CUR.stack[CUR.args - L];    \
  }


#define DO_JROT               \
    if ( args[1] != 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_JMPR             \
    CUR.IP      += args[0]; \
    CUR.step_ins = FALSE;


#define DO_JROF               \
    if ( args[1] == 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_LT                        \
    args[0] = ( args[0] < args[1] );


#define DO_LTEQ                       \
    args[0] = ( args[0] <= args[1] );


#define DO_GT                        \
    args[0] = ( args[0] > args[1] );


#define DO_GTEQ                       \
    args[0] = ( args[0] >= args[1] );


#define DO_EQ                         \
    args[0] = ( args[0] == args[1] );


#define DO_NEQ                        \
    args[0] = ( args[0] != args[1] );


#define DO_ODD                                                  \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 64 );


#define DO_EVEN                                                \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 0 );


#define DO_AND                        \
    args[0] = ( args[0] && args[1] );


#define DO_OR                         \
    args[0] = ( args[0] || args[1] );


#define DO_NOT          \
    args[0] = !args[0];


#define DO_ADD          \
    args[0] += args[1];


#define DO_SUB          \
    args[0] -= args[1];


#define DO_DIV                                               \
    if ( args[1] == 0 )                                      \
      CUR.error = TT_Err_Divide_By_Zero;                     \
    else                                                     \
      args[0] = TT_MULDIV_NO_ROUND( args[0], 64L, args[1] );


#define DO_MUL                                    \
    args[0] = TT_MULDIV( args[0], args[1], 64L );


#define DO_ABS                   \
    args[0] = FT_ABS( args[0] );


#define DO_NEG          \
    args[0] = -args[0];


#define DO_FLOOR    \
    args[0] = FT_PIX_FLOOR( args[0] );


#define DO_CEILING                    \
    args[0] = FT_PIX_CEIL( args[0] );


#define DO_RS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
       else                            \
         args[0] = 0;                  \
     }                                 \
     else                              \
       args[0] = CUR.storage[I];       \
   }


#define DO_WS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
     }                                 \
     else                              \
       CUR.storage[I] = args[1];       \
   }


#define DO_RCVT                          \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
       else                              \
         args[0] = 0;                    \
     }                                   \
     else                                \
       args[0] = CUR_Func_read_cvt( I ); \
   }


#define DO_WCVTP                         \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
     }                                   \
     else                                \
       CUR_Func_write_cvt( I, args[1] ); \
   }


#define DO_WCVTF                                                \
   {                                                            \
     FT_ULong  I = (FT_ULong)args[0];                           \
                                                                \
                                                                \
     if ( BOUNDS( I, CUR.cvtSize ) )                            \
     {                                                          \
       if ( CUR.pedantic_hinting )                              \
       {                                                        \
         ARRAY_BOUND_ERROR;                                     \
       }                                                        \
     }                                                          \
     else                                                       \
       CUR.cvt[I] = TT_MULFIX( args[1], CUR.tt_metrics.scale ); \
   }


#define DO_DEBUG                     \
    CUR.error = TT_Err_Debug_OpCode;


#define DO_ROUND                                                   \
    args[0] = CUR_Func_round(                                      \
                args[0],                                           \
                CUR.tt_metrics.compensations[CUR.opcode - 0x68] );


#define DO_NROUND                                                            \
    args[0] = ROUND_None( args[0],                                           \
                          CUR.tt_metrics.compensations[CUR.opcode - 0x6C] );


#define DO_MAX               \
    if ( args[1] > args[0] ) \
      args[0] = args[1];


#define DO_MIN               \
    if ( args[1] < args[0] ) \
      args[0] = args[1];


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR                   \
    {                                       \
      CUR.error = TT_Err_Invalid_Reference; \
      return;                               \
    }


  /*************************************************************************/
  /*                                                                       */
  /* SVTCA[a]:     Set (F and P) Vectors to Coordinate Axis                */
  /* Opcode range: 0x00-0x01                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SVTCA( INS_ARG )
  {
    DO_SVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVTCA[a]:    Set PVector to Coordinate Axis                          */
  /* Opcode range: 0x02-0x03                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SPVTCA( INS_ARG )
  {
    DO_SPVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTCA[a]:    Set FVector to Coordinate Axis                          */
  /* Opcode range: 0x04-0x05                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SFVTCA( INS_ARG )
  {
    DO_SFVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVTL[a]:     Set PVector To Line                                     */
  /* Opcode range: 0x06-0x07                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SPVTL( INS_ARG )
  {
    DO_SPVTL
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTL[a]:     Set FVector To Line                                     */
  /* Opcode range: 0x08-0x09                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SFVTL( INS_ARG )
  {
    DO_SFVTL
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTPV[]:     Set FVector To PVector                                  */
  /* Opcode range: 0x0E                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SFVTPV( INS_ARG )
  {
    DO_SFVTPV
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVFS[]:      Set PVector From Stack                                  */
  /* Opcode range: 0x0A                                                    */
  /* Stack:        f2.14 f2.14 -->                                         */
  /*                                                                       */
  static void
  Ins_SPVFS( INS_ARG )
  {
    DO_SPVFS
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVFS[]:      Set FVector From Stack                                  */
  /* Opcode range: 0x0B                                                    */
  /* Stack:        f2.14 f2.14 -->                                         */
  /*                                                                       */
  static void
  Ins_SFVFS( INS_ARG )
  {
    DO_SFVFS
  }


  /*************************************************************************/
  /*                                                                       */
  /* GPV[]:        Get Projection Vector                                   */
  /* Opcode range: 0x0C                                                    */
  /* Stack:        ef2.14 --> ef2.14                                       */
  /*                                                                       */
  static void
  Ins_GPV( INS_ARG )
  {
    DO_GPV
  }


  /*************************************************************************/
  /* GFV[]:        Get Freedom Vector                                      */
  /* Opcode range: 0x0D                                                    */
  /* Stack:        ef2.14 --> ef2.14                                       */
  /*                                                                       */
  static void
  Ins_GFV( INS_ARG )
  {
    DO_GFV
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP0[]:       Set Reference Point 0                                   */
  /* Opcode range: 0x10                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP0( INS_ARG )
  {
    DO_SRP0
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP1[]:       Set Reference Point 1                                   */
  /* Opcode range: 0x11                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP1( INS_ARG )
  {
    DO_SRP1
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP2[]:       Set Reference Point 2                                   */
  /* Opcode range: 0x12                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP2( INS_ARG )
  {
    DO_SRP2
  }


  /*************************************************************************/
  /*                                                                       */
  /* RTHG[]:       Round To Half Grid                                      */
  /* Opcode range: 0x19                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTHG( INS_ARG )
  {
    DO_RTHG
  }


  /*************************************************************************/
  /*                                                                       */
  /* RTG[]:        Round To Grid                                           */
  /* Opcode range: 0x18                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTG( INS_ARG )
  {
    DO_RTG
  }


  /*************************************************************************/
  /* RTDG[]:       Round To Double Grid                                    */
  /* Opcode range: 0x3D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTDG( INS_ARG )
  {
    DO_RTDG
  }


  /*************************************************************************/
  /* RUTG[]:       Round Up To Grid                                        */
  /* Opcode range: 0x7C                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RUTG( INS_ARG )
  {
    DO_RUTG
  }


  /*************************************************************************/
  /*                                                                       */
  /* RDTG[]:       Round Down To Grid                                      */
  /* Opcode range: 0x7D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RDTG( INS_ARG )
  {
    DO_RDTG
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROFF[]:       Round OFF                                               */
  /* Opcode range: 0x7A                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ROFF( INS_ARG )
  {
    DO_ROFF
  }


  /*************************************************************************/
  /*                                                                       */
  /* SROUND[]:     Super ROUND                                             */
  /* Opcode range: 0x76                                                    */
  /* Stack:        Eint8 -->                                               */
  /*                                                                       */
  static void
  Ins_SROUND( INS_ARG )
  {
    DO_SROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* S45ROUND[]:   Super ROUND 45 degrees                                  */
  /* Opcode range: 0x77                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_S45ROUND( INS_ARG )
  {
    DO_S45ROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* SLOOP[]:      Set LOOP variable                                       */
  /* Opcode range: 0x17                                                    */
  /* Stack:        int32? -->                                              */
  /*                                                                       */
  static void
  Ins_SLOOP( INS_ARG )
  {
    DO_SLOOP
  }


  /*************************************************************************/
  /*                                                                       */
  /* SMD[]:        Set Minimum Distance                                    */
  /* Opcode range: 0x1A                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SMD( INS_ARG )
  {
    DO_SMD
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCVTCI[]:     Set Control Value Table Cut In                          */
  /* Opcode range: 0x1D                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SCVTCI( INS_ARG )
  {
    DO_SCVTCI
  }


  /*************************************************************************/
  /*                                                                       */
  /* SSWCI[]:      Set Single Width Cut In                                 */
  /* Opcode range: 0x1E                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SSWCI( INS_ARG )
  {
    DO_SSWCI
  }


  /*************************************************************************/
  /*                                                                       */
  /* SSW[]:        Set Single Width                                        */
  /* Opcode range: 0x1F                                                    */
  /* Stack:        int32? -->                                              */
  /*                                                                       */
  static void
  Ins_SSW( INS_ARG )
  {
    DO_SSW
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPON[]:     Set auto-FLIP to ON                                     */
  /* Opcode range: 0x4D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_FLIPON( INS_ARG )
  {
    DO_FLIPON
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPOFF[]:    Set auto-FLIP to OFF                                    */
  /* Opcode range: 0x4E                                                    */
  /* Stack: -->                                                            */
  /*                                                                       */
  static void
  Ins_FLIPOFF( INS_ARG )
  {
    DO_FLIPOFF
  }


  /*************************************************************************/
  /*                                                                       */
  /* SANGW[]:      Set ANGle Weight                                        */
  /* Opcode range: 0x7E                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SANGW( INS_ARG )
  {
    /* instruction not supported anymore */
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDB[]:        Set Delta Base                                          */
  /* Opcode range: 0x5E                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SDB( INS_ARG )
  {
    DO_SDB
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDS[]:        Set Delta Shift                                         */
  /* Opcode range: 0x5F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SDS( INS_ARG )
  {
    DO_SDS
  }


  /*************************************************************************/
  /*                                                                       */
  /* MPPEM[]:      Measure Pixel Per EM                                    */
  /* Opcode range: 0x4B                                                    */
  /* Stack:        --> Euint16                                             */
  /*                                                                       */
  static void
  Ins_MPPEM( INS_ARG )
  {
    DO_MPPEM
  }


  /*************************************************************************/
  /*                                                                       */
  /* MPS[]:        Measure Point Size                                      */
  /* Opcode range: 0x4C                                                    */
  /* Stack:        --> Euint16                                             */
  /*                                                                       */
  static void
  Ins_MPS( INS_ARG )
  {
    DO_MPS
  }


  /*************************************************************************/
  /*                                                                       */
  /* DUP[]:        DUPlicate the top stack's element                       */
  /* Opcode range: 0x20                                                    */
  /* Stack:        StkElt --> StkElt StkElt                                */
  /*                                                                       */
  static void
  Ins_DUP( INS_ARG )
  {
    DO_DUP
  }


  /*************************************************************************/
  /*                                                                       */
  /* POP[]:        POP the stack's top element                             */
  /* Opcode range: 0x21                                                    */
  /* Stack:        StkElt -->                                              */
  /*                                                                       */
  static void
  Ins_POP( INS_ARG )
  {
    /* nothing to do */
  }


  /*************************************************************************/
  /*                                                                       */
  /* CLEAR[]:      CLEAR the entire stack                                  */
  /* Opcode range: 0x22                                                    */
  /* Stack:        StkElt... -->                                           */
  /*                                                                       */
  static void
  Ins_CLEAR( INS_ARG )
  {
    DO_CLEAR
  }


  /*************************************************************************/
  /*                                                                       */
  /* SWAP[]:       SWAP the stack's top two elements                       */
  /* Opcode range: 0x23                                                    */
  /* Stack:        2 * StkElt --> 2 * StkElt                               */
  /*                                                                       */
  static void
  Ins_SWAP( INS_ARG )
  {
    DO_SWAP
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEPTH[]:      return the stack DEPTH                                  */
  /* Opcode range: 0x24                                                    */
  /* Stack:        --> uint32                                              */
  /*                                                                       */
  static void
  Ins_DEPTH( INS_ARG )
  {
    DO_DEPTH
  }


  /*************************************************************************/
  /*                                                                       */
  /* CINDEX[]:     Copy INDEXed element                                    */
  /* Opcode range: 0x25                                                    */
  /* Stack:        int32 --> StkElt                                        */
  /*                                                                       */
  static void
  Ins_CINDEX( INS_ARG )
  {
    DO_CINDEX
  }


  /*************************************************************************/
  /*                                                                       */
  /* EIF[]:        End IF                                                  */
  /* Opcode range: 0x59                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_EIF( INS_ARG )
  {
    /* nothing to do */
  }


  /*************************************************************************/
  /*                                                                       */
  /* JROT[]:       Jump Relative On True                                   */
  /* Opcode range: 0x78                                                    */
  /* Stack:        StkElt int32 -->                                        */
  /*                                                                       */
  static void
  Ins_JROT( INS_ARG )
  {
    DO_JROT
  }


  /*************************************************************************/
  /*                                                                       */
  /* JMPR[]:       JuMP Relative                                           */
  /* Opcode range: 0x1C                                                    */
  /* Stack:        int32 -->                                               */
  /*                                                                       */
  static void
  Ins_JMPR( INS_ARG )
  {
    DO_JMPR
  }


  /*************************************************************************/
  /*                                                                       */
  /* JROF[]:       Jump Relative On False                                  */
  /* Opcode range: 0x79                                                    */
  /* Stack:        StkElt int32 -->                                        */
  /*                                                                       */
  static void
  Ins_JROF( INS_ARG )
  {
    DO_JROF
  }


  /*************************************************************************/
  /*                                                                       */
  /* LT[]:         Less Than                                               */
  /* Opcode range: 0x50                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_LT( INS_ARG )
  {
    DO_LT
  }


  /*************************************************************************/
  /*                                                                       */
  /* LTEQ[]:       Less Than or EQual                                      */
  /* Opcode range: 0x51                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_LTEQ( INS_ARG )
  {
    DO_LTEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* GT[]:         Greater Than                                            */
  /* Opcode range: 0x52                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_GT( INS_ARG )
  {
    DO_GT
  }


  /*************************************************************************/
  /*                                                                       */
  /* GTEQ[]:       Greater Than or EQual                                   */
  /* Opcode range: 0x53                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_GTEQ( INS_ARG )
  {
    DO_GTEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* EQ[]:         EQual                                                   */
  /* Opcode range: 0x54                                                    */
  /* Stack:        StkElt StkElt --> bool                                  */
  /*                                                                       */
  static void
  Ins_EQ( INS_ARG )
  {
    DO_EQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* NEQ[]:        Not EQual                                               */
  /* Opcode range: 0x55                                                    */
  /* Stack:        StkElt StkElt --> bool                                  */
  /*                                                                       */
  static void
  Ins_NEQ( INS_ARG )
  {
    DO_NEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* ODD[]:        Is ODD                                                  */
  /* Opcode range: 0x56                                                    */
  /* Stack:        f26.6 --> bool                                          */
  /*                                                                       */
  static void
  Ins_ODD( INS_ARG )
  {
    DO_ODD
  }


  /*************************************************************************/
  /*                                                                       */
  /* EVEN[]:       Is EVEN                                                 */
  /* Opcode range: 0x57                                                    */
  /* Stack:        f26.6 --> bool                                          */
  /*                                                                       */
  static void
  Ins_EVEN( INS_ARG )
  {
    DO_EVEN
  }


  /*************************************************************************/
  /*                                                                       */
  /* AND[]:        logical AND                                             */
  /* Opcode range: 0x5A                                                    */
  /* Stack:        uint32 uint32 --> uint32                                */
  /*                                                                       */
  static void
  Ins_AND( INS_ARG )
  {
    DO_AND
  }


  /*************************************************************************/
  /*                                                                       */
  /* OR[]:         logical OR                                              */
  /* Opcode range: 0x5B                                                    */
  /* Stack:        uint32 uint32 --> uint32                                */
  /*                                                                       */
  static void
  Ins_OR( INS_ARG )
  {
    DO_OR
  }


  /*************************************************************************/
  /*                                                                       */
  /* NOT[]:        logical NOT                                             */
  /* Opcode range: 0x5C                                                    */
  /* Stack:        StkElt --> uint32                                       */
  /*                                                                       */
  static void
  Ins_NOT( INS_ARG )
  {
    DO_NOT
  }


  /*************************************************************************/
  /*                                                                       */
  /* ADD[]:        ADD                                                     */
  /* Opcode range: 0x60                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_ADD( INS_ARG )
  {
    DO_ADD
  }


  /*************************************************************************/
  /*                                                                       */
  /* SUB[]:        SUBtract                                                */
  /* Opcode range: 0x61                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_SUB( INS_ARG )
  {
    DO_SUB
  }


  /*************************************************************************/
  /*                                                                       */
  /* DIV[]:        DIVide                                                  */
  /* Opcode range: 0x62                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_DIV( INS_ARG )
  {
    DO_DIV
  }


  /*************************************************************************/
  /*                                                                       */
  /* MUL[]:        MULtiply                                                */
  /* Opcode range: 0x63                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_MUL( INS_ARG )
  {
    DO_MUL
  }


  /*************************************************************************/
  /*                                                                       */
  /* ABS[]:        ABSolute value                                          */
  /* Opcode range: 0x64                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_ABS( INS_ARG )
  {
    DO_ABS
  }


  /*************************************************************************/
  /*                                                                       */
  /* NEG[]:        NEGate                                                  */
  /* Opcode range: 0x65                                                    */
  /* Stack: f26.6 --> f26.6                                                */
  /*                                                                       */
  static void
  Ins_NEG( INS_ARG )
  {
    DO_NEG
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLOOR[]:      FLOOR                                                   */
  /* Opcode range: 0x66                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_FLOOR( INS_ARG )
  {
    DO_FLOOR
  }


  /*************************************************************************/
  /*                                                                       */
  /* CEILING[]:    CEILING                                                 */
  /* Opcode range: 0x67                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_CEILING( INS_ARG )
  {
    DO_CEILING
  }


  /*************************************************************************/
  /*                                                                       */
  /* RS[]:         Read Store                                              */
  /* Opcode range: 0x43                                                    */
  /* Stack:        uint32 --> uint32                                       */
  /*                                                                       */
  static void
  Ins_RS( INS_ARG )
  {
    DO_RS
  }


  /*************************************************************************/
  /*                                                                       */
  /* WS[]:         Write Store                                             */
  /* Opcode range: 0x42                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_WS( INS_ARG )
  {
    DO_WS
  }


  /*************************************************************************/
  /*                                                                       */
  /* WCVTP[]:      Write CVT in Pixel units                                */
  /* Opcode range: 0x44                                                    */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  static void
  Ins_WCVTP( INS_ARG )
  {
    DO_WCVTP
  }


  /*************************************************************************/
  /*                                                                       */
  /* WCVTF[]:      Write CVT in Funits                                     */
  /* Opcode range: 0x70                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_WCVTF( INS_ARG )
  {
    DO_WCVTF
  }


  /*************************************************************************/
  /*                                                                       */
  /* RCVT[]:       Read CVT                                                */
  /* Opcode range: 0x45                                                    */
  /* Stack:        uint32 --> f26.6                                        */
  /*                                                                       */
  static void
  Ins_RCVT( INS_ARG )
  {
    DO_RCVT
  }


  /*************************************************************************/
  /*                                                                       */
  /* AA[]:         Adjust Angle                                            */
  /* Opcode range: 0x7F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_AA( INS_ARG )
  {
    /* intentionally no longer supported */
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEBUG[]:      DEBUG.  Unsupported.                                    */
  /* Opcode range: 0x4F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  /* Note: The original instruction pops a value from the stack.           */
  /*                                                                       */
  static void
  Ins_DEBUG( INS_ARG )
  {
    DO_DEBUG
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROUND[ab]:    ROUND value                                             */
  /* Opcode range: 0x68-0x6B                                               */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_ROUND( INS_ARG )
  {
    DO_ROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* NROUND[ab]:   No ROUNDing of value                                    */
  /* Opcode range: 0x6C-0x6F                                               */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_NROUND( INS_ARG )
  {
    DO_NROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* MAX[]:        MAXimum                                                 */
  /* Opcode range: 0x68                                                    */
  /* Stack:        int32? int32? --> int32                                 */
  /*                                                                       */
  static void
  Ins_MAX( INS_ARG )
  {
    DO_MAX
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIN[]:        MINimum                                                 */
  /* Opcode range: 0x69                                                    */
  /* Stack:        int32? int32? --> int32                                 */
  /*                                                                       */
  static void
  Ins_MIN( INS_ARG )
  {
    DO_MIN
  }


#endif  /* !TT_CONFIG_OPTION_INTERPRETER_SWITCH */


  /*************************************************************************/
  /*                                                                       */
  /* The following functions are called as is within the switch statement. */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* MINDEX[]:     Move INDEXed element                                    */
  /* Opcode range: 0x26                                                    */
  /* Stack:        int32? --> StkElt                                       */
  /*                                                                       */
  static void
  Ins_MINDEX( INS_ARG )
  {
    FT_Long  L, K;


    L = args[0];

    if ( L <= 0 || L > CUR.args )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR.stack[CUR.args - L];

    FT_ARRAY_MOVE( &CUR.stack[CUR.args - L    ],
                   &CUR.stack[CUR.args - L + 1],
                   ( L - 1 ) );

    CUR.stack[CUR.args - 1] = K;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROLL[]:       ROLL top three elements                                 */
  /* Opcode range: 0x8A                                                    */
  /* Stack:        3 * StkElt --> 3 * StkElt                               */
  /*                                                                       */
  static void
  Ins_ROLL( INS_ARG )
  {
    FT_Long  A, B, C;

    FT_UNUSED_EXEC;


    A = args[2];
    B = args[1];
    C = args[0];

    args[2] = C;
    args[1] = A;
    args[0] = B;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING THE FLOW OF CONTROL                                          */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  static FT_Bool
  SkipCode( EXEC_OP )
  {
    CUR.IP += CUR.length;

    if ( CUR.IP < CUR.codeSize )
    {
      CUR.opcode = CUR.code[CUR.IP];

      CUR.length = opcode_length[CUR.opcode];
      if ( CUR.length < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto Fail_Overflow;
        CUR.length = 2 - CUR.length * CUR.code[CUR.IP + 1];
      }

      if ( CUR.IP + CUR.length <= CUR.codeSize )
        return SUCCESS;
    }

  Fail_Overflow:
    CUR.error = TT_Err_Code_Overflow;
    return FAILURE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* IF[]:         IF test                                                 */
  /* Opcode range: 0x58                                                    */
  /* Stack:        StkElt -->                                              */
  /*                                                                       */
  static void
  Ins_IF( INS_ARG )
  {
    FT_Int   nIfs;
    FT_Bool  Out;


    if ( args[0] != 0 )
      return;

    nIfs = 1;
    Out = 0;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:      /* IF */
        nIfs++;
        break;

      case 0x1B:      /* ELSE */
        Out = FT_BOOL( nIfs == 1 );
        break;

      case 0x59:      /* EIF */
        nIfs--;
        Out = FT_BOOL( nIfs == 0 );
        break;
      }
    } while ( Out == 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* ELSE[]:       ELSE                                                    */
  /* Opcode range: 0x1B                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ELSE( INS_ARG )
  {
    FT_Int  nIfs;

    FT_UNUSED_ARG;


    nIfs = 1;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:    /* IF */
        nIfs++;
        break;

      case 0x59:    /* EIF */
        nIfs--;
        break;
      }
    } while ( nIfs != 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEFINING AND USING FUNCTIONS AND INSTRUCTIONS                         */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* FDEF[]:       Function DEFinition                                     */
  /* Opcode range: 0x2C                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_FDEF( INS_ARG )
  {
    FT_ULong       n;
    TT_DefRecord*  rec;
    TT_DefRecord*  limit;


    /* some font programs are broken enough to redefine functions! */
    /* We will then parse the current table.                       */

    rec   = CUR.FDefs;
    limit = rec + CUR.numFDefs;
    n     = args[0];

    for ( ; rec < limit; rec++ )
    {
      if ( rec->opc == n )
        break;
    }

    if ( rec == limit )
    {
      /* check that there is enough room for new functions */
      if ( CUR.numFDefs >= CUR.maxFDefs )
      {
        CUR.error = TT_Err_Too_Many_Function_Defs;
        return;
      }
      CUR.numFDefs++;
    }

    rec->range  = CUR.curRange;
    rec->opc    = n;
    rec->start  = CUR.IP + 1;
    rec->active = TRUE;

    if ( n > CUR.maxFunc )
      CUR.maxFunc = n;

    /* Now skip the whole function definition. */
    /* We don't allow nested IDEFS & FDEFs.    */

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:    /* IDEF */
      case 0x2C:    /* FDEF */
        CUR.error = TT_Err_Nested_DEFS;
        return;

      case 0x2D:   /* ENDF */
        return;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* ENDF[]:       END Function definition                                 */
  /* Opcode range: 0x2D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ENDF( INS_ARG )
  {
    TT_CallRec*  pRec;

    FT_UNUSED_ARG;


    if ( CUR.callTop <= 0 )     /* We encountered an ENDF without a call */
    {
      CUR.error = TT_Err_ENDF_In_Exec_Stream;
      return;
    }

    CUR.callTop--;

    pRec = &CUR.callStack[CUR.callTop];

    pRec->Cur_Count--;

    CUR.step_ins = FALSE;

    if ( pRec->Cur_Count > 0 )
    {
      CUR.callTop++;
      CUR.IP = pRec->Cur_Restart;
    }
    else
      /* Loop through the current function */
      INS_Goto_CodeRange( pRec->Caller_Range,
                          pRec->Caller_IP );

    /* Exit the current call frame.                      */

    /* NOTE: If the last intruction of a program is a    */
    /*       CALL or LOOPCALL, the return address is     */
    /*       always out of the code range.  This is a    */
    /*       valid address, and it is why we do not test */
    /*       the result of Ins_Goto_CodeRange() here!    */
  }


  /*************************************************************************/
  /*                                                                       */
  /* CALL[]:       CALL function                                           */
  /* Opcode range: 0x2B                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_CALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    /* first of all, check the index */

    F = args[0];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    /* Except for some old Apple fonts, all functions in a TrueType */
    /* font are defined in increasing order, starting from 0.  This */
    /* means that we normally have                                  */
    /*                                                              */
    /*    CUR.maxFunc+1 == CUR.numFDefs                             */
    /*    CUR.FDefs[n].opc == n for n in 0..CUR.maxFunc             */
    /*                                                              */
    /* If this isn't true, we need to look up the function table.   */

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      /* look up the FDefs table */
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    /* check that the function is active */
    if ( !def->active )
      goto Fail;

    /* check the call stack */
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    pCrec = CUR.callStack + CUR.callTop;

    pCrec->Caller_Range = CUR.curRange;
    pCrec->Caller_IP    = CUR.IP + 1;
    pCrec->Cur_Count    = 1;
    pCrec->Cur_Restart  = def->start;

    CUR.callTop++;

    INS_Goto_CodeRange( def->range,
                        def->start );

    CUR.step_ins = FALSE;
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  /*************************************************************************/
  /*                                                                       */
  /* LOOPCALL[]:   LOOP and CALL function                                  */
  /* Opcode range: 0x2A                                                    */
  /* Stack:        uint32? Eint16? -->                                     */
  /*                                                                       */
  static void
  Ins_LOOPCALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    /* first of all, check the index */
    F = args[1];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    /* Except for some old Apple fonts, all functions in a TrueType */
    /* font are defined in increasing order, starting from 0.  This */
    /* means that we normally have                                  */
    /*                                                              */
    /*    CUR.maxFunc+1 == CUR.numFDefs                             */
    /*    CUR.FDefs[n].opc == n for n in 0..CUR.maxFunc             */
    /*                                                              */
    /* If this isn't true, we need to look up the function table.   */

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      /* look up the FDefs table */
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    /* check that the function is active */
    if ( !def->active )
      goto Fail;

    /* check stack */
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    if ( args[0] > 0 )
    {
      pCrec = CUR.callStack + CUR.callTop;

      pCrec->Caller_Range = CUR.curRange;
      pCrec->Caller_IP    = CUR.IP + 1;
      pCrec->Cur_Count    = (FT_Int)args[0];
      pCrec->Cur_Restart  = def->start;

      CUR.callTop++;

      INS_Goto_CodeRange( def->range, def->start );

      CUR.step_ins = FALSE;
    }
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  /*************************************************************************/
  /*                                                                       */
  /* IDEF[]:       Instruction DEFinition                                  */
  /* Opcode range: 0x89                                                    */
  /* Stack:        Eint8 -->                                               */
  /*                                                                       */
  static void
  Ins_IDEF( INS_ARG )
  {
    TT_DefRecord*  def;
    TT_DefRecord*  limit;


    /*  First of all, look for the same function in our table */

    def   = CUR.IDefs;
    limit = def + CUR.numIDefs;

    for ( ; def < limit; def++ )
      if ( def->opc == (FT_ULong)args[0] )
        break;

    if ( def == limit )
    {
      /* check that there is enough room for a new instruction */
      if ( CUR.numIDefs >= CUR.maxIDefs )
      {
        CUR.error = TT_Err_Too_Many_Instruction_Defs;
        return;
      }
      CUR.numIDefs++;
    }

    def->opc    = args[0];
    def->start  = CUR.IP+1;
    def->range  = CUR.curRange;
    def->active = TRUE;

    if ( (FT_ULong)args[0] > CUR.maxIns )
      CUR.maxIns = args[0];

    /* Now skip the whole function definition. */
    /* We don't allow nested IDEFs & FDEFs.    */

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:   /* IDEF */
      case 0x2C:   /* FDEF */
        CUR.error = TT_Err_Nested_DEFS;
        return;
      case 0x2D:   /* ENDF */
        return;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHING DATA ONTO THE INTERPRETER STACK                               */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* NPUSHB[]:     PUSH N Bytes                                            */
  /* Opcode range: 0x40                                                    */
  /* Stack:        --> uint32...                                           */
  /*                                                                       */
  static void
  Ins_NPUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K + 1];

    CUR.new_top += L;
  }


  /*************************************************************************/
  /*                                                                       */
  /* NPUSHW[]:     PUSH N Words                                            */
  /* Opcode range: 0x41                                                    */
  /* Stack:        --> int32...                                            */
  /*                                                                       */
  static void
  Ins_NPUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP += 2;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
    CUR.new_top += L;
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHB[abc]:   PUSH Bytes                                              */
  /* Opcode range: 0xB0-0xB7                                               */
  /* Stack:        --> uint32...                                           */
  /*                                                                       */
  static void
  Ins_PUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)(CUR.opcode - 0xB0 + 1);

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K];
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHW[abc]:   PUSH Words                                              */
  /* Opcode range: 0xB8-0xBF                                               */
  /* Stack:        --> int32...                                            */
  /*                                                                       */
  static void
  Ins_PUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)(CUR.opcode - 0xB8 + 1);

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP++;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING THE GRAPHICS STATE                                           */
  /*                                                                       */
  /*  Instructions appear in the specs' order.                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* GC[a]:        Get Coordinate projected onto                           */
  /* Opcode range: 0x46-0x47                                               */
  /* Stack:        uint32 --> f26.6                                        */
  /*                                                                       */
  /* BULLSHIT: Measures from the original glyph must be taken along the    */
  /*           dual projection vector!                                     */
  /*                                                                       */
  static void
  Ins_GC( INS_ARG )
  {
    FT_ULong    L;
    FT_F26Dot6  R;


    L = (FT_ULong)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      else
        R = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        R = CUR_Func_dualproj( CUR.zp2.org + L, NULL_Vector );
      else
        R = CUR_Func_project( CUR.zp2.cur + L, NULL_Vector );
    }

    args[0] = R;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCFS[]:       Set Coordinate From Stack                               */
  /* Opcode range: 0x48                                                    */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  /* Formula:                                                              */
  /*                                                                       */
  /*   OA := OA + ( value - OA.p )/( f.p ) * f                             */
  /*                                                                       */
  static void
  Ins_SCFS( INS_ARG )
  {
    FT_Long    K;
    FT_UShort  L;


    L = (FT_UShort)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR_Func_project( CUR.zp2.cur + L, NULL_Vector );

    CUR_Func_move( &CUR.zp2, L, args[1] - K );

    /* not part of the specs, but here for safety */

    if ( CUR.GS.gep2 == 0 )
      CUR.zp2.org[L] = CUR.zp2.cur[L];
  }


  /*************************************************************************/
  /*                                                                       */
  /* MD[a]:        Measure Distance                                        */
  /* Opcode range: 0x49-0x4A                                               */
  /* Stack:        uint32 uint32 --> f26.6                                 */
  /*                                                                       */
  /* BULLSHIT: Measure taken in the original glyph must be along the dual  */
  /*           projection vector.                                          */
  /*                                                                       */
  /* Second BULLSHIT: Flag attributes are inverted!                        */
  /*                  0 => measure distance in original outline            */
  /*                  1 => measure distance in grid-fitted outline         */
  /*                                                                       */
  /* Third one: `zp0 - zp1', and not `zp2 - zp1!                           */
  /*                                                                       */
  static void
  Ins_MD( INS_ARG )
  {
    FT_UShort   K, L;
    FT_F26Dot6  D;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if( BOUNDS( L, CUR.zp0.n_points ) ||
        BOUNDS( K, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      D = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        D = CUR_Func_project( CUR.zp0.cur + L, CUR.zp1.cur + K );
      else
        D = CUR_Func_dualproj( CUR.zp0.org + L, CUR.zp1.org + K );
    }

    args[0] = D;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDPVTL[a]:    Set Dual PVector to Line                                */
  /* Opcode range: 0x86-0x87                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SDPVTL( INS_ARG )
  {
    FT_Long    A, B, C;
    FT_UShort  p1, p2;   /* was FT_Int in pas type ERROR */


    p1 = (FT_UShort)args[1];
    p2 = (FT_UShort)args[0];

    if ( BOUNDS( p2, CUR.zp1.n_points ) ||
         BOUNDS( p1, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    {
      FT_Vector* v1 = CUR.zp1.org + p2;
      FT_Vector* v2 = CUR.zp2.org + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.dualVector );

    {
      FT_Vector*  v1 = CUR.zp1.cur + p2;
      FT_Vector*  v2 = CUR.zp2.cur + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.projVector );

    GUESS_VECTOR( freeVector );

    COMPUTE_Funcs();
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP0[]:       Set Zone Pointer 0                                      */
  /* Opcode range: 0x13                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP0( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep0 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP1[]:       Set Zone Pointer 1                                      */
  /* Opcode range: 0x14                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP1( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp1 = CUR.twilight;
      break;

    case 1:
      CUR.zp1 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep1 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP2[]:       Set Zone Pointer 2                                      */
  /* Opcode range: 0x15                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP2( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp2 = CUR.twilight;
      break;

    case 1:
      CUR.zp2 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZPS[]:       Set Zone PointerS                                       */
  /* Opcode range: 0x16                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZPS( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.zp1 = CUR.zp0;
    CUR.zp2 = CUR.zp0;

    CUR.GS.gep0 = (FT_UShort)args[0];
    CUR.GS.gep1 = (FT_UShort)args[0];
    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* INSTCTRL[]:   INSTruction ConTRoL                                     */
  /* Opcode range: 0x8e                                                    */
  /* Stack:        int32 int32 -->                                         */
  /*                                                                       */
  static void
  Ins_INSTCTRL( INS_ARG )
  {
    FT_Long  K, L;


    K = args[1];
    L = args[0];

    if ( K < 1 || K > 2 )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( L != 0 )
        L = K;

    CUR.GS.instruct_control = FT_BOOL(
      ( (FT_Byte)CUR.GS.instruct_control & ~(FT_Byte)K ) | (FT_Byte)L );
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCANCTRL[]:   SCAN ConTRoL                                            */
  /* Opcode range: 0x85                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_SCANCTRL( INS_ARG )
  {
    FT_Int  A;


    /* Get Threshold */
    A = (FT_Int)( args[0] & 0xFF );

    if ( A == 0xFF )
    {
      CUR.GS.scan_control = TRUE;
      return;
    }
    else if ( A == 0 )
    {
      CUR.GS.scan_control = FALSE;
      return;
    }

    A *= 64;

#if 0
    if ( (args[0] & 0x100) != 0 && CUR.metrics.pointSize <= A )
      CUR.GS.scan_control = TRUE;
#endif

    if ( (args[0] & 0x200) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = TRUE;

    if ( (args[0] & 0x400) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = TRUE;

#if 0
    if ( (args[0] & 0x800) != 0 && CUR.metrics.pointSize > A )
      CUR.GS.scan_control = FALSE;
#endif

    if ( (args[0] & 0x1000) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = FALSE;

    if ( (args[0] & 0x2000) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = FALSE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCANTYPE[]:   SCAN TYPE                                               */
  /* Opcode range: 0x8D                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_SCANTYPE( INS_ARG )
  {
    /* for compatibility with future enhancements, */
    /* we must ignore new modes                    */

    if ( args[0] >= 0 && args[0] <= 5 )
    {
      if ( args[0] == 3 )
        args[0] = 2;

      CUR.GS.scan_type = (FT_Int)args[0];
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING OUTLINES                                                     */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* FLIPPT[]:     FLIP PoinT                                              */
  /* Opcode range: 0x80                                                    */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_FLIPPT( INS_ARG )
  {
    FT_UShort  point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Too_Few_Arguments;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.pts.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        CUR.pts.tags[point] ^= FT_CURVE_TAG_ON;

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPRGON[]:   FLIP RanGe ON                                           */
  /* Opcode range: 0x81                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_FLIPRGON( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] |= FT_CURVE_TAG_ON;
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPRGOFF:    FLIP RanGe OFF                                          */
  /* Opcode range: 0x82                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_FLIPRGOFF( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] &= ~FT_CURVE_TAG_ON;
  }


  static FT_Bool
  Compute_Point_Displacement( EXEC_OP_ FT_F26Dot6*   x,
                                       FT_F26Dot6*   y,
                                       TT_GlyphZone  zone,
                                       FT_UShort*    refp )
  {
    TT_GlyphZoneRec  zp;
    FT_UShort        p;
    FT_F26Dot6       d;


    if ( CUR.opcode & 1 )
    {
      zp = CUR.zp0;
      p  = CUR.GS.rp1;
    }
    else
    {
      zp = CUR.zp1;
      p  = CUR.GS.rp2;
    }

    if ( BOUNDS( p, zp.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    *zone = zp;
    *refp = p;

    d = CUR_Func_project( zp.cur + p, zp.org + p );

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        *x = d;
        *y = 0;
      }
      else
      {
        *x = 0;
        *y = d;
      }
    }
    else
#endif
    {
      *x = TT_MULDIV( d,
                      (FT_Long)CUR.GS.freeVector.x * 0x10000L,
                      CUR.F_dot_P );
      *y = TT_MULDIV( d,
                      (FT_Long)CUR.GS.freeVector.y * 0x10000L,
                      CUR.F_dot_P );
    }

    return SUCCESS;
  }


  static void
  Move_Zp2_Point( EXEC_OP_ FT_UShort   point,
                           FT_F26Dot6  dx,
                           FT_F26Dot6  dy,
                           FT_Bool     touch )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        CUR.zp2.cur[point].x += dx;
        if ( touch )
          CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_X;
      }
      else
      {
        CUR.zp2.cur[point].y += dy;
        if ( touch )
          CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_Y;
      }
      return;
    }
#endif

    if ( CUR.GS.freeVector.x != 0 )
    {
      CUR.zp2.cur[point].x += dx;
      if ( touch )
        CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_X;
    }

    if ( CUR.GS.freeVector.y != 0 )
    {
      CUR.zp2.cur[point].y += dy;
      if ( touch )
        CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_Y;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHP[a]:       SHift Point by the last point                           */
  /* Opcode range: 0x32-0x33                                               */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_SHP( INS_ARG )
  {
    TT_GlyphZoneRec  zp;
    FT_UShort        refp;

    FT_F26Dot6       dx,
                     dy;
    FT_UShort        point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;
      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        /* XXX: UNDOCUMENTED! SHP touches the points */
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHC[a]:       SHift Contour                                           */
  /* Opcode range: 0x34-35                                                 */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SHC( INS_ARG )
  {
    TT_GlyphZoneRec zp;
    FT_UShort       refp;
    FT_F26Dot6      dx,
                    dy;

    FT_Short        contour;
    FT_UShort       first_point, last_point, i;


    contour = (FT_UShort)args[0];

    if ( BOUNDS( contour, CUR.pts.n_contours ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( contour == 0 )
      first_point = 0;
    else
      first_point = (FT_UShort)(CUR.pts.contours[contour - 1] + 1);

    last_point = CUR.pts.contours[contour];

    /* XXX: this is probably wrong... at least it prevents memory */
    /*      corruption when zp2 is the twilight zone              */
    if ( last_point > CUR.zp2.n_points )
    {
      if ( CUR.zp2.n_points > 0 )
        last_point = (FT_UShort)(CUR.zp2.n_points - 1);
      else
        last_point = 0;
    }

    /* XXX: UNDOCUMENTED! SHC does touch the points */
    for ( i = first_point; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, TRUE );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHZ[a]:       SHift Zone                                              */
  /* Opcode range: 0x36-37                                                 */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SHZ( INS_ARG )
  {
    TT_GlyphZoneRec zp;
    FT_UShort       refp;
    FT_F26Dot6      dx,
                    dy;

    FT_UShort       last_point, i;


    if ( BOUNDS( args[0], 2 ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( CUR.zp2.n_points > 0 )
      last_point = (FT_UShort)(CUR.zp2.n_points - 1);
    else
      last_point = 0;

    /* XXX: UNDOCUMENTED! SHZ doesn't touch the points */
    for ( i = 0; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHPIX[]:      SHift points by a PIXel amount                          */
  /* Opcode range: 0x38                                                    */
  /* Stack:        f26.6 uint32... -->                                     */
  /*                                                                       */
  static void
  Ins_SHPIX( INS_ARG )
  {
    FT_F26Dot6  dx, dy;
    FT_UShort   point;


    if ( CUR.top < CUR.GS.loop + 1 )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        dx = TT_MulFix14( args[0], 0x4000 );
        dy = 0;
      }
      else
      {
        dx = 0;
        dy = TT_MulFix14( args[0], 0x4000 );
      }
    }
    else
#endif
    {
      dx = TT_MulFix14( args[0], CUR.GS.freeVector.x );
      dy = TT_MulFix14( args[0], CUR.GS.freeVector.y );
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MSIRP[a]:     Move Stack Indirect Relative Position                   */
  /* Opcode range: 0x3A-0x3B                                               */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  static void
  Ins_MSIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: UNDOCUMENTED! behaviour */
    if ( CUR.GS.gep1 == 0 )   /* if the point that is to be moved */
                              /* is in twilight zone              */
    {
      CUR.zp1.org[point] = CUR.zp0.org[CUR.GS.rp0];
      CUR_Func_move_orig( &CUR.zp1, point, args[1] );
      CUR.zp1.cur[point] = CUR.zp1.org[point];
    }

    distance = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, args[1] - distance );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( (CUR.opcode & 1) != 0 )
      CUR.GS.rp0 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MDAP[a]:      Move Direct Absolute Point                              */
  /* Opcode range: 0x2E-0x2F                                               */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_MDAP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  cur_dist,
                distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone? ?                                */
    if ( ( CUR.opcode & 1 ) != 0 )
    {
      cur_dist = CUR_Func_project( CUR.zp0.cur + point, NULL_Vector );
      distance = CUR_Func_round( cur_dist,
                                 CUR.tt_metrics.compensations[0] ) - cur_dist;
    }
    else
      distance = 0;

    CUR_Func_move( &CUR.zp0, point, distance );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIAP[a]:      Move Indirect Absolute Point                            */
  /* Opcode range: 0x3E-0x3F                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_MIAP( INS_ARG )
  {
    FT_ULong    cvtEntry;
    FT_UShort   point;
    FT_F26Dot6  distance,
                org_dist;


    cvtEntry = (FT_ULong)args[1];
    point    = (FT_UShort)args[0];

    if ( BOUNDS( point,    CUR.zp0.n_points ) ||
         BOUNDS( cvtEntry, CUR.cvtSize )      )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* UNDOCUMENTED!                                     */
    /*                                                   */
    /* The behaviour of an MIAP instruction is quite     */
    /* different when used in the twilight zone.         */
    /*                                                   */
    /* First, no control value cutin test is performed   */
    /* as it would fail anyway.  Second, the original    */
    /* point, i.e. (org_x,org_y) of zp0.point, is set    */
    /* to the absolute, unrounded distance found in      */
    /* the CVT.                                          */
    /*                                                   */
    /* This is used in the CVT programs of the Microsoft */
    /* fonts Arial, Times, etc., in order to re-adjust   */
    /* some key font heights.  It allows the use of the  */
    /* IP instruction in the twilight zone, which        */
    /* otherwise would be `illegal' according to the     */
    /* specification.                                    */
    /*                                                   */
    /* We implement it with a special sequence for the   */
    /* twilight zone.  This is a bad hack, but it seems  */
    /* to work.                                          */

    distance = CUR_Func_read_cvt( cvtEntry );

    if ( CUR.GS.gep0 == 0 )   /* If in twilight zone */
    {
      CUR.zp0.org[point].x = TT_MulFix14( distance, CUR.GS.freeVector.x );
      CUR.zp0.org[point].y = TT_MulFix14( distance, CUR.GS.freeVector.y ),
      CUR.zp0.cur[point]   = CUR.zp0.org[point];
    }

    org_dist = CUR_Func_project( CUR.zp0.cur + point, NULL_Vector );

    if ( ( CUR.opcode & 1 ) != 0 )   /* rounding and control cutin flag */
    {
      if ( FT_ABS( distance - org_dist ) > CUR.GS.control_value_cutin )
        distance = org_dist;

      distance = CUR_Func_round( distance, CUR.tt_metrics.compensations[0] );
    }

    CUR_Func_move( &CUR.zp0, point, distance - org_dist );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MDRP[abcde]:  Move Direct Relative Point                              */
  /* Opcode range: 0xC0-0xDF                                               */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_MDRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  org_dist, distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone?                                  */

    org_dist = CUR_Func_dualproj( CUR.zp1.org + point,
                                  CUR.zp0.org + CUR.GS.rp0 );

    /* single width cutin test */

    if ( FT_ABS( org_dist - CUR.GS.single_width_value ) <
         CUR.GS.single_width_cutin )
    {
      if ( org_dist >= 0 )
        org_dist = CUR.GS.single_width_value;
      else
        org_dist = -CUR.GS.single_width_value;
    }

    /* round flag */

    if ( ( CUR.opcode & 4 ) != 0 )
      distance = CUR_Func_round(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    else
      distance = ROUND_None(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    /* minimum distance flag */

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    /* now move the point */

    org_dist = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, distance - org_dist );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIRP[abcde]:  Move Indirect Relative Point                            */
  /* Opcode range: 0xE0-0xFF                                               */
  /* Stack:        int32? uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_MIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_ULong    cvtEntry;

    FT_F26Dot6  cvt_dist,
                distance,
                cur_dist,
                org_dist;


    point    = (FT_UShort)args[0];
    cvtEntry = (FT_ULong)( args[1] + 1 );

    /* XXX: UNDOCUMENTED! cvt[-1] = 0 always */

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( cvtEntry,   CUR.cvtSize + 1 )  ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( !cvtEntry )
      cvt_dist = 0;
    else
      cvt_dist = CUR_Func_read_cvt( cvtEntry - 1 );

    /* single width test */

    if ( FT_ABS( cvt_dist - CUR.GS.single_width_value ) <
         CUR.GS.single_width_cutin )
    {
      if ( cvt_dist >= 0 )
        cvt_dist =  CUR.GS.single_width_value;
      else
        cvt_dist = -CUR.GS.single_width_value;
    }

    /* XXX: UNDOCUMENTED! -- twilight zone */

    if ( CUR.GS.gep1 == 0 )
    {
      CUR.zp1.org[point].x = CUR.zp0.org[CUR.GS.rp0].x +
                             TT_MulFix14( cvt_dist, CUR.GS.freeVector.x );

      CUR.zp1.org[point].y = CUR.zp0.org[CUR.GS.rp0].y +
                             TT_MulFix14( cvt_dist, CUR.GS.freeVector.y );

      CUR.zp1.cur[point] = CUR.zp1.org[point];
    }

    org_dist = CUR_Func_dualproj( CUR.zp1.org + point,
                                  CUR.zp0.org + CUR.GS.rp0 );

    cur_dist = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    /* auto-flip test */

    if ( CUR.GS.auto_flip )
    {
      if ( ( org_dist ^ cvt_dist ) < 0 )
        cvt_dist = -cvt_dist;
    }

    /* control value cutin and round */

    if ( ( CUR.opcode & 4 ) != 0 )
    {
      /* XXX: UNDOCUMENTED!  Only perform cut-in test when both points */
      /*      refer to the same zone.                                  */

      if ( CUR.GS.gep0 == CUR.GS.gep1 )
        if ( FT_ABS( cvt_dist - org_dist ) >= CUR.GS.control_value_cutin )
          cvt_dist = org_dist;

      distance = CUR_Func_round(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    }
    else
      distance = ROUND_None(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    /* minimum distance test */

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    CUR_Func_move( &CUR.zp1, point, distance - cur_dist );

    CUR.GS.rp1 = CUR.GS.rp0;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;

    /* XXX: UNDOCUMENTED! */

    CUR.GS.rp2 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ALIGNRP[]:    ALIGN Relative Point                                    */
  /* Opcode range: 0x3C                                                    */
  /* Stack:        uint32 uint32... -->                                    */
  /*                                                                       */
  static void
  Ins_ALIGNRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp1.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        distance = CUR_Func_project( CUR.zp1.cur + point,
                                     CUR.zp0.cur + CUR.GS.rp0 );

        CUR_Func_move( &CUR.zp1, point, -distance );
      }

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ISECT[]:      moves point to InterSECTion                             */
  /* Opcode range: 0x0F                                                    */
  /* Stack:        5 * uint32 -->                                          */
  /*                                                                       */
  static void
  Ins_ISECT( INS_ARG )
  {
    FT_UShort   point,
                a0, a1,
                b0, b1;

    FT_F26Dot6  discriminant;

    FT_F26Dot6  dx,  dy,
                dax, day,
                dbx, dby;

    FT_F26Dot6  val;

    FT_Vector   R;


    point = (FT_UShort)args[0];

    a0 = (FT_UShort)args[1];
    a1 = (FT_UShort)args[2];
    b0 = (FT_UShort)args[3];
    b1 = (FT_UShort)args[4];

    if ( BOUNDS( b0, CUR.zp0.n_points )  ||
         BOUNDS( b1, CUR.zp0.n_points )  ||
         BOUNDS( a0, CUR.zp1.n_points )  ||
         BOUNDS( a1, CUR.zp1.n_points )  ||
         BOUNDS( point, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dbx = CUR.zp0.cur[b1].x - CUR.zp0.cur[b0].x;
    dby = CUR.zp0.cur[b1].y - CUR.zp0.cur[b0].y;

    dax = CUR.zp1.cur[a1].x - CUR.zp1.cur[a0].x;
    day = CUR.zp1.cur[a1].y - CUR.zp1.cur[a0].y;

    dx = CUR.zp0.cur[b0].x - CUR.zp1.cur[a0].x;
    dy = CUR.zp0.cur[b0].y - CUR.zp1.cur[a0].y;

    CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_BOTH;

    discriminant = TT_MULDIV( dax, -dby, 0x40 ) +
                   TT_MULDIV( day, dbx, 0x40 );

    if ( FT_ABS( discriminant ) >= 0x40 )
    {
      val = TT_MULDIV( dx, -dby, 0x40 ) + TT_MULDIV( dy, dbx, 0x40 );

      R.x = TT_MULDIV( val, dax, discriminant );
      R.y = TT_MULDIV( val, day, discriminant );

      CUR.zp2.cur[point].x = CUR.zp1.cur[a0].x + R.x;
      CUR.zp2.cur[point].y = CUR.zp1.cur[a0].y + R.y;
    }
    else
    {
      /* else, take the middle of the middles of A and B */

      CUR.zp2.cur[point].x = ( CUR.zp1.cur[a0].x +
                               CUR.zp1.cur[a1].x +
                               CUR.zp0.cur[b0].x +
                               CUR.zp0.cur[b1].x ) / 4;
      CUR.zp2.cur[point].y = ( CUR.zp1.cur[a0].y +
                               CUR.zp1.cur[a1].y +
                               CUR.zp0.cur[b0].y +
                               CUR.zp0.cur[b1].y ) / 4;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* ALIGNPTS[]:   ALIGN PoinTS                                            */
  /* Opcode range: 0x27                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_ALIGNPTS( INS_ARG )
  {
    FT_UShort   p1, p2;
    FT_F26Dot6  distance;


    p1 = (FT_UShort)args[0];
    p2 = (FT_UShort)args[1];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) ||
         BOUNDS( args[1], CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    distance = CUR_Func_project( CUR.zp0.cur + p2,
                                 CUR.zp1.cur + p1 ) / 2;

    CUR_Func_move( &CUR.zp1, p1, distance );
    CUR_Func_move( &CUR.zp0, p2, -distance );
  }


  /*************************************************************************/
  /*                                                                       */
  /* IP[]:         Interpolate Point                                       */
  /* Opcode range: 0x39                                                    */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_IP( INS_ARG )
  {
    FT_F26Dot6  org_a, org_b, org_x,
                cur_a, cur_b, cur_x,
                distance;
    FT_UShort   point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: There are some glyphs in some braindead but popular  */
    /*      fonts out there (e.g. [aeu]grave in monotype.ttf)    */
    /*      calling IP[] with bad values of rp[12].              */
    /*      Do something sane when this odd thing happens.       */

    if ( BOUNDS( CUR.GS.rp1, CUR.zp0.n_points ) ||
         BOUNDS( CUR.GS.rp2, CUR.zp1.n_points ) )
    {
      org_a = cur_a = 0;
      org_b = cur_b = 0;
    }
    else
    {
      org_a = CUR_Func_dualproj( CUR.zp0.org + CUR.GS.rp1, NULL_Vector );
      org_b = CUR_Func_dualproj( CUR.zp1.org + CUR.GS.rp2, NULL_Vector );

      cur_a = CUR_Func_project( CUR.zp0.cur + CUR.GS.rp1, NULL_Vector );
      cur_b = CUR_Func_project( CUR.zp1.cur + CUR.GS.rp2, NULL_Vector );
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];
      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        org_x = CUR_Func_dualproj( CUR.zp2.org + point, NULL_Vector );
        cur_x = CUR_Func_project ( CUR.zp2.cur + point, NULL_Vector );

        if ( ( org_a <= org_b && org_x <= org_a ) ||
             ( org_a >  org_b && org_x >= org_a ) )

          distance = ( cur_a - org_a ) + ( org_x - cur_x );

        else if ( ( org_a <= org_b  &&  org_x >= org_b ) ||
                  ( org_a >  org_b  &&  org_x <  org_b ) )

          distance = ( cur_b - org_b ) + ( org_x - cur_x );

        else
           /* note: it seems that rounding this value isn't a good */
           /*       idea (cf. width of capital `S' in Times)       */

           distance = TT_MULDIV( cur_b - cur_a,
                                 org_x - org_a,
                                 org_b - org_a ) + ( cur_a - cur_x );

        CUR_Func_move( &CUR.zp2, point, distance );
      }

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* UTP[a]:       UnTouch Point                                           */
  /* Opcode range: 0x29                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_UTP( INS_ARG )
  {
    FT_UShort  point;
    FT_Byte    mask;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    mask = 0xFF;

    if ( CUR.GS.freeVector.x != 0 )
      mask &= ~FT_CURVE_TAG_TOUCH_X;

    if ( CUR.GS.freeVector.y != 0 )
      mask &= ~FT_CURVE_TAG_TOUCH_Y;

    CUR.zp0.tags[point] &= mask;
  }


  /* Local variables for Ins_IUP: */
  struct  LOC_Ins_IUP
  {
    FT_Vector*  orgs;   /* original and current coordinate */
    FT_Vector*  curs;   /* arrays                          */
  };


  static void
  Shift( FT_UInt              p1,
         FT_UInt              p2,
         FT_UInt              p,
         struct LOC_Ins_IUP*  LINK )
  {
    FT_UInt     i;
    FT_F26Dot6  x;


    x = LINK->curs[p].x - LINK->orgs[p].x;

    for ( i = p1; i < p; i++ )
      LINK->curs[i].x += x;

    for ( i = p + 1; i <= p2; i++ )
      LINK->curs[i].x += x;
  }


  static void
  Interp( FT_UInt              p1,
          FT_UInt              p2,
          FT_UInt              ref1,
          FT_UInt              ref2,
          struct LOC_Ins_IUP*  LINK )
  {
    FT_UInt     i;
    FT_F26Dot6  x, x1, x2, d1, d2;


    if ( p1 > p2 )
      return;

    x1 = LINK->orgs[ref1].x;
    d1 = LINK->curs[ref1].x - LINK->orgs[ref1].x;
    x2 = LINK->orgs[ref2].x;
    d2 = LINK->curs[ref2].x - LINK->orgs[ref2].x;

    if ( x1 == x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i].x;

        if ( x <= x1 )
          x += d1;
        else
          x += d2;

        LINK->curs[i].x = x;
      }
      return;
    }

    if ( x1 < x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i].x;

        if ( x <= x1 )
          x += d1;
        else
        {
          if ( x >= x2 )
            x += d2;
          else
            x = LINK->curs[ref1].x +
                  TT_MULDIV( x - x1,
                             LINK->curs[ref2].x - LINK->curs[ref1].x,
                             x2 - x1 );
        }
        LINK->curs[i].x = x;
      }
      return;
    }

    /* x2 < x1 */

    for ( i = p1; i <= p2; i++ )
    {
      x = LINK->orgs[i].x;
      if ( x <= x2 )
        x += d2;
      else
      {
        if ( x >= x1 )
          x += d1;
        else
          x = LINK->curs[ref1].x +
              TT_MULDIV( x - x1,
                         LINK->curs[ref2].x - LINK->curs[ref1].x,
                         x2 - x1 );
      }
      LINK->curs[i].x = x;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* IUP[a]:       Interpolate Untouched Points                            */
  /* Opcode range: 0x30-0x31                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_IUP( INS_ARG )
  {
    struct LOC_Ins_IUP  V;
    FT_Byte             mask;

    FT_UInt   first_point;   /* first point of contour        */
    FT_UInt   end_point;     /* end point (last+1) of contour */

    FT_UInt   first_touched; /* first touched point in contour   */
    FT_UInt   cur_touched;   /* current touched point in contour */

    FT_UInt   point;         /* current point   */
    FT_Short  contour;       /* current contour */

    FT_UNUSED_ARG;


    if ( CUR.opcode & 1 )
    {
      mask   = FT_CURVE_TAG_TOUCH_X;
      V.orgs = CUR.pts.org;
      V.curs = CUR.pts.cur;
    }
    else
    {
      mask   = FT_CURVE_TAG_TOUCH_Y;
      V.orgs = (FT_Vector*)( (FT_Pos*)CUR.pts.org + 1 );
      V.curs = (FT_Vector*)( (FT_Pos*)CUR.pts.cur + 1 );
    }

    contour = 0;
    point   = 0;

    do
    {
      end_point   = CUR.pts.contours[contour];
      first_point = point;

      while ( point <= end_point && (CUR.pts.tags[point] & mask) == 0 )
        point++;

      if ( point <= end_point )
      {
        first_touched = point;
        cur_touched   = point;

        point++;

        while ( point <= end_point )
        {
          if ( ( CUR.pts.tags[point] & mask ) != 0 )
          {
            if ( point > 0 )
              Interp( cur_touched + 1,
                      point - 1,
                      cur_touched,
                      point,
                      &V );
            cur_touched = point;
          }

          point++;
        }

        if ( cur_touched == first_touched )
          Shift( first_point, end_point, cur_touched, &V );
        else
        {
          Interp( (FT_UShort)( cur_touched + 1 ),
                  end_point,
                  cur_touched,
                  first_touched,
                  &V );

          if ( first_touched > 0 )
            Interp( first_point,
                    first_touched - 1,
                    cur_touched,
                    first_touched,
                    &V );
        }
      }
      contour++;
    } while ( contour < CUR.pts.n_contours );
  }


  /*************************************************************************/
  /*                                                                       */
  /* DELTAPn[]:    DELTA exceptions P1, P2, P3                             */
  /* Opcode range: 0x5D,0x71,0x72                                          */
  /* Stack:        uint32 (2 * uint32)... -->                              */
  /*                                                                       */
  static void
  Ins_DELTAP( INS_ARG )
  {
    FT_ULong   k, nump;
    FT_UShort  A;
    FT_ULong   C;
    FT_Long    B;

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    /* Delta hinting is covered by US Patent 5159668. */
    if ( CUR.face->unpatented_hinting )
      {
      FT_Long n = args[0] * 2;
      if ( CUR.args < n )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= n;
      CUR.new_top = CUR.args;
      return;
    }
#endif

    nump = (FT_ULong)args[0];   /* some points theoretically may occur more
                                   than once, thus UShort isn't enough */

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_UShort)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      /* XXX: Because some popular fonts contain some invalid DeltaP */
      /*      instructions, we simply ignore them when the stacked   */
      /*      point reference is off limit, rather than returning an */
      /*      error.  As a delta instruction doesn't change a glyph  */
      /*      in great ways, this shouldn't be a problem.            */

      if ( !BOUNDS( A, CUR.zp0.n_points ) )
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x5D:
          break;

        case 0x71:
          C += 16;
          break;

        case 0x72:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move( &CUR.zp0, A, B );
        }
      }
      else
        if ( CUR.pedantic_hinting )
          CUR.error = TT_Err_Invalid_Reference;
    }

    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* DELTACn[]:    DELTA exceptions C1, C2, C3                             */
  /* Opcode range: 0x73,0x74,0x75                                          */
  /* Stack:        uint32 (2 * uint32)... -->                              */
  /*                                                                       */
  static void
  Ins_DELTAC( INS_ARG )
  {
    FT_ULong  nump, k;
    FT_ULong  A, C;
    FT_Long   B;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    /* Delta hinting is covered by US Patent 5159668. */
    if ( CUR.face->unpatented_hinting )
    {
      FT_Long  n = args[0] * 2;


      if ( CUR.args < n )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= n;
      CUR.new_top = CUR.args;
      return;
    }
#endif

    nump = (FT_ULong)args[0];

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_ULong)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      if ( BOUNDS( A, CUR.cvtSize ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x73:
          break;

        case 0x74:
          C += 16;
          break;

        case 0x75:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move_cvt( A, B );
        }
      }
    }

    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MISC. INSTRUCTIONS                                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* GETINFO[]:    GET INFOrmation                                         */
  /* Opcode range: 0x88                                                    */
  /* Stack:        uint32 --> uint32                                       */
  /*                                                                       */
  static void
  Ins_GETINFO( INS_ARG )
  {
    FT_Long  K;


    K = 0;

    /* We return MS rasterizer version 1.7 for the font scaler. */
    if ( ( args[0] & 1 ) != 0 )
      K = 35;

    /* Has the glyph been rotated? */
    if ( ( args[0] & 2 ) != 0 && CUR.tt_metrics.rotated )
      K |= 0x80;

    /* Has the glyph been stretched? */
    if ( ( args[0] & 4 ) != 0 && CUR.tt_metrics.stretched )
      K |= 1 << 8;

    /* Are we hinting for grayscale? */
    if ( ( args[0] & 32 ) != 0 && CUR.grayscale )
      K |= (1 << 12);

    args[0] = K;
  }


  static void
  Ins_UNKNOWN( INS_ARG )
  {
    TT_DefRecord*  def   = CUR.IDefs;
    TT_DefRecord*  limit = def + CUR.numIDefs;

    FT_UNUSED_ARG;


    for ( ; def < limit; def++ )
    {
      if ( (FT_Byte)def->opc == CUR.opcode && def->active )
      {
        TT_CallRec*  call;


        if ( CUR.callTop >= CUR.callSize )
        {
          CUR.error = TT_Err_Stack_Overflow;
          return;
        }

        call = CUR.callStack + CUR.callTop++;

        call->Caller_Range = CUR.curRange;
        call->Caller_IP    = CUR.IP+1;
        call->Cur_Count    = 1;
        call->Cur_Restart  = def->start;

        INS_Goto_CodeRange( def->range, def->start );

        CUR.step_ins = FALSE;
        return;
      }
    }

    CUR.error = TT_Err_Invalid_Opcode;
  }


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


  static
  TInstruction_Function  Instruct_Dispatch[256] =
  {
    /* Opcodes are gathered in groups of 16. */
    /* Please keep the spaces as they are.   */

    /*  SVTCA  y  */  Ins_SVTCA,
    /*  SVTCA  x  */  Ins_SVTCA,
    /*  SPvTCA y  */  Ins_SPVTCA,
    /*  SPvTCA x  */  Ins_SPVTCA,
    /*  SFvTCA y  */  Ins_SFVTCA,
    /*  SFvTCA x  */  Ins_SFVTCA,
    /*  SPvTL //  */  Ins_SPVTL,
    /*  SPvTL +   */  Ins_SPVTL,
    /*  SFvTL //  */  Ins_SFVTL,
    /*  SFvTL +   */  Ins_SFVTL,
    /*  SPvFS     */  Ins_SPVFS,
    /*  SFvFS     */  Ins_SFVFS,
    /*  GPV       */  Ins_GPV,
    /*  GFV       */  Ins_GFV,
    /*  SFvTPv    */  Ins_SFVTPV,
    /*  ISECT     */  Ins_ISECT,

    /*  SRP0      */  Ins_SRP0,
    /*  SRP1      */  Ins_SRP1,
    /*  SRP2      */  Ins_SRP2,
    /*  SZP0      */  Ins_SZP0,
    /*  SZP1      */  Ins_SZP1,
    /*  SZP2      */  Ins_SZP2,
    /*  SZPS      */  Ins_SZPS,
    /*  SLOOP     */  Ins_SLOOP,
    /*  RTG       */  Ins_RTG,
    /*  RTHG      */  Ins_RTHG,
    /*  SMD       */  Ins_SMD,
    /*  ELSE      */  Ins_ELSE,
    /*  JMPR      */  Ins_JMPR,
    /*  SCvTCi    */  Ins_SCVTCI,
    /*  SSwCi     */  Ins_SSWCI,
    /*  SSW       */  Ins_SSW,

    /*  DUP       */  Ins_DUP,
    /*  POP       */  Ins_POP,
    /*  CLEAR     */  Ins_CLEAR,
    /*  SWAP      */  Ins_SWAP,
    /*  DEPTH     */  Ins_DEPTH,
    /*  CINDEX    */  Ins_CINDEX,
    /*  MINDEX    */  Ins_MINDEX,
    /*  AlignPTS  */  Ins_ALIGNPTS,
    /*  INS_0x28  */  Ins_UNKNOWN,
    /*  UTP       */  Ins_UTP,
    /*  LOOPCALL  */  Ins_LOOPCALL,
    /*  CALL      */  Ins_CALL,
    /*  FDEF      */  Ins_FDEF,
    /*  ENDF      */  Ins_ENDF,
    /*  MDAP[0]   */  Ins_MDAP,
    /*  MDAP[1]   */  Ins_MDAP,

    /*  IUP[0]    */  Ins_IUP,
    /*  IUP[1]    */  Ins_IUP,
    /*  SHP[0]    */  Ins_SHP,
    /*  SHP[1]    */  Ins_SHP,
    /*  SHC[0]    */  Ins_SHC,
    /*  SHC[1]    */  Ins_SHC,
    /*  SHZ[0]    */  Ins_SHZ,
    /*  SHZ[1]    */  Ins_SHZ,
    /*  SHPIX     */  Ins_SHPIX,
    /*  IP        */  Ins_IP,
    /*  MSIRP[0]  */  Ins_MSIRP,
    /*  MSIRP[1]  */  Ins_MSIRP,
    /*  AlignRP   */  Ins_ALIGNRP,
    /*  RTDG      */  Ins_RTDG,
    /*  MIAP[0]   */  Ins_MIAP,
    /*  MIAP[1]   */  Ins_MIAP,

    /*  NPushB    */  Ins_NPUSHB,
    /*  NPushW    */  Ins_NPUSHW,
    /*  WS        */  Ins_WS,
    /*  RS        */  Ins_RS,
    /*  WCvtP     */  Ins_WCVTP,
    /*  RCvt      */  Ins_RCVT,
    /*  GC[0]     */  Ins_GC,
    /*  GC[1]     */  Ins_GC,
    /*  SCFS      */  Ins_SCFS,
    /*  MD[0]     */  Ins_MD,
    /*  MD[1]     */  Ins_MD,
    /*  MPPEM     */  Ins_MPPEM,
    /*  MPS       */  Ins_MPS,
    /*  FlipON    */  Ins_FLIPON,
    /*  FlipOFF   */  Ins_FLIPOFF,
    /*  DEBUG     */  Ins_DEBUG,

    /*  LT        */  Ins_LT,
    /*  LTEQ      */  Ins_LTEQ,
    /*  GT        */  Ins_GT,
    /*  GTEQ      */  Ins_GTEQ,
    /*  EQ        */  Ins_EQ,
    /*  NEQ       */  Ins_NEQ,
    /*  ODD       */  Ins_ODD,
    /*  EVEN      */  Ins_EVEN,
    /*  IF        */  Ins_IF,
    /*  EIF       */  Ins_EIF,
    /*  AND       */  Ins_AND,
    /*  OR        */  Ins_OR,
    /*  NOT       */  Ins_NOT,
    /*  DeltaP1   */  Ins_DELTAP,
    /*  SDB       */  Ins_SDB,
    /*  SDS       */  Ins_SDS,

    /*  ADD       */  Ins_ADD,
    /*  SUB       */  Ins_SUB,
    /*  DIV       */  Ins_DIV,
    /*  MUL       */  Ins_MUL,
    /*  ABS       */  Ins_ABS,
    /*  NEG       */  Ins_NEG,
    /*  FLOOR     */  Ins_FLOOR,
    /*  CEILING   */  Ins_CEILING,
    /*  ROUND[0]  */  Ins_ROUND,
    /*  ROUND[1]  */  Ins_ROUND,
    /*  ROUND[2]  */  Ins_ROUND,
    /*  ROUND[3]  */  Ins_ROUND,
    /*  NROUND[0] */  Ins_NROUND,
    /*  NROUND[1] */  Ins_NROUND,
    /*  NROUND[2] */  Ins_NROUND,
    /*  NROUND[3] */  Ins_NROUND,

    /*  WCvtF     */  Ins_WCVTF,
    /*  DeltaP2   */  Ins_DELTAP,
    /*  DeltaP3   */  Ins_DELTAP,
    /*  DeltaCn[0] */ Ins_DELTAC,
    /*  DeltaCn[1] */ Ins_DELTAC,
    /*  DeltaCn[2] */ Ins_DELTAC,
    /*  SROUND    */  Ins_SROUND,
    /*  S45Round  */  Ins_S45ROUND,
    /*  JROT      */  Ins_JROT,
    /*  JROF      */  Ins_JROF,
    /*  ROFF      */  Ins_ROFF,
    /*  INS_0x7B  */  Ins_UNKNOWN,
    /*  RUTG      */  Ins_RUTG,
    /*  RDTG      */  Ins_RDTG,
    /*  SANGW     */  Ins_SANGW,
    /*  AA        */  Ins_AA,

    /*  FlipPT    */  Ins_FLIPPT,
    /*  FlipRgON  */  Ins_FLIPRGON,
    /*  FlipRgOFF */  Ins_FLIPRGOFF,
    /*  INS_0x83  */  Ins_UNKNOWN,
    /*  INS_0x84  */  Ins_UNKNOWN,
    /*  ScanCTRL  */  Ins_SCANCTRL,
    /*  SDPVTL[0] */  Ins_SDPVTL,
    /*  SDPVTL[1] */  Ins_SDPVTL,
    /*  GetINFO   */  Ins_GETINFO,
    /*  IDEF      */  Ins_IDEF,
    /*  ROLL      */  Ins_ROLL,
    /*  MAX       */  Ins_MAX,
    /*  MIN       */  Ins_MIN,
    /*  ScanTYPE  */  Ins_SCANTYPE,
    /*  InstCTRL  */  Ins_INSTCTRL,
    /*  INS_0x8F  */  Ins_UNKNOWN,

    /*  INS_0x90  */   Ins_UNKNOWN,
    /*  INS_0x91  */   Ins_UNKNOWN,
    /*  INS_0x92  */   Ins_UNKNOWN,
    /*  INS_0x93  */   Ins_UNKNOWN,
    /*  INS_0x94  */   Ins_UNKNOWN,
    /*  INS_0x95  */   Ins_UNKNOWN,
    /*  INS_0x96  */   Ins_UNKNOWN,
    /*  INS_0x97  */   Ins_UNKNOWN,
    /*  INS_0x98  */   Ins_UNKNOWN,
    /*  INS_0x99  */   Ins_UNKNOWN,
    /*  INS_0x9A  */   Ins_UNKNOWN,
    /*  INS_0x9B  */   Ins_UNKNOWN,
    /*  INS_0x9C  */   Ins_UNKNOWN,
    /*  INS_0x9D  */   Ins_UNKNOWN,
    /*  INS_0x9E  */   Ins_UNKNOWN,
    /*  INS_0x9F  */   Ins_UNKNOWN,

    /*  INS_0xA0  */   Ins_UNKNOWN,
    /*  INS_0xA1  */   Ins_UNKNOWN,
    /*  INS_0xA2  */   Ins_UNKNOWN,
    /*  INS_0xA3  */   Ins_UNKNOWN,
    /*  INS_0xA4  */   Ins_UNKNOWN,
    /*  INS_0xA5  */   Ins_UNKNOWN,
    /*  INS_0xA6  */   Ins_UNKNOWN,
    /*  INS_0xA7  */   Ins_UNKNOWN,
    /*  INS_0xA8  */   Ins_UNKNOWN,
    /*  INS_0xA9  */   Ins_UNKNOWN,
    /*  INS_0xAA  */   Ins_UNKNOWN,
    /*  INS_0xAB  */   Ins_UNKNOWN,
    /*  INS_0xAC  */   Ins_UNKNOWN,
    /*  INS_0xAD  */   Ins_UNKNOWN,
    /*  INS_0xAE  */   Ins_UNKNOWN,
    /*  INS_0xAF  */   Ins_UNKNOWN,

    /*  PushB[0]  */  Ins_PUSHB,
    /*  PushB[1]  */  Ins_PUSHB,
    /*  PushB[2]  */  Ins_PUSHB,
    /*  PushB[3]  */  Ins_PUSHB,
    /*  PushB[4]  */  Ins_PUSHB,
    /*  PushB[5]  */  Ins_PUSHB,
    /*  PushB[6]  */  Ins_PUSHB,
    /*  PushB[7]  */  Ins_PUSHB,
    /*  PushW[0]  */  Ins_PUSHW,
    /*  PushW[1]  */  Ins_PUSHW,
    /*  PushW[2]  */  Ins_PUSHW,
    /*  PushW[3]  */  Ins_PUSHW,
    /*  PushW[4]  */  Ins_PUSHW,
    /*  PushW[5]  */  Ins_PUSHW,
    /*  PushW[6]  */  Ins_PUSHW,
    /*  PushW[7]  */  Ins_PUSHW,

    /*  MDRP[00]  */  Ins_MDRP,
    /*  MDRP[01]  */  Ins_MDRP,
    /*  MDRP[02]  */  Ins_MDRP,
    /*  MDRP[03]  */  Ins_MDRP,
    /*  MDRP[04]  */  Ins_MDRP,
    /*  MDRP[05]  */  Ins_MDRP,
    /*  MDRP[06]  */  Ins_MDRP,
    /*  MDRP[07]  */  Ins_MDRP,
    /*  MDRP[08]  */  Ins_MDRP,
    /*  MDRP[09]  */  Ins_MDRP,
    /*  MDRP[10]  */  Ins_MDRP,
    /*  MDRP[11]  */  Ins_MDRP,
    /*  MDRP[12]  */  Ins_MDRP,
    /*  MDRP[13]  */  Ins_MDRP,
    /*  MDRP[14]  */  Ins_MDRP,
    /*  MDRP[15]  */  Ins_MDRP,

    /*  MDRP[16]  */  Ins_MDRP,
    /*  MDRP[17]  */  Ins_MDRP,
    /*  MDRP[18]  */  Ins_MDRP,
    /*  MDRP[19]  */  Ins_MDRP,
    /*  MDRP[20]  */  Ins_MDRP,
    /*  MDRP[21]  */  Ins_MDRP,
    /*  MDRP[22]  */  Ins_MDRP,
    /*  MDRP[23]  */  Ins_MDRP,
    /*  MDRP[24]  */  Ins_MDRP,
    /*  MDRP[25]  */  Ins_MDRP,
    /*  MDRP[26]  */  Ins_MDRP,
    /*  MDRP[27]  */  Ins_MDRP,
    /*  MDRP[28]  */  Ins_MDRP,
    /*  MDRP[29]  */  Ins_MDRP,
    /*  MDRP[30]  */  Ins_MDRP,
    /*  MDRP[31]  */  Ins_MDRP,

    /*  MIRP[00]  */  Ins_MIRP,
    /*  MIRP[01]  */  Ins_MIRP,
    /*  MIRP[02]  */  Ins_MIRP,
    /*  MIRP[03]  */  Ins_MIRP,
    /*  MIRP[04]  */  Ins_MIRP,
    /*  MIRP[05]  */  Ins_MIRP,
    /*  MIRP[06]  */  Ins_MIRP,
    /*  MIRP[07]  */  Ins_MIRP,
    /*  MIRP[08]  */  Ins_MIRP,
    /*  MIRP[09]  */  Ins_MIRP,
    /*  MIRP[10]  */  Ins_MIRP,
    /*  MIRP[11]  */  Ins_MIRP,
    /*  MIRP[12]  */  Ins_MIRP,
    /*  MIRP[13]  */  Ins_MIRP,
    /*  MIRP[14]  */  Ins_MIRP,
    /*  MIRP[15]  */  Ins_MIRP,

    /*  MIRP[16]  */  Ins_MIRP,
    /*  MIRP[17]  */  Ins_MIRP,
    /*  MIRP[18]  */  Ins_MIRP,
    /*  MIRP[19]  */  Ins_MIRP,
    /*  MIRP[20]  */  Ins_MIRP,
    /*  MIRP[21]  */  Ins_MIRP,
    /*  MIRP[22]  */  Ins_MIRP,
    /*  MIRP[23]  */  Ins_MIRP,
    /*  MIRP[24]  */  Ins_MIRP,
    /*  MIRP[25]  */  Ins_MIRP,
    /*  MIRP[26]  */  Ins_MIRP,
    /*  MIRP[27]  */  Ins_MIRP,
    /*  MIRP[28]  */  Ins_MIRP,
    /*  MIRP[29]  */  Ins_MIRP,
    /*  MIRP[30]  */  Ins_MIRP,
    /*  MIRP[31]  */  Ins_MIRP
  };


#endif /* !TT_CONFIG_OPTION_INTERPRETER_SWITCH */


  /*************************************************************************/
  /*                                                                       */
  /* RUN                                                                   */
  /*                                                                       */
  /*  This function executes a run of opcodes.  It will exit in the        */
  /*  following cases:                                                     */
  /*                                                                       */
  /*  - Errors (in which case it returns FALSE).                           */
  /*                                                                       */
  /*  - Reaching the end of the main code range (returns TRUE).            */
  /*    Reaching the end of a code range within a function call is an      */
  /*    error.                                                             */
  /*                                                                       */
  /*  - After executing one single opcode, if the flag `Instruction_Trap'  */
  /*    is set to TRUE (returns TRUE).                                     */
  /*                                                                       */
  /*  On exit whith TRUE, test IP < CodeSize to know wether it comes from  */
  /*  an instruction trap or a normal termination.                         */
  /*                                                                       */
  /*                                                                       */
  /*  Note: The documented DEBUG opcode pops a value from the stack.  This */
  /*        behaviour is unsupported; here a DEBUG opcode is always an     */
  /*        error.                                                         */
  /*                                                                       */
  /*                                                                       */
  /* THIS IS THE INTERPRETER'S MAIN LOOP.                                  */
  /*                                                                       */
  /*  Instructions appear in the specification's order.                    */
  /*                                                                       */
  /*************************************************************************/


  /* documentation is in ttinterp.h */

  FT_EXPORT_DEF( FT_Error )
  TT_RunIns( TT_ExecContext  exc )
  {
    FT_Long  ins_counter = 0;  /* executed instructions counter */


#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    cur = *exc;
#endif

    /* set CVT functions */
    CUR.tt_metrics.ratio = 0;
    if ( CUR.metrics.x_ppem != CUR.metrics.y_ppem )
    {
      /* non-square pixels, use the stretched routines */
      CUR.func_read_cvt  = Read_CVT_Stretched;
      CUR.func_write_cvt = Write_CVT_Stretched;
      CUR.func_move_cvt  = Move_CVT_Stretched;
    }
    else
    {
      /* square pixels, use normal routines */
      CUR.func_read_cvt  = Read_CVT;
      CUR.func_write_cvt = Write_CVT;
      CUR.func_move_cvt  = Move_CVT;
    }

    COMPUTE_Funcs();
    COMPUTE_Round( (FT_Byte)exc->GS.round_state );

    do
    {
      CUR.opcode = CUR.code[CUR.IP];

      if ( ( CUR.length = opcode_length[CUR.opcode] ) < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto LErrorCodeOverflow_;

        CUR.length = 2 - CUR.length * CUR.code[CUR.IP + 1];
      }

      if ( CUR.IP + CUR.length > CUR.codeSize )
        goto LErrorCodeOverflow_;

      /* First, let's check for empty stack and overflow */
      CUR.args = CUR.top - ( Pop_Push_Count[CUR.opcode] >> 4 );

      /* `args' is the top of the stack once arguments have been popped. */
      /* One can also interpret it as the index of the last argument.    */
      if ( CUR.args < 0 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        goto LErrorLabel_;
      }

      CUR.new_top = CUR.args + ( Pop_Push_Count[CUR.opcode] & 15 );

      /* `new_top' is the new top of the stack, after the instruction's */
      /* execution.  `top' will be set to `new_top' after the `switch'  */
      /* statement.                                                     */
      if ( CUR.new_top > CUR.stackSize )
      {
        CUR.error = TT_Err_Stack_Overflow;
        goto LErrorLabel_;
      }

      CUR.step_ins = TRUE;
      CUR.error    = TT_Err_Ok;

#ifdef TT_CONFIG_OPTION_INTERPRETER_SWITCH

      {
        FT_Long*  args   = CUR.stack + CUR.args;
        FT_Byte   opcode = CUR.opcode;


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR  goto Set_Invalid_Ref


        switch ( opcode )
        {
        case 0x00:  /* SVTCA y  */
        case 0x01:  /* SVTCA x  */
        case 0x02:  /* SPvTCA y */
        case 0x03:  /* SPvTCA x */
        case 0x04:  /* SFvTCA y */
        case 0x05:  /* SFvTCA x */
          {
            FT_Short AA, BB;


            AA = (FT_Short)( ( opcode & 1 ) << 14 );
            BB = (FT_Short)( AA ^ 0x4000 );

            if ( opcode < 4 )
            {
              CUR.GS.projVector.x = AA;
              CUR.GS.projVector.y = BB;

              CUR.GS.dualVector.x = AA;
              CUR.GS.dualVector.y = BB;
            }
            else
            {
              GUESS_VECTOR( projVector );
            }

            if ( ( opcode & 2 ) == 0 )
            {
              CUR.GS.freeVector.x = AA;
              CUR.GS.freeVector.y = BB;
            }
            else
            {
              GUESS_VECTOR( freeVector );
            }

            COMPUTE_Funcs();
          }
          break;

        case 0x06:  /* SPvTL // */
        case 0x07:  /* SPvTL +  */
          DO_SPVTL
          break;

        case 0x08:  /* SFvTL // */
        case 0x09:  /* SFvTL +  */
          DO_SFVTL
          break;

        case 0x0A:  /* SPvFS */
          DO_SPVFS
          break;

        case 0x0B:  /* SFvFS */
          DO_SFVFS
          break;

        case 0x0C:  /* GPV */
          DO_GPV
          break;

        case 0x0D:  /* GFV */
          DO_GFV
          break;

        case 0x0E:  /* SFvTPv */
          DO_SFVTPV
          break;

        case 0x0F:  /* ISECT  */
          Ins_ISECT( EXEC_ARG_ args );
          break;

        case 0x10:  /* SRP0 */
          DO_SRP0
          break;

        case 0x11:  /* SRP1 */
          DO_SRP1
          break;

        case 0x12:  /* SRP2 */
          DO_SRP2
          break;

        case 0x13:  /* SZP0 */
          Ins_SZP0( EXEC_ARG_ args );
          break;

        case 0x14:  /* SZP1 */
          Ins_SZP1( EXEC_ARG_ args );
          break;

        case 0x15:  /* SZP2 */
          Ins_SZP2( EXEC_ARG_ args );
          break;

        case 0x16:  /* SZPS */
          Ins_SZPS( EXEC_ARG_ args );
          break;

        case 0x17:  /* SLOOP */
          DO_SLOOP
          break;

        case 0x18:  /* RTG */
          DO_RTG
          break;

        case 0x19:  /* RTHG */
          DO_RTHG
          break;

        case 0x1A:  /* SMD */
          DO_SMD
          break;

        case 0x1B:  /* ELSE */
          Ins_ELSE( EXEC_ARG_ args );
          break;

        case 0x1C:  /* JMPR */
          DO_JMPR
          break;

        case 0x1D:  /* SCVTCI */
          DO_SCVTCI
          break;

        case 0x1E:  /* SSWCI */
          DO_SSWCI
          break;

        case 0x1F:  /* SSW */
          DO_SSW
          break;

        case 0x20:  /* DUP */
          DO_DUP
          break;

        case 0x21:  /* POP */
          /* nothing :-) */
          break;

        case 0x22:  /* CLEAR */
          DO_CLEAR
          break;

        case 0x23:  /* SWAP */
          DO_SWAP
          break;

        case 0x24:  /* DEPTH */
          DO_DEPTH
          break;

        case 0x25:  /* CINDEX */
          DO_CINDEX
          break;

        case 0x26:  /* MINDEX */
          Ins_MINDEX( EXEC_ARG_ args );
          break;

        case 0x27:  /* ALIGNPTS */
          Ins_ALIGNPTS( EXEC_ARG_ args );
          break;

        case 0x28:  /* ???? */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x29:  /* UTP */
          Ins_UTP( EXEC_ARG_ args );
          break;

        case 0x2A:  /* LOOPCALL */
          Ins_LOOPCALL( EXEC_ARG_ args );
          break;

        case 0x2B:  /* CALL */
          Ins_CALL( EXEC_ARG_ args );
          break;

        case 0x2C:  /* FDEF */
          Ins_FDEF( EXEC_ARG_ args );
          break;

        case 0x2D:  /* ENDF */
          Ins_ENDF( EXEC_ARG_ args );
          break;

        case 0x2E:  /* MDAP */
        case 0x2F:  /* MDAP */
          Ins_MDAP( EXEC_ARG_ args );
          break;


        case 0x30:  /* IUP */
        case 0x31:  /* IUP */
          Ins_IUP( EXEC_ARG_ args );
          break;

        case 0x32:  /* SHP */
        case 0x33:  /* SHP */
          Ins_SHP( EXEC_ARG_ args );
          break;

        case 0x34:  /* SHC */
        case 0x35:  /* SHC */
          Ins_SHC( EXEC_ARG_ args );
          break;

        case 0x36:  /* SHZ */
        case 0x37:  /* SHZ */
          Ins_SHZ( EXEC_ARG_ args );
          break;

        case 0x38:  /* SHPIX */
          Ins_SHPIX( EXEC_ARG_ args );
          break;

        case 0x39:  /* IP    */
          Ins_IP( EXEC_ARG_ args );
          break;

        case 0x3A:  /* MSIRP */
        case 0x3B:  /* MSIRP */
          Ins_MSIRP( EXEC_ARG_ args );
          break;

        case 0x3C:  /* AlignRP */
          Ins_ALIGNRP( EXEC_ARG_ args );
          break;

        case 0x3D:  /* RTDG */
          DO_RTDG
          break;

        case 0x3E:  /* MIAP */
        case 0x3F:  /* MIAP */
          Ins_MIAP( EXEC_ARG_ args );
          break;

        case 0x40:  /* NPUSHB */
          Ins_NPUSHB( EXEC_ARG_ args );
          break;

        case 0x41:  /* NPUSHW */
          Ins_NPUSHW( EXEC_ARG_ args );
          break;

        case 0x42:  /* WS */
          DO_WS
          break;

      Set_Invalid_Ref:
            CUR.error = TT_Err_Invalid_Reference;
          break;

        case 0x43:  /* RS */
          DO_RS
          break;

        case 0x44:  /* WCVTP */
          DO_WCVTP
          break;

        case 0x45:  /* RCVT */
          DO_RCVT
          break;

        case 0x46:  /* GC */
        case 0x47:  /* GC */
          Ins_GC( EXEC_ARG_ args );
          break;

        case 0x48:  /* SCFS */
          Ins_SCFS( EXEC_ARG_ args );
          break;

        case 0x49:  /* MD */
        case 0x4A:  /* MD */
          Ins_MD( EXEC_ARG_ args );
          break;

        case 0x4B:  /* MPPEM */
          DO_MPPEM
          break;

        case 0x4C:  /* MPS */
          DO_MPS
          break;

        case 0x4D:  /* FLIPON */
          DO_FLIPON
          break;

        case 0x4E:  /* FLIPOFF */
          DO_FLIPOFF
          break;

        case 0x4F:  /* DEBUG */
          DO_DEBUG
          break;

        case 0x50:  /* LT */
          DO_LT
          break;

        case 0x51:  /* LTEQ */
          DO_LTEQ
          break;

        case 0x52:  /* GT */
          DO_GT
          break;

        case 0x53:  /* GTEQ */
          DO_GTEQ
          break;

        case 0x54:  /* EQ */
          DO_EQ
          break;

        case 0x55:  /* NEQ */
          DO_NEQ
          break;

        case 0x56:  /* ODD */
          DO_ODD
          break;

        case 0x57:  /* EVEN */
          DO_EVEN
          break;

        case 0x58:  /* IF */
          Ins_IF( EXEC_ARG_ args );
          break;

        case 0x59:  /* EIF */
          /* do nothing */
          break;

        case 0x5A:  /* AND */
          DO_AND
          break;

        case 0x5B:  /* OR */
          DO_OR
          break;

        case 0x5C:  /* NOT */
          DO_NOT
          break;

        case 0x5D:  /* DELTAP1 */
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x5E:  /* SDB */
          DO_SDB
          break;

        case 0x5F:  /* SDS */
          DO_SDS
          break;

        case 0x60:  /* ADD */
          DO_ADD
          break;

        case 0x61:  /* SUB */
          DO_SUB
          break;

        case 0x62:  /* DIV */
          DO_DIV
          break;

        case 0x63:  /* MUL */
          DO_MUL
          break;

        case 0x64:  /* ABS */
          DO_ABS
          break;

        case 0x65:  /* NEG */
          DO_NEG
          break;

        case 0x66:  /* FLOOR */
          DO_FLOOR
          break;

        case 0x67:  /* CEILING */
          DO_CEILING
          break;

        case 0x68:  /* ROUND */
        case 0x69:  /* ROUND */
        case 0x6A:  /* ROUND */
        case 0x6B:  /* ROUND */
          DO_ROUND
          break;

        case 0x6C:  /* NROUND */
        case 0x6D:  /* NROUND */
        case 0x6E:  /* NRRUND */
        case 0x6F:  /* NROUND */
          DO_NROUND
          break;

        case 0x70:  /* WCVTF */
          DO_WCVTF
          break;

        case 0x71:  /* DELTAP2 */
        case 0x72:  /* DELTAP3 */
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x73:  /* DELTAC0 */
        case 0x74:  /* DELTAC1 */
        case 0x75:  /* DELTAC2 */
          Ins_DELTAC( EXEC_ARG_ args );
          break;

        case 0x76:  /* SROUND */
          DO_SROUND
          break;

        case 0x77:  /* S45Round */
          DO_S45ROUND
          break;

        case 0x78:  /* JROT */
          DO_JROT
          break;

        case 0x79:  /* JROF */
          DO_JROF
          break;

        case 0x7A:  /* ROFF */
          DO_ROFF
          break;

        case 0x7B:  /* ???? */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x7C:  /* RUTG */
          DO_RUTG
          break;

        case 0x7D:  /* RDTG */
          DO_RDTG
          break;

        case 0x7E:  /* SANGW */
        case 0x7F:  /* AA    */
          /* nothing - obsolete */
          break;

        case 0x80:  /* FLIPPT */
          Ins_FLIPPT( EXEC_ARG_ args );
          break;

        case 0x81:  /* FLIPRGON */
          Ins_FLIPRGON( EXEC_ARG_ args );
          break;

        case 0x82:  /* FLIPRGOFF */
          Ins_FLIPRGOFF( EXEC_ARG_ args );
          break;

        case 0x83:  /* UNKNOWN */
        case 0x84:  /* UNKNOWN */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x85:  /* SCANCTRL */
          Ins_SCANCTRL( EXEC_ARG_ args );
          break;

        case 0x86:  /* SDPVTL */
        case 0x87:  /* SDPVTL */
          Ins_SDPVTL( EXEC_ARG_ args );
          break;

        case 0x88:  /* GETINFO */
          Ins_GETINFO( EXEC_ARG_ args );
          break;

        case 0x89:  /* IDEF */
          Ins_IDEF( EXEC_ARG_ args );
          break;

        case 0x8A:  /* ROLL */
          Ins_ROLL( EXEC_ARG_ args );
          break;

        case 0x8B:  /* MAX */
          DO_MAX
          break;

        case 0x8C:  /* MIN */
          DO_MIN
          break;

        case 0x8D:  /* SCANTYPE */
          Ins_SCANTYPE( EXEC_ARG_ args );
          break;

        case 0x8E:  /* INSTCTRL */
          Ins_INSTCTRL( EXEC_ARG_ args );
          break;

        case 0x8F:
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        default:
          if ( opcode >= 0xE0 )
            Ins_MIRP( EXEC_ARG_ args );
          else if ( opcode >= 0xC0 )
            Ins_MDRP( EXEC_ARG_ args );
          else if ( opcode >= 0xB8 )
            Ins_PUSHW( EXEC_ARG_ args );
          else if ( opcode >= 0xB0 )
            Ins_PUSHB( EXEC_ARG_ args );
          else
            Ins_UNKNOWN( EXEC_ARG_ args );
        }

      }

#else

      Instruct_Dispatch[CUR.opcode]( EXEC_ARG_ &CUR.stack[CUR.args] );

#endif /* TT_CONFIG_OPTION_INTERPRETER_SWITCH */

      if ( CUR.error != TT_Err_Ok )
      {
        switch ( CUR.error )
        {
        case TT_Err_Invalid_Opcode: /* looking for redefined instructions */
          {
            TT_DefRecord*  def   = CUR.IDefs;
            TT_DefRecord*  limit = def + CUR.numIDefs;


            for ( ; def < limit; def++ )
            {
              if ( def->active && CUR.opcode == (FT_Byte)def->opc )
              {
                TT_CallRec*  callrec;


                if ( CUR.callTop >= CUR.callSize )
                {
                  CUR.error = TT_Err_Invalid_Reference;
                  goto LErrorLabel_;
                }

                callrec = &CUR.callStack[CUR.callTop];

                callrec->Caller_Range = CUR.curRange;
                callrec->Caller_IP    = CUR.IP + 1;
                callrec->Cur_Count    = 1;
                callrec->Cur_Restart  = def->start;

                if ( INS_Goto_CodeRange( def->range, def->start ) == FAILURE )
                  goto LErrorLabel_;

                goto LSuiteLabel_;
              }
            }
          }

          CUR.error = TT_Err_Invalid_Opcode;
          goto LErrorLabel_;

#if 0
          break;   /* Unreachable code warning suppression.             */
                   /* Leave to remind in case a later change the editor */
                   /* to consider break;                                */
#endif

        default:
          goto LErrorLabel_;

#if 0
        break;
#endif
        }
      }

      CUR.top = CUR.new_top;

      if ( CUR.step_ins )
        CUR.IP += CUR.length;

      /* increment instruction counter and check if we didn't */
      /* run this program for too long (e.g. infinite loops). */
      if ( ++ins_counter > MAX_RUNNABLE_OPCODES )
        return TT_Err_Execution_Too_Long;

    LSuiteLabel_:
      if ( CUR.IP >= CUR.codeSize )
      {
        if ( CUR.callTop > 0 )
        {
          CUR.error = TT_Err_Code_Overflow;
          goto LErrorLabel_;
        }
        else
          goto LNo_Error_;
      }
    } while ( !CUR.instruction_trap );

  LNo_Error_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return TT_Err_Ok;

  LErrorCodeOverflow_:
    CUR.error = TT_Err_Code_Overflow;

  LErrorLabel_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return CUR.error;
  }


#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


/* END */

#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
/***************************************************************************/
/*                                                                         */
/*  ttgxvar.c                                                              */
/*                                                                         */
/*    TrueType GX Font Variation loader                                    */
/*                                                                         */
/*  Copyright 2004, 2005, 2006 by                                          */
/*  David Turner, Robert Wilhelm, Werner Lemberg, and George Williams.     */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


/***************************************************************************/
/*                                                                         */
/* Apple documents the `fvar', `gvar', `cvar', and `avar' tables at        */
/*                                                                         */
/*   http://developer.apple.com/fonts/TTRefMan/RM06/Chap6[fgca]var.html    */
/*                                                                         */
/* The documentation for `fvar' is inconsistant.  At one point it says     */
/* that `countSizePairs' should be 3, at another point 2.  It should be 2. */
/*                                                                         */
/* The documentation for `gvar' is not intelligible; `cvar' refers you to  */
/* `gvar' and is thus also incomprehensible.                               */
/*                                                                         */
/* The documentation for `avar' appears correct, but Apple has no fonts    */
/* with an `avar' table, so it is hard to test.                            */
/*                                                                         */
/* Many thanks to John Jenkins (at Apple) in figuring this out.            */
/*                                                                         */
/*                                                                         */
/* Apple's `kern' table has some references to tuple indices, but as there */
/* is no indication where these indices are defined, nor how to            */
/* interpolate the kerning values (different tuples have different         */
/* classes) this issue is ignored.                                         */
/*                                                                         */
/***************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_DEBUG_H
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_MULTIPLE_MASTERS_H

#include "ttdriver.h"
#include "ttpload.h"
#include "ttgxvar.h"

#include "tterrors.h"


#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT


#define FT_Stream_FTell( stream )  \
          ( (stream)->cursor - (stream)->base )
#define FT_Stream_SeekSet( stream, off ) \
              ( (stream)->cursor = (stream)->base+(off) )


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttgxvar


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       Internal Routines                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The macro ALL_POINTS is used in `ft_var_readpackedpoints'.  It        */
  /* indicates that there is a delta for every point without needing to    */
  /* enumerate all of them.                                                */
  /*                                                                       */
#define ALL_POINTS  (FT_UShort*)( -1 )


  enum
  {
    GX_PT_POINTS_ARE_WORDS     = 0x80,
    GX_PT_POINT_RUN_COUNT_MASK = 0x7F
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_var_readpackedpoints                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Read a set of points to which the following deltas will apply.     */
  /*    Points are packed with a run length encoding.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream    :: The data stream.                                      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    point_cnt :: The number of points read.  A zero value means that   */
  /*                 all points in the glyph will be affected, without     */
  /*                 enumerating them individually.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    An array of FT_UShort containing the affected points or the        */
  /*    special value ALL_POINTS.                                          */
  /*                                                                       */
  static FT_UShort*
  ft_var_readpackedpoints( FT_Stream  stream,
                           FT_UInt   *point_cnt )
  {
    FT_UShort *points;
    FT_Int     n;
    FT_Int     runcnt;
    FT_Int     i;
    FT_Int     j;
    FT_Int     first;
    FT_Memory  memory = stream->memory;
    FT_Error   error = TT_Err_Ok;

    FT_UNUSED( error );


    *point_cnt = n = FT_GET_BYTE();
    if ( n == 0 )
      return ALL_POINTS;

    if ( n & GX_PT_POINTS_ARE_WORDS )
      n = FT_GET_BYTE() | ( ( n & GX_PT_POINT_RUN_COUNT_MASK ) << 8 );

    if ( FT_NEW_ARRAY( points, n ) )
      return NULL;

    i = 0;
    while ( i < n )
    {
      runcnt = FT_GET_BYTE();
      if ( runcnt & GX_PT_POINTS_ARE_WORDS )
      {
        runcnt = runcnt & GX_PT_POINT_RUN_COUNT_MASK;
        first  = points[i++] = FT_GET_USHORT();

        /* first point not included in runcount */
        for ( j = 0; j < runcnt; ++j )
          points[i++] = (FT_UShort)( first += FT_GET_USHORT() );
      }
      else
      {
        first = points[i++] = FT_GET_BYTE();

        for ( j = 0; j < runcnt; ++j )
          points[i++] = (FT_UShort)( first += FT_GET_BYTE() );
      }
    }

    return points;
  }


  enum
  {
    GX_DT_DELTAS_ARE_ZERO      = 0x80,
    GX_DT_DELTAS_ARE_WORDS     = 0x40,
    GX_DT_DELTA_RUN_COUNT_MASK = 0x3F
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_var_readpackeddeltas                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Read a set of deltas.  These are packed slightly differently than  */
  /*    points.  In particular there is no overall count.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream    :: The data stream.                                      */
  /*                                                                       */
  /*    delta_cnt :: The number of to be read.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    An array of FT_Short containing the deltas for the affected        */
  /*    points.  (This only gets the deltas for one dimension.  It will    */
  /*    generally be called twice, once for x, once for y.  When used in   */
  /*    cvt table, it will only be called once.)                           */
  /*                                                                       */
  static FT_Short*
  ft_var_readpackeddeltas( FT_Stream  stream,
                           FT_Int     delta_cnt )
  {
    FT_Short  *deltas;
    FT_Int     runcnt;
    FT_Int     i;
    FT_Int     j;
    FT_Memory  memory = stream->memory;
    FT_Error   error = TT_Err_Ok;

    FT_UNUSED( error );


    if ( FT_NEW_ARRAY( deltas, delta_cnt ) )
      return NULL;

    i = 0;
    while ( i < delta_cnt )
    {
      runcnt = FT_GET_BYTE();
      if ( runcnt & GX_DT_DELTAS_ARE_ZERO )
      {
        /* runcnt zeroes get added */
        for ( j = 0;
              j <= ( runcnt & GX_DT_DELTA_RUN_COUNT_MASK ) && i < delta_cnt;
              ++j )
          deltas[i++] = 0;
      }
      else if ( runcnt & GX_DT_DELTAS_ARE_WORDS )
      {
        /* runcnt shorts from the stack */
        for ( j = 0;
              j <= ( runcnt & GX_DT_DELTA_RUN_COUNT_MASK ) && i < delta_cnt;
              ++j )
          deltas[i++] = FT_GET_SHORT();
      }
      else
      {
        /* runcnt signed bytes from the stack */
        for ( j = 0;
              j <= ( runcnt & GX_DT_DELTA_RUN_COUNT_MASK ) && i < delta_cnt;
              ++j )
          deltas[i++] = FT_GET_CHAR();
      }

      if ( j <= ( runcnt & GX_DT_DELTA_RUN_COUNT_MASK ) )
      {
        /* Bad format */
        FT_FREE( deltas );
        return NULL;
      }
    }

    return deltas;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_var_load_avar                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parse the `avar' table if present.  It need not be, so we return   */
  /*    nothing.                                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face :: The font face.                                             */
  /*                                                                       */
  static void
  ft_var_load_avar( TT_Face  face )
  {
    FT_Stream       stream = FT_FACE_STREAM(face);
    FT_Memory       memory = stream->memory;
    GX_Blend        blend  = face->blend;
    GX_AVarSegment  segment;
    FT_Error        error = TT_Err_Ok;
    FT_ULong        version;
    FT_Long         axisCount;
    FT_Int          i, j;
    FT_ULong        table_len;

    FT_UNUSED( error );


    blend->avar_checked = TRUE;
    if ( (error = face->goto_table( face, TTAG_avar, stream, &table_len )) != 0 )
      return;

    if ( FT_FRAME_ENTER( table_len ) )
      return;

    version   = FT_GET_LONG();
    axisCount = FT_GET_LONG();

    if ( version != 0x00010000L                       ||
         axisCount != (FT_Long)blend->mmvar->num_axis )
      goto Exit;

    if ( FT_NEW_ARRAY( blend->avar_segment, axisCount ) )
      goto Exit;

    segment = &blend->avar_segment[0];
    for ( i = 0; i < axisCount; ++i, ++segment )
    {
      segment->pairCount = FT_GET_USHORT();
      if ( FT_NEW_ARRAY( segment->correspondence, segment->pairCount ) )
      {
        /* Failure.  Free everything we have done so far.  We must do */
        /* it right now since loading the `avar' table is optional.   */

        for ( j = i - 1; j >= 0; --j )
          FT_FREE( blend->avar_segment[j].correspondence );

        FT_FREE( blend->avar_segment );
        blend->avar_segment = NULL;
        goto Exit;
      }

      for ( j = 0; j < segment->pairCount; ++j )
      {
        segment->correspondence[j].fromCoord =
          FT_GET_SHORT() << 2;    /* convert to Fixed */
        segment->correspondence[j].toCoord =
          FT_GET_SHORT()<<2;    /* convert to Fixed */
      }
    }

  Exit:
    FT_FRAME_EXIT();
  }


  typedef struct  GX_GVar_Head_ {
    FT_Long    version;
    FT_UShort  axisCount;
    FT_UShort  globalCoordCount;
    FT_ULong   offsetToCoord;
    FT_UShort  glyphCount;
    FT_UShort  flags;
    FT_ULong   offsetToData;

  } GX_GVar_Head;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_var_load_gvar                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses the `gvar' table if present.  If `fvar' is there, `gvar'    */
  /*    had better be there too.                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face :: The font face.                                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  ft_var_load_gvar( TT_Face  face )
  {
    FT_Stream     stream = FT_FACE_STREAM(face);
    FT_Memory     memory = stream->memory;
    GX_Blend      blend  = face->blend;
    FT_Error      error;
    FT_UInt       i, j;
    FT_ULong      table_len;
    FT_ULong      gvar_start;
    FT_ULong      offsetToData;
    GX_GVar_Head  gvar_head;

    static const FT_Frame_Field  gvar_fields[] =
    {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  GX_GVar_Head

      FT_FRAME_START( 20 ),
        FT_FRAME_LONG  ( version ),
        FT_FRAME_USHORT( axisCount ),
        FT_FRAME_USHORT( globalCoordCount ),
        FT_FRAME_ULONG ( offsetToCoord ),
        FT_FRAME_USHORT( glyphCount ),
        FT_FRAME_USHORT( flags ),
        FT_FRAME_ULONG ( offsetToData ),
      FT_FRAME_END
    };

    if ( (error = face->goto_table( face, TTAG_gvar, stream, &table_len )) != 0 )
      goto Exit;

    gvar_start = FT_STREAM_POS( );
    if ( FT_STREAM_READ_FIELDS( gvar_fields, &gvar_head ) )
      goto Exit;

    blend->tuplecount  = gvar_head.globalCoordCount;
    blend->gv_glyphcnt = gvar_head.glyphCount;
    offsetToData       = gvar_start + gvar_head.offsetToData;

    if ( gvar_head.version   != (FT_Long)0x00010000L              ||
         gvar_head.axisCount != (FT_UShort)blend->mmvar->num_axis )
    {
      error = TT_Err_Invalid_Table;
      goto Exit;
    }

    if ( FT_NEW_ARRAY( blend->glyphoffsets, blend->gv_glyphcnt + 1 ) )
      goto Exit;

    if ( gvar_head.flags & 1 )
    {
      /* long offsets (one more offset than glyphs, to mark size of last) */
      if ( FT_FRAME_ENTER( ( blend->gv_glyphcnt + 1 ) * 4L ) )
        goto Exit;

      for ( i = 0; i <= blend->gv_glyphcnt; ++i )
        blend->glyphoffsets[i] = offsetToData + FT_GET_LONG();

      FT_FRAME_EXIT();
    }
    else
    {
      /* short offsets (one more offset than glyphs, to mark size of last) */
      if ( FT_FRAME_ENTER( ( blend->gv_glyphcnt + 1 ) * 2L ) )
        goto Exit;

      for ( i = 0; i <= blend->gv_glyphcnt; ++i )
        blend->glyphoffsets[i] = offsetToData + FT_GET_USHORT() * 2;
                                              /* XXX: Undocumented: `*2'! */

      FT_FRAME_EXIT();
    }

    if ( blend->tuplecount != 0 )
    {
      if ( FT_NEW_ARRAY( blend->tuplecoords,
                         gvar_head.axisCount * blend->tuplecount ) )
        goto Exit;

      if ( FT_STREAM_SEEK( gvar_start + gvar_head.offsetToCoord )       ||
           FT_FRAME_ENTER( blend->tuplecount * gvar_head.axisCount * 2L )                   )
        goto Exit;

      for ( i = 0; i < blend->tuplecount; ++i )
        for ( j = 0 ; j < (FT_UInt)gvar_head.axisCount; ++j )
          blend->tuplecoords[i * gvar_head.axisCount + j] =
            FT_GET_SHORT() << 2;                /* convert to FT_Fixed */

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_var_apply_tuple                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Figure out whether a given tuple (design) applies to the current   */
  /*    blend, and if so, what is the scaling factor.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    blend           :: The current blend of the font.                  */
  /*                                                                       */
  /*    tupleIndex      :: A flag saying whether this is an intermediate   */
  /*                       tuple or not.                                   */
  /*                                                                       */
  /*    tuple_coords    :: The coordinates of the tuple in normalized axis */
  /*                       units.                                          */
  /*                                                                       */
  /*    im_start_coords :: The initial coordinates where this tuple starts */
  /*                       to apply (for intermediate coordinates).        */
  /*                                                                       */
  /*    im_end_coords   :: The final coordinates after which this tuple no */
  /*                       longer applies (for intermediate coordinates).  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    An FT_Fixed value containing the scaling factor.                   */
  /*                                                                       */
  static FT_Fixed
  ft_var_apply_tuple( GX_Blend   blend,
                      FT_UShort  tupleIndex,
                      FT_Fixed*  tuple_coords,
                      FT_Fixed*  im_start_coords,
                      FT_Fixed*  im_end_coords )
  {
    FT_UInt   i;
    FT_Fixed  apply;
    FT_Fixed  temp;


    apply = 0x10000L;
    for ( i = 0; i < blend->num_axis; ++i )
    {
      if ( tuple_coords[i] == 0 )
        /* It's not clear why (for intermediate tuples) we don't need     */
        /* to check against start/end -- the documentation says we don't. */
        /* Similarly, it's unclear why we don't need to scale along the   */
        /* axis.                                                          */
        continue;

      else if ( blend->normalizedcoords[i] == 0                           ||
                ( blend->normalizedcoords[i] < 0 && tuple_coords[i] > 0 ) ||
                ( blend->normalizedcoords[i] > 0 && tuple_coords[i] < 0 ) )
      {
        apply = 0;
        break;
      }

      else if ( !( tupleIndex & GX_TI_INTERMEDIATE_TUPLE ) )
        /* not an intermediate tuple */
        apply = FT_MulDiv( apply,
                           blend->normalizedcoords[i] > 0
                             ? blend->normalizedcoords[i]
                             : -blend->normalizedcoords[i],
                           0x10000L );

      else if ( blend->normalizedcoords[i] <= im_start_coords[i] ||
                blend->normalizedcoords[i] >= im_end_coords[i]   )
      {
        apply = 0;
        break;
      }

      else if ( blend->normalizedcoords[i] < tuple_coords[i] )
      {
        temp = FT_MulDiv( blend->normalizedcoords[i] - im_start_coords[i],
                          0x10000L,
                          tuple_coords[i] - im_start_coords[i]);
        apply = FT_MulDiv( apply, temp, 0x10000L );
      }

      else
      {
        temp = FT_MulDiv( im_end_coords[i] - blend->normalizedcoords[i],
                          0x10000L,
                          im_end_coords[i] - tuple_coords[i] );
        apply = FT_MulDiv( apply, temp, 0x10000L );
      }
    }

    return apply;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               MULTIPLE MASTERS SERVICE FUNCTIONS              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  typedef struct  GX_FVar_Head_ {
    FT_Long    version;
    FT_UShort  offsetToData;
    FT_UShort  countSizePairs;
    FT_UShort  axisCount;
    FT_UShort  axisSize;
    FT_UShort  instanceCount;
    FT_UShort  instanceSize;

  } GX_FVar_Head;


  typedef struct  fvar_axis {
    FT_ULong   axisTag;
    FT_ULong   minValue;
    FT_ULong   defaultValue;
    FT_ULong   maxValue;
    FT_UShort  flags;
    FT_UShort  nameID;

  } GX_FVar_Axis;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Get_MM_Var                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Check that the font's `fvar' table is valid, parse it, and return  */
  /*    those data.                                                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: The font face.                                           */
  /*              TT_Get_MM_Var initializes the blend structure.           */
  /*                                                                       */
  /* <Output>                                                              */
  /*    master :: The `fvar' data (must be freed by caller).               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Get_MM_Var( TT_Face      face,
                 FT_MM_Var*  *master )
  {
    FT_Stream            stream = face->root.stream;
    FT_Memory            memory = face->root.memory;
    FT_ULong             table_len;
    FT_Error             error  = TT_Err_Ok;
    FT_ULong             fvar_start;
    FT_Int               i, j;
    FT_MM_Var*           mmvar;
    FT_Fixed*            next_coords;
    FT_String*           next_name;
    FT_Var_Axis*         a;
    FT_Var_Named_Style*  ns;
    GX_FVar_Head         fvar_head;

    static const FT_Frame_Field  fvar_fields[] =
    {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  GX_FVar_Head

      FT_FRAME_START( 16 ),
        FT_FRAME_LONG  ( version ),
        FT_FRAME_USHORT( offsetToData ),
        FT_FRAME_USHORT( countSizePairs ),
        FT_FRAME_USHORT( axisCount ),
        FT_FRAME_USHORT( axisSize ),
        FT_FRAME_USHORT( instanceCount ),
        FT_FRAME_USHORT( instanceSize ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  fvaraxis_fields[] =
    {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  GX_FVar_Axis

      FT_FRAME_START( 20 ),
        FT_FRAME_ULONG ( axisTag ),
        FT_FRAME_ULONG ( minValue ),
        FT_FRAME_ULONG ( defaultValue ),
        FT_FRAME_ULONG ( maxValue ),
        FT_FRAME_USHORT( flags ),
        FT_FRAME_USHORT( nameID ),
      FT_FRAME_END
    };


    if ( face->blend == NULL )
    {
      /* both `fvar' and `gvar' must be present */
      if ( (error = face->goto_table( face, TTAG_gvar,
                                      stream, &table_len )) != 0 )
        goto Exit;

      if ( (error = face->goto_table( face, TTAG_fvar,
                                      stream, &table_len )) != 0 )
        goto Exit;

      fvar_start = FT_STREAM_POS( );

      if ( FT_STREAM_READ_FIELDS( fvar_fields, &fvar_head ) )
        goto Exit;

      if ( fvar_head.version != (FT_Long)0x00010000L                      ||
           fvar_head.countSizePairs != 2                                  ||
           fvar_head.axisSize != 20                                       ||
           fvar_head.instanceSize != 4 + 4 * fvar_head.axisCount          ||
           fvar_head.offsetToData + fvar_head.axisCount * 20U +
             fvar_head.instanceCount * fvar_head.instanceSize > table_len )
      {
        error = TT_Err_Invalid_Table;
        goto Exit;
      }

      if ( FT_NEW( face->blend ) )
        goto Exit;

      /* XXX: TODO - check for overflows */
      face->blend->mmvar_len =
        sizeof ( FT_MM_Var ) +
        fvar_head.axisCount * sizeof ( FT_Var_Axis ) +
        fvar_head.instanceCount * sizeof ( FT_Var_Named_Style ) +
        fvar_head.instanceCount * fvar_head.axisCount * sizeof ( FT_Fixed ) +
        5 * fvar_head.axisCount;

      if ( FT_ALLOC( mmvar, face->blend->mmvar_len ) )
        goto Exit;
      face->blend->mmvar = mmvar;

      mmvar->num_axis =
        fvar_head.axisCount;
      mmvar->num_designs =
        (FT_UInt)-1;           /* meaningless in this context; each glyph */
                               /* may have a different number of designs  */
                               /* (or tuples, as called by Apple)         */
      mmvar->num_namedstyles =
        fvar_head.instanceCount;
      mmvar->axis =
        (FT_Var_Axis*)&(mmvar[1]);
      mmvar->namedstyle =
        (FT_Var_Named_Style*)&(mmvar->axis[fvar_head.axisCount]);

      next_coords =
        (FT_Fixed*)&(mmvar->namedstyle[fvar_head.instanceCount]);
      for ( i = 0; i < fvar_head.instanceCount; ++i )
      {
        mmvar->namedstyle[i].coords  = next_coords;
        next_coords                 += fvar_head.axisCount;
      }

      next_name = (FT_String*)next_coords;
      for ( i = 0; i < fvar_head.axisCount; ++i )
      {
        mmvar->axis[i].name  = next_name;
        next_name           += 5;
      }

      if ( FT_STREAM_SEEK( fvar_start + fvar_head.offsetToData ) )
        goto Exit;

      a = mmvar->axis;
      for ( i = 0; i < fvar_head.axisCount; ++i )
      {
        GX_FVar_Axis  axis_rec;


        if ( FT_STREAM_READ_FIELDS( fvaraxis_fields, &axis_rec ) )
          goto Exit;
        a->tag     = axis_rec.axisTag;
        a->minimum = axis_rec.minValue;     /* A Fixed */
        a->def     = axis_rec.defaultValue; /* A Fixed */
        a->maximum = axis_rec.maxValue;     /* A Fixed */
        a->strid   = axis_rec.nameID;

        a->name[0] = (FT_String)(   a->tag >> 24 );
        a->name[1] = (FT_String)( ( a->tag >> 16 ) & 0xFF );
        a->name[2] = (FT_String)( ( a->tag >>  8 ) & 0xFF );
        a->name[3] = (FT_String)( ( a->tag       ) & 0xFF );
        a->name[4] = 0;

        ++a;
      }

      ns = mmvar->namedstyle;
      for ( i = 0; i < fvar_head.instanceCount; ++i )
      {
        if ( FT_FRAME_ENTER( 4L + 4L * fvar_head.axisCount ) )
          goto Exit;

        ns->strid       =    FT_GET_USHORT();
        (void) /* flags = */ FT_GET_USHORT();

        for ( j = 0; j < fvar_head.axisCount; ++j )
          ns->coords[j] = FT_GET_ULONG();     /* A Fixed */

        FT_FRAME_EXIT();
      }
    }

    if ( master != NULL )
    {
      FT_UInt  n;


      if ( FT_ALLOC( mmvar, face->blend->mmvar_len ) )
        goto Exit;
      FT_MEM_COPY( mmvar, face->blend->mmvar, face->blend->mmvar_len );

      mmvar->axis =
        (FT_Var_Axis*)&(mmvar[1]);
      mmvar->namedstyle =
        (FT_Var_Named_Style*)&(mmvar->axis[mmvar->num_axis]);
      next_coords =
        (FT_Fixed*)&(mmvar->namedstyle[mmvar->num_namedstyles]);

      for ( n = 0; n < mmvar->num_namedstyles; ++n )
      {
        mmvar->namedstyle[n].coords  = next_coords;
        next_coords                 += mmvar->num_axis;
      }

      a = mmvar->axis;
      next_name = (FT_String*)next_coords;
      for ( n = 0; n < mmvar->num_axis; ++n )
      {
        a->name = next_name;

        /* standard PostScript names for some standard apple tags */
        if ( a->tag == TTAG_wght )
          a->name = (char *)"Weight";
        else if ( a->tag == TTAG_wdth )
          a->name = (char *)"Width";
        else if ( a->tag == TTAG_opsz )
          a->name = (char *)"OpticalSize";
        else if ( a->tag == TTAG_slnt )
          a->name = (char *)"Slant";

        next_name += 5;
        ++a;
      }

      *master = mmvar;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Set_MM_Blend                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Set the blend (normalized) coordinates for this instance of the    */
  /*    font.  Check that the `gvar' table is reasonable and does some     */
  /*    initial preparation.                                               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The font.                                            */
  /*                  Initialize the blend structure with `gvar' data.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_coords :: Must be the axis count of the font.                  */
  /*                                                                       */
  /*    coords     :: An array of num_coords, each between [-1,1].         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Set_MM_Blend( TT_Face    face,
                   FT_UInt    num_coords,
                   FT_Fixed*  coords )
  {
    FT_Error    error = TT_Err_Ok;
    GX_Blend    blend;
    FT_MM_Var*  mmvar;
    FT_UInt     i;
    FT_Memory   memory = face->root.memory;

    enum
    {
      mcvt_retain,
      mcvt_modify,
      mcvt_load

    } manageCvt;


    face->doblend = FALSE;

    if ( face->blend == NULL )
    {
      if ( (error = TT_Get_MM_Var( face, NULL)) != 0 )
        goto Exit;
    }

    blend = face->blend;
    mmvar = blend->mmvar;

    if ( num_coords != mmvar->num_axis )
    {
      error = TT_Err_Invalid_Argument;
      goto Exit;
    }

    for ( i = 0; i < num_coords; ++i )
      if ( coords[i] < -0x00010000L || coords[i] > 0x00010000L )
      {
        error = TT_Err_Invalid_Argument;
        goto Exit;
      }

    if ( blend->glyphoffsets == NULL )
      if ( (error = ft_var_load_gvar( face )) != 0 )
        goto Exit;

    if ( blend->normalizedcoords == NULL )
    {
      if ( FT_NEW_ARRAY( blend->normalizedcoords, num_coords ) )
        goto Exit;

      manageCvt = mcvt_modify;

      /* If we have not set the blend coordinates before this, then the  */
      /* cvt table will still be what we read from the `cvt ' table and  */
      /* we don't need to reload it.  We may need to change it though... */
    }
    else
    {
      for ( i = 0;
            i < num_coords && blend->normalizedcoords[i] == coords[i];
            ++i );
        if ( i == num_coords )
          manageCvt = mcvt_retain;
        else
          manageCvt = mcvt_load;

      /* If we don't change the blend coords then we don't need to do  */
      /* anything to the cvt table.  It will be correct.  Otherwise we */
      /* no longer have the original cvt (it was modified when we set  */
      /* the blend last time), so we must reload and then modify it.   */
    }

    blend->num_axis = num_coords;
    FT_MEM_COPY( blend->normalizedcoords,
                 coords,
                 num_coords * sizeof ( FT_Fixed ) );

    face->doblend = TRUE;

    if ( face->cvt != NULL )
    {
      switch ( manageCvt )
      {
      case mcvt_load:
        /* The cvt table has been loaded already; every time we change the */
        /* blend we may need to reload and remodify the cvt table.         */
        FT_FREE( face->cvt );
        face->cvt = NULL;

        tt_face_load_cvt( face, face->root.stream );
        break;

      case mcvt_modify:
        /* The original cvt table is in memory.  All we need to do is */
        /* apply the `cvar' table (if any).                           */
        tt_face_vary_cvt( face, face->root.stream );
        break;

      case mcvt_retain:
        /* The cvt table is correct for this set of coordinates. */
        break;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Set_Var_Design                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Set the coordinates for the instance, measured in the user         */
  /*    coordinate system.  Parse the `avar' table (if present) to convert */
  /*    from user to normalized coordinates.                               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The font face.                                       */
  /*                  Initialize the blend struct with `gvar' data.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_coords :: This must be the axis count of the font.             */
  /*                                                                       */
  /*    coords     :: A coordinate array with `num_coords' elements.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Set_Var_Design( TT_Face    face,
                     FT_UInt    num_coords,
                     FT_Fixed*  coords )
  {
    FT_Error        error      = TT_Err_Ok;
    FT_Fixed*       normalized = NULL;
    GX_Blend        blend;
    FT_MM_Var*      mmvar;
    FT_UInt         i, j;
    FT_Var_Axis*    a;
    GX_AVarSegment  av;
    FT_Memory       memory = face->root.memory;


    if ( face->blend == NULL )
    {
      if ( (error = TT_Get_MM_Var( face, NULL )) != 0 )
        goto Exit;
    }

    blend = face->blend;
    mmvar = blend->mmvar;

    if ( num_coords != mmvar->num_axis )
    {
      error = TT_Err_Invalid_Argument;
      goto Exit;
    }

    /* Axis normalization is a two stage process.  First we normalize */
    /* based on the [min,def,max] values for the axis to be [-1,0,1]. */
    /* Then, if there's an `avar' table, we renormalize this range.   */

    if ( FT_NEW_ARRAY( normalized, mmvar->num_axis ) )
      goto Exit;

    a = mmvar->axis;
    for ( i = 0; i < mmvar->num_axis; ++i, ++a )
    {
      if ( coords[i] > a->maximum || coords[i] < a->minimum )
      {
        error = TT_Err_Invalid_Argument;
        goto Exit;
      }

      if ( coords[i] < a->def )
      {
        normalized[i] = -FT_MulDiv( coords[i] - a->def,
                                    0x10000L,
                                    a->minimum - a->def );
      }
      else if ( a->maximum == a->def )
        normalized[i] = 0;
      else
      {
        normalized[i] = FT_MulDiv( coords[i] - a->def,
                                   0x10000L,
                                   a->maximum - a->def );
      }
    }

    if ( !blend->avar_checked )
      ft_var_load_avar( face );

    if ( blend->avar_segment != NULL )
    {
      av = blend->avar_segment;
      for ( i = 0; i < mmvar->num_axis; ++i, ++av )
      {
        for ( j = 1; j < (FT_UInt)av->pairCount; ++j )
          if ( normalized[i] < av->correspondence[j].fromCoord )
          {
            normalized[i] =
              FT_MulDiv(
                FT_MulDiv(
                  normalized[i] - av->correspondence[j - 1].fromCoord,
                  0x10000L,
                  av->correspondence[j].fromCoord -
                    av->correspondence[j - 1].fromCoord ),
                av->correspondence[j].toCoord -
                  av->correspondence[j - 1].toCoord,
                0x10000L ) +
              av->correspondence[j - 1].toCoord;
            break;
          }
      }
    }

    error = TT_Set_MM_Blend( face, num_coords, normalized );

  Exit:
    FT_FREE( normalized );
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     GX VAR PARSING ROUTINES                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_vary_cvt                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Modify the loaded cvt table according to the `cvar' table and the  */
  /*    font's blend.                                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /*    Most errors are ignored.  It is perfectly valid not to have a      */
  /*    `cvar' table even if there is a `gvar' and `fvar' table.           */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_vary_cvt( TT_Face    face,
                    FT_Stream  stream )
  {
    FT_Error    error;
    FT_Memory   memory = stream->memory;
    FT_ULong    table_start;
    FT_ULong    table_len;
    FT_UInt     tupleCount;
    FT_ULong    offsetToData;
    FT_ULong    here;
    FT_UInt     i, j;
    FT_Fixed*   tuple_coords    = NULL;
    FT_Fixed*   im_start_coords = NULL;
    FT_Fixed*   im_end_coords   = NULL;
    GX_Blend    blend           = face->blend;
    FT_UInt     point_count;
    FT_UShort*  localpoints;
    FT_Short*   deltas;


    FT_TRACE2(( "CVAR " ));

    if ( blend == NULL )
    {
      FT_TRACE2(( "no blend specified!\n" ));

      error = TT_Err_Ok;
      goto Exit;
    }

    if ( face->cvt == NULL )
    {
      FT_TRACE2(( "no `cvt ' table!\n" ));

      error = TT_Err_Ok;
      goto Exit;
    }

    error = face->goto_table( face, TTAG_cvar, stream, &table_len );
    if ( error )
    {
      FT_TRACE2(( "is missing!\n" ));

      error = TT_Err_Ok;
      goto Exit;
    }

    if ( FT_FRAME_ENTER( table_len ) )
    {
      error = TT_Err_Ok;
      goto Exit;
    }

    table_start = FT_Stream_FTell( stream );
    if ( FT_GET_LONG() != 0x00010000L )
    {
      FT_TRACE2(( "bad table version!\n" ));

      error = TT_Err_Ok;
      goto FExit;
    }

    if ( FT_NEW_ARRAY( tuple_coords, blend->num_axis )    ||
         FT_NEW_ARRAY( im_start_coords, blend->num_axis ) ||
         FT_NEW_ARRAY( im_end_coords, blend->num_axis )   )
      goto FExit;

    tupleCount   = FT_GET_USHORT();
    offsetToData = table_start + FT_GET_USHORT();

    /* The documentation implies there are flags packed into the        */
    /* tuplecount, but John Jenkins says that shared points don't apply */
    /* to `cvar', and no other flags are defined.                       */

    for ( i = 0; i < ( tupleCount & 0xFFF ); ++i )
    {
      FT_UInt   tupleDataSize;
      FT_UInt   tupleIndex;
      FT_Fixed  apply;


      tupleDataSize = FT_GET_USHORT();
      tupleIndex    = FT_GET_USHORT();

      /* There is no provision here for a global tuple coordinate section, */
      /* so John says.  There are no tuple indices, just embedded tuples.  */

      if ( tupleIndex & GX_TI_EMBEDDED_TUPLE_COORD )
      {
        for ( j = 0; j < blend->num_axis; ++j )
          tuple_coords[j] = FT_GET_SHORT() << 2; /* convert from        */
                                                 /* short frac to fixed */
      }
      else
      {
        /* skip this tuple; it makes no sense */

        if ( tupleIndex & GX_TI_INTERMEDIATE_TUPLE )
          for ( j = 0; j < 2 * blend->num_axis; ++j )
            (void)FT_GET_SHORT();

        offsetToData += tupleDataSize;
        continue;
      }

      if ( tupleIndex & GX_TI_INTERMEDIATE_TUPLE )
      {
        for ( j = 0; j < blend->num_axis; ++j )
          im_start_coords[j] = FT_GET_SHORT() << 2;
        for ( j = 0; j < blend->num_axis; ++j )
          im_end_coords[j] = FT_GET_SHORT() << 2;
      }

      apply = ft_var_apply_tuple( blend,
                                  (FT_UShort)tupleIndex,
                                  tuple_coords,
                                  im_start_coords,
                                  im_end_coords );
      if ( /* tuple isn't active for our blend */
           apply == 0                                    ||
           /* global points not allowed,           */
           /* if they aren't local, makes no sense */
           !( tupleIndex & GX_TI_PRIVATE_POINT_NUMBERS ) )
      {
        offsetToData += tupleDataSize;
        continue;
      }

      here = FT_Stream_FTell( stream );

      FT_Stream_SeekSet( stream, offsetToData );

      localpoints = ft_var_readpackedpoints( stream, &point_count );
      deltas      = ft_var_readpackeddeltas( stream,
                                             point_count == 0 ? face->cvt_size
                                                              : point_count );
      if ( localpoints == NULL || deltas == NULL )
        /* failure, ignore it */;

      else if ( localpoints == ALL_POINTS )
      {
        /* this means that there are deltas for every entry in cvt */
        for ( j = 0; j < face->cvt_size; ++j )
          face->cvt[j] = (FT_Short)( face->cvt[j] +
                                     FT_MulFix( deltas[j], apply ) );
      }

      else
      {
        for ( j = 0; j < point_count; ++j )
        {
          int  pindex = localpoints[j];

          face->cvt[pindex] = (FT_Short)( face->cvt[pindex] +
                                          FT_MulFix( deltas[j], apply ) );
        }
      }

      if ( localpoints != ALL_POINTS )
        FT_FREE( localpoints );
      FT_FREE( deltas );

      offsetToData += tupleDataSize;

      FT_Stream_SeekSet( stream, here );
    }

  FExit:
    FT_FRAME_EXIT();

  Exit:
    FT_FREE( tuple_coords );
    FT_FREE( im_start_coords );
    FT_FREE( im_end_coords );

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Vary_Get_Glyph_Deltas                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the appropriate deltas for the current glyph.                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the target face object.                 */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph being modified.              */
  /*                                                                       */
  /*    n_points    :: The number of the points in the glyph, including    */
  /*                   phantom points.                                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    deltas      :: The array of points to change.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Vary_Get_Glyph_Deltas( TT_Face      face,
                            FT_UInt      glyph_index,
                            FT_Vector*  *deltas,
                            FT_UInt      n_points )
  {
    FT_Stream   stream = face->root.stream;
    FT_Memory   memory = stream->memory;
    GX_Blend    blend  = face->blend;
    FT_Vector*  delta_xy;

    FT_Error    error;
    FT_ULong    glyph_start;
    FT_UInt     tupleCount;
    FT_ULong    offsetToData;
    FT_ULong    here;
    FT_UInt     i, j;
    FT_Fixed*   tuple_coords    = NULL;
    FT_Fixed*   im_start_coords = NULL;
    FT_Fixed*   im_end_coords   = NULL;
    FT_UInt     point_count, spoint_count = 0;
    FT_UShort*  sharedpoints = NULL;
    FT_UShort*  localpoints  = NULL;
    FT_UShort*  points;
    FT_Short    *deltas_x, *deltas_y;


    if ( !face->doblend || blend == NULL )
      return TT_Err_Invalid_Argument;

    /* to be freed by the caller */
    if ( FT_NEW_ARRAY( delta_xy, n_points ) )
      goto Exit;
    *deltas = delta_xy;

    if ( glyph_index >= blend->gv_glyphcnt      ||
         blend->glyphoffsets[glyph_index] ==
           blend->glyphoffsets[glyph_index + 1] )
      return TT_Err_Ok;               /* no variation data for this glyph */

    if ( FT_STREAM_SEEK( blend->glyphoffsets[glyph_index] )   ||
         FT_FRAME_ENTER( blend->glyphoffsets[glyph_index + 1] -
                           blend->glyphoffsets[glyph_index] ) )
      goto Fail1;

    glyph_start = FT_Stream_FTell( stream );

    /* each set of glyph variation data is formatted similarly to `cvar' */
    /* (except we get shared points and global tuples)                   */

    if ( FT_NEW_ARRAY( tuple_coords, blend->num_axis )    ||
         FT_NEW_ARRAY( im_start_coords, blend->num_axis ) ||
         FT_NEW_ARRAY( im_end_coords, blend->num_axis )   )
      goto Fail2;

    tupleCount   = FT_GET_USHORT();
    offsetToData = glyph_start + FT_GET_USHORT();

    if ( tupleCount & GX_TC_TUPLES_SHARE_POINT_NUMBERS )
    {
      here = FT_Stream_FTell( stream );

      FT_Stream_SeekSet( stream, offsetToData );

      sharedpoints = ft_var_readpackedpoints( stream, &spoint_count );
      offsetToData = FT_Stream_FTell( stream );

      FT_Stream_SeekSet( stream, here );
    }

    for ( i = 0; i < ( tupleCount & GX_TC_TUPLE_COUNT_MASK ); ++i )
    {
      FT_UInt   tupleDataSize;
      FT_UInt   tupleIndex;
      FT_Fixed  apply;


      tupleDataSize = FT_GET_USHORT();
      tupleIndex    = FT_GET_USHORT();

      if ( tupleIndex & GX_TI_EMBEDDED_TUPLE_COORD )
      {
        for ( j = 0; j < blend->num_axis; ++j )
          tuple_coords[j] = FT_GET_SHORT() << 2;  /* convert from        */
                                                  /* short frac to fixed */
      }
      else if ( ( tupleIndex & GX_TI_TUPLE_INDEX_MASK ) >= blend->tuplecount )
      {
        error = TT_Err_Invalid_Table;
        goto Fail3;
      }
      else
      {
        FT_MEM_COPY(
          tuple_coords,
          &blend->tuplecoords[(tupleIndex & 0xFFF) * blend->num_axis],
          blend->num_axis * sizeof ( FT_Fixed ) );
      }

      if ( tupleIndex & GX_TI_INTERMEDIATE_TUPLE )
      {
        for ( j = 0; j < blend->num_axis; ++j )
          im_start_coords[j] = FT_GET_SHORT() << 2;
        for ( j = 0; j < blend->num_axis; ++j )
          im_end_coords[j] = FT_GET_SHORT() << 2;
      }

      apply = ft_var_apply_tuple( blend,
                                  (FT_UShort)tupleIndex,
                                  tuple_coords,
                                  im_start_coords,
                                  im_end_coords );

      if ( apply == 0 )              /* tuple isn't active for our blend */
      {
        offsetToData += tupleDataSize;
        continue;
      }

      here = FT_Stream_FTell( stream );

      if ( tupleIndex & GX_TI_PRIVATE_POINT_NUMBERS )
      {
        FT_Stream_SeekSet( stream, offsetToData );

        localpoints = ft_var_readpackedpoints( stream, &point_count );
        points      = localpoints;
      }
      else
      {
        points      = sharedpoints;
        point_count = spoint_count;
      }

      deltas_x = ft_var_readpackeddeltas( stream,
                                          point_count == 0 ? n_points
                                                           : point_count );
      deltas_y = ft_var_readpackeddeltas( stream,
                                          point_count == 0 ? n_points
                                                           : point_count );

      if ( points == NULL || deltas_y == NULL || deltas_x == NULL )
        ; /* failure, ignore it */

      else if ( points == ALL_POINTS )
      {
        /* this means that there are deltas for every point in the glyph */
        for ( j = 0; j < n_points; ++j )
        {
          delta_xy[j].x += FT_MulFix( deltas_x[j], apply );
          delta_xy[j].y += FT_MulFix( deltas_y[j], apply );
        }
      }

      else
      {
        for ( j = 0; j < point_count; ++j )
        {
          delta_xy[localpoints[j]].x += FT_MulFix( deltas_x[j], apply );
          delta_xy[localpoints[j]].y += FT_MulFix( deltas_y[j], apply );
        }
      }

      if ( localpoints != ALL_POINTS )
        FT_FREE( localpoints );
      FT_FREE( deltas_x );
      FT_FREE( deltas_y );

      offsetToData += tupleDataSize;

      FT_Stream_SeekSet( stream, here );
    }

  Fail3:
    FT_FREE( tuple_coords );
    FT_FREE( im_start_coords );
    FT_FREE( im_end_coords );

  Fail2:
    FT_FRAME_EXIT();

  Fail1:
    if ( error )
    {
      FT_FREE( delta_xy );
      *deltas = NULL;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_done_blend                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the blend internal data structure.                           */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_done_blend( FT_Memory  memory,
                 GX_Blend   blend )
  {
    if ( blend != NULL )
    {
      FT_UInt  i;


      FT_FREE( blend->normalizedcoords );
      FT_FREE( blend->mmvar );

      if ( blend->avar_segment != NULL )
      {
        for ( i = 0; i < blend->num_axis; ++i )
          FT_FREE( blend->avar_segment[i].correspondence );
        FT_FREE( blend->avar_segment );
      }

      FT_FREE( blend->tuplecoords );
      FT_FREE( blend->glyphoffsets );
      FT_FREE( blend );
    }
  }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */


/* END */

#endif


/* END */

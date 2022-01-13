/***************************************************************************/
/*                                                                         */
/*  type42.c                                                               */
/*                                                                         */
/*    FreeType Type 42 driver component.                                   */
/*                                                                         */
/*  Copyright 2002 by                                                      */
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
/*  t42objs.c                                                              */
/*                                                                         */
/*    Type 42 objects manager (body).                                      */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005, 2006 by Roberto Alameda.             */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "t42objs.h"
#include "t42parse.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_LIST_H


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


  static FT_Error
  T42_Open_Face( T42_Face  face )
  {
    T42_LoaderRec  loader;
    T42_Parser     parser;
    T1_Font        type1 = &face->type1;
    FT_Memory      memory = face->root.memory;
    FT_Error       error;

    PSAux_Service  psaux  = (PSAux_Service)face->psaux;


    t42_loader_init( &loader, face );

    parser = &loader.parser;

    if ( FT_ALLOC( face->ttf_data, 12 ) )
      goto Exit;

    error = t42_parser_init( parser,
                             face->root.stream,
                             memory,
                             psaux);
    if ( error )
      goto Exit;

    error = t42_parse_dict( face, &loader,
                            parser->base_dict, parser->base_len );

    if ( type1->font_type != 42 )
    {
      error = T42_Err_Unknown_File_Format;
      goto Exit;
    }

    /* now, propagate the charstrings and glyphnames tables */
    /* to the Type1 data                                    */
    type1->num_glyphs = loader.num_glyphs;

    if ( !loader.charstrings.init )
    {
      FT_ERROR(( "T42_Open_Face: no charstrings array in face!\n" ));
      error = T42_Err_Invalid_File_Format;
    }

    loader.charstrings.init  = 0;
    type1->charstrings_block = loader.charstrings.block;
    type1->charstrings       = loader.charstrings.elements;
    type1->charstrings_len   = loader.charstrings.lengths;

    /* we copy the glyph names `block' and `elements' fields; */
    /* the `lengths' field must be released later             */
    type1->glyph_names_block    = loader.glyph_names.block;
    type1->glyph_names          = (FT_String**)loader.glyph_names.elements;
    loader.glyph_names.block    = 0;
    loader.glyph_names.elements = 0;

    /* we must now build type1.encoding when we have a custom array */
    if ( type1->encoding_type == T1_ENCODING_TYPE_ARRAY )
    {
      FT_Int    charcode, idx, min_char, max_char;
      FT_Byte*  char_name;
      FT_Byte*  glyph_name;


      /* OK, we do the following: for each element in the encoding   */
      /* table, look up the index of the glyph having the same name  */
      /* as defined in the CharStrings array.                        */
      /* The index is then stored in type1.encoding.char_index, and  */
      /* the name in type1.encoding.char_name                        */

      min_char = +32000;
      max_char = -32000;

      charcode = 0;
      for ( ; charcode < loader.encoding_table.max_elems; charcode++ )
      {
        type1->encoding.char_index[charcode] = 0;
        type1->encoding.char_name [charcode] = (char *)".notdef";

        char_name = loader.encoding_table.elements[charcode];
        if ( char_name )
          for ( idx = 0; idx < type1->num_glyphs; idx++ )
          {
            glyph_name = (FT_Byte*)type1->glyph_names[idx];
            if ( ft_strcmp( (const char*)char_name,
                            (const char*)glyph_name ) == 0 )
            {
              type1->encoding.char_index[charcode] = (FT_UShort)idx;
              type1->encoding.char_name [charcode] = (char*)glyph_name;

              /* Change min/max encoded char only if glyph name is */
              /* not /.notdef                                      */
              if ( ft_strcmp( (const char*)".notdef",
                              (const char*)glyph_name ) != 0 )
              {
                if ( charcode < min_char )
                  min_char = charcode;
                if ( charcode > max_char )
                  max_char = charcode;
              }
              break;
            }
          }
      }
      type1->encoding.code_first = min_char;
      type1->encoding.code_last  = max_char;
      type1->encoding.num_chars  = loader.num_chars;
    }

  Exit:
    t42_loader_done( &loader );
    return error;
  }


  /***************** Driver Functions *************/


  FT_LOCAL_DEF( FT_Error )
  T42_Face_Init( FT_Stream      stream,
                 T42_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error            error;
    FT_Service_PsCMaps  psnames;
    PSAux_Service       psaux;
    FT_Face             root  = (FT_Face)&face->root;
    T1_Font             type1 = &face->type1;
    PS_FontInfo         info  = &type1->font_info;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );
    FT_UNUSED( stream );


    face->ttf_face       = NULL;
    face->root.num_faces = 1;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    face->psnames = psnames;

    face->psaux = FT_Get_Module_Interface( FT_FACE_LIBRARY( face ),
                                           "psaux" );
    psaux = (PSAux_Service)face->psaux;

    /* open the tokenizer, this will also check the font format */
    error = T42_Open_Face( face );
    if ( error )
      goto Exit;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Exit;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "T42_Face_Init: invalid face index\n" ));
      error = T42_Err_Invalid_Argument;
      goto Exit;
    }

    /* Now load the font program into the face object */

    /* Init the face object fields */
    /* Now set up root face fields */

    root->num_glyphs   = type1->num_glyphs;
    root->num_charmaps = 0;
    root->face_index   = face_index;

    root->face_flags = FT_FACE_FLAG_SCALABLE    |
                       FT_FACE_FLAG_HORIZONTAL  |
                       FT_FACE_FLAG_GLYPH_NAMES;

    if ( info->is_fixed_pitch )
      root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    root->face_flags |= FT_FACE_FLAG_HINTER;
#endif

    /* XXX: TODO -- add kerning with .afm support */

    /* get style name -- be careful, some broken fonts only */
    /* have a `/FontName' dictionary entry!                 */
    root->family_name = info->family_name;
    /* assume "Regular" style if we don't know better */
    root->style_name = (char *)"Regular";
    if ( root->family_name )
    {
      char*  full   = info->full_name;
      char*  family = root->family_name;


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
                root->style_name = full;
              break;
            }
          }
        }
      }
    }
    else
    {
      /* do we have a `/FontName'? */
      if ( type1->font_name )
        root->family_name = type1->font_name;
    }

    /* no embedded bitmap support */
    root->num_fixed_sizes = 0;
    root->available_sizes = 0;

    /* Load the TTF font embedded in the T42 font */
    {
      FT_Open_Args  args;


      args.flags       = FT_OPEN_MEMORY;
      args.memory_base = face->ttf_data;
      args.memory_size = face->ttf_size;

      if ( num_params )
      {
        args.flags     |= FT_OPEN_PARAMS;
        args.num_params = num_params;
        args.params     = params;
      }

      error = FT_Open_Face( FT_FACE_LIBRARY( face ),
                            &args, 0, &face->ttf_face );
    }

    if ( error )
      goto Exit;

    FT_Done_Size( face->ttf_face->size );

    /* Ignore info in FontInfo dictionary and use the info from the  */
    /* loaded TTF font.  The PostScript interpreter also ignores it. */
    root->bbox         = face->ttf_face->bbox;
    root->units_per_EM = face->ttf_face->units_per_EM;

    root->ascender  = face->ttf_face->ascender;
    root->descender = face->ttf_face->descender;
    root->height    = face->ttf_face->height;

    root->max_advance_width  = face->ttf_face->max_advance_width;
    root->max_advance_height = face->ttf_face->max_advance_height;

    root->underline_position  = (FT_Short)info->underline_position;
    root->underline_thickness = (FT_Short)info->underline_thickness;

    /* compute style flags */
    root->style_flags = 0;
    if ( info->italic_angle )
      root->style_flags |= FT_STYLE_FLAG_ITALIC;

    if ( face->ttf_face->style_flags & FT_STYLE_FLAG_BOLD )
      root->style_flags |= FT_STYLE_FLAG_BOLD;

    if ( face->ttf_face->face_flags & FT_FACE_FLAG_VERTICAL )
      root->face_flags |= FT_FACE_FLAG_VERTICAL;

    {
      if ( psnames && psaux )
      {
        FT_CharMapRec    charmap;
        T1_CMap_Classes  cmap_classes = psaux->t1_cmap_classes;
        FT_CMap_Class    clazz;


        charmap.face = root;

        /* first of all, try to synthetize a Unicode charmap */
        charmap.platform_id = 3;
        charmap.encoding_id = 1;
        charmap.encoding    = FT_ENCODING_UNICODE;

        FT_CMap_New( cmap_classes->unicode, NULL, &charmap, NULL );

        /* now, generate an Adobe Standard encoding when appropriate */
        charmap.platform_id = 7;
        clazz               = NULL;

        switch ( type1->encoding_type )
        {
        case T1_ENCODING_TYPE_STANDARD:
          charmap.encoding    = FT_ENCODING_ADOBE_STANDARD;
          charmap.encoding_id = 0;
          clazz               = cmap_classes->standard;
          break;

        case T1_ENCODING_TYPE_EXPERT:
          charmap.encoding    = FT_ENCODING_ADOBE_EXPERT;
          charmap.encoding_id = 1;
          clazz               = cmap_classes->expert;
          break;

        case T1_ENCODING_TYPE_ARRAY:
          charmap.encoding    = FT_ENCODING_ADOBE_CUSTOM;
          charmap.encoding_id = 2;
          clazz               = cmap_classes->custom;
          break;

        case T1_ENCODING_TYPE_ISOLATIN1:
          charmap.encoding    = FT_ENCODING_ADOBE_LATIN_1;
          charmap.encoding_id = 3;
          clazz               = cmap_classes->unicode;
          break;

        default:
          ;
        }

        if ( clazz )
          FT_CMap_New( clazz, NULL, &charmap, NULL );

#if 0
        /* Select default charmap */
        if ( root->num_charmaps )
          root->charmap = root->charmaps[0];
#endif
      }
    }
  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  T42_Face_Done( T42_Face  face )
  {
    T1_Font      type1;
    PS_FontInfo  info;
    FT_Memory    memory;


    if ( face )
    {
      type1  = &face->type1;
      info   = &type1->font_info;
      memory = face->root.memory;

      /* delete internal ttf face prior to freeing face->ttf_data */
      if ( face->ttf_face )
        FT_Done_Face( face->ttf_face );

      /* release font info strings */
      FT_FREE( info->version );
      FT_FREE( info->notice );
      FT_FREE( info->full_name );
      FT_FREE( info->family_name );
      FT_FREE( info->weight );

      /* release top dictionary */
      FT_FREE( type1->charstrings_len );
      FT_FREE( type1->charstrings );
      FT_FREE( type1->glyph_names );

      FT_FREE( type1->charstrings_block );
      FT_FREE( type1->glyph_names_block );

      FT_FREE( type1->encoding.char_index );
      FT_FREE( type1->encoding.char_name );
      FT_FREE( type1->font_name );

      FT_FREE( face->ttf_data );

#if 0
      /* release afm data if present */
      if ( face->afm_data )
        T1_Done_AFM( memory, (T1_AFM*)face->afm_data );
#endif

      /* release unicode map, if any */
      FT_FREE( face->unicode_map.maps );
      face->unicode_map.num_maps = 0;

      face->root.family_name = 0;
      face->root.style_name  = 0;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T42_Driver_Init                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given Type 42 driver object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  T42_Driver_Init( T42_Driver  driver )
  {
    FT_Module  ttmodule;


    ttmodule = FT_Get_Module( FT_MODULE(driver)->library, "truetype" );
    driver->ttclazz = (FT_Driver_Class)ttmodule->clazz;

    return T42_Err_Ok;
  }


  FT_LOCAL_DEF( void )
  T42_Driver_Done( T42_Driver  driver )
  {
    FT_UNUSED( driver );
  }


  FT_LOCAL_DEF( FT_Error )
  T42_Size_Init( T42_Size  size )
  {
    FT_Face   face = size->root.face;
    T42_Face  t42face = (T42_Face)face;
    FT_Size   ttsize;
    FT_Error  error   = T42_Err_Ok;


    error = FT_New_Size( t42face->ttf_face, &ttsize );
    size->ttsize = ttsize;

    FT_Activate_Size( ttsize );

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T42_Size_Request( T42_Size         size,
                    FT_Size_Request  req )
  {
    T42_Face  face = (T42_Face)size->root.face;
    FT_Error  error;


    FT_Activate_Size( size->ttsize );

    error = FT_Request_Size( face->ttf_face, req );
    if ( !error )
      ( (FT_Size)size )->metrics = face->ttf_face->size->metrics;

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T42_Size_Select( T42_Size  size,
                   FT_ULong  strike_index )
  {
    T42_Face  face = (T42_Face)size->root.face;
    FT_Error  error;


    FT_Activate_Size( size->ttsize );

    error = FT_Select_Size( face->ttf_face, strike_index );
    if ( !error )
      ( (FT_Size)size )->metrics = face->ttf_face->size->metrics;

    return error;

  }


  FT_LOCAL_DEF( void )
  T42_Size_Done( T42_Size  size )
  {
    FT_Face      face    = size->root.face;
    T42_Face     t42face = (T42_Face)face;
    FT_ListNode  node;


    node = FT_List_Find( &t42face->ttf_face->sizes_list, size->ttsize );
    if ( node )
    {
      FT_Done_Size( size->ttsize );
      size->ttsize = NULL;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  T42_GlyphSlot_Init( T42_GlyphSlot  slot )
  {
    FT_Face       face    = slot->root.face;
    T42_Face      t42face = (T42_Face)face;
    FT_GlyphSlot  ttslot;
    FT_Error      error   = T42_Err_Ok;


    if ( face->glyph == NULL )
    {
      /* First glyph slot for this face */
      slot->ttslot = t42face->ttf_face->glyph;
    }
    else
    {
      error = FT_New_GlyphSlot( t42face->ttf_face, &ttslot );
      slot->ttslot = ttslot;
    }

    return error;
  }


  FT_LOCAL_DEF( void )
  T42_GlyphSlot_Done( T42_GlyphSlot slot )
  {
    FT_Done_GlyphSlot( slot->ttslot );
  }


  static void
  t42_glyphslot_clear( FT_GlyphSlot  slot )
  {
    /* free bitmap if needed */
    ft_glyphslot_free_bitmap( slot );

    /* clear all public fields in the glyph slot */
    FT_ZERO( &slot->metrics );
    FT_ZERO( &slot->outline );
    FT_ZERO( &slot->bitmap );

    slot->bitmap_left   = 0;
    slot->bitmap_top    = 0;
    slot->num_subglyphs = 0;
    slot->subglyphs     = 0;
    slot->control_data  = 0;
    slot->control_len   = 0;
    slot->other         = 0;
    slot->format        = FT_GLYPH_FORMAT_NONE;

    slot->linearHoriAdvance = 0;
    slot->linearVertAdvance = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  T42_GlyphSlot_Load( FT_GlyphSlot  glyph,
                      FT_Size       size,
                      FT_UInt       glyph_index,
                      FT_Int32      load_flags )
  {
    FT_Error         error;
    T42_GlyphSlot    t42slot = (T42_GlyphSlot)glyph;
    T42_Size         t42size = (T42_Size)size;
    FT_Driver_Class  ttclazz = ((T42_Driver)glyph->face->driver)->ttclazz;


    t42_glyphslot_clear( t42slot->ttslot );
    error = ttclazz->load_glyph( t42slot->ttslot,
                                 t42size->ttsize,
                                 glyph_index,
                                 load_flags | FT_LOAD_NO_BITMAP );

    if ( !error )
    {
      glyph->metrics = t42slot->ttslot->metrics;

      glyph->linearHoriAdvance = t42slot->ttslot->linearHoriAdvance;
      glyph->linearVertAdvance = t42slot->ttslot->linearVertAdvance;

      glyph->format  = t42slot->ttslot->format;
      glyph->outline = t42slot->ttslot->outline;

      glyph->bitmap      = t42slot->ttslot->bitmap;
      glyph->bitmap_left = t42slot->ttslot->bitmap_left;
      glyph->bitmap_top  = t42slot->ttslot->bitmap_top;

      glyph->num_subglyphs = t42slot->ttslot->num_subglyphs;
      glyph->subglyphs     = t42slot->ttslot->subglyphs;

      glyph->control_data  = t42slot->ttslot->control_data;
      glyph->control_len   = t42slot->ttslot->control_len;
    }

    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  t42parse.c                                                             */
/*                                                                         */
/*    Type 42 font parser (body).                                          */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005, 2006 by Roberto Alameda.             */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "t42parse.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_LIST_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader );
  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader );

  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader );

  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader );


  static const
  T1_FieldRec  t42_keywords[] = {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontInfo
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_INFO

    T1_FIELD_STRING( "version",            version )
    T1_FIELD_STRING( "Notice",             notice )
    T1_FIELD_STRING( "FullName",           full_name )
    T1_FIELD_STRING( "FamilyName",         family_name )
    T1_FIELD_STRING( "Weight",             weight )
    T1_FIELD_NUM   ( "ItalicAngle",        italic_angle )
    T1_FIELD_BOOL  ( "isFixedPitch",       is_fixed_pitch )
    T1_FIELD_NUM   ( "UnderlinePosition",  underline_position )
    T1_FIELD_NUM   ( "UnderlineThickness", underline_thickness )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_DICT

    T1_FIELD_KEY  ( "FontName",    font_name )
    T1_FIELD_NUM  ( "PaintType",   paint_type )
    T1_FIELD_NUM  ( "FontType",    font_type )
    T1_FIELD_FIXED( "StrokeWidth", stroke_width )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_BBox
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_BBOX

    T1_FIELD_BBOX("FontBBox", xMin )

    T1_FIELD_CALLBACK( "FontMatrix",  t42_parse_font_matrix )
    T1_FIELD_CALLBACK( "Encoding",    t42_parse_encoding )
    T1_FIELD_CALLBACK( "CharStrings", t42_parse_charstrings )
    T1_FIELD_CALLBACK( "sfnts",       t42_parse_sfnts )

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0 }
  };


#define T1_Add_Table( p, i, o, l )  (p)->funcs.add( (p), i, o, l )
#define T1_Done_Table( p )          \
          do                        \
          {                         \
            if ( (p)->funcs.done )  \
              (p)->funcs.done( p ); \
          } while ( 0 )
#define T1_Release_Table( p )          \
          do                           \
          {                            \
            if ( (p)->funcs.release )  \
              (p)->funcs.release( p ); \
          } while ( 0 )

#define T1_Skip_Spaces( p )    (p)->root.funcs.skip_spaces( &(p)->root )
#define T1_Skip_PS_Token( p )  (p)->root.funcs.skip_PS_token( &(p)->root )

#define T1_ToInt( p )                          \
          (p)->root.funcs.to_int( &(p)->root )
#define T1_ToBytes( p, b, m, n, d )                          \
          (p)->root.funcs.to_bytes( &(p)->root, b, m, n, d )

#define T1_ToFixedArray( p, m, f, t )                           \
          (p)->root.funcs.to_fixed_array( &(p)->root, m, f, t )
#define T1_ToToken( p, t )                          \
          (p)->root.funcs.to_token( &(p)->root, t )

#define T1_Load_Field( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field( &(p)->root, f, o, m, pf )
#define T1_Load_Field_Table( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field_table( &(p)->root, f, o, m, pf )


  /********************* Parsing Functions ******************/

  FT_LOCAL_DEF( FT_Error )
  t42_parser_init( T42_Parser     parser,
                   FT_Stream      stream,
                   FT_Memory      memory,
                   PSAux_Service  psaux )
  {
    FT_Error  error = T42_Err_Ok;
    FT_Long   size;


    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream    = stream;
    parser->base_len  = 0;
    parser->base_dict = 0;
    parser->in_memory = 0;

    /*******************************************************************/
    /*                                                                 */
    /* Here a short summary of what is going on:                       */
    /*                                                                 */
    /*   When creating a new Type 42 parser, we try to locate and load */
    /*   the base dictionary, loading the whole font into memory.      */
    /*                                                                 */
    /*   When `loading' the base dictionary, we only set up pointers   */
    /*   in the case of a memory-based stream.  Otherwise, we allocate */
    /*   and load the base dictionary in it.                           */
    /*                                                                 */
    /*   parser->in_memory is set if we have a memory stream.          */
    /*                                                                 */

    if ( FT_STREAM_SEEK( 0L ) ||
         FT_FRAME_ENTER( 17 ) )
      goto Exit;

    if ( ft_memcmp( stream->cursor, "%!PS-TrueTypeFont", 17 ) != 0 )
    {
      FT_TRACE2(( "not a Type42 font\n" ));
      error = T42_Err_Unknown_File_Format;
    }

    FT_FRAME_EXIT();

    if ( error || FT_STREAM_SEEK( 0 ) )
      goto Exit;

    size = stream->size;

    /* now, try to load `size' bytes of the `base' dictionary we */
    /* found previously                                          */

    /* if it is a memory-based resource, set up pointers */
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      /* check that the `size' field is valid */
      if ( FT_STREAM_SKIP( size ) )
        goto Exit;
    }
    else
    {
      /* read segment in memory */
      if ( FT_ALLOC( parser->base_dict, size )       ||
           FT_STREAM_READ( parser->base_dict, size ) )
        goto Exit;

      parser->base_len = size;
    }

    parser->root.base   = parser->base_dict;
    parser->root.cursor = parser->base_dict;
    parser->root.limit  = parser->root.cursor + parser->base_len;

  Exit:
    if ( error && !parser->in_memory )
      FT_FREE( parser->base_dict );

    return error;
  }


  FT_LOCAL_DEF( void )
  t42_parser_done( T42_Parser  parser )
  {
    FT_Memory  memory = parser->root.memory;


    /* free the base dictionary only when we have a disk stream */
    if ( !parser->in_memory )
      FT_FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  static int
  t42_is_space( FT_Byte  c )
  {
    return ( c == ' '  || c == '\t'              ||
             c == '\r' || c == '\n' || c == '\f' ||
             c == '\0'                           );
  }


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Vector*  offset = &face->type1.font_offset;
    FT_Face     root   = (FT_Face)&face->root;
    FT_Fixed    temp[6];
    FT_Fixed    temp_scale;


    (void)T1_ToFixedArray( parser, 6, temp, 3 );

    temp_scale = FT_ABS( temp[3] );

    /* Set Units per EM based on FontMatrix values.  We set the value to */
    /* 1000 / temp_scale, because temp_scale was already multiplied by   */
    /* 1000 (in t1_tofixed, from psobjs.c).                              */

    root->units_per_EM = (FT_UShort)( FT_DivFix( 1000 * 0x10000L,
                                                 temp_scale ) >> 16 );

    /* we need to scale the values by 1.0/temp_scale */
    if ( temp_scale != 0x10000L ) {
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

    /* note that the offsets must be expressed in integer font units */
    offset->x = temp[4] >> 16;
    offset->y = temp[5] >> 16;
  }


  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Byte*    cur;
    FT_Byte*    limit  = parser->root.limit;

    PSAux_Service  psaux  = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );
    cur = parser->root.cursor;
    if ( cur >= limit )
    {
      FT_ERROR(( "t42_parse_encoding: out of bounds!\n" ));
      parser->root.error = T42_Err_Invalid_File_Format;
      return;
    }

    /* if we have a number or `[', the encoding is an array, */
    /* and we must load it now                               */
    if ( ft_isdigit( *cur ) || *cur == '[' )
    {
      T1_Encoding  encode          = &face->type1.encoding;
      FT_UInt      count, n;
      PS_Table     char_table      = &loader->encoding_table;
      FT_Memory    memory          = parser->root.memory;
      FT_Error     error;
      FT_Bool      only_immediates = 0;


      /* read the number of entries in the encoding; should be 256 */
      if ( *cur == '[' )
      {
        count           = 256;
        only_immediates = 1;
        parser->root.cursor++;
      }
      else
        count = (FT_UInt)T1_ToInt( parser );

      T1_Skip_Spaces( parser );
      if ( parser->root.cursor >= limit )
        return;

      /* we use a T1_Table to store our charnames */
      loader->num_chars = encode->num_chars = count;
      if ( FT_NEW_ARRAY( encode->char_index, count )     ||
           FT_NEW_ARRAY( encode->char_name,  count )     ||
           FT_SET_ERROR( psaux->ps_table_funcs->init(
                           char_table, count, memory ) ) )
      {
        parser->root.error = error;
        return;
      }

      /* We need to `zero' out encoding_table.elements */
      for ( n = 0; n < count; n++ )
      {
        char*  notdef = (char *)".notdef";


        T1_Add_Table( char_table, n, notdef, 8 );
      }

      /* Now we need to read records of the form                */
      /*                                                        */
      /*   ... charcode /charname ...                           */
      /*                                                        */
      /* for each entry in our table.                           */
      /*                                                        */
      /* We simply look for a number followed by an immediate   */
      /* name.  Note that this ignores correctly the sequence   */
      /* that is often seen in type42 fonts:                    */
      /*                                                        */
      /*   0 1 255 { 1 index exch /.notdef put } for dup        */
      /*                                                        */
      /* used to clean the encoding array before anything else. */
      /*                                                        */
      /* Alternatively, if the array is directly given as       */
      /*                                                        */
      /*   /Encoding [ ... ]                                    */
      /*                                                        */
      /* we only read immediates.                               */

      n = 0;
      T1_Skip_Spaces( parser );

      while ( parser->root.cursor < limit )
      {
        cur = parser->root.cursor;

        /* we stop when we encounter `def' or `]' */
        if ( *cur == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e'          &&
               cur[2] == 'f'          &&
               t42_is_space( cur[3] ) )
          {
            FT_TRACE6(( "encoding end\n" ));
            cur += 3;
            break;
          }
        }
        if ( *cur == ']' )
        {
          FT_TRACE6(( "encoding end\n" ));
          cur++;
          break;
        }

        /* check whether we've found an entry */
        if ( ft_isdigit( *cur ) || only_immediates )
        {
          FT_Int  charcode;


          if ( only_immediates )
            charcode = n;
          else
          {
            charcode = (FT_Int)T1_ToInt( parser );
            T1_Skip_Spaces( parser );
          }

          cur = parser->root.cursor;

          if ( *cur == '/' && cur + 2 < limit && n < count )
          {
            FT_PtrDist  len;


            cur++;

            parser->root.cursor = cur;
            T1_Skip_PS_Token( parser );
            if ( parser->root.error )
              return;

            len = parser->root.cursor - cur;

            parser->root.error = T1_Add_Table( char_table, charcode,
                                               cur, len + 1 );
            if ( parser->root.error )
              return;
            char_table->elements[charcode][len] = '\0';

            n++;
          }
        }
        else
          T1_Skip_PS_Token( parser );

        T1_Skip_Spaces( parser );
      }

      face->type1.encoding_type  = T1_ENCODING_TYPE_ARRAY;
      parser->root.cursor        = cur;
    }

    /* Otherwise, we should have either `StandardEncoding', */
    /* `ExpertEncoding', or `ISOLatin1Encoding'             */
    else
    {
      if ( cur + 17 < limit                                            &&
           ft_strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_STANDARD;

      else if ( cur + 15 < limit                                          &&
                ft_strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_EXPERT;

      else if ( cur + 18 < limit                                             &&
                ft_strncmp( (const char*)cur, "ISOLatin1Encoding", 17 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_ISOLATIN1;

      else
      {
        FT_ERROR(( "t42_parse_encoding: invalid token!\n" ));
        parser->root.error = T42_Err_Invalid_File_Format;
      }
    }
  }


  typedef enum
  {
    BEFORE_START,
    BEFORE_TABLE_DIR,
    OTHER_TABLES

  } T42_Load_Status;


  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Memory   memory = parser->root.memory;
    FT_Byte*    cur;
    FT_Byte*    limit  = parser->root.limit;
    FT_Error    error;
    FT_Int      num_tables = 0;
    FT_ULong    count, ttf_size = 0;

    FT_Long     n, string_size, old_string_size, real_size;
    FT_Byte*    string_buf = NULL;
    FT_Bool     alloc      = 0;

    T42_Load_Status  status;


    /* The format is                                */
    /*                                              */
    /*   /sfnts [ <hexstring> <hexstring> ... ] def */
    /*                                              */
    /* or                                           */
    /*                                              */
    /*   /sfnts [                                   */
    /*      <num_bin_bytes> RD <binary data>        */
    /*      <num_bin_bytes> RD <binary data>        */
    /*      ...                                     */
    /*   ] def                                      */
    /*                                              */
    /* with exactly one space after the `RD' token. */

    T1_Skip_Spaces( parser );

    if ( parser->root.cursor >= limit || *parser->root.cursor++ != '[' )
    {
      FT_ERROR(( "t42_parse_sfnts: can't find begin of sfnts vector!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    T1_Skip_Spaces( parser );
    status          = BEFORE_START;
    string_size     = 0;
    old_string_size = 0;
    count           = 0;

    while ( parser->root.cursor < limit )
    {
      cur = parser->root.cursor;

      if ( *cur == ']' )
      {
        parser->root.cursor++;
        goto Exit;
      }

      else if ( *cur == '<' )
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        /* don't include delimiters */
        string_size = (FT_Long)( ( parser->root.cursor - cur - 2 + 1 ) / 2 );
        if ( FT_REALLOC( string_buf, old_string_size, string_size ) )
          goto Fail;

        alloc = 1;

        parser->root.cursor = cur;
        (void)T1_ToBytes( parser, string_buf, string_size, &real_size, 1 );
        old_string_size = string_size;
        string_size = real_size;
      }

      else if ( ft_isdigit( *cur ) )
      {
        string_size = T1_ToInt( parser );

        T1_Skip_PS_Token( parser );             /* `RD' */
        if ( parser->root.error )
          return;

        string_buf = parser->root.cursor + 1;   /* one space after `RD' */

        parser->root.cursor += string_size + 1;
        if ( parser->root.cursor >= limit )
        {
          FT_ERROR(( "t42_parse_sfnts: too many binary data!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }
      }

      /* A string can have a trailing zero byte for padding.  Ignore it. */
      if ( string_buf[string_size - 1] == 0 && ( string_size % 2 == 1 ) )
        string_size--;

      for ( n = 0; n < string_size; n++ )
      {
        switch ( status )
        {
        case BEFORE_START:
          /* load offset table, 12 bytes */
          if ( count < 12 )
          {
            face->ttf_data[count++] = string_buf[n];
            continue;
          }
          else
          {
            num_tables = 16 * face->ttf_data[4] + face->ttf_data[5];
            status     = BEFORE_TABLE_DIR;
            ttf_size   = 12 + 16 * num_tables;

            if ( FT_REALLOC( face->ttf_data, 12, ttf_size ) )
              goto Fail;
          }
          /* fall through */

        case BEFORE_TABLE_DIR:
          /* the offset table is read; read the table directory */
          if ( count < ttf_size )
          {
            face->ttf_data[count++] = string_buf[n];
            continue;
          }
          else
          {
            int       i;
            FT_ULong  len;


            for ( i = 0; i < num_tables; i++ )
            {
              FT_Byte*  p = face->ttf_data + 12 + 16 * i + 12;


              len = FT_PEEK_ULONG( p );

              /* Pad to a 4-byte boundary length */
              ttf_size += ( len + 3 ) & ~3;
            }

            status         = OTHER_TABLES;
            face->ttf_size = ttf_size;

            /* there are no more than 256 tables, so no size check here */
            if ( FT_REALLOC( face->ttf_data, 12 + 16 * num_tables,
                             ttf_size + 1 ) )
              goto Fail;
          }
          /* fall through */

        case OTHER_TABLES:
          /* all other tables are just copied */
          if ( count >= ttf_size )
          {
            FT_ERROR(( "t42_parse_sfnts: too many binary data!\n" ));
            error = T42_Err_Invalid_File_Format;
            goto Fail;
          }
          face->ttf_data[count++] = string_buf[n];
        }
      }

      T1_Skip_Spaces( parser );
    }

    /* if control reaches this point, the format was not valid */
    error = T42_Err_Invalid_File_Format;

  Fail:
    parser->root.error = error;

  Exit:
    if ( alloc )
      FT_FREE( string_buf );
  }


  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser     parser       = &loader->parser;
    PS_Table       code_table   = &loader->charstrings;
    PS_Table       name_table   = &loader->glyph_names;
    PS_Table       swap_table   = &loader->swap_table;
    FT_Memory      memory       = parser->root.memory;
    FT_Error       error;

    PSAux_Service  psaux        = (PSAux_Service)face->psaux;

    FT_Byte*       cur;
    FT_Byte*       limit        = parser->root.limit;
    FT_UInt        n;
    FT_UInt        notdef_index = 0;
    FT_Byte        notdef_found = 0;


    T1_Skip_Spaces( parser );

    if ( parser->root.cursor >= limit )
    {
      FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( ft_isdigit( *parser->root.cursor ) )
    {
      loader->num_glyphs = (FT_UInt)T1_ToInt( parser );
      if ( parser->root.error )
        return;
    }
    else if ( *parser->root.cursor == '<' )
    {
      /* We have `<< ... >>'.  Count the number of `/' in the dictionary */
      /* to get its size.                                                */
      FT_UInt  count = 0;


      T1_Skip_PS_Token( parser );
      if ( parser->root.error )
        return;
      T1_Skip_Spaces( parser );
      cur = parser->root.cursor;

      while ( parser->root.cursor < limit )
      {
        if ( *parser->root.cursor == '/' )
          count++;
        else if ( *parser->root.cursor == '>' )
        {
          loader->num_glyphs  = count;
          parser->root.cursor = cur;        /* rewind */
          break;
        }
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          return;
        T1_Skip_Spaces( parser );
      }
    }
    else
    {
      FT_ERROR(( "t42_parse_charstrings: invalid token!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( parser->root.cursor >= limit )
    {
      FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    /* initialize tables */

    error = psaux->ps_table_funcs->init( code_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    error = psaux->ps_table_funcs->init( name_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    /* Initialize table for swapping index notdef_index and */
    /* index 0 names and codes (if necessary).              */

    error = psaux->ps_table_funcs->init( swap_table, 4, memory );
    if ( error )
      goto Fail;

    n = 0;

    for (;;)
    {
      /* The format is simple:                   */
      /*   `/glyphname' + index [+ def]          */

      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      /* We stop when we find an `end' keyword or '>' */
      if ( *cur   == 'e'          &&
           cur + 3 < limit        &&
           cur[1] == 'n'          &&
           cur[2] == 'd'          &&
           t42_is_space( cur[3] ) )
        break;
      if ( *cur == '>' )
        break;

      T1_Skip_PS_Token( parser );
      if ( parser->root.error )
        return;

      if ( *cur == '/' )
      {
        FT_PtrDist  len;


        if ( cur + 1 >= limit )
        {
          FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        cur++;                              /* skip `/' */
        len = parser->root.cursor - cur;

        error = T1_Add_Table( name_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        /* add a trailing zero to the name table */
        name_table->elements[n][len] = '\0';

        /* record index of /.notdef */
        if ( *cur == '.'                                              &&
             ft_strcmp( ".notdef",
                        (const char*)(name_table->elements[n]) ) == 0 )
        {
          notdef_index = n;
          notdef_found = 1;
        }

        T1_Skip_Spaces( parser );

        cur = parser->root.cursor;

        (void)T1_ToInt( parser );
        if ( parser->root.cursor >= limit )
        {
          FT_ERROR(( "t42_parse_charstrings: out of bounds!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        len = parser->root.cursor - cur;

        error = T1_Add_Table( code_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        code_table->elements[n][len] = '\0';

        n++;
        if ( n >= loader->num_glyphs )
          break;
      }
    }

    loader->num_glyphs = n;

    if ( !notdef_found )
    {
      FT_ERROR(( "t42_parse_charstrings: no /.notdef glyph!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    /* if /.notdef does not occupy index 0, do our magic. */
    if ( ft_strcmp( (const char*)".notdef",
                    (const char*)name_table->elements[0] ) )
    {
      /* Swap glyph in index 0 with /.notdef glyph.  First, add index 0  */
      /* name and code entries to swap_table.  Then place notdef_index   */
      /* name and code entries into swap_table.  Then swap name and code */
      /* entries at indices notdef_index and 0 using values stored in    */
      /* swap_table.                                                     */

      /* Index 0 name */
      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index 0 code */
      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index notdef_index name */
      error = T1_Add_Table( swap_table, 2,
                            name_table->elements[notdef_index],
                            name_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      /* Index notdef_index code */
      error = T1_Add_Table( swap_table, 3,
                            code_table->elements[notdef_index],
                            code_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, notdef_index,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, notdef_index,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0,
                            swap_table->elements[2],
                            swap_table->lengths [2] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0,
                            swap_table->elements[3],
                            swap_table->lengths [3] );
      if ( error )
        goto Fail;

    }

    return;

  Fail:
    parser->root.error = error;
  }


  static FT_Error
  t42_load_keyword( T42_Face    face,
                    T42_Loader  loader,
                    T1_Field    field )
  {
    FT_Error  error;
    void*     dummy_object;
    void**    objects;
    FT_UInt   max_objects = 0;


    /* if the keyword has a dedicated callback, call it */
    if ( field->type == T1_FIELD_TYPE_CALLBACK )
    {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    /* now the keyword is either a simple field or a table of fields; */
    /* we are now going to take care of it                            */

    switch ( field->location )
    {
    case T1_FIELD_LOCATION_FONT_INFO:
      dummy_object = &face->type1.font_info;
      break;

    case T1_FIELD_LOCATION_BBOX:
      dummy_object = &face->type1.font_bbox;
      break;

    default:
      dummy_object = &face->type1;
    }

    objects = &dummy_object;

    if ( field->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         field->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = T1_Load_Field_Table( &loader->parser, field,
                                   objects, max_objects, 0 );
    else
      error = T1_Load_Field( &loader->parser, field,
                             objects, max_objects, 0 );

   Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  t42_parse_dict( T42_Face    face,
                  T42_Loader  loader,
                  FT_Byte*    base,
                  FT_Long     size )
  {
    T42_Parser  parser     = &loader->parser;
    FT_Byte*    limit;
    FT_Int      n_keywords = (FT_Int)( sizeof ( t42_keywords ) /
                                         sizeof ( t42_keywords[0] ) );


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = T42_Err_Ok;

    limit = parser->root.limit;

    T1_Skip_Spaces( parser );

    while ( parser->root.cursor < limit )
    {
      FT_Byte*  cur;


      cur = parser->root.cursor;

      /* look for `FontDirectory' which causes problems for some fonts */
      if ( *cur == 'F' && cur + 25 < limit                    &&
           ft_strncmp( (char*)cur, "FontDirectory", 13 ) == 0 )
      {
        FT_Byte*  cur2;


        /* skip the `FontDirectory' keyword */
        T1_Skip_PS_Token( parser );
        T1_Skip_Spaces  ( parser );
        cur = cur2 = parser->root.cursor;

        /* look up the `known' keyword */
        while ( cur < limit )
        {
          if ( *cur == 'k' && cur + 5 < limit             &&
                ft_strncmp( (char*)cur, "known", 5 ) == 0 )
            break;

          T1_Skip_PS_Token( parser );
          if ( parser->root.error )
            goto Exit;
          T1_Skip_Spaces  ( parser );
          cur = parser->root.cursor;
        }

        if ( cur < limit )
        {
          T1_TokenRec  token;


          /* skip the `known' keyword and the token following it */
          T1_Skip_PS_Token( parser );
          T1_ToToken( parser, &token );

          /* if the last token was an array, skip it! */
          if ( token.type == T1_TOKEN_TYPE_ARRAY )
            cur2 = parser->root.cursor;
        }
        parser->root.cursor = cur2;
      }

      /* look for immediates */
      else if ( *cur == '/' && cur + 2 < limit )
      {
        FT_PtrDist  len;


        cur++;

        parser->root.cursor = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        len = parser->root.cursor - cur;

        if ( len > 0 && len < 22 && parser->root.cursor < limit )
        {
          int  i;


          /* now compare the immediate name to the keyword table */

          /* loop through all known keywords */
          for ( i = 0; i < n_keywords; i++ )
          {
            T1_Field  keyword = (T1_Field)&t42_keywords[i];
            FT_Byte   *name   = (FT_Byte*)keyword->ident;


            if ( !name )
              continue;

            if ( cur[0] == name[0]                                  &&
                 len == (FT_PtrDist)ft_strlen( (const char *)name ) &&
                 ft_memcmp( cur, name, len ) == 0                   )
            {
              /* we found it -- run the parsing callback! */
              parser->root.error = t42_load_keyword( face,
                                                     loader,
                                                     keyword );
              if ( parser->root.error )
                return parser->root.error;
              break;
            }
          }
        }
      }
      else
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
      }

      T1_Skip_Spaces( parser );
    }

  Exit:
    return parser->root.error;
  }


  FT_LOCAL_DEF( void )
  t42_loader_init( T42_Loader  loader,
                   T42_Face    face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    /* initialize the tables -- simply set their `init' field to 0 */
    loader->encoding_table.init = 0;
    loader->charstrings.init    = 0;
    loader->glyph_names.init    = 0;
  }


  FT_LOCAL_DEF( void )
  t42_loader_done( T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;


    /* finalize tables */
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );
    T1_Release_Table( &loader->swap_table );

    /* finalize parser */
    t42_parser_done( parser );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  t42drivr.c                                                             */
/*                                                                         */
/*    High-level Type 42 driver interface (body).                          */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2006 by Roberto Alameda.                   */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This driver implements Type42 fonts as described in the               */
  /* Technical Note #5012 from Adobe, with these limitations:              */
  /*                                                                       */
  /* 1) CID Fonts are not currently supported.                             */
  /* 2) Incremental fonts making use of the GlyphDirectory keyword         */
  /*    will be loaded, but the rendering will be using the TrueType       */
  /*    tables.                                                            */
  /* 3) As for Type1 fonts, CDevProc is not supported.                     */
  /* 4) The Metrics dictionary is not supported.                           */
  /* 5) AFM metrics are not supported.                                     */
  /*                                                                       */
  /* In other words, this driver supports Type42 fonts derived from        */
  /* TrueType fonts in a non-CID manner, as done by usual conversion       */
  /* programs.                                                             */
  /*                                                                       */
  /*************************************************************************/


#include "t42drivr.h"
#include "t42objs.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H

#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_GLYPH_DICT_H
#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_POSTSCRIPT_INFO_H

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


 /*
  *
  *  GLYPH DICT SERVICE
  *
  */

  static FT_Error
  t42_get_glyph_name( T42_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    FT_String*  gname;


    gname = face->type1.glyph_names[glyph_index];

    if ( buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)( ft_strlen( gname ) );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    return T42_Err_Ok;
  }


  static FT_UInt
  t42_get_name_index( T42_Face    face,
                      FT_String*  glyph_name )
  {
    FT_Int      i;
    FT_String*  gname;


    for ( i = 0; i < face->type1.num_glyphs; i++ )
    {
      gname = face->type1.glyph_names[i];

      if ( !ft_strcmp( glyph_name, gname ) )
        return (FT_UInt)ft_atol( (const char *)face->type1.charstrings[i] );
    }

    return 0;
  }


  static const FT_Service_GlyphDictRec  t42_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  t42_get_glyph_name,
    (FT_GlyphDict_NameIndexFunc)t42_get_name_index
  };


 /*
  *
  *  POSTSCRIPT NAME SERVICE
  *
  */

  static const char*
  t42_get_ps_font_name( T42_Face  face )
  {
    return (const char*)face->type1.font_name;
  }


  static const FT_Service_PsFontNameRec  t42_service_ps_font_name =
  {
    (FT_PsName_GetFunc)t42_get_ps_font_name
  };


 /*
  *
  *  POSTSCRIPT INFO SERVICE
  *
  */

  static FT_Error
  t42_ps_get_font_info( FT_Face          face,
                        PS_FontInfoRec*  afont_info )
  {
    *afont_info = ((T42_Face)face)->type1.font_info;
    return T42_Err_Ok;
  }


  static FT_Int
  t42_ps_has_glyph_names( FT_Face  face )
  {
    FT_UNUSED( face );
    return 1;
  }


  static FT_Error
  t42_ps_get_font_private( FT_Face         face,
                           PS_PrivateRec*  afont_private )
  {
    *afont_private = ((T42_Face)face)->type1.private_dict;
    return T42_Err_Ok;
  }


  static const FT_Service_PsInfoRec  t42_service_ps_info =
  {
    (PS_GetFontInfoFunc)   t42_ps_get_font_info,
    (PS_HasGlyphNamesFunc) t42_ps_has_glyph_names,
    (PS_GetFontPrivateFunc)t42_ps_get_font_private
  };


 /*
  *
  *  SERVICE LIST
  *
  */

  static const FT_ServiceDescRec  t42_services[] =
  {
    { FT_SERVICE_ID_GLYPH_DICT,           &t42_service_glyph_dict },
    { FT_SERVICE_ID_POSTSCRIPT_FONT_NAME, &t42_service_ps_font_name },
    { FT_SERVICE_ID_POSTSCRIPT_INFO,      &t42_service_ps_info },
    { FT_SERVICE_ID_XF86_NAME,            FT_XF86_FORMAT_TYPE_42 },
    { NULL, NULL }
  };


  static FT_Module_Interface
  T42_Get_Interface( FT_Driver         driver,
                     const FT_String*  t42_interface )
  {
    FT_UNUSED( driver );

    return ft_service_list_lookup( t42_services, t42_interface );
  }


  const FT_Driver_ClassRec  t42_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      FT_MODULE_DRIVER_HAS_HINTER,
#else
      0,
#endif

      sizeof ( T42_DriverRec ),

      "type42",
      0x10000L,
      0x20000L,

      0,    /* format interface */

      (FT_Module_Constructor)T42_Driver_Init,
      (FT_Module_Destructor) T42_Driver_Done,
      (FT_Module_Requester)  T42_Get_Interface,
    },

    sizeof ( T42_FaceRec ),
    sizeof ( T42_SizeRec ),
    sizeof ( T42_GlyphSlotRec ),

    (FT_Face_InitFunc)        T42_Face_Init,
    (FT_Face_DoneFunc)        T42_Face_Done,
    (FT_Size_InitFunc)        T42_Size_Init,
    (FT_Size_DoneFunc)        T42_Size_Done,
    (FT_Slot_InitFunc)        T42_GlyphSlot_Init,
    (FT_Slot_DoneFunc)        T42_GlyphSlot_Done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif
    (FT_Slot_LoadFunc)        T42_GlyphSlot_Load,

    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,

    (FT_Face_GetAdvancesFunc) 0,
    (FT_Size_RequestFunc)     T42_Size_Request,
    (FT_Size_SelectFunc)      T42_Size_Select
  };


/* END */


/* END */

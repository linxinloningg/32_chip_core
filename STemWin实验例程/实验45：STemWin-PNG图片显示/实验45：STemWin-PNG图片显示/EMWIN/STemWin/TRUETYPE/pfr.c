/***************************************************************************/
/*                                                                         */
/*  pfr.c                                                                  */
/*                                                                         */
/*    FreeType PFR driver component.                                       */
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
/*  pfrload.c                                                              */
/*                                                                         */
/*    FreeType PFR loader (body).                                          */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "pfrload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "pfrerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pfr


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          EXTRA ITEMS                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  pfr_extra_items_skip( FT_Byte*  *pp,
                        FT_Byte*   limit )
  {
    return pfr_extra_items_parse( pp, limit, NULL, NULL );
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_extra_items_parse( FT_Byte*       *pp,
                         FT_Byte*        limit,
                         PFR_ExtraItem   item_list,
                         FT_Pointer      item_data )
  {
    FT_Error  error = 0;
    FT_Byte*  p     = *pp;
    FT_UInt   num_items, item_type, item_size;


    PFR_CHECK( 1 );
    num_items = PFR_NEXT_BYTE( p );

    for ( ; num_items > 0; num_items-- )
    {
      PFR_CHECK( 2 );
      item_size = PFR_NEXT_BYTE( p );
      item_type = PFR_NEXT_BYTE( p );

      PFR_CHECK( item_size );

      if ( item_list )
      {
        PFR_ExtraItem  extra = item_list;


        for ( extra = item_list; extra->parser != NULL; extra++ )
        {
          if ( extra->type == item_type )
          {
            error = extra->parser( p, p + item_size, item_data );
            if ( error ) goto Exit;

            break;
          }
        }
      }

      p += item_size;
    }

  Exit:
    *pp = p;
    return error;

  Too_Short:
    FT_ERROR(( "pfr_extra_items_parse: invalid extra items table\n" ));
    error = PFR_Err_Invalid_Table;
    goto Exit;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          PFR HEADER                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

   static const FT_Frame_Field  pfr_header_fields[] =
   {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PFR_HeaderRec

     FT_FRAME_START( 58 ),
       FT_FRAME_ULONG ( signature ),
       FT_FRAME_USHORT( version ),
       FT_FRAME_USHORT( signature2 ),
       FT_FRAME_USHORT( header_size ),

       FT_FRAME_USHORT( log_dir_size ),
       FT_FRAME_USHORT( log_dir_offset ),

       FT_FRAME_USHORT( log_font_max_size ),
       FT_FRAME_UOFF3 ( log_font_section_size ),
       FT_FRAME_UOFF3 ( log_font_section_offset ),

       FT_FRAME_USHORT( phy_font_max_size ),
       FT_FRAME_UOFF3 ( phy_font_section_size ),
       FT_FRAME_UOFF3 ( phy_font_section_offset ),

       FT_FRAME_USHORT( gps_max_size ),
       FT_FRAME_UOFF3 ( gps_section_size ),
       FT_FRAME_UOFF3 ( gps_section_offset ),

       FT_FRAME_BYTE  ( max_blue_values ),
       FT_FRAME_BYTE  ( max_x_orus ),
       FT_FRAME_BYTE  ( max_y_orus ),

       FT_FRAME_BYTE  ( phy_font_max_size_high ),
       FT_FRAME_BYTE  ( color_flags ),

       FT_FRAME_UOFF3 ( bct_max_size ),
       FT_FRAME_UOFF3 ( bct_set_max_size ),
       FT_FRAME_UOFF3 ( phy_bct_set_max_size ),

       FT_FRAME_USHORT( num_phy_fonts ),
       FT_FRAME_BYTE  ( max_vert_stem_snap ),
       FT_FRAME_BYTE  ( max_horz_stem_snap ),
       FT_FRAME_USHORT( max_chars ),
     FT_FRAME_END
   };


  FT_LOCAL_DEF( FT_Error )
  pfr_header_load( PFR_Header  header,
                   FT_Stream   stream )
  {
    FT_Error  error;


    /* read header directly */
    if ( !FT_STREAM_SEEK( 0 )                                &&
         !FT_STREAM_READ_FIELDS( pfr_header_fields, header ) )
    {
      /* make a few adjustments to the header */
      header->phy_font_max_size +=
        (FT_UInt32)header->phy_font_max_size_high << 16;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Bool )
  pfr_header_check( PFR_Header  header )
  {
    FT_Bool  result = 1;


    /* check signature and header size */
    if ( header->signature  != 0x50465230L ||   /* "PFR0" */
         header->version     > 4           ||
         header->header_size < 58          ||
         header->signature2 != 0x0d0a      )    /* CR/LF  */
    {
      result = 0;
    }
    return  result;
  }


  /***********************************************************************/
  /***********************************************************************/
  /*****                                                             *****/
  /*****                    PFR LOGICAL FONTS                        *****/
  /*****                                                             *****/
  /***********************************************************************/
  /***********************************************************************/


  FT_LOCAL_DEF( FT_Error )
  pfr_log_font_count( FT_Stream  stream,
                      FT_UInt32  section_offset,
                      FT_UInt   *acount )
  {
    FT_Error  error;
    FT_UInt   count;
    FT_UInt   result = 0;


    if ( FT_STREAM_SEEK( section_offset ) || FT_READ_USHORT( count ) )
      goto Exit;

    result = count;

  Exit:
    *acount = result;
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_log_font_load( PFR_LogFont  log_font,
                     FT_Stream    stream,
                     FT_UInt      idx,
                     FT_UInt32    section_offset,
                     FT_Bool      size_increment )
  {
    FT_UInt    num_log_fonts;
    FT_UInt    flags;
    FT_UInt32  offset;
    FT_UInt32  size;
    FT_Error   error;


    if ( FT_STREAM_SEEK( section_offset ) ||
         FT_READ_USHORT( num_log_fonts )  )
      goto Exit;

    if ( idx >= num_log_fonts )
      return PFR_Err_Invalid_Argument;

    if ( FT_STREAM_SKIP( idx * 5 ) ||
         FT_READ_USHORT( size )    ||
         FT_READ_UOFF3 ( offset )  )
      goto Exit;

    /* save logical font size and offset */
    log_font->size   = size;
    log_font->offset = offset;

    /* now, check the rest of the table before loading it */
    {
      FT_Byte*  p;
      FT_Byte*  limit;
      FT_UInt   local;


      if ( FT_STREAM_SEEK( offset ) || FT_FRAME_ENTER( size ) )
        goto Exit;

      p     = stream->cursor;
      limit = p + size;

      PFR_CHECK(13);

      log_font->matrix[0] = PFR_NEXT_LONG( p );
      log_font->matrix[1] = PFR_NEXT_LONG( p );
      log_font->matrix[2] = PFR_NEXT_LONG( p );
      log_font->matrix[3] = PFR_NEXT_LONG( p );

      flags = PFR_NEXT_BYTE( p );

      local = 0;
      if ( flags & PFR_LOG_STROKE )
      {
        local++;
        if ( flags & PFR_LOG_2BYTE_STROKE )
          local++;

        if ( (flags & PFR_LINE_JOIN_MASK) == PFR_LINE_JOIN_MITER )
          local += 3;
      }
      if ( flags & PFR_LOG_BOLD )
      {
        local++;
        if ( flags & PFR_LOG_2BYTE_BOLD )
          local++;
      }

      PFR_CHECK( local );

      if ( flags & PFR_LOG_STROKE )
      {
        log_font->stroke_thickness = ( flags & PFR_LOG_2BYTE_STROKE )
                                     ? PFR_NEXT_SHORT( p )
                                     : PFR_NEXT_BYTE( p );

        if ( ( flags & PFR_LINE_JOIN_MASK ) == PFR_LINE_JOIN_MITER )
          log_font->miter_limit = PFR_NEXT_LONG( p );
      }

      if ( flags & PFR_LOG_BOLD )
      {
        log_font->bold_thickness = ( flags & PFR_LOG_2BYTE_BOLD )
                                   ? PFR_NEXT_SHORT( p )
                                   : PFR_NEXT_BYTE( p );
      }

      if ( flags & PFR_LOG_EXTRA_ITEMS )
      {
        error = pfr_extra_items_skip( &p, limit );
        if (error) goto Fail;
      }

      PFR_CHECK(5);
      log_font->phys_size   = PFR_NEXT_USHORT( p );
      log_font->phys_offset = PFR_NEXT_ULONG( p );
      if ( size_increment )
      {
        PFR_CHECK( 1 );
        log_font->phys_size += (FT_UInt32)PFR_NEXT_BYTE( p ) << 16;
      }
    }

  Fail:
    FT_FRAME_EXIT();

  Exit:
    return error;

  Too_Short:
    FT_ERROR(( "pfr_log_font_load: invalid logical font table\n" ));
    error = PFR_Err_Invalid_Table;
    goto Fail;
  }


  /***********************************************************************/
  /***********************************************************************/
  /*****                                                             *****/
  /*****                    PFR PHYSICAL FONTS                       *****/
  /*****                                                             *****/
  /***********************************************************************/
  /***********************************************************************/


  /* load bitmap strikes lists */
  FT_CALLBACK_DEF( FT_Error )
  pfr_extra_item_load_bitmap_info( FT_Byte*     p,
                                   FT_Byte*     limit,
                                   PFR_PhyFont  phy_font )
  {
    FT_Memory   memory = phy_font->memory;
    PFR_Strike  strike;
    FT_UInt     flags0;
    FT_UInt     n, count, size1;
    FT_Error    error = 0;


    PFR_CHECK( 5 );

    p += 3;  /* skip bctSize */
    flags0 = PFR_NEXT_BYTE( p );
    count  = PFR_NEXT_BYTE( p );

    /* re-allocate when needed */
    if ( phy_font->num_strikes + count > phy_font->max_strikes )
    {
      FT_UInt  new_max = FT_PAD_CEIL( phy_font->num_strikes + count, 4 );


      if ( FT_RENEW_ARRAY( phy_font->strikes,
                           phy_font->num_strikes,
                           new_max ) )
        goto Exit;

      phy_font->max_strikes = new_max;
    }

    size1 = 1 + 1 + 1 + 2 + 2 + 1;
    if ( flags0 & PFR_STRIKE_2BYTE_XPPM )
      size1++;

    if ( flags0 & PFR_STRIKE_2BYTE_YPPM )
      size1++;

    if ( flags0 & PFR_STRIKE_3BYTE_SIZE )
      size1++;

    if ( flags0 & PFR_STRIKE_3BYTE_OFFSET )
      size1++;

    if ( flags0 & PFR_STRIKE_2BYTE_COUNT )
      size1++;

    strike = phy_font->strikes + phy_font->num_strikes;

    PFR_CHECK( count * size1 );

    for ( n = 0; n < count; n++, strike++ )
    {
      strike->x_ppm       = ( flags0 & PFR_STRIKE_2BYTE_XPPM )
                            ? PFR_NEXT_USHORT( p )
                            : PFR_NEXT_BYTE( p );

      strike->y_ppm       = ( flags0 & PFR_STRIKE_2BYTE_YPPM )
                            ? PFR_NEXT_USHORT( p )
                            : PFR_NEXT_BYTE( p );

      strike->flags       = PFR_NEXT_BYTE( p );

      strike->bct_size    = ( flags0 & PFR_STRIKE_3BYTE_SIZE )
                            ? PFR_NEXT_ULONG( p )
                            : PFR_NEXT_USHORT( p );

      strike->bct_offset  = ( flags0 & PFR_STRIKE_3BYTE_OFFSET )
                            ? PFR_NEXT_ULONG( p )
                            : PFR_NEXT_USHORT( p );

      strike->num_bitmaps = ( flags0 & PFR_STRIKE_2BYTE_COUNT )
                            ? PFR_NEXT_USHORT( p )
                            : PFR_NEXT_BYTE( p );
    }

    phy_font->num_strikes += count;

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_extra_item_load_bitmap_info: invalid bitmap info table\n" ));
    goto Exit;
  }


  /* Load font ID.  This is a so-called "unique" name that is rather
   * long and descriptive (like "Tiresias ScreenFont v7.51").
   *
   * Note that a PFR font's family name is contained in an *undocumented*
   * string of the "auxiliary data" portion of a physical font record.  This
   * may also contain the "real" style name!
   *
   * If no family name is present, the font ID is used instead for the
   * family.
   */
  FT_CALLBACK_DEF( FT_Error )
  pfr_extra_item_load_font_id( FT_Byte*     p,
                               FT_Byte*     limit,
                               PFR_PhyFont  phy_font )
  {
    FT_Error    error  = 0;
    FT_Memory   memory = phy_font->memory;
    FT_PtrDist  len    = limit - p;


    if ( phy_font->font_id != NULL )
      goto Exit;

    if ( FT_ALLOC( phy_font->font_id, len + 1 ) )
      goto Exit;

    /* copy font ID name, and terminate it for safety */
    FT_MEM_COPY( phy_font->font_id, p, len );
    phy_font->font_id[len] = 0;

  Exit:
    return error;
  }


  /* load stem snap tables */
  FT_CALLBACK_DEF( FT_Error )
  pfr_extra_item_load_stem_snaps( FT_Byte*     p,
                                  FT_Byte*     limit,
                                  PFR_PhyFont  phy_font )
  {
    FT_UInt    count, num_vert, num_horz;
    FT_Int*    snaps;
    FT_Error   error  = 0;
    FT_Memory  memory = phy_font->memory;


    if ( phy_font->vertical.stem_snaps != NULL )
      goto Exit;

    PFR_CHECK( 1 );
    count = PFR_NEXT_BYTE( p );

    num_vert = count & 15;
    num_horz = count >> 4;
    count    = num_vert + num_horz;

    PFR_CHECK( count * 2 );

    if ( FT_NEW_ARRAY( snaps, count ) )
      goto Exit;

    phy_font->vertical.stem_snaps = snaps;
    phy_font->horizontal.stem_snaps = snaps + num_vert;

    for ( ; count > 0; count--, snaps++ )
      *snaps = FT_NEXT_SHORT( p );

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_exta_item_load_stem_snaps: invalid stem snaps table\n" ));
    goto Exit;
  }



  /* load kerning pair data */
  FT_CALLBACK_DEF( FT_Error )
  pfr_extra_item_load_kerning_pairs( FT_Byte*     p,
                                     FT_Byte*     limit,
                                     PFR_PhyFont  phy_font )
  {
    PFR_KernItem  item;
    FT_Error      error  = 0;
    FT_Memory     memory = phy_font->memory;


    FT_TRACE2(( "pfr_extra_item_load_kerning_pairs()\n" ));

    if ( FT_NEW( item ) )
      goto Exit;

    PFR_CHECK( 4 );

    item->pair_count = PFR_NEXT_BYTE( p );
    item->base_adj   = PFR_NEXT_SHORT( p );
    item->flags      = PFR_NEXT_BYTE( p );
    item->offset     = phy_font->offset + ( p - phy_font->cursor );

#ifndef PFR_CONFIG_NO_CHECKS
    item->pair_size = 3;

    if ( item->flags & PFR_KERN_2BYTE_CHAR )
      item->pair_size += 2;

    if ( item->flags & PFR_KERN_2BYTE_ADJ )
      item->pair_size += 1;

    PFR_CHECK( item->pair_count * item->pair_size );
#endif

    /* load first and last pairs into the item to speed up */
    /* lookup later...                                     */
    if ( item->pair_count > 0 )
    {
      FT_UInt   char1, char2;
      FT_Byte*  q;


      if ( item->flags & PFR_KERN_2BYTE_CHAR )
      {
        q     = p;
        char1 = PFR_NEXT_USHORT( q );
        char2 = PFR_NEXT_USHORT( q );

        item->pair1 = PFR_KERN_INDEX( char1, char2 );

        q = p + item->pair_size * ( item->pair_count - 1 );
        char1 = PFR_NEXT_USHORT( q );
        char2 = PFR_NEXT_USHORT( q );

        item->pair2 = PFR_KERN_INDEX( char1, char2 );
      }
      else
      {
        q     = p;
        char1 = PFR_NEXT_BYTE( q );
        char2 = PFR_NEXT_BYTE( q );

        item->pair1 = PFR_KERN_INDEX( char1, char2 );

        q = p + item->pair_size * ( item->pair_count - 1 );
        char1 = PFR_NEXT_BYTE( q );
        char2 = PFR_NEXT_BYTE( q );

        item->pair2 = PFR_KERN_INDEX( char1, char2 );
      }

      /* add new item to the current list */
      item->next                 = NULL;
      *phy_font->kern_items_tail = item;
      phy_font->kern_items_tail  = &item->next;
      phy_font->num_kern_pairs  += item->pair_count;
    }
    else
    {
      /* empty item! */
      FT_FREE( item );
    }

  Exit:
    return error;

  Too_Short:
    FT_FREE( item );

    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_extra_item_load_kerning_pairs: "
               "invalid kerning pairs table\n" ));
    goto Exit;
  }



  static const PFR_ExtraItemRec  pfr_phy_font_extra_items[] =
  {
    { 1, (PFR_ExtraItem_ParseFunc)pfr_extra_item_load_bitmap_info },
    { 2, (PFR_ExtraItem_ParseFunc)pfr_extra_item_load_font_id },
    { 3, (PFR_ExtraItem_ParseFunc)pfr_extra_item_load_stem_snaps },
    { 4, (PFR_ExtraItem_ParseFunc)pfr_extra_item_load_kerning_pairs },
    { 0, NULL }
  };


  /* Loads a name from the auxiliary data.  Since this extracts undocumented
   * strings from the font file, we need to be careful here.
   */
  static FT_Error
  pfr_aux_name_load( FT_Byte*     p,
                     FT_UInt      len,
                     FT_Memory    memory,
                     FT_String*  *astring )
  {
    FT_Error    error = 0;
    FT_String*  result = NULL;
    FT_UInt     n, ok;


    if ( len > 0 && p[len - 1] == 0 )
      len--;

    /* check that each character is ASCII for making sure not to
       load garbage
     */
    ok = ( len > 0 );
    for ( n = 0; n < len; n++ )
      if ( p[n] < 32 || p[n] > 127 )
      {
        ok = 0;
        break;
      }

    if ( ok )
    {
      if ( FT_ALLOC( result, len + 1 ) )
        goto Exit;

      FT_MEM_COPY( result, p, len );
      result[len] = 0;
    }
  Exit:
    *astring = result;
    return error;
  }


  FT_LOCAL_DEF( void )
  pfr_phy_font_done( PFR_PhyFont  phy_font,
                     FT_Memory    memory )
  {
    FT_FREE( phy_font->font_id );
    FT_FREE( phy_font->family_name );
    FT_FREE( phy_font->style_name );

    FT_FREE( phy_font->vertical.stem_snaps );
    phy_font->vertical.num_stem_snaps = 0;

    phy_font->horizontal.stem_snaps     = NULL;
    phy_font->horizontal.num_stem_snaps = 0;

    FT_FREE( phy_font->strikes );
    phy_font->num_strikes = 0;
    phy_font->max_strikes = 0;

    FT_FREE( phy_font->chars );
    phy_font->num_chars    = 0;
    phy_font->chars_offset = 0;

    FT_FREE( phy_font->blue_values );
    phy_font->num_blue_values = 0;

    {
      PFR_KernItem  item, next;


      item = phy_font->kern_items;
      while ( item )
      {
        next = item->next;
        FT_FREE( item );
        item = next;
      }
      phy_font->kern_items      = NULL;
      phy_font->kern_items_tail = NULL;
    }

    phy_font->num_kern_pairs = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_phy_font_load( PFR_PhyFont  phy_font,
                     FT_Stream    stream,
                     FT_UInt32    offset,
                     FT_UInt32    size )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_UInt    flags, num_aux;
    FT_Byte*   p;
    FT_Byte*   limit;


    phy_font->memory = memory;
    phy_font->offset = offset;

    phy_font->kern_items      = NULL;
    phy_font->kern_items_tail = &phy_font->kern_items;

    if ( FT_STREAM_SEEK( offset ) || FT_FRAME_ENTER( size ) )
      goto Exit;

    phy_font->cursor = stream->cursor;

    p     = stream->cursor;
    limit = p + size;

    PFR_CHECK( 15 );
    phy_font->font_ref_number    = PFR_NEXT_USHORT( p );
    phy_font->outline_resolution = PFR_NEXT_USHORT( p );
    phy_font->metrics_resolution = PFR_NEXT_USHORT( p );
    phy_font->bbox.xMin          = PFR_NEXT_SHORT( p );
    phy_font->bbox.yMin          = PFR_NEXT_SHORT( p );
    phy_font->bbox.xMax          = PFR_NEXT_SHORT( p );
    phy_font->bbox.yMax          = PFR_NEXT_SHORT( p );
    phy_font->flags      = flags = PFR_NEXT_BYTE( p );

    /* get the standard advance for non-proprotional fonts */
    if ( !(flags & PFR_PHY_PROPORTIONAL) )
    {
      PFR_CHECK( 2 );
      phy_font->standard_advance = PFR_NEXT_SHORT( p );
    }

    /* load the extra items when present */
    if ( flags & PFR_PHY_EXTRA_ITEMS )
    {
      error =  pfr_extra_items_parse( &p, limit,
                                      pfr_phy_font_extra_items, phy_font );

      if ( error )
        goto Fail;
    }

    /* In certain fonts, the auxiliary bytes contain interesting  */
    /* information. These are not in the specification but can be */
    /* guessed by looking at the content of a few PFR0 fonts.     */
    PFR_CHECK( 3 );
    num_aux = PFR_NEXT_ULONG( p );

    if ( num_aux > 0 )
    {
      FT_Byte*  q = p;
      FT_Byte*  q2;


      PFR_CHECK( num_aux );
      p += num_aux;

      while ( num_aux > 0 )
      {
        FT_UInt  length, type;


        if ( q + 4 > p )
          break;

        length = PFR_NEXT_USHORT( q );
        if ( length < 4 || length > num_aux )
          break;

        q2   = q + length - 2;
        type = PFR_NEXT_USHORT( q );

        switch ( type )
        {
        case 1:
          /* this seems to correspond to the font's family name,
           * padded to 16-bits with one zero when necessary
           */
          error = pfr_aux_name_load( q, length - 4U, memory,
                                     &phy_font->family_name );
          if ( error )
            goto Exit;
          break;

        case 2:
          if ( q + 32 > q2 )
            break;

          q += 10;
          phy_font->ascent  = PFR_NEXT_SHORT( q );
          phy_font->descent = PFR_NEXT_SHORT( q );
          phy_font->leading = PFR_NEXT_SHORT( q );
          q += 16;
          break;

        case 3:
          /* this seems to correspond to the font's style name,
           * padded to 16-bits with one zero when necessary
           */
          error = pfr_aux_name_load( q, length - 4U, memory,
                                     &phy_font->style_name );
          if ( error )
            goto Exit;
          break;

        default:
          ;
        }

        q        = q2;
        num_aux -= length;
      }
    }

    /* read the blue values */
    {
      FT_UInt  n, count;


      PFR_CHECK( 1 );
      phy_font->num_blue_values = count = PFR_NEXT_BYTE( p );

      PFR_CHECK( count * 2 );

      if ( FT_NEW_ARRAY( phy_font->blue_values, count ) )
        goto Fail;

      for ( n = 0; n < count; n++ )
        phy_font->blue_values[n] = PFR_NEXT_SHORT( p );
    }

    PFR_CHECK( 8 );
    phy_font->blue_fuzz  = PFR_NEXT_BYTE( p );
    phy_font->blue_scale = PFR_NEXT_BYTE( p );

    phy_font->vertical.standard   = PFR_NEXT_USHORT( p );
    phy_font->horizontal.standard = PFR_NEXT_USHORT( p );

    /* read the character descriptors */
    {
      FT_UInt  n, count, Size;


      phy_font->num_chars    = count = PFR_NEXT_USHORT( p );
      phy_font->chars_offset = offset + ( p - stream->cursor );

      if ( FT_NEW_ARRAY( phy_font->chars, count ) )
        goto Fail;

      Size = 1 + 1 + 2;
      if ( flags & PFR_PHY_2BYTE_CHARCODE )
        Size += 1;

      if ( flags & PFR_PHY_PROPORTIONAL )
        Size += 2;

      if ( flags & PFR_PHY_ASCII_CODE )
        Size += 1;

      if ( flags & PFR_PHY_2BYTE_GPS_SIZE )
        Size += 1;

      if ( flags & PFR_PHY_3BYTE_GPS_OFFSET )
        Size += 1;

      PFR_CHECK( count * Size );

      for ( n = 0; n < count; n++ )
      {
        PFR_Char  cur = &phy_font->chars[n];


        cur->char_code = ( flags & PFR_PHY_2BYTE_CHARCODE )
                         ? PFR_NEXT_USHORT( p )
                         : PFR_NEXT_BYTE( p );

        cur->advance   = ( flags & PFR_PHY_PROPORTIONAL )
                         ? PFR_NEXT_SHORT( p )
                         : (FT_Int) phy_font->standard_advance;

#if 0
        cur->ascii     = ( flags & PFR_PHY_ASCII_CODE )
                         ? PFR_NEXT_BYTE( p )
                         : 0;
#else
        if ( flags & PFR_PHY_ASCII_CODE )
          p += 1;
#endif
        cur->gps_size  = ( flags & PFR_PHY_2BYTE_GPS_SIZE )
                         ? PFR_NEXT_USHORT( p )
                         : PFR_NEXT_BYTE( p );

        cur->gps_offset = ( flags & PFR_PHY_3BYTE_GPS_OFFSET )
                          ? PFR_NEXT_ULONG( p )
                          : PFR_NEXT_USHORT( p );
      }
    }

    /* that's it! */

  Fail:
    FT_FRAME_EXIT();

    /* save position of bitmap info */
    phy_font->bct_offset = FT_STREAM_POS();
    phy_font->cursor     = NULL;

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_phy_font_load: invalid physical font table\n" ));
    goto Fail;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  pfrgload.c                                                             */
/*                                                                         */
/*    FreeType PFR glyph loader (body).                                    */
/*                                                                         */
/*  Copyright 2002, 2003, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "pfrgload.h"
#include "pfrsbit.h"
#include "pfrload.h"            /* for macro definitions */
#include FT_INTERNAL_DEBUG_H

#include "pfrerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pfr


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      PFR GLYPH BUILDER                        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( void )
  pfr_glyph_init( PFR_Glyph       glyph,
                  FT_GlyphLoader  loader )
  {
    FT_ZERO( glyph );

    glyph->loader     = loader;
    glyph->path_begun = 0;

    FT_GlyphLoader_Rewind( loader );
  }


  FT_LOCAL_DEF( void )
  pfr_glyph_done( PFR_Glyph  glyph )
  {
    FT_Memory  memory = glyph->loader->memory;


    FT_FREE( glyph->x_control );
    glyph->y_control = NULL;

    glyph->max_xy_control = 0;
    glyph->num_x_control  = 0;
    glyph->num_y_control  = 0;

    FT_FREE( glyph->subs );

    glyph->max_subs = 0;
    glyph->num_subs = 0;

    glyph->loader     = NULL;
    glyph->path_begun = 0;
  }


  /* close current contour, if any */
  static void
  pfr_glyph_close_contour( PFR_Glyph  glyph )
  {
    FT_GlyphLoader  loader  = glyph->loader;
    FT_Outline*     outline = &loader->current.outline;
    FT_Int          last, first;


    if ( !glyph->path_begun )
      return;

    /* compute first and last point indices in current glyph outline */
    last  = outline->n_points - 1;
    first = 0;
    if ( outline->n_contours > 0 )
      first = outline->contours[outline->n_contours - 1];

    /* if the last point falls on the same location than the first one */
    /* we need to delete it                                            */
    if ( last > first )
    {
      FT_Vector*  p1 = outline->points + first;
      FT_Vector*  p2 = outline->points + last;


      if ( p1->x == p2->x && p1->y == p2->y )
      {
        outline->n_points--;
        last--;
      }
    }

    /* don't add empty contours */
    if ( last >= first )
      outline->contours[outline->n_contours++] = (short)last;

    glyph->path_begun = 0;
  }


  /* reset glyph to start the loading of a new glyph */
  static void
  pfr_glyph_start( PFR_Glyph  glyph )
  {
    glyph->path_begun = 0;
  }


  static FT_Error
  pfr_glyph_line_to( PFR_Glyph   glyph,
                     FT_Vector*  to )
  {
    FT_GlyphLoader  loader  = glyph->loader;
    FT_Outline*     outline = &loader->current.outline;
    FT_Error        error;


    /* check that we have begun a new path */
    FT_ASSERT( glyph->path_begun != 0 );

    error = FT_GLYPHLOADER_CHECK_POINTS( loader, 1, 0 );
    if ( !error )
    {
      FT_UInt  n = outline->n_points;


      outline->points[n] = *to;
      outline->tags  [n] = FT_CURVE_TAG_ON;

      outline->n_points++;
    }

    return error;
  }


  static FT_Error
  pfr_glyph_curve_to( PFR_Glyph   glyph,
                      FT_Vector*  control1,
                      FT_Vector*  control2,
                      FT_Vector*  to )
  {
    FT_GlyphLoader  loader  = glyph->loader;
    FT_Outline*     outline = &loader->current.outline;
    FT_Error        error;


    /* check that we have begun a new path */
    FT_ASSERT( glyph->path_begun != 0 );

    error = FT_GLYPHLOADER_CHECK_POINTS( loader, 3, 0 );
    if ( !error )
    {
      FT_Vector*  vec = outline->points         + outline->n_points;
      FT_Byte*    tag = (FT_Byte*)outline->tags + outline->n_points;


      vec[0] = *control1;
      vec[1] = *control2;
      vec[2] = *to;
      tag[0] = FT_CURVE_TAG_CUBIC;
      tag[1] = FT_CURVE_TAG_CUBIC;
      tag[2] = FT_CURVE_TAG_ON;

      outline->n_points = (FT_Short)( outline->n_points + 3 );
    }

    return error;
  }


  static FT_Error
  pfr_glyph_move_to( PFR_Glyph   glyph,
                     FT_Vector*  to )
  {
    FT_GlyphLoader  loader  = glyph->loader;
    FT_Error        error;


    /* close current contour if any */
    pfr_glyph_close_contour( glyph );

    /* indicate that a new contour has started */
    glyph->path_begun = 1;

    /* check that there is space for a new contour and a new point */
    error = FT_GLYPHLOADER_CHECK_POINTS( loader, 1, 1 );
    if ( !error )
      /* add new start point */
      error = pfr_glyph_line_to( glyph, to );

    return error;
  }


  static void
  pfr_glyph_end( PFR_Glyph  glyph )
  {
    /* close current contour if any */
    pfr_glyph_close_contour( glyph );

    /* merge the current glyph into the stack */
    FT_GlyphLoader_Add( glyph->loader );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      PFR GLYPH LOADER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* load a simple glyph */
  static FT_Error
  pfr_glyph_load_simple( PFR_Glyph  glyph,
                         FT_Byte*   p,
                         FT_Byte*   limit )
  {
    FT_Error   error  = 0;
    FT_Memory  memory = glyph->loader->memory;
    FT_UInt    flags, x_count, y_count, i, count, mask;
    FT_Int     x;


    PFR_CHECK( 1 );
    flags = PFR_NEXT_BYTE( p );

    /* test for composite glyphs */
    FT_ASSERT( ( flags & PFR_GLYPH_IS_COMPOUND ) == 0 );

    x_count = 0;
    y_count = 0;

    if ( flags & PFR_GLYPH_1BYTE_XYCOUNT )
    {
      PFR_CHECK( 1 );
      count   = PFR_NEXT_BYTE( p );
      x_count = ( count & 15 );
      y_count = ( count >> 4 );
    }
    else
    {
      if ( flags & PFR_GLYPH_XCOUNT )
      {
        PFR_CHECK( 1 );
        x_count = PFR_NEXT_BYTE( p );
      }

      if ( flags & PFR_GLYPH_YCOUNT )
      {
        PFR_CHECK( 1 );
        y_count = PFR_NEXT_BYTE( p );
      }
    }

    count = x_count + y_count;

    /* re-allocate array when necessary */
    if ( count > glyph->max_xy_control )
    {
      FT_UInt  new_max = FT_PAD_CEIL( count, 8 );


      if ( FT_RENEW_ARRAY( glyph->x_control,
                           glyph->max_xy_control,
                           new_max ) )
        goto Exit;

      glyph->max_xy_control = new_max;
    }

    glyph->y_control = glyph->x_control + x_count;

    mask  = 0;
    x     = 0;

    for ( i = 0; i < count; i++ )
    {
      if ( ( i & 7 ) == 0 )
      {
        PFR_CHECK( 1 );
        mask = PFR_NEXT_BYTE( p );
      }

      if ( mask & 1 )
      {
        PFR_CHECK( 2 );
        x = PFR_NEXT_SHORT( p );
      }
      else
      {
        PFR_CHECK( 1 );
        x += PFR_NEXT_BYTE( p );
      }

      glyph->x_control[i] = x;

      mask >>= 1;
    }

    /* XXX: for now we ignore the secondary stroke and edge definitions */
    /*      since we don't want to support native PFR hinting           */
    /*                                                                  */
    if ( flags & PFR_GLYPH_EXTRA_ITEMS )
    {
      error = pfr_extra_items_skip( &p, limit );
      if ( error )
        goto Exit;
    }

    pfr_glyph_start( glyph );

    /* now load a simple glyph */
    {
      FT_Vector   pos[4];
      FT_Vector*  cur;


      pos[0].x = pos[0].y = 0;
      pos[3]   = pos[0];

      for (;;)
      {
        FT_Int  format, args_format = 0, args_count, n;


        /***************************************************************/
        /*  read instruction                                           */
        /*                                                             */
        PFR_CHECK( 1 );
        format = PFR_NEXT_BYTE( p );

        switch ( format >> 4 )
        {
        case 0:                             /* end glyph */
          FT_TRACE6(( "- end glyph" ));
          args_count = 0;
          break;

        case 1:                             /* general line operation */
          FT_TRACE6(( "- general line" ));
          goto Line1;

        case 4:                             /* move to inside contour  */
          FT_TRACE6(( "- move to inside" ));
          goto Line1;

        case 5:                             /* move to outside contour */
          FT_TRACE6(( "- move to outside" ));
        Line1:
          args_format = format & 15;
          args_count  = 1;
          break;

        case 2:                             /* horizontal line to */
          FT_TRACE6(( "- horizontal line to cx.%d", format & 15 ));
          pos[0].y   = pos[3].y;
          pos[0].x   = glyph->x_control[format & 15];
          pos[3]     = pos[0];
          args_count = 0;
          break;

        case 3:                             /* vertical line to */
          FT_TRACE6(( "- vertical line to cy.%d", format & 15 ));
          pos[0].x   = pos[3].x;
          pos[0].y   = glyph->y_control[format & 15];
          pos[3] = pos[0];
          args_count = 0;
          break;

        case 6:                             /* horizontal to vertical curve */
          FT_TRACE6(( "- hv curve " ));
          args_format  = 0xB8E;
          args_count   = 3;
          break;

        case 7:                             /* vertical to horizontal curve */
          FT_TRACE6(( "- vh curve" ));
          args_format = 0xE2B;
          args_count  = 3;
          break;

        default:                            /* general curve to */
          FT_TRACE6(( "- general curve" ));
          args_count  = 4;
          args_format = format & 15;
        }

        /***********************************************************/
        /*  now read arguments                                     */
        /*                                                         */
        cur = pos;
        for ( n = 0; n < args_count; n++ )
        {
          FT_Int  idx, delta;


          /* read the X argument */
          switch ( args_format & 3 )
          {
          case 0:                           /* 8-bit index */
            PFR_CHECK( 1 );
            idx  = PFR_NEXT_BYTE( p );
            cur->x = glyph->x_control[idx];
            FT_TRACE7(( " cx#%d", idx ));
            break;

          case 1:                           /* 16-bit value */
            PFR_CHECK( 2 );
            cur->x = PFR_NEXT_SHORT( p );
            FT_TRACE7(( " x.%d", cur->x ));
            break;

          case 2:                           /* 8-bit delta */
            PFR_CHECK( 1 );
            delta  = PFR_NEXT_INT8( p );
            cur->x = pos[3].x + delta;
            FT_TRACE7(( " dx.%d", delta ));
            break;

          default:
            FT_TRACE7(( " |" ));
            cur->x = pos[3].x;
          }

          /* read the Y argument */
          switch ( ( args_format >> 2 ) & 3 )
          {
          case 0:                           /* 8-bit index */
            PFR_CHECK( 1 );
            idx  = PFR_NEXT_BYTE( p );
            cur->y = glyph->y_control[idx];
            FT_TRACE7(( " cy#%d", idx ));
            break;

          case 1:                           /* 16-bit absolute value */
            PFR_CHECK( 2 );
            cur->y = PFR_NEXT_SHORT( p );
            FT_TRACE7(( " y.%d", cur->y ));
            break;

          case 2:                           /* 8-bit delta */
            PFR_CHECK( 1 );
            delta  = PFR_NEXT_INT8( p );
            cur->y = pos[3].y + delta;
            FT_TRACE7(( " dy.%d", delta ));
            break;

          default:
            FT_TRACE7(( " -" ));
            cur->y = pos[3].y;
          }

          /* read the additional format flag for the general curve */
          if ( n == 0 && args_count == 4 )
          {
            PFR_CHECK( 1 );
            args_format = PFR_NEXT_BYTE( p );
            args_count--;
          }
          else
            args_format >>= 4;

          /* save the previous point */
          pos[3] = cur[0];
          cur++;
        }

        FT_TRACE7(( "\n" ));

        /***********************************************************/
        /*  finally, execute instruction                           */
        /*                                                         */
        switch ( format >> 4 )
        {
        case 0:                             /* end glyph => EXIT */
          pfr_glyph_end( glyph );
          goto Exit;

        case 1:                             /* line operations */
        case 2:
        case 3:
          error = pfr_glyph_line_to( glyph, pos );
          goto Test_Error;

        case 4:                             /* move to inside contour  */
        case 5:                             /* move to outside contour */
          error = pfr_glyph_move_to( glyph, pos );
          goto Test_Error;

        default:                            /* curve operations */
          error = pfr_glyph_curve_to( glyph, pos, pos + 1, pos + 2 );

        Test_Error:  /* test error condition */
          if ( error )
            goto Exit;
        }
      } /* for (;;) */
    }

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_glyph_load_simple: invalid glyph data\n" ));
    goto Exit;
  }


  /* load a composite/compound glyph */
  static FT_Error
  pfr_glyph_load_compound( PFR_Glyph  glyph,
                           FT_Byte*   p,
                           FT_Byte*   limit )
  {
    FT_Error        error  = 0;
    FT_GlyphLoader  loader = glyph->loader;
    FT_Memory       memory = loader->memory;
    PFR_SubGlyph    subglyph;
    FT_UInt         flags, i, count, org_count;
    FT_Int          x_pos, y_pos;


    PFR_CHECK( 1 );
    flags = PFR_NEXT_BYTE( p );

    /* test for composite glyphs */
    FT_ASSERT( ( flags & PFR_GLYPH_IS_COMPOUND ) != 0 );

    count = flags & 0x3F;

    /* ignore extra items when present */
    /*                                 */
    if ( flags & PFR_GLYPH_EXTRA_ITEMS )
    {
      error = pfr_extra_items_skip( &p, limit );
      if (error) goto Exit;
    }

    /* we can't rely on the FT_GlyphLoader to load sub-glyphs, because   */
    /* the PFR format is dumb, using direct file offsets to point to the */
    /* sub-glyphs (instead of glyph indices).  Sigh.                     */
    /*                                                                   */
    /* For now, we load the list of sub-glyphs into a different array    */
    /* but this will prevent us from using the auto-hinter at its best   */
    /* quality.                                                          */
    /*                                                                   */
    org_count = glyph->num_subs;

    if ( org_count + count > glyph->max_subs )
    {
      FT_UInt  new_max = ( org_count + count + 3 ) & (FT_UInt)-4;


      if ( FT_RENEW_ARRAY( glyph->subs, glyph->max_subs, new_max ) )
        goto Exit;

      glyph->max_subs = new_max;
    }

    subglyph = glyph->subs + org_count;

    for ( i = 0; i < count; i++, subglyph++ )
    {
      FT_UInt  format;


      x_pos = 0;
      y_pos = 0;

      PFR_CHECK( 1 );
      format = PFR_NEXT_BYTE( p );

      /* read scale when available */
      subglyph->x_scale = 0x10000L;
      if ( format & PFR_SUBGLYPH_XSCALE )
      {
        PFR_CHECK( 2 );
        subglyph->x_scale = PFR_NEXT_SHORT( p ) << 4;
      }

      subglyph->y_scale = 0x10000L;
      if ( format & PFR_SUBGLYPH_YSCALE )
      {
        PFR_CHECK( 2 );
        subglyph->y_scale = PFR_NEXT_SHORT( p ) << 4;
      }

      /* read offset */
      switch ( format & 3 )
      {
      case 1:
        PFR_CHECK( 2 );
        x_pos = PFR_NEXT_SHORT( p );
        break;

      case 2:
        PFR_CHECK( 1 );
        x_pos += PFR_NEXT_INT8( p );
        break;

      default:
        ;
      }

      switch ( ( format >> 2 ) & 3 )
      {
      case 1:
        PFR_CHECK( 2 );
        y_pos = PFR_NEXT_SHORT( p );
        break;

      case 2:
        PFR_CHECK( 1 );
        y_pos += PFR_NEXT_INT8( p );
        break;

      default:
        ;
      }

      subglyph->x_delta = x_pos;
      subglyph->y_delta = y_pos;

      /* read glyph position and size now */
      if ( format & PFR_SUBGLYPH_2BYTE_SIZE )
      {
        PFR_CHECK( 2 );
        subglyph->gps_size = PFR_NEXT_USHORT( p );
      }
      else
      {
        PFR_CHECK( 1 );
        subglyph->gps_size = PFR_NEXT_BYTE( p );
      }

      if ( format & PFR_SUBGLYPH_3BYTE_OFFSET )
      {
        PFR_CHECK( 3 );
        subglyph->gps_offset = PFR_NEXT_LONG( p );
      }
      else
      {
        PFR_CHECK( 2 );
        subglyph->gps_offset = PFR_NEXT_USHORT( p );
      }

      glyph->num_subs++;
    }

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_glyph_load_compound: invalid glyph data\n" ));
    goto Exit;
  }





  static FT_Error
  pfr_glyph_load_rec( PFR_Glyph  glyph,
                      FT_Stream  stream,
                      FT_ULong   gps_offset,
                      FT_ULong   offset,
                      FT_ULong   size )
  {
    FT_Error  error;
    FT_Byte*  p;
    FT_Byte*  limit;


    if ( FT_STREAM_SEEK( gps_offset + offset ) ||
         FT_FRAME_ENTER( size )                )
      goto Exit;

    p     = (FT_Byte*)stream->cursor;
    limit = p + size;

    if ( size > 0 && *p & PFR_GLYPH_IS_COMPOUND )
    {
      FT_Int          n, old_count, count;
      FT_GlyphLoader  loader = glyph->loader;
      FT_Outline*     base   = &loader->base.outline;


      old_count = glyph->num_subs;

      /* this is a compound glyph - load it */
      error = pfr_glyph_load_compound( glyph, p, limit );

      FT_FRAME_EXIT();

      if ( error )
        goto Exit;

      count = glyph->num_subs - old_count;

      /* now, load each individual glyph */
      for ( n = 0; n < count; n++ )
      {
        FT_Int        i, old_points, num_points;
        PFR_SubGlyph  subglyph;


        subglyph   = glyph->subs + old_count + n;
        old_points = base->n_points;

        error = pfr_glyph_load_rec( glyph, stream, gps_offset,
                                    subglyph->gps_offset,
                                    subglyph->gps_size );
        if ( error )
          goto Exit;

        /* note that `glyph->subs' might have been re-allocated */
        subglyph   = glyph->subs + old_count + n;
        num_points = base->n_points - old_points;

        /* translate and eventually scale the new glyph points */
        if ( subglyph->x_scale != 0x10000L || subglyph->y_scale != 0x10000L )
        {
          FT_Vector*  vec = base->points + old_points;


          for ( i = 0; i < num_points; i++, vec++ )
          {
            vec->x = FT_MulFix( vec->x, subglyph->x_scale ) +
                       subglyph->x_delta;
            vec->y = FT_MulFix( vec->y, subglyph->y_scale ) +
                       subglyph->y_delta;
          }
        }
        else
        {
          FT_Vector*  vec = loader->base.outline.points + old_points;


          for ( i = 0; i < num_points; i++, vec++ )
          {
            vec->x += subglyph->x_delta;
            vec->y += subglyph->y_delta;
          }
        }

        /* proceed to next sub-glyph */
      }
    }
    else
    {
      /* load a simple glyph */
      error = pfr_glyph_load_simple( glyph, p, limit );

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }





  FT_LOCAL_DEF( FT_Error )
  pfr_glyph_load( PFR_Glyph  glyph,
                  FT_Stream  stream,
                  FT_ULong   gps_offset,
                  FT_ULong   offset,
                  FT_ULong   size )
  {
    /* initialize glyph loader */
    FT_GlyphLoader_Rewind( glyph->loader );

    glyph->num_subs = 0;

    /* load the glyph, recursively when needed */
    return pfr_glyph_load_rec( glyph, stream, gps_offset, offset, size );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  pfrcmap.c                                                              */
/*                                                                         */
/*    FreeType PFR cmap handling (body).                                   */
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


#include "pfrcmap.h"
#include "pfrobjs.h"
#include FT_INTERNAL_DEBUG_H


  FT_CALLBACK_DEF( FT_Error )
  pfr_cmap_init( PFR_CMap  cmap )
  {
    PFR_Face  face = (PFR_Face)FT_CMAP_FACE( cmap );


    cmap->num_chars = face->phy_font.num_chars;
    cmap->chars     = face->phy_font.chars;

    /* just for safety, check that the character entries are correctly */
    /* sorted in increasing character code order                       */
    {
      FT_UInt  n;


      for ( n = 1; n < cmap->num_chars; n++ )
      {
        if ( cmap->chars[n - 1].char_code >= cmap->chars[n].char_code )
          FT_ASSERT( 0 );
      }
    }

    return 0;
  }


  FT_CALLBACK_DEF( void )
  pfr_cmap_done( PFR_CMap  cmap )
  {
    cmap->chars     = NULL;
    cmap->num_chars = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  pfr_cmap_char_index( PFR_CMap   cmap,
                       FT_UInt32  char_code )
  {
    FT_UInt   min = 0;
    FT_UInt   max = cmap->num_chars;
    FT_UInt   mid;
    PFR_Char  gchar;


    while ( min < max )
    {
      mid   = min + ( max - min ) / 2;
      gchar = cmap->chars + mid;

      if ( gchar->char_code == char_code )
        return mid + 1;

      if ( gchar->char_code < char_code )
        min = mid + 1;
      else
        max = mid;
    }
    return 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  pfr_cmap_char_next( PFR_CMap    cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code + 1;


  Restart:
    {
      FT_UInt   min = 0;
      FT_UInt   max = cmap->num_chars;
      FT_UInt   mid;
      PFR_Char  gchar;


      while ( min < max )
      {
        mid   = min + ( ( max - min ) >> 1 );
        gchar = cmap->chars + mid;

        if ( gchar->char_code == char_code )
        {
          result = mid;
          if ( result != 0 )
          {
            result++;
            goto Exit;
          }

          char_code++;
          goto Restart;
        }

        if ( gchar->char_code < char_code )
          min = mid+1;
        else
          max = mid;
      }

      /* we didn't find it, but we have a pair just above it */
      char_code = 0;

      if ( min < cmap->num_chars )
      {
        gchar  = cmap->chars + min;
        result = min;
        if ( result != 0 )
        {
          result++;
          char_code = gchar->char_code;
        }
      }
    }

  Exit:
    *pchar_code = char_code;
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  pfr_cmap_class_rec =
  {
    sizeof ( PFR_CMapRec ),

    (FT_CMap_InitFunc)     pfr_cmap_init,
    (FT_CMap_DoneFunc)     pfr_cmap_done,
    (FT_CMap_CharIndexFunc)pfr_cmap_char_index,
    (FT_CMap_CharNextFunc) pfr_cmap_char_next
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  pfrobjs.c                                                              */
/*                                                                         */
/*    FreeType PFR object methods (body).                                  */
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


#include "pfrobjs.h"
#include "pfrload.h"
#include "pfrgload.h"
#include "pfrcmap.h"
#include "pfrsbit.h"
#include FT_OUTLINE_H
#include FT_INTERNAL_DEBUG_H

#include "pfrerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pfr


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     FACE OBJECT METHODS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  pfr_face_done( FT_Face  pfrface )     /* PFR_Face */
  {
    PFR_Face   face   = (PFR_Face)pfrface;
    FT_Memory  memory = pfrface->driver->root.memory;


    /* we don't want dangling pointers */
    pfrface->family_name = NULL;
    pfrface->style_name  = NULL;

    /* finalize the physical font record */
    pfr_phy_font_done( &face->phy_font, FT_FACE_MEMORY( face ) );

    /* no need to finalize the logical font or the header */
    FT_FREE( pfrface->available_sizes );
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_face_init( FT_Stream      stream,
                 FT_Face        pfrface,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    PFR_Face  face = (PFR_Face)pfrface;
    FT_Error  error;

    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* load the header and check it */
    error = pfr_header_load( &face->header, stream );
    if ( error )
      goto Exit;

    if ( !pfr_header_check( &face->header ) )
    {
      FT_TRACE4(( "pfr_face_init: not a valid PFR font\n" ));
      error = PFR_Err_Unknown_File_Format;
      goto Exit;
    }

    /* check face index */
    {
      FT_UInt  num_faces;


      error = pfr_log_font_count( stream,
                                  face->header.log_dir_offset,
                                  &num_faces );
      if ( error )
        goto Exit;

      pfrface->num_faces = num_faces;
    }

    if ( face_index < 0 )
      goto Exit;

    if ( face_index >= pfrface->num_faces )
    {
      FT_ERROR(( "pfr_face_init: invalid face index\n" ));
      error = PFR_Err_Invalid_Argument;
      goto Exit;
    }

    /* load the face */
    error = pfr_log_font_load(
               &face->log_font, stream, face_index,
               face->header.log_dir_offset,
               FT_BOOL( face->header.phy_font_max_size_high != 0 ) );
    if ( error )
      goto Exit;

    /* now load the physical font descriptor */
    error = pfr_phy_font_load( &face->phy_font, stream,
                               face->log_font.phys_offset,
                               face->log_font.phys_size );
    if ( error )
      goto Exit;

    /* now, set-up all root face fields */
    {
      PFR_PhyFont  phy_font = &face->phy_font;


      pfrface->face_index = face_index;
      pfrface->num_glyphs = phy_font->num_chars;
      pfrface->face_flags = FT_FACE_FLAG_SCALABLE;

      if ( (phy_font->flags & PFR_PHY_PROPORTIONAL) == 0 )
        pfrface->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      if ( phy_font->flags & PFR_PHY_VERTICAL )
        pfrface->face_flags |= FT_FACE_FLAG_VERTICAL;
      else
        pfrface->face_flags |= FT_FACE_FLAG_HORIZONTAL;

      if ( phy_font->num_strikes > 0 )
        pfrface->face_flags |= FT_FACE_FLAG_FIXED_SIZES;

      if ( phy_font->num_kern_pairs > 0 )
        pfrface->face_flags |= FT_FACE_FLAG_KERNING;

      /* If no family name was found in the "undocumented" auxiliary
       * data, use the font ID instead.  This sucks but is better than
       * nothing.
       */
      pfrface->family_name = phy_font->family_name;
      if ( pfrface->family_name == NULL )
        pfrface->family_name = phy_font->font_id;

      /* note that the style name can be NULL in certain PFR fonts,
       * probably meaning "Regular"
       */
      pfrface->style_name = phy_font->style_name;

      pfrface->num_fixed_sizes = 0;
      pfrface->available_sizes = 0;

      pfrface->bbox         = phy_font->bbox;
      pfrface->units_per_EM = (FT_UShort)phy_font->outline_resolution;
      pfrface->ascender     = (FT_Short) phy_font->bbox.yMax;
      pfrface->descender    = (FT_Short) phy_font->bbox.yMin;

      pfrface->height = (FT_Short)( ( pfrface->units_per_EM * 12 ) / 10 );
      if ( pfrface->height < pfrface->ascender - pfrface->descender )
        pfrface->height = (FT_Short)(pfrface->ascender - pfrface->descender);

      if ( phy_font->num_strikes > 0 )
      {
        FT_UInt          n, count = phy_font->num_strikes;
        FT_Bitmap_Size*  size;
        PFR_Strike       strike;
        FT_Memory        memory = pfrface->stream->memory;


        if ( FT_NEW_ARRAY( pfrface->available_sizes, count ) )
          goto Exit;

        size   = pfrface->available_sizes;
        strike = phy_font->strikes;
        for ( n = 0; n < count; n++, size++, strike++ )
        {
          size->height = (FT_UShort)strike->y_ppm;
          size->width  = (FT_UShort)strike->x_ppm;
          size->size   = strike->y_ppm << 6;
          size->x_ppem = strike->x_ppm << 6;
          size->y_ppem = strike->y_ppm << 6;
        }
        pfrface->num_fixed_sizes = count;
      }

      /* now compute maximum advance width */
      if ( ( phy_font->flags & PFR_PHY_PROPORTIONAL ) == 0 )
        pfrface->max_advance_width = (FT_Short)phy_font->standard_advance;
      else
      {
        FT_Int    max = 0;
        FT_UInt   count = phy_font->num_chars;
        PFR_Char  gchar = phy_font->chars;


        for ( ; count > 0; count--, gchar++ )
        {
          if ( max < gchar->advance )
            max = gchar->advance;
        }

        pfrface->max_advance_width = (FT_Short)max;
      }

      pfrface->max_advance_height = pfrface->height;

      pfrface->underline_position  = (FT_Short)( -pfrface->units_per_EM / 10 );
      pfrface->underline_thickness = (FT_Short)(  pfrface->units_per_EM / 30 );

      /* create charmap */
      {
        FT_CharMapRec  charmap;


        charmap.face        = pfrface;
        charmap.platform_id = 3;
        charmap.encoding_id = 1;
        charmap.encoding    = FT_ENCODING_UNICODE;

        FT_CMap_New( &pfr_cmap_class_rec, NULL, &charmap, NULL );

#if 0
        /* Select default charmap */
        if ( pfrface->num_charmaps )
          pfrface->charmap = pfrface->charmaps[0];
#endif
      }

      /* check whether we've loaded any kerning pairs */
      if ( phy_font->num_kern_pairs )
        pfrface->face_flags |= FT_FACE_FLAG_KERNING;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    SLOT OBJECT METHOD                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  pfr_slot_init( FT_GlyphSlot  pfrslot )        /* PFR_Slot */
  {
    PFR_Slot        slot   = (PFR_Slot)pfrslot;
    FT_GlyphLoader  loader = pfrslot->internal->loader;


    pfr_glyph_init( &slot->glyph, loader );

    return 0;
  }


  FT_LOCAL_DEF( void )
  pfr_slot_done( FT_GlyphSlot  pfrslot )        /* PFR_Slot */
  {
    PFR_Slot  slot = (PFR_Slot)pfrslot;


    pfr_glyph_done( &slot->glyph );
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_slot_load( FT_GlyphSlot  pfrslot,         /* PFR_Slot */
                 FT_Size       pfrsize,         /* PFR_Size */
                 FT_UInt       gindex,
                 FT_Int32      load_flags )
  {
    PFR_Slot     slot    = (PFR_Slot)pfrslot;
    PFR_Size     size    = (PFR_Size)pfrsize;
    FT_Error     error;
    PFR_Face     face    = (PFR_Face)pfrslot->face;
    PFR_Char     gchar;
    FT_Outline*  outline = &pfrslot->outline;
    FT_ULong     gps_offset;


    if ( gindex > 0 )
      gindex--;

    /* check that the glyph index is correct */
    FT_ASSERT( gindex < face->phy_font.num_chars );

    /* try to load an embedded bitmap */
    if ( ( load_flags & ( FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) ) == 0 )
    {
      error = pfr_slot_load_bitmap( slot, size, gindex );
      if ( error == 0 )
        goto Exit;
    }

    if ( load_flags & FT_LOAD_SBITS_ONLY )
    {
      error = PFR_Err_Invalid_Argument;
      goto Exit;
    }

    gchar               = face->phy_font.chars + gindex;
    pfrslot->format     = FT_GLYPH_FORMAT_OUTLINE;
    outline->n_points   = 0;
    outline->n_contours = 0;
    gps_offset          = face->header.gps_section_offset;

    /* load the glyph outline (FT_LOAD_NO_RECURSE isn't supported) */
    error = pfr_glyph_load( &slot->glyph, face->root.stream,
                            gps_offset, gchar->gps_offset, gchar->gps_size );

    if ( !error )
    {
      FT_BBox            cbox;
      FT_Glyph_Metrics*  metrics = &pfrslot->metrics;
      FT_Pos             advance;
      FT_Int             em_metrics, em_outline;
      FT_Bool            scaling;


      scaling = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE ) == 0 );

      /* copy outline data */
      *outline = slot->glyph.loader->base.outline;

      outline->flags &= ~FT_OUTLINE_OWNER;
      outline->flags |= FT_OUTLINE_REVERSE_FILL;

      if ( size && pfrsize->metrics.y_ppem < 24 )
        outline->flags |= FT_OUTLINE_HIGH_PRECISION;

      /* compute the advance vector */
      metrics->horiAdvance = 0;
      metrics->vertAdvance = 0;

      advance    = gchar->advance;
      em_metrics = face->phy_font.metrics_resolution;
      em_outline = face->phy_font.outline_resolution;

      if ( em_metrics != em_outline )
        advance = FT_MulDiv( advance, em_outline, em_metrics );

      if ( face->phy_font.flags & PFR_PHY_VERTICAL )
        metrics->vertAdvance = advance;
      else
        metrics->horiAdvance = advance;

      pfrslot->linearHoriAdvance = metrics->horiAdvance;
      pfrslot->linearVertAdvance = metrics->vertAdvance;

      /* make-up vertical metrics(?) */
      metrics->vertBearingX = 0;
      metrics->vertBearingY = 0;

#if 0 /* some fonts seem to be broken here! */

      /* Apply the font matrix, if any.                 */
      /* TODO: Test existing fonts with unusual matrix  */
      /* whether we have to adjust Units per EM.        */
      {
        FT_Matrix font_matrix;


        font_matrix.xx = face->log_font.matrix[0] << 8;
        font_matrix.yx = face->log_font.matrix[1] << 8;
        font_matrix.xy = face->log_font.matrix[2] << 8;
        font_matrix.yy = face->log_font.matrix[3] << 8;

        FT_Outline_Transform( outline, &font_matrix );
      }
#endif

      /* scale when needed */
      if ( scaling )
      {
        FT_Int      n;
        FT_Fixed    x_scale = pfrsize->metrics.x_scale;
        FT_Fixed    y_scale = pfrsize->metrics.y_scale;
        FT_Vector*  vec     = outline->points;


        /* scale outline points */
        for ( n = 0; n < outline->n_points; n++, vec++ )
        {
          vec->x = FT_MulFix( vec->x, x_scale );
          vec->y = FT_MulFix( vec->y, y_scale );
        }

        /* scale the advance */
        metrics->horiAdvance = FT_MulFix( metrics->horiAdvance, x_scale );
        metrics->vertAdvance = FT_MulFix( metrics->vertAdvance, y_scale );
      }

      /* compute the rest of the metrics */
      FT_Outline_Get_CBox( outline, &cbox );

      metrics->width        = cbox.xMax - cbox.xMin;
      metrics->height       = cbox.yMax - cbox.yMin;
      metrics->horiBearingX = cbox.xMin;
      metrics->horiBearingY = cbox.yMax - metrics->height;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      KERNING METHOD                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  pfr_face_get_kerning( FT_Face     pfrface,        /* PFR_Face */
                        FT_UInt     glyph1,
                        FT_UInt     glyph2,
                        FT_Vector*  kerning )
  {
    PFR_Face     face     = (PFR_Face)pfrface;
    FT_Error     error    = PFR_Err_Ok;
    PFR_PhyFont  phy_font = &face->phy_font;
    FT_UInt32    code1, code2, pair;


    kerning->x = 0;
    kerning->y = 0;

    if ( glyph1 > 0 )
      glyph1--;

    if ( glyph2 > 0 )
      glyph2--;

    /* convert glyph indices to character codes */
    if ( glyph1 > phy_font->num_chars ||
         glyph2 > phy_font->num_chars )
      goto Exit;

    code1 = phy_font->chars[glyph1].char_code;
    code2 = phy_font->chars[glyph2].char_code;
    pair  = PFR_KERN_INDEX( code1, code2 );

    /* now search the list of kerning items */
    {
      PFR_KernItem  item   = phy_font->kern_items;
      FT_Stream     stream = pfrface->stream;


      for ( ; item; item = item->next )
      {
        if ( pair >= item->pair1 && pair <= item->pair2 )
          goto FoundPair;
      }
      goto Exit;

    FoundPair: /* we found an item, now parse it and find the value if any */
      if ( FT_STREAM_SEEK( item->offset )                       ||
           FT_FRAME_ENTER( item->pair_count * item->pair_size ) )
        goto Exit;

      {
        FT_UInt    count    = item->pair_count;
        FT_UInt    size     = item->pair_size;
        FT_UInt    power    = (FT_UInt)ft_highpow2( (FT_UInt32)count );
        FT_UInt    probe    = power * size;
        FT_UInt    extra    = count - power;
        FT_Byte*   base     = stream->cursor;
        FT_Bool    twobytes = FT_BOOL( item->flags & 1 );
        FT_Byte*   p;
        FT_UInt32  cpair;


        if ( extra > 0 )
        {
          p = base + extra * size;

          if ( twobytes )
            cpair = FT_NEXT_ULONG( p );
          else
            cpair = PFR_NEXT_KPAIR( p );

          if ( cpair == pair )
            goto Found;

          if ( cpair < pair )
            base = p;
        }

        while ( probe > size )
        {
          probe >>= 1;
          p       = base + probe;

          if ( twobytes )
            cpair = FT_NEXT_ULONG( p );
          else
            cpair = PFR_NEXT_KPAIR( p );

          if ( cpair == pair )
            goto Found;

          if ( cpair < pair )
            base += probe;
        }

        p = base;

        if ( twobytes )
          cpair = FT_NEXT_ULONG( p );
        else
          cpair = PFR_NEXT_KPAIR( p );

        if ( cpair == pair )
        {
          FT_Int  value;


        Found:
          if ( item->flags & 2 )
            value = FT_PEEK_SHORT( p );
          else
            value = p[0];

          kerning->x = item->base_adj + value;
        }
      }

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }

/* END */

/***************************************************************************/
/*                                                                         */
/*  pfrdrivr.c                                                             */
/*                                                                         */
/*    FreeType PFR driver interface (body).                                */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2006 by                                    */
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
#include FT_SERVICE_PFR_H
#include FT_SERVICE_XFREE86_NAME_H
#include "pfrdrivr.h"
#include "pfrobjs.h"

#include "pfrerror.h"


  FT_CALLBACK_DEF( FT_Error )
  pfr_get_kerning( FT_Face     pfrface,     /* PFR_Face */
                   FT_UInt     left,
                   FT_UInt     right,
                   FT_Vector  *avector )
  {
    PFR_Face     face = (PFR_Face)pfrface;
    PFR_PhyFont  phys = &face->phy_font;


    pfr_face_get_kerning( pfrface, left, right, avector );

    /* convert from metrics to outline units when necessary */
    if ( phys->outline_resolution != phys->metrics_resolution )
    {
      if ( avector->x != 0 )
        avector->x = FT_MulDiv( avector->x, phys->outline_resolution,
                                            phys->metrics_resolution );

      if ( avector->y != 0 )
        avector->y = FT_MulDiv( avector->x, phys->outline_resolution,
                                            phys->metrics_resolution );
    }

    return PFR_Err_Ok;
  }


 /*
  *  PFR METRICS SERVICE
  *
  */

  FT_CALLBACK_DEF( FT_Error )
  pfr_get_advance( FT_Face   pfrface,       /* PFR_Face */
                   FT_UInt   gindex,
                   FT_Pos   *anadvance )
  {
    PFR_Face  face  = (PFR_Face)pfrface;
    FT_Error  error = PFR_Err_Bad_Argument;


    *anadvance = 0;
    if ( face )
    {
      PFR_PhyFont  phys = &face->phy_font;


      if ( gindex < phys->num_chars )
      {
        *anadvance = phys->chars[gindex].advance;
        error = 0;
      }
    }

    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  pfr_get_metrics( FT_Face    pfrface,      /* PFR_Face */
                   FT_UInt   *anoutline_resolution,
                   FT_UInt   *ametrics_resolution,
                   FT_Fixed  *ametrics_x_scale,
                   FT_Fixed  *ametrics_y_scale )
  {
    PFR_Face     face = (PFR_Face)pfrface;
    PFR_PhyFont  phys = &face->phy_font;
    FT_Fixed     x_scale, y_scale;
    FT_Size      size = face->root.size;


    if ( anoutline_resolution )
      *anoutline_resolution = phys->outline_resolution;

    if ( ametrics_resolution )
      *ametrics_resolution = phys->metrics_resolution;

    x_scale = 0x10000L;
    y_scale = 0x10000L;

    if ( size )
    {
      x_scale = FT_DivFix( size->metrics.x_ppem << 6,
                           phys->metrics_resolution );

      y_scale = FT_DivFix( size->metrics.y_ppem << 6,
                           phys->metrics_resolution );
    }

    if ( ametrics_x_scale )
      *ametrics_x_scale = x_scale;

    if ( ametrics_y_scale )
      *ametrics_y_scale = y_scale;

    return PFR_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Service_PfrMetricsRec  pfr_metrics_service_rec =
  {
    pfr_get_metrics,
    pfr_face_get_kerning,
    pfr_get_advance
  };


 /*
  *  SERVICE LIST
  *
  */

  static const FT_ServiceDescRec  pfr_services[] =
  {
    { FT_SERVICE_ID_PFR_METRICS, &pfr_metrics_service_rec },
    { FT_SERVICE_ID_XF86_NAME,   FT_XF86_FORMAT_PFR },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  pfr_get_service( FT_Module         module,
                   const FT_String*  service_id )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( pfr_services, service_id );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  pfr_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER     |
      FT_MODULE_DRIVER_SCALABLE,

      sizeof( FT_DriverRec ),

      "pfr",
      0x10000L,
      0x20000L,

      NULL,

      0,
      0,
      pfr_get_service
    },

    sizeof( PFR_FaceRec ),
    sizeof( PFR_SizeRec ),
    sizeof( PFR_SlotRec ),

    pfr_face_init,
    pfr_face_done,
    0,                  /* FT_Size_InitFunc */
    0,                  /* FT_Size_DoneFunc */
    pfr_slot_init,
    pfr_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif
    pfr_slot_load,

    pfr_get_kerning,
    0,                  /* FT_Face_AttachFunc      */
    0,                   /* FT_Face_GetAdvancesFunc */
    0,                  /* FT_Size_RequestFunc */
    0,                  /* FT_Size_SelectFunc  */
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  pfrsbit.c                                                              */
/*                                                                         */
/*    FreeType PFR bitmap loader (body).                                   */
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


#include "pfrsbit.h"
#include "pfrload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "pfrerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pfr


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      PFR BIT WRITER                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  PFR_BitWriter_
  {
    FT_Byte*  line;      /* current line start                    */
    FT_Int    pitch;     /* line size in bytes                    */
    FT_Int    width;     /* width in pixels/bits                  */
    FT_Int    rows;      /* number of remaining rows to scan      */
    FT_Int    total;     /* total number of bits to draw          */

  } PFR_BitWriterRec, *PFR_BitWriter;


  static void
  pfr_bitwriter_init( PFR_BitWriter  writer,
                      FT_Bitmap*     target,
                      FT_Bool        decreasing )
  {
    writer->line   = target->buffer;
    writer->pitch  = target->pitch;
    writer->width  = target->width;
    writer->rows   = target->rows;
    writer->total  = writer->width * writer->rows;

    if ( !decreasing )
    {
      writer->line += writer->pitch * ( target->rows-1 );
      writer->pitch = -writer->pitch;
    }
  }


  static void
  pfr_bitwriter_decode_bytes( PFR_BitWriter  writer,
                              FT_Byte*       p,
                              FT_Byte*       limit )
  {
    FT_Int    n, reload;
    FT_Int    left = writer->width;
    FT_Byte*  cur  = writer->line;
    FT_UInt   mask = 0x80;
    FT_UInt   val  = 0;
    FT_UInt   c    = 0;


    n = (FT_Int)( limit - p ) * 8;
    if ( n > writer->total )
      n = writer->total;

    reload = n & 7;

    for ( ; n > 0; n-- )
    {
      if ( ( n & 7 ) == reload )
        val = *p++;

      if ( val & 0x80 )
        c |= mask;

      val  <<= 1;
      mask >>= 1;

      if ( --left <= 0 )
      {
        cur[0] = (FT_Byte)c;
        left   = writer->width;
        mask   = 0x80;

        writer->line += writer->pitch;
        cur           = writer->line;
        c             = 0;
      }
      else if ( mask == 0 )
      {
        cur[0] = (FT_Byte)c;
        mask   = 0x80;
        c      = 0;
        cur ++;
      }
    }

    if ( mask != 0x80 )
      cur[0] = (FT_Byte)c;
  }


  static void
  pfr_bitwriter_decode_rle1( PFR_BitWriter  writer,
                             FT_Byte*       p,
                             FT_Byte*       limit )
  {
    FT_Int    n, phase, count, counts[2], reload;
    FT_Int    left = writer->width;
    FT_Byte*  cur  = writer->line;
    FT_UInt   mask = 0x80;
    FT_UInt   c    = 0;


    n = writer->total;

    phase     = 1;
    counts[0] = 0;
    counts[1] = 0;
    count     = 0;
    reload    = 1;

    for ( ; n > 0; n-- )
    {
      if ( reload )
      {
        do
        {
          if ( phase )
          {
            FT_Int  v;


            if ( p >= limit )
              break;

            v         = *p++;
            counts[0] = v >> 4;
            counts[1] = v & 15;
            phase     = 0;
            count     = counts[0];
          }
          else
          {
            phase = 1;
            count = counts[1];
          }

        } while ( count == 0 );
      }

      if ( phase )
        c |= mask;

      mask >>= 1;

      if ( --left <= 0 )
      {
        cur[0] = (FT_Byte) c;
        left   = writer->width;
        mask   = 0x80;

        writer->line += writer->pitch;
        cur           = writer->line;
        c             = 0;
      }
      else if ( mask == 0 )
      {
        cur[0] = (FT_Byte)c;
        mask   = 0x80;
        c      = 0;
        cur ++;
      }

      reload = ( --count <= 0 );
    }

    if ( mask != 0x80 )
      cur[0] = (FT_Byte) c;
  }


  static void
  pfr_bitwriter_decode_rle2( PFR_BitWriter  writer,
                             FT_Byte*       p,
                             FT_Byte*       limit )
  {
    FT_Int    n, phase, count, reload;
    FT_Int    left = writer->width;
    FT_Byte*  cur  = writer->line;
    FT_UInt   mask = 0x80;
    FT_UInt   c    = 0;


    n = writer->total;

    phase  = 1;
    count  = 0;
    reload = 1;

    for ( ; n > 0; n-- )
    {
      if ( reload )
      {
        do
        {
          if ( p >= limit )
            break;

          count = *p++;
          phase = phase ^ 1;

        } while ( count == 0 );
      }

      if ( phase )
        c |= mask;

      mask >>= 1;

      if ( --left <= 0 )
      {
        cur[0] = (FT_Byte) c;
        c      = 0;
        mask   = 0x80;
        left   = writer->width;

        writer->line += writer->pitch;
        cur           = writer->line;
      }
      else if ( mask == 0 )
      {
        cur[0] = (FT_Byte)c;
        c      = 0;
        mask   = 0x80;
        cur ++;
      }

      reload = ( --count <= 0 );
    }

    if ( mask != 0x80 )
      cur[0] = (FT_Byte) c;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  BITMAP DATA DECODING                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  pfr_lookup_bitmap_data( FT_Byte*   base,
                          FT_Byte*   limit,
                          FT_UInt    count,
                          FT_UInt    flags,
                          FT_UInt    char_code,
                          FT_ULong*  found_offset,
                          FT_ULong*  found_size )
  {
    FT_UInt   left, right, char_len;
    FT_Bool   two = FT_BOOL( flags & 1 );
    FT_Byte*  buff;


    char_len = 4;
    if ( two )       char_len += 1;
    if ( flags & 2 ) char_len += 1;
    if ( flags & 4 ) char_len += 1;

    left  = 0;
    right = count;

    while ( left < right )
    {
      FT_UInt  middle, code;


      middle = ( left + right ) >> 1;
      buff   = base + middle * char_len;

      /* check that we are not outside of the table -- */
      /* this is possible with broken fonts...         */
      if ( buff + char_len > limit )
        goto Fail;

      if ( two )
        code = PFR_NEXT_USHORT( buff );
      else
        code = PFR_NEXT_BYTE( buff );

      if ( code == char_code )
        goto Found_It;

      if ( code < char_code )
        left = middle;
      else
        right = middle;
    }

  Fail:
    /* Not found */
    *found_size   = 0;
    *found_offset = 0;
    return;

  Found_It:
    if ( flags & 2 )
      *found_size = PFR_NEXT_USHORT( buff );
    else
      *found_size = PFR_NEXT_BYTE( buff );

    if ( flags & 4 )
      *found_offset = PFR_NEXT_ULONG( buff );
    else
      *found_offset = PFR_NEXT_USHORT( buff );
  }


  /* load bitmap metrics.  "*padvance" must be set to the default value */
  /* before calling this function...                                    */
  /*                                                                    */
  static FT_Error
  pfr_load_bitmap_metrics( FT_Byte**  pdata,
                           FT_Byte*   limit,
                           FT_Long    scaled_advance,
                           FT_Long   *axpos,
                           FT_Long   *aypos,
                           FT_UInt   *axsize,
                           FT_UInt   *aysize,
                           FT_Long   *aadvance,
                           FT_UInt   *aformat )
  {
    FT_Error  error = 0;
    FT_Byte   flags;
    FT_Char   b;
    FT_Byte*  p = *pdata;
    FT_Long   xpos, ypos, advance;
    FT_UInt   xsize, ysize;


    PFR_CHECK( 1 );
    flags = PFR_NEXT_BYTE( p );

    xpos    = 0;
    ypos    = 0;
    xsize   = 0;
    ysize   = 0;
    advance = 0;

    switch ( flags & 3 )
    {
    case 0:
      PFR_CHECK( 1 );
      b    = PFR_NEXT_INT8( p );
      xpos = b >> 4;
      ypos = ( (FT_Char)( b << 4 ) ) >> 4;
      break;

    case 1:
      PFR_CHECK( 2 );
      xpos = PFR_NEXT_INT8( p );
      ypos = PFR_NEXT_INT8( p );
      break;

    case 2:
      PFR_CHECK( 4 );
      xpos = PFR_NEXT_SHORT( p );
      ypos = PFR_NEXT_SHORT( p );
      break;

    case 3:
      PFR_CHECK( 6 );
      xpos = PFR_NEXT_LONG( p );
      ypos = PFR_NEXT_LONG( p );
      break;

    default:
      ;
    }

    flags >>= 2;
    switch ( flags & 3 )
    {
    case 0:
      /* blank image */
      xsize = 0;
      ysize = 0;
      break;

    case 1:
      PFR_CHECK( 1 );
      b     = PFR_NEXT_BYTE( p );
      xsize = ( b >> 4 ) & 0xF;
      ysize = b & 0xF;
      break;

    case 2:
      PFR_CHECK( 2 );
      xsize = PFR_NEXT_BYTE( p );
      ysize = PFR_NEXT_BYTE( p );
      break;

    case 3:
      PFR_CHECK( 4 );
      xsize = PFR_NEXT_USHORT( p );
      ysize = PFR_NEXT_USHORT( p );
      break;

    default:
      ;
    }

    flags >>= 2;
    switch ( flags & 3 )
    {
    case 0:
      advance = scaled_advance;
      break;

    case 1:
      PFR_CHECK( 1 );
      advance = PFR_NEXT_INT8( p ) << 8;
      break;

    case 2:
      PFR_CHECK( 2 );
      advance = PFR_NEXT_SHORT( p );
      break;

    case 3:
      PFR_CHECK( 3 );
      advance = PFR_NEXT_LONG( p );
      break;

    default:
      ;
    }

    *axpos    = xpos;
    *aypos    = ypos;
    *axsize   = xsize;
    *aysize   = ysize;
    *aadvance = advance;
    *aformat  = flags >> 2;
    *pdata    = p;

  Exit:
    return error;

  Too_Short:
    error = PFR_Err_Invalid_Table;
    FT_ERROR(( "pfr_load_bitmap_metrics: invalid glyph data\n" ));
    goto Exit;
  }


  static FT_Error
  pfr_load_bitmap_bits( FT_Byte*    p,
                        FT_Byte*    limit,
                        FT_UInt     format,
                        FT_Bool     decreasing,
                        FT_Bitmap*  target )
  {
    FT_Error          error = 0;
    PFR_BitWriterRec  writer;


    if ( target->rows > 0 && target->width > 0 )
    {
      pfr_bitwriter_init( &writer, target, decreasing );

      switch ( format )
      {
      case 0: /* packed bits */
        pfr_bitwriter_decode_bytes( &writer, p, limit );
        break;

      case 1: /* RLE1 */
        pfr_bitwriter_decode_rle1( &writer, p, limit );
        break;

      case 2: /* RLE2 */
        pfr_bitwriter_decode_rle2( &writer, p, limit );
        break;

      default:
        FT_ERROR(( "pfr_read_bitmap_data: invalid image type\n" ));
        error = PFR_Err_Invalid_File_Format;
      }
    }

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     BITMAP LOADING                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( FT_Error )
  pfr_slot_load_bitmap( PFR_Slot  glyph,
                        PFR_Size  size,
                        FT_UInt   glyph_index )
  {
    FT_Error     error;
    PFR_Face     face   = (PFR_Face) glyph->root.face;
    FT_Stream    stream = face->root.stream;
    PFR_PhyFont  phys   = &face->phy_font;
    FT_ULong     gps_offset;
    FT_ULong     gps_size;
    PFR_Char     character;
    PFR_Strike   strike;


    character = &phys->chars[glyph_index];

    /* Look-up a bitmap strike corresponding to the current */
    /* character dimensions                                 */
    {
      FT_UInt  n;


      strike = phys->strikes;
      for ( n = 0; n < phys->num_strikes; n++ )
      {
        if ( strike->x_ppm == (FT_UInt)size->root.metrics.x_ppem &&
             strike->y_ppm == (FT_UInt)size->root.metrics.y_ppem )
        {
          goto Found_Strike;
        }

        strike++;
      }

      /* couldn't find it */
      return PFR_Err_Invalid_Argument;
    }

  Found_Strike:

    /* Now lookup the glyph's position within the file */
    {
      FT_UInt  char_len;


      char_len = 4;
      if ( strike->flags & 1 ) char_len += 1;
      if ( strike->flags & 2 ) char_len += 1;
      if ( strike->flags & 4 ) char_len += 1;

      /* Access data directly in the frame to speed lookups */
      if ( FT_STREAM_SEEK( phys->bct_offset + strike->bct_offset ) ||
           FT_FRAME_ENTER( char_len * strike->num_bitmaps )        )
        goto Exit;

      pfr_lookup_bitmap_data( stream->cursor,
                              stream->limit,
                              strike->num_bitmaps,
                              strike->flags,
                              character->char_code,
                              &gps_offset,
                              &gps_size );

      FT_FRAME_EXIT();

      if ( gps_size == 0 )
      {
        /* Could not find a bitmap program string for this glyph */
        error = PFR_Err_Invalid_Argument;
        goto Exit;
      }
    }

    /* get the bitmap metrics */
    {
      FT_Long   xpos, ypos, advance;
      FT_UInt   xsize, ysize, format;
      FT_Byte*  p;


      /* compute linear advance */
      advance = character->advance;
      if ( phys->metrics_resolution != phys->outline_resolution )
        advance = FT_MulDiv( advance,
                             phys->outline_resolution,
                             phys->metrics_resolution );

      glyph->root.linearHoriAdvance = advance;

      /* compute default advance, i.e., scaled advance.  This can be */
      /* overridden in the bitmap header of certain glyphs.          */
      advance = FT_MulDiv( (FT_Fixed)size->root.metrics.x_ppem << 8,
                           character->advance,
                           phys->metrics_resolution );

      if ( FT_STREAM_SEEK( face->header.gps_section_offset + gps_offset ) ||
           FT_FRAME_ENTER( gps_size )                                     )
        goto Exit;

      p     = stream->cursor;
      error = pfr_load_bitmap_metrics( &p, stream->limit,
                                       advance,
                                       &xpos, &ypos,
                                       &xsize, &ysize,
                                       &advance, &format );
      if ( !error )
      {
        glyph->root.format = FT_GLYPH_FORMAT_BITMAP;

        /* Set up glyph bitmap and metrics */
        glyph->root.bitmap.width      = (FT_Int)xsize;
        glyph->root.bitmap.rows       = (FT_Int)ysize;
        glyph->root.bitmap.pitch      = (FT_Long)( xsize + 7 ) >> 3;
        glyph->root.bitmap.pixel_mode = FT_PIXEL_MODE_MONO;

        glyph->root.metrics.width        = (FT_Long)xsize << 6;
        glyph->root.metrics.height       = (FT_Long)ysize << 6;
        glyph->root.metrics.horiBearingX = xpos << 6;
        glyph->root.metrics.horiBearingY = ypos << 6;
        glyph->root.metrics.horiAdvance  = FT_PIX_ROUND( ( advance >> 2 ) );
        glyph->root.metrics.vertBearingX = - glyph->root.metrics.width >> 1;
        glyph->root.metrics.vertBearingY = 0;
        glyph->root.metrics.vertAdvance  = size->root.metrics.height;

        glyph->root.bitmap_left = xpos;
        glyph->root.bitmap_top  = ypos + ysize;

        /* Allocate and read bitmap data */
        {
          FT_ULong  len = glyph->root.bitmap.pitch * ysize;


          error = ft_glyphslot_alloc_bitmap( &glyph->root, len );
          if ( !error )
          {
            error = pfr_load_bitmap_bits(
                      p,
                      stream->limit,
                      format,
                      FT_BOOL(face->header.color_flags & 2),
                      &glyph->root.bitmap );
          }
        }
      }

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }

/* END */


/* END */

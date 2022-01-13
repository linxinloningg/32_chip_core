/***************************************************************************/
/*                                                                         */
/*  sfnt.c                                                                 */
/*                                                                         */
/*    Single object library component.                                     */
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


#define FT_MAKE_OPTION_SINGLE_OBJECT

#include "ft2build.h"
/***************************************************************************/
/*                                                                         */
/*  ttload.c                                                               */
/*                                                                         */
/*    Load the basic TrueType tables, i.e., tables that can be either in   */
/*    TTF or OTF fonts (body).                                             */
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
#include FT_TRUETYPE_TAGS_H
#include "ttload.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttload


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_lookup_table                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A face object handle.                                      */
  /*                                                                       */
  /*    tag  :: The searched tag.                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the table directory entry.  0 if not found.           */
  /*                                                                       */
  FT_LOCAL_DEF( TT_Table  )
  tt_face_lookup_table( TT_Face   face,
                        FT_ULong  tag  )
  {
    TT_Table  entry;
    TT_Table  limit;


    FT_TRACE4(( "tt_face_lookup_table: %08p, `%c%c%c%c' -- ",
                face,
                (FT_Char)( tag >> 24 ),
                (FT_Char)( tag >> 16 ),
                (FT_Char)( tag >> 8  ),
                (FT_Char)( tag       ) ));

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {
      /* For compatibility with Windows, we consider 0-length */
      /* tables the same as missing tables.                   */
      if ( entry->Tag == tag && entry->Length != 0 )
      {
        FT_TRACE4(( "found table.\n" ));
        return entry;
      }
    }

    FT_TRACE4(( "could not find table!\n" ));
    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_goto_table                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name, then seek a stream to it.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A face object handle.                                    */
  /*                                                                       */
  /*    tag    :: The searched tag.                                        */
  /*                                                                       */
  /*    stream :: The stream to seek when the table is found.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    length :: The length of the table if found, undefined otherwise.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_goto_table( TT_Face    face,
                      FT_ULong   tag,
                      FT_Stream  stream,
                      FT_ULong*  length )
  {
    TT_Table  table;
    FT_Error  error;


    table = tt_face_lookup_table( face, tag );
    if ( table )
    {
      if ( length )
        *length = table->Length;

      if ( FT_STREAM_SEEK( table->Offset ) )
       goto Exit;
    }
    else
      error = SFNT_Err_Table_Missing;

  Exit:
    return error;
  }


  /* Here, we                                                              */
  /*                                                                       */
  /* - check that `num_tables' is valid                                    */
  /* - look for a `head' table, check its size, and parse it to check      */
  /*   whether its `magic' field is correctly set                          */
  /*                                                                       */
  /* When checking directory entries, ignore the tables `glyx' and `locx'  */
  /* which are hacked-out versions of `glyf' and `loca' in some PostScript */
  /* Type 42 fonts, and which are generally invalid.                       */
  /*                                                                       */
  static FT_Error
  check_table_dir( SFNT_Header  sfnt,
                   FT_Stream    stream )
  {
    FT_Error        error;
    FT_UInt         nn;
    FT_UInt         has_head = 0, has_sing = 0, has_meta = 0;
    FT_ULong        offset = sfnt->offset + 12;

    const FT_ULong  glyx_tag = FT_MAKE_TAG( 'g', 'l', 'y', 'x' );
    const FT_ULong  locx_tag = FT_MAKE_TAG( 'l', 'o', 'c', 'x' );

    static const FT_Frame_Field  table_dir_entry_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_TableRec

      FT_FRAME_START( 16 ),
        FT_FRAME_ULONG( Tag ),
        FT_FRAME_ULONG( CheckSum ),
        FT_FRAME_ULONG( Offset ),
        FT_FRAME_ULONG( Length ),
      FT_FRAME_END
    };


    if ( sfnt->num_tables == 0                         ||
         offset + sfnt->num_tables * 16 > stream->size )
      return SFNT_Err_Unknown_File_Format;

    if ( FT_STREAM_SEEK( offset ) )
      return error;

    for ( nn = 0; nn < sfnt->num_tables; nn++ )
    {
      TT_TableRec  table;


      if ( FT_STREAM_READ_FIELDS( table_dir_entry_fields, &table ) )
        return error;

      if ( table.Offset + table.Length > stream->size &&
           table.Tag != glyx_tag                      &&
           table.Tag != locx_tag                      )
        return SFNT_Err_Unknown_File_Format;

      if ( table.Tag == TTAG_head || table.Tag == TTAG_bhed )
      {
        FT_UInt32  magic;


#ifndef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
        if ( table.Tag == TTAG_head )
#endif
          has_head = 1;

        /*
         * The table length should be 0x36, but certain font tools make it
         * 0x38, so we will just check that it is greater.
         *
         * Note that according to the specification, the table must be
         * padded to 32-bit lengths, but this doesn't apply to the value of
         * its `Length' field!
         *
         */
        if ( table.Length < 0x36 )
          return SFNT_Err_Unknown_File_Format;

        if ( FT_STREAM_SEEK( table.Offset + 12 ) ||
             FT_READ_ULONG( magic )              )
          return error;

        if ( magic != 0x5F0F3CF5UL )
          return SFNT_Err_Unknown_File_Format;

        if ( FT_STREAM_SEEK( offset + ( nn + 1 ) * 16 ) )
          return error;
      }
      else if ( table.Tag == TTAG_SING )
        has_sing = 1;
      else if ( table.Tag == TTAG_META )
        has_meta = 1;
    }

    /* if `sing' and `meta' tables are present, there is no `head' table */
    if ( has_head || ( has_sing && has_meta ) )
      return SFNT_Err_Ok;
    else
      return SFNT_Err_Unknown_File_Format;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_font_dir                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the header of a SFNT font file.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /* <Output>                                                              */
  /*    sfnt       :: The SFNT header.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the beginning of the font directory.  */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_font_dir( TT_Face    face,
                         FT_Stream  stream )
  {
    SFNT_HeaderRec  sfnt;
    FT_Error        error;
    FT_Memory       memory = stream->memory;
    TT_TableRec*    entry;
    TT_TableRec*    limit;

    static const FT_Frame_Field  offset_table_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  SFNT_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_USHORT( num_tables ),
        FT_FRAME_USHORT( search_range ),
        FT_FRAME_USHORT( entry_selector ),
        FT_FRAME_USHORT( range_shift ),
      FT_FRAME_END
    };


    FT_TRACE2(( "tt_face_load_font_dir: %08p\n", face ));

    /* read the offset table */

    sfnt.offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( sfnt.format_tag )                    ||
         FT_STREAM_READ_FIELDS( offset_table_fields, &sfnt ) )
      return error;

    /* many fonts don't have these fields set correctly */
#if 0
    if ( sfnt.search_range != 1 << ( sfnt.entry_selector + 4 )        ||
         sfnt.search_range + sfnt.range_shift != sfnt.num_tables << 4 )
      return SFNT_Err_Unknown_File_Format;
#endif

    /* load the table directory */

    FT_TRACE2(( "-- Tables count:   %12u\n",  sfnt.num_tables ));
    FT_TRACE2(( "-- Format version: %08lx\n", sfnt.format_tag ));

    /* check first */
    error = check_table_dir( &sfnt, stream );
    if ( error )
    {
      FT_TRACE2(( "tt_face_load_font_dir: invalid table directory!\n" ));

      return error;
    }

    face->num_tables = sfnt.num_tables;
    face->format_tag = sfnt.format_tag;

    if ( FT_QNEW_ARRAY( face->dir_tables, face->num_tables ) )
      return error;

    if ( FT_STREAM_SEEK( sfnt.offset + 12 )       ||
         FT_FRAME_ENTER( face->num_tables * 16L ) )
      return error;

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {
      entry->Tag      = FT_GET_TAG4();
      entry->CheckSum = FT_GET_ULONG();
      entry->Offset   = FT_GET_LONG();
      entry->Length   = FT_GET_LONG();

      FT_TRACE2(( "  %c%c%c%c  -  %08lx  -  %08lx\n",
                  (FT_Char)( entry->Tag >> 24 ),
                  (FT_Char)( entry->Tag >> 16 ),
                  (FT_Char)( entry->Tag >> 8  ),
                  (FT_Char)( entry->Tag       ),
                  entry->Offset,
                  entry->Length ));
    }

    FT_FRAME_EXIT();

    FT_TRACE2(( "table directory loaded\n\n" ));

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_any                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads any font table into client memory.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The face object to look for.                             */
  /*                                                                       */
  /*    tag    :: The tag of table to load.  Use the value 0 if you want   */
  /*              to access the whole font file, else set this parameter   */
  /*              to a valid TrueType table tag that you can forge with    */
  /*              the MAKE_TT_TAG macro.                                   */
  /*                                                                       */
  /*    offset :: The starting offset in the table (or the file if         */
  /*              tag == 0).                                               */
  /*                                                                       */
  /*    length :: The address of the decision variable:                    */
  /*                                                                       */
  /*                If length == NULL:                                     */
  /*                  Loads the whole table.  Returns an error if          */
  /*                  `offset' == 0!                                       */
  /*                                                                       */
  /*                If *length == 0:                                       */
  /*                  Exits immediately; returning the length of the given */
  /*                  table or of the font file, depending on the value of */
  /*                  `tag'.                                               */
  /*                                                                       */
  /*                If *length != 0:                                       */
  /*                  Loads the next `length' bytes of table or font,      */
  /*                  starting at offset `offset' (in table or font too).  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer :: The address of target buffer.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_any( TT_Face    face,
                    FT_ULong   tag,
                    FT_Long    offset,
                    FT_Byte*   buffer,
                    FT_ULong*  length )
  {
    FT_Error   error;
    FT_Stream  stream;
    TT_Table   table;
    FT_ULong   size;


    if ( tag != 0 )
    {
      /* look for tag in font directory */
      table = tt_face_lookup_table( face, tag );
      if ( !table )
      {
        error = SFNT_Err_Table_Missing;
        goto Exit;
      }

      offset += table->Offset;
      size    = table->Length;
    }
    else
      /* tag == 0 -- the user wants to access the font file directly */
      size = face->root.stream->size;

    if ( length && *length == 0 )
    {
      *length = size;

      return SFNT_Err_Ok;
    }

    if ( length )
      size = *length;

    stream = face->root.stream;
    /* the `if' is syntactic sugar for picky compilers */
    if ( FT_STREAM_READ_AT( offset, buffer, size ) )
      goto Exit;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_generic_header                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the TrueType table `head' or `bhed'.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  tt_face_load_generic_header( TT_Face    face,
                               FT_Stream  stream,
                               FT_ULong   tag )
  {
    FT_Error    error;
    TT_Header*  header;

    static const FT_Frame_Field  header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_Header

      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG ( Table_Version ),
        FT_FRAME_ULONG ( Font_Revision ),
        FT_FRAME_LONG  ( CheckSum_Adjust ),
        FT_FRAME_LONG  ( Magic_Number ),
        FT_FRAME_USHORT( Flags ),
        FT_FRAME_USHORT( Units_Per_EM ),
        FT_FRAME_LONG  ( Created[0] ),
        FT_FRAME_LONG  ( Created[1] ),
        FT_FRAME_LONG  ( Modified[0] ),
        FT_FRAME_LONG  ( Modified[1] ),
        FT_FRAME_SHORT ( xMin ),
        FT_FRAME_SHORT ( yMin ),
        FT_FRAME_SHORT ( xMax ),
        FT_FRAME_SHORT ( yMax ),
        FT_FRAME_USHORT( Mac_Style ),
        FT_FRAME_USHORT( Lowest_Rec_PPEM ),
        FT_FRAME_SHORT ( Font_Direction ),
        FT_FRAME_SHORT ( Index_To_Loc_Format ),
        FT_FRAME_SHORT ( Glyph_Data_Format ),
      FT_FRAME_END
    };


    error = face->goto_table( face, tag, stream, 0 );
    if ( error )
      goto Exit;

    header = &face->header;

    if ( FT_STREAM_READ_FIELDS( header_fields, header ) )
      goto Exit;

    FT_TRACE3(( "Units per EM: %4u\n", header->Units_Per_EM ));
    FT_TRACE3(( "IndexToLoc:   %4d\n", header->Index_To_Loc_Format ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_head( TT_Face    face,
                     FT_Stream  stream )
  {
    return tt_face_load_generic_header( face, stream, TTAG_head );
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_bhed( TT_Face    face,
                     FT_Stream  stream )
  {
    return tt_face_load_generic_header( face, stream, TTAG_bhed );
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_max_profile                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the maximum profile into a face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_maxp( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error        error;
    TT_MaxProfile*  maxProfile = &face->max_profile;

    const FT_Frame_Field  maxp_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_MaxProfile

      FT_FRAME_START( 6 ),
        FT_FRAME_LONG  ( version ),
        FT_FRAME_USHORT( numGlyphs ),
      FT_FRAME_END
    };

    const FT_Frame_Field  maxp_fields_extra[] =
    {
      FT_FRAME_START( 26 ),
        FT_FRAME_USHORT( maxPoints ),
        FT_FRAME_USHORT( maxContours ),
        FT_FRAME_USHORT( maxCompositePoints ),
        FT_FRAME_USHORT( maxCompositeContours ),
        FT_FRAME_USHORT( maxZones ),
        FT_FRAME_USHORT( maxTwilightPoints ),
        FT_FRAME_USHORT( maxStorage ),
        FT_FRAME_USHORT( maxFunctionDefs ),
        FT_FRAME_USHORT( maxInstructionDefs ),
        FT_FRAME_USHORT( maxStackElements ),
        FT_FRAME_USHORT( maxSizeOfInstructions ),
        FT_FRAME_USHORT( maxComponentElements ),
        FT_FRAME_USHORT( maxComponentDepth ),
      FT_FRAME_END
    };


    error = face->goto_table( face, TTAG_maxp, stream, 0 );
    if ( error )
      goto Exit;

    if ( FT_STREAM_READ_FIELDS( maxp_fields, maxProfile ) )
      goto Exit;

    maxProfile->maxPoints             = 0;
    maxProfile->maxContours           = 0;
    maxProfile->maxCompositePoints    = 0;
    maxProfile->maxCompositeContours  = 0;
    maxProfile->maxZones              = 0;
    maxProfile->maxTwilightPoints     = 0;
    maxProfile->maxStorage            = 0;
    maxProfile->maxFunctionDefs       = 0;
    maxProfile->maxInstructionDefs    = 0;
    maxProfile->maxStackElements      = 0;
    maxProfile->maxSizeOfInstructions = 0;
    maxProfile->maxComponentElements  = 0;
    maxProfile->maxComponentDepth     = 0;

    if ( maxProfile->version >= 0x10000L )
    {
      if ( FT_STREAM_READ_FIELDS( maxp_fields_extra, maxProfile ) )
        goto Exit;

      /* XXX: an adjustment that is necessary to load certain */
      /*      broken fonts like `Keystrokes MT' :-(           */
      /*                                                      */
      /*   We allocate 64 function entries by default when    */
      /*   the maxFunctionDefs field is null.                 */

      if ( maxProfile->maxFunctionDefs == 0 )
        maxProfile->maxFunctionDefs = 64;
    }

    FT_TRACE3(( "numGlyphs: %u\n", maxProfile->numGlyphs ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_names                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_name( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error      error;
    FT_Memory     memory = stream->memory;
    FT_ULong      table_pos, table_len;
    FT_ULong      storage_start, storage_limit;
    FT_UInt       count;
    TT_NameTable  table;

    static const FT_Frame_Field  name_table_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameTableRec

      FT_FRAME_START( 6 ),
        FT_FRAME_USHORT( format ),
        FT_FRAME_USHORT( numNameRecords ),
        FT_FRAME_USHORT( storageOffset ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  name_record_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameEntryRec

      /* no FT_FRAME_START */
        FT_FRAME_USHORT( platformID ),
        FT_FRAME_USHORT( encodingID ),
        FT_FRAME_USHORT( languageID ),
        FT_FRAME_USHORT( nameID ),
        FT_FRAME_USHORT( stringLength ),
        FT_FRAME_USHORT( stringOffset ),
      FT_FRAME_END
    };


    table         = &face->name_table;
    table->stream = stream;

    error = face->goto_table( face, TTAG_name, stream, &table_len );
    if ( error )
      goto Exit;

    table_pos = FT_STREAM_POS();


    if ( FT_STREAM_READ_FIELDS( name_table_fields, table ) )
      goto Exit;

    /* Some popular Asian fonts have an invalid `storageOffset' value   */
    /* (it should be at least "6 + 12*num_names").  However, the string */
    /* offsets, computed as "storageOffset + entry->stringOffset", are  */
    /* valid pointers within the name table...                          */
    /*                                                                  */
    /* We thus can't check `storageOffset' right now.                   */
    /*                                                                  */
    storage_start = table_pos + 6 + 12*table->numNameRecords;
    storage_limit = table_pos + table_len;

    if ( storage_start > storage_limit )
    {
      FT_ERROR(( "invalid `name' table\n" ));
      error = SFNT_Err_Name_Table_Missing;
      goto Exit;
    }

    /* Allocate the array of name records. */
    count                 = table->numNameRecords;
    table->numNameRecords = 0;

    if ( FT_NEW_ARRAY( table->names, count ) ||
         FT_FRAME_ENTER( count * 12 )        )
      goto Exit;

    /* Load the name records and determine how much storage is needed */
    /* to hold the strings themselves.                                */
    {
      TT_NameEntryRec*  entry = table->names;


      for ( ; count > 0; count-- )
      {
        if ( FT_STREAM_READ_FIELDS( name_record_fields, entry ) )
          continue;

        /* check that the name is not empty */
        if ( entry->stringLength == 0 )
          continue;

        /* check that the name string is within the table */
        entry->stringOffset += table_pos + table->storageOffset;
        if ( entry->stringOffset                       < storage_start ||
             entry->stringOffset + entry->stringLength > storage_limit )
        {
          /* invalid entry - ignore it */
          entry->stringOffset = 0;
          entry->stringLength = 0;
          continue;
        }

        entry++;
      }

      table->numNameRecords = (FT_UInt)( entry - table->names );
    }

    FT_FRAME_EXIT();

    /* everything went well, update face->num_names */
    face->num_names = (FT_UShort) table->numNameRecords;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_free_names                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_face_free_name( TT_Face  face )
  {
    FT_Memory     memory = face->root.driver->root.memory;
    TT_NameTable  table  = &face->name_table;
    TT_NameEntry  entry  = table->names;
    FT_UInt       count  = table->numNameRecords;


    if ( table->names )
    {
      for ( ; count > 0; count--, entry++ )
      {
        FT_FREE( entry->string );
        entry->stringLength = 0;
      }

      /* free strings table */
      FT_FREE( table->names );
    }

    table->numNameRecords = 0;
    table->format         = 0;
    table->storageOffset  = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_cmap                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the cmap directory in a face object.  The cmaps itselves are */
  /*    loaded on demand in the `ttcmap.c' module.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_cmap( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error  error;


    error = face->goto_table( face, TTAG_cmap, stream, &face->cmap_size );
    if ( error )
      goto Exit;

    if ( FT_FRAME_EXTRACT( face->cmap_size, face->cmap_table ) )
      face->cmap_size = 0;

  Exit:
    return error;
  }



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_os2                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the OS2 table.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_os2( TT_Face    face,
                    FT_Stream  stream )
  {
    FT_Error  error;
    TT_OS2*   os2;

    const FT_Frame_Field  os2_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_OS2

      FT_FRAME_START( 78 ),
        FT_FRAME_USHORT( version ),
        FT_FRAME_SHORT ( xAvgCharWidth ),
        FT_FRAME_USHORT( usWeightClass ),
        FT_FRAME_USHORT( usWidthClass ),
        FT_FRAME_SHORT ( fsType ),
        FT_FRAME_SHORT ( ySubscriptXSize ),
        FT_FRAME_SHORT ( ySubscriptYSize ),
        FT_FRAME_SHORT ( ySubscriptXOffset ),
        FT_FRAME_SHORT ( ySubscriptYOffset ),
        FT_FRAME_SHORT ( ySuperscriptXSize ),
        FT_FRAME_SHORT ( ySuperscriptYSize ),
        FT_FRAME_SHORT ( ySuperscriptXOffset ),
        FT_FRAME_SHORT ( ySuperscriptYOffset ),
        FT_FRAME_SHORT ( yStrikeoutSize ),
        FT_FRAME_SHORT ( yStrikeoutPosition ),
        FT_FRAME_SHORT ( sFamilyClass ),
        FT_FRAME_BYTE  ( panose[0] ),
        FT_FRAME_BYTE  ( panose[1] ),
        FT_FRAME_BYTE  ( panose[2] ),
        FT_FRAME_BYTE  ( panose[3] ),
        FT_FRAME_BYTE  ( panose[4] ),
        FT_FRAME_BYTE  ( panose[5] ),
        FT_FRAME_BYTE  ( panose[6] ),
        FT_FRAME_BYTE  ( panose[7] ),
        FT_FRAME_BYTE  ( panose[8] ),
        FT_FRAME_BYTE  ( panose[9] ),
        FT_FRAME_ULONG ( ulUnicodeRange1 ),
        FT_FRAME_ULONG ( ulUnicodeRange2 ),
        FT_FRAME_ULONG ( ulUnicodeRange3 ),
        FT_FRAME_ULONG ( ulUnicodeRange4 ),
        FT_FRAME_BYTE  ( achVendID[0] ),
        FT_FRAME_BYTE  ( achVendID[1] ),
        FT_FRAME_BYTE  ( achVendID[2] ),
        FT_FRAME_BYTE  ( achVendID[3] ),

        FT_FRAME_USHORT( fsSelection ),
        FT_FRAME_USHORT( usFirstCharIndex ),
        FT_FRAME_USHORT( usLastCharIndex ),
        FT_FRAME_SHORT ( sTypoAscender ),
        FT_FRAME_SHORT ( sTypoDescender ),
        FT_FRAME_SHORT ( sTypoLineGap ),
        FT_FRAME_USHORT( usWinAscent ),
        FT_FRAME_USHORT( usWinDescent ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_ULONG( ulCodePageRange1 ),
        FT_FRAME_ULONG( ulCodePageRange2 ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra2[] =
    {
      FT_FRAME_START( 10 ),
        FT_FRAME_SHORT ( sxHeight ),
        FT_FRAME_SHORT ( sCapHeight ),
        FT_FRAME_USHORT( usDefaultChar ),
        FT_FRAME_USHORT( usBreakChar ),
        FT_FRAME_USHORT( usMaxContext ),
      FT_FRAME_END
    };


    /* We now support old Mac fonts where the OS/2 table doesn't  */
    /* exist.  Simply put, we set the `version' field to 0xFFFF   */
    /* and test this value each time we need to access the table. */
    error = face->goto_table( face, TTAG_OS2, stream, 0 );
    if ( error )
      goto Exit;

    os2 = &face->os2;

    if ( FT_STREAM_READ_FIELDS( os2_fields, os2 ) )
      goto Exit;

    os2->ulCodePageRange1 = 0;
    os2->ulCodePageRange2 = 0;
    os2->sxHeight         = 0;
    os2->sCapHeight       = 0;
    os2->usDefaultChar    = 0;
    os2->usBreakChar      = 0;
    os2->usMaxContext     = 0;

    if ( os2->version >= 0x0001 )
    {
      /* only version 1 tables */
      if ( FT_STREAM_READ_FIELDS( os2_fields_extra, os2 ) )
        goto Exit;

      if ( os2->version >= 0x0002 )
      {
        /* only version 2 tables */
        if ( FT_STREAM_READ_FIELDS( os2_fields_extra2, os2 ) )
          goto Exit;
      }
    }

    FT_TRACE3(( "sTypoAscender:  %4d\n",   os2->sTypoAscender ));
    FT_TRACE3(( "sTypoDescender: %4d\n",   os2->sTypoDescender ));
    FT_TRACE3(( "usWinAscent:    %4u\n",   os2->usWinAscent ));
    FT_TRACE3(( "usWinDescent:   %4u\n",   os2->usWinDescent ));
    FT_TRACE3(( "fsSelection:    0x%2x\n", os2->fsSelection ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_postscript                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the Postscript table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_post( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error        error;
    TT_Postscript*  post = &face->postscript;

    static const FT_Frame_Field  post_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_Postscript

      FT_FRAME_START( 32 ),
        FT_FRAME_ULONG( FormatType ),
        FT_FRAME_ULONG( italicAngle ),
        FT_FRAME_SHORT( underlinePosition ),
        FT_FRAME_SHORT( underlineThickness ),
        FT_FRAME_ULONG( isFixedPitch ),
        FT_FRAME_ULONG( minMemType42 ),
        FT_FRAME_ULONG( maxMemType42 ),
        FT_FRAME_ULONG( minMemType1 ),
        FT_FRAME_ULONG( maxMemType1 ),
      FT_FRAME_END
    };


    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      return error;

    if ( FT_STREAM_READ_FIELDS( post_fields, post ) )
      return error;

    /* we don't load the glyph names, we do that in another */
    /* module (ttpost).                                     */

    FT_TRACE3(( "FormatType:   0x%x\n", post->FormatType ));
    FT_TRACE3(( "isFixedPitch:   %s\n", post->isFixedPitch
                                        ? "  yes" : "   no" ));

    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_pclt                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the PCL 5 Table.                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_pclt( TT_Face    face,
                     FT_Stream  stream )
  {
    static const FT_Frame_Field  pclt_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_PCLT

      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG ( Version ),
        FT_FRAME_ULONG ( FontNumber ),
        FT_FRAME_USHORT( Pitch ),
        FT_FRAME_USHORT( xHeight ),
        FT_FRAME_USHORT( Style ),
        FT_FRAME_USHORT( TypeFamily ),
        FT_FRAME_USHORT( CapHeight ),
        FT_FRAME_BYTES ( TypeFace, 16 ),
        FT_FRAME_BYTES ( CharacterComplement, 8 ),
        FT_FRAME_BYTES ( FileName, 6 ),
        FT_FRAME_CHAR  ( StrokeWeight ),
        FT_FRAME_CHAR  ( WidthType ),
        FT_FRAME_BYTE  ( SerifStyle ),
        FT_FRAME_BYTE  ( Reserved ),
      FT_FRAME_END
    };

    FT_Error  error;
    TT_PCLT*  pclt = &face->pclt;


    /* optional table */
    error = face->goto_table( face, TTAG_PCLT, stream, 0 );
    if ( error )
      goto Exit;

    if ( FT_STREAM_READ_FIELDS( pclt_fields, pclt ) )
      goto Exit;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_gasp                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the `gasp' table into a face object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_gasp( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt        j,num_ranges;
    TT_GaspRange   gaspranges;


    /* the gasp table is optional */
    error = face->goto_table( face, TTAG_gasp, stream, 0 );
    if ( error )
      goto Exit;

    if ( FT_FRAME_ENTER( 4L ) )
      goto Exit;

    face->gasp.version   = FT_GET_USHORT();
    face->gasp.numRanges = FT_GET_USHORT();

    FT_FRAME_EXIT();

    num_ranges = face->gasp.numRanges;
    FT_TRACE3(( "numRanges: %u\n", num_ranges ));

    if ( FT_QNEW_ARRAY( gaspranges, num_ranges ) ||
         FT_FRAME_ENTER( num_ranges * 4L )      )
      goto Exit;

    face->gasp.gaspRanges = gaspranges;

    for ( j = 0; j < num_ranges; j++ )
    {
      gaspranges[j].maxPPEM  = FT_GET_USHORT();
      gaspranges[j].gaspFlag = FT_GET_USHORT();

      FT_TRACE3(( "gaspRange %d: rangeMaxPPEM %5d, rangeGaspBehavior 0x%x\n",
                  j,
                  gaspranges[j].maxPPEM,
                  gaspranges[j].gaspFlag ));
    }

    FT_FRAME_EXIT();

  Exit:
    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttmtx.c                                                                */
/*                                                                         */
/*    Load the metrics tables common to TTF and OTF fonts (body).          */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttmtx.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttmtx


  /*
   *  Unfortunately, we can't enable our memory optimizations if
   *  FT_CONFIG_OPTION_OLD_INTERNALS is defined.  This is because at least
   *  one rogue client (libXfont in the X.Org XServer) is directly accessing
   *  the metrics.
   */

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hmtx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hmtx' or `vmtx' table into a face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load `vmtx'.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#if defined FT_OPTIMIZE_MEMORY && !defined FT_CONFIG_OPTION_OLD_INTERNALS

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hmtx( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error   error;
    FT_ULong   table_size;
    FT_Byte**  ptable;
    FT_ULong*  ptable_size;
    
    
    if ( vertical )
    {
      error = face->goto_table( face, TTAG_vmtx, stream, &table_size );
      if ( error )
        goto Fail;

      ptable      = &face->vert_metrics;
      ptable_size = &face->vert_metrics_size;
    }
    else
    {
      error = face->goto_table( face, TTAG_hmtx, stream, &table_size );
      if ( error )
        goto Fail;

      ptable      = &face->horz_metrics;
      ptable_size = &face->horz_metrics_size;
    }
    
    if ( FT_FRAME_EXTRACT( table_size, *ptable ) )
      goto Fail;
      
    *ptable_size = table_size;

  Fail:
    return error;
  }

#else /* !OPTIMIZE_MEMORY || OLD_INTERNALS */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hmtx( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_ULong   table_len;
    FT_Long    num_shorts, num_longs, num_shorts_checked;

    TT_LongMetrics *   longs;
    TT_ShortMetrics**  shorts;


    if ( vertical )
    {
      error = face->goto_table( face, TTAG_vmtx, stream, &table_len );
      if ( error )
        goto Fail;

      num_longs = face->vertical.number_Of_VMetrics;
      if ( (FT_ULong)num_longs > table_len / 4 )
        num_longs = (FT_Long)(table_len / 4);

      face->vertical.number_Of_VMetrics = 0;

      longs  = (TT_LongMetrics *)&face->vertical.long_metrics;
      shorts = (TT_ShortMetrics**)&face->vertical.short_metrics;
    }
    else
    {
      error = face->goto_table( face, TTAG_hmtx, stream, &table_len );
      if ( error )
        goto Fail;

      num_longs = face->horizontal.number_Of_HMetrics;
      if ( (FT_ULong)num_longs > table_len / 4 )
        num_longs = (FT_Long)(table_len / 4);

      face->horizontal.number_Of_HMetrics = 0;

      longs  = (TT_LongMetrics *)&face->horizontal.long_metrics;
      shorts = (TT_ShortMetrics**)&face->horizontal.short_metrics;
    }

    /* never trust derived values */

    num_shorts         = face->max_profile.numGlyphs - num_longs;
    num_shorts_checked = ( table_len - num_longs * 4L ) / 2;

    if ( num_shorts < 0 )
    {
      FT_ERROR(( "%cmtx has more metrics than glyphs.\n" ));

      /* Adobe simply ignores this problem.  So we shall do the same. */
#if 0
      error = vertical ? SFNT_Err_Invalid_Vert_Metrics
                       : SFNT_Err_Invalid_Horiz_Metrics;
      goto Exit;
#else
      num_shorts = 0;
#endif
    }

    if ( FT_QNEW_ARRAY( *longs,  num_longs  ) ||
         FT_QNEW_ARRAY( *shorts, num_shorts ) )
      goto Fail;

    if ( FT_FRAME_ENTER( table_len ) )
      goto Fail;

    {
      TT_LongMetrics  cur   = *longs;
      TT_LongMetrics  limit = cur + num_longs;


      for ( ; cur < limit; cur++ )
      {
        cur->advance = FT_GET_USHORT();
        cur->bearing = FT_GET_SHORT();
      }
    }

    /* do we have an inconsistent number of metric values? */
    {
      TT_ShortMetrics*  cur   = *shorts;
      TT_ShortMetrics*  limit = cur +
                                FT_MIN( num_shorts, num_shorts_checked );


      for ( ; cur < limit; cur++ )
        *cur = FT_GET_SHORT();

      /* We fill up the missing left side bearings with the     */
      /* last valid value.  Since this will occur for buggy CJK */
      /* fonts usually only, nothing serious will happen.       */
      if ( num_shorts > num_shorts_checked && num_shorts_checked > 0 )
      {
        FT_Short  val = (*shorts)[num_shorts_checked - 1];


        limit = *shorts + num_shorts;
        for ( ; cur < limit; cur++ )
          *cur = val;
      }
    }

    FT_FRAME_EXIT();

    if ( vertical )
      face->vertical.number_Of_VMetrics = (FT_UShort)num_longs;
    else
      face->horizontal.number_Of_HMetrics = (FT_UShort)num_longs;

  Fail:
    return error;
  }

#endif /* !OPTIMIZE_MEMORY || OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hhea                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hhea' or 'vhea' table into a face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load `vhea'.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hhea( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error        error;
    TT_HoriHeader*  header;

    const FT_Frame_Field  metrics_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_HoriHeader

      FT_FRAME_START( 36 ),
        FT_FRAME_ULONG ( Version ),
        FT_FRAME_SHORT ( Ascender ),
        FT_FRAME_SHORT ( Descender ),
        FT_FRAME_SHORT ( Line_Gap ),
        FT_FRAME_USHORT( advance_Width_Max ),
        FT_FRAME_SHORT ( min_Left_Side_Bearing ),
        FT_FRAME_SHORT ( min_Right_Side_Bearing ),
        FT_FRAME_SHORT ( xMax_Extent ),
        FT_FRAME_SHORT ( caret_Slope_Rise ),
        FT_FRAME_SHORT ( caret_Slope_Run ),
        FT_FRAME_SHORT ( caret_Offset ),
        FT_FRAME_SHORT ( Reserved[0] ),
        FT_FRAME_SHORT ( Reserved[1] ),
        FT_FRAME_SHORT ( Reserved[2] ),
        FT_FRAME_SHORT ( Reserved[3] ),
        FT_FRAME_SHORT ( metric_Data_Format ),
        FT_FRAME_USHORT( number_Of_HMetrics ),
      FT_FRAME_END
    };


    if ( vertical )
    {
      error = face->goto_table( face, TTAG_vhea, stream, 0 );
      if ( error )
        goto Fail;

      header = (TT_HoriHeader*)&face->vertical;
    }
    else
    {
      error = face->goto_table( face, TTAG_hhea, stream, 0 );
      if ( error )
        goto Fail;

      header = &face->horizontal;
    }

    if ( FT_STREAM_READ_FIELDS( metrics_header_fields, header ) )
      goto Fail;

    FT_TRACE3(( "Ascender:          %5d\n", header->Ascender ));
    FT_TRACE3(( "Descender:         %5d\n", header->Descender ));
    FT_TRACE3(( "number_Of_Metrics: %5u\n", header->number_Of_HMetrics ));

    header->long_metrics  = NULL;
    header->short_metrics = NULL;

  Fail:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_metrics                                                */ 
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the horizontal or vertical metrics in font units for a     */
  /*    given glyph.  The metrics are the left side bearing (resp. top     */
  /*    side bearing) and advance width (resp. advance height).            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    header  :: A pointer to either the horizontal or vertical metrics  */
  /*               structure.                                              */
  /*                                                                       */
  /*    idx     :: The glyph index.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    bearing :: The bearing, either left side or top side.              */
  /*                                                                       */
  /*    advance :: The advance width resp. advance height.                 */
  /*                                                                       */
#if defined FT_OPTIMIZE_MEMORY && !defined FT_CONFIG_OPTION_OLD_INTERNALS

  FT_LOCAL_DEF( FT_Error )
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     gindex,
                       FT_Short   *abearing,
                       FT_UShort  *aadvance )
  {
    TT_HoriHeader*  header;
    FT_Byte*        p;
    FT_Byte*        limit;
    FT_UShort       k;


    if ( vertical )
    {
      header = (TT_HoriHeader*)&face->vertical;
      p      = face->vert_metrics;
      limit  = p + face->vert_metrics_size;
    }
    else
    {
      header = &face->horizontal;
      p      = face->horz_metrics;
      limit  = p + face->horz_metrics_size;
    }

    k = header->number_Of_HMetrics;

    if ( k > 0 )
    {
      if ( gindex < (FT_UInt)k )
      {
        p += 4 * gindex;
        if ( p + 4 > limit )
          goto NoData;

        *aadvance = FT_NEXT_USHORT( p );
        *abearing = FT_NEXT_SHORT( p );
      }
      else
      {
        p += 4 * ( k - 1 );
        if ( p + 4 > limit )
          goto NoData;

        *aadvance = FT_NEXT_USHORT( p );
        p += 2 + 2 * ( gindex - k );
        if ( p + 2 > limit )
          *abearing = 0;
        else
          *abearing = FT_PEEK_SHORT( p );
      }
    }
    else
    {
    NoData:
      *abearing = 0;
      *aadvance = 0;
    }

    return SFNT_Err_Ok;
  }

#else /* !OPTIMIZE_MEMORY || OLD_INTERNALS */

  FT_LOCAL_DEF( FT_Error )
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     gindex,
                       FT_Short*   abearing,
                       FT_UShort*  aadvance )
  {
    TT_HoriHeader*  header = vertical ? (TT_HoriHeader*)&face->vertical
                                      :                 &face->horizontal;
    TT_LongMetrics  longs_m;
    FT_UShort       k = header->number_Of_HMetrics;


    if ( k == 0                                         ||
         !header->long_metrics                          ||
         gindex >= (FT_UInt)face->max_profile.numGlyphs )
    {
      *abearing = *aadvance = 0;
      return SFNT_Err_Ok;
    }

    if ( gindex < (FT_UInt)k )
    {
      longs_m   = (TT_LongMetrics)header->long_metrics + gindex;
      *abearing = longs_m->bearing;
      *aadvance = longs_m->advance;
    }
    else
    {
      *abearing = ((TT_ShortMetrics*)header->short_metrics)[gindex - k];
      *aadvance = ((TT_LongMetrics)header->long_metrics)[k - 1].advance;
    }

    return SFNT_Err_Ok;
  }

#endif /* !OPTIMIZE_MEMORY || OLD_INTERNALS */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttcmap.c                                                               */
/*                                                                         */
/*    TrueType character mapping table (cmap) support (body).              */
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
#include FT_INTERNAL_DEBUG_H

#include "sferrors.h"           /* must come before FT_INTERNAL_VALIDATE_H */

#include FT_INTERNAL_VALIDATE_H
#include FT_INTERNAL_STREAM_H
#include "ttload.h"
#include "ttcmap.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttcmap


#define TT_PEEK_SHORT   FT_PEEK_SHORT
#define TT_PEEK_USHORT  FT_PEEK_USHORT
#define TT_PEEK_LONG    FT_PEEK_LONG
#define TT_PEEK_ULONG   FT_PEEK_ULONG

#define TT_NEXT_SHORT   FT_NEXT_SHORT
#define TT_NEXT_USHORT  FT_NEXT_USHORT
#define TT_NEXT_LONG    FT_NEXT_LONG
#define TT_NEXT_ULONG   FT_NEXT_ULONG


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap_init( TT_CMap   cmap,
                FT_Byte*  table )
  {
    cmap->data = table;
    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           FORMAT 0                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME        OFFSET         TYPE          DESCRIPTION                */
  /*                                                                       */
  /*   format      0              USHORT        must be 0                  */
  /*   length      2              USHORT        table length in bytes      */
  /*   language    4              USHORT        Mac language code          */
  /*   glyph_ids   6              BYTE[256]     array of glyph indices     */
  /*               262                                                     */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_0

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap0_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;
    FT_UInt   length = TT_NEXT_USHORT( p );


    if ( table + length > valid->limit || length < 262 )
      FT_INVALID_TOO_SHORT;

    /* check glyph indices whenever necessary */
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  n, idx;


      p = table + 6;
      for ( n = 0; n < 256; n++ )
      {
        idx = *p++;
        if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap0_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table = cmap->data;


    return char_code < 256 ? table[6 + char_code] : 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap0_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_Byte*   table    = cmap->data;
    FT_UInt32  charcode = *pchar_code;
    FT_UInt32  result   = 0;
    FT_UInt    gindex   = 0;


    table += 6;  /* go to glyph ids */
    while ( ++charcode < 256 )
    {
      gindex = table[charcode];
      if ( gindex != 0 )
      {
        result = charcode;
        break;
      }
    }

    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap0_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap0_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap0_char_index,
      (FT_CMap_CharNextFunc) tt_cmap0_char_next
    },
    0,
    (TT_CMap_ValidateFunc)   tt_cmap0_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap0_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_0 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          FORMAT 2                             *****/
  /*****                                                               *****/
  /***** This is used for certain CJK encodings that encode text in a  *****/
  /***** mixed 8/16 bits encoding along the following lines:           *****/
  /*****                                                               *****/
  /***** * Certain byte values correspond to an 8-bit character code   *****/
  /*****   (typically in the range 0..127 for ASCII compatibility).    *****/
  /*****                                                               *****/
  /***** * Certain byte values signal the first byte of a 2-byte       *****/
  /*****   character code (but these values are also valid as the      *****/
  /*****   second byte of a 2-byte character).                         *****/
  /*****                                                               *****/
  /***** The following charmap lookup and iteration functions all      *****/
  /***** assume that the value "charcode" correspond to following:     *****/
  /*****                                                               *****/
  /*****   - For one byte characters, "charcode" is simply the         *****/
  /*****     character code.                                           *****/
  /*****                                                               *****/
  /*****   - For two byte characters, "charcode" is the 2-byte         *****/
  /*****     character code in big endian format.  More exactly:       *****/
  /*****                                                               *****/
  /*****       (charcode >> 8)    is the first byte value              *****/
  /*****       (charcode & 0xFF)  is the second byte value             *****/
  /*****                                                               *****/
  /***** Note that not all values of "charcode" are valid according    *****/
  /***** to these rules, and the function moderately check the         *****/
  /***** arguments.                                                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME        OFFSET         TYPE            DESCRIPTION              */
  /*                                                                       */
  /*   format      0              USHORT          must be 2                */
  /*   length      2              USHORT          table length in bytes    */
  /*   language    4              USHORT          Mac language code        */
  /*   keys        6              USHORT[256]     sub-header keys          */
  /*   subs        518            SUBHEAD[NSUBS]  sub-headers array        */
  /*   glyph_ids   518+NSUB*8     USHORT[]        glyph id array           */
  /*                                                                       */
  /* The `keys' table is used to map charcode high-bytes to sub-headers.   */
  /* The value of `NSUBS' is the number of sub-headers defined in the      */
  /* table and is computed by finding the maximum of the `keys' table.     */
  /*                                                                       */
  /* Note that for any n, `keys[n]' is a byte offset within the `subs'     */
  /* table, i.e., it is the corresponding sub-header index multiplied      */
  /* by 8.                                                                 */
  /*                                                                       */
  /* Each sub-header has the following format:                             */
  /*                                                                       */
  /*   NAME        OFFSET      TYPE            DESCRIPTION                 */
  /*                                                                       */
  /*   first       0           USHORT          first valid low-byte        */
  /*   count       2           USHORT          number of valid low-bytes   */
  /*   delta       4           SHORT           see below                   */
  /*   offset      6           USHORT          see below                   */
  /*                                                                       */
  /* A sub-header defines, for each high-byte, the range of valid          */
  /* low-bytes within the charmap.  Note that the range defined by `first' */
  /* and `count' must be completely included in the interval [0..255]      */
  /* according to the specification.                                       */
  /*                                                                       */
  /* If a character code is contained within a given sub-header, then      */
  /* mapping it to a glyph index is done as follows:                       */
  /*                                                                       */
  /* * The value of `offset' is read.  This is a _byte_ distance from the  */
  /*   location of the `offset' field itself into a slice of the           */
  /*   `glyph_ids' table.  Let's call it `slice' (it's a USHORT[] too).    */
  /*                                                                       */
  /* * The value `slice[char.lo - first]' is read.  If it is 0, there is   */
  /*   no glyph for the charcode.  Otherwise, the value of `delta' is      */
  /*   added to it (modulo 65536) to form a new glyph index.               */
  /*                                                                       */
  /* It is up to the validation routine to check that all offsets fall     */
  /* within the glyph ids table (and not within the `subs' table itself or */
  /* outside of the CMap).                                                 */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_2

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap2_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;           /* skip format */
    FT_UInt   length = TT_PEEK_USHORT( p );
    FT_UInt   n, max_subs;
    FT_Byte*  keys;                         /* keys table */
    FT_Byte*  subs;                         /* sub-headers */
    FT_Byte*  glyph_ids;                    /* glyph id array */


    if ( table + length > valid->limit || length < 6 + 512 )
      FT_INVALID_TOO_SHORT;

    keys = table + 6;

    /* parse keys to compute sub-headers count */
    p        = keys;
    max_subs = 0;
    for ( n = 0; n < 256; n++ )
    {
      FT_UInt  idx = TT_NEXT_USHORT( p );


      /* value must be multiple of 8 */
      if ( valid->level >= FT_VALIDATE_PARANOID && ( idx & 7 ) != 0 )
        FT_INVALID_DATA;

      idx >>= 3;

      if ( idx > max_subs )
        max_subs = idx;
    }

    FT_ASSERT( p == table + 518 );

    subs      = p;
    glyph_ids = subs + (max_subs + 1) * 8;
    if ( glyph_ids > valid->limit )
      FT_INVALID_TOO_SHORT;

    /* parse sub-headers */
    for ( n = 0; n <= max_subs; n++ )
    {
      FT_UInt   first_code, code_count, offset;
      FT_Int    delta;
      FT_Byte*  ids;


      first_code = TT_NEXT_USHORT( p );
      code_count = TT_NEXT_USHORT( p );
      delta      = TT_NEXT_SHORT( p );
      offset     = TT_NEXT_USHORT( p );

      /* check range within 0..255 */
      if ( valid->level >= FT_VALIDATE_PARANOID )
      {
        if ( first_code >= 256 || first_code + code_count > 256 )
          FT_INVALID_DATA;
      }

      /* check offset */
      if ( offset != 0 )
      {
        ids = p - 2 + offset;
        if ( ids < glyph_ids || ids + code_count*2 > table + length )
          FT_INVALID_OFFSET;

        /* check glyph ids */
        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          FT_Byte*  limit = p + code_count * 2;
          FT_UInt   idx;


          for ( ; p < limit; )
          {
            idx = TT_NEXT_USHORT( p );
            if ( idx != 0 )
            {
              idx = ( idx + delta ) & 0xFFFFU;
              if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
                FT_INVALID_GLYPH_ID;
            }
          }
        }
      }
    }

    return SFNT_Err_Ok;
  }


  /* return sub header corresponding to a given character code */
  /* NULL on invalid charcode                                  */
  static FT_Byte*
  tt_cmap2_get_subheader( FT_Byte*   table,
                          FT_UInt32  char_code )
  {
    FT_Byte*  result = NULL;


    if ( char_code < 0x10000UL )
    {
      FT_UInt   char_lo = (FT_UInt)( char_code & 0xFF );
      FT_UInt   char_hi = (FT_UInt)( char_code >> 8 );
      FT_Byte*  p       = table + 6;    /* keys table */
      FT_Byte*  subs    = table + 518;  /* subheaders table */
      FT_Byte*  sub;


      if ( char_hi == 0 )
      {
        /* an 8-bit character code -- we use subHeader 0 in this case */
        /* to test whether the character code is in the charmap       */
        /*                                                            */
        sub = subs;  /* jump to first sub-header */

        /* check that the sub-header for this byte is 0, which */
        /* indicates that it's really a valid one-byte value   */
        /* Otherwise, return 0                                 */
        /*                                                     */
        p += char_lo * 2;
        if ( TT_PEEK_USHORT( p ) != 0 )
          goto Exit;
      }
      else
      {
        /* a 16-bit character code */

        /* jump to key entry  */
        p  += char_hi * 2;
        /* jump to sub-header */
        sub = subs + ( FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 8 ) );

        /* check that the high byte isn't a valid one-byte value */
        if ( sub == subs )
          goto Exit;
      }
      result = sub;
    }
  Exit:
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap2_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table   = cmap->data;
    FT_UInt   result  = 0;
    FT_Byte*  subheader;


    subheader = tt_cmap2_get_subheader( table, char_code );
    if ( subheader )
    {
      FT_Byte*  p   = subheader;
      FT_UInt   idx = (FT_UInt)(char_code & 0xFF);
      FT_UInt   start, count;
      FT_Int    delta;
      FT_UInt   offset;


      start  = TT_NEXT_USHORT( p );
      count  = TT_NEXT_USHORT( p );
      delta  = TT_NEXT_SHORT ( p );
      offset = TT_PEEK_USHORT( p );

      idx -= start;
      if ( idx < count && offset != 0 )
      {
        p  += offset + 2 * idx;
        idx = TT_PEEK_USHORT( p );

        if ( idx != 0 )
          result = (FT_UInt)( idx + delta ) & 0xFFFFU;
      }
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap2_char_next( TT_CMap     cmap,
                      FT_UInt32  *pcharcode )
  {
    FT_Byte*   table    = cmap->data;
    FT_UInt    gindex   = 0;
    FT_UInt32  result   = 0;
    FT_UInt32  charcode = *pcharcode + 1;
    FT_Byte*   subheader;


    while ( charcode < 0x10000UL )
    {
      subheader = tt_cmap2_get_subheader( table, charcode );
      if ( subheader )
      {
        FT_Byte*  p       = subheader;
        FT_UInt   start   = TT_NEXT_USHORT( p );
        FT_UInt   count   = TT_NEXT_USHORT( p );
        FT_Int    delta   = TT_NEXT_SHORT ( p );
        FT_UInt   offset  = TT_PEEK_USHORT( p );
        FT_UInt   char_lo = (FT_UInt)( charcode & 0xFF );
        FT_UInt   pos, idx;


        if ( offset == 0 )
          goto Next_SubHeader;

        if ( char_lo < start )
        {
          char_lo = start;
          pos     = 0;
        }
        else
          pos = (FT_UInt)( char_lo - start );

        p       += offset + pos * 2;
        charcode = FT_PAD_FLOOR( charcode, 256 ) + char_lo;

        for ( ; pos < count; pos++, charcode++ )
        {
          idx = TT_NEXT_USHORT( p );

          if ( idx != 0 )
          {
            gindex = ( idx + delta ) & 0xFFFFU;
            if ( gindex != 0 )
            {
              result = charcode;
              goto Exit;
            }
          }
        }
      }

      /* jump to next sub-header, i.e. higher byte value */
    Next_SubHeader:
      charcode = FT_PAD_FLOOR( charcode, 256 ) + 256;
    }

  Exit:
    *pcharcode = result;

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap2_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap2_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap2_char_index,
      (FT_CMap_CharNextFunc) tt_cmap2_char_next
    },
    2,
    (TT_CMap_ValidateFunc)   tt_cmap2_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap2_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_2 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           FORMAT 4                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME          OFFSET         TYPE              DESCRIPTION          */
  /*                                                                       */
  /*   format        0              USHORT            must be 4            */
  /*   length        2              USHORT            table length         */
  /*                                                  in bytes             */
  /*   language      4              USHORT            Mac language code    */
  /*                                                                       */
  /*   segCountX2    6              USHORT            2*NUM_SEGS           */
  /*   searchRange   8              USHORT            2*(1 << LOG_SEGS)    */
  /*   entrySelector 10             USHORT            LOG_SEGS             */
  /*   rangeShift    12             USHORT            segCountX2 -         */
  /*                                                    searchRange        */
  /*                                                                       */
  /*   endCount      14             USHORT[NUM_SEGS]  end charcode for     */
  /*                                                  each segment; last   */
  /*                                                  is 0xFFFF            */
  /*                                                                       */
  /*   pad           14+NUM_SEGS*2  USHORT            padding              */
  /*                                                                       */
  /*   startCount    16+NUM_SEGS*2  USHORT[NUM_SEGS]  first charcode for   */
  /*                                                  each segment         */
  /*                                                                       */
  /*   idDelta       16+NUM_SEGS*4  SHORT[NUM_SEGS]   delta for each       */
  /*                                                  segment              */
  /*   idOffset      16+NUM_SEGS*6  SHORT[NUM_SEGS]   range offset for     */
  /*                                                  each segment; can be */
  /*                                                  zero                 */
  /*                                                                       */
  /*   glyphIds      16+NUM_SEGS*8  USHORT[]          array of glyph id    */
  /*                                                  ranges               */
  /*                                                                       */
  /* Character codes are modelled by a series of ordered (increasing)      */
  /* intervals called segments.  Each segment has start and end codes,     */
  /* provided by the `startCount' and `endCount' arrays.  Segments must    */
  /* not be overlapping and the last segment should always contain the     */
  /* `0xFFFF' endCount.                                                    */
  /*                                                                       */
  /* The fields `searchRange', `entrySelector' and `rangeShift' are better */
  /* ignored (they are traces of over-engineering in the TrueType          */
  /* specification).                                                       */
  /*                                                                       */
  /* Each segment also has a signed `delta', as well as an optional offset */
  /* within the `glyphIds' table.                                          */
  /*                                                                       */
  /* If a segment's idOffset is 0, the glyph index corresponding to any    */
  /* charcode within the segment is obtained by adding the value of        */
  /* `idDelta' directly to the charcode, modulo 65536.                     */
  /*                                                                       */
  /* Otherwise, a glyph index is taken from the glyph ids sub-array for    */
  /* the segment, and the value of `idDelta' is added to it.               */
  /*                                                                       */
  /*                                                                       */
  /* Finally, note that certain fonts contain invalid charmaps that        */
  /* contain end=0xFFFF, start=0xFFFF, delta=0x0001, offset=0xFFFF at the  */
  /* of their charmaps (e.g. opens___.ttf which comes with OpenOffice.org) */
  /* we need special code to deal with them correctly...                   */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_4

  typedef struct  TT_CMap4Rec_
  {
    TT_CMapRec  cmap;
    FT_UInt32   cur_charcode;   /* current charcode */
    FT_UInt     cur_gindex;     /* current glyph index */

    FT_UInt     num_ranges;
    FT_UInt     cur_range;
    FT_UInt     cur_start;
    FT_UInt     cur_end;
    FT_Int      cur_delta;
    FT_Byte*    cur_values;

  } TT_CMap4Rec, *TT_CMap4;


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_init( TT_CMap4  cmap,
                 FT_Byte*  table )
  {
    FT_Byte*  p;


    cmap->cmap.data    = table;

    p                  = table + 6;
    cmap->num_ranges   = FT_PEEK_USHORT( p ) >> 1;
    cmap->cur_charcode = 0xFFFFFFFFUL;
    cmap->cur_gindex   = 0;

    return SFNT_Err_Ok;
  }


  static FT_Int
  tt_cmap4_set_range( TT_CMap4  cmap,
                      FT_UInt   range_index )
  {
    FT_Byte*  table = cmap->cmap.data;
    FT_Byte*  p;
    FT_UInt   num_ranges = cmap->num_ranges;


    while ( range_index < num_ranges )
    {
      FT_UInt  offset;


      p             = table + 14 + range_index * 2;
      cmap->cur_end = FT_PEEK_USHORT( p );

      p              += 2 + num_ranges * 2;
      cmap->cur_start = FT_PEEK_USHORT( p );

      p              += num_ranges * 2;
      cmap->cur_delta = FT_PEEK_SHORT( p );

      p     += num_ranges * 2;
      offset = FT_PEEK_USHORT( p );

      if ( offset != 0xFFFFU )
      {
        cmap->cur_values = offset ? p + offset : NULL;
        cmap->cur_range  = range_index;
        return 0;
      }

      /* we skip empty segments */
      range_index++;
    }

    return -1;
  }


  /* search the index of the charcode next to cmap->cur_charcode; */
  /* caller should call tt_cmap4_set_range with proper range      */
  /* before calling this function                                 */
  /*                                                              */
  static void
  tt_cmap4_next( TT_CMap4  cmap )
  {
    FT_UInt  charcode;
    

    if ( cmap->cur_charcode >= 0xFFFFUL )
      goto Fail;

    charcode = cmap->cur_charcode + 1;

    if ( charcode < cmap->cur_start )
      charcode = cmap->cur_start;

    for ( ;; )
    {
      FT_Byte*  values = cmap->cur_values;
      FT_UInt   end    = cmap->cur_end;
      FT_Int    delta  = cmap->cur_delta;


      if ( charcode <= end )
      {
        if ( values )
        {
          FT_Byte*  p = values + 2 * ( charcode - cmap->cur_start );


          do
          {
            FT_UInt  gindex = FT_NEXT_USHORT( p );


            if ( gindex != 0 )
            {
              gindex = (FT_UInt)( ( gindex + delta ) & 0xFFFFU );
              if ( gindex != 0 )
              {
                cmap->cur_charcode = charcode;
                cmap->cur_gindex   = gindex;
                return;
              }
            }
          } while ( ++charcode <= end );
        }
        else
        {
          do
          {
            FT_UInt  gindex = (FT_UInt)( ( charcode + delta ) & 0xFFFFU );


            if ( gindex != 0 )
            {
              cmap->cur_charcode = charcode;
              cmap->cur_gindex   = gindex;
              return;
            }
          } while ( ++charcode <= end );
        }
      }

      /* we need to find another range */
      if ( tt_cmap4_set_range( cmap, cmap->cur_range + 1 ) < 0 )
        break;

      if ( charcode < cmap->cur_start )
        charcode = cmap->cur_start;
    }

  Fail:
    cmap->cur_charcode = 0xFFFFFFFFUL;
    cmap->cur_gindex   = 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;               /* skip format */
    FT_UInt   length = TT_NEXT_USHORT( p );
    FT_Byte   *ends, *starts, *offsets, *deltas, *glyph_ids;
    FT_UInt   num_segs;
    FT_Error  error = SFNT_Err_Ok;


    if ( length < 16 )
      FT_INVALID_TOO_SHORT;

    /* in certain fonts, the `length' field is invalid and goes */
    /* out of bound.  We try to correct this here...            */
    if ( table + length > valid->limit )
    {
      if ( valid->level >= FT_VALIDATE_TIGHT )
        FT_INVALID_TOO_SHORT;

      length = (FT_UInt)( valid->limit - table );
    }

    p        = table + 6;
    num_segs = TT_NEXT_USHORT( p );   /* read segCountX2 */

    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      /* check that we have an even value here */
      if ( num_segs & 1 )
        FT_INVALID_DATA;
    }

    num_segs /= 2;

    if ( length < 16 + num_segs * 2 * 4 )
      FT_INVALID_TOO_SHORT;

    /* check the search parameters - even though we never use them */
    /*                                                             */
    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      /* check the values of 'searchRange', 'entrySelector', 'rangeShift' */
      FT_UInt  search_range   = TT_NEXT_USHORT( p );
      FT_UInt  entry_selector = TT_NEXT_USHORT( p );
      FT_UInt  range_shift    = TT_NEXT_USHORT( p );


      if ( ( search_range | range_shift ) & 1 )  /* must be even values */
        FT_INVALID_DATA;

      search_range /= 2;
      range_shift  /= 2;

      /* `search range' is the greatest power of 2 that is <= num_segs */

      if ( search_range                > num_segs                 ||
           search_range * 2            < num_segs                 ||
           search_range + range_shift != num_segs                 ||
           search_range               != ( 1U << entry_selector ) )
        FT_INVALID_DATA;
    }

    ends      = table   + 14;
    starts    = table   + 16 + num_segs * 2;
    deltas    = starts  + num_segs * 2;
    offsets   = deltas  + num_segs * 2;
    glyph_ids = offsets + num_segs * 2;

    /* check last segment, its end count must be FFFF */
    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      p = ends + ( num_segs - 1 ) * 2;
      if ( TT_PEEK_USHORT( p ) != 0xFFFFU )
        FT_INVALID_DATA;
    }

    {
      FT_UInt  start, end, offset, n;
      FT_UInt  last_start = 0, last_end = 0;
      FT_Int   delta;


      for ( n = 0; n < num_segs; n++ )
      {
        p = starts + n * 2;
        start = TT_PEEK_USHORT( p );
        p = ends + n * 2;
        end = TT_PEEK_USHORT( p );
        p = deltas + n * 2;
        delta = TT_PEEK_SHORT( p );
        p = offsets + n * 2;
        offset = TT_PEEK_USHORT( p );

        if ( start > end )
          FT_INVALID_DATA;

        /* this test should be performed at default validation level;  */
        /* unfortunately, some popular Asian fonts present overlapping */
        /* ranges in their charmaps                                    */
        /*                                                             */
        if ( start <= last_end && n > 0 )
        {
          if ( valid->level >= FT_VALIDATE_TIGHT )
            FT_INVALID_DATA;
          else
          {
            /* allow overlapping segments, provided their start points */
            /* and end points, respectively, are in ascending order.   */
            /*                                                         */
            if ( last_start > start || last_end > end )
              error |= TT_CMAP_FLAG_UNSORTED;
            else
              error |= TT_CMAP_FLAG_OVERLAPPING;
          }
        }

        if ( offset && offset != 0xFFFFU )
        {
          p += offset;  /* start of glyph id array */

          /* check that we point within the glyph ids table only */
          if ( valid->level >= FT_VALIDATE_TIGHT )
          {
            if ( p < glyph_ids                                ||
                 p + ( end - start + 1 ) * 2 > table + length )
              FT_INVALID_DATA;
          }
          else
          {
            if ( p < glyph_ids                              ||
                 p + ( end - start + 1 ) * 2 > valid->limit )
              FT_INVALID_DATA;
          }

          /* check glyph indices within the segment range */
          if ( valid->level >= FT_VALIDATE_TIGHT )
          {
            FT_UInt  i, idx;


            for ( i = start; i < end; i++ )
            {
              idx = FT_NEXT_USHORT( p );
              if ( idx != 0 )
              {
                idx = (FT_UInt)( idx + delta ) & 0xFFFFU;

                if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
                  FT_INVALID_GLYPH_ID;
              }
            }
          }
        }
        else if ( offset == 0xFFFFU )
        {
          /* Some fonts (erroneously?) use a range offset of 0xFFFF */
          /* to mean missing glyph in cmap table                    */
          /*                                                        */
          if ( valid->level >= FT_VALIDATE_PARANOID                     ||
               n != num_segs - 1                                        ||
               !( start == 0xFFFFU && end == 0xFFFFU && delta == 0x1U ) )
            FT_INVALID_DATA;
        }

        last_start = start;
        last_end   = end;
      }
    }

    return error;
  }


  static FT_UInt
  tt_cmap4_char_map_linear( TT_CMap   cmap,
                            FT_UInt32*  pcharcode,
                            FT_Bool   next )
  {
    FT_UInt    num_segs2, start, end, offset;
    FT_Int     delta;
    FT_UInt    i, num_segs;
    FT_UInt32  charcode = *pcharcode;
    FT_UInt    gindex   = 0;
    FT_Byte*   p;


    p = cmap->data + 6;
    num_segs2 = FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 2 );

    num_segs = num_segs2 >> 1;

    if ( !num_segs )
      return 0;

    if ( next )
      charcode++;

    /* linear search */
    for ( ; charcode <= 0xFFFFU; charcode++ )
    {
      FT_Byte*  q;
      

      p = cmap->data + 14;               /* ends table   */
      q = cmap->data + 16 + num_segs2;   /* starts table */

      for ( i = 0; i < num_segs; i++ )
      {
        end   = TT_NEXT_USHORT( p );
        start = TT_NEXT_USHORT( q );

        if ( charcode >= start && charcode <= end )
        {
          p       = q - 2 + num_segs2;
          delta   = TT_PEEK_SHORT( p );
          p      += num_segs2;
          offset  = TT_PEEK_USHORT( p );

          if ( offset == 0xFFFFU )
            continue;

          if ( offset )
          {
            p += offset + ( charcode - start ) * 2;
            gindex = TT_PEEK_USHORT( p );
            if ( gindex != 0 )
              gindex = (FT_UInt)( gindex + delta ) & 0xFFFFU;
          }
          else
            gindex = (FT_UInt)( charcode + delta ) & 0xFFFFU;

          break;
        }
      }

      if ( !next || gindex )
        break;
    }

    if ( next && gindex )
      *pcharcode = charcode;

    return gindex;
  }


  static FT_UInt
  tt_cmap4_char_map_binary( TT_CMap   cmap,
                            FT_UInt32*  pcharcode,
                            FT_Bool   next )
  {
    FT_UInt   num_segs2, start, end, offset;
    FT_Int    delta;
    FT_UInt   max, min, mid, num_segs;
    FT_UInt32 charcode = *pcharcode;
    FT_UInt   gindex   = 0;
    FT_Byte*  p;
    

    p = cmap->data + 6;
    num_segs2 = FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 2 );

    if ( !num_segs2 )
      return 0;

    num_segs = num_segs2 >> 1;

    /* make compiler happy */
    mid = num_segs;
    end = 0xFFFFU;

    if ( next )
      charcode++;

    min = 0;
    max = num_segs;

    /* binary search */
    while ( min < max )
    {
      mid    = ( min + max ) >> 1;
      p      = cmap->data + 14 + mid * 2;
      end    = TT_PEEK_USHORT( p );
      p     += 2 + num_segs2;
      start  = TT_PEEK_USHORT( p );

      if ( charcode < start )
        max = mid;
      else if ( charcode > end )
        min = mid + 1;
      else
      {
        p     += num_segs2;
        delta  = TT_PEEK_SHORT( p );
        p     += num_segs2;
        offset = TT_PEEK_USHORT( p );

        /* search the first segment containing `charcode' */
        if ( cmap->flags & TT_CMAP_FLAG_OVERLAPPING )
        {
          FT_UInt  i;


          /* call the current segment `max' */
          max = mid;

          if ( offset == 0xFFFFU )
            mid = max + 1;

          /* search in segments before the current segment */
          for ( i = max ; i > 0; i-- )
          {
            FT_UInt  prev_end;


            p = cmap->data + 14 + ( i - 1 ) * 2;
            prev_end = TT_PEEK_USHORT( p );

            if ( charcode > prev_end )
              break;

            end    = prev_end;
            p     += 2 + num_segs2;
            start  = TT_PEEK_USHORT( p );
            p     += num_segs2;
            delta  = TT_PEEK_SHORT( p );
            p     += num_segs2;
            offset = TT_PEEK_USHORT( p );

            if ( offset != 0xFFFFU )
              mid = i - 1;
          }

          /* no luck */
          if ( mid == max + 1 )
          {
            if ( i != max )
            {
              p      = cmap->data + 14 + max * 2;
              end    = TT_PEEK_USHORT( p );
              p     += 2 + num_segs2;
              start  = TT_PEEK_USHORT( p );
              p     += num_segs2;
              delta  = TT_PEEK_SHORT( p );
              p     += num_segs2;
              offset = TT_PEEK_USHORT( p );
            }

            mid = max;

            /* search in segments after the current segment */
            for ( i = max + 1; i < num_segs; i++ )
            {
              FT_UInt  next_end, next_start;


              p          = cmap->data + 14 + i * 2;
              next_end   = TT_PEEK_USHORT( p );
              p         += 2 + num_segs2;
              next_start = TT_PEEK_USHORT( p );

              if ( charcode < next_start )
                break;

              end    = next_end;
              start  = next_start;
              p     += num_segs2;
              delta  = TT_PEEK_SHORT( p );
              p     += num_segs2;
              offset = TT_PEEK_USHORT( p );

              if ( offset != 0xFFFFU )
                mid = i;
            }
            i--;

            /* still no luck */
            if ( mid == max )
            {
              mid = i;

              break;
            }
          }

          /* end, start, delta, and offset are for the i'th segment */
          if ( mid != i )
          {
            p      = cmap->data + 14 + mid * 2;
            end    = TT_PEEK_USHORT( p );
            p     += 2 + num_segs2;
            start  = TT_PEEK_USHORT( p );
            p     += num_segs2;
            delta  = TT_PEEK_SHORT( p );
            p     += num_segs2;
            offset = TT_PEEK_USHORT( p );
          }
        }
        else
        {
          if ( offset == 0xFFFFU )
            break;
        }

        if ( offset )
        {
          p += offset + ( charcode - start ) * 2;
          gindex = TT_PEEK_USHORT( p );
          if ( gindex != 0 )
            gindex = (FT_UInt)( gindex + delta ) & 0xFFFFU;
        }
        else
          gindex = (FT_UInt)( charcode + delta ) & 0xFFFFU;

        break;
      }
    }

    if ( next )
    {
      TT_CMap4  cmap4 = (TT_CMap4)cmap;


      /* if `charcode' is not in any segment, then `mid' is */
      /* the segment nearest to `charcode'                  */
      /*                                                    */

      if ( charcode > end )
      {
        mid++;
        if ( mid == num_segs )
          return 0;
      }

      if ( tt_cmap4_set_range( cmap4, mid ) )
      {
        if ( gindex )
          *pcharcode = charcode;
      }
      else
      {
        cmap4->cur_charcode = charcode;

        if ( gindex )
          cmap4->cur_gindex = gindex;
        else
        {
          cmap4->cur_charcode = charcode;
          tt_cmap4_next( cmap4 );
          gindex = cmap4->cur_gindex;
        }

        if ( gindex )
          *pcharcode = cmap4->cur_charcode;
      }
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap4_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    if ( char_code >= 0x10000UL )
      return 0;

    if ( cmap->flags & TT_CMAP_FLAG_UNSORTED )
      return tt_cmap4_char_map_linear( cmap, &char_code, 0 );
    else
      return tt_cmap4_char_map_binary( cmap, &char_code, 0 );
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap4_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt  gindex;


    if ( *pchar_code >= 0xFFFFU )
      return 0;

    if ( cmap->flags & TT_CMAP_FLAG_UNSORTED )
      gindex = tt_cmap4_char_map_linear( cmap, pchar_code, 1 );
    else
    {
      TT_CMap4  cmap4 = (TT_CMap4)cmap;


      /* no need to search */
      if ( *pchar_code == cmap4->cur_charcode )
      {
        tt_cmap4_next( cmap4 );
        gindex = cmap4->cur_gindex;
        if ( gindex )
          *pchar_code = cmap4->cur_charcode;
      }
      else
        gindex = tt_cmap4_char_map_binary( cmap, pchar_code, 1 );
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap4_class_rec =
  {
    {
      sizeof ( TT_CMap4Rec ),
      (FT_CMap_InitFunc)     tt_cmap4_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap4_char_index,
      (FT_CMap_CharNextFunc) tt_cmap4_char_next
    },
    4,
    (TT_CMap_ValidateFunc)   tt_cmap4_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap4_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_4 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          FORMAT 6                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME        OFFSET          TYPE             DESCRIPTION            */
  /*                                                                       */
  /*   format       0              USHORT           must be 4              */
  /*   length       2              USHORT           table length in bytes  */
  /*   language     4              USHORT           Mac language code      */
  /*                                                                       */
  /*   first        6              USHORT           first segment code     */
  /*   count        8              USHORT           segment size in chars  */
  /*   glyphIds     10             USHORT[count]    glyph ids              */
  /*                                                                       */
  /* A very simplified segment mapping.                                    */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_6

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap6_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p;
    FT_UInt   length, count;


    if ( table + 10 > valid->limit )
      FT_INVALID_TOO_SHORT;

    p      = table + 2;
    length = TT_NEXT_USHORT( p );

    p      = table + 8;             /* skip language and start index */
    count  = TT_NEXT_USHORT( p );

    if ( table + length > valid->limit || length < 10 + count * 2 )
      FT_INVALID_TOO_SHORT;

    /* check glyph indices */
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  gindex;


      for ( ; count > 0; count-- )
      {
        gindex = TT_NEXT_USHORT( p );
        if ( gindex >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap6_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table  = cmap->data;
    FT_UInt   result = 0;
    FT_Byte*  p      = table + 6;
    FT_UInt   start  = TT_NEXT_USHORT( p );
    FT_UInt   count  = TT_NEXT_USHORT( p );
    FT_UInt   idx    = (FT_UInt)( char_code - start );


    if ( idx < count )
    {
      p += 2 * idx;
      result = TT_PEEK_USHORT( p );
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap6_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_Byte*   table     = cmap->data;
    FT_UInt32  result    = 0;
    FT_UInt32  char_code = *pchar_code + 1;
    FT_UInt    gindex    = 0;

    FT_Byte*   p         = table + 6;
    FT_UInt    start     = TT_NEXT_USHORT( p );
    FT_UInt    count     = TT_NEXT_USHORT( p );
    FT_UInt    idx;


    if ( char_code >= 0x10000UL )
      goto Exit;

    if ( char_code < start )
      char_code = start;

    idx = (FT_UInt)( char_code - start );
    p  += 2 * idx;

    for ( ; idx < count; idx++ )
    {
      gindex = TT_NEXT_USHORT( p );
      if ( gindex != 0 )
      {
        result = char_code;
        break;
      }
      char_code++;
    }

  Exit:
    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap6_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap6_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap6_char_index,
      (FT_CMap_CharNextFunc) tt_cmap6_char_next
    },
    6,
    (TT_CMap_ValidateFunc)   tt_cmap6_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap6_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_6 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          FORMAT 8                             *****/
  /*****                                                               *****/
  /***** It's hard to completely understand what the OpenType spec     *****/
  /***** says about this format, but here is my conclusion.            *****/
  /*****                                                               *****/
  /***** The purpose of this format is to easily map UTF-16 text to    *****/
  /***** glyph indices.  Basically, the `char_code' must be in one of  *****/
  /***** the following formats:                                        *****/
  /*****                                                               *****/
  /*****   - A 16-bit value that isn't part of the Unicode Surrogates  *****/
  /*****     Area (i.e. U+D800-U+DFFF).                                *****/
  /*****                                                               *****/
  /*****   - A 32-bit value, made of two surrogate values, i.e.. if    *****/
  /*****     `char_code = (char_hi << 16) | char_lo', then both        *****/
  /*****     `char_hi' and `char_lo' must be in the Surrogates Area.   *****/
  /*****      Area.                                                    *****/
  /*****                                                               *****/
  /***** The 'is32' table embedded in the charmap indicates whether a  *****/
  /***** given 16-bit value is in the surrogates area or not.          *****/
  /*****                                                               *****/
  /***** So, for any given `char_code', we can assert the following:   *****/
  /*****                                                               *****/
  /*****   If `char_hi == 0' then we must have `is32[char_lo] == 0'.   *****/
  /*****                                                               *****/
  /*****   If `char_hi != 0' then we must have both                    *****/
  /*****   `is32[char_hi] != 0' and `is32[char_lo] != 0'.              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME        OFFSET         TYPE        DESCRIPTION                  */
  /*                                                                       */
  /*   format      0              USHORT      must be 8                    */
  /*   reseved     2              USHORT      reserved                     */
  /*   length      4              ULONG       length in bytes              */
  /*   language    8              ULONG       Mac language code            */
  /*   is32        12             BYTE[8192]  32-bitness bitmap            */
  /*   count       8204           ULONG       number of groups             */
  /*                                                                       */
  /* This header is followed by 'count' groups of the following format:    */
  /*                                                                       */
  /*   start       0              ULONG       first charcode               */
  /*   end         4              ULONG       last charcode                */
  /*   startId     8              ULONG       start glyph id for the group */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_8

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap8_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*   p = table + 4;
    FT_Byte*   is32;
    FT_UInt32  length;
    FT_UInt32  num_groups;


    if ( table + 16 + 8192 > valid->limit )
      FT_INVALID_TOO_SHORT;

    length = TT_NEXT_ULONG( p );
    if ( table + length > valid->limit || length < 8208 )
      FT_INVALID_TOO_SHORT;

    is32       = table + 12;
    p          = is32  + 8192;          /* skip `is32' array */
    num_groups = TT_NEXT_ULONG( p );

    if ( p + num_groups * 12 > valid->limit )
      FT_INVALID_TOO_SHORT;

    /* check groups, they must be in increasing order */
    {
      FT_UInt32  n, start, end, start_id, count, last = 0;


      for ( n = 0; n < num_groups; n++ )
      {
        FT_UInt   hi, lo;


        start    = TT_NEXT_ULONG( p );
        end      = TT_NEXT_ULONG( p );
        start_id = TT_NEXT_ULONG( p );

        if ( start > end )
          FT_INVALID_DATA;

        if ( n > 0 && start <= last )
          FT_INVALID_DATA;

        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          if ( start_id + end - start >= TT_VALID_GLYPH_COUNT( valid ) )
            FT_INVALID_GLYPH_ID;

          count = (FT_UInt32)( end - start + 1 );

          if ( start & ~0xFFFFU )
          {
            /* start_hi != 0; check that is32[i] is 1 for each i in */
            /* the `hi' and `lo' of the range [start..end]          */
            for ( ; count > 0; count--, start++ )
            {
              hi = (FT_UInt)( start >> 16 );
              lo = (FT_UInt)( start & 0xFFFFU );

              if ( (is32[hi >> 3] & ( 0x80 >> ( hi & 7 ) ) ) == 0 )
                FT_INVALID_DATA;

              if ( (is32[lo >> 3] & ( 0x80 >> ( lo & 7 ) ) ) == 0 )
                FT_INVALID_DATA;
            }
          }
          else
          {
            /* start_hi == 0; check that is32[i] is 0 for each i in */
            /* the range [start..end]                               */

            /* end_hi cannot be != 0! */
            if ( end & ~0xFFFFU )
              FT_INVALID_DATA;

            for ( ; count > 0; count--, start++ )
            {
              lo = (FT_UInt)( start & 0xFFFFU );

              if ( (is32[lo >> 3] & ( 0x80 >> ( lo & 7 ) ) ) != 0 )
                FT_INVALID_DATA;
            }
          }
        }

        last = end;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap8_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*   table      = cmap->data;
    FT_UInt    result     = 0;
    FT_Byte*   p          = table + 8204;
    FT_UInt32  num_groups = TT_NEXT_ULONG( p );
    FT_UInt32  start, end, start_id;


    for ( ; num_groups > 0; num_groups-- )
    {
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_NEXT_ULONG( p );

      if ( char_code < start )
        break;

      if ( char_code <= end )
      {
        result = (FT_UInt)( start_id + char_code - start );
        break;
      }
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap8_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt32  result     = 0;
    FT_UInt32  char_code  = *pchar_code + 1;
    FT_UInt    gindex     = 0;
    FT_Byte*   table      = cmap->data;
    FT_Byte*   p          = table + 8204;
    FT_UInt32  num_groups = TT_NEXT_ULONG( p );
    FT_UInt32  start, end, start_id;


    p = table + 8208;

    for ( ; num_groups > 0; num_groups-- )
    {
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_NEXT_ULONG( p );

      if ( char_code < start )
        char_code = start;

      if ( char_code <= end )
      {
        gindex = (FT_UInt)( char_code - start + start_id );
        if ( gindex != 0 )
        {
          result = char_code;
          goto Exit;
        }
      }
    }

  Exit:
    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap8_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap8_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap8_char_index,
      (FT_CMap_CharNextFunc) tt_cmap8_char_next
    },
    8,
    (TT_CMap_ValidateFunc)   tt_cmap8_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap8_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_8 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          FORMAT 10                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME      OFFSET  TYPE               DESCRIPTION                    */
  /*                                                                       */
  /*   format     0      USHORT             must be 10                     */
  /*   reserved   2      USHORT             reserved                       */
  /*   length     4      ULONG              length in bytes                */
  /*   language   8      ULONG              Mac language code              */
  /*                                                                       */
  /*   start     12      ULONG              first char in range            */
  /*   count     16      ULONG              number of chars in range       */
  /*   glyphIds  20      USHORT[count]      glyph indices covered          */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_10

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap10_validate( FT_Byte*      table,
                      FT_Validator  valid )
  {
    FT_Byte*  p = table + 4;
    FT_ULong  length, count;


    if ( table + 20 > valid->limit )
      FT_INVALID_TOO_SHORT;

    length = TT_NEXT_ULONG( p );
    p      = table + 16;
    count  = TT_NEXT_ULONG( p );

    if ( table + length > valid->limit || length < 20 + count * 2 )
      FT_INVALID_TOO_SHORT;

    /* check glyph indices */
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  gindex;


      for ( ; count > 0; count-- )
      {
        gindex = TT_NEXT_USHORT( p );
        if ( gindex >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap10_char_index( TT_CMap    cmap,
                        FT_UInt32  char_code )
  {
    FT_Byte*   table  = cmap->data;
    FT_UInt    result = 0;
    FT_Byte*   p      = table + 12;
    FT_UInt32  start  = TT_NEXT_ULONG( p );
    FT_UInt32  count  = TT_NEXT_ULONG( p );
    FT_UInt32  idx    = (FT_ULong)( char_code - start );


    if ( idx < count )
    {
      p     += 2 * idx;
      result = TT_PEEK_USHORT( p );
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap10_char_next( TT_CMap     cmap,
                       FT_UInt32  *pchar_code )
  {
    FT_Byte*   table     = cmap->data;
    FT_UInt32  char_code = *pchar_code + 1;
    FT_UInt    gindex    = 0;
    FT_Byte*   p         = table + 12;
    FT_UInt32  start     = TT_NEXT_ULONG( p );
    FT_UInt32  count     = TT_NEXT_ULONG( p );
    FT_UInt32  idx;


    if ( char_code < start )
      char_code = start;

    idx = (FT_UInt32)( char_code - start );
    p  += 2 * idx;

    for ( ; idx < count; idx++ )
    {
      gindex = TT_NEXT_USHORT( p );
      if ( gindex != 0 )
        break;
      char_code++;
    }

    *pchar_code = char_code;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap10_get_info( TT_CMap       cmap,
                      TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap10_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap10_char_index,
      (FT_CMap_CharNextFunc) tt_cmap10_char_next
    },
    10,
    (TT_CMap_ValidateFunc)   tt_cmap10_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap10_get_info
  };

#endif /* TT_CONFIG_CMAP_FORMAT_10 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                          FORMAT 12                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* TABLE OVERVIEW                                                        */
  /* --------------                                                        */
  /*                                                                       */
  /*   NAME        OFFSET     TYPE       DESCRIPTION                       */
  /*                                                                       */
  /*   format      0          USHORT     must be 12                        */
  /*   reserved    2          USHORT     reserved                          */
  /*   length      4          ULONG      length in bytes                   */
  /*   language    8          ULONG      Mac language code                 */
  /*   count       12         ULONG      number of groups                  */
  /*               16                                                      */
  /*                                                                       */
  /* This header is followed by `count' groups of the following format:    */
  /*                                                                       */
  /*   start       0          ULONG      first charcode                    */
  /*   end         4          ULONG      last charcode                     */
  /*   startId     8          ULONG      start glyph id for the group      */
  /*                                                                       */

#ifdef TT_CONFIG_CMAP_FORMAT_12

  typedef struct  TT_CMap12Rec_
  {
    TT_CMapRec  cmap;
    FT_Bool     valid;
    FT_ULong    cur_charcode;
    FT_UInt     cur_gindex;
    FT_ULong    cur_group;
    FT_ULong    num_groups;

  } TT_CMap12Rec, *TT_CMap12;


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_init( TT_CMap12  cmap,
                  FT_Byte*   table )
  {
    cmap->cmap.data  = table;

    table           += 12;
    cmap->num_groups = FT_PEEK_ULONG( table );

    cmap->valid      = 0;

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_validate( FT_Byte*      table,
                      FT_Validator  valid )
  {
    FT_Byte*   p;
    FT_ULong   length;
    FT_ULong   num_groups;


    if ( table + 16 > valid->limit )
      FT_INVALID_TOO_SHORT;

    p      = table + 4;
    length = TT_NEXT_ULONG( p );

    p          = table + 12;
    num_groups = TT_NEXT_ULONG( p );

    if ( table + length > valid->limit || length < 16 + 12 * num_groups )
      FT_INVALID_TOO_SHORT;

    /* check groups, they must be in increasing order */
    {
      FT_ULong  n, start, end, start_id, last = 0;


      for ( n = 0; n < num_groups; n++ )
      {
        start    = TT_NEXT_ULONG( p );
        end      = TT_NEXT_ULONG( p );
        start_id = TT_NEXT_ULONG( p );

        if ( start > end )
          FT_INVALID_DATA;

        if ( n > 0 && start <= last )
          FT_INVALID_DATA;

        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          if ( start_id + end - start >= TT_VALID_GLYPH_COUNT( valid ) )
            FT_INVALID_GLYPH_ID;
        }

        last = end;
      }
    }

    return SFNT_Err_Ok;
  }


  /* search the index of the charcode next to cmap->cur_charcode */
  /* cmap->cur_group should be set up properly by caller         */
  /*                                                             */
  static void
  tt_cmap12_next( TT_CMap12  cmap )
  {
    FT_Byte*  p;
    FT_ULong  start, end, start_id, char_code;
    FT_ULong  n;
    FT_UInt   gindex;


    if ( cmap->cur_charcode >= 0xFFFFFFFFUL )
      goto Fail;

    char_code = cmap->cur_charcode + 1;

    n = cmap->cur_group;

    for ( n = cmap->cur_group; n < cmap->num_groups; n++ )
    {
      p        = cmap->cmap.data + 16 + 12 * n;
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_PEEK_ULONG( p );

      if ( char_code < start )
        char_code = start;

      for ( ; char_code <= end; char_code++ )
      {
        gindex = (FT_UInt)( start_id + char_code - start );

        if ( gindex )
        {
          cmap->cur_charcode = char_code;;
          cmap->cur_gindex   = gindex;
          cmap->cur_group    = n;

          return;
        }
      }
    }

  Fail:
    cmap->valid = 0;
  }


  static FT_UInt
  tt_cmap12_char_map_binary( TT_CMap     cmap,
                             FT_UInt32*  pchar_code,
                             FT_Bool     next )
  {
    FT_UInt    gindex     = 0;
    FT_Byte*   p          = cmap->data + 12;
    FT_UInt32  num_groups = TT_PEEK_ULONG( p );
    FT_UInt32  char_code  = *pchar_code;
    FT_UInt32  start, end, start_id;
    FT_UInt32  max, min, mid;


    if ( !num_groups )
      return 0;

    /* make compiler happy */
    mid = num_groups;
    end = 0xFFFFFFFFUL;

    if ( next )
      char_code++;

    min = 0;
    max = num_groups;

    /* binary search */
    while ( min < max )
    {
      mid = ( min + max ) >> 1;
      p   = cmap->data + 16 + 12 * mid;

      start = TT_NEXT_ULONG( p );
      end   = TT_NEXT_ULONG( p );

      if ( char_code < start )
        max = mid;
      else if ( char_code > end )
        min = mid + 1;
      else
      {
        start_id = TT_PEEK_ULONG( p );
        gindex = (FT_UInt)( start_id + char_code - start );

        break;
      }
    }

    if ( next )
    {
      TT_CMap12  cmap12 = (TT_CMap12)cmap;


      /* if `char_code' is not in any group, then `mid' is */
      /* the group nearest to `char_code'                  */
      /*                                                   */

      if ( char_code > end )
      {
        mid++;
        if ( mid == num_groups )
          return 0;
      }

      cmap12->valid        = 1;
      cmap12->cur_charcode = char_code;
      cmap12->cur_group    = mid;

      if ( !gindex )
      {
        tt_cmap12_next( cmap12 );

        if ( cmap12->valid )
          gindex = cmap12->cur_gindex;
      }
      else
        cmap12->cur_gindex = gindex;

      if ( gindex )
        *pchar_code = cmap12->cur_charcode;
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap12_char_index( TT_CMap    cmap,
                        FT_UInt32  char_code )
  {
    return tt_cmap12_char_map_binary( cmap, &char_code, 0 );
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap12_char_next( TT_CMap     cmap,
                       FT_UInt32  *pchar_code )
  {
    TT_CMap12  cmap12 = (TT_CMap12)cmap;
    FT_ULong   gindex;


    if ( cmap12->cur_charcode >= 0xFFFFFFFFUL )
      return 0;

    /* no need to search */
    if ( cmap12->valid && cmap12->cur_charcode == *pchar_code )
    {
      tt_cmap12_next( cmap12 );
      if ( cmap12->valid )
      {
        gindex = cmap12->cur_gindex;
        if ( gindex )
          *pchar_code = cmap12->cur_charcode;
      }
      else
        gindex = 0;
    }
    else
      gindex = tt_cmap12_char_map_binary( cmap, pchar_code, 1 );

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_get_info( TT_CMap       cmap,
                      TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap12_class_rec =
  {
    {
      sizeof ( TT_CMap12Rec ),

      (FT_CMap_InitFunc)     tt_cmap12_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap12_char_index,
      (FT_CMap_CharNextFunc) tt_cmap12_char_next
    },
    12,
    (TT_CMap_ValidateFunc)   tt_cmap12_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap12_get_info
  };


#endif /* TT_CONFIG_CMAP_FORMAT_12 */


  static const TT_CMap_Class  tt_cmap_classes[] =
  {
#ifdef TT_CONFIG_CMAP_FORMAT_0
    &tt_cmap0_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_2
    &tt_cmap2_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_4
    &tt_cmap4_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_6
    &tt_cmap6_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_8
    &tt_cmap8_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_10
    &tt_cmap10_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_12
    &tt_cmap12_class_rec,
#endif

    NULL,
  };


  /* parse the `cmap' table and build the corresponding TT_CMap objects */
  /* in the current face                                                */
  /*                                                                    */
  FT_LOCAL_DEF( FT_Error )
  tt_face_build_cmaps( TT_Face  face )
  {
    FT_Byte*           table = face->cmap_table;
    FT_Byte*           limit = table + face->cmap_size;
    FT_UInt volatile   num_cmaps;
    FT_Byte* volatile  p     = table;


    if ( p + 4 > limit )
      return SFNT_Err_Invalid_Table;

    /* only recognize format 0 */
    if ( TT_NEXT_USHORT( p ) != 0 )
    {
      p -= 2;
      FT_ERROR(( "tt_face_build_cmaps: unsupported `cmap' table format = %d\n",
                 TT_PEEK_USHORT( p ) ));
      return SFNT_Err_Invalid_Table;
    }

    num_cmaps = TT_NEXT_USHORT( p );

    for ( ; num_cmaps > 0 && p + 8 <= limit; num_cmaps-- )
    {
      FT_CharMapRec  charmap;
      FT_UInt32      offset;


      charmap.platform_id = TT_NEXT_USHORT( p );
      charmap.encoding_id = TT_NEXT_USHORT( p );
      charmap.face        = FT_FACE( face );
      charmap.encoding    = FT_ENCODING_NONE;  /* will be filled later */
      offset              = TT_NEXT_ULONG( p );

      if ( offset && offset <= face->cmap_size - 2 )
      {
        FT_Byte*                       cmap   = table + offset;
        volatile FT_UInt               format = TT_PEEK_USHORT( cmap );
        const TT_CMap_Class* volatile  pclazz = tt_cmap_classes;
        TT_CMap_Class volatile         clazz;


        for ( ; *pclazz; pclazz++ )
        {
          clazz = *pclazz;
          if ( clazz->format == format )
          {
            volatile TT_ValidatorRec  valid;
            volatile FT_Error         error = SFNT_Err_Ok;


            ft_validator_init( FT_VALIDATOR( &valid ), cmap, limit,
                               FT_VALIDATE_DEFAULT );

            valid.num_glyphs = (FT_UInt)face->max_profile.numGlyphs;

            if ( ft_setjmp( FT_VALIDATOR( &valid )->jump_buffer ) == 0 )
            {
              /* validate this cmap sub-table */
              error = clazz->validate( cmap, FT_VALIDATOR( &valid ) );
            }

            if ( valid.validator.error == 0 )
            {
              FT_CMap  ttcmap;


              if ( !FT_CMap_New( (FT_CMap_Class)clazz,
                                 cmap, &charmap, &ttcmap ) )
              {
                /* it is simpler to directly set `flags' than adding */
                /* a parameter to FT_CMap_New                        */
                ((TT_CMap)ttcmap)->flags = (FT_Int)error;
              }
            }
            else
            {
              FT_ERROR(( "tt_face_build_cmaps:" ));
              FT_ERROR(( " broken cmap sub-table ignored!\n" ));
            }
            break;
          }
        }
      }
    }

    return SFNT_Err_Ok;
  }


  FT_LOCAL( FT_Error )
  tt_get_cmap_info( FT_CharMap    charmap,
                    TT_CMapInfo  *cmap_info )
  {
    FT_CMap        cmap  = (FT_CMap)charmap;
    TT_CMap_Class  clazz = (TT_CMap_Class)cmap->clazz;


    return clazz->get_cmap_info( charmap, cmap_info );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ttkern.c                                                               */
/*                                                                         */
/*    Load the basic TrueType kerning table.  This doesn't handle          */
/*    kerning data within the GPOS table at the moment.                    */
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
#include FT_TRUETYPE_TAGS_H
#include "ttkern.h"
#include "ttload.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttkern


#undef  TT_KERN_INDEX
#define TT_KERN_INDEX( g1, g2 )  ( ( (FT_ULong)(g1) << 16 ) | (g2) )


#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_ULong   table_size;
    FT_Byte*   p;
    FT_Byte*   p_limit;
    FT_UInt    nn, num_tables;
    FT_UInt32  avail = 0, ordered = 0;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, &table_size );
    if ( error )
      goto Exit;

    if ( table_size < 4 )  /* the case of a malformed table */
    {
      FT_ERROR(( "kerning table is too small - ignored\n" ));
      error = SFNT_Err_Table_Missing;
      goto Exit;
    }

    if ( FT_FRAME_EXTRACT( table_size, face->kern_table ) )
    {
      FT_ERROR(( "could not extract kerning table\n" ));
      goto Exit;
    }

    face->kern_table_size = table_size;

    p       = face->kern_table;
    p_limit = p + table_size;

    p         += 2; /* skip version */
    num_tables = FT_NEXT_USHORT( p );

    if ( num_tables > 32 ) /* we only support up to 32 sub-tables */
      num_tables = 32;

    for ( nn = 0; nn < num_tables; nn++ )
    {
      FT_UInt    num_pairs, /*version, */length, coverage;
      FT_Byte*   p_next;
      FT_UInt32  mask = 1UL << nn;


      if ( p + 6 > p_limit )
        break;

      p_next = p;

      /*version  = FT_NEXT_USHORT( p );*/
      length   = FT_NEXT_USHORT( p );
      coverage = FT_NEXT_USHORT( p );

      if ( length <= 6 )
        break;

      p_next += length;

      /* only use horizontal kerning tables */
      if ( ( coverage & ~8 ) != 0x0001 ||
           p + 8 > p_limit             )
        goto NextTable;

      num_pairs = FT_NEXT_USHORT( p );
      p        += 6;

      if ( p + 6 * num_pairs > p_limit )
        goto NextTable;

      avail |= mask;

      /*
       *  Now check whether the pairs in this table are ordered.
       *  We then can use binary search.
       */
      if ( num_pairs > 0 )
      {
        FT_UInt  count;
        FT_UInt  old_pair;


        old_pair = FT_NEXT_ULONG( p );
        p       += 2;

        for ( count = num_pairs - 1; count > 0; count-- )
        {
          FT_UInt32  cur_pair;


          cur_pair = FT_NEXT_ULONG( p );
          if ( cur_pair <= old_pair )
            break;

          p += 2;
          old_pair = cur_pair;
        }

        if ( count == 0 )
          ordered |= mask;
      }

    NextTable:
      p = p_next;
    }

    face->num_kern_tables = nn;
    face->kern_avail_bits = avail;
    face->kern_order_bits = ordered;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_kern( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;


    FT_FRAME_RELEASE( face->kern_table );
    face->kern_table_size = 0;
    face->num_kern_tables = 0;
    face->kern_avail_bits = 0;
    face->kern_order_bits = 0;
  }


  FT_LOCAL_DEF( FT_Int )
  tt_face_get_kerning( TT_Face  face,
                       FT_UInt  left_glyph,
                       FT_UInt  right_glyph )
  {
    FT_Int    result = 0;
    FT_UInt   count, mask = 1;
    FT_Byte*  p       = face->kern_table;


    p   += 4;
    mask = 0x0001;

    for ( count = face->num_kern_tables; count > 0; count--, mask <<= 1 )
    {
      FT_Byte* base     = p;
      FT_Byte* next     = base;
      FT_UInt  version  = FT_NEXT_USHORT( p );
      FT_UInt  length   = FT_NEXT_USHORT( p );
      FT_UInt  coverage = FT_NEXT_USHORT( p );
      FT_Int   value    = 0;

      FT_UNUSED( version );


      next = base + length;

      if ( ( face->kern_avail_bits & mask ) == 0 )
        goto NextTable;

      if ( p + 8 > next )
        goto NextTable;

      switch ( coverage >> 8 )
      {
      case 0:
        {
          FT_UInt   num_pairs = FT_NEXT_USHORT( p );
          FT_ULong  key0      = TT_KERN_INDEX( left_glyph, right_glyph );


          p += 6;

          if ( face->kern_order_bits & mask )   /* binary search */
          {
            FT_UInt   min = 0;
            FT_UInt   max = num_pairs;


            while ( min < max )
            {
              FT_UInt   mid = ( min + max ) >> 1;
              FT_Byte*  q   = p + 6 * mid;
              FT_ULong  key;


              key = FT_NEXT_ULONG( q );

              if ( key == key0 )
              {
                value = FT_PEEK_SHORT( q );
                goto Found;
              }
              if ( key < key0 )
                min = mid + 1;
              else
                max = mid;
            }
          }
          else /* linear search */
          {
            FT_UInt  count2;


            for ( count2 = num_pairs; count2 > 0; count2-- )
            {
              FT_ULong  key = FT_NEXT_ULONG( p );


              if ( key == key0 )
              {
                value = FT_PEEK_SHORT( p );
                goto Found;
              }
              p += 2;
            }
          }
        }
        break;

       /*
        *  We don't support format 2 because we haven't seen a single font
        *  using it in real life...
        */

      default:
        ;
      }

      goto NextTable;

    Found:
      if ( coverage & 8 ) /* overide or add */
        result = value;
      else
        result += value;

    NextTable:
      p = next;
    }

    return result;
  }

#else /* !OPTIMIZE_MEMORY */

  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b );


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt    n, num_tables;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, 0 );
    if ( error )
      return SFNT_Err_Ok;

    if ( FT_FRAME_ENTER( 4L ) )
      goto Exit;

    (void)FT_GET_USHORT();         /* version */
    num_tables = FT_GET_USHORT();

    FT_FRAME_EXIT();

    for ( n = 0; n < num_tables; n++ )
    {
      FT_UInt  coverage;
      FT_UInt  length;


      if ( FT_FRAME_ENTER( 6L ) )
        goto Exit;

      (void)FT_GET_USHORT();           /* version                 */
      length   = FT_GET_USHORT() - 6;  /* substract header length */
      coverage = FT_GET_USHORT();

      FT_FRAME_EXIT();

      if ( coverage == 0x0001 )
      {
        FT_UInt        num_pairs;
        TT_Kern0_Pair  pair;
        TT_Kern0_Pair  limit;


        /* found a horizontal format 0 kerning table! */
        if ( FT_FRAME_ENTER( 8L ) )
          goto Exit;

        num_pairs = FT_GET_USHORT();

        /* skip the rest */

        FT_FRAME_EXIT();

        /* allocate array of kerning pairs */
        if ( FT_QNEW_ARRAY( face->kern_pairs, num_pairs ) ||
             FT_FRAME_ENTER( 6L * num_pairs )             )
          goto Exit;

        pair  = face->kern_pairs;
        limit = pair + num_pairs;
        for ( ; pair < limit; pair++ )
        {
          pair->left  = FT_GET_USHORT();
          pair->right = FT_GET_USHORT();
          pair->value = FT_GET_USHORT();
        }

        FT_FRAME_EXIT();

        face->num_kern_pairs   = num_pairs;
        face->kern_table_index = n;

        /* ensure that the kerning pair table is sorted (yes, some */
        /* fonts have unsorted tables!)                            */

        if ( num_pairs > 0 )
        {
          TT_Kern0_Pair  pair0 = face->kern_pairs;
          FT_ULong       prev  = TT_KERN_INDEX( pair0->left, pair0->right );


          for ( pair0++; pair0 < limit; pair0++ )
          {
            FT_ULong  next = TT_KERN_INDEX( pair0->left, pair0->right );


            if ( next < prev )
              goto SortIt;

            prev = next;
          }
          goto Exit;

        SortIt:
          ft_qsort( (void*)face->kern_pairs, (int)num_pairs,
                    sizeof ( TT_Kern0_PairRec ), tt_kern_pair_compare );
        }

        goto Exit;
      }

      if ( FT_STREAM_SKIP( length ) )
        goto Exit;
    }

    /* no kern table found -- doesn't matter */
    face->kern_table_index = -1;
    face->num_kern_pairs   = 0;
    face->kern_pairs       = NULL;

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b )
  {
    TT_Kern0_Pair  pair1 = (TT_Kern0_Pair)a;
    TT_Kern0_Pair  pair2 = (TT_Kern0_Pair)b;

    FT_ULong  index1 = TT_KERN_INDEX( pair1->left, pair1->right );
    FT_ULong  index2 = TT_KERN_INDEX( pair2->left, pair2->right );

    return index1 < index2 ? -1
                           : ( index1 > index2 ? 1
                                               : 0 );
  }


  FT_LOCAL_DEF( void )
  tt_face_done_kern( TT_Face  face )
  {
    FT_Memory  memory = face->root.stream->memory;


    FT_FREE( face->kern_pairs );
    face->num_kern_pairs = 0;
  }


  FT_LOCAL_DEF( FT_Int )
  tt_face_get_kerning( TT_Face  face,
                       FT_UInt  left_glyph,
                       FT_UInt  right_glyph )
  {
    FT_Int         result = 0;
    TT_Kern0_Pair  pair;


    if ( face && face->kern_pairs )
    {
      /* there are some kerning pairs in this font file! */
      FT_ULong  search_tag = TT_KERN_INDEX( left_glyph, right_glyph );
      FT_Long   left, right;


      left  = 0;
      right = face->num_kern_pairs - 1;

      while ( left <= right )
      {
        FT_Long   middle = left + ( ( right - left ) >> 1 );
        FT_ULong  cur_pair;


        pair     = face->kern_pairs + middle;
        cur_pair = TT_KERN_INDEX( pair->left, pair->right );

        if ( cur_pair == search_tag )
          goto Found;

        if ( cur_pair < search_tag )
          left = middle + 1;
        else
          right = middle - 1;
      }
    }

  Exit:
    return result;

  Found:
    result = pair->value;
    goto Exit;
  }

#endif /* !OPTIMIZE_MEMORY */


#undef TT_KERN_INDEX

/* END */

/***************************************************************************/
/*                                                                         */
/*  sfobjs.c                                                               */
/*                                                                         */
/*    SFNT object management (base).                                       */
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
#include "sfobjs.h"
#include "ttload.h"
#include "ttcmap.h"
#include "ttkern.h"
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_DEBUG_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include "sferrors.h"

#ifdef TT_CONFIG_OPTION_BDF
#include "ttbdf.h"
#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_sfobjs



  /* convert a UTF-16 name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_utf16( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength / 2;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = FT_NEXT_USHORT( read );
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  /* convert a UCS-4 name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_ucs4( TT_NameEntry  entry,
                                 FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength / 4;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = (FT_UInt)FT_NEXT_ULONG( read );
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  /* convert an Apple Roman or symbol name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_other( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = *read++;
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  typedef FT_String*  (*TT_NameEntry_ConvertFunc)( TT_NameEntry  entry,
                                                   FT_Memory     memory );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_name                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a given ENGLISH name record in ASCII.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /*    nameid :: The name id of the name record to return.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Character string.  NULL if no name is present.                     */
  /*                                                                       */
  static FT_String*
  tt_face_get_name( TT_Face    face,
                    FT_UShort  nameid )
  {
    FT_Memory         memory = face->root.memory;
    FT_String*        result = NULL;
    FT_UShort         n;
    TT_NameEntryRec*  rec;
    FT_Int            found_apple   = -1;
    FT_Int            found_win     = -1;
    FT_Int            found_unicode = -1;

    FT_Bool           is_english = 0;

    TT_NameEntry_ConvertFunc  convert;


    rec = face->name_table.names;
    for ( n = 0; n < face->num_names; n++, rec++ )
    {
      /* According to the OpenType 1.3 specification, only Microsoft or  */
      /* Apple platform IDs might be used in the `name' table.  The      */
      /* `Unicode' platform is reserved for the `cmap' table, and the    */
      /* `Iso' one is deprecated.                                        */
      /*                                                                 */
      /* However, the Apple TrueType specification doesn't say the same  */
      /* thing and goes to suggest that all Unicode `name' table entries */
      /* should be coded in UTF-16 (in big-endian format I suppose).     */
      /*                                                                 */
      if ( rec->nameID == nameid && rec->stringLength > 0 )
      {
        switch ( rec->platformID )
        {
        case TT_PLATFORM_APPLE_UNICODE:
        case TT_PLATFORM_ISO:
          /* there is `languageID' to check there.  We should use this */
          /* field only as a last solution when nothing else is        */
          /* available.                                                */
          /*                                                           */
          found_unicode = n;
          break;

        case TT_PLATFORM_MACINTOSH:
          if ( rec->languageID == TT_MAC_LANGID_ENGLISH )
            found_apple = n;

          break;

        case TT_PLATFORM_MICROSOFT:
          /* we only take a non-English name when there is nothing */
          /* else available in the font                            */
          /*                                                       */
          if ( found_win == -1 || ( rec->languageID & 0x3FF ) == 0x009 )
          {
            switch ( rec->encodingID )
            {
            case TT_MS_ID_SYMBOL_CS:
            case TT_MS_ID_UNICODE_CS:
            case TT_MS_ID_UCS_4:
              is_english = FT_BOOL( ( rec->languageID & 0x3FF ) == 0x009 );
              found_win  = n;
              break;

            default:
              ;
            }
          }
          break;

        default:
          ;
        }
      }
    }

    /* some fonts contain invalid Unicode or Macintosh formatted entries; */
    /* we will thus favor names encoded in Windows formats if available   */
    /* (provided it is an English name)                                   */
    /*                                                                    */
    convert = NULL;
    if ( found_win >= 0 && !( found_apple >= 0 && !is_english ) )
    {
      rec = face->name_table.names + found_win;
      switch ( rec->encodingID )
      {
      case TT_MS_ID_UNICODE_CS:
      case TT_MS_ID_SYMBOL_CS:
        convert = tt_name_entry_ascii_from_utf16;
        break;

      case TT_MS_ID_UCS_4:
        convert = tt_name_entry_ascii_from_ucs4;
        break;

      default:
        ;
      }
    }
    else if ( found_apple >= 0 )
    {
      rec     = face->name_table.names + found_apple;
      convert = tt_name_entry_ascii_from_other;
    }
    else if ( found_unicode >= 0 )
    {
      rec     = face->name_table.names + found_unicode;
      convert = tt_name_entry_ascii_from_utf16;
    }

    if ( rec && convert )
    {
      if ( rec->string == NULL )
      {
        FT_Error   error  = SFNT_Err_Ok;
        FT_Stream  stream = face->name_table.stream;

        FT_UNUSED( error );


        if ( FT_QNEW_ARRAY ( rec->string, rec->stringLength ) ||
             FT_STREAM_SEEK( rec->stringOffset )              ||
             FT_STREAM_READ( rec->string, rec->stringLength ) )
        {
          FT_FREE( rec->string );
          rec->stringLength = 0;
          result            = NULL;
          goto Exit;
        }
      }

      result = convert( rec, memory );
    }

  Exit:
    return result;
  }


  static FT_Encoding
  sfnt_find_encoding( int  platform_id,
                      int  encoding_id )
  {
    typedef struct  TEncoding
    {
      int          platform_id;
      int          encoding_id;
      FT_Encoding  encoding;

    } TEncoding;

    static
    const TEncoding  tt_encodings[] =
    {
      { TT_PLATFORM_ISO,           -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_APPLE_UNICODE, -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_MACINTOSH,     TT_MAC_ID_ROMAN,     FT_ENCODING_APPLE_ROMAN },

      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SYMBOL_CS,  FT_ENCODING_MS_SYMBOL },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UCS_4,      FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS, FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,       FT_ENCODING_SJIS },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,     FT_ENCODING_GB2312 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,      FT_ENCODING_BIG5 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,    FT_ENCODING_WANSUNG },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,      FT_ENCODING_JOHAB }
    };

    const TEncoding  *cur, *limit;


    cur   = tt_encodings;
    limit = cur + sizeof ( tt_encodings ) / sizeof ( tt_encodings[0] );

    for ( ; cur < limit; cur++ )
    {
      if ( cur->platform_id == platform_id )
      {
        if ( cur->encoding_id == encoding_id ||
             cur->encoding_id == -1          )
          return cur->encoding;
      }
    }

    return FT_ENCODING_NONE;
  }


  /* Fill in face->ttc_header.  If the font is not a TTC, it is */
  /* synthesized into a TTC with one offset table.              */
  static FT_Error
  sfnt_open_font( FT_Stream  stream,
                  TT_Face    face )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error;
    FT_ULong   tag, offset;

    static const FT_Frame_Field  ttc_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TTC_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_LONG( version ),
        FT_FRAME_LONG( count   ),
      FT_FRAME_END
    };


    face->ttc_header.tag     = 0;
    face->ttc_header.version = 0;
    face->ttc_header.count   = 0;

    offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( tag ) )
      return error;

    if ( tag != 0x00010000UL                      &&
         tag != TTAG_ttcf                         &&
         tag != FT_MAKE_TAG( 'O', 'T', 'T', 'O' ) &&
         tag != TTAG_true                         &&
         tag != 0x00020000UL                      )
      return SFNT_Err_Unknown_File_Format;

    face->ttc_header.tag = TTAG_ttcf;

    if ( tag == TTAG_ttcf )
    {
      FT_Int  n;


      FT_TRACE3(( "sfnt_open_font: file is a collection\n" ));

      if ( FT_STREAM_READ_FIELDS( ttc_header_fields, &face->ttc_header ) )
        return error;

      /* now read the offsets of each font in the file */
      if ( FT_NEW_ARRAY( face->ttc_header.offsets, face->ttc_header.count ) )
        return error;

      if ( FT_FRAME_ENTER( face->ttc_header.count * 4L ) )
        return error;

      for ( n = 0; n < face->ttc_header.count; n++ )
        face->ttc_header.offsets[n] = FT_GET_ULONG();

      FT_FRAME_EXIT();
    }
    else
    {
      FT_TRACE3(( "sfnt_open_font: synthesize TTC\n" ));

      face->ttc_header.version = 1 << 16;
      face->ttc_header.count   = 1;

      if ( FT_NEW( face->ttc_header.offsets) )
        return error;

      face->ttc_header.offsets[0] = offset;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  sfnt_init_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error        error;
    FT_Library      library = face->root.driver->root.library;
    SFNT_Service    sfnt;


    /* for now, parameters are unused */
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    sfnt = (SFNT_Service)face->sfnt;
    if ( !sfnt )
    {
      sfnt = (SFNT_Service)FT_Get_Module_Interface( library, "sfnt" );
      if ( !sfnt )
        return SFNT_Err_Invalid_File_Format;

      face->sfnt       = sfnt;
      face->goto_table = sfnt->goto_table;
    }

    FT_FACE_FIND_GLOBAL_SERVICE( face, face->psnames, POSTSCRIPT_CMAPS );

    error = sfnt_open_font( stream, face );
    if ( error )
      return error;

    FT_TRACE2(( "sfnt_init_face: %08p, %ld\n", face, face_index ));

    if ( face_index < 0 )
      face_index = 0;

    if ( face_index >= face->ttc_header.count )
        return SFNT_Err_Bad_Argument;

    if ( FT_STREAM_SEEK( face->ttc_header.offsets[face_index] ) )
      return error;

    /* check that we have a valid TrueType file */
    error = sfnt->load_font_dir( face, stream );
    if ( error )
      return error;

    face->root.num_faces = face->ttc_header.count;

    return error;
  }


#define LOAD_( x )                                            \
  do {                                                        \
    FT_TRACE2(( "`" #x "' " ));                               \
    FT_TRACE3(( "-->\n" ));                                   \
                                                              \
    error = sfnt->load_##x( face, stream );                   \
                                                              \
    FT_TRACE2(( "%s\n", ( !error )                            \
                        ? "loaded"                            \
                        : ( error == SFNT_Err_Table_Missing ) \
                          ? "missing"                         \
                          : "failed to load" ));              \
    FT_TRACE3(( "\n" ));                                      \
  } while ( 0 )

#define LOADM_( x, vertical )                                 \
  do {                                                        \
    FT_TRACE2(( "`%s" #x "' ",                                \
                vertical ? "vertical " : "" ));               \
    FT_TRACE3(( "-->\n" ));                                   \
                                                              \
    error = sfnt->load_##x( face, stream, vertical );         \
                                                              \
    FT_TRACE2(( "%s\n", ( !error )                            \
                        ? "loaded"                            \
                        : ( error == SFNT_Err_Table_Missing ) \
                          ? "missing"                         \
                          : "failed to load" ));              \
    FT_TRACE3(( "\n" ));                                      \
  } while ( 0 )


  FT_LOCAL_DEF( FT_Error )
  sfnt_load_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES /* JE */
    FT_Error      psnames_error;
#endif

    FT_Error      error/*, psnames_error*/;
    FT_Bool       has_outline;
    FT_Bool       is_apple_sbit;

    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;

    FT_UNUSED( face_index );
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* Load tables */

    /* We now support two SFNT-based bitmapped font formats.  They */
    /* are recognized easily as they do not include a `glyf'       */
    /* table.                                                      */
    /*                                                             */
    /* The first format comes from Apple, and uses a table named   */
    /* `bhed' instead of `head' to store the font header (using    */
    /* the same format).  It also doesn't include horizontal and   */
    /* vertical metrics tables (i.e. `hhea' and `vhea' tables are  */
    /* missing).                                                   */
    /*                                                             */
    /* The other format comes from Microsoft, and is used with     */
    /* WinCE/PocketPC.  It looks like a standard TTF, except that  */
    /* it doesn't contain outlines.                                */
    /*                                                             */

    FT_TRACE2(( "sfnt_load_face: %08p\n\n", face ));

    /* do we have outlines in there? */
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    has_outline   = FT_BOOL( face->root.internal->incremental_interface != 0 ||
                             tt_face_lookup_table( face, TTAG_glyf )    != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )     != 0 );
#else
    has_outline   = FT_BOOL( tt_face_lookup_table( face, TTAG_glyf ) != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )  != 0 );
#endif

    is_apple_sbit = 0;

    /* if this font doesn't contain outlines, we try to load */
    /* a `bhed' table                                        */
    if ( !has_outline && sfnt->load_bhed )
    {
      LOAD_( bhed );
      is_apple_sbit = FT_BOOL( !error );
    }

    /* load the font header (`head' table) if this isn't an Apple */
    /* sbit font file                                             */
    if ( !is_apple_sbit )
    {
      LOAD_( head );
      if ( error )
        goto Exit;
    }

    if ( face->header.Units_Per_EM == 0 )
    {
      error = SFNT_Err_Invalid_Table;

      goto Exit;
    }

    /* the following tables are often not present in embedded TrueType */
    /* fonts within PDF documents, so don't check for them.            */
    LOAD_( maxp );
    LOAD_( cmap );

    /* the following tables are optional in PCL fonts -- */
    /* don't check for errors                            */
    LOAD_( name );
    LOAD_( post );
#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES /* JE */
    psnames_error = error;
#endif

    /* do not load the metrics headers and tables if this is an Apple */
    /* sbit font file                                                 */
    if ( !is_apple_sbit )
    {
      /* load the `hhea' and `hmtx' tables */
      LOADM_( hhea, 0 );
      if ( !error )
      {
        LOADM_( hmtx, 0 );
        if ( error == SFNT_Err_Table_Missing )
        {
          error = SFNT_Err_Hmtx_Table_Missing;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
          /* If this is an incrementally loaded font and there are */
          /* overriding metrics, tolerate a missing `hmtx' table.  */
          if ( face->root.internal->incremental_interface          &&
               face->root.internal->incremental_interface->funcs->
                 get_glyph_metrics                                 )
          {
            face->horizontal.number_Of_HMetrics = 0;
            error = SFNT_Err_Ok;
          }
#endif
        }
      }
      else if ( error == SFNT_Err_Table_Missing )
      {
        /* No `hhea' table necessary for SFNT Mac fonts. */
        if ( face->format_tag == TTAG_true )
        {
          FT_TRACE2(( "This is an SFNT Mac font.\n" ));
          has_outline = 0;
          error = SFNT_Err_Ok;
        }
        else
          error = SFNT_Err_Horiz_Header_Missing;
      }

      if ( error )
        goto Exit;

      /* try to load the `vhea' and `vmtx' tables */
      LOADM_( hhea, 1 );
      if ( !error )
      {
        LOADM_( hmtx, 1 );
        if ( !error )
          face->vertical_info = 1;
      }

      if ( error && error != SFNT_Err_Table_Missing )
        goto Exit;

      LOAD_( os2 );
      if ( error )
      {
        if ( error != SFNT_Err_Table_Missing )
          goto Exit;

        face->os2.version = 0xFFFFU;
      }

    }

    /* the optional tables */

    /* embedded bitmap support. */
    if ( sfnt->load_eblc )
    {
      LOAD_( eblc );
      if ( error )
      {
        /* return an error if this font file has no outlines */
        if ( error == SFNT_Err_Table_Missing && has_outline )
          error = SFNT_Err_Ok;
        else
          goto Exit;
      }
    }

    LOAD_( pclt );
    if ( error )
    {
      if ( error != SFNT_Err_Table_Missing )
        goto Exit;

      face->pclt.Version = 0;
    }

    /* consider the kerning and gasp tables as optional */
    LOAD_( gasp );
    LOAD_( kern );

    error = SFNT_Err_Ok;

    face->root.num_glyphs = face->max_profile.numGlyphs;

    face->root.family_name = tt_face_get_name( face,
                                               TT_NAME_ID_PREFERRED_FAMILY );
    if ( !face->root.family_name )
      face->root.family_name = tt_face_get_name( face,
                                                 TT_NAME_ID_FONT_FAMILY );

    face->root.style_name = tt_face_get_name( face,
                                              TT_NAME_ID_PREFERRED_SUBFAMILY );
    if ( !face->root.style_name )
      face->root.style_name  = tt_face_get_name( face,
                                                 TT_NAME_ID_FONT_SUBFAMILY );

    /* now set up root fields */
    {
      FT_Face    root = &face->root;
      FT_Int32   flags = root->face_flags;


      /*********************************************************************/
      /*                                                                   */
      /* Compute face flags.                                               */
      /*                                                                   */
      if ( has_outline == TRUE )
        flags |= FT_FACE_FLAG_SCALABLE;   /* scalable outlines */

      /* The sfnt driver only supports bitmap fonts natively, thus we */
      /* don't set FT_FACE_FLAG_HINTER.                               */
      flags |= FT_FACE_FLAG_SFNT       |  /* SFNT file format  */
               FT_FACE_FLAG_HORIZONTAL;   /* horizontal data   */

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
      if ( psnames_error == SFNT_Err_Ok &&
           face->postscript.FormatType != 0x00030000L )
        flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      /* fixed width font? */
      if ( face->postscript.isFixedPitch )
        flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* vertical information? */
      if ( face->vertical_info )
        flags |= FT_FACE_FLAG_VERTICAL;

      /* kerning available ? */
      if ( TT_FACE_HAS_KERNING( face ) )
        flags |= FT_FACE_FLAG_KERNING;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
      /* Don't bother to load the tables unless somebody asks for them. */
      /* No need to do work which will (probably) not be used.          */
      if ( tt_face_lookup_table( face, TTAG_glyf ) != 0 &&
           tt_face_lookup_table( face, TTAG_fvar ) != 0 &&
           tt_face_lookup_table( face, TTAG_gvar ) != 0 )
        flags |= FT_FACE_FLAG_MULTIPLE_MASTERS;
#endif

      root->face_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Compute style flags.                                              */
      /*                                                                   */
      flags = 0;
      if ( has_outline == TRUE && face->os2.version != 0xFFFFU )
      {
        /* we have an OS/2 table; use the `fsSelection' field */
        if ( face->os2.fsSelection & 1 )
          flags |= FT_STYLE_FLAG_ITALIC;

        if ( face->os2.fsSelection & 32 )
          flags |= FT_STYLE_FLAG_BOLD;
      }
      else
      {
        /* this is an old Mac font, use the header field */
        if ( face->header.Mac_Style & 1 )
          flags |= FT_STYLE_FLAG_BOLD;

        if ( face->header.Mac_Style & 2 )
          flags |= FT_STYLE_FLAG_ITALIC;
      }

      root->style_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Polish the charmaps.                                              */
      /*                                                                   */
      /*   Try to set the charmap encoding according to the platform &     */
      /*   encoding ID of each charmap.                                    */
      /*                                                                   */

      tt_face_build_cmaps( face );  /* ignore errors */


      /* set the encoding fields */
      {
        FT_Int  m;


        for ( m = 0; m < root->num_charmaps; m++ )
        {
          FT_CharMap  charmap = root->charmaps[m];


          charmap->encoding = sfnt_find_encoding( charmap->platform_id,
                                                  charmap->encoding_id );

#if 0
          if ( root->charmap     == NULL &&
               charmap->encoding == FT_ENCODING_UNICODE )
          {
            /* set 'root->charmap' to the first Unicode encoding we find */
            root->charmap = charmap;
          }
#endif
        }
      }


      /*********************************************************************/
      /*                                                                   */
      /*  Set up metrics.                                                  */
      /*                                                                   */
      if ( has_outline == TRUE )
      {
        /* XXX What about if outline header is missing */
        /*     (e.g. sfnt wrapped bitmap)?             */
        root->bbox.xMin    = face->header.xMin;
        root->bbox.yMin    = face->header.yMin;
        root->bbox.xMax    = face->header.xMax;
        root->bbox.yMax    = face->header.yMax;
        root->units_per_EM = face->header.Units_Per_EM;


        /* XXX: Computing the ascender/descender/height is very different */
        /*      from what the specification tells you.  Apparently, we    */
        /*      must be careful because                                   */
        /*                                                                */
        /*      - not all fonts have an OS/2 table; in this case, we take */
        /*        the values in the horizontal header.  However, these    */
        /*        values very often are not reliable.                     */
        /*                                                                */
        /*      - otherwise, the correct typographic values are in the    */
        /*        sTypoAscender, sTypoDescender & sTypoLineGap fields.    */
        /*                                                                */
        /*        However, certains fonts have these fields set to 0.     */
        /*        Rather, they have usWinAscent & usWinDescent correctly  */
        /*        set (but with different values).                        */
        /*                                                                */
        /*      As an example, Arial Narrow is implemented through four   */
        /*      files ARIALN.TTF, ARIALNI.TTF, ARIALNB.TTF & ARIALNBI.TTF */
        /*                                                                */
        /*      Strangely, all fonts have the same values in their        */
        /*      sTypoXXX fields, except ARIALNB which sets them to 0.     */
        /*                                                                */
        /*      On the other hand, they all have different                */
        /*      usWinAscent/Descent values -- as a conclusion, the OS/2   */
        /*      table cannot be used to compute the text height reliably! */
        /*                                                                */

        /* The ascender/descender/height are computed from the OS/2 table */
        /* when found.  Otherwise, they're taken from the horizontal      */
        /* header.                                                        */
        /*                                                                */

        root->ascender  = face->horizontal.Ascender;
        root->descender = face->horizontal.Descender;

        root->height    = (FT_Short)( root->ascender - root->descender +
                                      face->horizontal.Line_Gap );

#if 0
        /* if the line_gap is 0, we add an extra 15% to the text height --  */
        /* this computation is based on various versions of Times New Roman */
        if ( face->horizontal.Line_Gap == 0 )
          root->height = (FT_Short)( ( root->height * 115 + 50 ) / 100 );
#endif

#if 0

        /* some fonts have the OS/2 "sTypoAscender", "sTypoDescender" & */
        /* "sTypoLineGap" fields set to 0, like ARIALNB.TTF             */
        if ( face->os2.version != 0xFFFFU && root->ascender )
        {
          FT_Int  height;


          root->ascender  =  face->os2.sTypoAscender;
          root->descender = -face->os2.sTypoDescender;

          height = root->ascender + root->descender + face->os2.sTypoLineGap;
          if ( height > root->height )
            root->height = height;
        }

#endif /* 0 */

        root->max_advance_width   = face->horizontal.advance_Width_Max;

        root->max_advance_height  = (FT_Short)( face->vertical_info
                                      ? face->vertical.advance_Height_Max
                                      : root->height );

        root->underline_position  = face->postscript.underlinePosition;
        root->underline_thickness = face->postscript.underlineThickness;
      }

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

      /*
       *  Now allocate the root array of FT_Bitmap_Size records and
       *  populate them.  Unfortunately, it isn't possible to indicate bit
       *  depths in the FT_Bitmap_Size record.  This is a design error.
       */
      {
        FT_UInt  i, count;


#if defined FT_OPTIMIZE_MEMORY && !defined FT_CONFIG_OPTION_OLD_INTERNALS
        count = face->sbit_num_strikes;
#else
        count = (FT_UInt)face->num_sbit_strikes;
#endif

        if ( count > 0 )
        {
          FT_Memory        memory   = face->root.stream->memory;
          FT_UShort        em_size  = face->header.Units_Per_EM;
          FT_Short         avgwidth = face->os2.xAvgCharWidth;
          FT_Size_Metrics  metrics;


          if ( em_size == 0 || face->os2.version == 0xFFFFU )
          {
            avgwidth = 0;
            em_size = 1;
          }

          if ( FT_NEW_ARRAY( root->available_sizes, count ) )
            goto Exit;

          for ( i = 0; i < count; i++ )
          {
            FT_Bitmap_Size*  bsize = root->available_sizes + i;


            error = sfnt->load_strike_metrics( face, i, &metrics );
            if ( error )
              goto Exit;

            bsize->height = (FT_Short)( metrics.height >> 6 );
            bsize->width = (FT_Short)(
                ( avgwidth * metrics.x_ppem + em_size / 2 ) / em_size );

            bsize->x_ppem = metrics.x_ppem << 6;
            bsize->y_ppem = metrics.y_ppem << 6;

            /* assume 72dpi */
            bsize->size   = metrics.y_ppem << 6;
          }

          root->face_flags     |= FT_FACE_FLAG_FIXED_SIZES;
          root->num_fixed_sizes = (FT_Int)count;
        }
      }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    }

  Exit:
    FT_TRACE2(( "sfnt_load_face: done\n" ));

    return error;
  }


#undef LOAD_
#undef LOADM_


  FT_LOCAL_DEF( void )
  sfnt_done_face( TT_Face  face )
  {
    FT_Memory     memory = face->root.memory;
    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    if ( sfnt )
    {
      /* destroy the postscript names table if it is loaded */
      if ( sfnt->free_psnames )
        sfnt->free_psnames( face );

      /* destroy the embedded bitmaps table if it is loaded */
      if ( sfnt->free_eblc )
        sfnt->free_eblc( face );
    }

#ifdef TT_CONFIG_OPTION_BDF
    /* freeing the embedded BDF properties */
    tt_face_free_bdf_props( face );
#endif

    /* freeing the kerning table */
    tt_face_done_kern( face );

    /* freeing the collection table */
    FT_FREE( face->ttc_header.offsets );
    face->ttc_header.count = 0;

    /* freeing table directory */
    FT_FREE( face->dir_tables );
    face->num_tables = 0;

    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      /* simply release the 'cmap' table frame */
      FT_FRAME_RELEASE( face->cmap_table );
      face->cmap_size = 0;
    }

    /* freeing the horizontal metrics */
#if defined FT_OPTIMIZE_MEMORY && !defined FT_CONFIG_OPTION_OLD_INTERNALS
    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      FT_FRAME_RELEASE( face->horz_metrics );
      FT_FRAME_RELEASE( face->vert_metrics );
      face->horz_metrics_size = 0;
      face->vert_metrics_size = 0;
    }
#else
    FT_FREE( face->horizontal.long_metrics );
    FT_FREE( face->horizontal.short_metrics );
#endif

    /* freeing the vertical ones, if any */
    if ( face->vertical_info )
    {
      FT_FREE( face->vertical.long_metrics  );
      FT_FREE( face->vertical.short_metrics );
      face->vertical_info = 0;
    }

    /* freeing the gasp table */
    FT_FREE( face->gasp.gaspRanges );
    face->gasp.numRanges = 0;

    /* freeing the name table */
    sfnt->free_name( face );

    /* freeing family and style name */
    FT_FREE( face->root.family_name );
    FT_FREE( face->root.style_name );

    /* freeing sbit size table */
    FT_FREE( face->root.available_sizes );
    face->root.num_fixed_sizes = 0;

    FT_FREE( face->postscript_name );

    face->sfnt = 0;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  sfdriver.c                                                             */
/*                                                                         */
/*    High-level SFNT driver interface (body).                             */
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
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_OBJECTS_H

#include "sfdriver.h"
#include "ttload.h"
#include "sfobjs.h"

#include "sferrors.h"

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#include "ttsbit.h"
#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
#include "ttpost.h"
#endif

#ifdef TT_CONFIG_OPTION_BDF
#include "ttbdf.h"
#include FT_SERVICE_BDF_H
#endif

#include "ttcmap.h"
#include "ttkern.h"
#include "ttmtx.h"

#include FT_SERVICE_GLYPH_DICT_H
#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_SFNT_H
#include FT_SERVICE_TT_CMAP_H


 /*
  *  SFNT TABLE SERVICE
  *
  */

  static void*
  get_sfnt_table( TT_Face      face,
                  FT_Sfnt_Tag  tag )
  {
    void*  table;


    switch ( tag )
    {
    case ft_sfnt_head:
      table = &face->header;
      break;

    case ft_sfnt_hhea:
      table = &face->horizontal;
      break;

    case ft_sfnt_vhea:
      table = face->vertical_info ? &face->vertical : 0;
      break;

    case ft_sfnt_os2:
      table = face->os2.version == 0xFFFFU ? 0 : &face->os2;
      break;

    case ft_sfnt_post:
      table = &face->postscript;
      break;

    case ft_sfnt_maxp:
      table = &face->max_profile;
      break;

    case ft_sfnt_pclt:
      table = face->pclt.Version ? &face->pclt : 0;
      break;

    default:
      table = 0;
    }

    return table;
  }


  static FT_Error
  sfnt_table_info( TT_Face    face,
                   FT_UInt    idx,
                   FT_ULong  *tag,
                   FT_ULong  *length )
  {
    if ( !tag || !length )
      return SFNT_Err_Invalid_Argument;

    if ( idx >= face->num_tables )
      return SFNT_Err_Table_Missing;

    *tag    = face->dir_tables[idx].Tag;
    *length = face->dir_tables[idx].Length;

    return SFNT_Err_Ok;
  }


  static const FT_Service_SFNT_TableRec  sfnt_service_sfnt_table =
  {
    (FT_SFNT_TableLoadFunc)tt_face_load_any,
    (FT_SFNT_TableGetFunc) get_sfnt_table,
    (FT_SFNT_TableInfoFunc)sfnt_table_info
  };


#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES

 /*
  *  GLYPH DICT SERVICE
  *
  */

  static FT_Error
  sfnt_get_glyph_name( TT_Face     face,
                       FT_UInt     glyph_index,
                       FT_Pointer  buffer,
                       FT_UInt     buffer_max )
  {
    FT_String*  gname;
    FT_Error    error;


    error = tt_face_get_ps_name( face, glyph_index, &gname );
    if ( !error && buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)( ft_strlen( gname ) );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    return error;
  }


  static const FT_Service_GlyphDictRec  sfnt_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  sfnt_get_glyph_name,
    (FT_GlyphDict_NameIndexFunc)NULL
  };

#endif /* TT_CONFIG_OPTION_POSTSCRIPT_NAMES */


 /*
  *  POSTSCRIPT NAME SERVICE
  *
  */

  static const char*
  sfnt_get_ps_name( TT_Face  face )
  {
    FT_Int       n, found_win, found_apple;
    const char*  result = NULL;


    /* shouldn't happen, but just in case to avoid memory leaks */
    if ( face->postscript_name )
      return face->postscript_name;

    /* scan the name table to see whether we have a Postscript name here, */
    /* either in Macintosh or Windows platform encodings                  */
    found_win   = -1;
    found_apple = -1;

    for ( n = 0; n < face->num_names; n++ )
    {
      TT_NameEntryRec*  name = face->name_table.names + n;


      if ( name->nameID == 6 && name->stringLength > 0 )
      {
        if ( name->platformID == 3     &&
             name->encodingID == 1     &&
             name->languageID == 0x409 )
          found_win = n;

        if ( name->platformID == 1 &&
             name->encodingID == 0 &&
             name->languageID == 0 )
          found_apple = n;
      }
    }

    if ( found_win != -1 )
    {
      FT_Memory         memory = face->root.memory;
      TT_NameEntryRec*  name   = face->name_table.names + found_win;
      FT_UInt           len    = name->stringLength / 2;
      FT_Error          error  = SFNT_Err_Ok;

      FT_UNUSED( error );


      if ( !FT_ALLOC( result, name->stringLength + 1 ) )
      {
        FT_Stream   stream = face->name_table.stream;
        FT_String*  r      = (FT_String*)result;
        FT_Byte*    p      = (FT_Byte*)name->string;


        if ( FT_STREAM_SEEK( name->stringOffset ) ||
             FT_FRAME_ENTER( name->stringLength ) )
        {
          FT_FREE( result );
          name->stringLength = 0;
          name->stringOffset = 0;
          FT_FREE( name->string );

          goto Exit;
        }

        p = (FT_Byte*)stream->cursor;

        for ( ; len > 0; len--, p += 2 )
        {
          if ( p[0] == 0 && p[1] >= 32 && p[1] < 128 )
            *r++ = p[1];
        }
        *r = '\0';

        FT_FRAME_EXIT();
      }
      goto Exit;
    }

    if ( found_apple != -1 )
    {
      FT_Memory         memory = face->root.memory;
      TT_NameEntryRec*  name   = face->name_table.names + found_apple;
      FT_UInt           len    = name->stringLength;
      FT_Error          error  = SFNT_Err_Ok;

      FT_UNUSED( error );


      if ( !FT_ALLOC( result, len + 1 ) )
      {
        FT_Stream  stream = face->name_table.stream;


        if ( FT_STREAM_SEEK( name->stringOffset ) ||
             FT_STREAM_READ( result, len )        )
        {
          name->stringOffset = 0;
          name->stringLength = 0;
          FT_FREE( name->string );
          FT_FREE( result );
          goto Exit;
        }
        ((char*)result)[len] = '\0';
      }
    }

  Exit:
    face->postscript_name = result;
    return result;
  }

  static const FT_Service_PsFontNameRec  sfnt_service_ps_name =
  {
    (FT_PsName_GetFunc)sfnt_get_ps_name
  };


  /*
   *  TT CMAP INFO
   */
  static const FT_Service_TTCMapsRec  tt_service_get_cmap_info =
  {
    (TT_CMap_Info_GetFunc)tt_get_cmap_info
  };


#ifdef TT_CONFIG_OPTION_BDF

  static FT_Error
  sfnt_get_charset_id( TT_Face       face,
                       const char*  *acharset_encoding,
                       const char*  *acharset_registry )
  {
    BDF_PropertyRec  encoding, registry;
    FT_Error         error;


    /* XXX: I don't know whether this is correct, since
     *      tt_face_find_bdf_prop only returns something correct if we have
     *      previously selected a size that is listed in the BDF table.
     *      Should we change the BDF table format to include single offsets
     *      for `CHARSET_REGISTRY' and `CHARSET_ENCODING'?
     */
    error = tt_face_find_bdf_prop( face, "CHARSET_REGISTRY", &registry );
    if ( !error )
    {
      error = tt_face_find_bdf_prop( face, "CHARSET_ENCODING", &encoding );
      if ( !error )
      {
        if ( registry.type == BDF_PROPERTY_TYPE_ATOM &&
             encoding.type == BDF_PROPERTY_TYPE_ATOM )
        {
          *acharset_encoding = encoding.u.atom;
          *acharset_registry = registry.u.atom;
        }
        else
          error = FT_Err_Invalid_Argument;
      }
    }

    return error;
  }


  static const FT_Service_BDFRec  sfnt_service_bdf =
  {
    (FT_BDF_GetCharsetIdFunc) sfnt_get_charset_id,
    (FT_BDF_GetPropertyFunc)  tt_face_find_bdf_prop,
  };

#endif /* TT_CONFIG_OPTION_BDF */


  /*
   *  SERVICE LIST
   */

  static const FT_ServiceDescRec  sfnt_services[] =
  {
    { FT_SERVICE_ID_SFNT_TABLE,           &sfnt_service_sfnt_table },
    { FT_SERVICE_ID_POSTSCRIPT_FONT_NAME, &sfnt_service_ps_name },
#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
    { FT_SERVICE_ID_GLYPH_DICT,           &sfnt_service_glyph_dict },
#endif
#ifdef TT_CONFIG_OPTION_BDF
    { FT_SERVICE_ID_BDF,                  &sfnt_service_bdf },
#endif
    { FT_SERVICE_ID_TT_CMAP,              &tt_service_get_cmap_info },

    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  sfnt_get_interface( FT_Module    module,
                      const char*  module_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( sfnt_services, module_interface );
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_CALLBACK_DEF( FT_Error )
  tt_face_load_sfnt_header_stub( TT_Face      face,
                                 FT_Stream    stream,
                                 FT_Long      face_index,
                                 SFNT_Header  header )
  {
    FT_UNUSED( face );
    FT_UNUSED( stream );
    FT_UNUSED( face_index );
    FT_UNUSED( header );

    return FT_Err_Unimplemented_Feature;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_face_load_directory_stub( TT_Face      face,
                               FT_Stream    stream,
                               SFNT_Header  header )
  {
    FT_UNUSED( face );
    FT_UNUSED( stream );
    FT_UNUSED( header );

    return FT_Err_Unimplemented_Feature;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_face_load_hdmx_stub( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_UNUSED( face );
    FT_UNUSED( stream );
    
    return FT_Err_Unimplemented_Feature;
  }                          


  FT_CALLBACK_DEF( void )
  tt_face_free_hdmx_stub( TT_Face  face )
  {
    FT_UNUSED( face );
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_face_set_sbit_strike_stub( TT_Face    face,
                                FT_UInt    x_ppem,
                                FT_UInt    y_ppem,
                                FT_ULong*  astrike_index )
  {
    /*
     * We simply forge a FT_Size_Request and call the real function
     * that does all the work.
     *
     * This stub might be called by libXfont in the X.Org Xserver,
     * compiled against version 2.1.8 or newer.
     */

    FT_Size_RequestRec  req;


    req.type           = FT_SIZE_REQUEST_TYPE_NOMINAL;
    req.width          = (FT_F26Dot6)x_ppem;
    req.height         = (FT_F26Dot6)y_ppem;
    req.horiResolution = 0;
    req.vertResolution = 0;

    *astrike_index = 0x7FFFFFFFUL;

    return tt_face_set_sbit_strike( face, &req, astrike_index );    
  }                                


  FT_CALLBACK_DEF( FT_Error )
  tt_face_load_sbit_stub( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_UNUSED( face );
    FT_UNUSED( stream );
    
    /*
     *  This function was originally implemented to load the sbit table. 
     *  However, it has been replaced by `tt_face_load_eblc', and this stub
     *  is only there for some rogue clients which would want to call it
     *  directly (which doesn't make much sense).
     */
    return FT_Err_Unimplemented_Feature;
  }                          


  FT_CALLBACK_DEF( void )
  tt_face_free_sbit_stub( TT_Face  face )
  {
    /* nothing to do in this stub */
    FT_UNUSED( face );
  }
  
  
  FT_CALLBACK_DEF( FT_Error )
  tt_face_load_charmap_stub( TT_Face    face,
                             void*      cmap,
                             FT_Stream  input )
  {
    FT_UNUSED( face );
    FT_UNUSED( cmap );
    FT_UNUSED( input );
    
    return FT_Err_Unimplemented_Feature;
  }                             


  FT_CALLBACK_DEF( FT_Error )
  tt_face_free_charmap_stub( TT_Face  face,
                             void*    cmap )
  {
    FT_UNUSED( face );
    FT_UNUSED( cmap );
    
    return 0;
  }                             
  
#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


  static
  const SFNT_Interface  sfnt_interface =
  {
    tt_face_goto_table,

    sfnt_init_face,
    sfnt_load_face,
    sfnt_done_face,
    sfnt_get_interface,

    tt_face_load_any,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    tt_face_load_sfnt_header_stub,
    tt_face_load_directory_stub,
#endif

    tt_face_load_head,
    tt_face_load_hhea,
    tt_face_load_cmap,
    tt_face_load_maxp,
    tt_face_load_os2,
    tt_face_load_post,

    tt_face_load_name,
    tt_face_free_name,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    tt_face_load_hdmx_stub,
    tt_face_free_hdmx_stub,
#endif

    tt_face_load_kern,
    tt_face_load_gasp,
    tt_face_load_pclt,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    /* see `ttload.h' */
    tt_face_load_bhed,
#else
    0,
#endif

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    tt_face_set_sbit_strike_stub,
    tt_face_load_sbit_stub,

    tt_find_sbit_image,
    tt_load_sbit_metrics,
#endif

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    tt_face_load_sbit_image,
#else
    0,
#endif

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    tt_face_free_sbit_stub,
#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
    /* see `ttpost.h' */
    tt_face_get_ps_name,
    tt_face_free_ps_names,
#else
    0,
    0,
#endif

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    tt_face_load_charmap_stub,
    tt_face_free_charmap_stub,
#endif

    /* since version 2.1.8 */

    tt_face_get_kerning,

    /* since version 2.2 */

    tt_face_load_font_dir,
    tt_face_load_hmtx,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    /* see `ttsbit.h' and `sfnt.h' */
    tt_face_load_eblc,
    tt_face_free_eblc,

    tt_face_set_sbit_strike,
    tt_face_load_strike_metrics,
#else
    0,
    0,
    0,
    0,
#endif    

    tt_face_get_metrics
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  sfnt_module_class =
  {
    0,  /* not a font driver or renderer */
    sizeof( FT_ModuleRec ),

    "sfnt",     /* driver name                            */
    0x10000L,   /* driver version 1.0                     */
    0x20000L,   /* driver requires FreeType 2.0 or higher */

    (const void*)&sfnt_interface,  /* module specific interface */

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  sfnt_get_interface
  };


/* END */


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
/***************************************************************************/
/*                                                                         */
/*  ttsbit.c                                                               */
/*                                                                         */
/*    TrueType and OpenType embedded bitmap support (body).                */
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
#include FT_TRUETYPE_TAGS_H

  /*
   *  Alas, the memory-optimized sbit loader can't be used when implementing
   *  the `old internals' hack
   */
#if defined FT_OPTIMIZE_MEMORY && !defined FT_CONFIG_OPTION_OLD_INTERNALS

#include "ttsbit0.c"

#else /* !OPTIMIZE_MEMORY || OLD_INTERNALS */

#include "ft2build.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttsbit.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttsbit


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    blit_sbit                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Blits a bitmap from an input stream into a given target.  Supports */
  /*    x and y offsets as well as byte padded lines.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    target      :: The target bitmap/pixmap.                           */
  /*                                                                       */
  /*    source      :: The input packed bitmap data.                       */
  /*                                                                       */
  /*    line_bits   :: The number of bits per line.                        */
  /*                                                                       */
  /*    byte_padded :: A flag which is true if lines are byte-padded.      */
  /*                                                                       */
  /*    x_offset    :: The horizontal offset.                              */
  /*                                                                       */
  /*    y_offset    :: The vertical offset.                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    IMPORTANT: The x and y offsets are relative to the top corner of   */
  /*               the target bitmap (unlike the normal TrueType           */
  /*               convention).  A positive y offset indicates a downwards */
  /*               direction!                                              */
  /*                                                                       */
  static void
  blit_sbit( FT_Bitmap*  target,
             FT_Byte*    source,
             FT_Int      line_bits,
             FT_Bool     byte_padded,
             FT_Int      x_offset,
             FT_Int      y_offset )
  {
    FT_Byte*   line_buff;
    FT_Int     line_incr;
    FT_Int     height;

    FT_UShort  acc;
    FT_UInt    loaded;


    /* first of all, compute starting write position */
    line_incr = target->pitch;
    line_buff = target->buffer;

    if ( line_incr < 0 )
      line_buff -= line_incr * ( target->rows - 1 );

    line_buff += ( x_offset >> 3 ) + y_offset * line_incr;

    /***********************************************************************/
    /*                                                                     */
    /* We use the extra-classic `accumulator' trick to extract the bits    */
    /* from the source byte stream.                                        */
    /*                                                                     */
    /* Namely, the variable `acc' is a 16-bit accumulator containing the   */
    /* last `loaded' bits from the input stream.  The bits are shifted to  */
    /* the upmost position in `acc'.                                       */
    /*                                                                     */
    /***********************************************************************/

    acc    = 0;  /* clear accumulator   */
    loaded = 0;  /* no bits were loaded */

    for ( height = target->rows; height > 0; height-- )
    {
      FT_Byte*  cur   = line_buff;        /* current write cursor          */
      FT_Int    count = line_bits;        /* # of bits to extract per line */
      FT_Byte   shift = (FT_Byte)( x_offset & 7 ); /* current write shift  */
      FT_Byte   space = (FT_Byte)( 8 - shift );


      /* first of all, read individual source bytes */
      if ( count >= 8 )
      {
        count -= 8;
        {
          do
          {
            FT_Byte  val;


            /* ensure that there are at least 8 bits in the accumulator */
            if ( loaded < 8 )
            {
              acc    |= (FT_UShort)((FT_UShort)*source++ << ( 8 - loaded ));
              loaded += 8;
            }

            /* now write one byte */
            val = (FT_Byte)( acc >> 8 );
            if ( shift )
            {
              cur[0] |= (FT_Byte)( val >> shift );
              cur[1] |= (FT_Byte)( val << space );
            }
            else
              cur[0] |= val;

            cur++;
            acc   <<= 8;  /* remove bits from accumulator */
            loaded -= 8;
            count  -= 8;

          } while ( count >= 0 );
        }

        /* restore `count' to correct value */
        count += 8;
      }

      /* now write remaining bits (count < 8) */
      if ( count > 0 )
      {
        FT_Byte  val;


        /* ensure that there are at least `count' bits in the accumulator */
        if ( (FT_Int)loaded < count )
        {
          acc    |= (FT_UShort)((FT_UShort)*source++ << ( 8 - loaded ));
          loaded += 8;
        }

        /* now write remaining bits */
        val     = (FT_Byte)( ( (FT_Byte)( acc >> 8 ) ) & ~( 0xFF >> count ) );
        cur[0] |= (FT_Byte)( val >> shift );

        if ( count > space )
          cur[1] |= (FT_Byte)( val << space );

        acc   <<= count;
        loaded -= count;
      }

      /* now, skip to next line */
      if ( byte_padded )
      {
        acc    = 0;
        loaded = 0;   /* clear accumulator on byte-padded lines */
      }

      line_buff += line_incr;
    }
  }


  static const FT_Frame_Field  sbit_metrics_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_MetricsRec

    FT_FRAME_START( 8 ),
      FT_FRAME_BYTE( height ),
      FT_FRAME_BYTE( width ),

      FT_FRAME_CHAR( horiBearingX ),
      FT_FRAME_CHAR( horiBearingY ),
      FT_FRAME_BYTE( horiAdvance ),

      FT_FRAME_CHAR( vertBearingX ),
      FT_FRAME_CHAR( vertBearingY ),
      FT_FRAME_BYTE( vertAdvance ),
    FT_FRAME_END
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Const_Metrics                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the metrics for `EBLC' index tables format 2 and 5.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The target range.                                        */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Const_Metrics( TT_SBit_Range  range,
                           FT_Stream      stream )
  {
    FT_Error  error;


    if ( FT_READ_ULONG( range->image_size ) )
      return error;

    return FT_STREAM_READ_FIELDS( sbit_metrics_fields, &range->metrics );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Range_Codes                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the range codes for `EBLC' index tables format 4 and 5.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range        :: The target range.                                  */
  /*                                                                       */
  /*    stream       :: The input stream.                                  */
  /*                                                                       */
  /*    load_offsets :: A flag whether to load the glyph offset table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Range_Codes( TT_SBit_Range  range,
                         FT_Stream      stream,
                         FT_Bool        load_offsets )
  {
    FT_Error   error;
    FT_ULong   count, n, size;
    FT_Memory  memory = stream->memory;


    if ( FT_READ_ULONG( count ) )
      goto Exit;

    range->num_glyphs = count;

    /* Allocate glyph offsets table if needed */
    if ( load_offsets )
    {
      if ( FT_NEW_ARRAY( range->glyph_offsets, count ) )
        goto Exit;

      size = count * 4L;
    }
    else
      size = count * 2L;

    /* Allocate glyph codes table and access frame */
    if ( FT_NEW_ARRAY ( range->glyph_codes, count ) ||
         FT_FRAME_ENTER( size )                     )
      goto Exit;

    for ( n = 0; n < count; n++ )
    {
      range->glyph_codes[n] = FT_GET_USHORT();

      if ( load_offsets )
        range->glyph_offsets[n] = (FT_ULong)range->image_offset +
                                  FT_GET_USHORT();
    }

    FT_FRAME_EXIT();

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Range                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given `EBLC' index/range table.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The target range.                                        */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Range( TT_SBit_Range  range,
                   FT_Stream      stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;


    switch( range->index_format )
    {
    case 1:   /* variable metrics with 4-byte offsets */
    case 3:   /* variable metrics with 2-byte offsets */
      {
        FT_ULong  num_glyphs, n;
        FT_Int    size_elem;
        FT_Bool   large = FT_BOOL( range->index_format == 1 );



        if ( range->last_glyph < range->first_glyph )
        {
          error = SFNT_Err_Invalid_File_Format;
          goto Exit;
        }

        num_glyphs        = range->last_glyph - range->first_glyph + 1L;
        range->num_glyphs = num_glyphs;
        num_glyphs++;                       /* XXX: BEWARE - see spec */

        size_elem = large ? 4 : 2;

        if ( FT_NEW_ARRAY( range->glyph_offsets, num_glyphs ) ||
             FT_FRAME_ENTER( num_glyphs * size_elem )         )
          goto Exit;

        for ( n = 0; n < num_glyphs; n++ )
          range->glyph_offsets[n] = (FT_ULong)( range->image_offset +
                                                ( large ? FT_GET_ULONG()
                                                        : FT_GET_USHORT() ) );
        FT_FRAME_EXIT();
      }
      break;

    case 2:   /* all glyphs have identical metrics */
      error = Load_SBit_Const_Metrics( range, stream );
      break;

    case 4:
      error = Load_SBit_Range_Codes( range, stream, 1 );
      break;

    case 5:
      error = Load_SBit_Const_Metrics( range, stream )   ||
              Load_SBit_Range_Codes( range, stream, 0 );
      break;

    default:
      error = SFNT_Err_Invalid_File_Format;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_sbit_strikes                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table of embedded bitmap sizes for this face.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The target face object.                                  */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_eblc( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error  = 0;
    FT_Memory  memory = stream->memory;
    FT_Fixed   version;
    FT_ULong   num_strikes;
    FT_ULong   table_base;

    static const FT_Frame_Field  sbit_line_metrics_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_LineMetricsRec

      /* no FT_FRAME_START */
        FT_FRAME_CHAR( ascender ),
        FT_FRAME_CHAR( descender ),
        FT_FRAME_BYTE( max_width ),

        FT_FRAME_CHAR( caret_slope_numerator ),
        FT_FRAME_CHAR( caret_slope_denominator ),
        FT_FRAME_CHAR( caret_offset ),

        FT_FRAME_CHAR( min_origin_SB ),
        FT_FRAME_CHAR( min_advance_SB ),
        FT_FRAME_CHAR( max_before_BL ),
        FT_FRAME_CHAR( min_after_BL ),
        FT_FRAME_CHAR( pads[0] ),
        FT_FRAME_CHAR( pads[1] ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  strike_start_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_StrikeRec

      /* no FT_FRAME_START */
        FT_FRAME_ULONG( ranges_offset ),
        FT_FRAME_SKIP_LONG,
        FT_FRAME_ULONG( num_ranges ),
        FT_FRAME_ULONG( color_ref ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  strike_end_fields[] =
    {
      /* no FT_FRAME_START */
        FT_FRAME_USHORT( start_glyph ),
        FT_FRAME_USHORT( end_glyph ),
        FT_FRAME_BYTE  ( x_ppem ),
        FT_FRAME_BYTE  ( y_ppem ),
        FT_FRAME_BYTE  ( bit_depth ),
        FT_FRAME_CHAR  ( flags ),
      FT_FRAME_END
    };


    face->num_sbit_strikes = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_EBLC, stream, 0 );
    if ( error )
      error = face->goto_table( face, TTAG_bloc, stream, 0 );
    if ( error )
      goto Exit;

    table_base = FT_STREAM_POS();
    if ( FT_FRAME_ENTER( 8L ) )
      goto Exit;

    version     = FT_GET_LONG();
    num_strikes = FT_GET_ULONG();

    FT_FRAME_EXIT();

    /* check version number and strike count */
    if ( version     != 0x00020000L ||
         num_strikes >= 0x10000L    )
    {
      FT_ERROR(( "tt_face_load_sbit_strikes: invalid table version!\n" ));
      error = SFNT_Err_Invalid_File_Format;

      goto Exit;
    }

    /* allocate the strikes table */
    if ( FT_NEW_ARRAY( face->sbit_strikes, num_strikes ) )
      goto Exit;

    face->num_sbit_strikes = num_strikes;

    /* now read each strike table separately */
    {
      TT_SBit_Strike  strike = face->sbit_strikes;
      FT_ULong        count  = num_strikes;


      if ( FT_FRAME_ENTER( 48L * num_strikes ) )
        goto Exit;

      while ( count > 0 )
      {
        if ( FT_STREAM_READ_FIELDS( strike_start_fields, strike )             ||
             FT_STREAM_READ_FIELDS( sbit_line_metrics_fields, &strike->hori ) ||
             FT_STREAM_READ_FIELDS( sbit_line_metrics_fields, &strike->vert ) ||
             FT_STREAM_READ_FIELDS( strike_end_fields, strike )               )
          break;

        count--;
        strike++;
      }

      FT_FRAME_EXIT();
    }

    /* allocate the index ranges for each strike table */
    {
      TT_SBit_Strike  strike = face->sbit_strikes;
      FT_ULong        count  = num_strikes;


      while ( count > 0 )
      {
        TT_SBit_Range  range;
        FT_ULong       count2 = strike->num_ranges;


        /* read each range */
        if ( FT_STREAM_SEEK( table_base + strike->ranges_offset ) ||
             FT_FRAME_ENTER( strike->num_ranges * 8L )            )
          goto Exit;

        if ( FT_NEW_ARRAY( strike->sbit_ranges, strike->num_ranges ) )
          goto Exit;

        range = strike->sbit_ranges;
        while ( count2 > 0 )
        {
          range->first_glyph  = FT_GET_USHORT();
          range->last_glyph   = FT_GET_USHORT();
          range->table_offset = table_base + strike->ranges_offset +
                                  FT_GET_ULONG();
          count2--;
          range++;
        }

        FT_FRAME_EXIT();

        /* Now, read each index table */
        count2 = strike->num_ranges;
        range  = strike->sbit_ranges;
        while ( count2 > 0 )
        {
          /* Read the header */
          if ( FT_STREAM_SEEK( range->table_offset ) ||
               FT_FRAME_ENTER( 8L )                  )
            goto Exit;

          range->index_format = FT_GET_USHORT();
          range->image_format = FT_GET_USHORT();
          range->image_offset = FT_GET_ULONG();

          FT_FRAME_EXIT();

          error = Load_SBit_Range( range, stream );
          if ( error )
            goto Exit;

          count2--;
          range++;
        }

        count--;
        strike++;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_free_sbit_strikes                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases the embedded bitmap tables.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: The target face object.                                    */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_face_free_eblc( TT_Face  face )
  {
    FT_Memory       memory       = face->root.memory;
    TT_SBit_Strike  strike       = face->sbit_strikes;
    TT_SBit_Strike  strike_limit = strike + face->num_sbit_strikes;


    if ( strike )
    {
      for ( ; strike < strike_limit; strike++ )
      {
        TT_SBit_Range  range       = strike->sbit_ranges;
        TT_SBit_Range  range_limit = range + strike->num_ranges;


        if ( range )
        {
          for ( ; range < range_limit; range++ )
          {
            /* release the glyph offsets and codes tables */
            /* where appropriate                          */
            FT_FREE( range->glyph_offsets );
            FT_FREE( range->glyph_codes );
          }
        }
        FT_FREE( strike->sbit_ranges );
        strike->num_ranges = 0;
      }
      FT_FREE( face->sbit_strikes );
    }
    face->num_sbit_strikes = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  tt_face_set_sbit_strike( TT_Face          face,
                           FT_Size_Request  req,
                           FT_ULong*        astrike_index )
  {
    return FT_Match_Size( (FT_Face)face, req, 0, astrike_index );
  }


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_strike_metrics( TT_Face           face,
                               FT_ULong          strike_index,
                               FT_Size_Metrics*  metrics )
  {
    TT_SBit_Strike  strike;


    if ( strike_index >= face->num_sbit_strikes )
      return SFNT_Err_Invalid_Argument;

    strike = face->sbit_strikes + strike_index;

    metrics->x_ppem = strike->x_ppem;
    metrics->y_ppem = strike->y_ppem;

    metrics->ascender  = strike->hori.ascender << 6;
    metrics->descender = strike->hori.descender << 6;

    /* XXX: Is this correct? */
    metrics->max_advance = ( strike->hori.min_origin_SB  +
                             strike->hori.max_width      +
                             strike->hori.min_advance_SB ) << 6;

    metrics->height = metrics->ascender - metrics->descender;

    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    find_sbit_range                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Scans a given strike's ranges and return, for a given glyph        */
  /*    index, the corresponding sbit range, and `EBDT' offset.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph_index   :: The glyph index.                                  */
  /*                                                                       */
  /*    strike        :: The source/current sbit strike.                   */
  /*                                                                       */
  /* <Output>                                                              */
  /*    arange        :: The sbit range containing the glyph index.        */
  /*                                                                       */
  /*    aglyph_offset :: The offset of the glyph data in `EBDT' table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means the glyph index was found.           */
  /*                                                                       */
  static FT_Error
  find_sbit_range( FT_UInt          glyph_index,
                   TT_SBit_Strike   strike,
                   TT_SBit_Range   *arange,
                   FT_ULong        *aglyph_offset )
  {
    TT_SBit_RangeRec  *range, *range_limit;


    /* check whether the glyph index is within this strike's */
    /* glyph range                                           */
    if ( glyph_index < (FT_UInt)strike->start_glyph ||
         glyph_index > (FT_UInt)strike->end_glyph   )
      goto Fail;

    /* scan all ranges in strike */
    range       = strike->sbit_ranges;
    range_limit = range + strike->num_ranges;
    if ( !range )
      goto Fail;

    for ( ; range < range_limit; range++ )
    {
      if ( glyph_index >= (FT_UInt)range->first_glyph &&
           glyph_index <= (FT_UInt)range->last_glyph  )
      {
        FT_UShort  delta = (FT_UShort)( glyph_index - range->first_glyph );


        switch ( range->index_format )
        {
        case 1:
        case 3:
          *aglyph_offset = range->glyph_offsets[delta];
          break;

        case 2:
          *aglyph_offset = range->image_offset +
                           range->image_size * delta;
          break;

        case 4:
        case 5:
          {
            FT_ULong  n;


            for ( n = 0; n < range->num_glyphs; n++ )
            {
              if ( (FT_UInt)range->glyph_codes[n] == glyph_index )
              {
                if ( range->index_format == 4 )
                  *aglyph_offset = range->glyph_offsets[n];
                else
                  *aglyph_offset = range->image_offset +
                                   n * range->image_size;
                goto Found;
              }
            }
          }

        /* fall-through */
        default:
          goto Fail;
        }

      Found:
        /* return successfully! */
        *arange  = range;
        return 0;
      }
    }

  Fail:
    *arange        = 0;
    *aglyph_offset = 0;

    return SFNT_Err_Invalid_Argument;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_find_sbit_image                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks whether an embedded bitmap (an `sbit') exists for a given   */
  /*    glyph, at a given strike.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    glyph_index   :: The glyph index.                                  */
  /*                                                                       */
  /*    strike_index  :: The current strike index.                         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    arange        :: The SBit range containing the glyph index.        */
  /*                                                                       */
  /*    astrike       :: The SBit strike containing the glyph index.       */
  /*                                                                       */
  /*    aglyph_offset :: The offset of the glyph data in `EBDT' table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns                    */
  /*    SFNT_Err_Invalid_Argument if no sbit exists for the requested      */
  /*    glyph.                                                             */
  /*                                                                       */
  FT_LOCAL( FT_Error )
  tt_find_sbit_image( TT_Face          face,
                      FT_UInt          glyph_index,
                      FT_ULong         strike_index,
                      TT_SBit_Range   *arange,
                      TT_SBit_Strike  *astrike,
                      FT_ULong        *aglyph_offset )
  {
    FT_Error        error;
    TT_SBit_Strike  strike;


    if ( !face->sbit_strikes                        ||
         ( face->num_sbit_strikes <= strike_index ) )
      goto Fail;

    strike = &face->sbit_strikes[strike_index];

    error = find_sbit_range( glyph_index, strike,
                             arange, aglyph_offset );
    if ( error )
      goto Fail;

    *astrike = strike;

    return SFNT_Err_Ok;

  Fail:
    /* no embedded bitmap for this glyph in face */
    *arange        = 0;
    *astrike       = 0;
    *aglyph_offset = 0;

    return SFNT_Err_Invalid_Argument;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_load_sbit_metrics                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Gets the big metrics for a given SBit.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream      :: The input stream.                                   */
  /*                                                                       */
  /*    range       :: The SBit range containing the glyph.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    big_metrics :: A big SBit metrics structure for the glyph.         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be positioned at the glyph's offset within  */
  /*    the `EBDT' table before the call.                                  */
  /*                                                                       */
  /*    If the image format uses variable metrics, the stream cursor is    */
  /*    positioned just after the metrics header in the `EBDT' table on    */
  /*    function exit.                                                     */
  /*                                                                       */
  FT_LOCAL( FT_Error )
  tt_load_sbit_metrics( FT_Stream        stream,
                        TT_SBit_Range    range,
                        TT_SBit_Metrics  metrics )
  {
    FT_Error  error = SFNT_Err_Ok;


    switch ( range->image_format )
    {
    case 1:
    case 2:
    case 8:
      /* variable small metrics */
      {
        TT_SBit_SmallMetricsRec  smetrics;

        static const FT_Frame_Field  sbit_small_metrics_fields[] =
        {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_SmallMetricsRec

          FT_FRAME_START( 5 ),
            FT_FRAME_BYTE( height ),
            FT_FRAME_BYTE( width ),
            FT_FRAME_CHAR( bearingX ),
            FT_FRAME_CHAR( bearingY ),
            FT_FRAME_BYTE( advance ),
          FT_FRAME_END
        };


        /* read small metrics */
        if ( FT_STREAM_READ_FIELDS( sbit_small_metrics_fields, &smetrics ) )
          goto Exit;

        /* convert it to a big metrics */
        metrics->height       = smetrics.height;
        metrics->width        = smetrics.width;
        metrics->horiBearingX = smetrics.bearingX;
        metrics->horiBearingY = smetrics.bearingY;
        metrics->horiAdvance  = smetrics.advance;

        /* these metrics are made up at a higher level when */
        /* needed.                                          */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;
      }
      break;

    case 6:
    case 7:
    case 9:
      /* variable big metrics */
      if ( FT_STREAM_READ_FIELDS( sbit_metrics_fields, metrics ) )
        goto Exit;
      break;

    case 5:
    default:  /* constant metrics */
      if ( range->index_format == 2 || range->index_format == 5 )
        *metrics = range->metrics;
      else
        return SFNT_Err_Invalid_File_Format;
   }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    crop_bitmap                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Crops a bitmap to its tightest bounding box, and adjusts its       */
  /*    metrics.                                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    map     :: The bitmap.                                             */
  /*                                                                       */
  /*    metrics :: The corresponding metrics structure.                    */
  /*                                                                       */
  static void
  crop_bitmap( FT_Bitmap*       map,
               TT_SBit_Metrics  metrics )
  {
    /***********************************************************************/
    /*                                                                     */
    /* In this situation, some bounding boxes of embedded bitmaps are too  */
    /* large.  We need to crop it to a reasonable size.                    */
    /*                                                                     */
    /*      ---------                                                      */
    /*      |       |                -----                                 */
    /*      |  ***  |                |***|                                 */
    /*      |   *   |                | * |                                 */
    /*      |   *   |    ------>     | * |                                 */
    /*      |   *   |                | * |                                 */
    /*      |   *   |                | * |                                 */
    /*      |  ***  |                |***|                                 */
    /*      ---------                -----                                 */
    /*                                                                     */
    /***********************************************************************/

    FT_Int    rows, count;
    FT_Long   line_len;
    FT_Byte*  line;


    /***********************************************************************/
    /*                                                                     */
    /* first of all, check the top-most lines of the bitmap, and remove    */
    /* them if they're empty.                                              */
    /*                                                                     */
    {
      line     = (FT_Byte*)map->buffer;
      rows     = map->rows;
      line_len = map->pitch;


      for ( count = 0; count < rows; count++ )
      {
        FT_Byte*  cur   = line;
        FT_Byte*  limit = line + line_len;


        for ( ; cur < limit; cur++ )
          if ( cur[0] )
            goto Found_Top;

        /* the current line was empty - skip to next one */
        line  = limit;
      }

    Found_Top:
      /* check that we have at least one filled line */
      if ( count >= rows )
        goto Empty_Bitmap;

      /* now, crop the empty upper lines */
      if ( count > 0 )
      {
        line = (FT_Byte*)map->buffer;

        FT_MEM_MOVE( line, line + count * line_len,
                     ( rows - count ) * line_len );

        metrics->height       = (FT_Byte)( metrics->height - count );
        metrics->horiBearingY = (FT_Char)( metrics->horiBearingY - count );
        metrics->vertBearingY = (FT_Char)( metrics->vertBearingY - count );

        map->rows -= count;
        rows      -= count;
      }
    }

    /***********************************************************************/
    /*                                                                     */
    /* second, crop the lower lines                                        */
    /*                                                                     */
    {
      line = (FT_Byte*)map->buffer + ( rows - 1 ) * line_len;

      for ( count = 0; count < rows; count++ )
      {
        FT_Byte*  cur   = line;
        FT_Byte*  limit = line + line_len;


        for ( ; cur < limit; cur++ )
          if ( cur[0] )
            goto Found_Bottom;

        /* the current line was empty - skip to previous one */
        line -= line_len;
      }

    Found_Bottom:
      if ( count > 0 )
      {
        metrics->height  = (FT_Byte)( metrics->height - count );
        rows            -= count;
        map->rows       -= count;
      }
    }

    /***********************************************************************/
    /*                                                                     */
    /* third, get rid of the space on the left side of the glyph           */
    /*                                                                     */
    do
    {
      FT_Byte*  limit;


      line  = (FT_Byte*)map->buffer;
      limit = line + rows * line_len;

      for ( ; line < limit; line += line_len )
        if ( line[0] & 0x80 )
          goto Found_Left;

      /* shift the whole glyph one pixel to the left */
      line  = (FT_Byte*)map->buffer;
      limit = line + rows * line_len;

      for ( ; line < limit; line += line_len )
      {
        FT_Int    n, width = map->width;
        FT_Byte   old;
        FT_Byte*  cur = line;


        old = (FT_Byte)(cur[0] << 1);
        for ( n = 8; n < width; n += 8 )
        {
          FT_Byte  val;


          val    = cur[1];
          cur[0] = (FT_Byte)( old | ( val >> 7 ) );
          old    = (FT_Byte)( val << 1 );
          cur++;
        }
        cur[0] = old;
      }

      map->width--;
      metrics->horiBearingX++;
      metrics->vertBearingX++;
      metrics->width--;

    } while ( map->width > 0 );

  Found_Left:

    /***********************************************************************/
    /*                                                                     */
    /* finally, crop the bitmap width to get rid of the space on the right */
    /* side of the glyph.                                                  */
    /*                                                                     */
    do
    {
      FT_Int    right = map->width - 1;
      FT_Byte*  limit;
      FT_Byte   mask;


      line  = (FT_Byte*)map->buffer + ( right >> 3 );
      limit = line + rows * line_len;
      mask  = (FT_Byte)( 0x80 >> ( right & 7 ) );

      for ( ; line < limit; line += line_len )
        if ( line[0] & mask )
          goto Found_Right;

      /* crop the whole glyph to the right */
      map->width--;
      metrics->width--;

    } while ( map->width > 0 );

  Found_Right:
    /* all right, the bitmap was cropped */
    return;

  Empty_Bitmap:
    map->width      = 0;
    map->rows       = 0;
    map->pitch      = 0;
    map->pixel_mode = FT_PIXEL_MODE_MONO;
  }


  static FT_Error
  Load_SBit_Single( FT_Bitmap*       map,
                    FT_Int           x_offset,
                    FT_Int           y_offset,
                    FT_Int           pix_bits,
                    FT_UShort        image_format,
                    TT_SBit_Metrics  metrics,
                    FT_Stream        stream )
  {
    FT_Error  error;


    /* check that the source bitmap fits into the target pixmap */
    if ( x_offset < 0 || x_offset + metrics->width  > map->width ||
         y_offset < 0 || y_offset + metrics->height > map->rows  )
    {
      error = SFNT_Err_Invalid_Argument;

      goto Exit;
    }

    {
      FT_Int   glyph_width  = metrics->width;
      FT_Int   glyph_height = metrics->height;
      FT_Int   glyph_size;
      FT_Int   line_bits    = pix_bits * glyph_width;
      FT_Bool  pad_bytes    = 0;


      /* compute size of glyph image */
      switch ( image_format )
      {
      case 1:  /* byte-padded formats */
      case 6:
        {
          FT_Int  line_length;


          switch ( pix_bits )
          {
          case 1:
            line_length = ( glyph_width + 7 ) >> 3;
            break;
          case 2:
            line_length = ( glyph_width + 3 ) >> 2;
            break;
          case 4:
            line_length = ( glyph_width + 1 ) >> 1;
            break;
          default:
            line_length =   glyph_width;
          }

          glyph_size = glyph_height * line_length;
          pad_bytes  = 1;
        }
        break;

      case 2:
      case 5:
      case 7:
        line_bits  =   glyph_width  * pix_bits;
        glyph_size = ( glyph_height * line_bits + 7 ) >> 3;
        break;

      default:  /* invalid format */
        return SFNT_Err_Invalid_File_Format;
      }

      /* Now read data and draw glyph into target pixmap       */
      if ( FT_FRAME_ENTER( glyph_size ) )
        goto Exit;

      /* don't forget to multiply `x_offset' by `map->pix_bits' as */
      /* the sbit blitter doesn't make a difference between pixmap */
      /* depths.                                                   */
      blit_sbit( map, (FT_Byte*)stream->cursor, line_bits, pad_bytes,
                 x_offset * pix_bits, y_offset );

      FT_FRAME_EXIT();
    }

  Exit:
    return error;
  }


  static FT_Error
  Load_SBit_Image( TT_SBit_Strike   strike,
                   TT_SBit_Range    range,
                   FT_ULong         ebdt_pos,
                   FT_ULong         glyph_offset,
                   FT_GlyphSlot     slot,
                   FT_Int           x_offset,
                   FT_Int           y_offset,
                   FT_Stream        stream,
                   TT_SBit_Metrics  metrics,
                   FT_Int           depth )
  {
    FT_Memory   memory = stream->memory;
    FT_Bitmap*  map    = &slot->bitmap;
    FT_Error    error;


    /* place stream at beginning of glyph data and read metrics */
    if ( FT_STREAM_SEEK( ebdt_pos + glyph_offset ) )
      goto Exit;

    error = tt_load_sbit_metrics( stream, range, metrics );
    if ( error )
      goto Exit;

    /* This function is recursive.  At the top-level call, we  */
    /* compute the dimensions of the higher-level glyph to     */
    /* allocate the final pixmap buffer.                       */
    if ( depth == 0 )
    {
      FT_Long  size;


      map->width = metrics->width;
      map->rows  = metrics->height;

      switch ( strike->bit_depth )
      {
      case 1:
        map->pixel_mode = FT_PIXEL_MODE_MONO;
        map->pitch      = ( map->width + 7 ) >> 3;
        break;

      case 2:
        map->pixel_mode = FT_PIXEL_MODE_GRAY2;
        map->pitch      = ( map->width + 3 ) >> 2;
        break;

      case 4:
        map->pixel_mode = FT_PIXEL_MODE_GRAY4;
        map->pitch      = ( map->width + 1 ) >> 1;
        break;

      case 8:
        map->pixel_mode = FT_PIXEL_MODE_GRAY;
        map->pitch      = map->width;
        break;

      default:
        return SFNT_Err_Invalid_File_Format;
      }

      size = map->rows * map->pitch;

      /* check that there is no empty image */
      if ( size == 0 )
        goto Exit;     /* exit successfully! */

      error = ft_glyphslot_alloc_bitmap( slot, size );
      if (error)
        goto Exit;
    }

    switch ( range->image_format )
    {
    case 1:  /* single sbit image - load it */
    case 2:
    case 5:
    case 6:
    case 7:
      return Load_SBit_Single( map, x_offset, y_offset, strike->bit_depth,
                               range->image_format, metrics, stream );

    case 8:  /* compound format */
      FT_Stream_Skip( stream, 1L );
      /* fallthrough */

    case 9:
      break;

    default: /* invalid image format */
      return SFNT_Err_Invalid_File_Format;
    }

    /* All right, we have a compound format.  First of all, read */
    /* the array of elements.                                    */
    {
      TT_SBit_Component  components;
      TT_SBit_Component  comp;
      FT_UShort          num_components, count;


      if ( FT_READ_USHORT( num_components )           ||
           FT_NEW_ARRAY( components, num_components ) )
        goto Exit;

      count = num_components;

      if ( FT_FRAME_ENTER( 4L * num_components ) )
        goto Fail_Memory;

      for ( comp = components; count > 0; count--, comp++ )
      {
        comp->glyph_code = FT_GET_USHORT();
        comp->x_offset   = FT_GET_CHAR();
        comp->y_offset   = FT_GET_CHAR();
      }

      FT_FRAME_EXIT();

      /* Now recursively load each element glyph */
      count = num_components;
      comp  = components;
      for ( ; count > 0; count--, comp++ )
      {
        TT_SBit_Range       elem_range;
        TT_SBit_MetricsRec  elem_metrics;
        FT_ULong            elem_offset;


        /* find the range for this element */
        error = find_sbit_range( comp->glyph_code,
                                 strike,
                                 &elem_range,
                                 &elem_offset );
        if ( error )
          goto Fail_Memory;

        /* now load the element, recursively */
        error = Load_SBit_Image( strike,
                                 elem_range,
                                 ebdt_pos,
                                 elem_offset,
                                 slot,
                                 x_offset + comp->x_offset,
                                 y_offset + comp->y_offset,
                                 stream,
                                 &elem_metrics,
                                 depth + 1 );
        if ( error )
          goto Fail_Memory;
      }

    Fail_Memory:
      FT_FREE( components );
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_sbit_image                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given glyph sbit image from the font resource.  This also  */
  /*    returns its metrics.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face         :: The target face object.                            */
  /*                                                                       */
  /*    strike_index :: The current strike index.                          */
  /*                                                                       */
  /*    glyph_index  :: The current glyph index.                           */
  /*                                                                       */
  /*    load_flags   :: The glyph load flags (the code checks for the flag */
  /*                    FT_LOAD_CROP_BITMAP).                              */
  /*                                                                       */
  /*    stream       :: The input stream.                                  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    map          :: The target pixmap.                                 */
  /*                                                                       */
  /*    metrics      :: A big sbit metrics structure for the glyph image.  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    glyph sbit exists for the index.                                   */
  /*                                                                       */
  /*  <Note>                                                               */
  /*    The `map.buffer' field is always freed before the glyph is loaded. */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_sbit_image( TT_Face              face,
                           FT_ULong             strike_index,
                           FT_UInt              glyph_index,
                           FT_UInt              load_flags,
                           FT_Stream            stream,
                           FT_Bitmap           *map,
                           TT_SBit_MetricsRec  *metrics )
  {
    FT_Error        error;
    FT_ULong        ebdt_pos, glyph_offset;

    TT_SBit_Strike  strike;
    TT_SBit_Range   range;


    /* Check whether there is a glyph sbit for the current index */
    error = tt_find_sbit_image( face, glyph_index, strike_index,
                                &range, &strike, &glyph_offset );
    if ( error )
      goto Exit;

    /* now, find the location of the `EBDT' table in */
    /* the font file                                 */
    error = face->goto_table( face, TTAG_EBDT, stream, 0 );
    if ( error )
      error = face->goto_table( face, TTAG_bdat, stream, 0 );
    if ( error )
      goto Exit;

    ebdt_pos = FT_STREAM_POS();

    error = Load_SBit_Image( strike, range, ebdt_pos, glyph_offset,
                             face->root.glyph, 0, 0, stream, metrics, 0 );
    if ( error )
      goto Exit;

    /* setup vertical metrics if needed */
    if ( strike->flags & 1 )
    {
      /* in case of a horizontal strike only */
      FT_Int  advance;


      advance = strike->hori.ascender - strike->hori.descender;

      /* some heuristic values */

      metrics->vertBearingX = (FT_Char)(-metrics->width / 2 );
      metrics->vertBearingY = (FT_Char)( ( advance - metrics->height ) / 2 );
      metrics->vertAdvance  = (FT_Char)( advance * 12 / 10 );
    }

    /* Crop the bitmap now, unless specified otherwise */
    if ( load_flags & FT_LOAD_CROP_BITMAP )
      crop_bitmap( map, metrics );

  Exit:
    return error;
  }

#endif /* !OPTIMIZE_MEMORY || OLD_INTERNALS */


/* END */

#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
/***************************************************************************/
/*                                                                         */
/*  ttpost.c                                                               */
/*                                                                         */
/*    Postcript name table processing for TrueType and OpenType fonts      */
/*    (body).                                                              */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2006 by                               */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
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
  /* The post table is not completely loaded by the core engine.  This     */
  /* file loads the missing PS glyph names and implements an API to access */
  /* them.                                                                 */
  /*                                                                       */
  /*************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttpost.h"
#include "ttload.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttpost


  /* If this configuration macro is defined, we rely on the `PSNames' */
  /* module to grab the glyph names.                                  */

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES


#include FT_SERVICE_POSTSCRIPT_CMAPS_H

#define MAC_NAME( x )  ( (FT_String*)psnames->macintosh_name( x ) )


#else /* FT_CONFIG_OPTION_POSTSCRIPT_NAMES */


   /* Otherwise, we ignore the `PSNames' module, and provide our own  */
   /* table of Mac names.  Thus, it is possible to build a version of */
   /* FreeType without the Type 1 driver & PSNames module.            */

#define MAC_NAME( x )  (FT_String*)tt_post_default_names[x] /* JE: Cast inserted */

  /* the 258 default Mac PS glyph names */

  static const FT_String*  tt_post_default_names[258] =
  {
    /*   0 */
    ".notdef", ".null", "CR", "space", "exclam",
    "quotedbl", "numbersign", "dollar", "percent", "ampersand",
    /*  10 */
    "quotesingle", "parenleft", "parenright", "asterisk", "plus",
    "comma", "hyphen", "period", "slash", "zero",
    /*  20 */
    "one", "two", "three", "four", "five",
    "six", "seven", "eight", "nine", "colon",
    /*  30 */
    "semicolon", "less", "equal", "greater", "question",
    "at", "A", "B", "C", "D",
    /*  40 */
    "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N",
    /*  50 */
    "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X",
    /*  60 */
    "Y", "Z", "bracketleft", "backslash", "bracketright",
    "asciicircum", "underscore", "grave", "a", "b",
    /*  70 */
    "c", "d", "e", "f", "g",
    "h", "i", "j", "k", "l",
    /*  80 */
    "m", "n", "o", "p", "q",
    "r", "s", "t", "u", "v",
    /*  90 */
    "w", "x", "y", "z", "braceleft",
    "bar", "braceright", "asciitilde", "Adieresis", "Aring",
    /* 100 */
    "Ccedilla", "Eacute", "Ntilde", "Odieresis", "Udieresis",
    "aacute", "agrave", "acircumflex", "adieresis", "atilde",
    /* 110 */
    "aring", "ccedilla", "eacute", "egrave", "ecircumflex",
    "edieresis", "iacute", "igrave", "icircumflex", "idieresis",
    /* 120 */
    "ntilde", "oacute", "ograve", "ocircumflex", "odieresis",
    "otilde", "uacute", "ugrave", "ucircumflex", "udieresis",
    /* 130 */
    "dagger", "degree", "cent", "sterling", "section",
    "bullet", "paragraph", "germandbls", "registered", "copyright",
    /* 140 */
    "trademark", "acute", "dieresis", "notequal", "AE",
    "Oslash", "infinity", "plusminus", "lessequal", "greaterequal",
    /* 150 */
    "yen", "mu", "partialdiff", "summation", "product",
    "pi", "integral", "ordfeminine", "ordmasculine", "Omega",
    /* 160 */
    "ae", "oslash", "questiondown", "exclamdown", "logicalnot",
    "radical", "florin", "approxequal", "Delta", "guillemotleft",
    /* 170 */
    "guillemotright", "ellipsis", "nbspace", "Agrave", "Atilde",
    "Otilde", "OE", "oe", "endash", "emdash",
    /* 180 */
    "quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide",
    "lozenge", "ydieresis", "Ydieresis", "fraction", "currency",
    /* 190 */
    "guilsinglleft", "guilsinglright", "fi", "fl", "daggerdbl",
    "periodcentered", "quotesinglbase", "quotedblbase", "perthousand", "Acircumflex",
    /* 200 */
    "Ecircumflex", "Aacute", "Edieresis", "Egrave", "Iacute",
    "Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex",
    /* 210 */
    "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave",
    "dotlessi", "circumflex", "tilde", "macron", "breve",
    /* 220 */
    "dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek",
    "caron", "Lslash", "lslash", "Scaron", "scaron",
    /* 230 */
    "Zcaron", "zcaron", "brokenbar", "Eth", "eth",
    "Yacute", "yacute", "Thorn", "thorn", "minus",
    /* 240 */
    "multiply", "onesuperior", "twosuperior", "threesuperior", "onehalf",
    "onequarter", "threequarters", "franc", "Gbreve", "gbreve",
    /* 250 */
    "Idot", "Scedilla", "scedilla", "Cacute", "cacute",
    "Ccaron", "ccaron", "dmacron",
  };


#endif /* FT_CONFIG_OPTION_POSTSCRIPT_NAMES */


  static FT_Error
  load_format_20( TT_Face    face,
                  FT_Stream  stream )
  {
    FT_Memory   memory = stream->memory;
    FT_Error    error;

    FT_Int      num_glyphs;
    FT_UShort   num_names;

    FT_UShort*  glyph_indices = 0;
    FT_Char**   name_strings  = 0;


    if ( FT_READ_USHORT( num_glyphs ) )
      goto Exit;

    /* UNDOCUMENTED!  The number of glyphs in this table can be smaller */
    /* than the value in the maxp table (cf. cyberbit.ttf).             */

    /* There already exist fonts which have more than 32768 glyph names */
    /* in this table, so the test for this threshold has been dropped.  */

    if ( num_glyphs > face->max_profile.numGlyphs )
    {
      error = SFNT_Err_Invalid_File_Format;
      goto Exit;
    }

    /* load the indices */
    {
      FT_Int  n;


      if ( FT_NEW_ARRAY ( glyph_indices, num_glyphs ) ||
           FT_FRAME_ENTER( num_glyphs * 2L )          )
        goto Fail;

      for ( n = 0; n < num_glyphs; n++ )
        glyph_indices[n] = FT_GET_USHORT();

      FT_FRAME_EXIT();
    }

    /* compute number of names stored in table */
    {
      FT_Int  n;


      num_names = 0;

      for ( n = 0; n < num_glyphs; n++ )
      {
        FT_Int  idx;


        idx = glyph_indices[n];
        if ( idx >= 258 )
        {
          idx -= 257;
          if ( idx > num_names )
            num_names = (FT_UShort)idx;
        }
      }
    }

    /* now load the name strings */
    {
      FT_UShort  n;


      if ( FT_NEW_ARRAY( name_strings, num_names ) )
        goto Fail;

      for ( n = 0; n < num_names; n++ )
      {
        FT_UInt  len;


        if ( FT_READ_BYTE  ( len )                    ||
             FT_NEW_ARRAY( name_strings[n], len + 1 ) ||
             FT_STREAM_READ  ( name_strings[n], len ) )
          goto Fail1;

        name_strings[n][len] = '\0';
      }
    }

    /* all right, set table fields and exit successfuly */
    {
      TT_Post_20  table = &face->postscript_names.names.format_20;


      table->num_glyphs    = (FT_UShort)num_glyphs;
      table->num_names     = (FT_UShort)num_names;
      table->glyph_indices = glyph_indices;
      table->glyph_names   = name_strings;
    }
    return SFNT_Err_Ok;

  Fail1:
    {
      FT_UShort  n;


      for ( n = 0; n < num_names; n++ )
        FT_FREE( name_strings[n] );
    }

  Fail:
    FT_FREE( name_strings );
    FT_FREE( glyph_indices );

  Exit:
    return error;
  }


  static FT_Error
  load_format_25( TT_Face    face,
                  FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error;

    FT_Int     num_glyphs;
    FT_Char*   offset_table = 0;


    /* UNDOCUMENTED!  This value appears only in the Apple TT specs. */
    if ( FT_READ_USHORT( num_glyphs ) )
      goto Exit;

    /* check the number of glyphs */
    if ( num_glyphs > face->max_profile.numGlyphs || num_glyphs > 258 )
    {
      error = SFNT_Err_Invalid_File_Format;
      goto Exit;
    }

    if ( FT_NEW_ARRAY( offset_table, num_glyphs )   ||
         FT_STREAM_READ( offset_table, num_glyphs ) )
      goto Fail;

    /* now check the offset table */
    {
      FT_Int  n;


      for ( n = 0; n < num_glyphs; n++ )
      {
        FT_Long  idx = (FT_Long)n + offset_table[n];


        if ( idx < 0 || idx > num_glyphs )
        {
          error = SFNT_Err_Invalid_File_Format;
          goto Fail;
        }
      }
    }

    /* OK, set table fields and exit successfuly */
    {
      TT_Post_25  table = &face->postscript_names.names.format_25;


      table->num_glyphs = (FT_UShort)num_glyphs;
      table->offsets    = offset_table;
    }

    return SFNT_Err_Ok;

  Fail:
    FT_FREE( offset_table );

  Exit:
    return error;
  }


  static FT_Error
  load_post_names( TT_Face  face )
  {
    FT_Stream  stream;
    FT_Error   error;
    FT_Fixed   format;


    /* get a stream for the face's resource */
    stream = face->root.stream;

    /* seek to the beginning of the PS names table */
    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      goto Exit;

    format = face->postscript.FormatType;

    /* go to beginning of subtable */
    if ( FT_STREAM_SKIP( 32 ) )
      goto Exit;

    /* now read postscript table */
    if ( format == 0x00020000L )
      error = load_format_20( face, stream );
    else if ( format == 0x00028000L )
      error = load_format_25( face, stream );
    else
      error = SFNT_Err_Invalid_File_Format;

    face->postscript_names.loaded = 1;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_ps_names( TT_Face  face )
  {
    FT_Memory      memory = face->root.memory;
    TT_Post_Names  names  = &face->postscript_names;
    FT_Fixed       format;


    if ( names->loaded )
    {
      format = face->postscript.FormatType;

      if ( format == 0x00020000L )
      {
        TT_Post_20  table = &names->names.format_20;
        FT_UShort   n;


        FT_FREE( table->glyph_indices );
        table->num_glyphs = 0;

        for ( n = 0; n < table->num_names; n++ )
          FT_FREE( table->glyph_names[n] );

        FT_FREE( table->glyph_names );
        table->num_names = 0;
      }
      else if ( format == 0x00028000L )
      {
        TT_Post_25  table = &names->names.format_25;


        FT_FREE( table->offsets );
        table->num_glyphs = 0;
      }
    }
    names->loaded = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_ps_name                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Gets the PostScript glyph name of a glyph.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the parent face.                             */
  /*                                                                       */
  /*    idx    :: The glyph index.                                         */
  /*                                                                       */
  /*    PSname :: The address of a string pointer.  Will be NULL in case   */
  /*              of error, otherwise it is a pointer to the glyph name.   */
  /*                                                                       */
  /*              You must not modify the returned string!                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_get_ps_name( TT_Face      face,
                       FT_UInt      idx,
                       FT_String**  PSname )
  {
    FT_Error         error;
    TT_Post_Names    names;
    FT_Fixed         format;

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
    FT_Service_PsCMaps  psnames;
#endif


    if ( !face )
      return SFNT_Err_Invalid_Face_Handle;

    if ( idx >= (FT_UInt)face->max_profile.numGlyphs )
      return SFNT_Err_Invalid_Glyph_Index;

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
    psnames = (FT_Service_PsCMaps)face->psnames;
    if ( !psnames )
      return SFNT_Err_Unimplemented_Feature;
#endif

    names = &face->postscript_names;

    /* `.notdef' by default */
    *PSname = MAC_NAME( 0 );

    format = face->postscript.FormatType;

    if ( format == 0x00010000L )
    {
      if ( idx < 258 )                    /* paranoid checking */
        *PSname = MAC_NAME( idx );
    }
    else if ( format == 0x00020000L )
    {
      TT_Post_20  table = &names->names.format_20;


      if ( !names->loaded )
      {
        error = load_post_names( face );
        if ( error )
          goto End;
      }

      if ( idx < (FT_UInt)table->num_glyphs )
      {
        FT_UShort  name_index = table->glyph_indices[idx];


        if ( name_index < 258 )
          *PSname = MAC_NAME( name_index );
        else
          *PSname = (FT_String*)table->glyph_names[name_index - 258];
      }
    }
    else if ( format == 0x00028000L )
    {
      TT_Post_25  table = &names->names.format_25;


      if ( !names->loaded )
      {
        error = load_post_names( face );
        if ( error )
          goto End;
      }

      if ( idx < (FT_UInt)table->num_glyphs )    /* paranoid checking */
      {
        idx    += table->offsets[idx];
        *PSname = MAC_NAME( idx );
      }
    }

    /* nothing to do for format == 0x00030000L */

  End:
    return SFNT_Err_Ok;
  }


/* END */

#endif

#ifdef TT_CONFIG_OPTION_BDF
/***************************************************************************/
/*                                                                         */
/*  ttbdf.c                                                                */
/*                                                                         */
/*    TrueType and OpenType embedded BDF properties (body).                */
/*                                                                         */
/*  Copyright 2005, 2006 by                                                */
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
#include FT_TRUETYPE_TAGS_H
#include "ttbdf.h"

#include "sferrors.h"


#ifdef TT_CONFIG_OPTION_BDF

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttbdf


  FT_LOCAL_DEF( void )
  tt_face_free_bdf_props( TT_Face  face )
  {
    TT_BDF  bdf = &face->bdf;


    if ( bdf->loaded )
    {
      FT_Stream  stream = FT_FACE(face)->stream;


      if ( bdf->table != NULL )
        FT_FRAME_RELEASE( bdf->table );

      bdf->table_end    = NULL;
      bdf->strings      = NULL;
      bdf->strings_size = 0;
    }
  }


  static FT_Error
  tt_face_load_bdf_props( TT_Face    face,
                          FT_Stream  stream )
  {
    TT_BDF    bdf = &face->bdf;
    FT_ULong  length;
    FT_Error  error;


    FT_ZERO( bdf );

    error = tt_face_goto_table( face, TTAG_BDF, stream, &length );
    if ( error                                  ||
         length < 8                             ||
         FT_FRAME_EXTRACT( length, bdf->table ) )
    {
      error = FT_Err_Invalid_Table;
      goto Exit;
    }

    bdf->table_end = bdf->table + length;

    {
      FT_Byte*   p           = bdf->table;
      FT_UInt    version     = FT_NEXT_USHORT( p );
      FT_UInt    num_strikes = FT_NEXT_USHORT( p );
      FT_UInt32  strings     = FT_NEXT_ULONG ( p );
      FT_UInt    count;
      FT_Byte*   strike;


      if ( version != 0x0001                 ||
           strings < 8                       ||
           ( strings - 8 ) / 4 < num_strikes ||
           strings + 1 > length              )
      {
        goto BadTable;
      }

      bdf->num_strikes  = num_strikes;
      bdf->strings      = bdf->table + strings;
      bdf->strings_size = length - strings;

      count  = bdf->num_strikes;
      p      = bdf->table + 8;
      strike = p + count * 4;


      for ( ; count > 0; count-- )
      {
        FT_UInt  num_items = FT_PEEK_USHORT( p + 2 );

        /*
         *  We don't need to check the value sets themselves, since this
         *  is done later.
         */
        strike += 10 * num_items;

        p += 4;
      }

      if ( strike > bdf->strings )
        goto BadTable;
    }

    bdf->loaded = 1;

  Exit:
    return error;

  BadTable:
    FT_FRAME_RELEASE( bdf->table );
    FT_ZERO( bdf );
    error = FT_Err_Invalid_Table;
    goto Exit;
  }


  FT_LOCAL_DEF( FT_Error )
  tt_face_find_bdf_prop( TT_Face           face,
                         const char*       property_name,
                         BDF_PropertyRec  *aprop )
  {
    TT_BDF    bdf   = &face->bdf;
    FT_Size   size  = FT_FACE(face)->size;
    FT_Error  error = 0;
    FT_Byte*  p;
    FT_UInt   count;
    FT_Byte*  strike;
    FT_UInt   property_len;


    aprop->type = BDF_PROPERTY_TYPE_NONE;

    if ( bdf->loaded == 0 )
    {
      error = tt_face_load_bdf_props( face, FT_FACE( face )->stream );
      if ( error )
        goto Exit;
    }

    count  = bdf->num_strikes;
    p      = bdf->table + 8;
    strike = p + 4 * count;

    error = FT_Err_Invalid_Argument;

    if ( size == NULL || property_name == NULL )
      goto Exit;

    property_len = ft_strlen( property_name );
    if ( property_len == 0 )
      goto Exit;

    for ( ; count > 0; count-- )
    {
      FT_UInt  _ppem  = FT_NEXT_USHORT( p );
      FT_UInt  _count = FT_NEXT_USHORT( p );

      if ( _ppem == size->metrics.y_ppem )
      {
        count = _count;
        goto FoundStrike;
      }

      strike += 10 * _count;
    }
    goto Exit;

  FoundStrike:
    p = strike;
    for ( ; count > 0; count-- )
    {
      FT_UInt  type = FT_PEEK_USHORT( p + 4 );

      if ( ( type & 0x10 ) != 0 )
      {
        FT_UInt32  name_offset = FT_PEEK_ULONG( p     );
        FT_UInt32  value       = FT_PEEK_ULONG( p + 6 );

        /* be a bit paranoid for invalid entries here */
        if ( name_offset < bdf->strings_size                    &&
             property_len < bdf->strings_size - name_offset     &&
             ft_strncmp( property_name,
                         (const char*)bdf->strings + name_offset,
                         bdf->strings_size - name_offset ) == 0 )
        {
          switch ( type & 0x0F )
          {
          case 0x00:  /* string */
          case 0x01:  /* atoms */
            /* check that the content is really 0-terminated */
            if ( value < bdf->strings_size &&
                 ft_memchr( bdf->strings + value, 0, bdf->strings_size ) )
            {
              aprop->type   = BDF_PROPERTY_TYPE_ATOM;
              aprop->u.atom = (const char*)bdf->strings + value;
              error         = 0;
              goto Exit;
            }
            break;

          case 0x02:
            aprop->type      = BDF_PROPERTY_TYPE_INTEGER;
            aprop->u.integer = (FT_Int32)value;
            error            = 0;
            goto Exit;

          case 0x03:
            aprop->type       = BDF_PROPERTY_TYPE_CARDINAL;
            aprop->u.cardinal = value;
            error             = 0;
            goto Exit;

          default:
            ;
          }
        }
      }
      p += 10;
    }

  Exit:
    return error;
  }

#endif /* TT_CONFIG_OPTION_BDF */


/* END */

#endif

/* END */

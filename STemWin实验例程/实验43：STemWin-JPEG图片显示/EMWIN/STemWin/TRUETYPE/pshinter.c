/***************************************************************************/
/*                                                                         */
/*  pshinter.c                                                             */
/*                                                                         */
/*    FreeType PostScript Hinting module                                   */
/*                                                                         */
/*  Copyright 2001, 2003 by                                                */
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
/*  pshrec.c                                                               */
/*                                                                         */
/*    FreeType PostScript hints recorder (body).                           */
/*                                                                         */
/*  Copyright 2001, 2002, 2003, 2004 by                                    */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include "pshrec.h"
#include "pshalgo.h"

#include "pshnterr.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshrec

#ifdef DEBUG_HINTER
  PS_Hints  ps_debug_hints         = 0;
  int       ps_debug_no_horz_hints = 0;
  int       ps_debug_no_vert_hints = 0;
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      PS_HINT MANAGEMENT                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* destroy hints table */
  static void
  ps_hint_table_done( PS_Hint_Table  table,
                      FT_Memory      memory )
  {
    FT_FREE( table->hints );
    table->num_hints = 0;
    table->max_hints = 0;
  }


  /* ensure that a table can contain "count" elements */
  static FT_Error
  ps_hint_table_ensure( PS_Hint_Table  table,
                        FT_UInt        count,
                        FT_Memory      memory )
  {
    FT_UInt   old_max = table->max_hints;
    FT_UInt   new_max = count;
    FT_Error  error   = 0;


    if ( new_max > old_max )
    {
      /* try to grow the table */
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( table->hints, old_max, new_max ) )
        table->max_hints = new_max;
    }
    return error;
  }


  static FT_Error
  ps_hint_table_alloc( PS_Hint_Table  table,
                       FT_Memory      memory,
                       PS_Hint       *ahint )
  {
    FT_Error  error = 0;
    FT_UInt   count;
    PS_Hint   hint = 0;


    count = table->num_hints;
    count++;

    if ( count >= table->max_hints )
    {
      error = ps_hint_table_ensure( table, count, memory );
      if ( error )
        goto Exit;
    }

    hint        = table->hints + count - 1;
    hint->pos   = 0;
    hint->len   = 0;
    hint->flags = 0;

    table->num_hints = count;

  Exit:
    *ahint = hint;
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      PS_MASK MANAGEMENT                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* destroy mask */
  static void
  ps_mask_done( PS_Mask    mask,
                FT_Memory  memory )
  {
    FT_FREE( mask->bytes );
    mask->num_bits  = 0;
    mask->max_bits  = 0;
    mask->end_point = 0;
  }


  /* ensure that a mask can contain "count" bits */
  static FT_Error
  ps_mask_ensure( PS_Mask    mask,
                  FT_UInt    count,
                  FT_Memory  memory )
  {
    FT_UInt   old_max = ( mask->max_bits + 7 ) >> 3;
    FT_UInt   new_max = ( count          + 7 ) >> 3;
    FT_Error  error   = 0;


    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( mask->bytes, old_max, new_max ) )
        mask->max_bits = new_max * 8;
    }
    return error;
  }


  /* test a bit value in a given mask */
  static FT_Int
  ps_mask_test_bit( PS_Mask  mask,
                    FT_Int   idx )
  {
    if ( (FT_UInt)idx >= mask->num_bits )
      return 0;

    return mask->bytes[idx >> 3] & ( 0x80 >> ( idx & 7 ) );
  }


  /* clear a given bit */
  static void
  ps_mask_clear_bit( PS_Mask  mask,
                     FT_Int   idx )
  {
    FT_Byte*  p;


    if ( (FT_UInt)idx >= mask->num_bits )
      return;

    p    = mask->bytes + ( idx >> 3 );
    p[0] = (FT_Byte)( p[0] & ~( 0x80 >> ( idx & 7 ) ) );
  }


  /* set a given bit, possibly grow the mask */
  static FT_Error
  ps_mask_set_bit( PS_Mask    mask,
                   FT_Int     idx,
                   FT_Memory  memory )
  {
    FT_Error  error = 0;
    FT_Byte*  p;


    if ( idx < 0 )
      goto Exit;

    if ( (FT_UInt)idx >= mask->num_bits )
    {
      error = ps_mask_ensure( mask, idx + 1, memory );
      if ( error )
        goto Exit;

      mask->num_bits = idx + 1;
    }

    p    = mask->bytes + ( idx >> 3 );
    p[0] = (FT_Byte)( p[0] | ( 0x80 >> ( idx & 7 ) ) );

  Exit:
    return error;
  }


  /* destroy mask table */
  static void
  ps_mask_table_done( PS_Mask_Table  table,
                      FT_Memory      memory )
  {
    FT_UInt  count = table->max_masks;
    PS_Mask  mask  = table->masks;


    for ( ; count > 0; count--, mask++ )
      ps_mask_done( mask, memory );

    FT_FREE( table->masks );
    table->num_masks = 0;
    table->max_masks = 0;
  }


  /* ensure that a mask table can contain "count" masks */
  static FT_Error
  ps_mask_table_ensure( PS_Mask_Table  table,
                        FT_UInt        count,
                        FT_Memory      memory )
  {
    FT_UInt   old_max = table->max_masks;
    FT_UInt   new_max = count;
    FT_Error  error   = 0;


    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( table->masks, old_max, new_max ) )
        table->max_masks = new_max;
    }
    return error;
  }


  /* allocate a new mask in a table */
  static FT_Error
  ps_mask_table_alloc( PS_Mask_Table  table,
                       FT_Memory      memory,
                       PS_Mask       *amask )
  {
    FT_UInt   count;
    FT_Error  error = 0;
    PS_Mask   mask  = 0;


    count = table->num_masks;
    count++;

    if ( count > table->max_masks )
    {
      error = ps_mask_table_ensure( table, count, memory );
      if ( error )
        goto Exit;
    }

    mask             = table->masks + count - 1;
    mask->num_bits   = 0;
    mask->end_point  = 0;
    table->num_masks = count;

  Exit:
    *amask = mask;
    return error;
  }


  /* return last hint mask in a table, create one if the table is empty */
  static FT_Error
  ps_mask_table_last( PS_Mask_Table  table,
                      FT_Memory      memory,
                      PS_Mask       *amask )
  {
    FT_Error  error = 0;
    FT_UInt   count;
    PS_Mask   mask;


    count = table->num_masks;
    if ( count == 0 )
    {
      error = ps_mask_table_alloc( table, memory, &mask );
      if ( error )
        goto Exit;
    }
    else
      mask = table->masks + count - 1;

  Exit:
    *amask = mask;
    return error;
  }


  /* set a new mask to a given bit range */
  static FT_Error
  ps_mask_table_set_bits( PS_Mask_Table  table,
                          FT_Byte*       source,
                          FT_UInt        bit_pos,
                          FT_UInt        bit_count,
                          FT_Memory      memory )
  {
    FT_Error  error = 0;
    PS_Mask   mask;


    error = ps_mask_table_last( table, memory, &mask );
    if ( error )
      goto Exit;

    error = ps_mask_ensure( mask, bit_count, memory );
    if ( error )
      goto Exit;

    mask->num_bits = bit_count;

    /* now, copy bits */
    {
      FT_Byte*  read  = source + ( bit_pos >> 3 );
      FT_Int    rmask = 0x80 >> ( bit_pos & 7 );
      FT_Byte*  write = mask->bytes;
      FT_Int    wmask = 0x80;
      FT_Int    val;


      for ( ; bit_count > 0; bit_count-- )
      {
        val = write[0] & ~wmask;

        if ( read[0] & rmask )
          val |= wmask;

        write[0] = (FT_Byte)val;

        rmask >>= 1;
        if ( rmask == 0 )
        {
          read++;
          rmask = 0x80;
        }

        wmask >>= 1;
        if ( wmask == 0 )
        {
          write++;
          wmask = 0x80;
        }
      }
    }

  Exit:
    return error;
  }


  /* test whether two masks in a table intersect */
  static FT_Int
  ps_mask_table_test_intersect( PS_Mask_Table  table,
                                FT_Int         index1,
                                FT_Int         index2 )
  {
    PS_Mask   mask1  = table->masks + index1;
    PS_Mask   mask2  = table->masks + index2;
    FT_Byte*  p1     = mask1->bytes;
    FT_Byte*  p2     = mask2->bytes;
    FT_UInt   count1 = mask1->num_bits;
    FT_UInt   count2 = mask2->num_bits;
    FT_UInt   count;


    count = ( count1 <= count2 ) ? count1 : count2;
    for ( ; count >= 8; count -= 8 )
    {
      if ( p1[0] & p2[0] )
        return 1;

      p1++;
      p2++;
    }

    if ( count == 0 )
      return 0;

    return ( p1[0] & p2[0] ) & ~( 0xFF >> count );
  }


  /* merge two masks, used by ps_mask_table_merge_all */
  static FT_Error
  ps_mask_table_merge( PS_Mask_Table  table,
                       FT_Int         index1,
                       FT_Int         index2,
                       FT_Memory      memory )
  {
    FT_UInt   temp;
    FT_Error  error = 0;


    /* swap index1 and index2 so that index1 < index2 */
    if ( index1 > index2 )
    {
      temp   = index1;
      index1 = index2;
      index2 = temp;
    }

    if ( index1 < index2 && index1 >= 0 && index2 < (FT_Int)table->num_masks )
    {
      /* we need to merge the bitsets of index1 and index2 with a */
      /* simple union                                             */
      PS_Mask  mask1  = table->masks + index1;
      PS_Mask  mask2  = table->masks + index2;
      FT_UInt  count1 = mask1->num_bits;
      FT_UInt  count2 = mask2->num_bits;
      FT_Int   delta;


      if ( count2 > 0 )
      {
        FT_UInt   pos;
        FT_Byte*  read;
        FT_Byte*  write;


        /* if "count2" is greater than "count1", we need to grow the */
        /* first bitset, and clear the highest bits                  */
        if ( count2 > count1 )
        {
          error = ps_mask_ensure( mask1, count2, memory );
          if ( error )
            goto Exit;

          for ( pos = count1; pos < count2; pos++ )
            ps_mask_clear_bit( mask1, pos );
        }

        /* merge (unite) the bitsets */
        read  = mask2->bytes;
        write = mask1->bytes;
        pos   = (FT_UInt)( ( count2 + 7 ) >> 3 );

        for ( ; pos > 0; pos-- )
        {
          write[0] = (FT_Byte)( write[0] | read[0] );
          write++;
          read++;
        }
      }

      /* Now, remove "mask2" from the list.  We need to keep the masks */
      /* sorted in order of importance, so move table elements.        */
      mask2->num_bits  = 0;
      mask2->end_point = 0;

      delta = table->num_masks - 1 - index2; /* number of masks to move */
      if ( delta > 0 )
      {
        /* move to end of table for reuse */
        PS_MaskRec  dummy = *mask2;


        ft_memmove( mask2, mask2 + 1, delta * sizeof ( PS_MaskRec ) );

        mask2[delta] = dummy;
      }

      table->num_masks--;
    }
    else
      FT_ERROR(( "ps_mask_table_merge: ignoring invalid indices (%d,%d)\n",
                 index1, index2 ));

  Exit:
    return error;
  }


  /* Try to merge all masks in a given table.  This is used to merge */
  /* all counter masks into independent counter "paths".             */
  /*                                                                 */
  static FT_Error
  ps_mask_table_merge_all( PS_Mask_Table  table,
                           FT_Memory      memory )
  {
    FT_Int    index1, index2;
    FT_Error  error = 0;


    for ( index1 = table->num_masks - 1; index1 > 0; index1-- )
    {
      for ( index2 = index1 - 1; index2 >= 0; index2-- )
      {
        if ( ps_mask_table_test_intersect( table, index1, index2 ) )
        {
          error = ps_mask_table_merge( table, index2, index1, memory );
          if ( error )
            goto Exit;

          break;
        }
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    PS_DIMENSION MANAGEMENT                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* finalize a given dimension */
  static void
  ps_dimension_done( PS_Dimension  dimension,
                     FT_Memory     memory )
  {
    ps_mask_table_done( &dimension->counters, memory );
    ps_mask_table_done( &dimension->masks,    memory );
    ps_hint_table_done( &dimension->hints,    memory );
  }


  /* initialize a given dimension */
  static void
  ps_dimension_init( PS_Dimension  dimension )
  {
    dimension->hints.num_hints    = 0;
    dimension->masks.num_masks    = 0;
    dimension->counters.num_masks = 0;
  }


#if 0

  /* set a bit at a given index in the current hint mask */
  static FT_Error
  ps_dimension_set_mask_bit( PS_Dimension  dim,
                             FT_UInt       idx,
                             FT_Memory     memory )
  {
    PS_Mask  mask;
    FT_Error  error = 0;


    /* get last hint mask */
    error = ps_mask_table_last( &dim->masks, memory, &mask );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( mask, idx, memory );

  Exit:
    return error;
  }

#endif

  /* set the end point in a mask, called from "End" & "Reset" methods */
  static void
  ps_dimension_end_mask( PS_Dimension  dim,
                         FT_UInt       end_point )
  {
    FT_UInt  count = dim->masks.num_masks;
    PS_Mask  mask;


    if ( count > 0 )
    {
      mask            = dim->masks.masks + count - 1;
      mask->end_point = end_point;
    }
  }


  /* set the end point in the current mask, then create a new empty one */
  /* (called by "Reset" method)                                         */
  static FT_Error
  ps_dimension_reset_mask( PS_Dimension  dim,
                           FT_UInt       end_point,
                           FT_Memory     memory )
  {
    PS_Mask  mask;


    /* end current mask */
    ps_dimension_end_mask( dim, end_point );

    /* allocate new one */
    return ps_mask_table_alloc( &dim->masks, memory, &mask );
  }


  /* set a new mask, called from the "T2Stem" method */
  static FT_Error
  ps_dimension_set_mask_bits( PS_Dimension    dim,
                              const FT_Byte*  source,
                              FT_UInt         source_pos,
                              FT_UInt         source_bits,
                              FT_UInt         end_point,
                              FT_Memory       memory )
  {
    FT_Error  error = 0;


    /* reset current mask, if any */
    error = ps_dimension_reset_mask( dim, end_point, memory );
    if ( error )
      goto Exit;

    /* set bits in new mask */
    error = ps_mask_table_set_bits( &dim->masks, (FT_Byte*)source,
                                    source_pos, source_bits, memory );

  Exit:
    return error;
  }


  /* add a new single stem (called from "T1Stem" method) */
  static FT_Error
  ps_dimension_add_t1stem( PS_Dimension  dim,
                           FT_Int        pos,
                           FT_Int        len,
                           FT_Memory     memory,
                           FT_Int       *aindex )
  {
    FT_Error  error = 0;
    FT_UInt   flags = 0;


    /* detect ghost stem */
    if ( len < 0 )
    {
      flags |= PS_HINT_FLAG_GHOST;
      if ( len == -21 )
      {
        flags |= PS_HINT_FLAG_BOTTOM;
        pos   += len;
      }
      len = 0;
    }

    if ( aindex )
      *aindex = -1;

    /* now, lookup stem in the current hints table */
    {
      PS_Mask  mask;
      FT_UInt  idx;
      FT_UInt  max   = dim->hints.num_hints;
      PS_Hint  hint  = dim->hints.hints;


      for ( idx = 0; idx < max; idx++, hint++ )
      {
        if ( hint->pos == pos && hint->len == len )
          break;
      }

      /* we need to create a new hint in the table */
      if ( idx >= max )
      {
        error = ps_hint_table_alloc( &dim->hints, memory, &hint );
        if ( error )
          goto Exit;

        hint->pos   = pos;
        hint->len   = len;
        hint->flags = flags;
      }

      /* now, store the hint in the current mask */
      error = ps_mask_table_last( &dim->masks, memory, &mask );
      if ( error )
        goto Exit;

      error = ps_mask_set_bit( mask, idx, memory );
      if ( error )
        goto Exit;

      if ( aindex )
        *aindex = (FT_Int)idx;
    }

  Exit:
    return error;
  }


  /* add a "hstem3/vstem3" counter to our dimension table */
  static FT_Error
  ps_dimension_add_counter( PS_Dimension  dim,
                            FT_Int        hint1,
                            FT_Int        hint2,
                            FT_Int        hint3,
                            FT_Memory     memory )
  {
    FT_Error  error   = 0;
    FT_UInt   count   = dim->counters.num_masks;
    PS_Mask   counter = dim->counters.masks;


    /* try to find an existing counter mask that already uses */
    /* one of these stems here                                */
    for ( ; count > 0; count--, counter++ )
    {
      if ( ps_mask_test_bit( counter, hint1 ) ||
           ps_mask_test_bit( counter, hint2 ) ||
           ps_mask_test_bit( counter, hint3 ) )
        break;
    }

    /* creat a new counter when needed */
    if ( count == 0 )
    {
      error = ps_mask_table_alloc( &dim->counters, memory, &counter );
      if ( error )
        goto Exit;
    }

    /* now, set the bits for our hints in the counter mask */
    error = ps_mask_set_bit( counter, hint1, memory );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( counter, hint2, memory );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( counter, hint3, memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  /* end of recording session for a given dimension */
  static FT_Error
  ps_dimension_end( PS_Dimension  dim,
                    FT_UInt       end_point,
                    FT_Memory     memory )
  {
    /* end hint mask table */
    ps_dimension_end_mask( dim, end_point );

    /* merge all counter masks into independent "paths" */
    return ps_mask_table_merge_all( &dim->counters, memory );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    PS_RECORDER MANAGEMENT                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* destroy hints */
  FT_LOCAL( void )
  ps_hints_done( PS_Hints  hints )
  {
    FT_Memory  memory = hints->memory;


    ps_dimension_done( &hints->dimension[0], memory );
    ps_dimension_done( &hints->dimension[1], memory );

    hints->error  = 0;
    hints->memory = 0;
  }


  FT_LOCAL( FT_Error )
  ps_hints_init( PS_Hints   hints,
                 FT_Memory  memory )
  {
    FT_MEM_ZERO( hints, sizeof ( *hints ) );
    hints->memory = memory;
    return 0;
  }


  /* initialize a hints for a new session */
  static void
  ps_hints_open( PS_Hints      hints,
                 PS_Hint_Type  hint_type )
  {
    switch ( hint_type )
    {
    case PS_HINT_TYPE_1:
    case PS_HINT_TYPE_2:
      hints->error     = 0;
      hints->hint_type = hint_type;

      ps_dimension_init( &hints->dimension[0] );
      ps_dimension_init( &hints->dimension[1] );
      break;

    default:
      hints->error     = PSH_Err_Invalid_Argument;
      hints->hint_type = hint_type;

      FT_ERROR(( "ps_hints_open: invalid charstring type!\n" ));
      break;
    }
  }


  /* add one or more stems to the current hints table */
  static void
  ps_hints_stem( PS_Hints  hints,
                 FT_Int    dimension,
                 FT_UInt   count,
                 FT_Long*  stems )
  {
    if ( !hints->error )
    {
      /* limit "dimension" to 0..1 */
      if ( dimension < 0 || dimension > 1 )
      {
        FT_ERROR(( "ps_hints_stem: invalid dimension (%d) used\n",
                   dimension ));
        dimension = ( dimension != 0 );
      }

      /* record the stems in the current hints/masks table */
      switch ( hints->hint_type )
      {
      case PS_HINT_TYPE_1:  /* Type 1 "hstem" or "vstem" operator */
      case PS_HINT_TYPE_2:  /* Type 2 "hstem" or "vstem" operator */
        {
          PS_Dimension  dim = &hints->dimension[dimension];


          for ( ; count > 0; count--, stems += 2 )
          {
            FT_Error   error;
            FT_Memory  memory = hints->memory;


            error = ps_dimension_add_t1stem(
                      dim, (FT_Int)stems[0], (FT_Int)stems[1],
                      memory, NULL );
            if ( error )
            {
              FT_ERROR(( "ps_hints_stem: could not add stem"
                         " (%d,%d) to hints table\n", stems[0], stems[1] ));

              hints->error = error;
              return;
            }
          }
          break;
        }

      default:
        FT_ERROR(( "ps_hints_stem: called with invalid hint type (%d)\n",
                   hints->hint_type ));
        break;
      }
    }
  }


  /* add one Type1 counter stem to the current hints table */
  static void
  ps_hints_t1stem3( PS_Hints  hints,
                    FT_Int    dimension,
                    FT_Long*  stems )
  {
    FT_Error  error = 0;


    if ( !hints->error )
    {
      PS_Dimension  dim;
      FT_Memory     memory = hints->memory;
      FT_Int        count;
      FT_Int        idx[3];


      /* limit "dimension" to 0..1 */
      if ( dimension < 0 || dimension > 1 )
      {
        FT_ERROR(( "ps_hints_t1stem3: invalid dimension (%d) used\n",
                   dimension ));
        dimension = ( dimension != 0 );
      }

      dim = &hints->dimension[dimension];

      /* there must be 6 elements in the 'stem' array */
      if ( hints->hint_type == PS_HINT_TYPE_1 )
      {
        /* add the three stems to our hints/masks table */
        for ( count = 0; count < 3; count++, stems += 2 )
        {
          error = ps_dimension_add_t1stem(
                    dim, (FT_Int)stems[0], (FT_Int)stems[1],
                    memory, &idx[count] );
          if ( error )
            goto Fail;
        }

        /* now, add the hints to the counters table */
        error = ps_dimension_add_counter( dim, idx[0], idx[1], idx[2],
                                          memory );
        if ( error )
          goto Fail;
      }
      else
      {
        FT_ERROR(( "ps_hints_t1stem3: called with invalid hint type!\n" ));
        error = PSH_Err_Invalid_Argument;
        goto Fail;
      }
    }

    return;

  Fail:
    FT_ERROR(( "ps_hints_t1stem3: could not add counter stems to table\n" ));
    hints->error = error;
  }


  /* reset hints (only with Type 1 hints) */
  static void
  ps_hints_t1reset( PS_Hints  hints,
                    FT_UInt   end_point )
  {
    FT_Error  error = 0;


    if ( !hints->error )
    {
      FT_Memory  memory = hints->memory;


      if ( hints->hint_type == PS_HINT_TYPE_1 )
      {
        error = ps_dimension_reset_mask( &hints->dimension[0],
                                         end_point, memory );
        if ( error )
          goto Fail;

        error = ps_dimension_reset_mask( &hints->dimension[1],
                                         end_point, memory );
        if ( error )
          goto Fail;
      }
      else
      {
        /* invalid hint type */
        error = PSH_Err_Invalid_Argument;
        goto Fail;
      }
    }
    return;

  Fail:
    hints->error = error;
  }


  /* Type2 "hintmask" operator, add a new hintmask to each direction */
  static void
  ps_hints_t2mask( PS_Hints        hints,
                   FT_UInt         end_point,
                   FT_UInt         bit_count,
                   const FT_Byte*  bytes )
  {
    FT_Error  error;


    if ( !hints->error )
    {
      PS_Dimension  dim    = hints->dimension;
      FT_Memory     memory = hints->memory;
      FT_UInt       count1 = dim[0].hints.num_hints;
      FT_UInt       count2 = dim[1].hints.num_hints;


      /* check bit count; must be equal to current total hint count */
      if ( bit_count !=  count1 + count2 )
      {
        FT_ERROR(( "ps_hints_t2mask: "
                   "called with invalid bitcount %d (instead of %d)\n",
                   bit_count, count1 + count2 ));

        /* simply ignore the operator */
        return;
      }

      /* set-up new horizontal and vertical hint mask now */
      error = ps_dimension_set_mask_bits( &dim[0], bytes, count2, count1,
                                          end_point, memory );
      if ( error )
        goto Fail;

      error = ps_dimension_set_mask_bits( &dim[1], bytes, 0, count2,
                                          end_point, memory );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    hints->error = error;
  }


  static void
  ps_hints_t2counter( PS_Hints        hints,
                      FT_UInt         bit_count,
                      const FT_Byte*  bytes )
  {
    FT_Error  error;


    if ( !hints->error )
    {
      PS_Dimension  dim    = hints->dimension;
      FT_Memory     memory = hints->memory;
      FT_UInt       count1 = dim[0].hints.num_hints;
      FT_UInt       count2 = dim[1].hints.num_hints;


      /* check bit count, must be equal to current total hint count */
      if ( bit_count !=  count1 + count2 )
      {
        FT_ERROR(( "ps_hints_t2counter: "
                   "called with invalid bitcount %d (instead of %d)\n",
                   bit_count, count1 + count2 ));

        /* simply ignore the operator */
        return;
      }

      /* set-up new horizontal and vertical hint mask now */
      error = ps_dimension_set_mask_bits( &dim[0], bytes, 0, count1,
                                          0, memory );
      if ( error )
        goto Fail;

      error = ps_dimension_set_mask_bits( &dim[1], bytes, count1, count2,
                                          0, memory );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    hints->error = error;
  }


  /* end recording session */
  static FT_Error
  ps_hints_close( PS_Hints  hints,
                  FT_UInt   end_point )
  {
    FT_Error  error;


    error = hints->error;
    if ( !error )
    {
      FT_Memory     memory = hints->memory;
      PS_Dimension  dim    = hints->dimension;


      error = ps_dimension_end( &dim[0], end_point, memory );
      if ( !error )
      {
        error = ps_dimension_end( &dim[1], end_point, memory );
      }
    }

#ifdef DEBUG_HINTER
    if ( !error )
      ps_debug_hints = hints;
#endif
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                TYPE 1 HINTS RECORDING INTERFACE               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  t1_hints_open( T1_Hints  hints )
  {
    ps_hints_open( (PS_Hints)hints, PS_HINT_TYPE_1 );
  }

  static void
  t1_hints_stem( T1_Hints  hints,
                 FT_Int    dimension,
                 FT_Long*  coords )
  {
    ps_hints_stem( (PS_Hints)hints, dimension, 1, coords );
  }


  FT_LOCAL_DEF( void )
  t1_hints_funcs_init( T1_Hints_FuncsRec*  funcs )
  {
    FT_MEM_ZERO( (char*)funcs, sizeof ( *funcs ) );

    funcs->open  = (T1_Hints_OpenFunc)    t1_hints_open;
    funcs->close = (T1_Hints_CloseFunc)   ps_hints_close;
    funcs->stem  = (T1_Hints_SetStemFunc) t1_hints_stem;
    funcs->stem3 = (T1_Hints_SetStem3Func)ps_hints_t1stem3;
    funcs->reset = (T1_Hints_ResetFunc)   ps_hints_t1reset;
    funcs->apply = (T1_Hints_ApplyFunc)   ps_hints_apply;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                TYPE 2 HINTS RECORDING INTERFACE               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  t2_hints_open( T2_Hints  hints )
  {
    ps_hints_open( (PS_Hints)hints, PS_HINT_TYPE_2 );
  }


  static void
  t2_hints_stems( T2_Hints   hints,
                  FT_Int     dimension,
                  FT_Int     count,
                  FT_Fixed*  coords )
  {
    FT_Pos  stems[32], y, n;
    FT_Int  total = count;


    y = 0;
    while ( total > 0 )
    {
      /* determine number of stems to write */
      count = total;
      if ( count > 16 )
        count = 16;

      /* compute integer stem positions in font units */
      for ( n = 0; n < count * 2; n++ )
      {
        y       += coords[n];
        stems[n] = ( y + 0x8000L ) >> 16;
      }

      /* compute lengths */
      for ( n = 0; n < count * 2; n += 2 )
        stems[n + 1] = stems[n + 1] - stems[n];

      /* add them to the current dimension */
      ps_hints_stem( (PS_Hints)hints, dimension, count, stems );

      total -= count;
    }
  }


  FT_LOCAL_DEF( void )
  t2_hints_funcs_init( T2_Hints_FuncsRec*  funcs )
  {
    FT_MEM_ZERO( funcs, sizeof ( *funcs ) );

    funcs->open    = (T2_Hints_OpenFunc)   t2_hints_open;
    funcs->close   = (T2_Hints_CloseFunc)  ps_hints_close;
    funcs->stems   = (T2_Hints_StemsFunc)  t2_hints_stems;
    funcs->hintmask= (T2_Hints_MaskFunc)   ps_hints_t2mask;
    funcs->counter = (T2_Hints_CounterFunc)ps_hints_t2counter;
    funcs->apply   = (T2_Hints_ApplyFunc)  ps_hints_apply;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  pshglob.c                                                              */
/*                                                                         */
/*    PostScript hinter global hinting management (body).                  */
/*    Inspired by the new auto-hinter module.                              */
/*                                                                         */
/*  Copyright 2001, 2002, 2003, 2004, 2006 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "pshglob.h"

#ifdef DEBUG_HINTER
  PSH_Globals  ps_debug_globals = 0;
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       STANDARD WIDTHS                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* scale the widths/heights table */
  static void
  psh_globals_scale_widths( PSH_Globals  globals,
                            FT_UInt      direction )
  {
    PSH_Dimension  dim   = &globals->dimension[direction];
    PSH_Widths     stdw  = &dim->stdw;
    FT_UInt        count = stdw->count;
    PSH_Width      width = stdw->widths;
    PSH_Width      stand = width;               /* standard width/height */
    FT_Fixed       scale = dim->scale_mult;


    if ( count > 0 )
    {
      width->cur = FT_MulFix( width->org, scale );
      width->fit = FT_PIX_ROUND( width->cur );

      width++;
      count--;

      for ( ; count > 0; count--, width++ )
      {
        FT_Pos  w, dist;


        w    = FT_MulFix( width->org, scale );
        dist = w - stand->cur;

        if ( dist < 0 )
          dist = -dist;

        if ( dist < 128 )
          w = stand->cur;

        width->cur = w;
        width->fit = FT_PIX_ROUND( w );
      }
    }
  }


#if 0

  /* org_width is is font units, result in device pixels, 26.6 format */
  FT_LOCAL_DEF( FT_Pos )
  psh_dimension_snap_width( PSH_Dimension  dimension,
                            FT_Int         org_width )
  {
    FT_UInt  n;
    FT_Pos   width     = FT_MulFix( org_width, dimension->scale_mult );
    FT_Pos   best      = 64 + 32 + 2;
    FT_Pos   reference = width;


    for ( n = 0; n < dimension->stdw.count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = dimension->stdw.widths[n].cur;
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    if ( width >= reference )
    {
      width -= 0x21;
      if ( width < reference )
        width = reference;
    }
    else
    {
      width += 0x21;
      if ( width > reference )
        width = reference;
    }

    return width;
  }

#endif /* 0 */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       BLUE ZONES                              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  psh_blues_set_zones_0( PSH_Blues       target,
                         FT_Bool         is_others,
                         FT_UInt         read_count,
                         FT_Short*       read,
                         PSH_Blue_Table  top_table,
                         PSH_Blue_Table  bot_table )
  {
    FT_UInt  count_top = top_table->count;
    FT_UInt  count_bot = bot_table->count;
    FT_Bool  first     = 1;

    FT_UNUSED( target );


    for ( ; read_count > 1; read_count -= 2 )
    {
      FT_Int         reference, delta;
      FT_UInt        count;
      PSH_Blue_Zone  zones, zone;
      FT_Bool        top;


      /* read blue zone entry, and select target top/bottom zone */
      top = 0;
      if ( first || is_others )
      {
        reference = read[1];
        delta     = read[0] - reference;

        zones = bot_table->zones;
        count = count_bot;
        first = 0;
      }
      else
      {
        reference = read[0];
        delta     = read[1] - reference;

        zones = top_table->zones;
        count = count_top;
        top   = 1;
      }

      /* insert into sorted table */
      zone = zones;
      for ( ; count > 0; count--, zone++ )
      {
        if ( reference < zone->org_ref )
          break;

        if ( reference == zone->org_ref )
        {
          FT_Int  delta0 = zone->org_delta;


          /* we have two zones on the same reference position -- */
          /* only keep the largest one                           */
          if ( delta < 0 )
          {
            if ( delta < delta0 )
              zone->org_delta = delta;
          }
          else
          {
            if ( delta > delta0 )
              zone->org_delta = delta;
          }
          goto Skip;
        }
      }

      for ( ; count > 0; count-- )
        zone[count] = zone[count-1];

      zone->org_ref   = reference;
      zone->org_delta = delta;

      if ( top )
        count_top++;
      else
        count_bot++;

    Skip:
      read += 2;
    }

    top_table->count = count_top;
    bot_table->count = count_bot;
  }


  /* Re-read blue zones from the original fonts and store them into out */
  /* private structure.  This function re-orders, sanitizes and         */
  /* fuzz-expands the zones as well.                                    */
  static void
  psh_blues_set_zones( PSH_Blues  target,
                       FT_UInt    count,
                       FT_Short*  blues,
                       FT_UInt    count_others,
                       FT_Short*  other_blues,
                       FT_Int     fuzz,
                       FT_Int     family )
  {
    PSH_Blue_Table  top_table, bot_table;
    FT_Int          count_top, count_bot;


    if ( family )
    {
      top_table = &target->family_top;
      bot_table = &target->family_bottom;
    }
    else
    {
      top_table = &target->normal_top;
      bot_table = &target->normal_bottom;
    }

    /* read the input blue zones, and build two sorted tables  */
    /* (one for the top zones, the other for the bottom zones) */
    top_table->count = 0;
    bot_table->count = 0;

    /* first, the blues */
    psh_blues_set_zones_0( target, 0,
                           count, blues, top_table, bot_table );
    psh_blues_set_zones_0( target, 1,
                           count_others, other_blues, top_table, bot_table );

    count_top = top_table->count;
    count_bot = bot_table->count;

    /* sanitize top table */
    if ( count_top > 0 )
    {
      PSH_Blue_Zone  zone = top_table->zones;


      for ( count = count_top; count > 0; count--, zone++ )
      {
        FT_Int  delta;


        if ( count > 1 )
        {
          delta = zone[1].org_ref - zone[0].org_ref;
          if ( zone->org_delta > delta )
            zone->org_delta = delta;
        }

        zone->org_bottom = zone->org_ref;
        zone->org_top    = zone->org_delta + zone->org_ref;
      }
    }

    /* sanitize bottom table */
    if ( count_bot > 0 )
    {
      PSH_Blue_Zone  zone = bot_table->zones;


      for ( count = count_bot; count > 0; count--, zone++ )
      {
        FT_Int  delta;


        if ( count > 1 )
        {
          delta = zone[0].org_ref - zone[1].org_ref;
          if ( zone->org_delta < delta )
            zone->org_delta = delta;
        }

        zone->org_top    = zone->org_ref;
        zone->org_bottom = zone->org_delta + zone->org_ref;
      }
    }

    /* expand top and bottom tables with blue fuzz */
    {
      FT_Int         dim, top, bot, delta;
      PSH_Blue_Zone  zone;


      zone  = top_table->zones;
      count = count_top;

      for ( dim = 1; dim >= 0; dim-- )
      {
        if ( count > 0 )
        {
          /* expand the bottom of the lowest zone normally */
          zone->org_bottom -= fuzz;

          /* expand the top and bottom of intermediate zones;    */
          /* checking that the interval is smaller than the fuzz */
          top = zone->org_top;

          for ( count--; count > 0; count-- )
          {
            bot   = zone[1].org_bottom;
            delta = bot - top;

            if ( delta < 2 * fuzz )
              zone[0].org_top = zone[1].org_bottom = top + delta / 2;
            else
            {
              zone[0].org_top    = top + fuzz;
              zone[1].org_bottom = bot - fuzz;
            }

            zone++;
            top = zone->org_top;
          }

          /* expand the top of the highest zone normally */
          zone->org_top = top + fuzz;
        }
        zone  = bot_table->zones;
        count = count_bot;
      }
    }
  }


  /* reset the blues table when the device transform changes */
  static void
  psh_blues_scale_zones( PSH_Blues  blues,
                         FT_Fixed   scale,
                         FT_Pos     delta )
  {
    FT_UInt         count;
    FT_UInt         num;
    PSH_Blue_Table  table = 0;

    /*                                                        */
    /* Determine whether we need to suppress overshoots or    */
    /* not.  We simply need to compare the vertical scale     */
    /* parameter to the raw bluescale value.  Here is why:    */
    /*                                                        */
    /*   We need to suppress overshoots for all pointsizes.   */
    /*   At 300dpi that satisfies:                            */
    /*                                                        */
    /*      pointsize < 240*bluescale + 0.49                  */
    /*                                                        */
    /*   This corresponds to:                                 */
    /*                                                        */
    /*      pixelsize < 1000*bluescale + 49/24                */
    /*                                                        */
    /*      scale*EM_Size < 1000*bluescale + 49/24            */
    /*                                                        */
    /*   However, for normal Type 1 fonts, EM_Size is 1000!   */
    /*   We thus only check:                                  */
    /*                                                        */
    /*      scale < bluescale + 49/24000                      */
    /*                                                        */
    /*   which we shorten to                                  */
    /*                                                        */
    /*      "scale < bluescale"                               */
    /*                                                        */
    /* Note that `blue_scale' is stored 1000 times its real   */
    /* value, and that `scale' converts from font units to    */
    /* fractional pixels.                                     */
    /*                                                        */

    /* 1000 / 64 = 125 / 8 */
    if ( scale >= 0x20C49BAL )
      blues->no_overshoots = FT_BOOL( scale < blues->blue_scale * 8 / 125 );
    else
      blues->no_overshoots = FT_BOOL( scale * 125 < blues->blue_scale * 8 );

    /*                                                        */
    /*  The blue threshold is the font units distance under   */
    /*  which overshoots are suppressed due to the BlueShift  */
    /*  even if the scale is greater than BlueScale.          */
    /*                                                        */
    /*  It is the smallest distance such that                 */
    /*                                                        */
    /*    dist <= BlueShift && dist*scale <= 0.5 pixels       */
    /*                                                        */
    {
      FT_Int  threshold = blues->blue_shift;


      while ( threshold > 0 && FT_MulFix( threshold, scale ) > 32 )
        threshold--;

      blues->blue_threshold = threshold;
    }

    for ( num = 0; num < 4; num++ )
    {
      PSH_Blue_Zone  zone;


      switch ( num )
      {
      case 0:
        table = &blues->normal_top;
        break;
      case 1:
        table = &blues->normal_bottom;
        break;
      case 2:
        table = &blues->family_top;
        break;
      default:
        table = &blues->family_bottom;
        break;
      }

      zone  = table->zones;
      count = table->count;
      for ( ; count > 0; count--, zone++ )
      {
        zone->cur_top    = FT_MulFix( zone->org_top,    scale ) + delta;
        zone->cur_bottom = FT_MulFix( zone->org_bottom, scale ) + delta;
        zone->cur_ref    = FT_MulFix( zone->org_ref,    scale ) + delta;
        zone->cur_delta  = FT_MulFix( zone->org_delta,  scale );

        /* round scaled reference position */
        zone->cur_ref = FT_PIX_ROUND( zone->cur_ref );

#if 0
        if ( zone->cur_ref > zone->cur_top )
          zone->cur_ref -= 64;
        else if ( zone->cur_ref < zone->cur_bottom )
          zone->cur_ref += 64;
#endif
      }
    }

    /* process the families now */

    for ( num = 0; num < 2; num++ )
    {
      PSH_Blue_Zone   zone1, zone2;
      FT_UInt         count1, count2;
      PSH_Blue_Table  normal, family;


      switch ( num )
      {
      case 0:
        normal = &blues->normal_top;
        family = &blues->family_top;
        break;

      default:
        normal = &blues->normal_bottom;
        family = &blues->family_bottom;
      }

      zone1  = normal->zones;
      count1 = normal->count;

      for ( ; count1 > 0; count1--, zone1++ )
      {
        /* try to find a family zone whose reference position is less */
        /* than 1 pixel far from the current zone                     */
        zone2  = family->zones;
        count2 = family->count;

        for ( ; count2 > 0; count2--, zone2++ )
        {
          FT_Pos  Delta;


          Delta = zone1->org_ref - zone2->org_ref;
          if ( Delta < 0 )
            Delta = -Delta;

          if ( FT_MulFix( Delta, scale ) < 64 )
          {
            zone1->cur_top    = zone2->cur_top;
            zone1->cur_bottom = zone2->cur_bottom;
            zone1->cur_ref    = zone2->cur_ref;
            zone1->cur_delta  = zone2->cur_delta;
            break;
          }
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  psh_blues_snap_stem( PSH_Blues      blues,
                       FT_Int         stem_top,
                       FT_Int         stem_bot,
                       PSH_Alignment  alignment )
  {
    PSH_Blue_Table  table;
    FT_UInt         count;
    FT_Pos          delta;
    PSH_Blue_Zone   zone;
    FT_Int          no_shoots;


    alignment->align = PSH_BLUE_ALIGN_NONE;

    no_shoots = blues->no_overshoots;

    /* look up stem top in top zones table */
    table = &blues->normal_top;
    count = table->count;
    zone  = table->zones;

    for ( ; count > 0; count--, zone++ )
    {
      delta = stem_top - zone->org_bottom;
      if ( delta < -blues->blue_fuzz )
        break;

      if ( stem_top <= zone->org_top + blues->blue_fuzz )
      {
        if ( no_shoots || delta <= blues->blue_threshold )
        {
          alignment->align    |= PSH_BLUE_ALIGN_TOP;
          alignment->align_top = zone->cur_ref;
        }
        break;
      }
    }

    /* look up stem bottom in bottom zones table */
    table = &blues->normal_bottom;
    count = table->count;
    zone  = table->zones + count-1;

    for ( ; count > 0; count--, zone-- )
    {
      delta = zone->org_top - stem_bot;
      if ( delta < -blues->blue_fuzz )
        break;

      if ( stem_bot >= zone->org_bottom - blues->blue_fuzz )
      {
        if ( no_shoots || delta < blues->blue_threshold )
        {
          alignment->align    |= PSH_BLUE_ALIGN_BOT;
          alignment->align_bot = zone->cur_ref;
        }
        break;
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        GLOBAL HINTS                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  psh_globals_destroy( PSH_Globals  globals )
  {
    if ( globals )
    {
      FT_Memory  memory;


      memory = globals->memory;
      globals->dimension[0].stdw.count = 0;
      globals->dimension[1].stdw.count = 0;

      globals->blues.normal_top.count    = 0;
      globals->blues.normal_bottom.count = 0;
      globals->blues.family_top.count    = 0;
      globals->blues.family_bottom.count = 0;

      FT_FREE( globals );

#ifdef DEBUG_HINTER
      ps_debug_globals = 0;
#endif
    }
  }


  static FT_Error
  psh_globals_new( FT_Memory     memory,
                   T1_Private*   priv,
                   PSH_Globals  *aglobals )
  {
    PSH_Globals  globals;
    FT_Error     error;


    if ( !FT_NEW( globals ) )
    {
      FT_UInt    count;
      FT_Short*  read;


      globals->memory = memory;

      /* copy standard widths */
      {
        PSH_Dimension  dim   = &globals->dimension[1];
        PSH_Width      write = dim->stdw.widths;


        write->org = priv->standard_width[0];
        write++;

        read = priv->snap_widths;
        for ( count = priv->num_snap_widths; count > 0; count-- )
        {
          write->org = *read;
          write++;
          read++;
        }

        dim->stdw.count = priv->num_snap_widths + 1;
      }

      /* copy standard heights */
      {
        PSH_Dimension  dim = &globals->dimension[0];
        PSH_Width      write = dim->stdw.widths;


        write->org = priv->standard_height[0];
        write++;
        read = priv->snap_heights;
        for ( count = priv->num_snap_heights; count > 0; count-- )
        {
          write->org = *read;
          write++;
          read++;
        }

        dim->stdw.count = priv->num_snap_heights + 1;
      }

      /* copy blue zones */
      psh_blues_set_zones( &globals->blues, priv->num_blue_values,
                           priv->blue_values, priv->num_other_blues,
                           priv->other_blues, priv->blue_fuzz, 0 );

      psh_blues_set_zones( &globals->blues, priv->num_family_blues,
                           priv->family_blues, priv->num_family_other_blues,
                           priv->family_other_blues, priv->blue_fuzz, 1 );

      globals->blues.blue_scale = priv->blue_scale;
      globals->blues.blue_shift = priv->blue_shift;
      globals->blues.blue_fuzz  = priv->blue_fuzz;

      globals->dimension[0].scale_mult  = 0;
      globals->dimension[0].scale_delta = 0;
      globals->dimension[1].scale_mult  = 0;
      globals->dimension[1].scale_delta = 0;

#ifdef DEBUG_HINTER
      ps_debug_globals = globals;
#endif
    }

    *aglobals = globals;
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  psh_globals_set_scale( PSH_Globals  globals,
                         FT_Fixed     x_scale,
                         FT_Fixed     y_scale,
                         FT_Fixed     x_delta,
                         FT_Fixed     y_delta )
  {
    PSH_Dimension  dim = &globals->dimension[0];


    dim = &globals->dimension[0];
    if ( x_scale != dim->scale_mult  ||
         x_delta != dim->scale_delta )
    {
      dim->scale_mult  = x_scale;
      dim->scale_delta = x_delta;

      psh_globals_scale_widths( globals, 0 );
    }

    dim = &globals->dimension[1];
    if ( y_scale != dim->scale_mult  ||
         y_delta != dim->scale_delta )
    {
      dim->scale_mult  = y_scale;
      dim->scale_delta = y_delta;

      psh_globals_scale_widths( globals, 1 );
      psh_blues_scale_zones( &globals->blues, y_scale, y_delta );
    }

    return 0;
  }


  FT_LOCAL_DEF( void )
  psh_globals_funcs_init( PSH_Globals_FuncsRec*  funcs )
  {
    funcs->create    = psh_globals_new;
    funcs->set_scale = psh_globals_set_scale;
    funcs->destroy   = psh_globals_destroy;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  pshalgo.c                                                              */
/*                                                                         */
/*    PostScript hinting algorithm (body).                                 */
/*                                                                         */
/*  Copyright 2001, 2002, 2003, 2004, 2005 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include "pshalgo.h"

#include "pshnterr.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshalgo2


#ifdef DEBUG_HINTER
  PSH_Hint_Table  ps_debug_hint_table = 0;
  PSH_HintFunc    ps_debug_hint_func  = 0;
  PSH_Glyph       ps_debug_glyph      = 0;
#endif


#define  COMPUTE_INFLEXS  /* compute inflection points to optimize `S' */
                          /* and similar glyphs                        */
#define  STRONGER         /* slightly increase the contrast of smooth  */
                          /* hinting                                   */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  BASIC HINTS RECORDINGS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* return true if two stem hints overlap */
  static FT_Int
  psh_hint_overlap( PSH_Hint  hint1,
                    PSH_Hint  hint2 )
  {
    return hint1->org_pos + hint1->org_len >= hint2->org_pos &&
           hint2->org_pos + hint2->org_len >= hint1->org_pos;
  }


  /* destroy hints table */
  static void
  psh_hint_table_done( PSH_Hint_Table  table,
                       FT_Memory       memory )
  {
    FT_FREE( table->zones );
    table->num_zones = 0;
    table->zone      = 0;

    FT_FREE( table->sort );
    FT_FREE( table->hints );
    table->num_hints   = 0;
    table->max_hints   = 0;
    table->sort_global = 0;
  }


  /* deactivate all hints in a table */
  static void
  psh_hint_table_deactivate( PSH_Hint_Table  table )
  {
    FT_UInt   count = table->max_hints;
    PSH_Hint  hint  = table->hints;


    for ( ; count > 0; count--, hint++ )
    {
      psh_hint_deactivate( hint );
      hint->order = -1;
    }
  }


  /* internal function to record a new hint */
  static void
  psh_hint_table_record( PSH_Hint_Table  table,
                         FT_UInt         idx )
  {
    PSH_Hint  hint = table->hints + idx;


    if ( idx >= table->max_hints )
    {
      FT_ERROR(( "psh_hint_table_record: invalid hint index %d\n", idx ));
      return;
    }

    /* ignore active hints */
    if ( psh_hint_is_active( hint ) )
      return;

    psh_hint_activate( hint );

    /* now scan the current active hint set to check */
    /* whether `hint' overlaps with another hint     */
    {
      PSH_Hint*  sorted = table->sort_global;
      FT_UInt    count  = table->num_hints;
      PSH_Hint   hint2;


      hint->parent = 0;
      for ( ; count > 0; count--, sorted++ )
      {
        hint2 = sorted[0];

        if ( psh_hint_overlap( hint, hint2 ) )
        {
          hint->parent = hint2;
          break;
        }
      }
    }

    if ( table->num_hints < table->max_hints )
      table->sort_global[table->num_hints++] = hint;
    else
      FT_ERROR(( "psh_hint_table_record: too many sorted hints!  BUG!\n" ));
  }


  static void
  psh_hint_table_record_mask( PSH_Hint_Table  table,
                              PS_Mask         hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit;


    limit = hint_mask->num_bits;

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
        psh_hint_table_record( table, idx );

      mask >>= 1;
    }
  }


  /* create hints table */
  static FT_Error
  psh_hint_table_init( PSH_Hint_Table  table,
                       PS_Hint_Table   hints,
                       PS_Mask_Table   hint_masks,
                       PS_Mask_Table   counter_masks,
                       FT_Memory       memory )
  {
    FT_UInt   count;
    FT_Error  error;

    FT_UNUSED( counter_masks );


    count = hints->num_hints;

    /* allocate our tables */
    if ( FT_NEW_ARRAY( table->sort,  2 * count     ) ||
         FT_NEW_ARRAY( table->hints,     count     ) ||
         FT_NEW_ARRAY( table->zones, 2 * count + 1 ) )
      goto Exit;

    table->max_hints   = count;
    table->sort_global = table->sort + count;
    table->num_hints   = 0;
    table->num_zones   = 0;
    table->zone        = 0;

    /* initialize the `table->hints' array */
    {
      PSH_Hint  write = table->hints;
      PS_Hint   read  = hints->hints;


      for ( ; count > 0; count--, write++, read++ )
      {
        write->org_pos = read->pos;
        write->org_len = read->len;
        write->flags   = read->flags;
      }
    }

    /* we now need to determine the initial `parent' stems; first  */
    /* activate the hints that are given by the initial hint masks */
    if ( hint_masks )
    {
      PS_Mask  mask = hint_masks->masks;


      count             = hint_masks->num_masks;
      table->hint_masks = hint_masks;

      for ( ; count > 0; count--, mask++ )
        psh_hint_table_record_mask( table, mask );
    }

    /* finally, do a linear parse in case some hints were left alone */
    if ( table->num_hints != table->max_hints )
    {
      FT_UInt  idx;


      FT_ERROR(( "psh_hint_table_init: missing/incorrect hint masks!\n" ));

      count = table->max_hints;
      for ( idx = 0; idx < count; idx++ )
        psh_hint_table_record( table, idx );
    }

  Exit:
    return error;
  }


  static void
  psh_hint_table_activate_mask( PSH_Hint_Table  table,
                                PS_Mask         hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit, count;


    limit = hint_mask->num_bits;
    count = 0;

    psh_hint_table_deactivate( table );

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
      {
        PSH_Hint  hint = &table->hints[idx];


        if ( !psh_hint_is_active( hint ) )
        {
          FT_UInt     count2;

#if 0
          PSH_Hint*  sort = table->sort;
          PSH_Hint   hint2;


          for ( count2 = count; count2 > 0; count2--, sort++ )
          {
            hint2 = sort[0];
            if ( psh_hint_overlap( hint, hint2 ) )
              FT_ERROR(( "psh_hint_table_activate_mask:"
                         " found overlapping hints\n" ))
          }
#else
          count2 = 0;
#endif

          if ( count2 == 0 )
          {
            psh_hint_activate( hint );
            if ( count < table->max_hints )
              table->sort[count++] = hint;
            else
              FT_ERROR(( "psh_hint_tableactivate_mask:"
                         " too many active hints\n" ));
          }
        }
      }

      mask >>= 1;
    }
    table->num_hints = count;

    /* now, sort the hints; they are guaranteed to not overlap */
    /* so we can compare their "org_pos" field directly        */
    {
      FT_Int     i1, i2;
      PSH_Hint   hint1, hint2;
      PSH_Hint*  sort = table->sort;


      /* a simple bubble sort will do, since in 99% of cases, the hints */
      /* will be already sorted -- and the sort will be linear          */
      for ( i1 = 1; i1 < (FT_Int)count; i1++ )
      {
        hint1 = sort[i1];
        for ( i2 = i1 - 1; i2 >= 0; i2-- )
        {
          hint2 = sort[i2];

          if ( hint2->org_pos < hint1->org_pos )
            break;

          sort[i2 + 1] = hint2;
          sort[i2]     = hint1;
        }
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               HINTS GRID-FITTING AND OPTIMIZATION             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#if 1
  static FT_Pos
  psh_dimension_quantize_len( PSH_Dimension  dim,
                              FT_Pos         len,
                              FT_Bool        do_snapping )
  {
    if ( len <= 64 )
      len = 64;
    else
    {
      FT_Pos  delta = len - dim->stdw.widths[0].cur;


      if ( delta < 0 )
        delta = -delta;

      if ( delta < 40 )
      {
        len = dim->stdw.widths[0].cur;
        if ( len < 48 )
          len = 48;
      }

      if ( len < 3 * 64 )
      {
        delta = ( len & 63 );
        len  &= -64;

        if ( delta < 10 )
          len += delta;

        else if ( delta < 32 )
          len += 10;

        else if ( delta < 54 )
          len += 54;

        else
          len += delta;
      }
      else
        len = FT_PIX_ROUND( len );
    }

    if ( do_snapping )
      len = FT_PIX_ROUND( len );

    return  len;
  }
#endif /* 0 */


#ifdef DEBUG_HINTER

  static void
  ps_simple_scale( PSH_Hint_Table  table,
                   FT_Fixed        scale,
                   FT_Fixed        delta,
                   FT_Int          dimension )
  {
    PSH_Hint  hint;
    FT_UInt   count;


    for ( count = 0; count < table->max_hints; count++ )
    {
      hint = table->hints + count;

      hint->cur_pos = FT_MulFix( hint->org_pos, scale ) + delta;
      hint->cur_len = FT_MulFix( hint->org_len, scale );

      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
    }
  }

#endif /* DEBUG_HINTER */


  static FT_Fixed
  psh_hint_snap_stem_side_delta( FT_Fixed  pos,
                                 FT_Fixed  len )
  {
    FT_Fixed  delta1 = FT_PIX_ROUND( pos ) - pos;
    FT_Fixed  delta2 = FT_PIX_ROUND( pos + len ) - pos - len;


    if ( FT_ABS( delta1 ) <= FT_ABS( delta2 ) )
      return delta1;
    else
      return delta2;
  }


  static void
  psh_hint_align( PSH_Hint     hint,
                  PSH_Globals  globals,
                  FT_Int       dimension,
                  PSH_Glyph    glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh_hint_is_fitted( hint ) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Int            do_snapping;
      FT_Pos            fit_len;
      PSH_AlignmentRec  align;


      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh_hint_set_fitted( hint );
        return;
      }

      /* perform stem snapping when requested - this is necessary
       * for monochrome and LCD hinting modes only
       */
      do_snapping = ( dimension == 0 && glyph->do_horz_snapping ) ||
                    ( dimension == 1 && glyph->do_vert_snapping );

      hint->cur_len = fit_len = len;

      /* check blue zones for horizontal stems */
      align.align     = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh_hint_is_fitted( parent ) )
              psh_hint_align( parent, globals, dimension, glyph );

            /* keep original relation between hints, this is, use the */
            /* scaled distance between the centers of the hints to    */
            /* compute the new position                               */
            par_org_center = parent->org_pos + ( parent->org_len >> 1 );
            par_cur_center = parent->cur_pos + ( parent->cur_len >> 1 );
            cur_org_center = hint->org_pos   + ( hint->org_len   >> 1 );

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

          hint->cur_pos = pos;
          hint->cur_len = fit_len;

          /* Stem adjustment tries to snap stem widths to standard
           * ones.  This is important to prevent unpleasant rounding
           * artefacts.
           */
          if ( glyph->do_stem_adjust )
          {
            if ( len <= 64 )
            {
              /* the stem is less than one pixel; we will center it
               * around the nearest pixel center
               */
#if 1
              pos = FT_PIX_FLOOR( pos + ( len >> 1 ) );
#else
             /* this seems to be a bug! */
              pos = pos + FT_PIX_FLOOR( len >> 1 );
#endif
              len = 64;
            }
            else
            {
              len = psh_dimension_quantize_len( dim, len, 0 );
            }
          }

          /* now that we have a good hinted stem width, try to position */
          /* the stem along a pixel grid integer coordinate             */
          hint->cur_pos = pos + psh_hint_snap_stem_side_delta( pos, len );
          hint->cur_len = len;
        }
      }

      if ( do_snapping )
      {
        pos = hint->cur_pos;
        len = hint->cur_len;

        if ( len < 64 )
          len = 64;
        else
          len = FT_PIX_ROUND( len );

        switch ( align.align )
        {
          case PSH_BLUE_ALIGN_TOP:
            hint->cur_pos = align.align_top - len;
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT:
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT | PSH_BLUE_ALIGN_TOP:
            /* don't touch */
            break;


          default:
            hint->cur_len = len;
            if ( len & 64 )
              pos = FT_PIX_FLOOR( pos + ( len >> 1 ) ) + 32;
            else
              pos = FT_PIX_ROUND( pos + ( len >> 1 ) );

            hint->cur_pos = pos - ( len >> 1 );
            hint->cur_len = len;
        }
      }

      psh_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
#endif
    }
  }


#if 0  /* not used for now, experimental */

 /*
  *  A variant to perform "light" hinting (i.e. FT_RENDER_MODE_LIGHT)
  *  of stems
  */
  static void
  psh_hint_align_light( PSH_Hint     hint,
                        PSH_Globals  globals,
                        FT_Int       dimension,
                        PSH_Glyph    glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh_hint_is_fitted( hint ) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Pos  fit_len;

      PSH_AlignmentRec  align;


      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh_hint_set_fitted( hint );
        return;
      }

      fit_len = len;

      hint->cur_len = fit_len;

      /* check blue zones for horizontal stems */
      align.align = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh_hint_is_fitted( parent ) )
              psh_hint_align_light( parent, globals, dimension, glyph );

            par_org_center = parent->org_pos + ( parent->org_len / 2 );
            par_cur_center = parent->cur_pos + ( parent->cur_len / 2 );
            cur_org_center = hint->org_pos   + ( hint->org_len   / 2 );

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

          /* Stems less than one pixel wide are easy -- we want to
           * make them as dark as possible, so they must fall within
           * one pixel.  If the stem is split between two pixels
           * then snap the edge that is nearer to the pixel boundary
           * to the pixel boundary.
           */
          if ( len <= 64 )
          {
            if ( ( pos + len + 63 ) / 64  != pos / 64 + 1 )
              pos += psh_hint_snap_stem_side_delta ( pos, len );
          }

          /* Position stems other to minimize the amount of mid-grays.
           * There are, in general, two positions that do this,
           * illustrated as A) and B) below.
           *
           *   +                   +                   +                   +
           *
           * A)             |--------------------------------|
           * B)   |--------------------------------|
           * C)       |--------------------------------|
           *
           * Position A) (split the excess stem equally) should be better
           * for stems of width N + f where f < 0.5.
           *
           * Position B) (split the deficiency equally) should be better
           * for stems of width N + f where f > 0.5.
           *
           * It turns out though that minimizing the total number of lit
           * pixels is also important, so position C), with one edge
           * aligned with a pixel boundary is actually preferable
           * to A).  There are also more possibile positions for C) than
           * for A) or B), so it involves less distortion of the overall
           * character shape.
           */
          else /* len > 64 */
          {
            FT_Fixed  frac_len = len & 63;
            FT_Fixed  center = pos + ( len >> 1 );
            FT_Fixed  delta_a, delta_b;


            if ( ( len / 64 ) & 1 )
            {
              delta_a = FT_PIX_FLOOR( center ) + 32 - center;
              delta_b = FT_PIX_ROUND( center ) - center;
            }
            else
            {
              delta_a = FT_PIX_ROUND( center ) - center;
              delta_b = FT_PIX_FLOOR( center ) + 32 - center;
            }

            /* We choose between B) and C) above based on the amount
             * of fractinal stem width; for small amounts, choose
             * C) always, for large amounts, B) always, and inbetween,
             * pick whichever one involves less stem movement.
             */
            if ( frac_len < 32 )
            {
              pos += psh_hint_snap_stem_side_delta ( pos, len );
            }
            else if ( frac_len < 48 )
            {
              FT_Fixed  side_delta = psh_hint_snap_stem_side_delta ( pos,
                                                                     len );

              if ( FT_ABS( side_delta ) < FT_ABS( delta_b ) )
                pos += side_delta;
              else
                pos += delta_b;
            }
            else
            {
              pos += delta_b;
            }
          }

          hint->cur_pos = pos;
        }
      }  /* switch */

      psh_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
#endif
    }
  }

#endif /* 0 */


  static void
  psh_hint_table_align_hints( PSH_Hint_Table  table,
                              PSH_Globals     globals,
                              FT_Int          dimension,
                              PSH_Glyph       glyph )
  {
    PSH_Hint       hint;
    FT_UInt        count;

#ifdef DEBUG_HINTER

    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( ps_debug_no_vert_hints && dimension == 0 )
    {
      ps_simple_scale( table, scale, delta, dimension );
      return;
    }

    if ( ps_debug_no_horz_hints && dimension == 1 )
    {
      ps_simple_scale( table, scale, delta, dimension );
      return;
    }

#endif /* DEBUG_HINTER*/

    hint  = table->hints;
    count = table->max_hints;

    for ( ; count > 0; count--, hint++ )
      psh_hint_align( hint, globals, dimension, glyph );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                POINTS INTERPOLATION ROUTINES                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define PSH_ZONE_MIN  -3200000L
#define PSH_ZONE_MAX  +3200000L

#define xxDEBUG_ZONES


#ifdef DEBUG_ZONES

#include <stdio.h>

  static void
  psh_print_zone( PSH_Zone  zone )
  {
    printf( "zone [scale,delta,min,max] = [%.3f,%.3f,%d,%d]\n",
             zone->scale / 65536.0,
             zone->delta / 64.0,
             zone->min,
             zone->max );
  }

#else

#define psh_print_zone( x )  do { } while ( 0 )

#endif /* DEBUG_ZONES */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    HINTER GLYPH MANAGEMENT                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#ifdef COMPUTE_INFLEXS

  /* compute all inflex points in a given glyph */
  static void
  psh_glyph_compute_inflections( PSH_Glyph  glyph )
  {
    FT_UInt  n;


    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH_Point  first, start, end, before, after;
      FT_Angle   angle_in, angle_seg, angle_out;
      FT_Angle   diff_in, diff_out;
      FT_Int     finished = 0;


      /* we need at least 4 points to create an inflection point */
      if ( glyph->contours[n].count < 4 )
        continue;

      /* compute first segment in contour */
      first = glyph->contours[n].start;

      start = end = first;
      do
      {
        end = end->next;
        if ( end == first )
          goto Skip;

      } while ( PSH_POINT_EQUAL_ORG( end, first ) );

      angle_seg = PSH_POINT_ANGLE( start, end );

      /* extend the segment start whenever possible */
      before = start;
      do
      {
        do
        {
          start  = before;
          before = before->prev;
          if ( before == first )
            goto Skip;

        } while ( PSH_POINT_EQUAL_ORG( before, start ) );

        angle_in = PSH_POINT_ANGLE( before, start );

      } while ( angle_in == angle_seg );

      first   = start;
      diff_in = FT_Angle_Diff( angle_in, angle_seg );

      /* now, process all segments in the contour */
      do
      {
        /* first, extend current segment's end whenever possible */
        after = end;
        do
        {
          do
          {
            end   = after;
            after = after->next;
            if ( after == first )
              finished = 1;

          } while ( PSH_POINT_EQUAL_ORG( end, after ) );

          angle_out = PSH_POINT_ANGLE( end, after );

        } while ( angle_out == angle_seg );

        diff_out = FT_Angle_Diff( angle_seg, angle_out );

        if ( ( diff_in ^ diff_out ) < 0 )
        {
          /* diff_in and diff_out have different signs, we have */
          /* inflection points here...                          */

          do
          {
            psh_point_set_inflex( start );
            start = start->next;
          }
          while ( start != end );

          psh_point_set_inflex( start );
        }

        start     = end;
        end       = after;
        angle_seg = angle_out;
        diff_in   = diff_out;

      } while ( !finished );

    Skip:
      ;
    }
  }

#endif /* COMPUTE_INFLEXS */


  static void
  psh_glyph_done( PSH_Glyph  glyph )
  {
    FT_Memory  memory = glyph->memory;


    psh_hint_table_done( &glyph->hint_tables[1], memory );
    psh_hint_table_done( &glyph->hint_tables[0], memory );

    FT_FREE( glyph->points );
    FT_FREE( glyph->contours );

    glyph->num_points   = 0;
    glyph->num_contours = 0;

    glyph->memory = 0;
  }


  static int
  psh_compute_dir( FT_Pos  dx,
                   FT_Pos  dy )
  {
    FT_Pos  ax, ay;
    int     result = PSH_DIR_NONE;


    ax = ( dx >= 0 ) ? dx : -dx;
    ay = ( dy >= 0 ) ? dy : -dy;

    if ( ay * 12 < ax )
    {
      /* |dy| <<< |dx|  means a near-horizontal segment */
      result = ( dx >= 0 ) ? PSH_DIR_RIGHT : PSH_DIR_LEFT;
    }
    else if ( ax * 12 < ay )
    {
      /* |dx| <<< |dy|  means a near-vertical segment */
      result = ( dy >= 0 ) ? PSH_DIR_UP : PSH_DIR_DOWN;
    }

    return result;
  }


  /* load outline point coordinates into hinter glyph */
  static void
  psh_glyph_load_points( PSH_Glyph  glyph,
                         FT_Int     dimension )
  {
    FT_Vector*  vec   = glyph->outline->points;
    PSH_Point   point = glyph->points;
    FT_UInt     count = glyph->num_points;


    for ( ; count > 0; count--, point++, vec++ )
    {
      point->flags2 = 0;
      point->hint   = NULL;
      if ( dimension == 0 )
      {
        point->org_u = vec->x;
        point->org_v = vec->y;
      }
      else
      {
        point->org_u = vec->y;
        point->org_v = vec->x;
      }

#ifdef DEBUG_HINTER
      point->org_x = vec->x;
      point->org_y = vec->y;
#endif

    }
  }


  /* save hinted point coordinates back to outline */
  static void
  psh_glyph_save_points( PSH_Glyph  glyph,
                         FT_Int     dimension )
  {
    FT_UInt     n;
    PSH_Point   point = glyph->points;
    FT_Vector*  vec   = glyph->outline->points;
    char*       tags  = glyph->outline->tags;


    for ( n = 0; n < glyph->num_points; n++ )
    {
      if ( dimension == 0 )
        vec[n].x = point->cur_u;
      else
        vec[n].y = point->cur_u;

      if ( psh_point_is_strong( point ) )
        tags[n] |= (char)( ( dimension == 0 ) ? 32 : 64 );

#ifdef DEBUG_HINTER

      if ( dimension == 0 )
      {
        point->cur_x   = point->cur_u;
        point->flags_x = point->flags2 | point->flags;
      }
      else
      {
        point->cur_y   = point->cur_u;
        point->flags_y = point->flags2 | point->flags;
      }

#endif

      point++;
    }
  }


  static FT_Error
  psh_glyph_init( PSH_Glyph    glyph,
                  FT_Outline*  outline,
                  PS_Hints     ps_hints,
                  PSH_Globals  globals )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* clear all fields */
    FT_MEM_ZERO( glyph, sizeof ( *glyph ) );

    memory = glyph->memory = globals->memory;

    /* allocate and setup points + contours arrays */
    if ( FT_NEW_ARRAY( glyph->points,   outline->n_points   ) ||
         FT_NEW_ARRAY( glyph->contours, outline->n_contours ) )
      goto Exit;

    glyph->num_points   = outline->n_points;
    glyph->num_contours = outline->n_contours;

    {
      FT_UInt      first = 0, next, n;
      PSH_Point    points  = glyph->points;
      PSH_Contour  contour = glyph->contours;


      for ( n = 0; n < glyph->num_contours; n++ )
      {
        FT_Int     count;
        PSH_Point  point;


        next  = outline->contours[n] + 1;
        count = next - first;

        contour->start = points + first;
        contour->count = (FT_UInt)count;

        if ( count > 0 )
        {
          point = points + first;

          point->prev    = points + next - 1;
          point->contour = contour;

          for ( ; count > 1; count-- )
          {
            point[0].next = point + 1;
            point[1].prev = point;
            point++;
            point->contour = contour;
          }
          point->next = points + first;
        }

        contour++;
        first = next;
      }
    }

    {
      PSH_Point   points = glyph->points;
      PSH_Point   point  = points;
      FT_Vector*  vec    = outline->points;
      FT_UInt     n;


      for ( n = 0; n < glyph->num_points; n++, point++ )
      {
        FT_Int  n_prev = (FT_Int)( point->prev - points );
        FT_Int  n_next = (FT_Int)( point->next - points );
        FT_Pos  dxi, dyi, dxo, dyo;


        if ( !( outline->tags[n] & FT_CURVE_TAG_ON ) )
          point->flags = PSH_POINT_OFF;

        dxi = vec[n].x - vec[n_prev].x;
        dyi = vec[n].y - vec[n_prev].y;

        point->dir_in = (FT_Char)psh_compute_dir( dxi, dyi );

        dxo = vec[n_next].x - vec[n].x;
        dyo = vec[n_next].y - vec[n].y;

        point->dir_out = (FT_Char)psh_compute_dir( dxo, dyo );

        /* detect smooth points */
        if ( point->flags & PSH_POINT_OFF )
          point->flags |= PSH_POINT_SMOOTH;
        else if ( point->dir_in  != PSH_DIR_NONE ||
                  point->dir_out != PSH_DIR_NONE )
        {
          if ( point->dir_in == point->dir_out )
            point->flags |= PSH_POINT_SMOOTH;
        }
        else
        {
          FT_Angle  angle_in, angle_out, diff;


          angle_in  = FT_Atan2( dxi, dyi );
          angle_out = FT_Atan2( dxo, dyo );

          diff = angle_in - angle_out;
          if ( diff < 0 )
            diff = -diff;

          if ( diff > FT_ANGLE_PI )
            diff = FT_ANGLE_2PI - diff;

          if ( diff < FT_ANGLE_PI / 16 )
            point->flags |= PSH_POINT_SMOOTH;
        }
      }
    }

    glyph->outline = outline;
    glyph->globals = globals;

#ifdef COMPUTE_INFLEXS
    psh_glyph_load_points( glyph, 0 );
    psh_glyph_compute_inflections( glyph );
#endif /* COMPUTE_INFLEXS */

    /* now deal with hints tables */
    error = psh_hint_table_init( &glyph->hint_tables [0],
                                 &ps_hints->dimension[0].hints,
                                 &ps_hints->dimension[0].masks,
                                 &ps_hints->dimension[0].counters,
                                 memory );
    if ( error )
      goto Exit;

    error = psh_hint_table_init( &glyph->hint_tables [1],
                                 &ps_hints->dimension[1].hints,
                                 &ps_hints->dimension[1].masks,
                                 &ps_hints->dimension[1].counters,
                                 memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  /* compute all extrema in a glyph for a given dimension */
  static void
  psh_glyph_compute_extrema( PSH_Glyph  glyph )
  {
    FT_UInt  n;


    /* first of all, compute all local extrema */
    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH_Point  first = glyph->contours[n].start;
      PSH_Point  point, before, after;


      if ( glyph->contours[n].count == 0 )
        continue;

      point  = first;
      before = point;
      after  = point;

      do
      {
        before = before->prev;
        if ( before == first )
          goto Skip;

      } while ( before->org_u == point->org_u );

      first = point = before->next;

      for (;;)
      {
        after = point;
        do
        {
          after = after->next;
          if ( after == first )
            goto Next;

        } while ( after->org_u == point->org_u );

        if ( before->org_u < point->org_u )
        {
          if ( after->org_u < point->org_u )
          {
            /* local maximum */
            goto Extremum;
          }
        }
        else /* before->org_u > point->org_u */
        {
          if ( after->org_u > point->org_u )
          {
            /* local minimum */
          Extremum:
            do
            {
              psh_point_set_extremum( point );
              point = point->next;

            } while ( point != after );
          }
        }

        before = after->prev;
        point  = after;

      } /* for  */

    Next:
      ;
    }

    /* for each extremum, determine its direction along the */
    /* orthogonal axis                                      */
    for ( n = 0; n < glyph->num_points; n++ )
    {
      PSH_Point  point, before, after;


      point  = &glyph->points[n];
      before = point;
      after  = point;

      if ( psh_point_is_extremum( point ) )
      {
        do
        {
          before = before->prev;
          if ( before == point )
            goto Skip;

        } while ( before->org_v == point->org_v );

        do
        {
          after = after->next;
          if ( after == point )
            goto Skip;

        } while ( after->org_v == point->org_v );
      }

      if ( before->org_v < point->org_v &&
           after->org_v  > point->org_v )
      {
        psh_point_set_positive( point );
      }
      else if ( before->org_v > point->org_v &&
                after->org_v  < point->org_v )
      {
        psh_point_set_negative( point );
      }

    Skip:
      ;
    }
  }


  /* major_dir is the direction for points on the bottom/left of the stem; */
  /* Points on the top/right of the stem will have a direction of          */
  /* -major_dir.                                                           */

  static void
  psh_hint_table_find_strong_point( PSH_Hint_Table  table,
                                    PSH_Point       point,
                                    FT_Int          threshold,
                                    FT_Int          major_dir )
  {
    PSH_Hint*  sort      = table->sort;
    FT_UInt    num_hints = table->num_hints;
    FT_Int     point_dir = 0;


    if ( PSH_DIR_COMPARE( point->dir_in, major_dir ) )
      point_dir = point->dir_in;

    else if ( PSH_DIR_COMPARE( point->dir_out, major_dir ) )
      point_dir = point->dir_out;

    if ( point_dir )
    {
      FT_UInt  flag;


      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH_Hint  hint = sort[0];
        FT_Pos    d;


        if ( point_dir == major_dir )
        {
          flag = PSH_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( FT_ABS( d ) < threshold )
          {
          Is_Strong:
            psh_point_set_strong( point );
            point->flags2 |= flag;
            point->hint    = hint;
            break;
          }
        }
        else if ( point_dir == -major_dir )
        {
          flag = PSH_POINT_EDGE_MAX;
          d    = point->org_u - hint->org_pos - hint->org_len;

          if ( FT_ABS( d ) < threshold )
            goto Is_Strong;
        }
      }
    }

#if 1
    else if ( psh_point_is_extremum( point ) )
    {
      /* treat extrema as special cases for stem edge alignment */
      FT_UInt  min_flag, max_flag;


      if ( major_dir == PSH_DIR_HORIZONTAL )
      {
        min_flag = PSH_POINT_POSITIVE;
        max_flag = PSH_POINT_NEGATIVE;
      }
      else
      {
        min_flag = PSH_POINT_NEGATIVE;
        max_flag = PSH_POINT_POSITIVE;
      }

      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH_Hint  hint = sort[0];
        FT_Pos    d;
        FT_Int    flag;


        if ( point->flags2 & min_flag )
        {
          flag = PSH_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( FT_ABS( d ) < threshold )
          {
          Is_Strong2:
            point->flags2 |= flag;
            point->hint    = hint;
            psh_point_set_strong( point );
            break;
          }
        }
        else if ( point->flags2 & max_flag )
        {
          flag = PSH_POINT_EDGE_MAX;
          d    = point->org_u - hint->org_pos - hint->org_len;

          if ( FT_ABS( d ) < threshold )
            goto Is_Strong2;
        }

        if ( point->org_u >= hint->org_pos                 &&
             point->org_u <= hint->org_pos + hint->org_len )
        {
          point->hint = hint;
        }
      }
    }

#endif /* 1 */
  }


  /* the accepted shift for strong points in fractional pixels */
#define PSH_STRONG_THRESHOLD  32

  /* the maximum shift value in font units */
#define PSH_STRONG_THRESHOLD_MAXIMUM  30


  /* find strong points in a glyph */
  static void
  psh_glyph_find_strong_points( PSH_Glyph  glyph,
                                FT_Int     dimension )
  {
    /* a point is `strong' if it is located on a stem edge and       */
    /* has an `in' or `out' tangent parallel to the hint's direction */

    PSH_Hint_Table  table     = &glyph->hint_tables[dimension];
    PS_Mask         mask      = table->hint_masks->masks;
    FT_UInt         num_masks = table->hint_masks->num_masks;
    FT_UInt         first     = 0;
    FT_Int          major_dir = dimension == 0 ? PSH_DIR_VERTICAL
                                               : PSH_DIR_HORIZONTAL;
    PSH_Dimension   dim       = &glyph->globals->dimension[dimension];
    FT_Fixed        scale     = dim->scale_mult;
    FT_Int          threshold;


    threshold = (FT_Int)FT_DivFix( PSH_STRONG_THRESHOLD, scale );
    if ( threshold > PSH_STRONG_THRESHOLD_MAXIMUM )
      threshold = PSH_STRONG_THRESHOLD_MAXIMUM;

    /* process secondary hints to `selected' points */
    if ( num_masks > 1 && glyph->num_points > 0 )
    {
      first = mask->end_point;
      mask++;
      for ( ; num_masks > 1; num_masks--, mask++ )
      {
        FT_UInt  next;
        FT_Int   count;


        next  = mask->end_point;
        count = next - first;
        if ( count > 0 )
        {
          PSH_Point  point = glyph->points + first;


          psh_hint_table_activate_mask( table, mask );

          for ( ; count > 0; count--, point++ )
            psh_hint_table_find_strong_point( table, point,
                                              threshold, major_dir );
        }
        first = next;
      }
    }

    /* process primary hints for all points */
    if ( num_masks == 1 )
    {
      FT_UInt    count = glyph->num_points;
      PSH_Point  point = glyph->points;


      psh_hint_table_activate_mask( table, table->hint_masks->masks );
      for ( ; count > 0; count--, point++ )
      {
        if ( !psh_point_is_strong( point ) )
          psh_hint_table_find_strong_point( table, point,
                                            threshold, major_dir );
      }
    }

    /* now, certain points may have been attached to a hint and */
    /* not marked as strong; update their flags then            */
    {
      FT_UInt    count = glyph->num_points;
      PSH_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
        if ( point->hint && !psh_point_is_strong( point ) )
          psh_point_set_strong( point );
    }
  }


  /* find points in a glyph which are in a blue zone and have `in' or */
  /* `out' tangents parallel to the horizontal axis                   */
  static void
  psh_glyph_find_blue_points( PSH_Blues  blues,
                              PSH_Glyph  glyph )
  {
    PSH_Blue_Table  table;
    PSH_Blue_Zone   zone;
    FT_UInt         glyph_count = glyph->num_points;
    FT_UInt         blue_count;
    PSH_Point       point = glyph->points;


    for ( ; glyph_count > 0; glyph_count--, point++ )
    {
      FT_Pos  y;


      /* check tangents */
      if ( !PSH_DIR_COMPARE( point->dir_in,  PSH_DIR_HORIZONTAL ) &&
           !PSH_DIR_COMPARE( point->dir_out, PSH_DIR_HORIZONTAL ) )
        continue;

      /* skip strong points */
      if ( psh_point_is_strong( point ) )
        continue;

      y = point->org_u;

      /* look up top zones */
      table      = &blues->normal_top;
      blue_count = table->count;
      zone       = table->zones;

      for ( ; blue_count > 0; blue_count--, zone++ )
      {
        FT_Pos  delta = y - zone->org_bottom;


        if ( delta < -blues->blue_fuzz )
          break;

        if ( y <= zone->org_top + blues->blue_fuzz )
          if ( blues->no_overshoots || delta <= blues->blue_threshold )
          {
            point->cur_u = zone->cur_bottom;
            psh_point_set_strong( point );
            psh_point_set_fitted( point );
          }
      }

      /* look up bottom zones */
      table      = &blues->normal_bottom;
      blue_count = table->count;
      zone       = table->zones + blue_count - 1;

      for ( ; blue_count > 0; blue_count--, zone-- )
      {
        FT_Pos  delta = zone->org_top - y;


        if ( delta < -blues->blue_fuzz )
          break;

        if ( y >= zone->org_bottom - blues->blue_fuzz )
          if ( blues->no_overshoots || delta < blues->blue_threshold )
          {
            point->cur_u = zone->cur_top;
            psh_point_set_strong( point );
            psh_point_set_fitted( point );
          }
      }
    }
  }


  /* interpolate strong points with the help of hinted coordinates */
  static void
  psh_glyph_interpolate_strong_points( PSH_Glyph  glyph,
                                       FT_Int     dimension )
  {
    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;

    FT_UInt        count = glyph->num_points;
    PSH_Point      point = glyph->points;


    for ( ; count > 0; count--, point++ )
    {
      PSH_Hint  hint = point->hint;


      if ( hint )
      {
        FT_Pos  delta;


        if ( psh_point_is_edge_min( point ) )
          point->cur_u = hint->cur_pos;

        else if ( psh_point_is_edge_max( point ) )
          point->cur_u = hint->cur_pos + hint->cur_len;

        else
        {
          delta = point->org_u - hint->org_pos;

          if ( delta <= 0 )
            point->cur_u = hint->cur_pos + FT_MulFix( delta, scale );

          else if ( delta >= hint->org_len )
            point->cur_u = hint->cur_pos + hint->cur_len +
                             FT_MulFix( delta - hint->org_len, scale );

          else if ( hint->org_len > 0 )
            point->cur_u = hint->cur_pos +
                             FT_MulDiv( delta, hint->cur_len,
                                        hint->org_len );
          else
            point->cur_u = hint->cur_pos;
        }
        psh_point_set_fitted( point );
      }
    }
  }


  static void
  psh_glyph_interpolate_normal_points( PSH_Glyph  glyph,
                                       FT_Int     dimension )
  {

#if 1
    /* first technique: a point is strong if it is a local extremum */

    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;

    FT_UInt        count = glyph->num_points;
    PSH_Point      point = glyph->points;


    for ( ; count > 0; count--, point++ )
    {
      if ( psh_point_is_strong( point ) )
        continue;

      /* sometimes, some local extrema are smooth points */
      if ( psh_point_is_smooth( point ) )
      {
        if ( point->dir_in == PSH_DIR_NONE   ||
             point->dir_in != point->dir_out )
          continue;

        if ( !psh_point_is_extremum( point ) &&
             !psh_point_is_inflex( point )   )
          continue;

        point->flags &= ~PSH_POINT_SMOOTH;
      }

      /* find best enclosing point coordinates */
      {
        PSH_Point  before = 0;
        PSH_Point  after  = 0;

        FT_Pos     diff_before = -32000;
        FT_Pos     diff_after  =  32000;
        FT_Pos     u = point->org_u;

        FT_Int     count2 = glyph->num_points;
        PSH_Point  cur    = glyph->points;


        for ( ; count2 > 0; count2--, cur++ )
        {
          if ( psh_point_is_strong( cur ) )
          {
            FT_Pos  diff = cur->org_u - u;


            if ( diff <= 0 )
            {
              if ( diff > diff_before )
              {
                diff_before = diff;
                before      = cur;
              }
            }

            else if ( diff >= 0 )
            {
              if ( diff < diff_after )
              {
                diff_after = diff;
                after      = cur;
              }
            }
          }
        }

        if ( !before )
        {
          if ( !after )
            continue;

          /* we are before the first strong point coordinate; */
          /* simply translate the point                       */
          point->cur_u = after->cur_u +
                           FT_MulFix( point->org_u - after->org_u, scale );
        }
        else if ( !after )
        {
          /* we are after the last strong point coordinate; */
          /* simply translate the point                     */
          point->cur_u = before->cur_u +
                           FT_MulFix( point->org_u - before->org_u, scale );
        }
        else
        {
          if ( diff_before == 0 )
            point->cur_u = before->cur_u;

          else if ( diff_after == 0 )
            point->cur_u = after->cur_u;

          else
            point->cur_u = before->cur_u +
                             FT_MulDiv( u - before->org_u,
                                        after->cur_u - before->cur_u,
                                        after->org_u - before->org_u );
        }

        psh_point_set_fitted( point );
      }
    }

#endif /* 1 */

  }


  /* interpolate other points */
  static void
  psh_glyph_interpolate_other_points( PSH_Glyph  glyph,
                                      FT_Int     dimension )
  {
    PSH_Dimension  dim          = &glyph->globals->dimension[dimension];
    FT_Fixed       scale        = dim->scale_mult;
    FT_Fixed       delta        = dim->scale_delta;
    PSH_Contour    contour      = glyph->contours;
    FT_UInt        num_contours = glyph->num_contours;


    for ( ; num_contours > 0; num_contours--, contour++ )
    {
      PSH_Point  start = contour->start;
      PSH_Point  first, next, point;
      FT_UInt    fit_count;


      /* count the number of strong points in this contour */
      next      = start + contour->count;
      fit_count = 0;
      first     = 0;

      for ( point = start; point < next; point++ )
        if ( psh_point_is_fitted( point ) )
        {
          if ( !first )
            first = point;

          fit_count++;
        }

      /* if there are less than 2 fitted points in the contour, we */
      /* simply scale and eventually translate the contour points  */
      if ( fit_count < 2 )
      {
        if ( fit_count == 1 )
          delta = first->cur_u - FT_MulFix( first->org_u, scale );

        for ( point = start; point < next; point++ )
          if ( point != first )
            point->cur_u = FT_MulFix( point->org_u, scale ) + delta;

        goto Next_Contour;
      }

      /* there are more than 2 strong points in this contour; we */
      /* need to interpolate weak points between them            */
      start = first;
      do
      {
        point = first;

        /* skip consecutive fitted points */
        for (;;)
        {
          next = first->next;
          if ( next == start )
            goto Next_Contour;

          if ( !psh_point_is_fitted( next ) )
            break;

          first = next;
        }

        /* find next fitted point after unfitted one */
        for (;;)
        {
          next = next->next;
          if ( psh_point_is_fitted( next ) )
            break;
        }

        /* now interpolate between them */
        {
          FT_Pos    org_a, org_ab, cur_a, cur_ab;
          FT_Pos    org_c, org_ac, cur_c;
          FT_Fixed  scale_ab;


          if ( first->org_u <= next->org_u )
          {
            org_a  = first->org_u;
            cur_a  = first->cur_u;
            org_ab = next->org_u - org_a;
            cur_ab = next->cur_u - cur_a;
          }
          else
          {
            org_a  = next->org_u;
            cur_a  = next->cur_u;
            org_ab = first->org_u - org_a;
            cur_ab = first->cur_u - cur_a;
          }

          scale_ab = 0x10000L;
          if ( org_ab > 0 )
            scale_ab = FT_DivFix( cur_ab, org_ab );

          point = first->next;
          do
          {
            org_c  = point->org_u;
            org_ac = org_c - org_a;

            if ( org_ac <= 0 )
            {
              /* on the left of the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale );
            }
            else if ( org_ac >= org_ab )
            {
              /* on the right on the interpolation zone */
              cur_c = cur_a + cur_ab + FT_MulFix( org_ac - org_ab, scale );
            }
            else
            {
              /* within the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale_ab );
            }

            point->cur_u = cur_c;

            point = point->next;

          } while ( point != next );
        }

        /* keep going until all points in the contours have been processed */
        first = next;

      } while ( first != start );

    Next_Contour:
      ;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     HIGH-LEVEL INTERFACE                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_Error
  ps_hints_apply( PS_Hints        ps_hints,
                  FT_Outline*     outline,
                  PSH_Globals     globals,
                  FT_Render_Mode  hint_mode )
  {
    PSH_GlyphRec  glyphrec;
    PSH_Glyph     glyph = &glyphrec;
    FT_Error      error;
#ifdef DEBUG_HINTER
    FT_Memory     memory;
#endif
    FT_Int        dimension;


    /* something to do? */
    if ( outline->n_points == 0 || outline->n_contours == 0 )
      return PSH_Err_Ok;

#ifdef DEBUG_HINTER

    memory = globals->memory;

    if ( ps_debug_glyph )
    {
      psh_glyph_done( ps_debug_glyph );
      FT_FREE( ps_debug_glyph );
    }

    if ( FT_NEW( glyph ) )
      return error;

    ps_debug_glyph = glyph;

#endif /* DEBUG_HINTER */

    error = psh_glyph_init( glyph, outline, ps_hints, globals );
    if ( error )
      goto Exit;

    /* try to optimize the y_scale so that the top of non-capital letters
     * is aligned on a pixel boundary whenever possible
     */
    {
      PSH_Dimension  dim_x = &glyph->globals->dimension[0];
      PSH_Dimension  dim_y = &glyph->globals->dimension[1];

      FT_Fixed x_scale = dim_x->scale_mult;
      FT_Fixed y_scale = dim_y->scale_mult;

      FT_Fixed scaled;
      FT_Fixed fitted;


      scaled = FT_MulFix( globals->blues.normal_top.zones->org_ref, y_scale );
      fitted = FT_PIX_ROUND( scaled );

      if ( fitted != 0 && scaled != fitted )
      {
        y_scale = FT_MulDiv( y_scale, fitted, scaled );

        if ( fitted < scaled )
          x_scale -= x_scale / 50;

        psh_globals_set_scale( glyph->globals, x_scale, y_scale, 0, 0 );
      }
    }

    glyph->do_horz_hints = 1;
    glyph->do_vert_hints = 1;

    glyph->do_horz_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO ||
                                       hint_mode == FT_RENDER_MODE_LCD  );

    glyph->do_vert_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO  ||
                                       hint_mode == FT_RENDER_MODE_LCD_V );

    glyph->do_stem_adjust   = FT_BOOL( hint_mode != FT_RENDER_MODE_LIGHT );

    for ( dimension = 0; dimension < 2; dimension++ )
    {
      /* load outline coordinates into glyph */
      psh_glyph_load_points( glyph, dimension );

      /* compute local extrema */
      psh_glyph_compute_extrema( glyph );

      /* compute aligned stem/hints positions */
      psh_hint_table_align_hints( &glyph->hint_tables[dimension],
                                  glyph->globals,
                                  dimension,
                                  glyph );

      /* find strong points, align them, then interpolate others */
      psh_glyph_find_strong_points( glyph, dimension );
      if ( dimension == 1 )
        psh_glyph_find_blue_points( &globals->blues, glyph );
      psh_glyph_interpolate_strong_points( glyph, dimension );
      psh_glyph_interpolate_normal_points( glyph, dimension );
      psh_glyph_interpolate_other_points( glyph, dimension );

      /* save hinted coordinates back to outline */
      psh_glyph_save_points( glyph, dimension );
    }

  Exit:

#ifndef DEBUG_HINTER
    psh_glyph_done( glyph );
#endif

    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  pshmod.c                                                               */
/*                                                                         */
/*    FreeType PostScript hinter module implementation (body).             */
/*                                                                         */
/*  Copyright 2001, 2002 by                                                */
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
#include FT_INTERNAL_OBJECTS_H
#include "pshrec.h"
#include "pshalgo.h"


  /* the Postscript Hinter module structure */
  typedef struct  PS_Hinter_Module_Rec_
  {
    FT_ModuleRec          root;
    PS_HintsRec           ps_hints;

    PSH_Globals_FuncsRec  globals_funcs;
    T1_Hints_FuncsRec     t1_funcs;
    T2_Hints_FuncsRec     t2_funcs;

  } PS_Hinter_ModuleRec, *PS_Hinter_Module;


  /* finalize module */
  FT_CALLBACK_DEF( void )
  ps_hinter_done( PS_Hinter_Module  module )
  {
    module->t1_funcs.hints = NULL;
    module->t2_funcs.hints = NULL;

    ps_hints_done( &module->ps_hints );
  }


  /* initialize module, create hints recorder and the interface */
  FT_CALLBACK_DEF( FT_Error )
  ps_hinter_init( PS_Hinter_Module  module )
  {
    FT_Memory  memory = module->root.memory;


    ps_hints_init( &module->ps_hints, memory );

    psh_globals_funcs_init( &module->globals_funcs );

    t1_hints_funcs_init( &module->t1_funcs );
    module->t1_funcs.hints = (T1_Hints)&module->ps_hints;

    t2_hints_funcs_init( &module->t2_funcs );
    module->t2_funcs.hints = (T2_Hints)&module->ps_hints;

    return 0;
  }


  /* returns global hints interface */
  FT_CALLBACK_DEF( PSH_Globals_Funcs )
  pshinter_get_globals_funcs( FT_Module  module )
  {
    return &((PS_Hinter_Module)module)->globals_funcs;
  }


  /* return Type 1 hints interface */
  FT_CALLBACK_DEF( T1_Hints_Funcs )
  pshinter_get_t1_funcs( FT_Module  module )
  {
    return &((PS_Hinter_Module)module)->t1_funcs;
  }


  /* return Type 2 hints interface */
  FT_CALLBACK_DEF( T2_Hints_Funcs )
  pshinter_get_t2_funcs( FT_Module  module )
  {
    return &((PS_Hinter_Module)module)->t2_funcs;
  }


  static
  const PSHinter_Interface  pshinter_interface =
  {
    pshinter_get_globals_funcs,
    pshinter_get_t1_funcs,
    pshinter_get_t2_funcs
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  pshinter_module_class =
  {
    0,
    sizeof ( PS_Hinter_ModuleRec ),
    "pshinter",
    0x10000L,
    0x20000L,

    &pshinter_interface,            /* module-specific interface */

    (FT_Module_Constructor)ps_hinter_init,
    (FT_Module_Destructor) ps_hinter_done,
    (FT_Module_Requester)  0        /* no additional interface for now */
  };


/* END */



/* END */

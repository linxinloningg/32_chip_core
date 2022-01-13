/***************************************************************************/
/*                                                                         */
/*  autofit.c                                                              */
/*                                                                         */
/*    Auto-fitter module (body).                                           */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
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
/*  afangles.c                                                             */
/*                                                                         */
/*    Routines used to compute vector angles with limited accuracy         */
/*    and very high speed.  It also contains sorting routines (body).      */
/*                                                                         */
/*  Copyright 2003, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aftypes.h"


#if 1

  /* the following table has been automatically generated with */
  /* the `mather.py' Python script                             */

#define AF_ATAN_BITS  8

  static const FT_Byte  af_arctan[1L << AF_ATAN_BITS] =
  {
     0,  0,  1,  1,  1,  2,  2,  2,
     3,  3,  3,  3,  4,  4,  4,  5,
     5,  5,  6,  6,  6,  7,  7,  7,
     8,  8,  8,  9,  9,  9, 10, 10,
    10, 10, 11, 11, 11, 12, 12, 12,
    13, 13, 13, 14, 14, 14, 14, 15,
    15, 15, 16, 16, 16, 17, 17, 17,
    18, 18, 18, 18, 19, 19, 19, 20,
    20, 20, 21, 21, 21, 21, 22, 22,
    22, 23, 23, 23, 24, 24, 24, 24,
    25, 25, 25, 26, 26, 26, 26, 27,
    27, 27, 28, 28, 28, 28, 29, 29,
    29, 30, 30, 30, 30, 31, 31, 31,
    31, 32, 32, 32, 33, 33, 33, 33,
    34, 34, 34, 34, 35, 35, 35, 35,
    36, 36, 36, 36, 37, 37, 37, 38,
    38, 38, 38, 39, 39, 39, 39, 40,
    40, 40, 40, 41, 41, 41, 41, 42,
    42, 42, 42, 42, 43, 43, 43, 43,
    44, 44, 44, 44, 45, 45, 45, 45,
    46, 46, 46, 46, 46, 47, 47, 47,
    47, 48, 48, 48, 48, 48, 49, 49,
    49, 49, 50, 50, 50, 50, 50, 51,
    51, 51, 51, 51, 52, 52, 52, 52,
    52, 53, 53, 53, 53, 53, 54, 54,
    54, 54, 54, 55, 55, 55, 55, 55,
    56, 56, 56, 56, 56, 57, 57, 57,
    57, 57, 57, 58, 58, 58, 58, 58,
    59, 59, 59, 59, 59, 59, 60, 60,
    60, 60, 60, 61, 61, 61, 61, 61,
    61, 62, 62, 62, 62, 62, 62, 63,
    63, 63, 63, 63, 63, 64, 64, 64
  };


  FT_LOCAL_DEF( AF_Angle )
  af_angle_atan( FT_Fixed  dx,
                 FT_Fixed  dy )
  {
    AF_Angle  angle;


    /* check trivial cases */
    if ( dy == 0 )
    {
      angle = 0;
      if ( dx < 0 )
        angle = AF_ANGLE_PI;
      return angle;
    }
    else if ( dx == 0 )
    {
      angle = AF_ANGLE_PI2;
      if ( dy < 0 )
        angle = -AF_ANGLE_PI2;
      return angle;
    }

    angle = 0;
    if ( dx < 0 )
    {
      dx = -dx;
      dy = -dy;
      angle = AF_ANGLE_PI;
    }

    if ( dy < 0 )
    {
      FT_Pos  tmp;


      tmp = dx;
      dx  = -dy;
      dy  = tmp;
      angle -= AF_ANGLE_PI2;
    }

    if ( dx == 0 && dy == 0 )
      return 0;

    if ( dx == dy )
      angle += AF_ANGLE_PI4;
    else if ( dx > dy )
      angle += af_arctan[FT_DivFix( dy, dx ) >> ( 16 - AF_ATAN_BITS )];
    else
      angle += AF_ANGLE_PI2 -
               af_arctan[FT_DivFix( dx, dy ) >> ( 16 - AF_ATAN_BITS )];

    if ( angle > AF_ANGLE_PI )
      angle -= AF_ANGLE_2PI;

    return angle;
  }


#else /* 0 */

/*
 * a python script used to generate the following table
 *

import sys, math

units = 256
scale = units/math.pi
comma = ""

print ""
print "table of arctan( 1/2^n ) for PI = " + repr( units / 65536.0 ) + " units"

r = [-1] + range( 32 )

for n in r:
    if n >= 0:
        x = 1.0 / ( 2.0 ** n )   # tangent value
    else:
        x = 2.0 ** ( -n )

    angle  = math.atan( x )      # arctangent
    angle2 = angle * scale       # arctangent in FT_Angle units

    # determine which integer value for angle gives the best tangent
    lo  = int( angle2 )
    hi  = lo + 1
    tlo = math.tan( lo / scale )
    thi = math.tan( hi / scale )

    errlo = abs( tlo - x )
    errhi = abs( thi - x )

    angle2 = hi
    if errlo < errhi:
        angle2 = lo

    if angle2 <= 0:
        break

    sys.stdout.write( comma + repr( int( angle2 ) ) )
    comma = ", "

*
* end of python script
*/


  /* this table was generated for AF_ANGLE_PI = 256 */
#define AF_ANGLE_MAX_ITERS  8
#define AF_TRIG_MAX_ITERS   8

  static const FT_Fixed
  af_angle_arctan_table[9] =
  {
    90, 64, 38, 20, 10, 5, 3, 1, 1
  };


  static FT_Int
  af_angle_prenorm( FT_Vector*  vec )
  {
    FT_Fixed  x, y, z;
    FT_Int    shift;


    x = vec->x;
    y = vec->y;

    z     = ( ( x >= 0 ) ? x : - x ) | ( (y >= 0) ? y : -y );
    shift = 0;

    if ( z < ( 1L << 27 ) )
    {
      do
      {
        shift++;
        z <<= 1;
      } while ( z < ( 1L << 27 ) );

      vec->x = x << shift;
      vec->y = y << shift;
    }
    else if ( z > ( 1L << 28 ) )
    {
      do
      {
        shift++;
        z >>= 1;
      } while ( z > ( 1L << 28 ) );

      vec->x = x >> shift;
      vec->y = y >> shift;
      shift  = -shift;
    }
    return shift;
  }


  static void
  af_angle_pseudo_polarize( FT_Vector*  vec )
  {
    FT_Fixed         theta;
    FT_Fixed         yi, i;
    FT_Fixed         x, y;
    const FT_Fixed  *arctanptr;


    x = vec->x;
    y = vec->y;

    /* Get the vector into the right half plane */
    theta = 0;
    if ( x < 0 )
    {
      x = -x;
      y = -y;
      theta = AF_ANGLE_PI;
    }

    if ( y > 0 )
      theta = -theta;

    arctanptr = af_angle_arctan_table;

    if ( y < 0 )
    {
      /* Rotate positive */
      yi     = y + ( x << 1 );
      x      = x - ( y << 1 );
      y      = yi;
      theta -= *arctanptr++;  /* Subtract angle */
    }
    else
    {
      /* Rotate negative */
      yi     = y - ( x << 1 );
      x      = x + ( y << 1 );
      y      = yi;
      theta += *arctanptr++;  /* Add angle */
    }

    i = 0;
    do
    {
      if ( y < 0 )
      {
        /* Rotate positive */
        yi     = y + ( x >> i );
        x      = x - ( y >> i );
        y      = yi;
        theta -= *arctanptr++;
      }
      else
      {
        /* Rotate negative */
        yi     = y - ( x >> i );
        x      = x + ( y >> i );
        y      = yi;
        theta += *arctanptr++;
      }
    } while ( ++i < AF_TRIG_MAX_ITERS );

#if 0
    /* round theta */
    if ( theta >= 0 )
      theta =  FT_PAD_ROUND( theta, 2 );
    else
      theta = -FT_PAD_ROUND( -theta, 2 );
#endif

    vec->x = x;
    vec->y = theta;
  }


  /* cf. documentation in fttrigon.h */

  FT_LOCAL_DEF( AF_Angle )
  af_angle_atan( FT_Fixed  dx,
                 FT_Fixed  dy )
  {
    FT_Vector  v;


    if ( dx == 0 && dy == 0 )
      return 0;

    v.x = dx;
    v.y = dy;
    af_angle_prenorm( &v );
    af_angle_pseudo_polarize( &v );

    return v.y;
  }


  FT_LOCAL_DEF( AF_Angle )
  af_angle_diff( AF_Angle  angle1,
                 AF_Angle  angle2 )
  {
    AF_Angle  delta = angle2 - angle1;


    delta %= AF_ANGLE_2PI;
    if ( delta < 0 )
      delta += AF_ANGLE_2PI;

    if ( delta > AF_ANGLE_PI )
      delta -= AF_ANGLE_2PI;

    return delta;
  }

#endif /* 0 */


  FT_LOCAL_DEF( void )
  af_sort_pos( FT_UInt  count,
               FT_Pos*  table )
  {
    FT_UInt  i, j;
    FT_Pos   swap;


    for ( i = 1; i < count; i++ )
    {
      for ( j = i; j > 0; j-- )
      {
        if ( table[j] > table[j - 1] )
          break;

        swap         = table[j];
        table[j]     = table[j - 1];
        table[j - 1] = swap;
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_sort_widths( FT_UInt   count,
                  AF_Width  table )
  {
    FT_UInt      i, j;
    AF_WidthRec  swap;


    for ( i = 1; i < count; i++ )
    {
      for ( j = i; j > 0; j-- )
      {
        if ( table[j].org > table[j - 1].org )
          break;

        swap         = table[j];
        table[j]     = table[j - 1];
        table[j - 1] = swap;
      }
    }
  }


#ifdef TEST

#include <stdio.h>
#include <math.h>

int main( void )
{
  int  angle;
  int  dist;


  for ( dist = 100; dist < 1000; dist++ )
  {
    for ( angle = AF_ANGLE_PI; angle < AF_ANGLE_2PI * 4; angle++ )
    {
      double  a = ( angle * 3.1415926535 ) / ( 1.0 * AF_ANGLE_PI );
      int     dx, dy, angle1, angle2, delta;


      dx = dist * cos( a );
      dy = dist * sin( a );

      angle1 = ( ( atan2( dy, dx ) * AF_ANGLE_PI ) / 3.1415926535 );
      angle2 = af_angle_atan( dx, dy );
      delta  = ( angle2 - angle1 ) % AF_ANGLE_2PI;
      if ( delta < 0 )
        delta = -delta;

      if ( delta >= 2 )
      {
        printf( "dist:%4d angle:%4d => (%4d,%4d) angle1:%4d angle2:%4d\n",
                dist, angle, dx, dy, angle1, angle2 );
      }
    }
  }
  return 0;
}

#endif /* TEST */


/* END */

/***************************************************************************/
/*                                                                         */
/*  afglobal.c                                                             */
/*                                                                         */
/*    Auto-fitter routines to compute global hinting values (body).        */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afglobal.h"
#include "afdummy.h"
#include "aflatin.h"
#include "afcjk.h"
#include "aferrors.h"


  /* populate this list when you add new scripts */
  static AF_ScriptClass const  af_script_classes[] =
  {
    &af_dummy_script_class,
    &af_latin_script_class,
    &af_cjk_script_class,

    NULL  /* do not remove */
  };

  /* index of default script in `af_script_classes' */
#define AF_SCRIPT_LIST_DEFAULT  1
  /* indicates an uncovered glyph                   */
#define AF_SCRIPT_LIST_NONE   255


  /*
   *  Note that glyph_scripts[] is used to map each glyph into
   *  an index into the `af_script_classes' array.
   *
   */
  typedef struct  AF_FaceGlobalsRec_
  {
    FT_Face           face;
    FT_UInt           glyph_count;    /* same as face->num_glyphs */
    FT_Byte*          glyph_scripts;

    AF_ScriptMetrics  metrics[AF_SCRIPT_MAX];

  } AF_FaceGlobalsRec;


  /* Compute the script index of each glyph within a given face. */

  static FT_Error
  af_face_globals_compute_script_coverage( AF_FaceGlobals  globals )
  {
    FT_Error    error       = AF_Err_Ok;
    FT_Face     face        = globals->face;
    FT_CharMap  old_charmap = face->charmap;
    FT_Byte*    gscripts    = globals->glyph_scripts;
    FT_UInt     ss;


    /* the value 255 means `uncovered glyph' */
    FT_MEM_SET( globals->glyph_scripts,
                AF_SCRIPT_LIST_NONE,
                globals->glyph_count );

    error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
    if ( error )
    {
     /*
      *  Ignore this error; we simply use Latin as the standard
      *  script.  XXX: Shouldn't we rather disable hinting?
      */
      error = AF_Err_Ok;
      goto Exit;
    }

    /* scan each script in a Unicode charmap */
    for ( ss = 0; af_script_classes[ss]; ss++ )
    {
      AF_ScriptClass      clazz = af_script_classes[ss];
      AF_Script_UniRange  range;


      if ( clazz->script_uni_ranges == NULL )
        continue;

      /*
       *  Scan all unicode points in the range and set the corresponding
       *  glyph script index.
       */
      for ( range = clazz->script_uni_ranges; range->first != 0; range++ )
      {
        FT_ULong  charcode = range->first;
        FT_UInt   gindex;


        gindex = FT_Get_Char_Index( face, charcode );

        if ( gindex != 0                             &&
             gindex < globals->glyph_count           &&
             gscripts[gindex] == AF_SCRIPT_LIST_NONE )
        {
          gscripts[gindex] = (FT_Byte)ss;
        }

        for (;;)
        {
          charcode = FT_Get_Next_Char( face, charcode, &gindex );

          if ( gindex == 0 || charcode > range->last )
            break;

          if ( gindex < globals->glyph_count           &&
               gscripts[gindex] == AF_SCRIPT_LIST_NONE )
          {
            gscripts[gindex] = (FT_Byte)ss;
          }
        }
      }
    }

  Exit:
    /*
     *  By default, all uncovered glyphs are set to the latin script.
     *  XXX: Shouldnt' we disable hinting or do something similar?
     */
    {
      FT_UInt  nn;


      for ( nn = 0; nn < globals->glyph_count; nn++ )
      {
        if ( gscripts[nn] == AF_SCRIPT_LIST_NONE )
          gscripts[nn] = AF_SCRIPT_LIST_DEFAULT;
      }
    }

    FT_Set_Charmap( face, old_charmap );
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals )
  {
    FT_Error        error;
    FT_Memory       memory;
    AF_FaceGlobals  globals;


    memory = face->memory;

    if ( !FT_ALLOC( globals, sizeof ( *globals ) +
                             face->num_glyphs * sizeof ( FT_Byte ) ) )
    {
      globals->face          = face;
      globals->glyph_count   = face->num_glyphs;
      globals->glyph_scripts = (FT_Byte*)( globals + 1 );

      error = af_face_globals_compute_script_coverage( globals );
      if ( error )
      {
        af_face_globals_free( globals );
        globals = NULL;
      }
    }

    *aglobals = globals;
    return error;
  }


  FT_LOCAL_DEF( void )
  af_face_globals_free( AF_FaceGlobals  globals )
  {
    if ( globals )
    {
      FT_Memory  memory = globals->face->memory;
      FT_UInt    nn;


      for ( nn = 0; nn < AF_SCRIPT_MAX; nn++ )
      {
        if ( globals->metrics[nn] )
        {
          AF_ScriptClass  clazz = af_script_classes[nn];


          FT_ASSERT( globals->metrics[nn]->clazz == clazz );

          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( globals->metrics[nn] );

          FT_FREE( globals->metrics[nn] );
        }
      }

      globals->glyph_count   = 0;
      globals->glyph_scripts = NULL;  /* no need to free this one! */
      globals->face          = NULL;

      FT_FREE( globals );
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               AF_ScriptMetrics  *ametrics )
  {
    AF_ScriptMetrics  metrics = NULL;
    FT_UInt           gidx;
    AF_ScriptClass    clazz;
    FT_Error          error = AF_Err_Ok;


    if ( gindex >= globals->glyph_count )
    {
      error = AF_Err_Invalid_Argument;
      goto Exit;
    }

    gidx    = globals->glyph_scripts[gindex];
    clazz   = af_script_classes[gidx];
    metrics = globals->metrics[clazz->script];
    if ( metrics == NULL )
    {
      /* create the global metrics object when needed */
      FT_Memory  memory = globals->face->memory;


      if ( FT_ALLOC( metrics, clazz->script_metrics_size ) )
        goto Exit;

      metrics->clazz = clazz;

      if ( clazz->script_metrics_init )
      {
        error = clazz->script_metrics_init( metrics, globals->face );
        if ( error )
        {
          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( metrics );

          FT_FREE( metrics );
          goto Exit;
        }
      }

      globals->metrics[clazz->script] = metrics;
    }

  Exit:
    *ametrics = metrics;

    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  afhints.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines (body).                                 */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afhints.h"
#include "aferrors.h"


  FT_LOCAL_DEF( FT_Error )
  af_axis_hints_new_segment( AF_AxisHints  axis,
                             FT_Memory     memory,
                             AF_Segment   *asegment )
  {
    FT_Error    error   = AF_Err_Ok;
    AF_Segment  segment = NULL;


    if ( axis->num_segments >= axis->max_segments )
    {
      FT_Int  old_max = axis->max_segments;
      FT_Int  new_max = old_max;
      FT_Int  big_max = FT_INT_MAX / sizeof ( *segment );


      if ( old_max >= big_max )
      {
        error = AF_Err_Out_Of_Memory;
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->segments, old_max, new_max ) )
        goto Exit;

      axis->max_segments = new_max;
    }

    segment = axis->segments + axis->num_segments++;
    FT_ZERO( segment );

  Exit:
    *asegment = segment;
    return error;
  }


  FT_LOCAL( FT_Error )
  af_axis_hints_new_edge( AF_AxisHints  axis,
                          FT_Int        fpos,
                          FT_Memory     memory,
                          AF_Edge      *aedge )
  {
    FT_Error  error = AF_Err_Ok;
    AF_Edge   edge  = NULL;
    AF_Edge   edges;


    if ( axis->num_edges >= axis->max_edges )
    {
      FT_Int  old_max = axis->max_edges;
      FT_Int  new_max = old_max;
      FT_Int  big_max = FT_INT_MAX / sizeof ( *edge );


      if ( old_max >= big_max )
      {
        error = AF_Err_Out_Of_Memory;
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->edges, old_max, new_max ) )
        goto Exit;

      axis->max_edges = new_max;
    }

    edges = axis->edges;
    edge  = edges + axis->num_edges;

    while ( edge > edges && edge[-1].fpos > fpos )
    {
      edge[0] = edge[-1];
      edge--;
    }

    axis->num_edges++;

    FT_ZERO( edge );
    edge->fpos = (FT_Short)fpos;

  Exit:
    *aedge = edge;
    return error;
  }


#ifdef AF_DEBUG

#include <stdio.h>

  static const char*
  af_dir_str( AF_Direction  dir )
  {
    const char*  result;


    switch ( dir )
    {
    case AF_DIR_UP:
      result = "up";
      break;
    case AF_DIR_DOWN:
      result = "down";
      break;
    case AF_DIR_LEFT:
      result = "left";
      break;
    case AF_DIR_RIGHT:
      result = "right";
      break;
    default:
      result = "none";
    }

    return result;
  }


#define AF_INDEX_NUM( ptr, base )  ( (ptr) ? ( (ptr) - (base) ) : -1 )


  void
  af_glyph_hints_dump_points( AF_GlyphHints  hints )
  {
    AF_Point  points = hints->points;
    AF_Point  limit  = points + hints->num_points;
    AF_Point  point;


    printf( "Table of points:\n" );
    printf(   "  [ index |  xorg |  yorg |  xscale |  yscale "
              "|  xfit  |  yfit  |  flags ]\n" );

    for ( point = points; point < limit; point++ )
    {
      printf( "  [ %5d | %5d | %5d | %-5.2f | %-5.2f "
              "| %-5.2f | %-5.2f | %c%c%c%c%c%c ]\n",
              point - points,
              point->fx,
              point->fy,
              point->ox/64.0,
              point->oy/64.0,
              point->x/64.0,
              point->y/64.0,
              ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) ? 'w' : ' ',
              ( point->flags & AF_FLAG_INFLECTION )         ? 'i' : ' ',
              ( point->flags & AF_FLAG_EXTREMA_X )          ? '<' : ' ',
              ( point->flags & AF_FLAG_EXTREMA_Y )          ? 'v' : ' ',
              ( point->flags & AF_FLAG_ROUND_X )            ? '(' : ' ',
              ( point->flags & AF_FLAG_ROUND_Y )            ? 'u' : ' ');
    }
    printf( "\n" );
  }


  /* A function to dump the array of linked segments. */
  void
  af_glyph_hints_dump_segments( AF_GlyphHints  hints )
  {
    AF_Point  points = hints->points;
    FT_Int    dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis     = &hints->axis[dimension];
      AF_Segment    segments = axis->segments;
      AF_Segment    limit    = segments + axis->num_segments;
      AF_Segment    seg;


      printf ( "Table of %s segments:\n",
               dimension == AF_DIMENSION_HORZ ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link | serif |"
               " numl | first | start ]\n" );

      for ( seg = segments; seg < limit; seg++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d | %5d | %4d | %5d | %5d ]\n",
                 seg - segments,
                 (int)seg->pos,
                 af_dir_str( seg->dir ),
                 AF_INDEX_NUM( seg->link, segments ),
                 AF_INDEX_NUM( seg->serif, segments ),
                 (int)seg->num_linked,
                 seg->first - points,
                 seg->last - points );
      }
      printf( "\n" );
    }
  }


  void
  af_glyph_hints_dump_edges( AF_GlyphHints  hints )
  {
    FT_Int  dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis  = &hints->axis[dimension];
      AF_Edge       edges = axis->edges;
      AF_Edge       limit = edges + axis->num_edges;
      AF_Edge       edge;


      /*
       *  note: AF_DIMENSION_HORZ corresponds to _vertical_ edges
       *        since they have constant a X coordinate.
       */
      printf ( "Table of %s edges:\n",
               dimension == AF_DIMENSION_HORZ ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link |"
               " serif | blue | opos  |  pos  ]\n" );

      for ( edge = edges; edge < limit; edge++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d |"
                 " %5d |   %c  | %5.2f | %5.2f ]\n",
                 edge - edges,
                 (int)edge->fpos,
                 af_dir_str( edge->dir ),
                 AF_INDEX_NUM( edge->link, edges ),
                 AF_INDEX_NUM( edge->serif, edges ),
                 edge->blue_edge ? 'y' : 'n',
                 edge->opos / 64.0,
                 edge->pos / 64.0 );
      }
      printf( "\n" );
    }
  }

#endif /* AF_DEBUG */



  /* compute the direction value of a given vector */
  FT_LOCAL_DEF( AF_Direction )
  af_direction_compute( FT_Pos  dx,
                        FT_Pos  dy )
  {
#if 1
    AF_Direction  dir = AF_DIR_NONE;


    /* atan(1/12) == 4.7 degrees */

    if ( dx < 0 )
    {
      if ( dy < 0 )
      {
        if ( -dx * 12 < -dy )
          dir = AF_DIR_DOWN;

        else if ( -dy * 12 < -dx )
          dir = AF_DIR_LEFT;
      }
      else /* dy >= 0 */
      {
        if ( -dx * 12 < dy )
          dir = AF_DIR_UP;

        else if ( dy * 12 < -dx )
          dir = AF_DIR_LEFT;
      }
    }
    else /* dx >= 0 */
    {
      if ( dy < 0 )
      {
        if ( dx * 12 < -dy )
          dir = AF_DIR_DOWN;

        else if ( -dy * 12 < dx )
          dir = AF_DIR_RIGHT;
      }
      else  /* dy >= 0 */
      {
        if ( dx * 12 < dy )
          dir = AF_DIR_UP;

        else if ( dy * 12 < dx )
          dir = AF_DIR_RIGHT;
      }
    }

    return dir;

#else /* 0 */

    AF_Direction  dir;
    FT_Pos        ax = FT_ABS( dx );
    FT_Pos        ay = FT_ABS( dy );


    dir = AF_DIR_NONE;

    /* atan(1/12) == 4.7 degrees */

    /* test for vertical direction */
    if ( ax * 12 < ay )
    {
      dir = dy > 0 ? AF_DIR_UP : AF_DIR_DOWN;
    }
    /* test for horizontal direction */
    else if ( ay * 12 < ax )
    {
      dir = dx > 0 ? AF_DIR_RIGHT : AF_DIR_LEFT;
    }

    return dir;

#endif /* 0 */

  }


  /* compute all inflex points in a given glyph */
  static void
  af_glyph_hints_compute_inflections( AF_GlyphHints  hints )
  {
    AF_Point*  contour       = hints->contours;
    AF_Point*  contour_limit = contour + hints->num_contours;


    /* do each contour separately */
    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  point = contour[0];
      AF_Point  first = point;
      AF_Point  start = point;
      AF_Point  end   = point;
      AF_Point  before;
      AF_Point  after;
      AF_Angle  angle_in, angle_seg, angle_out;
      AF_Angle  diff_in, diff_out;
      FT_Int    finished = 0;


      /* compute first segment in contour */
      first = point;

      start = end = first;
      do
      {
        end = end->next;
        if ( end == first )
          goto Skip;

      } while ( end->fx == first->fx && end->fy == first->fy );

      angle_seg = af_angle_atan( end->fx - start->fx,
                                 end->fy - start->fy );

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

        } while ( before->fx == start->fx && before->fy == start->fy );

        angle_in = af_angle_atan( start->fx - before->fx,
                                  start->fy - before->fy );

      } while ( angle_in == angle_seg );

      first = start;

      AF_ANGLE_DIFF( diff_in, angle_in, angle_seg );

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

          } while ( end->fx == after->fx && end->fy == after->fy );

          angle_out = af_angle_atan( after->fx - end->fx,
                                     after->fy - end->fy );

        } while ( angle_out == angle_seg );

        AF_ANGLE_DIFF( diff_out, angle_seg, angle_out );

        if ( ( diff_in ^ diff_out ) < 0 )
        {
          /* diff_in and diff_out have different signs, we have */
          /* inflection points here...                          */
          do
          {
            start->flags |= AF_FLAG_INFLECTION;
            start = start->next;

          } while ( start != end );

          start->flags |= AF_FLAG_INFLECTION;
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


  FT_LOCAL_DEF( void )
  af_glyph_hints_init( AF_GlyphHints  hints,
                       FT_Memory      memory )
  {
    FT_ZERO( hints );
    hints->memory = memory;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_done( AF_GlyphHints  hints )
  {
    if ( hints && hints->memory )
    {
      FT_Memory  memory = hints->memory;
      int        dim;


      /*
       *  note that we don't need to free the segment and edge
       *  buffers, since they are really within the hints->points array
       */
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_AxisHints  axis = &hints->axis[dim];


        axis->num_segments = 0;
        axis->max_segments = 0;
        FT_FREE( axis->segments );

        axis->num_edges    = 0;
        axis->max_edges    = 0;
        FT_FREE( axis->edges );
      }

      FT_FREE( hints->contours );
      hints->max_contours = 0;
      hints->num_contours = 0;

      FT_FREE( hints->points );
      hints->num_points = 0;
      hints->max_points = 0;

      hints->memory = NULL;
    }
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_rescale( AF_GlyphHints     hints,
                          AF_ScriptMetrics  metrics )
  {
    hints->metrics      = metrics;
    hints->scaler_flags = metrics->scaler.flags;
  }


  FT_LOCAL_DEF( FT_Error )
  af_glyph_hints_reload( AF_GlyphHints  hints,
                         FT_Outline*    outline )
  {
    FT_Error   error   = AF_Err_Ok;
    AF_Point   points;
    FT_UInt    old_max, new_max;
    FT_Fixed   x_scale = hints->x_scale;
    FT_Fixed   y_scale = hints->y_scale;
    FT_Pos     x_delta = hints->x_delta;
    FT_Pos     y_delta = hints->y_delta;
    FT_Memory  memory  = hints->memory;


    hints->num_points   = 0;
    hints->num_contours = 0;

    hints->axis[0].num_segments = 0;
    hints->axis[0].num_edges    = 0;
    hints->axis[1].num_segments = 0;
    hints->axis[1].num_edges    = 0;

    /* first of all, reallocate the contours array when necessary */
    new_max = (FT_UInt)outline->n_contours;
    old_max = hints->max_contours;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 3 ) & ~3;

      if ( FT_RENEW_ARRAY( hints->contours, old_max, new_max ) )
        goto Exit;

      hints->max_contours = new_max;
    }

    /*
     *  then reallocate the points arrays if necessary --
     *  note that we reserve two additional point positions, used to
     *  hint metrics appropriately
     */
    new_max = (FT_UInt)( outline->n_points + 2 );
    old_max = hints->max_points;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 2 + 7 ) & ~7;

      if ( FT_RENEW_ARRAY( hints->points, old_max, new_max ) )
        goto Exit;

      hints->max_points = new_max;
    }

    hints->num_points   = outline->n_points;
    hints->num_contours = outline->n_contours;

    /* We can't rely on the value of `FT_Outline.flags' to know the fill   */
    /* direction used for a glyph, given that some fonts are broken (e.g., */
    /* the Arphic ones).  We thus recompute it each time we need to.       */
    /*                                                                     */
    hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_UP;
    hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_LEFT;

    if ( FT_Outline_Get_Orientation( outline ) == FT_ORIENTATION_POSTSCRIPT )
    {
      hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_DOWN;
      hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_RIGHT;
    }

    hints->x_scale = x_scale;
    hints->y_scale = y_scale;
    hints->x_delta = x_delta;
    hints->y_delta = y_delta;

    points = hints->points;
    if ( hints->num_points == 0 )
      goto Exit;

    {
      AF_Point  point;
      AF_Point  point_limit = points + hints->num_points;


      /* compute coordinates & Bezier flags */
      {
        FT_Vector*  vec = outline->points;
        char*       tag = outline->tags;


        for ( point = points; point < point_limit; point++, vec++, tag++ )
        {
          point->fx = (FT_Short)vec->x;
          point->fy = (FT_Short)vec->y;
          point->ox = point->x = FT_MulFix( vec->x, x_scale ) + x_delta;
          point->oy = point->y = FT_MulFix( vec->y, y_scale ) + y_delta;

          switch ( FT_CURVE_TAG( *tag ) )
          {
          case FT_CURVE_TAG_CONIC:
            point->flags = AF_FLAG_CONIC;
            break;
          case FT_CURVE_TAG_CUBIC:
            point->flags = AF_FLAG_CUBIC;
            break;
          default:
            point->flags = 0;
          }
        }
      }

      /* compute `next' and `prev' */
      {
        FT_Int    contour_index;
        AF_Point  prev;
        AF_Point  first;
        AF_Point  end;


        contour_index = 0;

        first = points;
        end   = points + outline->contours[0];
        prev  = end;

        for ( point = points; point < point_limit; point++ )
        {
          point->prev = prev;
          if ( point < end )
          {
            point->next = point + 1;
            prev        = point;
          }
          else
          {
            point->next = first;
            contour_index++;
            if ( point + 1 < point_limit )
            {
              end   = points + outline->contours[contour_index];
              first = point + 1;
              prev  = end;
            }
          }
        }
      }

      /* set-up the contours array */
      {
        AF_Point*  contour       = hints->contours;
        AF_Point*  contour_limit = contour + hints->num_contours;
        short*     end           = outline->contours;
        short      idx           = 0;


        for ( ; contour < contour_limit; contour++, end++ )
        {
          contour[0] = points + idx;
          idx        = (short)( end[0] + 1 );
        }
      }

      /* compute directions of in & out vectors */
      {
        for ( point = points; point < point_limit; point++ )
        {
          AF_Point  prev;
          AF_Point  next;
          FT_Pos    in_x, in_y, out_x, out_y;


          prev   = point->prev;
          in_x   = point->fx - prev->fx;
          in_y   = point->fy - prev->fy;

          point->in_dir = (FT_Char)af_direction_compute( in_x, in_y );

          next   = point->next;
          out_x  = next->fx - point->fx;
          out_y  = next->fy - point->fy;

          point->out_dir = (FT_Char)af_direction_compute( out_x, out_y );

          if ( point->flags & ( AF_FLAG_CONIC | AF_FLAG_CUBIC ) )
          {
          Is_Weak_Point:
            point->flags |= AF_FLAG_WEAK_INTERPOLATION;
          }
          else if ( point->out_dir == point->in_dir )
          {
            AF_Angle  angle_in, angle_out, delta;


            if ( point->out_dir != AF_DIR_NONE )
              goto Is_Weak_Point;

            angle_in  = af_angle_atan( in_x, in_y );
            angle_out = af_angle_atan( out_x, out_y );

            AF_ANGLE_DIFF( delta, angle_in, angle_out );

            if ( delta < 2 && delta > -2 )
              goto Is_Weak_Point;
          }
          else if ( point->in_dir == -point->out_dir )
            goto Is_Weak_Point;
        }
      }
    }

    /* compute inflection points */
    af_glyph_hints_compute_inflections( hints );

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_save( AF_GlyphHints  hints,
                       FT_Outline*    outline )
  {
    AF_Point    point = hints->points;
    AF_Point    limit = point + hints->num_points;
    FT_Vector*  vec   = outline->points;
    char*       tag   = outline->tags;


    for ( ; point < limit; point++, vec++, tag++ )
    {
      vec->x = point->x;
      vec->y = point->y;

      if ( point->flags & AF_FLAG_CONIC )
        tag[0] = FT_CURVE_TAG_CONIC;
      else if ( point->flags & AF_FLAG_CUBIC )
        tag[0] = FT_CURVE_TAG_CUBIC;
      else
        tag[0] = FT_CURVE_TAG_ON;
    }
  }


  /****************************************************************
   *
   *                     EDGE POINT GRID-FITTING
   *
   ****************************************************************/


  FT_LOCAL_DEF( void )
  af_glyph_hints_align_edge_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_AxisHints  axis       = & hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    AF_Edge       edge;


    for ( edge = edges; edge < edge_limit; edge++ )
    {
      /* move the points of each segment     */
      /* in each edge to the edge's position */
      AF_Segment  seg = edge->first;


      do
      {
        AF_Point  point = seg->first;


        for (;;)
        {
          if ( dim == AF_DIMENSION_HORZ )
          {
            point->x      = edge->pos;
            point->flags |= AF_FLAG_TOUCH_X;
          }
          else
          {
            point->y      = edge->pos;
            point->flags |= AF_FLAG_TOUCH_Y;
          }

          if ( point == seg->last )
            break;

          point = point->next;
        }

        seg = seg->edge_next;

      } while ( seg != edge->first );
    }
  }


  /****************************************************************
   *
   *                    STRONG POINT INTERPOLATION
   *
   ****************************************************************/


  /* hint the strong points -- this is equivalent to the TrueType `IP' */
  /* hinting instruction                                               */

  FT_LOCAL_DEF( void )
  af_glyph_hints_align_strong_points( AF_GlyphHints  hints,
                                      AF_Dimension   dim )
  {
    AF_Point      points      = hints->points;
    AF_Point      point_limit = points + hints->num_points;
    AF_AxisHints  axis        = &hints->axis[dim];
    AF_Edge       edges       = axis->edges;
    AF_Edge       edge_limit  = edges + axis->num_edges;
    AF_Flags      touch_flag;


    if ( dim == AF_DIMENSION_HORZ )
      touch_flag = AF_FLAG_TOUCH_X;
    else
      touch_flag  = AF_FLAG_TOUCH_Y;

    if ( edges < edge_limit )
    {
      AF_Point  point;
      AF_Edge   edge;


      for ( point = points; point < point_limit; point++ )
      {
        FT_Pos  u, ou, fu;  /* point position */
        FT_Pos  delta;


        if ( point->flags & touch_flag )
          continue;

        /* if this point is candidate to weak interpolation, we       */
        /* interpolate it after all strong points have been processed */

        if (  ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) &&
             !( point->flags & AF_FLAG_INFLECTION )         )
          continue;

        if ( dim == AF_DIMENSION_VERT )
        {
          u  = point->fy;
          ou = point->oy;
        }
        else
        {
          u  = point->fx;
          ou = point->ox;
        }

        fu = u;

        /* is the point before the first edge? */
        edge  = edges;
        delta = edge->fpos - u;
        if ( delta >= 0 )
        {
          u = edge->pos - ( edge->opos - ou );
          goto Store_Point;
        }

        /* is the point after the last edge? */
        edge  = edge_limit - 1;
        delta = u - edge->fpos;
        if ( delta >= 0 )
        {
          u = edge->pos + ( ou - edge->opos );
          goto Store_Point;
        }

        {
          FT_UInt  min, max, mid;
          FT_Pos   fpos;


          /* find enclosing edges */
          min = 0;
          max = edge_limit - edges;

          while ( min < max )
          {
            mid  = ( max + min ) >> 1;
            edge = edges + mid;
            fpos = edge->fpos;

            if ( u < fpos )
              max = mid;
            else if ( u > fpos )
              min = mid + 1;
            else
            {
              /* we are on the edge */
              u = edge->pos;
              goto Store_Point;
            }
          }

          {
            AF_Edge  before = edges + min - 1;
            AF_Edge  after  = edges + min + 0;


            /* assert( before && after && before != after ) */
            if ( before->scale == 0 )
              before->scale = FT_DivFix( after->pos - before->pos,
                                         after->fpos - before->fpos );

            u = before->pos + FT_MulFix( fu - before->fpos,
                                         before->scale );
          }
        }

      Store_Point:
        /* save the point position */
        if ( dim == AF_DIMENSION_HORZ )
          point->x = u;
        else
          point->y = u;

        point->flags |= touch_flag;
      }
    }
  }


  /****************************************************************
   *
   *                    WEAK POINT INTERPOLATION
   *
   ****************************************************************/


  static void
  af_iup_shift( AF_Point  p1,
                AF_Point  p2,
                AF_Point  ref )
  {
    AF_Point  p;
    FT_Pos    delta = ref->u - ref->v;


    for ( p = p1; p < ref; p++ )
      p->u = p->v + delta;

    for ( p = ref + 1; p <= p2; p++ )
      p->u = p->v + delta;
  }


  static void
  af_iup_interp( AF_Point  p1,
                 AF_Point  p2,
                 AF_Point  ref1,
                 AF_Point  ref2 )
  {
    AF_Point  p;
    FT_Pos    u;
    FT_Pos    v1 = ref1->v;
    FT_Pos    v2 = ref2->v;
    FT_Pos    d1 = ref1->u - v1;
    FT_Pos    d2 = ref2->u - v2;


    if ( p1 > p2 )
      return;

    if ( v1 == v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else
          u += d2;

        p->u = u;
      }
      return;
    }

    if ( v1 < v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else if ( u >= v2 )
          u += d2;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
    else
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v2 )
          u += d2;
        else if ( u >= v1 )
          u += d1;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_align_weak_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_Point   points        = hints->points;
    AF_Point   point_limit   = points + hints->num_points;
    AF_Point*  contour       = hints->contours;
    AF_Point*  contour_limit = contour + hints->num_contours;
    AF_Flags   touch_flag;
    AF_Point   point;
    AF_Point   end_point;
    AF_Point   first_point;


    /* PASS 1: Move segment points to edge positions */

    if ( dim == AF_DIMENSION_HORZ )
    {
      touch_flag = AF_FLAG_TOUCH_X;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->x;
        point->v = point->ox;
      }
    }
    else
    {
      touch_flag = AF_FLAG_TOUCH_Y;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->y;
        point->v = point->oy;
      }
    }

    point = points;

    for ( ; contour < contour_limit; contour++ )
    {
      point       = *contour;
      end_point   = point->prev;
      first_point = point;

      while ( point <= end_point && !( point->flags & touch_flag ) )
        point++;

      if ( point <= end_point )
      {
        AF_Point  first_touched = point;
        AF_Point  cur_touched   = point;


        point++;
        while ( point <= end_point )
        {
          if ( point->flags & touch_flag )
          {
            /* we found two successive touched points; we interpolate */
            /* all contour points between them                        */
            af_iup_interp( cur_touched + 1, point - 1,
                           cur_touched, point );
            cur_touched = point;
          }
          point++;
        }

        if ( cur_touched == first_touched )
        {
          /* this is a special case: only one point was touched in the */
          /* contour; we thus simply shift the whole contour           */
          af_iup_shift( first_point, end_point, cur_touched );
        }
        else
        {
          /* now interpolate after the last touched point to the end */
          /* of the contour                                          */
          af_iup_interp( cur_touched + 1, end_point,
                         cur_touched, first_touched );

          /* if the first contour point isn't touched, interpolate */
          /* from the contour start to the first touched point     */
          if ( first_touched > points )
            af_iup_interp( first_point, first_touched - 1,
                           cur_touched, first_touched );
        }
      }
    }

    /* now save the interpolated values back to x/y */
    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < point_limit; point++ )
        point->x = point->u;
    }
    else
    {
      for ( point = points; point < point_limit; point++ )
        point->y = point->u;
    }
  }


#ifdef AF_USE_WARPER

  FT_LOCAL_DEF( void )
  af_glyph_hints_scale_dim( AF_GlyphHints  hints,
                            AF_Dimension   dim,
                            FT_Fixed       scale,
                            FT_Pos         delta )
  {
    AF_Point  points       = hints->points;
    AF_Point  points_limit = points + hints->num_points;
    AF_Point  point;
    

    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < points_limit; point++ )
        point->x = FT_MulFix( point->fx, scale ) + delta;
    }
    else
    {
      for ( point = points; point < points_limit; point++ )
        point->y = FT_MulFix( point->fy, scale ) + delta;
    }
  }

#endif /* AF_USE_WARPER */

/* END */

/***************************************************************************/
/*                                                                         */
/*  afdummy.c                                                              */
/*                                                                         */
/*    Auto-fitter dummy routines to be used if no hinting should be        */
/*    performed (body).                                                    */
/*                                                                         */
/*  Copyright 2003, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afdummy.h"
#include "afhints.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints     hints,
                       AF_ScriptMetrics  metrics )
  {
    af_glyph_hints_rescale( hints,
                            metrics );
    return 0;
  }


  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    FT_UNUSED( hints );
    FT_UNUSED( outline );

    return 0;
  }


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_dummy_script_class =
  {
    AF_SCRIPT_NONE,
    NULL,

    sizeof( AF_ScriptMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_dummy_hints_init,
    (AF_Script_ApplyHintsFunc)  af_dummy_hints_apply
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  aflatin.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines for latin script (body).                */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aflatin.h"
#include "aferrors.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****            L A T I N   G L O B A L   M E T R I C S            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  af_latin_metrics_init_widths( AF_LatinMetrics  metrics,
                                FT_Face          face,
                                FT_ULong         charcode )
  {
    /* scan the array of segments in each direction */
    AF_GlyphHintsRec  hints[1];


    af_glyph_hints_init( hints, face->memory );

    metrics->axis[AF_DIMENSION_HORZ].width_count = 0;
    metrics->axis[AF_DIMENSION_VERT].width_count = 0;

    {
      FT_Error             error;
      FT_UInt              glyph_index;
      int                  dim;
      AF_LatinMetricsRec   dummy[1];
      AF_Scaler            scaler = &dummy->root.scaler;


      glyph_index = FT_Get_Char_Index( face, charcode );
      if ( glyph_index == 0 )
        goto Exit;

      error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
      if ( error || face->glyph->outline.n_points <= 0 )
        goto Exit;

      FT_ZERO( dummy );

      dummy->units_per_em = metrics->units_per_em;
      scaler->x_scale     = scaler->y_scale = 0x10000L;
      scaler->x_delta     = scaler->y_delta = 0;
      scaler->face        = face;
      scaler->render_mode = FT_RENDER_MODE_NORMAL;
      scaler->flags       = 0;

      af_glyph_hints_rescale( hints, (AF_ScriptMetrics)dummy );

      error = af_glyph_hints_reload( hints, &face->glyph->outline );
      if ( error )
        goto Exit;

      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis    = &metrics->axis[dim];
        AF_AxisHints  axhints = &hints->axis[dim];
        AF_Segment    seg, limit, link;
        FT_UInt       num_widths = 0;


        error = af_latin_hints_compute_segments( hints,
                                                 (AF_Dimension)dim );
        if ( error )
          goto Exit;

        af_latin_hints_link_segments( hints,
                                      (AF_Dimension)dim );

        seg   = axhints->segments;
        limit = seg + axhints->num_segments;

        for ( ; seg < limit; seg++ )
        {
          link = seg->link;

          /* we only consider stem segments there! */
          if ( link && link->link == seg && link > seg )
          {
            FT_Pos  dist;


            dist = seg->pos - link->pos;
            if ( dist < 0 )
              dist = -dist;

            if ( num_widths < AF_LATIN_MAX_WIDTHS )
              axis->widths[ num_widths++ ].org = dist;
          }
        }

        af_sort_widths( num_widths, axis->widths );
        axis->width_count = num_widths;
      }

  Exit:
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis = &metrics->axis[dim];
        FT_Pos        stdw;


        stdw = ( axis->width_count > 0 )
                 ? axis->widths[0].org
                 : AF_LATIN_CONSTANT( metrics, 50 );

        /* let's try 20% of the smallest width */
        axis->edge_distance_threshold = stdw / 5;
      }
    }

    af_glyph_hints_done( hints );
  }



#define AF_LATIN_MAX_TEST_CHARACTERS  12


  static const char* const  af_latin_blue_chars[AF_LATIN_MAX_BLUES] =
  {
    "THEZOCQS",
    "HEZLOCUS",
    "fijkdbh",
    "xzroesc",
    "xzroesc",
    "pqgjy"
  };


  static void
  af_latin_metrics_init_blues( AF_LatinMetrics  metrics,
                               FT_Face          face )
  {
    FT_Pos        flats [AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Pos        rounds[AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Int        num_flats;
    FT_Int        num_rounds;
    FT_Int        bb;
    AF_LatinBlue  blue;
    FT_Error      error;
    AF_LatinAxis  axis  = &metrics->axis[AF_DIMENSION_VERT];
    FT_GlyphSlot  glyph = face->glyph;


    /* we compute the blues simply by loading each character from the    */
    /* 'af_latin_blue_chars[blues]' string, then compute its top-most or */
    /* bottom-most points (depending on `AF_IS_TOP_BLUE')                */

    AF_LOG(( "blue zones computation\n" ));
    AF_LOG(( " - - - - - - - - - - - - - - - - - - - - - - - -\n" ));

    for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
    {
      const char*  p     = af_latin_blue_chars[bb];
      const char*  limit = p + AF_LATIN_MAX_TEST_CHARACTERS;
      FT_Pos*      blue_ref;
      FT_Pos*      blue_shoot;


      AF_LOG(( "blue %3d: ", bb ));

      num_flats  = 0;
      num_rounds = 0;

      for ( ; p < limit && *p; p++ )
      {
        FT_UInt     glyph_index;
        FT_Vector*  extremum;
        FT_Vector*  points;
        FT_Vector*  point_limit;
        FT_Vector*  point;
        FT_Bool     round;


        AF_LOG(( "'%c'", *p ));

        /* load the character in the face -- skip unknown or empty ones */
        glyph_index = FT_Get_Char_Index( face, (FT_UInt)*p );
        if ( glyph_index == 0 )
          continue;

        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
        if ( error || glyph->outline.n_points <= 0 )
          continue;

        /* now compute min or max point indices and coordinates */
        points      = glyph->outline.points;
        point_limit = points + glyph->outline.n_points;
        point       = points;
        extremum    = point;
        point++;

        if ( AF_LATIN_IS_TOP_BLUE( bb ) )
        {
          for ( ; point < point_limit; point++ )
            if ( point->y > extremum->y )
              extremum = point;
        }
        else
        {
          for ( ; point < point_limit; point++ )
            if ( point->y < extremum->y )
              extremum = point;
        }

        AF_LOG(( "%5d", (int)extremum->y ));

        /* now, check whether the point belongs to a straight or round  */
        /* segment; we first need to find in which contour the extremum */
        /* lies, then see its previous and next points                  */
        {
          FT_Int  idx = (FT_Int)( extremum - points );
          FT_Int  n;
          FT_Int  first, last, prev, next, end;
          FT_Pos  dist;


          last  = -1;
          first = 0;

          for ( n = 0; n < glyph->outline.n_contours; n++ )
          {
            end = glyph->outline.contours[n];
            if ( end >= idx )
            {
              last = end;
              break;
            }
            first = end + 1;
          }

          /* XXX: should never happen! */
          if ( last < 0 )
            continue;

          /* now look for the previous and next points that are not on the */
          /* same Y coordinate.  Threshold the `closeness'...              */

          prev = idx;
          next = prev;

          do
          {
            if ( prev > first )
              prev--;
            else
              prev = last;

            dist = points[prev].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( prev != idx );

          do
          {
            if ( next < last )
              next++;
            else
              next = first;

            dist = points[next].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( next != idx );

          /* now, set the `round' flag depending on the segment's kind */
          round = FT_BOOL(
            FT_CURVE_TAG( glyph->outline.tags[prev] ) != FT_CURVE_TAG_ON ||
            FT_CURVE_TAG( glyph->outline.tags[next] ) != FT_CURVE_TAG_ON );

          AF_LOG(( "%c ", round ? 'r' : 'f' ));
        }

        if ( round )
          rounds[num_rounds++] = extremum->y;
        else
          flats[num_flats++] = extremum->y;
      }

      AF_LOG(( "\n" ));

      if ( num_flats == 0 && num_rounds == 0 )
      {
        /*
         *  we couldn't find a single glyph to compute this blue zone,
         *  we will simply ignore it then
         */
        AF_LOG(( "empty!\n" ));
        continue;
      }

      /* we have computed the contents of the `rounds' and `flats' tables, */
      /* now determine the reference and overshoot position of the blue -- */
      /* we simply take the median value after a simple sort               */
      af_sort_pos( num_rounds, rounds );
      af_sort_pos( num_flats,  flats );

      blue       = & axis->blues[axis->blue_count];
      blue_ref   = & blue->ref.org;
      blue_shoot = & blue->shoot.org;

      axis->blue_count++;

      if ( num_flats == 0 )
      {
        *blue_ref   =
        *blue_shoot = rounds[num_rounds / 2];
      }
      else if ( num_rounds == 0 )
      {
        *blue_ref   =
        *blue_shoot = flats[num_flats / 2];
      }
      else
      {
        *blue_ref   = flats[num_flats / 2];
        *blue_shoot = rounds[num_rounds / 2];
      }

      /* there are sometimes problems: if the overshoot position of top     */
      /* zones is under its reference position, or the opposite for bottom  */
      /* zones.  We must thus check everything there and correct the errors */
      if ( *blue_shoot != *blue_ref )
      {
        FT_Pos   ref      = *blue_ref;
        FT_Pos   shoot    = *blue_shoot;
        FT_Bool  over_ref = FT_BOOL( shoot > ref );


        if ( AF_LATIN_IS_TOP_BLUE( bb ) ^ over_ref )
          *blue_shoot = *blue_ref = ( shoot + ref ) / 2;
      }

      blue->flags = 0;
      if ( AF_LATIN_IS_TOP_BLUE( bb ) )
        blue->flags |= AF_LATIN_BLUE_TOP;

      /*
       * The following flags is used later to adjust the y and x scales
       * in order to optimize the pixel grid alignment of the top of small
       * letters.
       */
      if ( bb == AF_LATIN_BLUE_SMALL_TOP )
        blue->flags |= AF_LATIN_BLUE_ADJUSTMENT;

      AF_LOG(( "-- ref = %ld, shoot = %ld\n", *blue_ref, *blue_shoot ));
    }

    return;
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face )
  {
    FT_Error    error = AF_Err_Ok;
    FT_CharMap  oldmap = face->charmap;
    FT_UInt     ee;

    static const FT_Encoding  latin_encodings[] =
    {
      FT_ENCODING_UNICODE,
      FT_ENCODING_APPLE_ROMAN,
      FT_ENCODING_ADOBE_STANDARD,
      FT_ENCODING_ADOBE_LATIN_1,
      FT_ENCODING_NONE  /* end of list */
    };


    metrics->units_per_em = face->units_per_EM;

    /* do we have a latin charmap in there? */
    for ( ee = 0; latin_encodings[ee] != FT_ENCODING_NONE; ee++ )
    {
      error = FT_Select_Charmap( face, latin_encodings[ee] );
      if ( !error )
        break;
    }

    if ( !error )
    {
      /* For now, compute the standard width and height from the `o'. */
      af_latin_metrics_init_widths( metrics, face, 'o' );
      af_latin_metrics_init_blues( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );
    return AF_Err_Ok;
  }


  static void
  af_latin_metrics_scale_dim( AF_LatinMetrics  metrics,
                              AF_Scaler        scaler,
                              AF_Dimension     dim )
  {
    FT_Fixed      scale;
    FT_Pos        delta;
    AF_LatinAxis  axis;
    FT_UInt       nn;


    if ( dim == AF_DIMENSION_HORZ )
    {
      scale = scaler->x_scale;
      delta = scaler->x_delta;
    }
    else
    {
      scale = scaler->y_scale;
      delta = scaler->y_delta;
    }

    axis = &metrics->axis[dim];

    if ( axis->org_scale == scale && axis->org_delta == delta )
      return;

    axis->org_scale = scale;
    axis->org_delta = delta;

    /*
     * correct X and Y scale to optimize the alignment of the top of small
     * letters to the pixel grid
     */
    {
      AF_LatinAxis  Axis = &metrics->axis[AF_DIMENSION_VERT];
      AF_LatinBlue  blue = NULL;


      for ( nn = 0; nn < Axis->blue_count; nn++ )
      {
        if ( Axis->blues[nn].flags & AF_LATIN_BLUE_ADJUSTMENT )
        {
          blue = &Axis->blues[nn];
          break;
        }
      }

      if ( blue )
      {
        FT_Pos  scaled = FT_MulFix( blue->shoot.org, scaler->y_scale );
        FT_Pos  fitted = FT_PIX_ROUND( scaled );


        if ( scaled != fitted )
        {
          if ( dim == AF_DIMENSION_HORZ )
          {
            if ( fitted < scaled )
              scale -= scale/50;  /* x_scale = x_scale*0.98 */
          }
          else
          {
            scale = FT_MulDiv( scale, fitted, scaled );
          }
        }
      }
    }

    axis->scale = scale;
    axis->delta = delta;

    if ( dim == AF_DIMENSION_HORZ )
    {
      metrics->root.scaler.x_scale = scale;
      metrics->root.scaler.x_delta = delta;
    }
    else
    {
      metrics->root.scaler.y_scale = scale;
      metrics->root.scaler.y_delta = delta;
    }

    /* scale the standard widths */
    for ( nn = 0; nn < axis->width_count; nn++ )
    {
      AF_Width  width = axis->widths + nn;


      width->cur = FT_MulFix( width->org, scale );
      width->fit = width->cur;
    }

    if ( dim == AF_DIMENSION_VERT )
    {
      /* scale the blue zones */
      for ( nn = 0; nn < axis->blue_count; nn++ )
      {
        AF_LatinBlue  blue = &axis->blues[nn];
        FT_Pos        dist;


        blue->ref.cur   = FT_MulFix( blue->ref.org, scale ) + delta;
        blue->ref.fit   = blue->ref.cur;
        blue->shoot.cur = FT_MulFix( blue->shoot.org, scale ) + delta;
        blue->shoot.fit = blue->shoot.cur;
        blue->flags    &= ~AF_LATIN_BLUE_ACTIVE;

        /* a blue zone is only active if it is less than 3/4 pixels tall */
        dist = FT_MulFix( blue->ref.org - blue->shoot.org, scale );
        if ( dist <= 48 && dist >= -48 )
        {
          FT_Pos  delta1, delta2;


          delta1 = blue->shoot.org - blue->ref.org;
          delta2 = delta1;
          if ( delta1 < 0 )
            delta2 = -delta2;

          delta2 = FT_MulFix( delta2, scale );

          if ( delta2 < 32 )
            delta2 = 0;
          else if ( delta2 < 64 )
            delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & ~31 );
          else
            delta2 = FT_PIX_ROUND( delta2 );

          if ( delta1 < 0 )
            delta2 = -delta2;

          blue->ref.fit   = FT_PIX_ROUND( blue->ref.cur );
          blue->shoot.fit = blue->ref.fit + delta2;

          blue->flags |= AF_LATIN_BLUE_ACTIVE;
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_latin_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler )
  {
    metrics->root.scaler.render_mode = scaler->render_mode;
    metrics->root.scaler.face        = scaler->face;

    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_HORZ );
    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_VERT );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           L A T I N   G L Y P H   A N A L Y S I S             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_segments( AF_GlyphHints  hints,
                                   AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    FT_Memory     memory        = hints->memory;
    FT_Error      error         = AF_Err_Ok;
    AF_Segment    segment       = NULL;
    AF_Point*     contour       = hints->contours;
    AF_Point*     contour_limit = contour + hints->num_contours;
    AF_Direction  major_dir, segment_dir;

#ifdef AF_HINT_METRICS
    AF_Point  min_point =  0;
    AF_Point  max_point =  0;
    FT_Pos    min_coord =  32000;
    FT_Pos    max_coord = -32000;
#endif

    major_dir   = (AF_Direction)FT_ABS( axis->major_dir );
    segment_dir = major_dir;

    axis->num_segments = 0;

    /* set up (u,v) in each point */
    if ( dim == AF_DIMENSION_HORZ )
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fx;
        point->v = point->fy;
      }
    }
    else
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fy;
        point->v = point->fx;
      }
    }

    /* do each contour separately */
    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  point   =  contour[0];
      AF_Point  last    =  point->prev;
      int       on_edge =  0;
      FT_Pos    min_pos =  32000;  /* minimum segment pos != min_coord */
      FT_Pos    max_pos = -32000;  /* maximum segment pos != max_coord */
      FT_Bool   passed;


#ifdef AF_HINT_METRICS
      if ( point->u < min_coord )
      {
        min_coord = point->u;
        min_point = point;
      }
      if ( point->u > max_coord )
      {
        max_coord = point->u;
        max_point = point;
      }
#endif

      if ( point == last )  /* skip singletons -- just in case */
        continue;

      if ( FT_ABS( last->out_dir )  == major_dir &&
           FT_ABS( point->out_dir ) == major_dir )
      {
        /* we are already on an edge, try to locate its start */
        last = point;

        for (;;)
        {
          point = point->prev;
          if ( FT_ABS( point->out_dir ) != major_dir )
          {
            point = point->next;
            break;
          }
          if ( point == last )
            break;
        }
      }

      last   = point;
      passed = 0;

      for (;;)
      {
        FT_Pos  u, v;


        if ( on_edge )
        {
          u = point->u;
          if ( u < min_pos )
            min_pos = u;
          if ( u > max_pos )
            max_pos = u;

          if ( point->out_dir != segment_dir || point == last )
          {
            /* we are just leaving an edge; record a new segment! */
            segment->last = point;
            segment->pos  = (FT_Short)( ( min_pos + max_pos ) >> 1 );

            /* a segment is round if either its first or last point */
            /* is a control point                                   */
            if ( ( segment->first->flags | point->flags ) &
                   AF_FLAG_CONTROL                        )
              segment->flags |= AF_EDGE_ROUND;

            /* compute segment size */
            min_pos = max_pos = point->v;

            v = segment->first->v;
            if ( v < min_pos )
              min_pos = v;
            if ( v > max_pos )
              max_pos = v;

            segment->min_coord = (FT_Short)min_pos;
            segment->max_coord = (FT_Short)max_pos;

            on_edge = 0;
            segment = NULL;
            /* fallthrough */
          }
        }

        /* now exit if we are at the start/end point */
        if ( point == last )
        {
          if ( passed )
            break;
          passed = 1;
        }

        if ( !on_edge && FT_ABS( point->out_dir ) == major_dir )
        {
          /* this is the start of a new segment! */
          segment_dir = (AF_Direction)point->out_dir;

          /* clear all segment fields */
          error = af_axis_hints_new_segment( axis, memory, &segment );
          if ( error )
            goto Exit;

          segment->dir      = (FT_Char)segment_dir;
          segment->flags    = AF_EDGE_NORMAL;
          min_pos = max_pos = point->u;
          segment->first    = point;
          segment->last     = point;
          segment->contour  = contour;
          segment->score    = 32000;
          segment->len      = 0;
          segment->link     = NULL;
          on_edge           = 1;

#ifdef AF_HINT_METRICS
          if ( point == max_point )
            max_point = 0;

          if ( point == min_point )
            min_point = 0;
#endif
        }

        point = point->next;
      }

    } /* contours */

#ifdef AF_HINT_METRICS
    /* we need to ensure that there are edges on the left-most and  */
    /* right-most points of the glyph in order to hint the metrics; */
    /* we do this by inserting fake segments when needed            */

    if ( dim == AF_DIMENSION_HORZ )
    {
      AF_Point  point       = hints->points;
      AF_Point  point_limit = point + hints->num_points;

      FT_Pos    min_pos =  32000;
      FT_Pos    max_pos = -32000;


      min_point = 0;
      max_point = 0;

      /* compute minimum and maximum points */
      for ( ; point < point_limit; point++ )
      {
        FT_Pos  x = point->fx;


        if ( x < min_pos )
        {
          min_pos   = x;
          min_point = point;
        }
        if ( x > max_pos )
        {
          max_pos   = x;
          max_point = point;
        }
      }

      /* insert minimum segment */
      if ( min_point )
      {
        /* clear all segment fields */
        error = af_axis_hints_new_segment( axis, memory, &segment );
        if ( error )
          goto Exit;

        segment->dir   = segment_dir;
        segment->flags = AF_EDGE_NORMAL;
        segment->first = min_point;
        segment->last  = min_point;
        segment->pos   = min_pos;
        segment->score = 32000;
        segment->len   = 0;
        segment->link  = NULL;

        segment = NULL;
      }

      /* insert maximum segment */
      if ( max_point )
      {
        /* clear all segment fields */
        error = af_axis_hints_new_segment( axis, memory, &segment );
        if ( error )
          goto Exit;

        segment->dir   = segment_dir;
        segment->flags = AF_EDGE_NORMAL;
        segment->first = max_point;
        segment->last  = max_point;
        segment->pos   = max_pos;
        segment->score = 32000;
        segment->len   = 0;
        segment->link  = NULL;

        segment = NULL;
      }
    }
#endif /* AF_HINT_METRICS */

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_link_segments( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Direction  major_dir     = axis->major_dir;
    FT_Pos        len_threshold, len_score;
    AF_Segment    seg1, seg2;


    len_threshold = AF_LATIN_CONSTANT( hints->metrics, 8 );
    if ( len_threshold == 0 )
      len_threshold = 1;

    len_score = AF_LATIN_CONSTANT( hints->metrics, 3000 );

    /* now compare each segment to the others */
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      /* the fake segments are introduced to hint the metrics -- */
      /* we must never link them to anything                     */
      if ( seg1->first == seg1->last || seg1->dir != major_dir )
        continue;

      for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        if ( seg2 != seg1 && seg1->dir + seg2->dir == 0 )
        {
          FT_Pos  pos1 = seg1->pos;
          FT_Pos  pos2 = seg2->pos;
          FT_Pos  dist = pos2 - pos1;


          if ( dist < 0 )
            continue;

          {
            FT_Pos  min = seg1->min_coord;
            FT_Pos  max = seg1->max_coord;
            FT_Pos  len, score;


            if ( min < seg2->min_coord )
              min = seg2->min_coord;

            if ( max > seg2->max_coord )
              max = seg2->max_coord;

            len = max - min;
            if ( len >= len_threshold )
            {
              score = dist + len_score / len;

              if ( score < seg1->score )
              {
                seg1->score = score;
                seg1->link  = seg2;
              }

              if ( score < seg2->score )
              {
                seg2->score = score;
                seg2->link  = seg1;
              }
            }
          }
        }
    }

    /* now, compute the `serif' segments */
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      seg2 = seg1->link;

      if ( seg2 )
      {
        seg2->num_linked++;
        if ( seg2->link != seg1 )
        {
          seg1->link  = 0;
          seg1->serif = seg2->link;
        }
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_edges( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis   = &hints->axis[dim];
    FT_Error      error  = AF_Err_Ok;
    FT_Memory     memory = hints->memory;
    AF_LatinAxis  laxis  = &((AF_LatinMetrics)hints->metrics)->axis[dim];

    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;

    AF_Direction  up_dir;
    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;


    axis->num_edges = 0;

    scale = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                         : hints->y_scale;

    up_dir = ( dim == AF_DIMENSION_HORZ ) ? AF_DIR_UP
                                          : AF_DIR_RIGHT;

    /*********************************************************************/
    /*                                                                   */
    /* We will begin by generating a sorted table of edges for the       */
    /* current direction.  To do so, we simply scan each segment and try */
    /* to find an edge in our table that corresponds to its position.    */
    /*                                                                   */
    /* If no edge is found, we create and insert a new edge in the       */
    /* sorted table.  Otherwise, we simply add the segment to the edge's */
    /* list which will be processed in the second step to compute the    */
    /* edge's properties.                                                */
    /*                                                                   */
    /* Note that the edges table is sorted along the segment/edge        */
    /* position.                                                         */
    /*                                                                   */
    /*********************************************************************/

    edge_distance_threshold = FT_MulFix( laxis->edge_distance_threshold,
                                         scale );
    if ( edge_distance_threshold > 64 / 4 )
      edge_distance_threshold = 64 / 4;

    edge_distance_threshold = FT_DivFix( edge_distance_threshold,
                                         scale );

    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Edge  found = 0;
      FT_Int   ee;


      /* look for an edge corresponding to the segment */
      for ( ee = 0; ee < axis->num_edges; ee++ )
      {
        AF_Edge  edge = axis->edges + ee;
        FT_Pos   dist;


        dist = seg->pos - edge->fpos;
        if ( dist < 0 )
          dist = -dist;

        if ( dist < edge_distance_threshold )
        {
          found = edge;
          break;
        }
      }

      if ( !found )
      {
        AF_Edge   edge;


        /* insert a new edge in the list and */
        /* sort according to the position    */
        error = af_axis_hints_new_edge( axis, seg->pos, memory, &edge );
        if ( error )
          goto Exit;

        /* add the segment to the new edge's list */
        FT_ZERO( edge );

        edge->first    = seg;
        edge->last     = seg;
        edge->fpos     = seg->pos;
        edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
        seg->edge_next = seg;
      }
      else
      {
        /* if an edge was found, simply add the segment to the edge's */
        /* list                                                       */
        seg->edge_next         = found->first;
        found->last->edge_next = seg;
        found->last            = seg;
      }
    }


    /*********************************************************************/
    /*                                                                   */
    /* Good, we will now compute each edge's properties according to     */
    /* segments found on its position.  Basically, these are:            */
    /*                                                                   */
    /*  - edge's main direction                                          */
    /*  - stem edge, serif edge or both (which defaults to stem then)    */
    /*  - rounded edge, straight or both (which defaults to straight)    */
    /*  - link for edge                                                  */
    /*                                                                   */
    /*********************************************************************/

    /* first of all, set the `edge' field in each segment -- this is */
    /* required in order to compute edge links                       */

    /*
     * Note that removing this loop and setting the `edge' field of each
     * segment directly in the code above slows down execution speed for
     * some reasons on platforms like the Sun.
     */
    {
      AF_Edge  edges      = axis->edges;
      AF_Edge  edge_limit = edges + axis->num_edges;
      AF_Edge  edge;


      for ( edge = edges; edge < edge_limit; edge++ )
      {
        seg = edge->first;
        if ( seg )
          do
          {
            seg->edge = edge;
            seg       = seg->edge_next;

          } while ( seg != edge->first );
      }

      /* now, compute each edge properties */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        FT_Int  is_round    = 0;  /* does it contain round segments?    */
        FT_Int  is_straight = 0;  /* does it contain straight segments? */
        FT_Pos  ups         = 0;  /* number of upwards segments         */
        FT_Pos  downs       = 0;  /* number of downwards segments       */


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          /* check for roundness of segment */
          if ( seg->flags & AF_EDGE_ROUND )
            is_round++;
          else
            is_straight++;

          /* check for segment direction */
          if ( seg->dir == up_dir )
            ups   += seg->max_coord-seg->min_coord;
          else
            downs += seg->max_coord-seg->min_coord;

          /* check for links -- if seg->serif is set, then seg->link must */
          /* be ignored                                                   */
          is_serif = (FT_Bool)( seg->serif && seg->serif->edge != edge );

          if ( seg->link || is_serif )
          {
            AF_Edge     edge2;
            AF_Segment  seg2;


            edge2 = edge->link;
            seg2  = seg->link;

            if ( is_serif )
            {
              seg2  = seg->serif;
              edge2 = edge->serif;
            }

            if ( edge2 )
            {
              FT_Pos  edge_delta;
              FT_Pos  seg_delta;


              edge_delta = edge->fpos - edge2->fpos;
              if ( edge_delta < 0 )
                edge_delta = -edge_delta;

              seg_delta = seg->pos - seg2->pos;
              if ( seg_delta < 0 )
                seg_delta = -seg_delta;

              if ( seg_delta < edge_delta )
                edge2 = seg2->edge;
            }
            else
              edge2 = seg2->edge;

            if ( is_serif )
            {
              edge->serif   = edge2;
              edge2->flags |= AF_EDGE_SERIF;
            }
            else
              edge->link  = edge2;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );

        /* set the round/straight flags */
        edge->flags = AF_EDGE_NORMAL;

        if ( is_round > 0 && is_round >= is_straight )
          edge->flags |= AF_EDGE_ROUND;

        /* set the edge's main direction */
        edge->dir = AF_DIR_NONE;

        if ( ups > downs )
          edge->dir = (FT_Char)up_dir;

        else if ( ups < downs )
          edge->dir = (FT_Char)-up_dir;

        else if ( ups == downs )
          edge->dir = 0;  /* both up and down! */

        /* gets rid of serifs if link is set                */
        /* XXX: This gets rid of many unpleasant artefacts! */
        /*      Example: the `c' in cour.pfa at size 13     */

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_detect_features( AF_GlyphHints  hints,
                                  AF_Dimension   dim )
  {
    FT_Error  error;


    error = af_latin_hints_compute_segments( hints, dim );
    if ( !error )
    {
      af_latin_hints_link_segments( hints, dim );

      error = af_latin_hints_compute_edges( hints, dim );
    }
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_compute_blue_edges( AF_GlyphHints    hints,
                                     AF_LatinMetrics  metrics )
  {
    AF_AxisHints  axis       = &hints->axis[ AF_DIMENSION_VERT ];
    AF_Edge       edge       = axis->edges;
    AF_Edge       edge_limit = edge + axis->num_edges;
    AF_LatinAxis  latin      = &metrics->axis[ AF_DIMENSION_VERT ];
    FT_Fixed      scale      = latin->scale;


    /* compute which blue zones are active, i.e. have their scaled */
    /* size < 3/4 pixels                                           */

    /* for each horizontal edge search the blue zone which is closest */
    for ( ; edge < edge_limit; edge++ )
    {
      FT_Int    bb;
      AF_Width  best_blue = NULL;
      FT_Pos    best_dist;  /* initial threshold */


      /* compute the initial threshold as a fraction of the EM size */
      best_dist = FT_MulFix( metrics->units_per_em / 40, scale );

      if ( best_dist > 64 / 2 )
        best_dist = 64 / 2;

      for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
      {
        AF_LatinBlue  blue = latin->blues + bb;
        FT_Bool       is_top_blue, is_major_dir;


        /* skip inactive blue zones (i.e., those that are too small) */
        if ( !( blue->flags & AF_LATIN_BLUE_ACTIVE ) )
          continue;

        /* if it is a top zone, check for right edges -- if it is a bottom */
        /* zone, check for left edges                                      */
        /*                                                                 */
        /* of course, that's for TrueType                                  */
        is_top_blue  = (FT_Byte)( ( blue->flags & AF_LATIN_BLUE_TOP ) != 0 );
        is_major_dir = FT_BOOL( edge->dir == axis->major_dir );

        /* if it is a top zone, the edge must be against the major    */
        /* direction; if it is a bottom zone, it must be in the major */
        /* direction                                                  */
        if ( is_top_blue ^ is_major_dir )
        {
          FT_Pos  dist;


          /* first of all, compare it to the reference position */
          dist = edge->fpos - blue->ref.org;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, scale );
          if ( dist < best_dist )
          {
            best_dist = dist;
            best_blue = & blue->ref;
          }

          /* now, compare it to the overshoot position if the edge is     */
          /* rounded, and if the edge is over the reference position of a */
          /* top zone, or under the reference position of a bottom zone   */
          if ( edge->flags & AF_EDGE_ROUND && dist != 0 )
          {
            FT_Bool  is_under_ref = FT_BOOL( edge->fpos < blue->ref.org );


            if ( is_top_blue ^ is_under_ref )
            {
              blue = latin->blues + bb;
              dist = edge->fpos - blue->shoot.org;
              if ( dist < 0 )
                dist = -dist;

              dist = FT_MulFix( dist, scale );
              if ( dist < best_dist )
              {
                best_dist = dist;
                best_blue = & blue->shoot;
              }
            }
          }
        }
      }

      if ( best_blue )
        edge->blue_edge = best_blue;
    }
  }


  static FT_Error
  af_latin_hints_init( AF_GlyphHints    hints,
                       AF_LatinMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;
    FT_Face         face = metrics->root.scaler.face;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    /*
     *  correct x_scale and y_scale when needed, since they may have
     *  been modified af_latin_scale_dim above
     */
    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    /* compute flags depending on render mode, etc. */
    mode = metrics->root.scaler.render_mode;

#ifdef AF_USE_WARPER
    if ( mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V )
    {
      metrics->root.scaler.render_mode = mode = FT_RENDER_MODE_NORMAL;
    }
#endif

    scaler_flags = hints->scaler_flags;
    other_flags  = 0;

    /*
     *  We snap the width of vertical stems for the monochrome and
     *  horizontal LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD )
      other_flags |= AF_LATIN_HINTS_HORZ_SNAP;

    /*
     *  We snap the width of horizontal stems for the monochrome and
     *  vertical LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD_V )
      other_flags |= AF_LATIN_HINTS_VERT_SNAP;

    /*
     *  We adjust stems to full pixels only if we don't use the `light' mode.
     */
    if ( mode != FT_RENDER_MODE_LIGHT )
      other_flags |= AF_LATIN_HINTS_STEM_ADJUST;

    if ( mode == FT_RENDER_MODE_MONO )
      other_flags |= AF_LATIN_HINTS_MONO;

    /*
     *  In `light' hinting mode we disable horizontal hinting completely.
     *  We also do it if the face is italic.
     */
    if ( mode == FT_RENDER_MODE_LIGHT                    ||
         (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0 )
      scaler_flags |= AF_SCALER_FLAG_NO_HORIZONTAL;

    hints->scaler_flags = scaler_flags;
    hints->other_flags  = other_flags;

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****        L A T I N   G L Y P H   G R I D - F I T T I N G        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* snap a given width in scaled coordinates to one of the */
  /* current standard widths                                */

  static FT_Pos
  af_latin_snap_width( AF_Width  widths,
                       FT_Int    count,
                       FT_Pos    width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;
    FT_Pos  scaled;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n].cur;
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    scaled = FT_PIX_ROUND( reference );

    if ( width >= reference )
    {
      if ( width < scaled + 48 )
        width = reference;
    }
    else
    {
      if ( width > scaled - 48 )
        width = reference;
    }

    return width;
  }


  /* compute the snapped width of a given stem */

  static FT_Pos
  af_latin_compute_stem_width( AF_GlyphHints  hints,
                               AF_Dimension   dim,
                               FT_Pos         width,
                               AF_Edge_Flags  base_flags,
                               AF_Edge_Flags  stem_flags )
  {
    AF_LatinMetrics  metrics  = (AF_LatinMetrics) hints->metrics;
    AF_LatinAxis     axis     = & metrics->axis[dim];
    FT_Pos           dist     = width;
    FT_Int           sign     = 0;
    FT_Int           vertical = ( dim == AF_DIMENSION_VERT );


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
      return width;

    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( (  vertical && !AF_LATIN_HINTS_DO_VERT_SNAP( hints ) ) ||
         ( !vertical && !AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) ) )
    {
      /* smooth hinting process: very lightly quantize the stem width */

      /* leave the widths of serifs alone */

      if ( ( stem_flags & AF_EDGE_SERIF ) && vertical && ( dist < 3 * 64 ) )
        goto Done_Width;

      else if ( ( base_flags & AF_EDGE_ROUND ) )
      {
        if ( dist < 80 )
          dist = 64;
      }
      else if ( dist < 56 )
        dist = 56;

      if ( axis->width_count > 0 )
      {
        FT_Pos  delta;


        /* compare to standard width */
        if ( axis->width_count > 0 )
        {
          delta = dist - axis->widths[0].cur;

          if ( delta < 0 )
            delta = -delta;

          if ( delta < 40 )
          {
            dist = axis->widths[0].cur;
            if ( dist < 48 )
              dist = 48;

            goto Done_Width;
          }
        }

        if ( dist < 3 * 64 )
        {
          delta  = dist & 63;
          dist  &= -64;

          if ( delta < 10 )
            dist += delta;

          else if ( delta < 32 )
            dist += 10;

          else if ( delta < 54 )
            dist += 54;

          else
            dist += delta;
        }
        else
          dist = ( dist + 32 ) & ~63;
      }
    }
    else
    {
      /* strong hinting process: snap the stem width to integer pixels */

      dist = af_latin_snap_width( axis->widths, axis->width_count, dist );

      if ( vertical )
      {
        /* in the case of vertical hinting, always round */
        /* the stem heights to integer pixels            */

        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        if ( AF_LATIN_HINTS_DO_MONO( hints ) )
        {
          /* monochrome horizontal hinting: snap widths to integer pixels */
          /* with a different threshold                                   */

          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          /* for horizontal anti-aliased hinting, we adopt a more subtle */
          /* approach: we strengthen small stems, round stems whose size */
          /* is between 1 and 2 pixels to an integer, otherwise nothing  */

          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            /* round otherwise to prevent color fringes in LCD mode */
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

  Done_Width:
    if ( sign )
      dist = -dist;

    return dist;
  }


  /* align one stem edge relative to the previous stem edge */

  static void
  af_latin_align_linked_edge( AF_GlyphHints  hints,
                              AF_Dimension   dim,
                              AF_Edge        base_edge,
                              AF_Edge        stem_edge )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

    FT_Pos  fitted_width = af_latin_compute_stem_width(
                             hints, dim, dist,
                             (AF_Edge_Flags)base_edge->flags,
                             (AF_Edge_Flags)stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;
  }


  static void
  af_latin_align_serif_edge( AF_GlyphHints  hints,
                             AF_Edge        base,
                             AF_Edge        serif )
  {
    FT_UNUSED( hints );

    serif->pos = base->pos + (serif->opos - base->opos);
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                    E D G E   H I N T I N G                      ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( void )
  af_latin_hint_edges( AF_GlyphHints  hints,
                       AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    FT_Int        n_edges;
    AF_Edge       edge;
    AF_Edge       anchor     = 0;
    FT_Int        has_serifs = 0;


    /* we begin by aligning all stems relative to the blue zone */
    /* if needed -- that's only for horizontal edges            */

    if ( dim == AF_DIMENSION_VERT )
    {
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AF_Width  blue;
        AF_Edge   edge1, edge2;


        if ( edge->flags & AF_EDGE_DONE )
          continue;

        blue  = edge->blue_edge;
        edge1 = NULL;
        edge2 = edge->link;

        if ( blue )
        {
          edge1 = edge;
        }
        else if ( edge2 && edge2->blue_edge )
        {
          blue  = edge2->blue_edge;
          edge1 = edge2;
          edge2 = edge;
        }

        if ( !edge1 )
          continue;

        edge1->pos    = blue->fit;
        edge1->flags |= AF_EDGE_DONE;

        if ( edge2 && !edge2->blue_edge )
        {
          af_latin_align_linked_edge( hints, dim, edge1, edge2 );
          edge2->flags |= AF_EDGE_DONE;
        }

        if ( !anchor )
          anchor = edge;
      }
    }

    /* now we will align all stem edges, trying to maintain the */
    /* relative order of stems in the glyph                     */
    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  edge2;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      /* skip all non-stem edges */
      edge2 = edge->link;
      if ( !edge2 )
      {
        has_serifs++;
        continue;
      }

      /* now align the stem */

      /* this should not happen, but it's better to be safe */
      if ( edge2->blue_edge || edge2 < edge )
      {
        af_latin_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        continue;
      }

      if ( !anchor )
      {
        FT_Pos  org_len, org_center, cur_len;
        FT_Pos  cur_pos1, error1, error2, u_off, d_off;


        org_len = edge2->opos - edge->opos;
        cur_len = af_latin_compute_stem_width(
                    hints, dim, org_len,
                    (AF_Edge_Flags)edge->flags,
                    (AF_Edge_Flags)edge2->flags );
        if ( cur_len <= 64 )
          u_off = d_off = 32;
        else
        {
          u_off = 38;
          d_off = 26;
        }

        if ( cur_len < 96 )
        {
          org_center = edge->opos + ( org_len >> 1 );

          cur_pos1   = FT_PIX_ROUND( org_center );

          error1 = org_center - ( cur_pos1 - u_off );
          if ( error1 < 0 )
            error1 = -error1;

          error2 = org_center - ( cur_pos1 + d_off );
          if ( error2 < 0 )
            error2 = -error2;

          if ( error1 < error2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = cur_pos1 + cur_len / 2;

        }
        else
          edge->pos = FT_PIX_ROUND( edge->opos );

        anchor = edge;

        edge->flags |= AF_EDGE_DONE;

        af_latin_align_linked_edge( hints, dim, edge, edge2 );
      }
      else
      {
        FT_Pos  org_pos, org_len, org_center, cur_len;
        FT_Pos  cur_pos1, cur_pos2, delta1, delta2;


        org_pos    = anchor->pos + ( edge->opos - anchor->opos );
        org_len    = edge2->opos - edge->opos;
        org_center = org_pos + ( org_len >> 1 );

        cur_len = af_latin_compute_stem_width(
                   hints, dim, org_len,
                   (AF_Edge_Flags)edge->flags,
                   (AF_Edge_Flags)edge2->flags );

        if ( cur_len < 96 )
        {
          FT_Pos  u_off, d_off;


          cur_pos1 = FT_PIX_ROUND( org_center );

          if (cur_len <= 64 )
            u_off = d_off = 32;
          else
          {
            u_off = 38;
            d_off = 26;
          }

          delta1 = org_center - ( cur_pos1 - u_off );
          if ( delta1 < 0 )
            delta1 = -delta1;

          delta2 = org_center - ( cur_pos1 + d_off );
          if ( delta2 < 0 )
            delta2 = -delta2;

          if ( delta1 < delta2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = cur_pos1 + cur_len / 2;
        }
        else
        {
          org_pos    = anchor->pos + ( edge->opos - anchor->opos );
          org_len    = edge2->opos - edge->opos;
          org_center = org_pos + ( org_len >> 1 );

          cur_len    = af_latin_compute_stem_width(
                         hints, dim, org_len,
                         (AF_Edge_Flags)edge->flags,
                         (AF_Edge_Flags)edge2->flags );

          cur_pos1   = FT_PIX_ROUND( org_pos );
          delta1     = cur_pos1 + ( cur_len >> 1 ) - org_center;
          if ( delta1 < 0 )
            delta1 = -delta1;

          cur_pos2   = FT_PIX_ROUND( org_pos + org_len ) - cur_len;
          delta2     = cur_pos2 + ( cur_len >> 1 ) - org_center;
          if ( delta2 < 0 )
            delta2 = -delta2;

          edge->pos  = ( delta1 < delta2 ) ? cur_pos1 : cur_pos2;
          edge2->pos = edge->pos + cur_len;
        }

        edge->flags  |= AF_EDGE_DONE;
        edge2->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;
      }
    }

    /* make sure that lowercase m's maintain their symmetry */

    /* In general, lowercase m's have six vertical edges if they are sans */
    /* serif, or twelve if they are with serifs.  This implementation is  */
    /* based on that assumption, and seems to work very well with most    */
    /* faces.  However, if for a certain face this assumption is not      */
    /* true, the m is just rendered like before.  In addition, any stem   */
    /* correction will only be applied to symmetrical glyphs (even if the */
    /* glyph is not an m), so the potential for unwanted distortion is    */
    /* relatively low.                                                    */

    /* We don't handle horizontal edges since we can't easily assure that */
    /* the third (lowest) stem aligns with the base line; it might end up */
    /* one pixel higher or lower.                                         */

    n_edges = edge_limit - edges;
    if ( dim == AF_DIMENSION_HORZ && ( n_edges == 6 || n_edges == 12 ) )
    {
      AF_Edge  edge1, edge2, edge3;
      FT_Pos   dist1, dist2, span, delta;


      if ( n_edges == 6 )
      {
        edge1 = edges;
        edge2 = edges + 2;
        edge3 = edges + 4;
      }
      else
      {
        edge1 = edges + 1;
        edge2 = edges + 5;
        edge3 = edges + 9;
      }

      dist1 = edge2->opos - edge1->opos;
      dist2 = edge3->opos - edge2->opos;

      span = dist1 - dist2;
      if ( span < 0 )
        span = -span;

      if ( span < 8 )
      {
        delta = edge3->pos - ( 2 * edge2->pos - edge1->pos );
        edge3->pos -= delta;
        if ( edge3->link )
          edge3->link->pos -= delta;

        /* move the serifs along with the stem */
        if ( n_edges == 12 )
        {
          ( edges + 8 )->pos -= delta;
          ( edges + 11 )->pos -= delta;
        }

        edge3->flags |= AF_EDGE_DONE;
        if ( edge3->link )
          edge3->link->flags |= AF_EDGE_DONE;
      }
    }

    if ( has_serifs || !anchor )
    {
      /*
       *  now hint the remaining edges (serifs and single) in order
       *  to complete our processing
       */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        if ( edge->flags & AF_EDGE_DONE )
          continue;

        if ( edge->serif )
          af_latin_align_serif_edge( hints, edge->serif, edge );
        else if ( !anchor )
        {
          edge->pos = FT_PIX_ROUND( edge->opos );
          anchor    = edge;
        }
        else
          edge->pos = anchor->pos +
                      FT_PIX_ROUND( edge->opos - anchor->opos );

        edge->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;

        if ( edge + 1 < edge_limit        &&
             edge[1].flags & AF_EDGE_DONE &&
             edge->pos > edge[1].pos      )
          edge->pos = edge[1].pos;
      }
    }
  }


  static FT_Error
  af_latin_hints_apply( AF_GlyphHints    hints,
                        FT_Outline*      outline,
                        AF_LatinMetrics  metrics )
  {
    FT_Error  error;
    int       dim;


    error = af_glyph_hints_reload( hints, outline );
    if ( error )
      goto Exit;

    /* analyze glyph outline */
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;

      af_latin_hints_compute_blue_edges( hints, metrics );
    }

    /* grid-fit the outline */
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {
#ifdef AF_USE_WARPER
        if ( dim == AF_DIMENSION_HORZ &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_NORMAL )
        {
          AF_WarperRec  warper;
          FT_Fixed      scale;
          FT_Pos        delta;


          af_warper_compute( &warper, hints, dim, &scale, &delta );
          af_glyph_hints_scale_dim( hints, dim, scale, delta );
          continue;
        }
#endif
        af_latin_hint_edges( hints, (AF_Dimension)dim );
        af_glyph_hints_align_edge_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_strong_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_weak_points( hints, (AF_Dimension)dim );
      }
    }
    af_glyph_hints_save( hints, outline );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              L A T I N   S C R I P T   C L A S S              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static const AF_Script_UniRangeRec  af_latin_uniranges[] =
  {
    { 32,  127 },    /* XXX: TODO: Add new Unicode ranges here! */
    { 160, 255 },
    { 0,   0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_latin_script_class =
  {
    AF_SCRIPT_LATIN,
    af_latin_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_latin_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_latin_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_latin_hints_init,
    (AF_Script_ApplyHintsFunc)  af_latin_hints_apply
  };


/* END */

/***************************************************************************/
/*                                                                         */
/*  afcjk.c                                                                */
/*                                                                         */
/*    Auto-fitter hinting routines for CJK script (body).                  */
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

  /*
   *  The algorithm is based on akito's autohint patch, available here:
   *
   *  http://www.kde.gr.jp/~akito/patch/freetype2/
   *
   */

#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_CJK

#include "afcjk.h"
#include "aferrors.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              C J K   G L O B A L   M E T R I C S              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  af_cjk_metrics_init( AF_LatinMetrics  metrics,
                       FT_Face          face )
  {
    FT_CharMap  oldmap = face->charmap;


    metrics->units_per_em = face->units_per_EM;

    /* TODO are there blues? */

    if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
      face->charmap = NULL;

    /* latin's version would suffice */
    af_latin_metrics_init_widths( metrics, face, 0x7530 );

    FT_Set_Charmap( face, oldmap );

    return AF_Err_Ok;
  }


  static void
  af_cjk_metrics_scale_dim( AF_LatinMetrics  metrics,
                            AF_Scaler        scaler,
                            AF_Dimension     dim )
  {
    AF_LatinAxis  axis;


    axis = &metrics->axis[dim];

    if ( dim == AF_DIMENSION_HORZ )
    {
      axis->scale = scaler->x_scale;
      axis->delta = scaler->x_delta;
    }
    else
    {
      axis->scale = scaler->y_scale;
      axis->delta = scaler->y_delta;
    }
  }


  static void
  af_cjk_metrics_scale( AF_LatinMetrics  metrics,
                        AF_Scaler        scaler )
  {
    metrics->root.scaler = *scaler;

    af_cjk_metrics_scale_dim( metrics, scaler, AF_DIMENSION_HORZ );
    af_cjk_metrics_scale_dim( metrics, scaler, AF_DIMENSION_VERT );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              C J K   G L Y P H   A N A L Y S I S              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  af_cjk_hints_compute_segments( AF_GlyphHints  hints,
                                 AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    FT_Error      error;
    AF_Segment    seg;


    error = af_latin_hints_compute_segments( hints, dim );
    if ( error )
      return error;

    /* a segment is round if it doesn't have successive */
    /* on-curve points.                                 */
    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Point  pt   = seg->first;
      AF_Point  last = seg->last;
      AF_Flags  f0   = (AF_Flags)(pt->flags & AF_FLAG_CONTROL);
      AF_Flags  f1;


      seg->flags &= ~AF_EDGE_ROUND;

      for ( ; pt != last; f0 = f1 )
      {
        pt = pt->next;
        f1 = (AF_Flags)(pt->flags & AF_FLAG_CONTROL);

        if ( !f0 && !f1 )
          break;

        if ( pt == last )
          seg->flags |= AF_EDGE_ROUND;
      }
    }

    return AF_Err_Ok;
  }


  static void
  af_cjk_hints_link_segments( AF_GlyphHints  hints,
                              AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Direction  major_dir     = axis->major_dir;
    AF_Segment    seg1, seg2;
    FT_Pos        len_threshold;
    FT_Pos        dist_threshold;


    len_threshold = AF_LATIN_CONSTANT( hints->metrics, 8 );

    dist_threshold = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                                  : hints->y_scale;
    dist_threshold = FT_DivFix( 64 * 3, dist_threshold );

    /* now compare each segment to the others */
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      /* the fake segments are for metrics hinting only */
      if ( seg1->first == seg1->last )
        continue;

      if ( seg1->dir != major_dir )
        continue;

      for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        if ( seg2 != seg1 && seg1->dir + seg2->dir == 0 )
        {
          FT_Pos  dist = seg2->pos - seg1->pos;


          if ( dist < 0 )
            continue;

          {
            FT_Pos  min = seg1->min_coord;
            FT_Pos  max = seg1->max_coord;
            FT_Pos  len;


            if ( min < seg2->min_coord )
              min = seg2->min_coord;

            if ( max > seg2->max_coord )
              max = seg2->max_coord;

            len = max - min;
            if ( len >= len_threshold )
            {
              if ( dist * 8 < seg1->score * 9                        &&
                   ( dist * 8 < seg1->score * 7 || seg1->len < len ) )
              {
                seg1->score = dist;
                seg1->len   = len;
                seg1->link  = seg2;
              }

              if ( dist * 8 < seg2->score * 9                        &&
                   ( dist * 8 < seg2->score * 7 || seg2->len < len ) )
              {
                seg2->score = dist;
                seg2->len   = len;
                seg2->link  = seg1;
              }
            }
          }
        }
    }

    /*
     *  now compute the `serif' segments
     *
     *  In Hanzi, some strokes are wider on one or both of the ends.
     *  We either identify the stems on the ends as serifs or remove
     *  the linkage, depending on the length of the stems.
     *
     */

    {
      AF_Segment  link1, link2;


      for ( seg1 = segments; seg1 < segment_limit; seg1++ )
      {
        link1 = seg1->link;
        if ( !link1 || link1->link != seg1 || link1->pos <= seg1->pos )
          continue;

        if ( seg1->score >= dist_threshold )
          continue;

        for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        {
          if ( seg2->pos > seg1->pos || seg1 == seg2 )
            continue;

          link2 = seg2->link;
          if ( !link2 || link2->link != seg2 || link2->pos < link1->pos )
            continue;

          if ( seg1->pos == seg2->pos && link1->pos == link2->pos )
            continue;

          if ( seg2->score <= seg1->score || seg1->score * 4 <= seg2->score )
            continue;

          /* seg2 < seg1 < link1 < link2 */

          if ( seg1->len >= seg2->len * 3 )
          {
            AF_Segment  seg;


            for ( seg = segments; seg < segment_limit; seg++ )
            {
              AF_Segment  link = seg->link;


              if ( link == seg2 )
              {
                seg->link  = 0;
                seg->serif = link1;
              }
              else if ( link == link2 )
              {
                seg->link  = 0;
                seg->serif = seg1;
              }
            }
          }
          else
          {
            seg1->link = link1->link = 0;

            break;
          }
        }
      }
    }

    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      seg2 = seg1->link;

      if ( seg2 )
      {
        seg2->num_linked++;
        if ( seg2->link != seg1 )
        {
          seg1->link = 0;

          if ( seg2->score < dist_threshold || seg1->score < seg2->score * 4 )
            seg1->serif = seg2->link;
          else
            seg2->num_linked--;
        }
      }
    }
  }


  static FT_Error
  af_cjk_hints_compute_edges( AF_GlyphHints  hints,
                              AF_Dimension   dim )
  {
    AF_AxisHints  axis   = &hints->axis[dim];
    FT_Error      error  = AF_Err_Ok;
    FT_Memory     memory = hints->memory;
    AF_LatinAxis  laxis  = &((AF_LatinMetrics)hints->metrics)->axis[dim];

    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;

    /*AF_Direction  up_dir;*/
    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;


    axis->num_edges = 0;

    scale = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                         : hints->y_scale;

    /*up_dir = ( dim == AF_DIMENSION_HORZ ) ? AF_DIR_UP
                                          : AF_DIR_RIGHT;*/

    /*********************************************************************/
    /*                                                                   */
    /* We begin by generating a sorted table of edges for the current    */
    /* direction.  To do so, we simply scan each segment and try to find */
    /* an edge in our table that corresponds to its position.            */
    /*                                                                   */
    /* If no edge is found, we create and insert a new edge in the       */
    /* sorted table.  Otherwise, we simply add the segment to the edge's */
    /* list which is then processed in the second step to compute the    */
    /* edge's properties.                                                */
    /*                                                                   */
    /* Note that the edges table is sorted along the segment/edge        */
    /* position.                                                         */
    /*                                                                   */
    /*********************************************************************/

    edge_distance_threshold = FT_MulFix( laxis->edge_distance_threshold,
                                         scale );
    if ( edge_distance_threshold > 64 / 4 )
      edge_distance_threshold = FT_DivFix( 64 / 4, scale );
    else
      edge_distance_threshold = laxis->edge_distance_threshold;

    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Edge  found = 0;
      FT_Pos   best  = 0xFFFFU;
      FT_Int   ee;


      /* look for an edge corresponding to the segment */
      for ( ee = 0; ee < axis->num_edges; ee++ )
      {
        AF_Edge  edge = axis->edges + ee;
        FT_Pos   dist;


        if ( edge->dir != seg->dir )
          continue;

        dist = seg->pos - edge->fpos;
        if ( dist < 0 )
          dist = -dist;

        if ( dist < edge_distance_threshold && dist < best )
        {
          AF_Segment  link = seg->link;


          /* check whether all linked segments of the candidate edge */
          /* can make a single edge.                                 */
          if ( link )
          {
            AF_Segment  seg1 = edge->first;
            AF_Segment  link1;
            FT_Pos      dist2 = 0;


            do
            {
              link1 = seg1->link;
              if ( link1 )
              {
                dist2 = AF_SEGMENT_DIST( link, link1 );
                if ( dist2 >= edge_distance_threshold )
                  break;
              }

            } while ( ( seg1 = seg1->edge_next ) != edge->first );

            if ( dist2 >= edge_distance_threshold )
              continue;
          }

          best  = dist;
          found = edge;
        }
      }

      if ( !found )
      {
        AF_Edge  edge;


        /* insert a new edge in the list and */
        /* sort according to the position    */
        error = af_axis_hints_new_edge( axis, seg->pos, memory, &edge );
        if ( error )
          goto Exit;

        /* add the segment to the new edge's list */
        FT_ZERO( edge );

        edge->first    = seg;
        edge->last     = seg;
        edge->fpos     = seg->pos;
        edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
        seg->edge_next = seg;
        edge->dir      = seg->dir;
      }
      else
      {
        /* if an edge was found, simply add the segment to the edge's */
        /* list                                                       */
        seg->edge_next         = found->first;
        found->last->edge_next = seg;
        found->last            = seg;
      }
    }

    /*********************************************************************/
    /*                                                                   */
    /* Good, we now compute each edge's properties according to segments */
    /* found on its position.  Basically, these are as follows.          */
    /*                                                                   */
    /*  - edge's main direction                                          */
    /*  - stem edge, serif edge or both (which defaults to stem then)    */
    /*  - rounded edge, straight or both (which defaults to straight)    */
    /*  - link for edge                                                  */
    /*                                                                   */
    /*********************************************************************/

    /* first of all, set the `edge' field in each segment -- this is     */
    /* required in order to compute edge links                           */
    /*                                                                   */
    /* Note that removing this loop and setting the `edge' field of each */
    /* segment directly in the code above slows down execution speed for */
    /* some reasons on platforms like the Sun.                           */

    {
      AF_Edge  edges      = axis->edges;
      AF_Edge  edge_limit = edges + axis->num_edges;
      AF_Edge  edge;


      for ( edge = edges; edge < edge_limit; edge++ )
      {
        seg = edge->first;
        if ( seg )
          do
          {
            seg->edge = edge;
            seg       = seg->edge_next;

          } while ( seg != edge->first );
      }

      /* now compute each edge properties */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        FT_Int  is_round    = 0;  /* does it contain round segments?    */
        FT_Int  is_straight = 0;  /* does it contain straight segments? */


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          /* check for roundness of segment */
          if ( seg->flags & AF_EDGE_ROUND )
            is_round++;
          else
            is_straight++;

          /* check for links -- if seg->serif is set, then seg->link must */
          /* be ignored                                                   */
          is_serif = (FT_Bool)( seg->serif && seg->serif->edge != edge );

          if ( seg->link || is_serif )
          {
            AF_Edge     edge2;
            AF_Segment  seg2;


            edge2 = edge->link;
            seg2  = seg->link;

            if ( is_serif )
            {
              seg2  = seg->serif;
              edge2 = edge->serif;
            }

            if ( edge2 )
            {
              FT_Pos  edge_delta;
              FT_Pos  seg_delta;


              edge_delta = edge->fpos - edge2->fpos;
              if ( edge_delta < 0 )
                edge_delta = -edge_delta;

              seg_delta = AF_SEGMENT_DIST( seg, seg2 );

              if ( seg_delta < edge_delta )
                edge2 = seg2->edge;
            }
            else
              edge2 = seg2->edge;

            if ( is_serif )
            {
              edge->serif   = edge2;
              edge2->flags |= AF_EDGE_SERIF;
            }
            else
              edge->link  = edge2;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );

        /* set the round/straight flags */
        edge->flags = AF_EDGE_NORMAL;

        if ( is_round > 0 && is_round >= is_straight )
          edge->flags |= AF_EDGE_ROUND;

        /* get rid of serifs if link is set                 */
        /* XXX: This gets rid of many unpleasant artefacts! */
        /*      Example: the `c' in cour.pfa at size 13     */

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }
    }

  Exit:
    return error;
  }


  static FT_Error
  af_cjk_hints_detect_features( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    FT_Error  error;


    error = af_cjk_hints_compute_segments( hints, dim );
    if ( !error )
    {
      af_cjk_hints_link_segments( hints, dim );

      error = af_cjk_hints_compute_edges( hints, dim );
    }
    return error;
  }


  static FT_Error
  af_cjk_hints_init( AF_GlyphHints    hints,
                     AF_LatinMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    /*
     *  correct x_scale and y_scale when needed, since they may have
     *  been modified af_cjk_scale_dim above
     */
    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    /* compute flags depending on render mode, etc. */
    mode = metrics->root.scaler.render_mode;

#ifdef AF_USE_WARPER
    if ( mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V )
      metrics->root.scaler.render_mode = mode = FT_RENDER_MODE_NORMAL;
#endif

    scaler_flags = hints->scaler_flags;
    other_flags  = 0;

    /*
     *  We snap the width of vertical stems for the monochrome and
     *  horizontal LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD )
      other_flags |= AF_LATIN_HINTS_HORZ_SNAP;

    /*
     *  We snap the width of horizontal stems for the monochrome and
     *  vertical LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD_V )
      other_flags |= AF_LATIN_HINTS_VERT_SNAP;

    /*
     *  We adjust stems to full pixels only if we don't use the `light' mode.
     */
    if ( mode != FT_RENDER_MODE_LIGHT )
      other_flags |= AF_LATIN_HINTS_STEM_ADJUST;

    if ( mode == FT_RENDER_MODE_MONO )
      other_flags |= AF_LATIN_HINTS_MONO;

    scaler_flags |= AF_SCALER_FLAG_NO_ADVANCE;

    hints->scaler_flags = scaler_flags;
    hints->other_flags  = other_flags;

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****          C J K   G L Y P H   G R I D - F I T T I N G          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* snap a given width in scaled coordinates to one of the */
  /* current standard widths                                */

  static FT_Pos
  af_cjk_snap_width( AF_Width  widths,
                     FT_Int    count,
                     FT_Pos    width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;
    FT_Pos  scaled;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n].cur;
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    scaled = FT_PIX_ROUND( reference );

    if ( width >= reference )
    {
      if ( width < scaled + 48 )
        width = reference;
    }
    else
    {
      if ( width > scaled - 48 )
        width = reference;
    }

    return width;
  }


  /* compute the snapped width of a given stem */

  static FT_Pos
  af_cjk_compute_stem_width( AF_GlyphHints  hints,
                             AF_Dimension   dim,
                             FT_Pos         width,
                             AF_Edge_Flags  base_flags,
                             AF_Edge_Flags  stem_flags )
  {
    AF_LatinMetrics  metrics  = (AF_LatinMetrics) hints->metrics;
    AF_LatinAxis     axis     = & metrics->axis[dim];
    FT_Pos           dist     = width;
    FT_Int           sign     = 0;
    FT_Int           vertical = ( dim == AF_DIMENSION_VERT );

    FT_UNUSED( base_flags );
    FT_UNUSED( stem_flags );


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
      return width;

    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( (  vertical && !AF_LATIN_HINTS_DO_VERT_SNAP( hints ) ) ||
         ( !vertical && !AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) ) )
    {
      /* smooth hinting process: very lightly quantize the stem width */

      if ( axis->width_count > 0 )
      {
        if ( FT_ABS( dist - axis->widths[0].cur ) < 40 )
        {
          dist = axis->widths[0].cur;
          if ( dist < 48 )
            dist = 48;

          goto Done_Width;
        }
      }

      if ( dist < 54 )
        dist += ( 54 - dist ) / 2 ;
      else if ( dist < 3 * 64 )
      {
        FT_Pos  delta;


        delta  = dist & 63;
        dist  &= -64;

        if ( delta < 10 )
          dist += delta;
        else if ( delta < 22 )
          dist += 10;
        else if ( delta < 42 )
          dist += delta;
        else if ( delta < 54 )
          dist += 54;
        else
          dist += delta;
      }
    }
    else
    {
      /* strong hinting process: snap the stem width to integer pixels */

      dist = af_cjk_snap_width( axis->widths, axis->width_count, dist );

      if ( vertical )
      {
        /* in the case of vertical hinting, always round */
        /* the stem heights to integer pixels            */

        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        if ( AF_LATIN_HINTS_DO_MONO( hints ) )
        {
          /* monochrome horizontal hinting: snap widths to integer pixels */
          /* with a different threshold                                   */

          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          /* for horizontal anti-aliased hinting, we adopt a more subtle */
          /* approach: we strengthen small stems, round stems whose size */
          /* is between 1 and 2 pixels to an integer, otherwise nothing  */

          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            /* round otherwise to prevent color fringes in LCD mode */
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

  Done_Width:
    if ( sign )
      dist = -dist;

    return dist;
  }


  /* align one stem edge relative to the previous stem edge */

  static void
  af_cjk_align_linked_edge( AF_GlyphHints  hints,
                            AF_Dimension   dim,
                            AF_Edge        base_edge,
                            AF_Edge        stem_edge )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

    FT_Pos  fitted_width = af_cjk_compute_stem_width(
                             hints, dim, dist,
                             (AF_Edge_Flags)base_edge->flags,
                             (AF_Edge_Flags)stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;
  }


  static void
  af_cjk_align_serif_edge( AF_GlyphHints  hints,
                           AF_Edge        base,
                           AF_Edge        serif )
  {
    FT_UNUSED( hints );

    serif->pos = base->pos + ( serif->opos - base->opos );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                    E D G E   H I N T I N G                      ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define AF_LIGHT_MODE_MAX_HORZ_GAP    9
#define AF_LIGHT_MODE_MAX_VERT_GAP   15
#define AF_LIGHT_MODE_MAX_DELTA_ABS  14


  static FT_Pos
  af_hint_normal_stem( AF_GlyphHints  hints,
                       AF_Edge        edge,
                       AF_Edge        edge2,
                       FT_Pos         anchor,
                       AF_Dimension   dim )
  {
    FT_Pos  org_len, cur_len, org_center;
    FT_Pos  cur_pos1, cur_pos2;
    FT_Pos  d_off1, u_off1, d_off2, u_off2, delta;
    FT_Pos  offset;
    FT_Pos  threshold = 64;


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
    {
      if ( ( edge->flags  & AF_EDGE_ROUND ) &&
           ( edge2->flags & AF_EDGE_ROUND ) )
      {
        if ( dim == AF_DIMENSION_VERT )
          threshold = 64 - AF_LIGHT_MODE_MAX_HORZ_GAP;
        else
          threshold = 64 - AF_LIGHT_MODE_MAX_VERT_GAP;
      }
      else
      {
        if ( dim == AF_DIMENSION_VERT )
          threshold = 64 - AF_LIGHT_MODE_MAX_HORZ_GAP / 3;
        else
          threshold = 64 - AF_LIGHT_MODE_MAX_VERT_GAP / 3;
      }
    }

    org_len    = edge2->opos - edge->opos;
    cur_len    = af_cjk_compute_stem_width( hints, dim, org_len,
                                            (AF_Edge_Flags)edge->flags,
                                            (AF_Edge_Flags)edge2->flags );

    org_center = ( edge->opos + edge2->opos ) / 2 + anchor;
    cur_pos1   = org_center - cur_len / 2;
    cur_pos2   = cur_pos1 + cur_len;
    d_off1     = cur_pos1 - FT_PIX_FLOOR( cur_pos1 );
    d_off2     = cur_pos2 - FT_PIX_FLOOR( cur_pos2 );
    u_off1     = 64 - d_off1;
    u_off2     = 64 - d_off2;
    delta      = 0;


    if ( d_off1 == 0 || d_off2 == 0 )
      goto Exit;

    if ( cur_len <= threshold )
    {
      if ( d_off2 < cur_len )
      {
        if ( u_off1 <= d_off2 )
          delta =  u_off1;
        else
          delta = -d_off2;
      }

      goto Exit;
    }

    if ( threshold < 64 )
    {
      if ( d_off1 >= threshold || u_off1 >= threshold ||
           d_off2 >= threshold || u_off2 >= threshold )
        goto Exit;
    }

    offset = cur_len % 64;

    if ( offset < 32 )
    {
      if ( u_off1 <= offset || d_off2 <= offset )
        goto Exit;
    }
    else
      offset = 64 - threshold;

    d_off1 = threshold - u_off1;
    u_off1 = u_off1    - offset;
    u_off2 = threshold - d_off2;
    d_off2 = d_off2    - offset;

    if ( d_off1 <= u_off1 )
      u_off1 = -d_off1;

    if ( d_off2 <= u_off2 )
      u_off2 = -d_off2;

    if ( FT_ABS( u_off1 ) <= FT_ABS( u_off2 ) )
      delta = u_off1;
    else
      delta = u_off2;

  Exit:

#if 1
    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
    {
      if ( delta > AF_LIGHT_MODE_MAX_DELTA_ABS )
        delta = AF_LIGHT_MODE_MAX_DELTA_ABS;
      else if ( delta < -AF_LIGHT_MODE_MAX_DELTA_ABS )
        delta = -AF_LIGHT_MODE_MAX_DELTA_ABS;
    }
#endif

    cur_pos1 += delta;

    if ( edge->opos < edge2->opos )
    {
      edge->pos  = cur_pos1;
      edge2->pos = cur_pos1 + cur_len;
    }
    else
    {
      edge->pos  = cur_pos1 + cur_len;
      edge2->pos = cur_pos1;
    }

    return delta;
  }


  static void
  af_cjk_hint_edges( AF_GlyphHints  hints,
                     AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    FT_Int        n_edges;
    AF_Edge       edge;
    AF_Edge       anchor   = 0;
    FT_Pos        delta    = 0;
    FT_Int        skipped  = 0;


    /* now we align all stem edges. */
    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  edge2;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      /* skip all non-stem edges */
      edge2 = edge->link;
      if ( !edge2 )
      {
        skipped++;
        continue;
      }

      /* now align the stem */

      if ( edge2 < edge )
      {
        af_cjk_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        continue;
      }

      if ( dim != AF_DIMENSION_VERT && !anchor )
      {

#if 0
        if ( fixedpitch )
        {
          AF_Edge     left  = edge;
          AF_Edge     right = edge_limit - 1;
          AF_EdgeRec  left1, left2, right1, right2;
          FT_Pos      target, center1, center2;
          FT_Pos      delta1, delta2, d1, d2;


          while ( right > left && !right->link )
            right--;

          left1  = *left;
          left2  = *left->link;
          right1 = *right->link;
          right2 = *right;

          delta  = ( ( ( hinter->pp2.x + 32 ) & -64 ) - hinter->pp2.x ) / 2;
          target = left->opos + ( right->opos - left->opos ) / 2 + delta - 16;

          delta1  = delta;
          delta1 += af_hint_normal_stem( hints, left, left->link,
                                         delta1, 0 );

          if ( left->link != right )
            af_hint_normal_stem( hints, right->link, right, delta1, 0 );

          center1 = left->pos + ( right->pos - left->pos ) / 2;

          if ( center1 >= target )
            delta2 = delta - 32;
          else
            delta2 = delta + 32;

          delta2 += af_hint_normal_stem( hints, &left1, &left2, delta2, 0 );

          if ( delta1 != delta2 )
          {
            if ( left->link != right )
              af_hint_normal_stem( hints, &right1, &right2, delta2, 0 );

            center2 = left1.pos + ( right2.pos - left1.pos ) / 2;

            d1 = center1 - target;
            d2 = center2 - target;

            if ( FT_ABS( d2 ) < FT_ABS( d1 ) )
            {
              left->pos       = left1.pos;
              left->link->pos = left2.pos;

              if ( left->link != right )
              {
                right->link->pos = right1.pos;
                right->pos       = right2.pos;
              }

              delta1 = delta2;
            }
          }

          delta               = delta1;
          right->link->flags |= AF_EDGE_DONE;
          right->flags       |= AF_EDGE_DONE;
        }
        else

#endif /* 0 */

          delta = af_hint_normal_stem( hints, edge, edge2, 0,
                                       AF_DIMENSION_HORZ );
      }
      else
        af_hint_normal_stem( hints, edge, edge2, delta, dim );

#if 0
      printf( "stem (%d,%d) adjusted (%.1f,%.1f)\n",
               edge - edges, edge2 - edges,
               ( edge->pos - edge->opos ) / 64.0,
               ( edge2->pos - edge2->opos ) / 64.0 );
#endif

      anchor = edge;
      edge->flags  |= AF_EDGE_DONE;
      edge2->flags |= AF_EDGE_DONE;
    }

    /* make sure that lowercase m's maintain their symmetry */

    /* In general, lowercase m's have six vertical edges if they are sans */
    /* serif, or twelve if they are with serifs.  This implementation is  */
    /* based on that assumption, and seems to work very well with most    */
    /* faces.  However, if for a certain face this assumption is not      */
    /* true, the m is just rendered like before.  In addition, any stem   */
    /* correction will only be applied to symmetrical glyphs (even if the */
    /* glyph is not an m), so the potential for unwanted distortion is    */
    /* relatively low.                                                    */

    /* We don't handle horizontal edges since we can't easily assure that */
    /* the third (lowest) stem aligns with the base line; it might end up */
    /* one pixel higher or lower.                                         */

    n_edges = edge_limit - edges;
    if ( dim == AF_DIMENSION_HORZ && ( n_edges == 6 || n_edges == 12 ) )
    {
      AF_Edge  edge1, edge2, edge3;
      FT_Pos   dist1, dist2, span;


      if ( n_edges == 6 )
      {
        edge1 = edges;
        edge2 = edges + 2;
        edge3 = edges + 4;
      }
      else
      {
        edge1 = edges + 1;
        edge2 = edges + 5;
        edge3 = edges + 9;
      }

      dist1 = edge2->opos - edge1->opos;
      dist2 = edge3->opos - edge2->opos;

      span = dist1 - dist2;
      if ( span < 0 )
        span = -span;

      if ( edge1->link == edge1 + 1 &&
           edge2->link == edge2 + 1 &&
           edge3->link == edge3 + 1 && span < 8 )
      {
        delta = edge3->pos - ( 2 * edge2->pos - edge1->pos );
        edge3->pos -= delta;
        if ( edge3->link )
          edge3->link->pos -= delta;

        /* move the serifs along with the stem */
        if ( n_edges == 12 )
        {
          ( edges + 8 )->pos -= delta;
          ( edges + 11 )->pos -= delta;
        }

        edge3->flags |= AF_EDGE_DONE;
        if ( edge3->link )
          edge3->link->flags |= AF_EDGE_DONE;
      }
    }

    if ( !skipped )
      return;

    /*
     *  now hint the remaining edges (serifs and single) in order
     *  to complete our processing
     */
    for ( edge = edges; edge < edge_limit; edge++ )
    {
      if ( edge->flags & AF_EDGE_DONE )
        continue;

      if ( edge->serif )
      {
        af_cjk_align_serif_edge( hints, edge->serif, edge );
        edge->flags |= AF_EDGE_DONE;
        skipped--;
      }
    }

    if ( !skipped )
      return;

    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  before, after;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      before = after = edge;

      while ( --before >= edges )
        if ( before->flags & AF_EDGE_DONE )
          break;

      while ( ++after < edge_limit )
        if ( after->flags & AF_EDGE_DONE )
          break;

      if ( before >= edges || after < edge_limit )
      {
        if ( before < edges )
          af_cjk_align_serif_edge( hints, after, edge );
        else if ( after >= edge_limit )
          af_cjk_align_serif_edge( hints, before, edge );
        else
          edge->pos = before->pos +
            FT_MulDiv( edge->fpos - before->fpos,
                       after->pos - before->pos,
                       after->fpos - before->fpos );
      }
    }
  }


  static void
  af_cjk_align_edge_points( AF_GlyphHints  hints,
                            AF_Dimension   dim )
  {
    AF_AxisHints  axis       = & hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    AF_Edge       edge;
    FT_Bool       snapping;


    snapping = FT_BOOL( ( dim == AF_DIMENSION_HORZ             &&
                          AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) )  ||
                        ( dim == AF_DIMENSION_VERT             &&
                          AF_LATIN_HINTS_DO_VERT_SNAP( hints ) )  );

    for ( edge = edges; edge < edge_limit; edge++ )
    {
      /* move the points of each segment     */
      /* in each edge to the edge's position */
      AF_Segment  seg = edge->first;


      if ( snapping )
      {
        do
        {
          AF_Point  point = seg->first;


          for (;;)
          {
            if ( dim == AF_DIMENSION_HORZ )
            {
              point->x      = edge->pos;
              point->flags |= AF_FLAG_TOUCH_X;
            }
            else
            {
              point->y      = edge->pos;
              point->flags |= AF_FLAG_TOUCH_Y;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }
      else
      {
        FT_Pos  delta = edge->pos - edge->opos;


        do
        {
          AF_Point  point = seg->first;


          for (;;)
          {
            if ( dim == AF_DIMENSION_HORZ )
            {
              point->x     += delta;
              point->flags |= AF_FLAG_TOUCH_X;
            }
            else
            {
              point->y     += delta;
              point->flags |= AF_FLAG_TOUCH_Y;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }
    }
  }


  static FT_Error
  af_cjk_hints_apply( AF_GlyphHints    hints,
                      FT_Outline*      outline,
                      AF_LatinMetrics  metrics )
  {
    FT_Error  error;
    int       dim;

    FT_UNUSED( metrics );


    error = af_glyph_hints_reload( hints, outline );
    if ( error )
      goto Exit;

    /* analyze glyph outline */
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;
    }

    /* grid-fit the outline */
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {

#ifdef AF_USE_WARPER
        if ( dim == AF_DIMENSION_HORZ                                  &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_NORMAL )
        {
          AF_WarperRec  warper;
          FT_Fixed      scale;
          FT_Pos        delta;


          af_warper_compute( &warper, hints, dim, &scale, &delta );
          af_glyph_hints_scale_dim( hints, dim, scale, delta );
          continue;
        }
#endif /* AF_USE_WARPER */

        af_cjk_hint_edges( hints, (AF_Dimension)dim );
        af_cjk_align_edge_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_strong_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_weak_points( hints, (AF_Dimension)dim );
      }
    }

#if 0
    af_glyph_hints_dump_points( hints );
    af_glyph_hints_dump_segments( hints );
    af_glyph_hints_dump_edges( hints );
#endif

    af_glyph_hints_save( hints, outline );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                C J K   S C R I P T   C L A S S                *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static const AF_Script_UniRangeRec  af_cjk_uniranges[] =
  {
    { 0x0100,  0xFFFF },
    { 0x2E80,  0x2EFF },  /* CJK Radicals Supplement */
    { 0x2F00,  0x2FDF },  /* Kangxi Radicals */
    { 0x3000,  0x303F },  /* CJK Symbols and Punctuation */
    { 0x3040,  0x309F },  /* Hiragana */
    { 0x30A0,  0x30FF },  /* Katakana */
    { 0x3100,  0x312F },  /* Bopomofo */
    { 0x3130,  0x318F },  /* Hangul Compatibility Jamo */
    { 0x31A0,  0x31BF },  /* Bopomofo Extended */
    { 0x31C0,  0x31EF },  /* CJK Strokes */
    { 0x31F0,  0x31FF },  /* Katakana Phonetic Extensions */
    { 0x3200,  0x32FF },  /* Enclosed CJK Letters and Months */
    { 0x3300,  0x33FF },  /* CJK Compatibility */
    { 0x3400,  0x4DBF },  /* CJK Unified Ideographs Extension A */
    { 0x4DC0,  0x4DFF },  /* Yijing Hexagram Symbols */
    { 0x4E00,  0x9FFF },  /* CJK Unified Ideographs */
    { 0xF900,  0xFAFF },  /* CJK Compatibility Ideographs */
    { 0xFE30,  0xFE4F },  /* CJK Compatibility Forms */
    { 0xFF00,  0xFFEF },  /* Halfwidth and Fullwidth Forms */
    { 0x20000, 0x2A6DF }, /* CJK Unified Ideographs Extension B */
    { 0x2F800, 0x2FA1F }, /* CJK Compatibility Ideographs Supplement */
    { 0,       0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_cjk_script_class =
  {
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_cjk_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_cjk_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_cjk_hints_init,
    (AF_Script_ApplyHintsFunc)  af_cjk_hints_apply
  };

#else /* !AF_CONFIG_OPTION_CJK */

  static const AF_Script_UniRangeRec  af_cjk_uniranges[] =
  {
    { 0, 0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_cjk_script_class =
  {
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  };

#endif /* !AF_CONFIG_OPTION_CJK */


/* END */

/***************************************************************************/
/*                                                                         */
/*  afloader.c                                                             */
/*                                                                         */
/*    Auto-fitter glyph loading routines (body).                           */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afloader.h"
#include "afhints.h"
#include "afglobal.h"
#include "aflatin.h"
#include "aferrors.h"


  FT_LOCAL_DEF( FT_Error )
  af_loader_init( AF_Loader  loader,
                  FT_Memory  memory )
  {
    FT_ZERO( loader );

    af_glyph_hints_init( &loader->hints, memory );

    return FT_GlyphLoader_New( memory, &loader->gloader );
  }


  FT_LOCAL_DEF( FT_Error )
  af_loader_reset( AF_Loader  loader,
                   FT_Face    face )
  {
    FT_Error  error = AF_Err_Ok;


    loader->face    = face;
    loader->globals = (AF_FaceGlobals)face->autohint.data;

    FT_GlyphLoader_Rewind( loader->gloader );

    if ( loader->globals == NULL )
    {
      error = af_face_globals_new( face, &loader->globals );
      if ( !error )
      {
        face->autohint.data =
          (FT_Pointer)loader->globals;
        face->autohint.finalizer =
          (FT_Generic_Finalizer)af_face_globals_free;
      }
    }

    return error;
  }


  FT_LOCAL_DEF( void )
  af_loader_done( AF_Loader  loader )
  {
    af_glyph_hints_done( &loader->hints );

    loader->face    = NULL;
    loader->globals = NULL;

    FT_GlyphLoader_Done( loader->gloader );
    loader->gloader = NULL;
  }


  static FT_Error
  af_loader_load_g( AF_Loader  loader,
                    AF_Scaler  scaler,
                    FT_UInt    glyph_index,
                    FT_Int32   load_flags,
                    FT_UInt    depth )
  {
    FT_Error          error;
    FT_Face           face     = loader->face;
    FT_GlyphLoader    gloader  = loader->gloader;
    AF_ScriptMetrics  metrics  = loader->metrics;
    AF_GlyphHints     hints    = &loader->hints;
    FT_GlyphSlot      slot     = face->glyph;
    FT_Slot_Internal  internal = slot->internal;


    error = FT_Load_Glyph( face, glyph_index, load_flags );
    if ( error )
      goto Exit;

    loader->transformed = internal->glyph_transformed;
    if ( loader->transformed )
    {
      FT_Matrix  inverse;


      loader->trans_matrix = internal->glyph_matrix;
      loader->trans_delta  = internal->glyph_delta;

      inverse = loader->trans_matrix;
      FT_Matrix_Invert( &inverse );
      FT_Vector_Transform( &loader->trans_delta, &inverse );
    }

    /* set linear metrics */
    slot->linearHoriAdvance = slot->metrics.horiAdvance;
    slot->linearVertAdvance = slot->metrics.vertAdvance;

    switch ( slot->format )
    {
    case FT_GLYPH_FORMAT_OUTLINE:
      /* translate the loaded glyph when an internal transform is needed */
      if ( loader->transformed )
        FT_Outline_Translate( &slot->outline,
                              loader->trans_delta.x,
                              loader->trans_delta.y );

      /* copy the outline points in the loader's current               */
      /* extra points which is used to keep original glyph coordinates */
      error = FT_GLYPHLOADER_CHECK_POINTS( gloader,
                                           slot->outline.n_points + 4,
                                           slot->outline.n_contours );
      if ( error )
        goto Exit;

      FT_ARRAY_COPY( gloader->current.outline.points,
                     slot->outline.points,
                     slot->outline.n_points );

      FT_ARRAY_COPY( gloader->current.outline.contours,
                     slot->outline.contours,
                     slot->outline.n_contours );

      FT_ARRAY_COPY( gloader->current.outline.tags,
                     slot->outline.tags,
                     slot->outline.n_points );

      gloader->current.outline.n_points   = slot->outline.n_points;
      gloader->current.outline.n_contours = slot->outline.n_contours;

      /* compute original horizontal phantom points (and ignore */
      /* vertical ones)                                         */
      loader->pp1.x = hints->x_delta;
      loader->pp1.y = hints->y_delta;
      loader->pp2.x = FT_MulFix( slot->metrics.horiAdvance,
                                 hints->x_scale ) + hints->x_delta;
      loader->pp2.y = hints->y_delta;

      /* be sure to check for spacing glyphs */
      if ( slot->outline.n_points == 0 )
        goto Hint_Metrics;

      /* now load the slot image into the auto-outline and run the */
      /* automatic hinting process                                 */
      metrics->clazz->script_hints_apply( hints,
                                          &gloader->current.outline,
                                          metrics );

      /* we now need to hint the metrics according to the change in */
      /* width/positioning that occured during the hinting process  */
      {
#ifndef AF_USE_WARPER
        FT_Pos        old_advance, old_rsb, old_lsb, new_lsb;
        FT_Pos        pp1x_uh, pp2x_uh;
        AF_AxisHints  axis  = &hints->axis[AF_DIMENSION_HORZ];
        AF_Edge       edge1 = axis->edges;         /* leftmost edge  */
        AF_Edge       edge2 = edge1 +
                              axis->num_edges - 1; /* rightmost edge */


        if ( axis->num_edges > 1 && AF_HINTS_DO_ADVANCE( hints ) )
        {
          old_advance = loader->pp2.x;
          old_rsb     = old_advance - edge2->opos;
          old_lsb     = edge1->opos;
          new_lsb     = edge1->pos;

          /* remember unhinted values to later account */
          /* for rounding errors                       */

          pp1x_uh = new_lsb    - old_lsb;
          pp2x_uh = edge2->pos + old_rsb;

          /* prefer too much space over too little space */
          /* for very small sizes                        */

          if ( old_lsb < 24 )
            pp1x_uh -= 5;

          if ( old_rsb < 24 )
            pp2x_uh += 5;

          loader->pp1.x = FT_PIX_ROUND( pp1x_uh );
          loader->pp2.x = FT_PIX_ROUND( pp2x_uh );

          slot->lsb_delta = loader->pp1.x - pp1x_uh;
          slot->rsb_delta = loader->pp2.x - pp2x_uh;
        }
        else
#endif /* !AF_USE_WARPER */
        {
          FT_Pos   pp1x = loader->pp1.x;
          FT_Pos   pp2x = loader->pp2.x;

          loader->pp1.x = FT_PIX_ROUND( loader->pp1.x );
          loader->pp2.x = FT_PIX_ROUND( loader->pp2.x );

          slot->lsb_delta = loader->pp1.x - pp1x;
          slot->rsb_delta = loader->pp2.x - pp2x;
        }
      }

      /* good, we simply add the glyph to our loader's base */
      FT_GlyphLoader_Add( gloader );
      break;

    case FT_GLYPH_FORMAT_COMPOSITE:
      {
        FT_UInt      nn, num_subglyphs = slot->num_subglyphs;
        FT_UInt      num_base_subgs, start_point;
        FT_SubGlyph  subglyph;


        start_point = gloader->base.outline.n_points;

        /* first of all, copy the subglyph descriptors in the glyph loader */
        error = FT_GlyphLoader_CheckSubGlyphs( gloader, num_subglyphs );
        if ( error )
          goto Exit;

        FT_ARRAY_COPY( gloader->current.subglyphs,
                       slot->subglyphs,
                       num_subglyphs );

        gloader->current.num_subglyphs = num_subglyphs;
        num_base_subgs                 = gloader->base.num_subglyphs;

        /* now, read each subglyph independently */
        for ( nn = 0; nn < num_subglyphs; nn++ )
        {
          FT_Vector  pp1, pp2;
          FT_Pos     x, y;
          FT_UInt    num_points, num_new_points, num_base_points;


          /* gloader.current.subglyphs can change during glyph loading due */
          /* to re-allocation -- we must recompute the current subglyph on */
          /* each iteration                                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          pp1 = loader->pp1;
          pp2 = loader->pp2;

          num_base_points = gloader->base.outline.n_points;

          error = af_loader_load_g( loader, scaler, subglyph->index,
                                    load_flags, depth + 1 );
          if ( error )
            goto Exit;

          /* recompute subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          if ( subglyph->flags & FT_SUBGLYPH_FLAG_USE_MY_METRICS )
          {
            pp1 = loader->pp1;
            pp2 = loader->pp2;
          }
          else
          {
            loader->pp1 = pp1;
            loader->pp2 = pp2;
          }

          num_points     = gloader->base.outline.n_points;
          num_new_points = num_points - num_base_points;

          /* now perform the transform required for this subglyph */

          if ( subglyph->flags & ( FT_SUBGLYPH_FLAG_SCALE    |
                                   FT_SUBGLYPH_FLAG_XY_SCALE |
                                   FT_SUBGLYPH_FLAG_2X2      ) )
          {
            FT_Vector*  cur   = gloader->base.outline.points +
                                num_base_points;
            FT_Vector*  limit = cur + num_new_points;


            for ( ; cur < limit; cur++ )
              FT_Vector_Transform( cur, &subglyph->transform );
          }

          /* apply offset */

          if ( !( subglyph->flags & FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES ) )
          {
            FT_Int      k = subglyph->arg1;
            FT_UInt     l = subglyph->arg2;
            FT_Vector*  p1;
            FT_Vector*  p2;


            if ( start_point + k >= num_base_points         ||
                               l >= (FT_UInt)num_new_points )
            {
              error = AF_Err_Invalid_Composite;
              goto Exit;
            }

            l += num_base_points;

            /* for now, only use the current point coordinates;    */
            /* we may consider another approach in the near future */
            p1 = gloader->base.outline.points + start_point + k;
            p2 = gloader->base.outline.points + start_point + l;

            x = p1->x - p2->x;
            y = p1->y - p2->y;
          }
          else
          {
            x = FT_MulFix( subglyph->arg1, hints->x_scale ) + hints->x_delta;
            y = FT_MulFix( subglyph->arg2, hints->y_scale ) + hints->y_delta;

            x = FT_PIX_ROUND( x );
            y = FT_PIX_ROUND( y );
          }

          {
            FT_Outline  dummy = gloader->base.outline;


            dummy.points  += num_base_points;
            dummy.n_points = (short)num_new_points;

            FT_Outline_Translate( &dummy, x, y );
          }
        }
      }
      break;

    default:
      /* we don't support other formats (yet?) */
      error = AF_Err_Unimplemented_Feature;
    }

  Hint_Metrics:
    if ( depth == 0 )
    {
      FT_BBox    bbox;
      FT_Vector  vvector;


      vvector.x = slot->metrics.vertBearingX - slot->metrics.horiBearingX;
      vvector.y = slot->metrics.vertBearingY - slot->metrics.horiBearingY;
      vvector.x = FT_MulFix( vvector.x, metrics->scaler.x_scale );
      vvector.y = FT_MulFix( vvector.y, metrics->scaler.y_scale );

      /* transform the hinted outline if needed */
      if ( loader->transformed )
      {
        FT_Outline_Transform( &gloader->base.outline, &loader->trans_matrix );
        FT_Vector_Transform( &vvector, &loader->trans_matrix );
      }

      /* we must translate our final outline by -pp1.x and compute */
      /* the new metrics                                           */
      if ( loader->pp1.x )
        FT_Outline_Translate( &gloader->base.outline, -loader->pp1.x, 0 );

      FT_Outline_Get_CBox( &gloader->base.outline, &bbox );

      bbox.xMin = FT_PIX_FLOOR( bbox.xMin );
      bbox.yMin = FT_PIX_FLOOR( bbox.yMin );
      bbox.xMax = FT_PIX_CEIL(  bbox.xMax );
      bbox.yMax = FT_PIX_CEIL(  bbox.yMax );

      slot->metrics.width        = bbox.xMax - bbox.xMin;
      slot->metrics.height       = bbox.yMax - bbox.yMin;
      slot->metrics.horiBearingX = bbox.xMin;
      slot->metrics.horiBearingY = bbox.yMax;

      slot->metrics.vertBearingX = FT_PIX_FLOOR( bbox.xMin + vvector.x );
      slot->metrics.vertBearingY = FT_PIX_FLOOR( bbox.yMax + vvector.y );

      /* for mono-width fonts (like Andale, Courier, etc.) we need */
      /* to keep the original rounded advance width                */
#if 0
      if ( !FT_IS_FIXED_WIDTH( slot->face ) )
        slot->metrics.horiAdvance = loader->pp2.x - loader->pp1.x;
      else
        slot->metrics.horiAdvance = FT_MulFix( slot->metrics.horiAdvance,
                                               x_scale );
#else
      if ( !FT_IS_FIXED_WIDTH( slot->face ) )
        slot->metrics.horiAdvance = loader->pp2.x - loader->pp1.x;
      else
        slot->metrics.horiAdvance = FT_MulFix( slot->metrics.horiAdvance,
                                               metrics->scaler.x_scale );
#endif

      slot->metrics.vertAdvance = FT_MulFix( slot->metrics.vertAdvance,
                                              metrics->scaler.y_scale );

      slot->metrics.horiAdvance = FT_PIX_ROUND( slot->metrics.horiAdvance );
      slot->metrics.vertAdvance = FT_PIX_ROUND( slot->metrics.vertAdvance );

      /* now copy outline into glyph slot */
      FT_GlyphLoader_Rewind( internal->loader );
      error = FT_GlyphLoader_CopyPoints( internal->loader, gloader );
      if ( error )
        goto Exit;

      slot->outline = internal->loader->base.outline;
      slot->format  = FT_GLYPH_FORMAT_OUTLINE;
    }

#ifdef DEBUG_HINTER
    af_debug_hinter = hinter;
#endif

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_loader_load_glyph( AF_Loader  loader,
                        FT_Face    face,
                        FT_UInt    gindex,
                        FT_UInt32  load_flags )
  {
    FT_Error      error;
    FT_Size       size = face->size;
    AF_ScalerRec  scaler;


    if ( !size )
      return AF_Err_Invalid_Argument;

    FT_ZERO( &scaler );

    scaler.face    = face;
    scaler.x_scale = size->metrics.x_scale;
    scaler.x_delta = 0;  /* XXX: TODO: add support for sub-pixel hinting */
    scaler.y_scale = size->metrics.y_scale;
    scaler.y_delta = 0;  /* XXX: TODO: add support for sub-pixel hinting */

    scaler.render_mode = FT_LOAD_TARGET_MODE( load_flags );
    scaler.flags       = 0;  /* XXX: fix this */

    error = af_loader_reset( loader, face );
    if ( !error )
    {
      AF_ScriptMetrics  metrics;


      error = af_face_globals_get_metrics( loader->globals, gindex,
                                           &metrics );
      if ( !error )
      {
        loader->metrics = metrics;

        if ( metrics->clazz->script_metrics_scale )
          metrics->clazz->script_metrics_scale( metrics, &scaler );
        else
          metrics->scaler = scaler;

        load_flags |=  FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_TRANSFORM;
        load_flags &= ~FT_LOAD_RENDER;

        error = metrics->clazz->script_hints_init( &loader->hints, metrics );
        if ( error )
          goto Exit;

        error = af_loader_load_g( loader, &scaler, gindex, load_flags, 0 );
      }
    }
  Exit:
    return error;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  afmodule.c                                                             */
/*                                                                         */
/*    Auto-fitter module implementation (body).                            */
/*                                                                         */
/*  Copyright 2003, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afmodule.h"
#include "afloader.h"

#include FT_INTERNAL_OBJECTS_H


  typedef struct  FT_AutofitterRec_
  {
    FT_ModuleRec  root;
    AF_LoaderRec  loader[1];

  } FT_AutofitterRec, *FT_Autofitter;


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_init( FT_Autofitter  module )
  {
    return af_loader_init( module->loader, module->root.library->memory );
  }


  FT_CALLBACK_DEF( void )
  af_autofitter_done( FT_Autofitter  module )
  {
    af_loader_done( module->loader );
  }


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_load_glyph( FT_Autofitter  module,
                            FT_GlyphSlot   slot,
                            FT_Size        size,
                            FT_UInt        glyph_index,
                            FT_Int32       load_flags )
  {
    FT_UNUSED( size );

    return af_loader_load_glyph( module->loader, slot->face,
                                 glyph_index, load_flags );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_AutoHinter_ServiceRec  af_autofitter_service =
  {
    NULL,
    NULL,
    NULL,
    (FT_AutoHinter_GlyphLoadFunc)af_autofitter_load_glyph
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  autofit_module_class =
  {
    FT_MODULE_HINTER,
    sizeof ( FT_AutofitterRec ),

    "autofitter",
    0x10000L,   /* version 1.0 of the autofitter  */
    0x20000L,   /* requires FreeType 2.0 or above */

    (const void*)&af_autofitter_service,

    (FT_Module_Constructor)af_autofitter_init,
    (FT_Module_Destructor) af_autofitter_done,
    (FT_Module_Requester)  NULL
  };


/* END */


#ifdef AF_USE_WARPER
/***************************************************************************/
/*                                                                         */
/*  afwarp.c                                                               */
/*                                                                         */
/*    Auto-fitter warping algorithm (body).                                */
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


#include "afwarp.h"

#ifdef AF_USE_WARPER

#if 1
  static const AF_WarpScore
  af_warper_weights[64] =
  {
    35, 20, 20, 20, 15, 12, 10,  5,  2,  1,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0, -1, -2, -5, -8,-10,-10,-20,-20,-30,-30,

   -30,-30,-20,-20,-10,-10, -8, -5, -2, -1,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  1,  2,  5, 10, 12, 15, 20, 20, 20,
  };
#else
  static const AF_WarpScore
  af_warper_weights[64] =
  {
    30, 20, 10,  5,  4,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0, -1, -2, -2, -5, -5,-10,-10,-15,-20,

   -20,-15,-15,-10,-10, -5, -5, -2, -2, -1,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  4,  5, 10, 20,
  };
#endif


  static void
  af_warper_compute_line_best( AF_Warper     warper,
                               FT_Fixed      scale,
                               FT_Pos        delta,
                               FT_Pos        xx1,
                               FT_Pos        xx2,
                               AF_WarpScore  base_distort,
                               AF_Segment    segments,
                               FT_UInt       num_segments )
  {
    FT_Int        idx_min, idx_max, idx0;
    FT_UInt       nn;
    AF_WarpScore  scores[64];


    for ( nn = 0; nn < 64; nn++ )
      scores[nn] = 0;

    idx0 = xx1 - warper->t1;

    /* compute minimum and maximum indices */
    {
      FT_Pos  xx1min = warper->x1min;
      FT_Pos  xx1max = warper->x1max;
      FT_Pos  w      = xx2 - xx1;


      if ( xx1min + w < warper->x2min )
        xx1min = warper->x2min - ( xx2 - xx1 );

      xx1max = warper->x1max;
      if ( xx1max + w > warper->x2max )
        xx1max = warper->x2max - ( xx2 - xx1 );

      idx_min = xx1min - warper->t1;
      idx_max = xx1max - warper->t1;

      if ( idx_min > idx_max )
      {
        AF_LOG(( "invalid indices:\n"
                 "  min=%d max=%d, xx1=%ld xx2=%ld,\n"
                 "  x1min=%ld x1max=%ld, x2min=%ld x2max=%ld\n",
                 idx_min, idx_max, xx1, xx2,
                 warper->x1min, warper->x1max,
                 warper->x2min, warper->x2max ));
        return;
      }
    }

    for ( nn = 0; nn < num_segments; nn++ )
    {
      FT_Pos  len = segments[nn].max_coord - segments[nn].min_coord;
      FT_Pos  y0  = FT_MulFix( segments[nn].pos, scale ) + delta;
      FT_Pos  y   = y0 + ( idx_min - idx0 );

      FT_Int  idx;


      for ( idx = idx_min; idx <= idx_max; idx++, y++ )
        scores[idx] += af_warper_weights[y & 63] * len;
    }

    /* find best score */
    {
      FT_Int  idx;


      for ( idx = idx_min; idx <= idx_max; idx++ )
      {
        AF_WarpScore  score = scores[idx];
        AF_WarpScore  distort = base_distort + ( idx - idx0 );


        if ( score > warper->best_score           ||
             ( score == warper->best_score    &&
               distort < warper->best_distort )   )
        {
          warper->best_score   = score;
          warper->best_distort = distort;
          warper->best_scale   = scale;
          warper->best_delta   = delta + ( idx - idx0 );
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_warper_compute( AF_Warper      warper,
                     AF_GlyphHints  hints,
                     AF_Dimension   dim,
                     FT_Fixed      *a_scale,
                     FT_Pos        *a_delta )
  {
    AF_AxisHints  axis;
    AF_Point      points;

    FT_Fixed      org_scale;
    FT_Pos        org_delta;

    FT_UInt       nn, num_points, num_segments;
    FT_Int        X1, X2;
    FT_Int        w;

    AF_WarpScore  base_distort;
    AF_Segment    segments;


    /* get original scaling transformation */
    if ( dim == AF_DIMENSION_VERT )
    {
      org_scale = hints->y_scale;
      org_delta = hints->y_delta;
    }
    else
    {
      org_scale = hints->x_scale;
      org_delta = hints->x_delta;
    }

    warper->best_scale   = org_scale;
    warper->best_delta   = org_delta;
    warper->best_score   = 0;
    warper->best_distort = 0;

    axis         = &hints->axis[dim];
    segments     = axis->segments;
    num_segments = axis->num_segments;
    points       = hints->points;
    num_points   = hints->num_points;

    *a_scale = org_scale;
    *a_delta = org_delta;

    /* get X1 and X2, minimum and maximum in original coordinates */
    if ( axis->num_segments < 1 )
      return;

#if 1
    X1 = X2 = points[0].fx;
    for ( nn = 1; nn < num_points; nn++ )
    {
      FT_Int  X = points[nn].fx;


      if ( X < X1 )
        X1 = X;
      if ( X > X2 )
        X2 = X;
    }
#else
    X1 = X2 = segments[0].pos;
    for ( nn = 1; nn < num_segments; nn++ )
    {
      FT_Int  X = segments[nn].pos;


      if ( X < X1 )
        X1 = X;
      if ( X > X2 )
        X2 = X;
    }
#endif

    if ( X1 >= X2 )
      return;

    warper->x1 = FT_MulFix( X1, org_scale ) + org_delta;
    warper->x2 = FT_MulFix( X2, org_scale ) + org_delta;

    warper->t1 = AF_WARPER_FLOOR( warper->x1 );
    warper->t2 = AF_WARPER_CEIL( warper->x2 );

    warper->x1min = warper->x1 & ~31;
    warper->x1max = warper->x1min + 32;
    warper->x2min = warper->x2 & ~31;
    warper->x2max = warper->x2min + 32;

    if ( warper->x1max > warper->x2 )
      warper->x1max = warper->x2;

    if ( warper->x2min < warper->x1 )
      warper->x2min = warper->x1;

    warper->w0 = warper->x2 - warper->x1;

    if ( warper->w0 <= 64 )
    {
      warper->x1max = warper->x1;
      warper->x2min = warper->x2;
    }

    warper->wmin = warper->x2min - warper->x1max;
    warper->wmax = warper->x2max - warper->x1min;

    if ( warper->wmin < warper->w0 - 32 )
      warper->wmin = warper->w0 - 32;

    if ( warper->wmax > warper->w0 + 32 )
      warper->wmax = warper->w0 + 32;

    if ( warper->wmin < warper->w0 * 3 / 4 )
      warper->wmin = warper->w0 * 3 / 4;

    if ( warper->wmax > warper->w0 * 5 / 4 )
      warper->wmax = warper->w0 * 5 / 4;

    /* warper->wmin = warper->wmax = warper->w0; */

    for ( w = warper->wmin; w <= warper->wmax; w++ )
    {
      FT_Fixed  new_scale;
      FT_Pos    new_delta;
      FT_Pos    xx1, xx2;


      xx1 = warper->x1;
      xx2 = warper->x2;
      if ( w >= warper->w0 )
      {
        xx1 -= w - warper->w0;
        if ( xx1 < warper->x1min )
        {
          xx2 += warper->x1min - xx1;
          xx1  = warper->x1min;
        }
      }
      else
      {
        xx1 -= w - warper->w0;
        if ( xx1 > warper->x1max )
        {
          xx2 -= xx1 - warper->x1max;
          xx1  = warper->x1max;
        }
      }

      if ( xx1 < warper->x1 )
        base_distort = warper->x1 - xx1;
      else
        base_distort = xx1 - warper->x1;

      if ( xx2 < warper->x2 )
        base_distort += warper->x2 - xx2;
      else
        base_distort += xx2 - warper->x2;

      base_distort *= 10;

      new_scale = org_scale + FT_DivFix( w - warper->w0, X2 - X1 );
      new_delta = xx1 - FT_MulFix( X1, new_scale );

      af_warper_compute_line_best( warper, new_scale, new_delta, xx1, xx2,
                                   base_distort,
                                   segments, num_segments );
    }

    *a_scale = warper->best_scale;
    *a_delta = warper->best_delta;
  }

#else /* !AF_USE_WARPER */

char  af_warper_dummy = 0;  /* make compiler happy */

#endif /* !AF_USE_WARPER */

/* END */

#endif

/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcache.c                                                              */
/*                                                                         */
/*    The FreeType Caching sub-system (body only).                         */
/*                                                                         */
/*  Copyright 2000-2001, 2003 by                                           */
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
/*  ftcmru.c                                                               */
/*                                                                         */
/*    FreeType MRU support (body).                                         */
/*                                                                         */
/*  Copyright 2003, 2004, 2006 by                                          */
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
#include FT_CACHE_H
#include "ftcmru.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftcerror.h"


  FT_LOCAL_DEF( void )
  FTC_MruNode_Prepend( FTC_MruNode  *plist,
                       FTC_MruNode   node )
  {
    FTC_MruNode  first = *plist;


    if ( first )
    {
      FTC_MruNode  last = first->prev;


#ifdef FT_DEBUG_ERROR
      {
        FTC_MruNode  cnode = first;


        do
        {
          if ( cnode == node )
          {
            fprintf( stderr, "FTC_MruNode_Prepend: invalid action!\n" );
            exit( 2 );
          }
          cnode = cnode->next;

        } while ( cnode != first );
      }
#endif

      first->prev = node;
      last->next  = node;
      node->next  = first;
      node->prev  = last;
    }
    else
    {
      node->next = node;
      node->prev = node;
    }
    *plist = node;
  }


  FT_LOCAL_DEF( void )
  FTC_MruNode_Up( FTC_MruNode  *plist,
                  FTC_MruNode   node )
  {
    FTC_MruNode  first = *plist;


    FT_ASSERT( first != NULL );

    if ( first != node )
    {
      FTC_MruNode  prev, next, last;


#ifdef FT_DEBUG_ERROR
      {
        FTC_MruNode  cnode = first;
        do
        {
          if ( cnode == node )
            goto Ok;
          cnode = cnode->next;

        } while ( cnode != first );

        fprintf( stderr, "FTC_MruNode_Up: invalid action!\n" );
        exit( 2 );
      Ok:
      }
#endif
      prev = node->prev;
      next = node->next;

      prev->next = next;
      next->prev = prev;

      last = first->prev;

      last->next  = node;
      first->prev = node;

      node->next = first;
      node->prev = last;

      *plist = node;
    }
  }


  FT_LOCAL_DEF( void )
  FTC_MruNode_Remove( FTC_MruNode  *plist,
                      FTC_MruNode   node )
  {
    FTC_MruNode  first = *plist;
    FTC_MruNode  prev, next;


    FT_ASSERT( first != NULL );

#ifdef FT_DEBUG_ERROR
      {
        FTC_MruNode  cnode = first;


        do
        {
          if ( cnode == node )
            goto Ok;
          cnode = cnode->next;

        } while ( cnode != first );

        fprintf( stderr, "FTC_MruNode_Remove: invalid action!\n" );
        exit( 2 );
      Ok:
      }
#endif

    prev = node->prev;
    next = node->next;

    prev->next = next;
    next->prev = prev;

    if ( node == next )
    {
      FT_ASSERT( first == node );
      FT_ASSERT( prev  == node );

      *plist = NULL;
    }
    else if ( node == first )
        *plist = next;
  }


  FT_LOCAL_DEF( void )
  FTC_MruList_Init( FTC_MruList       list,
                    FTC_MruListClass  clazz,
                    FT_UInt           max_nodes,
                    FT_Pointer        data,
                    FT_Memory         memory )
  {
    list->num_nodes = 0;
    list->max_nodes = max_nodes;
    list->nodes     = NULL;
    list->clazz     = *clazz;
    list->data      = data;
    list->memory    = memory;
  }


  FT_LOCAL_DEF( void )
  FTC_MruList_Reset( FTC_MruList  list )
  {
    while ( list->nodes )
      FTC_MruList_Remove( list, list->nodes );

    FT_ASSERT( list->num_nodes == 0 );
  }


  FT_LOCAL_DEF( void )
  FTC_MruList_Done( FTC_MruList  list )
  {
    FTC_MruList_Reset( list );
  }


#ifndef FTC_INLINE
  FT_LOCAL_DEF( FTC_MruNode )
  FTC_MruList_Find( FTC_MruList  list,
                    FT_Pointer   key )
  {
    FTC_MruNode_CompareFunc  compare = list->clazz.node_compare;
    FTC_MruNode              first, node;


    first = list->nodes;
    node  = NULL;

    if ( first )
    {
      node = first;
      do
      {
        if ( compare( node, key ) )
        {
          if ( node != first )
            FTC_MruNode_Up( &list->nodes, node );

          return node;
        }

        node = node->next;

      } while ( node != first);
    }

    return NULL;
  }
#endif

  FT_LOCAL_DEF( FT_Error )
  FTC_MruList_New( FTC_MruList   list,
                   FT_Pointer    key,
                   FTC_MruNode  *anode )
  {
    FT_Error     error;
    FTC_MruNode  node;
    FT_Memory    memory = list->memory;


    if ( list->num_nodes >= list->max_nodes && list->max_nodes > 0 )
    {
      node = list->nodes->prev;

      FT_ASSERT( node );

      if ( list->clazz.node_reset )
      {
        FTC_MruNode_Up( &list->nodes, node );

        error = list->clazz.node_reset( node, key, list->data );
        if ( !error )
          goto Exit;
      }

      FTC_MruNode_Remove( &list->nodes, node );
      list->num_nodes--;

      if ( list->clazz.node_done )
        list->clazz.node_done( node, list->data );
    }
    else if ( FT_ALLOC( node, list->clazz.node_size ) )
        goto Exit;

    error = list->clazz.node_init( node, key, list->data );
    if ( error )
      goto Fail;

      FTC_MruNode_Prepend( &list->nodes, node );
      list->num_nodes++;

  Exit:
    *anode = node;
    return error;

  Fail:
    if ( list->clazz.node_done )
      list->clazz.node_done( node, list->data );

    FT_FREE( node );
    goto Exit;
  }


#ifndef FTC_INLINE
  FT_LOCAL_DEF( FT_Error )
  FTC_MruList_Lookup( FTC_MruList   list,
                      FT_Pointer    key,
                      FTC_MruNode  *anode )
  {
    FTC_MruNode  node;


    node = FTC_MruList_Find( list, key );
    if ( node == NULL )
      return FTC_MruList_New( list, key, anode );

    *anode = node;
    return 0;
  }
#endif /* FTC_INLINE */

  FT_LOCAL_DEF( void )
  FTC_MruList_Remove( FTC_MruList  list,
                      FTC_MruNode  node )
  {
    FTC_MruNode_Remove( &list->nodes, node );
    list->num_nodes--;

    {
      FT_Memory  memory = list->memory;


      if ( list->clazz.node_done )
       list->clazz.node_done( node, list->data );

      FT_FREE( node );
    }
  }


  FT_LOCAL_DEF( void )
  FTC_MruList_RemoveSelection( FTC_MruList              list,
                               FTC_MruNode_CompareFunc  selection,
                               FT_Pointer               key )
  {
    FTC_MruNode  first, node, next;


    first = list->nodes;
    while ( first && ( selection == NULL || selection( first, key ) ) )
    {
      FTC_MruList_Remove( list, first );
      first = list->nodes;
    }

    if ( first )
    {
      node = first->next;
      while ( node != first )
      {
        next = node->next;

        if ( selection( node, key ) )
          FTC_MruList_Remove( list, node );

        node = next;
      }
    }
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcmanag.c                                                             */
/*                                                                         */
/*    FreeType Cache Manager (body).                                       */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include FT_CACHE_H
#include "ftcmanag.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_SIZES_H

#include "ftccback.h"
#include "ftcerror.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache

#define FTC_LRU_GET_MANAGER( lru )  ( (FTC_Manager)(lru)->user_data )


  static FT_Error
  ftc_scaler_lookup_size( FTC_Manager  manager,
                          FTC_Scaler   scaler,
                          FT_Size     *asize )
  {
    FT_Face   face;
    FT_Size   size = NULL;
    FT_Error  error;


    error = FTC_Manager_LookupFace( manager, scaler->face_id, &face );
    if ( error )
      goto Exit;

    error = FT_New_Size( face, &size );
    if ( error )
      goto Exit;

    FT_Activate_Size( size );

    if ( scaler->pixel )
      error = FT_Set_Pixel_Sizes( face, scaler->width, scaler->height );
    else
      error = FT_Set_Char_Size( face, scaler->width, scaler->height,
                                scaler->x_res, scaler->y_res );
    if ( error )
    {
      FT_Done_Size( size );
      size = NULL;
    }

  Exit:
    *asize = size;
    return error;
  }


  typedef struct  FTC_SizeNodeRec_
  {
    FTC_MruNodeRec  node;
    FT_Size         size;
    FTC_ScalerRec   scaler;

  } FTC_SizeNodeRec, *FTC_SizeNode;


  FT_CALLBACK_DEF( void )
  ftc_size_node_done( FTC_MruNode  ftcnode,
                      FT_Pointer   data )
  {
    FTC_SizeNode  node = (FTC_SizeNode)ftcnode;
    FT_Size       size = node->size;
    FT_UNUSED( data );


    if ( size )
      FT_Done_Size( size );
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_size_node_compare( FTC_MruNode  ftcnode,
                         FT_Pointer   ftcscaler )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Scaler    scaler0 = &node->scaler;


    if ( FTC_SCALER_COMPARE( scaler0, scaler ) )
    {
      FT_Activate_Size( node->size );
      return 1;
    }
    return 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_init( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcscaler,
                      FT_Pointer   ftcmanager )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    node->scaler = scaler[0];

    return ftc_scaler_lookup_size( manager, scaler, &node->size );
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_reset( FTC_MruNode  ftcnode,
                       FT_Pointer   ftcscaler,
                       FT_Pointer   ftcmanager )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    FT_Done_Size( node->size );

    node->scaler = scaler[0];

    return ftc_scaler_lookup_size( manager, scaler, &node->size );
  }


  FT_CALLBACK_TABLE_DEF
  const FTC_MruListClassRec  ftc_size_list_class =
  {
    sizeof ( FTC_SizeNodeRec ),
    ftc_size_node_compare,
    ftc_size_node_init,
    ftc_size_node_reset,
    ftc_size_node_done
  };


  /* helper function used by ftc_face_node_done */
  static FT_Bool
  ftc_size_node_compare_faceid( FTC_MruNode  ftcnode,
                                FT_Pointer   ftcface_id )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;


    return FT_BOOL( node->scaler.face_id == face_id );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_LookupSize( FTC_Manager  manager,
                          FTC_Scaler   scaler,
                          FT_Size     *asize )
  {
    FT_Error      error;
    FTC_SizeNode  node;


    if ( asize == NULL )
      return FTC_Err_Bad_Argument;

    *asize = NULL;

    if ( !manager )
      return FTC_Err_Invalid_Cache_Handle;

#ifdef FTC_INLINE

    FTC_MRULIST_LOOKUP_CMP( &manager->sizes, scaler, ftc_size_node_compare,
                            node, error );

#else
    error = FTC_MruList_Lookup( &manager->sizes, scaler, (FTC_MruNode*)&node );
#endif

    if ( !error )
      *asize = node->size;

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    FACE MRU IMPLEMENTATION                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  FTC_FaceNodeRec_
  {
    FTC_MruNodeRec  node;
    FTC_FaceID      face_id;
    FT_Face         face;

  } FTC_FaceNodeRec, *FTC_FaceNode;


  FT_CALLBACK_DEF( FT_Error )
  ftc_face_node_init( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcface_id,
                      FT_Pointer   ftcmanager )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;
    FT_Error      error;


    node->face_id = face_id;

    error = manager->request_face( face_id,
                                   manager->library,
                                   manager->request_data,
                                   &node->face );
    if ( !error )
    {
      /* destroy initial size object; it will be re-created later */
      if ( node->face->size )
        FT_Done_Size( node->face->size );
    }

    return error;
  }


  FT_CALLBACK_DEF( void )
  ftc_face_node_done( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcmanager )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    /* we must begin by removing all scalers for the target face */
    /* from the manager's list                                   */
    FTC_MruList_RemoveSelection( &manager->sizes,
                                 ftc_size_node_compare_faceid,
                                 node->face_id );

    /* all right, we can discard the face now */
    FT_Done_Face( node->face );
    node->face    = NULL;
    node->face_id = NULL;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_face_node_compare( FTC_MruNode  ftcnode,
                         FT_Pointer   ftcface_id )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;


    return FT_BOOL( node->face_id == face_id );
  }


  FT_CALLBACK_TABLE_DEF
  const FTC_MruListClassRec  ftc_face_list_class =
  {
    sizeof ( FTC_FaceNodeRec),

    ftc_face_node_compare,
    ftc_face_node_init,
    0,                          /* FTC_MruNode_ResetFunc */
    ftc_face_node_done
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_LookupFace( FTC_Manager  manager,
                          FTC_FaceID   face_id,
                          FT_Face     *aface )
  {
    FT_Error      error;
    FTC_FaceNode  node;


    if ( aface == NULL )
      return FTC_Err_Bad_Argument;

    *aface = NULL;

    if ( !manager )
      return FTC_Err_Invalid_Cache_Handle;

    /* we break encapsulation for the sake of speed */
#ifdef FTC_INLINE

    FTC_MRULIST_LOOKUP_CMP( &manager->faces, face_id, ftc_face_node_compare,
                            node, error );

#else
    error = FTC_MruList_Lookup( &manager->faces, face_id, (FTC_MruNode*)&node );
#endif

    if ( !error )
      *aface = node->face;

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    CACHE MANAGER ROUTINES                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_New( FT_Library          library,
                   FT_UInt             max_faces,
                   FT_UInt             max_sizes,
                   FT_ULong            max_bytes,
                   FTC_Face_Requester  requester,
                   FT_Pointer          req_data,
                   FTC_Manager        *amanager )
  {
    FT_Error     error;
    FT_Memory    memory;
    FTC_Manager  manager = 0;


    if ( !library )
      return FTC_Err_Invalid_Library_Handle;

    memory = library->memory;

    if ( FT_NEW( manager ) )
      goto Exit;

    if ( max_faces == 0 )
      max_faces = FTC_MAX_FACES_DEFAULT;

    if ( max_sizes == 0 )
      max_sizes = FTC_MAX_SIZES_DEFAULT;

    if ( max_bytes == 0 )
      max_bytes = FTC_MAX_BYTES_DEFAULT;

    manager->library      = library;
    manager->memory       = memory;
    manager->max_weight   = max_bytes;

    manager->request_face = requester;
    manager->request_data = req_data;

    FTC_MruList_Init( &manager->faces,
                      &ftc_face_list_class,
                      max_faces,
                      manager,
                      memory );

    FTC_MruList_Init( &manager->sizes,
                      &ftc_size_list_class,
                      max_sizes,
                      manager,
                      memory );

    *amanager = manager;

  Exit:
    return error;
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Done( FTC_Manager  manager )
  {
    FT_Memory  memory;
    FT_UInt    idx;


    if ( !manager || !manager->library )
      return;

    memory = manager->memory;

    /* now discard all caches */
    for (idx = manager->num_caches; idx-- > 0; )
    {
      FTC_Cache  cache = manager->caches[idx];


      if ( cache )
      {
        cache->clazz.cache_done( cache );
        FT_FREE( cache );
        manager->caches[idx] = NULL;
      }
    }
    manager->num_caches = 0;

    /* discard faces and sizes */
    FTC_MruList_Done( &manager->sizes );
    FTC_MruList_Done( &manager->faces );

    manager->library = NULL;
    manager->memory  = NULL;

    FT_FREE( manager );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Reset( FTC_Manager  manager )
  {
    if ( manager )
    {
      FTC_MruList_Reset( &manager->sizes );
      FTC_MruList_Reset( &manager->faces );
    }
    /* XXX: FIXME: flush the caches? */
  }


#ifdef FT_DEBUG_ERROR

  static void
  FTC_Manager_Check( FTC_Manager  manager )
  {
    FTC_Node  node, first;


    first = manager->nodes_list;

    /* check node weights */
    if ( first )
    {
      FT_ULong  weight = 0;


      node = first;

      do
      {
        FTC_Cache  cache = manager->caches[node->cache_index];


        if ( (FT_UInt)node->cache_index >= manager->num_caches )
          FT_ERROR(( "FTC_Manager_Check: invalid node (cache index = %ld\n",
                     node->cache_index ));
        else
          weight += cache->clazz.node_weight( node, cache );

        node = FTC_NODE__NEXT( node );

      } while ( node != first );

      if ( weight != manager->cur_weight )
        FT_ERROR(( "FTC_Manager_Check: invalid weight %ld instead of %ld\n",
                   manager->cur_weight, weight ));
    }

    /* check circular list */
    if ( first )
    {
      FT_UFast  count = 0;


      node = first;
      do
      {
        count++;
        node = FTC_NODE__NEXT( node );

      } while ( node != first );

      if ( count != manager->num_nodes )
        FT_ERROR((
          "FTC_Manager_Check: invalid cache node count %d instead of %d\n",
          manager->num_nodes, count ));
    }
  }

#endif /* FT_DEBUG_ERROR */


  /* `Compress' the manager's data, i.e., get rid of old cache nodes */
  /* that are not referenced anymore in order to limit the total     */
  /* memory used by the cache.                                       */

  /* documentation is in ftcmanag.h */

  FT_LOCAL_DEF( void )
  FTC_Manager_Compress( FTC_Manager  manager )
  {
    FTC_Node   node, first;


    if ( !manager )
      return;

    first = manager->nodes_list;

#ifdef FT_DEBUG_ERROR
    FTC_Manager_Check( manager );

    FT_ERROR(( "compressing, weight = %ld, max = %ld, nodes = %d\n",
               manager->cur_weight, manager->max_weight,
               manager->num_nodes ));
#endif

    if ( manager->cur_weight < manager->max_weight || first == NULL )
      return;

    /* go to last node -- it's a circular list */
    node = FTC_NODE__PREV( first );
    do
    {
      FTC_Node  prev;


      prev = ( node == first ) ? NULL : FTC_NODE__PREV( node );

      if ( node->ref_count <= 0 )
        ftc_node_destroy( node, manager );

      node = prev;

    } while ( node && manager->cur_weight > manager->max_weight );
  }


  /* documentation is in ftcmanag.h */

  FT_LOCAL_DEF( FT_Error )
  FTC_Manager_RegisterCache( FTC_Manager      manager,
                             FTC_CacheClass   clazz,
                             FTC_Cache       *acache )
  {
    FT_Error   error = FTC_Err_Invalid_Argument;
    FTC_Cache  cache = NULL;


    if ( manager && clazz && acache )
    {
      FT_Memory  memory = manager->memory;


      if ( manager->num_caches >= FTC_MAX_CACHES )
      {
        error = FTC_Err_Too_Many_Caches;
        FT_ERROR(( "%s: too many registered caches\n",
                   "FTC_Manager_Register_Cache" ));
        goto Exit;
      }

      if ( !FT_ALLOC( cache, clazz->cache_size ) )
      {
        cache->manager   = manager;
        cache->memory    = memory;
        cache->clazz     = clazz[0];
        cache->org_class = clazz;

        /* THIS IS VERY IMPORTANT!  IT WILL WRETCH THE MANAGER */
        /* IF IT IS NOT SET CORRECTLY                          */
        cache->index = manager->num_caches;

        error = clazz->cache_init( cache );
        if ( error )
        {
          clazz->cache_done( cache );
          FT_FREE( cache );
          goto Exit;
        }

        manager->caches[manager->num_caches++] = cache;
      }
    }

  Exit:
    *acache = cache;
    return error;
  }


  FT_LOCAL_DEF( FT_UInt )
  FTC_Manager_FlushN( FTC_Manager  manager,
                      FT_UInt      count )
  {
    FTC_Node  first = manager->nodes_list;
    FTC_Node  node;
    FT_UInt   result;


    /* try to remove `count' nodes from the list */
    if ( first == NULL )  /* empty list! */
      return 0;

    /* go to last node - it's a circular list */
    node = FTC_NODE__PREV(first);
    for ( result = 0; result < count; )
    {
      FTC_Node  prev = FTC_NODE__PREV( node );


      /* don't touch locked nodes */
      if ( node->ref_count <= 0 )
      {
        ftc_node_destroy( node, manager );
        result++;
      }

      if ( node == first )
        break;

      node = prev;
    }
    return  result;
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_RemoveFaceID( FTC_Manager  manager,
                            FTC_FaceID   face_id )
  {
    FT_UInt  nn;

    /* this will remove all FTC_SizeNode that correspond to
     * the face_id as well
     */
    FTC_MruList_RemoveSelection( &manager->faces, NULL, face_id );

    for ( nn = 0; nn < manager->num_caches; nn++ )
      FTC_Cache_RemoveFaceID( manager->caches[nn], face_id );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Node_Unref( FTC_Node     node,
                  FTC_Manager  manager )
  {
    if ( node && (FT_UInt)node->cache_index < manager->num_caches )
      node->ref_count--;
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_Lookup_Face( FTC_Manager  manager,
                           FTC_FaceID   face_id,
                           FT_Face     *aface )
  {
    return FTC_Manager_LookupFace( manager, face_id, aface );
  }


  FT_EXPORT( FT_Error )
  FTC_Manager_Lookup_Size( FTC_Manager  manager,
                           FTC_Font     font,
                           FT_Face     *aface,
                           FT_Size     *asize )
  {
    FTC_ScalerRec  scaler;
    FT_Error       error;
    FT_Size        size;
    FT_Face        face;


    scaler.face_id = font->face_id;
    scaler.width   = font->pix_width;
    scaler.height  = font->pix_height;
    scaler.pixel   = TRUE;
    scaler.x_res   = 0;
    scaler.y_res   = 0;

    error = FTC_Manager_LookupSize( manager, &scaler, &size );
    if ( error )
    {
      face = NULL;
      size = NULL;
    }
    else
      face = size->face;

    if ( aface )
      *aface = face;

    if ( asize )
      *asize = size;

    return error;
  }

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftccache.c                                                             */
/*                                                                         */
/*    The FreeType internal cache interface (body).                        */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include "ftcmanag.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftccback.h"
#include "ftcerror.h"


#define FTC_HASH_MAX_LOAD  2
#define FTC_HASH_MIN_LOAD  1
#define FTC_HASH_SUB_LOAD  ( FTC_HASH_MAX_LOAD - FTC_HASH_MIN_LOAD )

/* this one _must_ be a power of 2! */
#define FTC_HASH_INITIAL_SIZE  8


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE NODE DEFINITIONS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* add a new node to the head of the manager's circular MRU list */
  static void
  ftc_node_mru_link( FTC_Node     node,
                     FTC_Manager  manager )
  {
    FTC_MruNode_Prepend( (FTC_MruNode*)&manager->nodes_list,
                         (FTC_MruNode)node );
    manager->num_nodes++;
  }


  /* remove a node from the manager's MRU list */
  static void
  ftc_node_mru_unlink( FTC_Node     node,
                       FTC_Manager  manager )
  {
    FTC_MruNode_Remove( (FTC_MruNode*)&manager->nodes_list,
                        (FTC_MruNode)node );
    manager->num_nodes--;
  }


#ifndef FTC_INLINE

  /* move a node to the head of the manager's MRU list */
  static void
  ftc_node_mru_up( FTC_Node     node,
                   FTC_Manager  manager )
  {
    FTC_MruNode_Up( (FTC_MruNode*)&manager->nodes_list,
                    (FTC_MruNode)node );
  }

#endif /* !FTC_INLINE */


  /* Note that this function cannot fail.  If we cannot re-size the
   * buckets array appropriately, we simply degrade the hash table's
   * performance!
   */
  static void
  ftc_cache_resize( FTC_Cache  cache )
  {
    for (;;)
    {
      FTC_Node  node, *pnode;
      FT_UInt   p      = cache->p;
      FT_UInt   mask   = cache->mask;
      FT_UInt   count  = mask + p + 1;    /* number of buckets */


      /* do we need to shrink the buckets array? */
      if ( cache->slack < 0 )
      {
        FTC_Node  new_list = NULL;


        /* try to expand the buckets array _before_ splitting
         * the bucket lists
         */
        if ( p >= mask )
        {
          FT_Memory  memory = cache->memory;
          FT_Error   error;


          /* if we can't expand the array, leave immediately */
          if ( FT_RENEW_ARRAY( cache->buckets, (mask+1)*2, (mask+1)*4 ) )
            break;
        }

        /* split a single bucket */
        pnode = cache->buckets + p;

        for (;;)
        {
          node = *pnode;
          if ( node == NULL )
            break;

          if ( node->hash & ( mask + 1 ) )
          {
            *pnode     = node->link;
            node->link = new_list;
            new_list   = node;
          }
          else
            pnode = &node->link;
        }

        cache->buckets[p + mask + 1] = new_list;

        cache->slack += FTC_HASH_MAX_LOAD;

        if ( p >= mask )
        {
          cache->mask = 2 * mask + 1;
          cache->p    = 0;
        }
        else
          cache->p = p + 1;
      }

      /* do we need to expand the buckets array? */
      else if ( cache->slack > (FT_Long)count * FTC_HASH_SUB_LOAD )
      {
        FT_UInt    old_index = p + mask;
        FTC_Node*  pold;


        if ( old_index + 1 <= FTC_HASH_INITIAL_SIZE )
          break;

        if ( p == 0 )
        {
          FT_Memory  memory = cache->memory;
          FT_Error   error;


          /* if we can't shrink the array, leave immediately */
          if ( FT_RENEW_ARRAY( cache->buckets,
                               ( mask + 1 ) * 2, mask + 1 ) )
            break;

          cache->mask >>= 1;
          p             = cache->mask;
        }
        else
          p--;

        pnode = cache->buckets + p;
        while ( *pnode )
          pnode = &(*pnode)->link;

        pold   = cache->buckets + old_index;
        *pnode = *pold;
        *pold  = NULL;

        cache->slack -= FTC_HASH_MAX_LOAD;
        cache->p      = p;
      }
      else /* the hash table is balanced */
        break;
    }
  }


  /* remove a node from its cache's hash table */
  static void
  ftc_node_hash_unlink( FTC_Node   node0,
                        FTC_Cache  cache )
  {
    FTC_Node  *pnode;
    FT_UInt    idx;


    idx = (FT_UInt)( node0->hash & cache->mask );
    if ( idx < cache->p )
      idx = (FT_UInt)( node0->hash & ( 2 * cache->mask + 1 ) );

    pnode = cache->buckets + idx;

    for (;;)
    {
      FTC_Node  node = *pnode;


      if ( node == NULL )
      {
        FT_ERROR(( "ftc_node_hash_unlink: unknown node!\n" ));
        return;
      }

      if ( node == node0 )
        break;

      pnode = &(*pnode)->link;
    }

    *pnode      = node0->link;
    node0->link = NULL;

    cache->slack++;
    ftc_cache_resize( cache );
  }


  /* add a node to the `top' of its cache's hash table */
  static void
  ftc_node_hash_link( FTC_Node   node,
                      FTC_Cache  cache )
  {
    FTC_Node  *pnode;
    FT_UInt    idx;


    idx = (FT_UInt)( node->hash & cache->mask );
    if ( idx < cache->p )
      idx = (FT_UInt)( node->hash & (2 * cache->mask + 1 ) );

    pnode = cache->buckets + idx;

    node->link = *pnode;
    *pnode     = node;

    cache->slack--;
    ftc_cache_resize( cache );
  }


  /* remove a node from the cache manager */
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
  FT_BASE_DEF( void )
#else
  FT_LOCAL_DEF( void )
#endif
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager )
  {
    FTC_Cache  cache;


#ifdef FT_DEBUG_ERROR
    /* find node's cache */
    if ( node->cache_index >= manager->num_caches )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    cache = manager->caches[node->cache_index];

#ifdef FT_DEBUG_ERROR
    if ( cache == NULL )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    manager->cur_weight -= cache->clazz.node_weight( node, cache );

    /* remove node from mru list */
    ftc_node_mru_unlink( node, manager );

    /* remove node from cache's hash table */
    ftc_node_hash_unlink( node, cache );

    /* now finalize it */
    cache->clazz.node_free( node, cache );

#if 0
    /* check, just in case of general corruption :-) */
    if ( manager->num_nodes == 0 )
      FT_ERROR(( "ftc_node_destroy: invalid cache node count! = %d\n",
                  manager->num_nodes ));
#endif
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    ABSTRACT CACHE CLASS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_Init( FTC_Cache  cache )
  {
    return ftc_cache_init( cache );
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_cache_init( FTC_Cache  cache )
  {
    FT_Memory  memory = cache->memory;
    FT_Error   error;


    cache->p     = 0;
    cache->mask  = FTC_HASH_INITIAL_SIZE - 1;
    cache->slack = FTC_HASH_INITIAL_SIZE * FTC_HASH_MAX_LOAD;

    (void)FT_NEW_ARRAY( cache->buckets, FTC_HASH_INITIAL_SIZE * 2 );
    return error;
  }


  static void
  FTC_Cache_Clear( FTC_Cache  cache )
  {
    if ( cache )
    {
      FTC_Manager  manager = cache->manager;
      FT_UFast     i;
      FT_UInt      count;


      count = cache->p + cache->mask + 1;

      for ( i = 0; i < count; i++ )
      {
        FTC_Node  *pnode = cache->buckets + i, next, node = *pnode;


        while ( node )
        {
          next        = node->link;
          node->link  = NULL;

          /* remove node from mru list */
          ftc_node_mru_unlink( node, manager );

          /* now finalize it */
          manager->cur_weight -= cache->clazz.node_weight( node, cache );

          cache->clazz.node_free( node, cache );
          node = next;
        }
        cache->buckets[i] = NULL;
      }
      ftc_cache_resize( cache );
    }
  }


  FT_LOCAL_DEF( void )
  ftc_cache_done( FTC_Cache  cache )
  {
    if ( cache->memory )
    {
      FT_Memory  memory = cache->memory;


      FTC_Cache_Clear( cache );

      FT_FREE( cache->buckets );
      cache->mask  = 0;
      cache->p     = 0;
      cache->slack = 0;

      cache->memory = NULL;
    }
  }


  FT_LOCAL_DEF( void )
  FTC_Cache_Done( FTC_Cache  cache )
  {
    ftc_cache_done( cache );
  }


  static void
  ftc_cache_add( FTC_Cache  cache,
                 FT_UInt32  hash,
                 FTC_Node   node )
  {
    node->hash = hash;
    node->cache_index = (FT_UInt16) cache->index;
    node->ref_count   = 0;

    ftc_node_hash_link( node, cache );
    ftc_node_mru_link( node, cache->manager );

    {
      FTC_Manager  manager = cache->manager;


      manager->cur_weight += cache->clazz.node_weight( node, cache );

      if ( manager->cur_weight >= manager->max_weight )
      {
        node->ref_count++;
        FTC_Manager_Compress( manager );
        node->ref_count--;
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_NewNode( FTC_Cache   cache,
                     FT_UInt32   hash,
                     FT_Pointer  query,
                     FTC_Node   *anode )
  {
    FT_Error  error;
    FTC_Node  node;


    /*
     * We use the FTC_CACHE_TRYLOOP macros to support out-of-memory
     * errors (OOM) correctly, i.e., by flushing the cache progressively
     * in order to make more room.
     */

    FTC_CACHE_TRYLOOP( cache )
    {
      error = cache->clazz.node_new( &node, query, cache );
    }
    FTC_CACHE_TRYLOOP_END();

    if ( error )
      node = NULL;
    else
    {
     /* don't assume that the cache has the same number of buckets, since
      * our allocation request might have triggered global cache flushing
      */
      ftc_cache_add( cache, hash, node );
    }

    *anode = node;
    return error;
  }


#ifndef FTC_INLINE

  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_Lookup( FTC_Cache   cache,
                    FT_UInt32   hash,
                    FT_Pointer  query,
                    FTC_Node   *anode )
  {
    FT_UFast   idx;
    FTC_Node*  bucket;
    FTC_Node*  pnode;
    FTC_Node   node;
    FT_Error   error = 0;

    FTC_Node_CompareFunc  compare = cache->clazz.node_compare;


    if ( cache == NULL || anode == NULL )
      return FT_Err_Invalid_Argument;

    idx = hash & cache->mask;
    if ( idx < cache->p )
      idx = hash & ( cache->mask * 2 + 1 );

    bucket = cache->buckets + idx;
    pnode  = bucket;
    for (;;)
    {
      node = *pnode;
      if ( node == NULL )
        goto NewNode;

      if ( node->hash == hash && compare( node, query, cache ) )
        break;

      pnode = &node->link;
    }

    if ( node != *bucket )
    {
      *pnode     = node->link;
      node->link = *bucket;
      *bucket    = node;
    }

    /* move to head of MRU list */
    {
      FTC_Manager  manager = cache->manager;


      if ( node != manager->nodes_list )
        ftc_node_mru_up( node, manager );
    }
    *anode = node;
    return error;

  NewNode:
    return FTC_Cache_NewNode( cache, hash, query, anode );
  }

#endif /* !FTC_INLINE */


  FT_LOCAL_DEF( void )
  FTC_Cache_RemoveFaceID( FTC_Cache   cache,
                          FTC_FaceID  face_id )
  {
    FT_UFast     i, count;
    FTC_Manager  manager = cache->manager;
    FTC_Node     frees   = NULL;


    count = cache->p + cache->mask;
    for ( i = 0; i < count; i++ )
    {
      FTC_Node*  bucket = cache->buckets + i;
      FTC_Node*  pnode  = bucket;


      for ( ;; )
      {
        FTC_Node  node = *pnode;


        if ( node == NULL )
          break;

        if ( cache->clazz.node_remove_faceid( node, face_id, cache ) )
        {
          *pnode     = node->link;
          node->link = frees;
          frees      = node;
        }
        else
          pnode = &node->link;
      }
    }

    /* remove all nodes in the free list */
    while ( frees )
    {
      FTC_Node  node;


      node  = frees;
      frees = node->link;

      manager->cur_weight -= cache->clazz.node_weight( node, cache );
      ftc_node_mru_unlink( node, manager );

      cache->clazz.node_free( node, cache );

      cache->slack++;
    }

    ftc_cache_resize( cache );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftccmap.c                                                              */
/*                                                                         */
/*    FreeType CharMap cache (body)                                        */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include FT_CACHE_H
#include "ftcmanag.h"
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_DEBUG_H
#include FT_TRUETYPE_IDS_H

#include "ftccback.h"
#include "ftcerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  typedef enum  FTC_OldCMapType_
  {
    FTC_OLD_CMAP_BY_INDEX    = 0,
    FTC_OLD_CMAP_BY_ENCODING = 1,
    FTC_OLD_CMAP_BY_ID       = 2

  } FTC_OldCMapType;


  typedef struct  FTC_OldCMapIdRec_
  {
    FT_UInt  platform;
    FT_UInt  encoding;

  } FTC_OldCMapIdRec, *FTC_OldCMapId;


  typedef struct  FTC_OldCMapDescRec_
  {
    FTC_FaceID       face_id;
    FTC_OldCMapType  type;

    union
    {
      FT_UInt           index;
      FT_Encoding       encoding;
      FTC_OldCMapIdRec  id;

    } u;

  } FTC_OldCMapDescRec, *FTC_OldCMapDesc;

#endif /* FT_CONFIG_OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* Each FTC_CMapNode contains a simple array to map a range of character */
  /* codes to equivalent glyph indices.                                    */
  /*                                                                       */
  /* For now, the implementation is very basic: Each node maps a range of  */
  /* 128 consecutive character codes to their corresponding glyph indices. */
  /*                                                                       */
  /* We could do more complex things, but I don't think it is really very  */
  /* useful.                                                               */
  /*                                                                       */
  /*************************************************************************/


  /* number of glyph indices / character code per node */
#define FTC_CMAP_INDICES_MAX  128

  /* compute a query/node hash */
#define FTC_CMAP_HASH( faceid, index, charcode )           \
          ( FTC_FACE_ID_HASH( faceid ) + 211 * ( index ) + \
            ( (char_code) / FTC_CMAP_INDICES_MAX )       )

  /* the charmap query */
  typedef struct  FTC_CMapQueryRec_
  {
    FTC_FaceID  face_id;
    FT_UInt     cmap_index;
    FT_UInt32   char_code;

  } FTC_CMapQueryRec, *FTC_CMapQuery;

#define FTC_CMAP_QUERY( x )  ((FTC_CMapQuery)(x))
#define FTC_CMAP_QUERY_HASH( x )                                         \
          FTC_CMAP_HASH( (x)->face_id, (x)->cmap_index, (x)->char_code )

  /* the cmap cache node */
  typedef struct  FTC_CMapNodeRec_
  {
    FTC_NodeRec  node;
    FTC_FaceID   face_id;
    FT_UInt      cmap_index;
    FT_UInt32    first;                         /* first character in node */
    FT_UInt16    indices[FTC_CMAP_INDICES_MAX]; /* array of glyph indices  */

  } FTC_CMapNodeRec, *FTC_CMapNode;

#define FTC_CMAP_NODE( x ) ( (FTC_CMapNode)( x ) )
#define FTC_CMAP_NODE_HASH( x )                                      \
          FTC_CMAP_HASH( (x)->face_id, (x)->cmap_index, (x)->first )

  /* if (indices[n] == FTC_CMAP_UNKNOWN), we assume that the corresponding */
  /* glyph indices haven't been queried through FT_Get_Glyph_Index() yet   */
#define FTC_CMAP_UNKNOWN  ( (FT_UInt16)-1 )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        CHARMAP NODES                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_DEF( void )
  ftc_cmap_node_free( FTC_Node   ftcnode,
                      FTC_Cache  cache )
  {
    FTC_CMapNode  node   = (FTC_CMapNode)ftcnode;
    FT_Memory     memory = cache->memory;


    FT_FREE( node );
  }


  /* initialize a new cmap node */
  FT_CALLBACK_DEF( FT_Error )
  ftc_cmap_node_new( FTC_Node   *ftcanode,
                     FT_Pointer  ftcquery,
                     FTC_Cache   cache )
  {
    FTC_CMapNode  *anode  = (FTC_CMapNode*)ftcanode;
    FTC_CMapQuery  query  = (FTC_CMapQuery)ftcquery;
    FT_Error       error;
    FT_Memory      memory = cache->memory;
    FTC_CMapNode   node;
    FT_UInt        nn;


    if ( !FT_NEW( node ) )
    {
      node->face_id    = query->face_id;
      node->cmap_index = query->cmap_index;
      node->first      = (query->char_code / FTC_CMAP_INDICES_MAX) *
                         FTC_CMAP_INDICES_MAX;

      for ( nn = 0; nn < FTC_CMAP_INDICES_MAX; nn++ )
        node->indices[nn] = FTC_CMAP_UNKNOWN;
    }

    *anode = node;
    return error;
  }


  /* compute the weight of a given cmap node */
  FT_CALLBACK_DEF( FT_ULong )
  ftc_cmap_node_weight( FTC_Node   cnode,
                        FTC_Cache  cache )
  {
    FT_UNUSED( cnode );
    FT_UNUSED( cache );

    return sizeof ( *cnode );
  }


  /* compare a cmap node to a given query */
  FT_CALLBACK_DEF( FT_Bool )
  ftc_cmap_node_compare( FTC_Node    ftcnode,
                         FT_Pointer  ftcquery,
                         FTC_Cache   cache )
  {
    FTC_CMapNode   node  = (FTC_CMapNode)ftcnode;
    FTC_CMapQuery  query = (FTC_CMapQuery)ftcquery;
    FT_UNUSED( cache );


    if ( node->face_id    == query->face_id    &&
         node->cmap_index == query->cmap_index )
    {
      FT_UInt32  offset = (FT_UInt32)( query->char_code - node->first );


      return FT_BOOL( offset < FTC_CMAP_INDICES_MAX );
    }

    return 0;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_cmap_node_remove_faceid( FTC_Node    ftcnode,
                               FT_Pointer  ftcface_id,
                               FTC_Cache   cache )
  {
    FTC_CMapNode  node    = (FTC_CMapNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;
    FT_UNUSED( cache );

    return FT_BOOL( node->face_id == face_id );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    GLYPH IMAGE CACHE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_TABLE_DEF
  const FTC_CacheClassRec  ftc_cmap_cache_class =
  {
    ftc_cmap_node_new,
    ftc_cmap_node_weight,
    ftc_cmap_node_compare,
    ftc_cmap_node_remove_faceid,
    ftc_cmap_node_free,

    sizeof ( FTC_CacheRec ),
    ftc_cache_init,
    ftc_cache_done,
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_CMapCache_New( FTC_Manager     manager,
                     FTC_CMapCache  *acache )
  {
    return FTC_Manager_RegisterCache( manager,
                                      &ftc_cmap_cache_class,
                                      FTC_CACHE_P( acache ) );
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /*
   *  Unfortunately, it is not possible to support binary backwards
   *  compatibility in the cmap cache.  The FTC_CMapCache_Lookup signature
   *  changes were too deep, and there is no clever hackish way to detect
   *  what kind of structure we are being passed.
   *
   *  On the other hand it seems that no production code is using this
   *  function on Unix distributions.
   */

#endif


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_UInt )
  FTC_CMapCache_Lookup( FTC_CMapCache  cmap_cache,
                        FTC_FaceID     face_id,
                        FT_Int         cmap_index,
                        FT_UInt32      char_code )
  {
    FTC_Cache         cache = FTC_CACHE( cmap_cache );
    FTC_CMapQueryRec  query;
    FTC_CMapNode      node;
    FT_Error          error;
    FT_UInt           gindex = 0;
    FT_UInt32         hash;


    if ( !cache )
    {
      FT_ERROR(( "FTC_CMapCache_Lookup: bad arguments, returning 0!\n" ));
      return 0;
    }

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    /*
     *  Detect a call from a rogue client that thinks it is linking
     *  to FreeType 2.1.7.  This is possible because the third parameter
     *  is then a character code, and we have never seen any font with
     *  more than a few charmaps, so if the index is very large...
     *
     *  It is also very unlikely that a rogue client is interested
     *  in Unicode values 0 to 3.
     */
    if ( cmap_index >= 4 )
    {
      FTC_OldCMapDesc  desc = (FTC_OldCMapDesc) face_id;


      char_code     = (FT_UInt32)cmap_index;
      query.face_id = desc->face_id;


      switch ( desc->type )
      {
      case FTC_OLD_CMAP_BY_INDEX:
        query.cmap_index = desc->u.index;
        query.char_code  = (FT_UInt32)cmap_index;
        break;

      case FTC_OLD_CMAP_BY_ENCODING:
        {
          FT_Face  face;


          error = FTC_Manager_LookupFace( cache->manager, desc->face_id,
                                          &face );
          if ( error )
            return 0;

          FT_Select_Charmap( face, desc->u.encoding );

          return FT_Get_Char_Index( face, char_code );
        }
        /*break;*/

      default:
        return 0;
      }
    }
    else

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */

    {
      query.face_id    = face_id;
      query.cmap_index = (FT_UInt)cmap_index;
      query.char_code  = char_code;
    }

    hash = FTC_CMAP_HASH( face_id, cmap_index, char_code );

#if 1
    FTC_CACHE_LOOKUP_CMP( cache, ftc_cmap_node_compare, hash, &query,
                          node, error );
#else
    error = FTC_Cache_Lookup( cache, hash, &query, (FTC_Node*) &node );
#endif
    if ( error )
      goto Exit;

    FT_ASSERT( (FT_UInt)( char_code - node->first ) < FTC_CMAP_INDICES_MAX );

    /* something rotten can happen with rogue clients */
    if ( (FT_UInt)( char_code - node->first >= FTC_CMAP_INDICES_MAX ) )
      return 0;

    gindex = node->indices[char_code - node->first];
    if ( gindex == FTC_CMAP_UNKNOWN )
    {
      FT_Face  face;


      gindex = 0;

      error = FTC_Manager_LookupFace( cache->manager, node->face_id, &face );
      if ( error )
        goto Exit;

      if ( (FT_UInt)cmap_index < (FT_UInt)face->num_charmaps )
      {
        FT_CharMap  old, cmap  = NULL;


        old  = face->charmap;
        cmap = face->charmaps[cmap_index];

        if ( old != cmap )
          FT_Set_Charmap( face, cmap );

        gindex = FT_Get_Char_Index( face, char_code );

        if ( old != cmap )
          FT_Set_Charmap( face, old );
      }

      node->indices[char_code - node->first] = (FT_UShort)gindex;
    }

  Exit:
    return gindex;
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcglyph.c                                                             */
/*                                                                         */
/*    FreeType Glyph Image (FT_Glyph) cache (body).                        */
/*                                                                         */
/*  Copyright 2000-2001, 2003, 2004, 2006 by                               */
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
#include FT_CACHE_H
#include "ftcglyph.h"
#include FT_ERRORS_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftccback.h"
#include "ftcerror.h"


  /* create a new chunk node, setting its cache index and ref count */
  FT_LOCAL_DEF( void )
  FTC_GNode_Init( FTC_GNode   gnode,
                  FT_UInt     gindex,
                  FTC_Family  family )
  {
    gnode->family = family;
    gnode->gindex = gindex;
    family->num_nodes++;
  }


  FT_LOCAL_DEF( void )
  FTC_GNode_UnselectFamily( FTC_GNode  gnode,
                            FTC_Cache  cache )
  {
    FTC_Family  family = gnode->family;


    gnode->family = NULL;
    if ( family && --family->num_nodes == 0 )
      FTC_FAMILY_FREE( family, cache );
  }


  FT_LOCAL_DEF( void )
  FTC_GNode_Done( FTC_GNode  gnode,
                  FTC_Cache  cache )
  {
    /* finalize the node */
    gnode->gindex = 0;

    FTC_GNode_UnselectFamily( gnode, cache );
  }


  FT_LOCAL_DEF( FT_Bool )
  ftc_gnode_compare( FTC_Node    ftcgnode,
                     FT_Pointer  ftcgquery,
                     FTC_Cache   cache )
  {
    FTC_GNode   gnode  = (FTC_GNode)ftcgnode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;
    FT_UNUSED( cache );


    return FT_BOOL(  gnode->family == gquery->family &&
                     gnode->gindex == gquery->gindex );
  }


  FT_LOCAL_DEF( FT_Bool )
  FTC_GNode_Compare( FTC_GNode   gnode,
                     FTC_GQuery  gquery )
  {
    return ftc_gnode_compare( FTC_NODE( gnode ), gquery, NULL );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      CHUNK SETS                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  FTC_Family_Init( FTC_Family  family,
                   FTC_Cache   cache )
  {
    FTC_GCacheClass  clazz = FTC_CACHE__GCACHE_CLASS( cache );


    family->clazz     = clazz->family_class;
    family->num_nodes = 0;
    family->cache     = cache;
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_gcache_init( FTC_Cache  ftccache )
  {
    FTC_GCache  cache = (FTC_GCache)ftccache;
    FT_Error    error;


    error = FTC_Cache_Init( FTC_CACHE( cache ) );
    if ( !error )
    {
      FTC_GCacheClass   clazz = (FTC_GCacheClass)FTC_CACHE( cache )->org_class;

      FTC_MruList_Init( &cache->families,
                        clazz->family_class,
                        0,  /* no maximum here! */
                        cache,
                        FTC_CACHE( cache )->memory );
    }

    return error;
  }


#if 0

  FT_LOCAL_DEF( FT_Error )
  FTC_GCache_Init( FTC_GCache  cache )
  {
    return ftc_gcache_init( FTC_CACHE( cache ) );
  }

#endif /* 0 */


  FT_LOCAL_DEF( void )
  ftc_gcache_done( FTC_Cache  ftccache )
  {
    FTC_GCache  cache = (FTC_GCache)ftccache;


    FTC_Cache_Done( (FTC_Cache)cache );
    FTC_MruList_Done( &cache->families );
  }


#if 0

  FT_LOCAL_DEF( void )
  FTC_GCache_Done( FTC_GCache  cache )
  {
    ftc_gcache_done( FTC_CACHE( cache ) );
  }

#endif /* 0 */


  FT_LOCAL_DEF( FT_Error )
  FTC_GCache_New( FTC_Manager       manager,
                  FTC_GCacheClass   clazz,
                  FTC_GCache       *acache )
  {
    return FTC_Manager_RegisterCache( manager, (FTC_CacheClass)clazz,
                                      (FTC_Cache*)acache );
  }


#ifndef FTC_INLINE

  FT_LOCAL_DEF( FT_Error )
  FTC_GCache_Lookup( FTC_GCache   cache,
                     FT_UInt32    hash,
                     FT_UInt      gindex,
                     FTC_GQuery   query,
                     FTC_Node    *anode )
  {
    FT_Error  error;


    query->gindex = gindex;

    FTC_MRULIST_LOOKUP( &cache->families, query, query->family, error );
    if ( !error )
    {
      FTC_Family  family = query->family;


      /* prevent the family from being destroyed too early when an        */
      /* out-of-memory condition occurs during glyph node initialization. */
      family->num_nodes++;

      error = FTC_Cache_Lookup( FTC_CACHE( cache ), hash, query, anode );

      if ( --family->num_nodes == 0 )
        FTC_FAMILY_FREE( family, cache );
    }
    return error;
  }

#endif /* !FTC_INLINE */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcimage.c                                                             */
/*                                                                         */
/*    FreeType Image cache (body).                                         */
/*                                                                         */
/*  Copyright 2000-2001, 2003, 2004, 2006 by                               */
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
#include FT_CACHE_H
#include "ftcimage.h"
#include FT_INTERNAL_MEMORY_H

#include "ftccback.h"
#include "ftcerror.h"


  /* finalize a given glyph image node */
  FT_LOCAL_DEF( void )
  ftc_inode_free( FTC_Node   ftcinode,
                  FTC_Cache  cache )
  {
    FTC_INode  inode = (FTC_INode)ftcinode;
    FT_Memory  memory = cache->memory;


    if ( inode->glyph )
    {
      FT_Done_Glyph( inode->glyph );
      inode->glyph = NULL;
    }

    FTC_GNode_Done( FTC_GNODE( inode ), cache );
    FT_FREE( inode );
  }


  FT_LOCAL_DEF( void )
  FTC_INode_Free( FTC_INode  inode,
                  FTC_Cache  cache )
  {
    ftc_inode_free( FTC_NODE( inode ), cache );
  }


  /* initialize a new glyph image node */
  FT_LOCAL_DEF( FT_Error )
  FTC_INode_New( FTC_INode   *pinode,
                 FTC_GQuery   gquery,
                 FTC_Cache    cache )
  {
    FT_Memory  memory = cache->memory;
    FT_Error   error;
    FTC_INode  inode;


    if ( !FT_NEW( inode ) )
    {
      FTC_GNode         gnode  = FTC_GNODE( inode );
      FTC_Family        family = gquery->family;
      FT_UInt           gindex = gquery->gindex;
      FTC_IFamilyClass  clazz  = FTC_CACHE__IFAMILY_CLASS( cache );


      /* initialize its inner fields */
      FTC_GNode_Init( gnode, gindex, family );

      /* we will now load the glyph image */
      error = clazz->family_load_glyph( family, gindex, cache,
                                        &inode->glyph );
      if ( error )
      {
        FTC_INode_Free( inode, cache );
        inode = NULL;
      }
    }

    *pinode = inode;
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_inode_new( FTC_Node   *ftcpinode,
                 FT_Pointer  ftcgquery,
                 FTC_Cache   cache )
  {
    FTC_INode  *pinode = (FTC_INode*)ftcpinode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;


    return FTC_INode_New( pinode, gquery, cache );
  }


  FT_LOCAL_DEF( FT_ULong )
  ftc_inode_weight( FTC_Node   ftcinode,
                    FTC_Cache  ftccache )
  {
    FTC_INode  inode = (FTC_INode)ftcinode;
    FT_ULong   size  = 0;
    FT_Glyph   glyph = inode->glyph;

    FT_UNUSED( ftccache );


    switch ( glyph->format )
    {
    case FT_GLYPH_FORMAT_BITMAP:
      {
        FT_BitmapGlyph  bitg;


        bitg = (FT_BitmapGlyph)glyph;
        size = bitg->bitmap.rows * ft_labs( bitg->bitmap.pitch ) +
               sizeof ( *bitg );
      }
      break;

    case FT_GLYPH_FORMAT_OUTLINE:
      {
        FT_OutlineGlyph  outg;


        outg = (FT_OutlineGlyph)glyph;
        size = outg->outline.n_points *
                 ( sizeof ( FT_Vector ) + sizeof ( FT_Byte ) ) +
               outg->outline.n_contours * sizeof ( FT_Short ) +
               sizeof ( *outg );
      }
      break;

    default:
      ;
    }

    size += sizeof ( *inode );
    return size;
  }


#if 0

  FT_LOCAL_DEF( FT_ULong )
  FTC_INode_Weight( FTC_INode  inode )
  {
    return ftc_inode_weight( FTC_NODE( inode ), NULL );
  }

#endif /* 0 */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcsbits.c                                                             */
/*                                                                         */
/*    FreeType sbits manager (body).                                       */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include FT_CACHE_H
#include "ftcsbits.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_ERRORS_H

#include "ftccback.h"
#include "ftcerror.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     SBIT CACHE NODES                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  ftc_sbit_copy_bitmap( FTC_SBit    sbit,
                        FT_Bitmap*  bitmap,
                        FT_Memory   memory )
  {
    FT_Error  error;
    FT_Int    pitch = bitmap->pitch;
    FT_ULong  size;


    if ( pitch < 0 )
      pitch = -pitch;

    size = (FT_ULong)( pitch * bitmap->rows );

    if ( !FT_ALLOC( sbit->buffer, size ) )
      FT_MEM_COPY( sbit->buffer, bitmap->buffer, size );

    return error;
  }


  FT_LOCAL_DEF( void )
  ftc_snode_free( FTC_Node   ftcsnode,
                  FTC_Cache  cache )
  {
    FTC_SNode  snode  = (FTC_SNode)ftcsnode;
    FTC_SBit   sbit   = snode->sbits;
    FT_UInt    count  = snode->count;
    FT_Memory  memory = cache->memory;


    for ( ; count > 0; sbit++, count-- )
      FT_FREE( sbit->buffer );

    FTC_GNode_Done( FTC_GNODE( snode ), cache );

    FT_FREE( snode );
  }


  FT_LOCAL_DEF( void )
  FTC_SNode_Free( FTC_SNode  snode,
                  FTC_Cache  cache )
  {
    ftc_snode_free( FTC_NODE( snode ), cache );
  }


  /*
   *  This function tries to load a small bitmap within a given FTC_SNode.
   *  Note that it returns a non-zero error code _only_ in the case of
   *  out-of-memory condition.  For all other errors (e.g., corresponding
   *  to a bad font file), this function will mark the sbit as `unavailable'
   *  and return a value of 0.
   *
   *  You should also read the comment within the @ftc_snode_compare
   *  function below to see how out-of-memory is handled during a lookup.
   */
  static FT_Error
  ftc_snode_load( FTC_SNode    snode,
                  FTC_Manager  manager,
                  FT_UInt      gindex,
                  FT_ULong    *asize )
  {
    FT_Error          error;
    FTC_GNode         gnode  = FTC_GNODE( snode );
    FTC_Family        family = gnode->family;
    FT_Memory         memory = manager->memory;
    FT_Face           face;
    FTC_SBit          sbit;
    FTC_SFamilyClass  clazz;


    if ( (FT_UInt)(gindex - gnode->gindex) >= snode->count )
    {
      FT_ERROR(( "ftc_snode_load: invalid glyph index" ));
      return FTC_Err_Invalid_Argument;
    }

    sbit  = snode->sbits + ( gindex - gnode->gindex );
    clazz = (FTC_SFamilyClass)family->clazz;

    sbit->buffer = 0;

    error = clazz->family_load_glyph( family, gindex, manager, &face );
    if ( error )
      goto BadGlyph;

    {
      FT_Int        temp;
      FT_GlyphSlot  slot   = face->glyph;
      FT_Bitmap*    bitmap = &slot->bitmap;
      FT_Int        xadvance, yadvance;


      if ( slot->format != FT_GLYPH_FORMAT_BITMAP )
      {
        FT_ERROR(( "%s: glyph loaded didn't return a bitmap!\n",
                   "ftc_snode_load" ));
        goto BadGlyph;
      }

      /* Check that our values fit into 8-bit containers!       */
      /* If this is not the case, our bitmap is too large       */
      /* and we will leave it as `missing' with sbit.buffer = 0 */

#define CHECK_CHAR( d )  ( temp = (FT_Char)d, temp == d )
#define CHECK_BYTE( d )  ( temp = (FT_Byte)d, temp == d )

      /* horizontal advance in pixels */
      xadvance = ( slot->advance.x + 32 ) >> 6;
      yadvance = ( slot->advance.y + 32 ) >> 6;

      if ( !CHECK_BYTE( bitmap->rows  )     ||
           !CHECK_BYTE( bitmap->width )     ||
           !CHECK_CHAR( bitmap->pitch )     ||
           !CHECK_CHAR( slot->bitmap_left ) ||
           !CHECK_CHAR( slot->bitmap_top  ) ||
           !CHECK_CHAR( xadvance )          ||
           !CHECK_CHAR( yadvance )          )
        goto BadGlyph;

      sbit->width     = (FT_Byte)bitmap->width;
      sbit->height    = (FT_Byte)bitmap->rows;
      sbit->pitch     = (FT_Char)bitmap->pitch;
      sbit->left      = (FT_Char)slot->bitmap_left;
      sbit->top       = (FT_Char)slot->bitmap_top;
      sbit->xadvance  = (FT_Char)xadvance;
      sbit->yadvance  = (FT_Char)yadvance;
      sbit->format    = (FT_Byte)bitmap->pixel_mode;
      sbit->max_grays = (FT_Byte)(bitmap->num_grays - 1);

      /* copy the bitmap into a new buffer -- ignore error */
      error = ftc_sbit_copy_bitmap( sbit, bitmap, memory );

      /* now, compute size */
      if ( asize )
        *asize = FT_ABS( sbit->pitch ) * sbit->height;

    } /* glyph loading successful */

    /* ignore the errors that might have occurred --   */
    /* we mark unloaded glyphs with `sbit.buffer == 0' */
    /* and `width == 255', `height == 0'               */
    /*                                                 */
    if ( error && error != FTC_Err_Out_Of_Memory )
    {
    BadGlyph:
      sbit->width  = 255;
      sbit->height = 0;
      sbit->buffer = NULL;
      error        = 0;
      if ( asize )
        *asize = 0;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  FTC_SNode_New( FTC_SNode  *psnode,
                 FTC_GQuery  gquery,
                 FTC_Cache   cache )
  {
    FT_Memory   memory = cache->memory;
    FT_Error    error;
    FTC_SNode   snode  = NULL;
    FT_UInt     gindex = gquery->gindex;
    FTC_Family  family = gquery->family;

    FTC_SFamilyClass  clazz = FTC_CACHE__SFAMILY_CLASS( cache );
    FT_UInt           total;


    total = clazz->family_get_count( family, cache->manager );
    if ( total == 0 || gindex >= total )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    if ( !FT_NEW( snode ) )
    {
      FT_UInt  count, start;


      start = gindex - ( gindex % FTC_SBIT_ITEMS_PER_NODE );
      count = total - start;
      if ( count > FTC_SBIT_ITEMS_PER_NODE )
        count = FTC_SBIT_ITEMS_PER_NODE;

      FTC_GNode_Init( FTC_GNODE( snode ), start, family );

      snode->count = count;

      error = ftc_snode_load( snode,
                              cache->manager,
                              gindex,
                              NULL );
      if ( error )
      {
        FTC_SNode_Free( snode, cache );
        snode = NULL;
      }
    }

  Exit:
    *psnode = snode;
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_snode_new( FTC_Node   *ftcpsnode,
                 FT_Pointer  ftcgquery,
                 FTC_Cache   cache )
  {
    FTC_SNode  *psnode = (FTC_SNode*)ftcpsnode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;


    return FTC_SNode_New( psnode, gquery, cache );
  }


  FT_LOCAL_DEF( FT_ULong )
  ftc_snode_weight( FTC_Node   ftcsnode,
                    FTC_Cache  cache )
  {
    FTC_SNode  snode = (FTC_SNode)ftcsnode;
    FT_UInt    count = snode->count;
    FTC_SBit   sbit  = snode->sbits;
    FT_Int     pitch;
    FT_ULong   size;

    FT_UNUSED( cache );


    FT_ASSERT( snode->count <= FTC_SBIT_ITEMS_PER_NODE );

    /* the node itself */
    size = sizeof ( *snode );

    for ( ; count > 0; count--, sbit++ )
    {
      if ( sbit->buffer )
      {
        pitch = sbit->pitch;
        if ( pitch < 0 )
          pitch = -pitch;

        /* add the size of a given glyph image */
        size += pitch * sbit->height;
      }
    }

    return size;
  }


#if 0

  FT_LOCAL_DEF( FT_ULong )
  FTC_SNode_Weight( FTC_SNode  snode )
  {
    return ftc_snode_weight( FTC_NODE( snode ), NULL );
  }

#endif /* 0 */


  FT_LOCAL_DEF( FT_Bool )
  ftc_snode_compare( FTC_Node    ftcsnode,
                     FT_Pointer  ftcgquery,
                     FTC_Cache   cache )
  {
    FTC_SNode   snode  = (FTC_SNode)ftcsnode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;
    FTC_GNode   gnode  = FTC_GNODE( snode );
    FT_UInt     gindex = gquery->gindex;
    FT_Bool     result;


    result = FT_BOOL( gnode->family == gquery->family                    &&
                      (FT_UInt)( gindex - gnode->gindex ) < snode->count );
    if ( result )
    {
      /* check if we need to load the glyph bitmap now */
      FTC_SBit  sbit = snode->sbits + ( gindex - gnode->gindex );


      /*
       *  The following code illustrates what to do when you want to
       *  perform operations that may fail within a lookup function.
       *
       *  Here, we want to load a small bitmap on-demand; we thus
       *  need to call the `ftc_snode_load' function which may return
       *  a non-zero error code only when we are out of memory (OOM).
       *
       *  The correct thing to do is to use @FTC_CACHE_TRYLOOP and
       *  @FTC_CACHE_TRYLOOP_END in order to implement a retry loop
       *  that is capable of flushing the cache incrementally when
       *  an OOM errors occur.
       *
       *  However, we need to `lock' the node before this operation to
       *  prevent it from being flushed within the loop.
       *
       *  When we exit the loop, we unlock the node, then check the `error'
       *  variable.  If it is non-zero, this means that the cache was
       *  completely flushed and that no usable memory was found to load
       *  the bitmap.
       *
       *  We then prefer to return a value of 0 (i.e., NO MATCH).  This
       *  ensures that the caller will try to allocate a new node.
       *  This operation consequently _fail_ and the lookup function
       *  returns the appropriate OOM error code.
       *
       *  Note that `buffer == NULL && width == 255' is a hack used to
       *  tag `unavailable' bitmaps in the array.  We should never try
       *  to load these.
       *
       */

      if ( sbit->buffer == NULL && sbit->width != 255 )
      {
        FT_ULong  size;
        FT_Error  error;


        ftcsnode->ref_count++;  /* lock node to prevent flushing */
                                /* in retry loop                 */

        FTC_CACHE_TRYLOOP( cache )
        {
          error = ftc_snode_load( snode, cache->manager, gindex, &size );
        }
        FTC_CACHE_TRYLOOP_END();

        ftcsnode->ref_count--;  /* unlock the node */

        if ( error )
          result = 0;
        else
          cache->manager->cur_weight += size;
      }
    }

    return result;
  }


  FT_LOCAL_DEF( FT_Bool )
  FTC_SNode_Compare( FTC_SNode   snode,
                     FTC_GQuery  gquery,
                     FTC_Cache   cache )
  {
    return ftc_snode_compare( FTC_NODE( snode ), gquery, cache );
  }


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcbasic.c                                                             */
/*                                                                         */
/*    The FreeType basic cache interface (body).                           */
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


#include "ft2build.h"
#include FT_CACHE_H
#include "ftcglyph.h"
#include "ftcimage.h"
#include "ftcsbits.h"
#include FT_INTERNAL_MEMORY_H

#include "ftccback.h"
#include "ftcerror.h"


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /*
   *  These structures correspond to the FTC_Font and FTC_ImageDesc types
   *  that were defined in version 2.1.7.
   */
  typedef struct  FTC_OldFontRec_
  {
    FTC_FaceID  face_id;
    FT_UShort   pix_width;
    FT_UShort   pix_height;

  } FTC_OldFontRec, *FTC_OldFont;


  typedef struct  FTC_OldImageDescRec_
  {
    FTC_OldFontRec  font;
    FT_UInt32       flags;

  } FTC_OldImageDescRec, *FTC_OldImageDesc;


  /*
   *  Notice that FTC_OldImageDescRec and FTC_ImageTypeRec are nearly
   *  identical, bit-wise.  The only difference is that the `width' and
   *  `height' fields are expressed as 16-bit integers in the old structure,
   *  and as normal `int' in the new one.
   *
   *  We are going to perform a weird hack to detect which structure is
   *  being passed to the image and sbit caches.  If the new structure's
   *  `width' is larger than 0x10000, we assume that we are really receiving
   *  an FTC_OldImageDesc.
   */

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


  /*
   *  Basic Families
   *
   */
  typedef struct  FTC_BasicAttrRec_
  {
    FTC_ScalerRec  scaler;
    FT_UInt        load_flags;

  } FTC_BasicAttrRec, *FTC_BasicAttrs;

#define FTC_BASIC_ATTR_COMPARE( a, b )                                 \
          FT_BOOL( FTC_SCALER_COMPARE( &(a)->scaler, &(b)->scaler ) && \
                   (a)->load_flags == (b)->load_flags               )

#define FTC_BASIC_ATTR_HASH( a )                                   \
          ( FTC_SCALER_HASH( &(a)->scaler ) + 31*(a)->load_flags )


  typedef struct  FTC_BasicQueryRec_
  {
    FTC_GQueryRec     gquery;
    FTC_BasicAttrRec  attrs;

  } FTC_BasicQueryRec, *FTC_BasicQuery;


  typedef struct  FTC_BasicFamilyRec_
  {
    FTC_FamilyRec     family;
    FTC_BasicAttrRec  attrs;

  } FTC_BasicFamilyRec, *FTC_BasicFamily;


  FT_CALLBACK_DEF( FT_Bool )
  ftc_basic_family_compare( FTC_MruNode  ftcfamily,
                            FT_Pointer   ftcquery )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)ftcfamily;
    FTC_BasicQuery   query  = (FTC_BasicQuery)ftcquery;


    return FTC_BASIC_ATTR_COMPARE( &family->attrs, &query->attrs );
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_basic_family_init( FTC_MruNode  ftcfamily,
                         FT_Pointer   ftcquery,
                         FT_Pointer   ftccache )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)ftcfamily;
    FTC_BasicQuery   query  = (FTC_BasicQuery)ftcquery;
    FTC_Cache        cache  = (FTC_Cache)ftccache;


    FTC_Family_Init( FTC_FAMILY( family ), cache );
    family->attrs = query->attrs;
    return 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  ftc_basic_family_get_count( FTC_Family   ftcfamily,
                              FTC_Manager  manager )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)ftcfamily;
    FT_Error         error;
    FT_Face          face;
    FT_UInt          result = 0;


    error = FTC_Manager_LookupFace( manager, family->attrs.scaler.face_id,
                                    &face );
    if ( !error )
      result = face->num_glyphs;

    return result;
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_basic_family_load_bitmap( FTC_Family   ftcfamily,
                                FT_UInt      gindex,
                                FTC_Manager  manager,
                                FT_Face     *aface )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)ftcfamily;
    FT_Error         error;
    FT_Size          size;


    error = FTC_Manager_LookupSize( manager, &family->attrs.scaler, &size );
    if ( !error )
    {
      FT_Face  face = size->face;


      error = FT_Load_Glyph( face, gindex,
                             family->attrs.load_flags | FT_LOAD_RENDER );
      if ( !error )
        *aface = face;
    }

    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_basic_family_load_glyph( FTC_Family  ftcfamily,
                               FT_UInt     gindex,
                               FTC_Cache   cache,
                               FT_Glyph   *aglyph )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)ftcfamily;
    FT_Error         error;
    FTC_Scaler       scaler = &family->attrs.scaler;
    FT_Face          face;
    FT_Size          size;


    /* we will now load the glyph image */
    error = FTC_Manager_LookupSize( cache->manager,
                                    scaler,
                                    &size );
    if ( !error )
    {
      face = size->face;

      error = FT_Load_Glyph( face, gindex, family->attrs.load_flags );
      if ( !error )
      {
        if ( face->glyph->format == FT_GLYPH_FORMAT_BITMAP  ||
             face->glyph->format == FT_GLYPH_FORMAT_OUTLINE )
        {
          /* ok, copy it */
          FT_Glyph  glyph;


          error = FT_Get_Glyph( face->glyph, &glyph );
          if ( !error )
          {
            *aglyph = glyph;
            goto Exit;
          }
        }
        else
          error = FTC_Err_Invalid_Argument;
      }
    }

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_basic_gnode_compare_faceid( FTC_Node    ftcgnode,
                                  FT_Pointer  ftcface_id,
                                  FTC_Cache   cache )
  {
    FTC_GNode        gnode   = (FTC_GNode)ftcgnode;
    FTC_FaceID       face_id = (FTC_FaceID)ftcface_id;
    FTC_BasicFamily  family  = (FTC_BasicFamily)gnode->family;
    FT_Bool          result;


    result = FT_BOOL( family->attrs.scaler.face_id == face_id );
    if ( result )
    {
      /* we must call this function to avoid this node from appearing
       * in later lookups with the same face_id!
       */
      FTC_GNode_UnselectFamily( gnode, cache );
    }
    return result;
  }


 /*
  *
  * basic image cache
  *
  */

  FT_CALLBACK_TABLE_DEF
  const FTC_IFamilyClassRec  ftc_basic_image_family_class =
  {
    {
      sizeof ( FTC_BasicFamilyRec ),
      ftc_basic_family_compare,
      ftc_basic_family_init,
      0,                        /* FTC_MruNode_ResetFunc */
      0                         /* FTC_MruNode_DoneFunc  */
    },
    ftc_basic_family_load_glyph
  };


  FT_CALLBACK_TABLE_DEF
  const FTC_GCacheClassRec  ftc_basic_image_cache_class =
  {
    {
      ftc_inode_new,
      ftc_inode_weight,
      ftc_gnode_compare,
      ftc_basic_gnode_compare_faceid,
      ftc_inode_free,

      sizeof ( FTC_GCacheRec ),
      ftc_gcache_init,
      ftc_gcache_done
    },
    (FTC_MruListClass)&ftc_basic_image_family_class
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_New( FTC_Manager      manager,
                      FTC_ImageCache  *acache )
  {
    return FTC_GCache_New( manager, &ftc_basic_image_cache_class,
                           (FTC_GCache*)acache );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_Lookup( FTC_ImageCache  cache,
                         FTC_ImageType   type,
                         FT_UInt         gindex,
                         FT_Glyph       *aglyph,
                         FTC_Node       *anode )
  {
    FTC_BasicQueryRec  query;
    FTC_INode          node = 0;  /* make compiler happy */
    FT_Error           error;
    FT_UInt32          hash;


    /* some argument checks are delayed to FTC_Cache_Lookup */
    if ( !aglyph )
    {
      error = FTC_Err_Invalid_Argument;
      goto Exit;
    }

    *aglyph = NULL;
    if ( anode )
      *anode  = NULL;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    /*
     *  This one is a major hack used to detect whether we are passed a
     *  regular FTC_ImageType handle, or a legacy FTC_OldImageDesc one.
     */
    if ( type->width >= 0x10000 )
    {
      FTC_OldImageDesc  desc = (FTC_OldImageDesc)type;


      query.attrs.scaler.face_id = desc->font.face_id;
      query.attrs.scaler.width   = desc->font.pix_width;
      query.attrs.scaler.height  = desc->font.pix_height;
      query.attrs.load_flags     = desc->flags;
    }
    else

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */

    {
      query.attrs.scaler.face_id = type->face_id;
      query.attrs.scaler.width   = type->width;
      query.attrs.scaler.height  = type->height;
      query.attrs.load_flags     = type->flags;
    }

    query.attrs.scaler.pixel = 1;
    query.attrs.scaler.x_res = 0;  /* make compilers happy */
    query.attrs.scaler.y_res = 0;

    hash = FTC_BASIC_ATTR_HASH( &query.attrs ) + gindex;

#if 1  /* inlining is about 50% faster! */
    FTC_GCACHE_LOOKUP_CMP( cache,
                           ftc_basic_family_compare,
                           FTC_GNode_Compare,
                           hash, gindex,
                           &query,
                           node,
                           error );
#else
    error = FTC_GCache_Lookup( FTC_GCACHE( cache ),
                               hash, gindex,
                               FTC_GQUERY( &query ),
                               (FTC_Node*) &node );
#endif
    if ( !error )
    {
      *aglyph = FTC_INODE( node )->glyph;

      if ( anode )
      {
        *anode = FTC_NODE( node );
        FTC_NODE( node )->ref_count++;
      }
    }

  Exit:
    return error;
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /* yet another backwards-legacy structure */
  typedef struct  FTC_OldImage_Desc_
  {
    FTC_FontRec  font;
    FT_UInt      image_type;

  } FTC_OldImage_Desc;


#define FTC_OLD_IMAGE_FORMAT( x )  ( (x) & 7 )


#define ftc_old_image_format_bitmap    0x0000
#define ftc_old_image_format_outline   0x0001

#define ftc_old_image_format_mask      0x000F

#define ftc_old_image_flag_monochrome  0x0010
#define ftc_old_image_flag_unhinted    0x0020
#define ftc_old_image_flag_autohinted  0x0040
#define ftc_old_image_flag_unscaled    0x0080
#define ftc_old_image_flag_no_sbits    0x0100

  /* monochrome bitmap */
#define ftc_old_image_mono             ftc_old_image_format_bitmap   | \
                                       ftc_old_image_flag_monochrome

  /* anti-aliased bitmap */
#define ftc_old_image_grays            ftc_old_image_format_bitmap

  /* scaled outline */
#define ftc_old_image_outline          ftc_old_image_format_outline


  static void
  ftc_image_type_from_old_desc( FTC_ImageType       typ,
                                FTC_OldImage_Desc*  desc )
  {
    typ->face_id = desc->font.face_id;
    typ->width   = desc->font.pix_width;
    typ->height  = desc->font.pix_height;

    /* convert image type flags to load flags */
    {
      FT_UInt  load_flags = FT_LOAD_DEFAULT;
      FT_UInt  type       = desc->image_type;


      /* determine load flags, depending on the font description's */
      /* image type                                                */

      if ( FTC_OLD_IMAGE_FORMAT( type ) == ftc_old_image_format_bitmap )
      {
        if ( type & ftc_old_image_flag_monochrome )
          load_flags |= FT_LOAD_MONOCHROME;

        /* disable embedded bitmaps loading if necessary */
        if ( type & ftc_old_image_flag_no_sbits )
          load_flags |= FT_LOAD_NO_BITMAP;
      }
      else
      {
        /* we want an outline, don't load embedded bitmaps */
        load_flags |= FT_LOAD_NO_BITMAP;

        if ( type & ftc_old_image_flag_unscaled )
          load_flags |= FT_LOAD_NO_SCALE;
      }

      /* always render glyphs to bitmaps */
      load_flags |= FT_LOAD_RENDER;

      if ( type & ftc_old_image_flag_unhinted )
        load_flags |= FT_LOAD_NO_HINTING;

      if ( type & ftc_old_image_flag_autohinted )
        load_flags |= FT_LOAD_FORCE_AUTOHINT;

      typ->flags = load_flags;
    }
  }


  FT_EXPORT( FT_Error )
  FTC_Image_Cache_New( FTC_Manager      manager,
                       FTC_ImageCache  *acache );

  FT_EXPORT( FT_Error )
  FTC_Image_Cache_Lookup( FTC_ImageCache      icache,
                          FTC_OldImage_Desc*  desc,
                          FT_UInt             gindex,
                          FT_Glyph           *aglyph );


  FT_EXPORT_DEF( FT_Error )
  FTC_Image_Cache_New( FTC_Manager      manager,
                       FTC_ImageCache  *acache )
  {
    return FTC_ImageCache_New( manager, (FTC_ImageCache*)acache );
  }



  FT_EXPORT_DEF( FT_Error )
  FTC_Image_Cache_Lookup( FTC_ImageCache      icache,
                          FTC_OldImage_Desc*  desc,
                          FT_UInt             gindex,
                          FT_Glyph           *aglyph )
  {
    FTC_ImageTypeRec  type0;


    if ( !desc )
      return FTC_Err_Invalid_Argument;

    ftc_image_type_from_old_desc( &type0, desc );

    return FTC_ImageCache_Lookup( (FTC_ImageCache)icache,
                                   &type0,
                                   gindex,
                                   aglyph,
                                   NULL );
  }

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


 /*
  *
  * basic small bitmap cache
  *
  */


  FT_CALLBACK_TABLE_DEF
  const FTC_SFamilyClassRec  ftc_basic_sbit_family_class =
  {
    {
      sizeof( FTC_BasicFamilyRec ),
      ftc_basic_family_compare,
      ftc_basic_family_init,
      0,                            /* FTC_MruNode_ResetFunc */
      0                             /* FTC_MruNode_DoneFunc  */
    },
    ftc_basic_family_get_count,
    ftc_basic_family_load_bitmap
  };


  FT_CALLBACK_TABLE_DEF
  const FTC_GCacheClassRec  ftc_basic_sbit_cache_class =
  {
    {
      ftc_snode_new,
      ftc_snode_weight,
      ftc_snode_compare,
      ftc_basic_gnode_compare_faceid,
      ftc_snode_free,

      sizeof ( FTC_GCacheRec ),
      ftc_gcache_init,
      ftc_gcache_done
    },
    (FTC_MruListClass)&ftc_basic_sbit_family_class
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_New( FTC_Manager     manager,
                     FTC_SBitCache  *acache )
  {
    return FTC_GCache_New( manager, &ftc_basic_sbit_cache_class,
                           (FTC_GCache*)acache );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_Lookup( FTC_SBitCache  cache,
                        FTC_ImageType  type,
                        FT_UInt        gindex,
                        FTC_SBit      *ansbit,
                        FTC_Node      *anode )
  {
    FT_Error           error;
    FTC_BasicQueryRec  query;
    FTC_SNode          node = 0; /* make compiler happy */
    FT_UInt32          hash;


    if ( anode )
      *anode = NULL;

    /* other argument checks delayed to FTC_Cache_Lookup */
    if ( !ansbit )
      return FTC_Err_Invalid_Argument;

    *ansbit = NULL;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    /*  This one is a major hack used to detect whether we are passed a
     *  regular FTC_ImageType handle, or a legacy FTC_OldImageDesc one.
     */
    if ( type->width >= 0x10000 )
    {
      FTC_OldImageDesc  desc = (FTC_OldImageDesc)type;


      query.attrs.scaler.face_id = desc->font.face_id;
      query.attrs.scaler.width   = desc->font.pix_width;
      query.attrs.scaler.height  = desc->font.pix_height;
      query.attrs.load_flags     = desc->flags;
    }
    else

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */

    {
      query.attrs.scaler.face_id = type->face_id;
      query.attrs.scaler.width   = type->width;
      query.attrs.scaler.height  = type->height;
      query.attrs.load_flags     = type->flags;
    }

    query.attrs.scaler.pixel = 1;
    query.attrs.scaler.x_res = 0;  /* make compilers happy */
    query.attrs.scaler.y_res = 0;

    /* beware, the hash must be the same for all glyph ranges! */
    hash = FTC_BASIC_ATTR_HASH( &query.attrs ) +
           gindex / FTC_SBIT_ITEMS_PER_NODE;

#if 1  /* inlining is about 50% faster! */
    FTC_GCACHE_LOOKUP_CMP( cache,
                           ftc_basic_family_compare,
                           FTC_SNode_Compare,
                           hash, gindex,
                           &query,
                           node,
                           error );
#else
    error = FTC_GCache_Lookup( FTC_GCACHE( cache ),
                               hash,
                               gindex,
                               FTC_GQUERY( &query ),
                               (FTC_Node*)&node );
#endif
    if ( error )
      goto Exit;

    *ansbit = node->sbits + ( gindex - FTC_GNODE( node )->gindex );

    if ( anode )
    {
      *anode = FTC_NODE( node );
      FTC_NODE( node )->ref_count++;
    }

  Exit:
    return error;
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_EXPORT( FT_Error )
  FTC_SBit_Cache_New( FTC_Manager     manager,
                      FTC_SBitCache  *acache );

  FT_EXPORT( FT_Error )
  FTC_SBit_Cache_Lookup( FTC_SBitCache       cache,
                         FTC_OldImage_Desc*  desc,
                         FT_UInt             gindex,
                         FTC_SBit           *ansbit );


  FT_EXPORT_DEF( FT_Error )
  FTC_SBit_Cache_New( FTC_Manager     manager,
                      FTC_SBitCache  *acache )
  {
    return FTC_SBitCache_New( manager, (FTC_SBitCache*)acache );
  }


  FT_EXPORT_DEF( FT_Error )
  FTC_SBit_Cache_Lookup( FTC_SBitCache       cache,
                         FTC_OldImage_Desc*  desc,
                         FT_UInt             gindex,
                         FTC_SBit           *ansbit )
  {
    FTC_ImageTypeRec  type0;


    if ( !desc )
      return FT_Err_Invalid_Argument;

    ftc_image_type_from_old_desc( &type0, desc );

    return FTC_SBitCache_Lookup( (FTC_SBitCache)cache,
                                  &type0,
                                  gindex,
                                  ansbit,
                                  NULL );
  }

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


/* END */


/* END */

/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2014  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.26 - Graphical user interface for embedded applications **
emWin is protected by international copyright laws.   Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with a license and should not be re-
distributed in any way. We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : GUI_TTF.c
Purpose     : Implementation of external binary fonts
---------------------------END-OF-HEADER------------------------------
*/

#include "ft2build.h"
#include FT_FREETYPE_H

#include FT_MODULE_H

#include FT_CACHE_H
#include FT_CACHE_MANAGER_H

#include "GUI_Private.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define FAMILY_NAME 0
#define STYLE_NAME  1

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  FT_Library        library;           // the FreeType library
  FTC_Manager       cache_manager;     // the cache manager
  FTC_ImageCache    image_cache;       // the glyph image cache
  FTC_SBitCache     sbits_cache;       // the glyph small bitmaps cache
  FTC_CMapCache     cmap_cache;        // the charmap cache
} FT_CONTEXT;

typedef struct {
  unsigned MaxFaces; // If not set the default will be 2
  unsigned MaxSizes; // If not set the default will be 4
  U32      MaxBytes; // If not set the default will be 200000L
} TF_CACHE_SIZE;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
FT_CONTEXT    _FTContext;
TF_CACHE_SIZE _FTCacheSize;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _cbFaceRequester
*
*  Function description
*    The face requester callback function is used by the cache manager
*    to translate a given FTC_FaceID into a new valid FT_Face object, on demand.
*
*  Parameters
*    face_id      - Id to be translated into a real face object.
*    library      - Handle to a FreeType library object (same as _FTContext.library).
*    pRequestData - Pointer given by FTC_Manager_New() that is passed to the requester each time it is called.
*    pFace        - Pointer to FT_Face object.
*
*  Return value
*    0 on success, != 0 on error.
*/
static FT_Error _cbFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer * pRequestData, FT_Face * pFace) {
  GUI_TTF_DATA * pTTF;
  GUI_USE_PARA(pRequestData);
  pTTF = (GUI_TTF_DATA *)face_id;
  return FT_New_Memory_Face(library, (const FT_Byte *)pTTF->pData, pTTF->NumBytes, 0, pFace);
}

/*********************************************************************
*
*       _CheckInit
*
*  Function description
*    Initializes TrueType-engine and caches.
*/
static int _CheckInit(void) {
  //
  // Initialize library (on demand)
  //
  if (!_FTContext.library) {
    if (FT_Init_FreeType(&_FTContext.library)) {
      return 1; // Could not initialize FreeType
    }
  }
  //
  // Initialize cache manager
  //
  if (!_FTContext.cache_manager) {
    if (FTC_Manager_New(_FTContext.library,
                        _FTCacheSize.MaxFaces,
                        _FTCacheSize.MaxSizes,
                        _FTCacheSize.MaxBytes,
                        (FTC_Face_Requester)_cbFaceRequester,
                        NULL, &_FTContext.cache_manager)) {
      return 1; // Could not initialize cache manager
    }
    //
    // Initialize charmap cache
    //
    if (FTC_CMapCache_New(_FTContext.cache_manager, &_FTContext.cmap_cache)) {
      return 1; // Could not initialize charmap cache
    }
    //
    // Initialize glyph image cache
    //
    if (FTC_ImageCache_New(_FTContext.cache_manager, &_FTContext.image_cache)) {
      return 1; // Could not initialize glyph image cache
    }
    //
    // Initialize bitmap cache
    //
    if (FTC_SBitCache_New(_FTContext.cache_manager, &_FTContext.sbits_cache)) {
      return 1; // Could not initialize small bitmaps cache
    }
  }
  return 0;
}

/*********************************************************************
*
*       _RequestGlyph
*
*  Function description
*    Draws a glyph (if DoRender == 1) and returns its width
*/
static int _RequestGlyph(U16P c, unsigned DoRender) {
  GUI_TTF_CS       * pCS;
  FTC_ImageTypeRec * pImageType;
  FTC_FaceID         face_id;
  FT_Face            face;
  FT_UInt            glyph_index;
  FTC_SBitRec      * pSBit;
  FTC_ScalerRec      scaler;
  FT_Size            size;
  int                r;
  
  r = -1;
  //
  // Get object pointer
  //
  pCS        = (GUI_TTF_CS *)GUI_pContext->pAFont->p.pFontData;
  face_id    = (FTC_FaceID)pCS->pTTF;
  pImageType = (FTC_ImageTypeRec *)pCS->aImageTypeBuffer;
  //
  // Request face object from cache
  //
  if (FTC_Manager_LookupFace(_FTContext.cache_manager, face_id, &face)) {
    return r;
  }
  //
  // Request size object from cache
  //
  scaler.face_id = face_id;
  scaler.width   = pImageType->width;
  scaler.height  = pImageType->height;
  scaler.pixel   = 1;
  if (FTC_Manager_LookupSize(_FTContext.cache_manager, &scaler, &size)) {
    return r;
  }
  //
  // Request glyph index from cache
  //
  glyph_index = FTC_CMapCache_Lookup(_FTContext.cmap_cache, face_id, 0, c);
  //
  // Request bitmap from cache
  //
  if (FTC_SBitCache_Lookup(_FTContext.sbits_cache, pImageType, glyph_index, &pSBit, NULL)) {
    return r;
  }
  if (pSBit->buffer) {
    if (DoRender) {
      //
      // Rendering cache data using the bitmap routine
      //
      LCD_DrawBitmap(GUI_pContext->DispPosX + pSBit->left,
                     GUI_pContext->DispPosY - pSBit->top + GUI_pContext->pAFont->Baseline,
                     pSBit->width,
                     pSBit->height,
                     1, 1, 1,
                     pSBit->pitch,
                     pSBit->buffer,
                     GUI_pContext->LCD_pBkColorIndex);
    }
    r = pSBit->xadvance;
  } else {
    //
    // No bitmap data
    //
    pImageType->flags = FT_LOAD_DEFAULT;
    if (FTC_SBitCache_Lookup(_FTContext.sbits_cache, pImageType, glyph_index, &pSBit, NULL)) {
      return r;
    }
    pImageType->flags = FT_LOAD_MONOCHROME;
    r = face->glyph->metrics.horiAdvance >> 6;
  }
  return r;
}

/*********************************************************************
*
*       _DispChar
*/
static void _DispChar(U16P c) {
  int xDist;
  xDist = _RequestGlyph(c, 1);
  if (xDist >= 0) {
    GUI_pContext->DispPosX += xDist;
  }
}

/*********************************************************************
*
*       _RequestGlyphAA
*
*  Function description
*    Draws a glyph (if DoRender == 1) and returns its width
*/
static int _RequestGlyphAA(U16P c, unsigned DoRender) {
  GUI_TTF_CS       * pCS;
  FTC_ImageTypeRec * pImageType;
  FTC_FaceID         face_id;
  FT_Face            face;
  FT_UInt            glyph_index;
  FTC_SBitRec      * pSBit;
  FTC_ScalerRec      scaler;
  FT_Size            size;
  int                r;
  
  r = -1;
  //
  // Get object pointer
  //
  pCS        = (GUI_TTF_CS *)GUI_pContext->pAFont->p.pFontData;
  face_id    = (FTC_FaceID)pCS->pTTF;
  pImageType = (FTC_ImageTypeRec *)pCS->aImageTypeBuffer;
  //
  // Request face object from cache
  //
  if (FTC_Manager_LookupFace(_FTContext.cache_manager, face_id, &face)) {
    return r;
  }
  //
  // Request size object from cache
  //
  scaler.face_id = face_id;
  scaler.width   = pImageType->width;
  scaler.height  = pImageType->height;
  scaler.pixel   = 1;
  if (FTC_Manager_LookupSize(_FTContext.cache_manager, &scaler, &size)) {
    return r;
  }
  //
  // Request glyph index from cache
  //
  glyph_index = FTC_CMapCache_Lookup(_FTContext.cmap_cache, face_id, 0, c);
  //
  // Request bitmap from cache
  //
  if (FTC_SBitCache_Lookup(_FTContext.sbits_cache, pImageType, glyph_index, &pSBit, NULL)) {
    return r;
  }
  if (pSBit->buffer) {
    if (DoRender) {
      //
      // Rendering cache data using the bitmap routine
      //
      if (GUI_pLCD_APIList) {
        GUI_pLCD_APIList->pfDrawBitmap(
                            GUI_pContext->DispPosX + pSBit->left,                                 // XPos
                            GUI_pContext->DispPosY - pSBit->top + GUI_pContext->pAFont->Baseline, // YPos
                            pSBit->width,                                                         // XSize
                            pSBit->height,                                                        // YSize
                            1, 1, 8,
                            pSBit->pitch,                                                         // BytesPerLine
                            (U8 const *)pSBit->buffer,                                            // Pointer to pixel data
                            NULL);
      } else {
        GUI_AA__DrawCharAA8(GUI_pContext->DispPosX + pSBit->left,                                 // XPos
                            GUI_pContext->DispPosY - pSBit->top + GUI_pContext->pAFont->Baseline, // YPos
                            pSBit->width,                                                         // XSize
                            pSBit->height,                                                        // YSize
                            pSBit->pitch,                                                         // BytesPerLine
                            (U8 const *)pSBit->buffer);                                           // Pointer to pixel data
      }
    }
    r = pSBit->xadvance;
  } else {
    //
    // No bitmap data
    //
    pImageType->flags = FT_LOAD_DEFAULT;
    if (FTC_SBitCache_Lookup(_FTContext.sbits_cache, pImageType, glyph_index, &pSBit, NULL)) {
      return r;
    }
    pImageType->flags = 0;
    r = face->glyph->metrics.horiAdvance >> 6;
  }
  return r;
}

/*********************************************************************
*
*       _DispCharAA
*/
static void _DispCharAA(U16P c) {
  int xDist;
  xDist = _RequestGlyphAA(c, 1);
  if (xDist >= 0) {
    GUI_pContext->DispPosX += xDist;
  }
}

/*********************************************************************
*
*       _ClearLine
*
*  Function description
*    If text should be rendered not in transparent mode first the whole line
*    needs to be cleared, because internally the characters always are drawn in
*    transparent mode to be sure, that also compound characters are drawn well.
*/
static void _ClearLine(const char * s, int Len) {
  int       xDist;
  int       yDist;
  int       xSize;
  U16       c;

  xDist    = 0;
  yDist    = GUI_pContext->pAFont->YDist * GUI_pContext->pAFont->YMag;
  c        = 0;
  while (--Len >= 0) {
    c     = GUI_UC__GetCharCodeInc(&s);
    xSize = _RequestGlyph(c, 0);
    if (xSize >= 0) {
      xDist += xSize;
    }
  }
  GUI__ClearTextBackground(xDist, yDist);
}

/*********************************************************************
*
*       _ClearLineAA
*
*  Function description
*    If text should be rendered not in transparent mode first the whole line
*    needs to be cleared, because internally the characters always are drawn in
*    transparent mode to be sure, that also compound characters are drawn well.
*/
static void _ClearLineAA(const char * s, int Len) {
  int       xDist;
  int       yDist;
  int       xSize;
  U16       c;

  xDist    = 0;
  yDist    = GUI_pContext->pAFont->YDist * GUI_pContext->pAFont->YMag;
  c        = 0;
  while (--Len >= 0) {
    c     = GUI_UC__GetCharCodeInc(&s);
    xSize = _RequestGlyphAA(c, 0);
    if (xSize >= 0) {
      xDist += xSize;
    }
  }
  GUI__ClearTextBackground(xDist, yDist);
}

/*********************************************************************
*
*       _DispLine
*
*  Function description
*    Displays a string. If current text mode is not transparent, the
*    line is cleared before.
*/
static void _DispLine(const char * s, int Len) {
  U16 Char;
  int OldMode;

  if (Len > 0) {
    //
    // Clear if not transparency mode has been selected
    //
    if (!(GUI_pContext->TextMode & (LCD_DRAWMODE_TRANS | LCD_DRAWMODE_XOR))) {
      _ClearLine(s, Len);
    }
    //
    // Draw characters always transparent
    //
    OldMode = GUI_pContext->TextMode; // ??? should be ->DrawMode instead of ->TextMode ???
    GUI_pContext->DrawMode |= GUI_DM_TRANS;
    while (--Len >= 0) {
      Char = GUI_UC__GetCharCodeInc(&s);
      GUI_pContext->pAFont->pfDispChar(Char);
    }
    GUI_pContext->DrawMode = OldMode;
  }
}

/*********************************************************************
*
*       _APIList
*/
static const tGUI_ENC_APIList _APIList = {
  NULL,
  NULL,
  _DispLine
};

/*********************************************************************
*
*       _DispLineAA
*
*  Function description
*    Displays a string of anti aliased characters. If current text mode
*    is not transparent, the line is cleared before drawing the text.
*/
static void _DispLineAA(const char * s, int Len) {
  U16 Char;
  int OldMode;

  if (Len > 0) {
    //
    // Clear if not transparency mode has been selected
    //
    if (!(GUI_pContext->TextMode & (LCD_DRAWMODE_TRANS | LCD_DRAWMODE_XOR))) {
      _ClearLineAA(s, Len);
    }
    //
    // Draw characters
    //
    OldMode = GUI_pContext->DrawMode;
    GUI_pContext->DrawMode = GUI_pContext->TextMode;
    while (--Len >= 0) {
      Char = GUI_UC__GetCharCodeInc(&s);
      GUI_pContext->pAFont->pfDispChar(Char);
    }
    GUI_pContext->DrawMode = OldMode;
  }
}

/*********************************************************************
*
*       _APIListAA
*/
static const tGUI_ENC_APIList _APIListAA = {
  NULL,
  NULL,
  _DispLineAA
};

/*********************************************************************
*
*       _GetCharDistX
*/
#if (GUI_VERSION < 50801)
static int _GetCharDistX(U16P c) {
  int xDist;
  xDist = _RequestGlyph(c, 0);
  return (xDist >= 0) ? xDist : 0;
}
#else
static int _GetCharDistX(U16P c, int * pSizeX) {
  int xDist;
  xDist = _RequestGlyph(c, 0);
  if (pSizeX) {
    *pSizeX = xDist; // Same as xDist here...
  }
  return (xDist >= 0) ? xDist : 0;
}
#endif

/*********************************************************************
*
*       _GetCharDistX_AA
*/
static int _GetCharDistX_AA(U16P c, int * pSizeX) {
  int xDist;
  xDist = _RequestGlyphAA(c, 0);
  if (pSizeX) {
    *pSizeX = xDist; // Same as xDist here...
  }
  return (xDist >= 0) ? xDist : 0;
}

/*********************************************************************
*
*       _GetFontInfo
*/
static void _GetFontInfo(const GUI_FONT * pFont, GUI_FONTINFO * pfi) {
  pfi->Baseline = pFont->Baseline;
  pfi->LHeight  = pFont->LHeight;
  pfi->CHeight  = pFont->CHeight;
  pfi->Flags    = GUI_FONTINFO_FLAG_PROP;
}

/*********************************************************************
*
*       _IsInFont
*/
static char _IsInFont(const GUI_FONT * pFont, U16 c) {
  FTC_FaceID face_id;
  FT_Face    face;
  FT_UInt    glyph_index;
  
  //
  // Get object pointer
  //
  face_id    = (FTC_FaceID)pFont->p.pFontData;
  //
  // Request face object from cache
  //
  if (FTC_Manager_LookupFace(_FTContext.cache_manager, face_id, &face)) {
    return 1;
  }
  glyph_index = FTC_CMapCache_Lookup(_FTContext.cmap_cache, face_id, 0, c);
  return glyph_index ? 1 : 0;
}

/*********************************************************************
*
*       _GetName
*/
static int _GetName(GUI_FONT * pFont, char * pBuffer, int NumBytes, int Index) {
  FTC_FaceID   face_id;
  FT_Face      face;
  int          Len;
  const char * pName;
  
  //
  // Get object pointer
  //
  face_id = (FTC_FaceID)((GUI_TTF_CS *)pFont->p.pFontData)->pTTF;
  //
  // Request face object from cache
  //
  if (FTC_Manager_LookupFace(_FTContext.cache_manager, face_id, &face)) {
    return 1;
  }
  switch (Index) {
  case FAMILY_NAME:
    pName = face->family_name;
    break;
  case STYLE_NAME:
    pName = face->style_name;
    break;
  }
  Len = strlen(pName);
  if (Len >= NumBytes) {
    Len = NumBytes - 1;
  }
  strncpy(pBuffer, pName, Len);
  *(pBuffer + Len) = 0;
  return 0;
}

/*********************************************************************
*
*       _CreateFont
*/
static int _CreateFont(GUI_FONT * pFont, GUI_TTF_CS * pCS, FT_Int Flags, void (* pfDispChar)(U16), const tGUI_ENC_APIList * pAPIList, int (* pfGetCharDistX)(U16P, int *)) {
  FTC_ImageTypeRec * pImageType;
  FTC_FaceID         face_id;
  FTC_ScalerRec      scaler;
  FT_Face            face;
  FT_Size            size;
  FT_GlyphSlot       slot;
  FT_Glyph           glyph;
  FT_UInt            glyph_index;
  
  //
  // Check initialization
  //
  _CheckInit();
  //
  // face_id is nothing but the address of the font file
  //
  face_id = (FTC_FaceID)pCS->pTTF;
  //
  // Request face object from cache
  //
  if (FTC_Manager_LookupFace(_FTContext.cache_manager, face_id, &face)) {
    return 1;
  }
  //
  // Set image type attributes
  //
  pImageType          = (FTC_ImageTypeRec *)pCS->aImageTypeBuffer;
  pImageType->face_id = face_id;
  pImageType->width   = 0;
  pImageType->height  = pCS->PixelHeight;
  pImageType->flags   = Flags;
  //
  // Request size object from cache
  //
  scaler.face_id = face_id;
  scaler.width   = pImageType->width;
  scaler.height  = pImageType->height;
  scaler.pixel   = 1;
  if (FTC_Manager_LookupSize(_FTContext.cache_manager, &scaler, &size)) {
    return 1;
  }
  //
  // Magnification is always 1
  //
  pFont->XMag = 1;
  pFont->YMag = 1;
  //
  // Set function pointers
  //
  pFont->p.pFontData    = pCS;
  pFont->pfDispChar     = pfDispChar;
  pFont->pfGetCharDistX = pfGetCharDistX;
  pFont->pfGetFontInfo  = _GetFontInfo;
  pFont->pfIsInFont     = _IsInFont;
  pFont->pafEncode      = pAPIList;
  //
  // Calculate baseline and vertical size
  //
  pFont->Baseline = face->size->metrics.ascender >> 6;
  pFont->YSize    = pFont->Baseline - (face->size->metrics.descender >> 6);
  pFont->YDist    = pFont->YSize;
  slot            = face->glyph;
  //
  // Calculate lowercase height
  //
  glyph_index = FTC_CMapCache_Lookup(_FTContext.cmap_cache, face_id, 0, 'g');
  if (FTC_ImageCache_Lookup(_FTContext.image_cache, pImageType, glyph_index, &glyph, NULL)) {
    return 1;
  }
  pFont->LHeight = slot->metrics.horiBearingY >> 6;
  //
  // Calculate capital height
  //
  glyph_index = FTC_CMapCache_Lookup(_FTContext.cmap_cache, face_id, 0, 'M');
  if (FTC_ImageCache_Lookup(_FTContext.image_cache, pImageType, glyph_index, &glyph, NULL)) {
    return 1;
  }
  pFont->CHeight = slot->metrics.height >> 6;
  //
  // Select the currently created font
  //
  GUI_SetFont(pFont);
  return 0;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       GUI_TTF_CreateFont
*
*  Function description
*    Creates a new GUI_FONT object by a given TTF font file available in memory
*    using 1bpp.
*
*  Parameters:
*    pFont      - Pointer to a GUI_FONT structure to be initialized by this function.
*    pCS        - Pointer to a GUI_TTF_CS structure which contains information about 
*                 file location, file size, pixel size and face index.
*/
int GUI_TTF_CreateFont(GUI_FONT * pFont, GUI_TTF_CS * pCS) {
  return _CreateFont(pFont, pCS, FT_LOAD_TARGET_MONO, _DispChar, &_APIList, _GetCharDistX);
}

/*********************************************************************
*
*       GUI_TTF_CreateFontAA
*
*  Function description
*    Creates a new GUI_FONT object by a given TTF font file available in memory
*    using 8bpp antialiasing.
*
*  Parameters:
*    pFont      - Pointer to a GUI_FONT structure to be initialized by this function.
*    pCS        - Pointer to a GUI_TTF_CS structure which contains information about 
*                 file location, file size, pixel size and face index.
*/
int GUI_TTF_CreateFontAA(GUI_FONT * pFont, GUI_TTF_CS * pCS) {
  return _CreateFont(pFont, pCS, 0, _DispCharAA, &_APIListAA, _GetCharDistX_AA);
}

/*********************************************************************
*
*       GUI_TTF_GetFamilyName
*/
int GUI_TTF_GetFamilyName(GUI_FONT * pFont, char * pBuffer, int NumBytes) {
  return _GetName(pFont, pBuffer, NumBytes, FAMILY_NAME);
}

/*********************************************************************
*
*       GUI_TTF_GetStyleName
*/
int GUI_TTF_GetStyleName(GUI_FONT * pFont, char * pBuffer, int NumBytes) {
  return _GetName(pFont, pBuffer, NumBytes, STYLE_NAME);
}

/*********************************************************************
*
*       GUI_TTF_SetCacheSize
*
*  Function description
*    Sets the maximum number of font faces, size objects and bytes used by the cache
*/
void GUI_TTF_SetCacheSize(unsigned MaxFaces, unsigned MaxSizes, U32 MaxBytes) {
  _FTCacheSize.MaxFaces = MaxFaces;
  _FTCacheSize.MaxSizes = MaxSizes;
  _FTCacheSize.MaxBytes = MaxBytes;
}

/*********************************************************************
*
*       GUI_TTF_DestroyCache
*
*  Function description
*    Destroys the FreeType font cache after emptying it.
*/
void GUI_TTF_DestroyCache(void) {
  if (_FTContext.cache_manager) {
    FTC_Manager_Done(_FTContext.cache_manager);
    _FTContext.cache_manager = 0;
  }
}

/*********************************************************************
*
*       GUI_TTF_Done
*/
void GUI_TTF_Done(void) {
  GUI_TTF_DestroyCache();
  if (_FTContext.library) {
    FT_Done_Library(_FTContext.library);
    _FTContext.library = 0;
  }
}

/*************************** End of file ****************************/

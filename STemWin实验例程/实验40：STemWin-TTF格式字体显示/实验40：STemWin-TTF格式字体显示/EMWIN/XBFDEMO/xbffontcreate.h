#ifndef _XBFFONTCREATE_H
#define _XBFFONTCREATE_H
#include "system.h"
#include "GUI.h"


extern GUI_FONT XBF12_Font;
extern GUI_FONT XBF16_Font;
extern GUI_FONT XBF24_Font;
extern GUI_FONT XBF36_Font;

u8 Create_XBF12(u8 *fxpath); 
u8 Create_XBF16(u8 *fxpath); 
u8 Create_XBF24(u8 *fxpath); 
u8 Create_XBF36(u8 *fxpath);
#endif

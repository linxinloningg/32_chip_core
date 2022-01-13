#ifndef _TTFFONTCREATE_H
#define _TTFFONTCREATE_H
#include "system.h"
#include "GUI.h"



extern GUI_FONT TTF12_Font;
extern GUI_FONT TTF18_Font;
extern GUI_FONT TTF24_Font;
extern GUI_FONT TTF36_Font;

int Create_TTFFont(u8 *fxpath);
void STemWin_TTFFont_Demo(void);


#endif

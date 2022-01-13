#include "Progbarwin_demo.h"
#include "GUI.h"
#include "PROGBAR.h"


/*******************************************************************
*
*       _DemoProgBar
*/
static void _DemoProgBar(void) {
  int i;
  PROGBAR_Handle ahProgBar[2];

  GUI_SetBkColor(GUI_WHITE);
  GUI_Clear();
  GUI_SetColor(GUI_RED);
  GUI_SetFont(&GUI_Font24_ASCII);
  GUI_DispStringHCenterAt("WIDGET_Progbar - Sample", 160, 5);
  GUI_SetFont(&GUI_Font8x16);
  GUI_DispStringAt("Progress bar", 100,80);
  //
  // Create the progbars
  //
  ahProgBar[0] = PROGBAR_Create(100,100,100,20, WM_CF_SHOW);
  ahProgBar[1] = PROGBAR_Create( 80,150,140,20, WM_CF_SHOW);
  //
  // Use memory device (optional, for better looks)
  //
	PROGBAR_EnableMemdev(ahProgBar[0]);
	PROGBAR_EnableMemdev(ahProgBar[1]);
  PROGBAR_SetMinMax(ahProgBar[1], 0, 500);
  PROGBAR_SetFont(ahProgBar[0], &GUI_Font8x16);
  GUI_Delay(500);
  while(1) {
    PROGBAR_SetFont(ahProgBar[0], &GUI_Font8x16);
    if (LCD_GetDevCap(LCD_DEVCAP_BITSPERPIXEL) <= 4) {
      PROGBAR_SetBarColor(ahProgBar[0], 0, GUI_DARKGRAY);
      PROGBAR_SetBarColor(ahProgBar[0], 1, GUI_LIGHTGRAY);
    } else 
	{
      PROGBAR_SetBarColor(ahProgBar[0], 0, GUI_GREEN);
      PROGBAR_SetBarColor(ahProgBar[0], 1, GUI_RED);
    }
    PROGBAR_SetTextAlign(ahProgBar[0], GUI_TA_HCENTER);
    PROGBAR_SetText(ahProgBar[0], NULL);
    for (i=0; i<=100; i++) {
      PROGBAR_SetValue(ahProgBar[0], i);
      PROGBAR_SetValue(ahProgBar[1], i);
      GUI_Delay(5);
    }
    PROGBAR_SetText(ahProgBar[0], "Tank empty");
    for (; i>=0; i--) {
      PROGBAR_SetValue(ahProgBar[0], i);
      PROGBAR_SetValue(ahProgBar[1], 200-i);
      GUI_Delay(5);
    }
    PROGBAR_SetText(ahProgBar[0], "Any text...");
    PROGBAR_SetTextAlign(ahProgBar[0], GUI_TA_LEFT);
    for (; i<=100; i++) {
      PROGBAR_SetValue(ahProgBar[0], i);
      PROGBAR_SetValue(ahProgBar[1], 200+i);
      GUI_Delay(5);
    }
    PROGBAR_SetTextAlign(ahProgBar[0], GUI_TA_RIGHT);
    for (; i>=0; i--) {
      PROGBAR_SetValue(ahProgBar[0], i);
      PROGBAR_SetValue(ahProgBar[1], 400-i);
      GUI_Delay(5);
    }
    PROGBAR_SetFont(ahProgBar[0], &GUI_FontComic18B_1);
    PROGBAR_SetText(ahProgBar[0], "Any font...");
    for (; i<=100; i++) {
      PROGBAR_SetValue(ahProgBar[0], i);
      PROGBAR_SetValue(ahProgBar[1], 400+i);
      GUI_Delay(5);
    }
    GUI_Delay(500);
  }
}



void STemWin_ProgbarWin_Test(void)
{
	while (1) 
	{
		_DemoProgBar();
	}
}	

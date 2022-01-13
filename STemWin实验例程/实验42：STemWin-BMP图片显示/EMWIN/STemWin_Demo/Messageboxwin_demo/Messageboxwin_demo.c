#include "Messageboxwin_demo.h"
#include "GUI.h"
#include "WM.h"


/*********************************************************************
*
*       _cbBkWindow
*/
static void _cbBkWindow(WM_MESSAGE* pMsg) {
  switch (pMsg->MsgId) {
  case WM_PAINT:
    GUI_SetBkColor(GUI_RED);
    GUI_Clear();
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font24_ASCII);
    GUI_DispStringHCenterAt("DIALOG_MessageBox - Sample", 160, 5);
    break;
  default:
    WM_DefaultProc(pMsg);
  }
}




void STemWin_MessageboxWin_Test(void)
{
	#if GUI_SUPPORT_MEMDEV
		WM_SetCreateFlags(WM_CF_MEMDEV);
	#endif
	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);
  //
  // Create message box and wait until it is closed
  //
	while (1) 
	{
		GUI_MessageBox("This text is shown\nin a message box",
					   "Caption/Title", GUI_MESSAGEBOX_CF_MOVEABLE);
		GUI_Delay(750);                    // Wait for a short moment ...
		GUI_MessageBox("New message !",
					   "Caption/Title", GUI_MESSAGEBOX_CF_MOVEABLE);
		GUI_Delay(750);
		
	}
}

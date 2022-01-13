#include "BUTTONBMPDLG.h"
#include "buttonbmp.h"
#include "BUTTON.h"
#include "led.h"
#include "beep.h"


#define ID_FRAMEWIN_0  (GUI_ID_USER + 0x0B)
#define ID_BUTTON_0  (GUI_ID_USER + 0x0C)
#define ID_TEXT_0  (GUI_ID_USER + 0x0D)
#define ID_TEXT_1  (GUI_ID_USER + 0x0E)
#define ID_BUTTON_1  (GUI_ID_USER + 0x0F)

GUI_BITMAP buttonbmp_tab[2];

static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, "Framewin", ID_FRAMEWIN_0, 0, 0, 320, 480, 0, 0x64, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_0, 80, 55, 110, 40, 0, 0x0, 0 },
  { TEXT_CreateIndirect, "Text", ID_TEXT_0, 5, 60, 80, 29, 0, 0x64, 0 },
  { TEXT_CreateIndirect, "Text", ID_TEXT_1, 0, 144, 80, 29, 0, 0x64, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_1, 80, 134, 110, 40, 0, 0x0, 0 },
  // USER START (Optionally insert additional widgets)
  // USER END
};


static void _cbDialog(WM_MESSAGE * pMsg) {
  WM_HWIN hItem;
  int     NCode;
  int     Id;
  static u8 ledflag=0;
  static u8 beepflag=0;

  switch (pMsg->MsgId) {

  case WM_PAINT:
		GUI_SetBkColor(GUI_WHITE);
		GUI_Clear();
		break;
		
  case WM_INIT_DIALOG:
    //
    // Initialization of 'Framewin'
    //
    hItem = pMsg->hWin;
    FRAMEWIN_SetTitleHeight(hItem, 40);
    FRAMEWIN_SetTextColor(hItem, 0x000000FF);
    FRAMEWIN_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
    FRAMEWIN_SetFont(hItem, GUI_FONT_24B_ASCII);
    FRAMEWIN_SetText(hItem, "BUTTON BMP TEST");
    //
    // Initialization of 'Text'
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_0);
    TEXT_SetFont(hItem, GUI_FONT_20B_ASCII);
    TEXT_SetText(hItem, "LED:");
    TEXT_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
    //
    // Initialization of 'Text'
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_1);
    TEXT_SetFont(hItem, GUI_FONT_20B_ASCII);
    TEXT_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
    TEXT_SetText(hItem, "BEEP:");
    // USER START (Optionally insert additional code for further widget initialization)
    // USER END

	//
    // Initialization of 'Button'
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_0);
    BUTTON_SetText(hItem, "");


	//
    // Initialization of 'Button'
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_1);
    BUTTON_SetText(hItem, "");
    // USER START (Optionally insert additional code for further widget initialization)
    // USER END

    break;

  case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
    switch(Id) {
    case ID_BUTTON_0: // Notifications sent by 'Button'

		hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_0);

      switch(NCode) {
      case WM_NOTIFICATION_CLICKED:
        // USER START (Optionally insert code for reacting on notification message)
        // USER END
        break;
      case WM_NOTIFICATION_RELEASED:
				led1=!led1;
			ledflag=!ledflag;
			BUTTON_SetBitmapEx(hItem,0,ledflag?&buttonbmp_tab[1]:&buttonbmp_tab[0],0,0);
	

        break;
      // USER START (Optionally insert additional code for further notification handling)
      // USER END
      }
      break;
    case ID_BUTTON_1: // Notifications sent by 'Button'

		hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_1);

      switch(NCode) {
      case WM_NOTIFICATION_CLICKED:
        // USER START (Optionally insert code for reacting on notification message)
        // USER END
        break;
      case WM_NOTIFICATION_RELEASED:
				beep=!beep;
			beepflag=~beepflag;
			BUTTON_SetBitmapEx(hItem,0,beepflag?&buttonbmp_tab[1]:&buttonbmp_tab[0],0,0);	

			
        break;
      // USER START (Optionally insert additional code for further notification handling)
      // USER END
      }
      break;
    // USER START (Optionally insert additional code for further Ids)
    // USER END
    }
    break;
  // USER START (Optionally insert additional message handling)
  // USER END
  default:
    WM_DefaultProc(pMsg);
    break;
  }
}



WM_HWIN BUTTONBMP_CreateFramewin(void) 
{
  WM_HWIN hWin;
	buttonbmp_tab[0]=bmBUTTONOFF;
	buttonbmp_tab[1]=bmBUTTONON;
  hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
  return hWin;
}

void STemWin_ButtonBMP_Test(void)
{
	//¸ü»»Æ¤·ô
#if 1
	BUTTON_SetDefaultSkin(BUTTON_SKIN_FLEX); 
	CHECKBOX_SetDefaultSkin(CHECKBOX_SKIN_FLEX);
	DROPDOWN_SetDefaultSkin(DROPDOWN_SKIN_FLEX);
	FRAMEWIN_SetDefaultSkin(FRAMEWIN_SKIN_FLEX);
	HEADER_SetDefaultSkin(HEADER_SKIN_FLEX);
	MENU_SetDefaultSkin(MENU_SKIN_FLEX);
	MULTIPAGE_SetDefaultSkin(MULTIPAGE_SKIN_FLEX);
	PROGBAR_SetDefaultSkin(PROGBAR_SKIN_FLEX);
	RADIO_SetDefaultSkin(RADIO_SKIN_FLEX);
	SCROLLBAR_SetDefaultSkin(SCROLLBAR_SKIN_FLEX);
	SLIDER_SetDefaultSkin(SLIDER_SKIN_FLEX);
	SPINBOX_SetDefaultSkin(SPINBOX_SKIN_FLEX);
#endif
	BUTTONBMP_CreateFramewin();
	while(1)
	{
		GUI_Delay(10);
	}
}


#include "DropDownDLG.h"
#include "led.h"
#include "beep.h"


#define ID_FRAMEWIN_0  (GUI_ID_USER + 0x0D)
#define ID_DROPDOWN_0  (GUI_ID_USER + 0x0E)



static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, "Framewin", ID_FRAMEWIN_0, 0, 0, 320, 480, 0, 0x64, 0 },
  { DROPDOWN_CreateIndirect, "Dropdown", ID_DROPDOWN_0, 44, 40, 135, 22, 0, 0x0, 0 },
  
};


static void _cbDialog(WM_MESSAGE * pMsg) 
{
  WM_HWIN hItem;
  int     NCode;
  int     Id;
  int index;

  switch (pMsg->MsgId) 
  {
  case WM_INIT_DIALOG:
    //
    // Initialization of 'Framewin'
    //
    hItem = pMsg->hWin;
    FRAMEWIN_SetTitleHeight(hItem, 30);
    FRAMEWIN_SetText(hItem, "DropDowm Test");
    FRAMEWIN_SetFont(hItem, GUI_FONT_24B_ASCII);
    FRAMEWIN_SetTextColor(hItem, 0x000000FF);
    FRAMEWIN_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
    //
    // Initialization of 'Dropdown'
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0);
    DROPDOWN_AddString(hItem, "LED");
    DROPDOWN_AddString(hItem, "BEEP");

    DROPDOWN_SetListHeight(hItem, 50);
    DROPDOWN_SetFont(hItem, GUI_FONT_16B_ASCII);
//  DROPDOWN_Expand(hItem);
//	DROPDOWN_IncSelExp(hItem);
	DROPDOWN_SetAutoScroll(hItem,1);  //开启下拉滚动条
	DROPDOWN_SetItemSpacing(hItem,5);  //设置下拉项目之间间隔距离    
    break;


  	case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
	hItem = WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0);	
	switch(Id)
	{
		case ID_DROPDOWN_0:
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
   				   	
        			break;

				case WM_NOTIFICATION_SEL_CHANGED:
					
					index=DROPDOWN_GetSel(hItem);
					switch(index)
					{
						case 0: led2=!led2;break;
					   	case 1: beep=!beep;break;
					}	
        			break;	
			}
			break;
	}
    
  	default:
    WM_DefaultProc(pMsg);
    break;
  }
}



WM_HWIN DropDown_CreateFramewin(void) {
  WM_HWIN hWin;

  hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
  return hWin;
}


void STemWin_DropDown_Test(void)
{
	//更换皮肤
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
	DropDown_CreateFramewin();
	while(1)
	{
		GUI_Delay(10);
	}
}


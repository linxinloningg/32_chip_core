#include "EditDLG.h"
#include "GUI.h"
#include "WM.h"

#define WM_APP_SHOW_TEXT	(WM_USER +0)
#define TEXT_MAXLEN	40

WM_HWIN DialoghWin;

//对话框资源表
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = 
{
	{ FRAMEWIN_CreateIndirect, "Framewin", 0, 20, 130, 200, 110, FRAMEWIN_CF_MOVEABLE},
	{ EDIT_CreateIndirect, "Edit", GUI_ID_EDIT0, 24, 10, 145, 25, 0,15},
	{ BUTTON_CreateIndirect,"Ok", GUI_ID_OK,65,	50,	70,	30}
};

//背景窗口的回调函数
static void _BkCallback(WM_MESSAGE *pMsg)
{
	static WM_HWIN hWin;
	static WM_HWIN hEdit;
	static U8 text;
	char   buffer[TEXT_MAXLEN];
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(GUI_DARKGRAY);
			GUI_Clear();
			GUI_SetFont(&GUI_Font24_ASCII);
			GUI_DispStringHCenterAt("WIDGET_Edit - Sample", 115, 5);
			GUI_SetFont(&GUI_Font8x16);
			if (text) 
			{
				hEdit=WM_GetDialogItem(DialoghWin,GUI_ID_EDIT0);
				GUI_DispStringHCenterAt("String you have modified is:", 120, 90);
				EDIT_GetText(hEdit, buffer, TEXT_MAXLEN);
				GUI_DispStringHCenterAt(buffer, 100, 110);
			} 
			else
			{	
				GUI_DispStringHCenterAt("Use keyboard to modify string...", 150, 90);
			}
			break;	
		case WM_APP_SHOW_TEXT:
			hWin=pMsg->hWinSrc;
			WM_HideWindow(hWin);	//隐藏对话框
			text=1;
			WM_InvalidateWindow(WM_HBKWIN); //背景窗口无效
			WM_CreateTimer(WM_HBKWIN,0,3000,0);
			break;
		case WM_TIMER:
			text=0;
			WM_InvalidateWindow(WM_HBKWIN);
			WM_ShowWindow(hWin);
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}	
}


//对话框对调函数
static void _cbDialog(WM_MESSAGE * pMsg) 
{
	WM_HWIN hItem;
	int     NCode;
	int     Id;
	WM_MESSAGE Msg;

	switch (pMsg->MsgId) 
	{
		case WM_INIT_DIALOG:
			//初始化FRAMEWIN
			hItem = pMsg->hWin;
			FRAMEWIN_SetText(hItem, "EDIT USER");
			FRAMEWIN_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
			FRAMEWIN_SetFont(hItem, GUI_FONT_16B_ASCII);
			//初始化EDIT
			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0);
			EDIT_EnableBlink(hItem, 500, 1);	 //打开光标
			EDIT_SetText(hItem, "Edit EMWIN...");
			EDIT_SetFont(hItem, &GUI_Font20_ASCII);
			EDIT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
		
			break;
		case WM_NOTIFY_PARENT:
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;
			switch(Id) 
			{
				case GUI_ID_EDIT0:	//EDIT控件通知消息
					switch(NCode) 
					{
						case WM_NOTIFICATION_CLICKED:
							break;
						case WM_NOTIFICATION_RELEASED:
							break;
						case WM_NOTIFICATION_VALUE_CHANGED:
							break;
					}
					break;
				case GUI_ID_OK:
					switch(NCode)
					{
						case WM_NOTIFICATION_CLICKED:
							break;
						
						case WM_NOTIFICATION_RELEASED:
							Msg.MsgId=WM_APP_SHOW_TEXT;
							Msg.hWinSrc=pMsg->hWin;
							WM_SendMessage(WM_HBKWIN,&Msg);
							break;
		
					}		
			}
			break;
	default:
		WM_DefaultProc(pMsg);
		break;
	}
}

void Editwinmode_Demo(void) 
{
	WM_SetCallback(WM_HBKWIN,_BkCallback);
	DialoghWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
}


void STemWin_Edit_Test(void)
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
	Editwinmode_Demo();
	while(1)
	{
		GUI_Delay(10);
	}
}























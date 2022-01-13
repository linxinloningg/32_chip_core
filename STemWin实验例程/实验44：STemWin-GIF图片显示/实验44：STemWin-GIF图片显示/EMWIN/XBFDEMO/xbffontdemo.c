#include "xbffontdemo.h"
#include "xbffontcreate.h"
#include "DIALOG.h"


#define ID_FRAMEWIN_0 	(GUI_ID_USER + 0)
#define ID_MULTIEDIT_0 	(GUI_ID_USER + 1)


WM_HWIN hDialog;


char *MultiEdit_String	=  
"普中STM32开发板\r\n		\
CPU:STM32F103ZET6\r\n			\
FLASH:1M\r\n					\
RAM	 :64K\r\n				\
";
	
//对话框资源列表
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, 	"Framewin", 	ID_FRAMEWIN_0, 	0, 		0, 		320, 	480, 	0, FRAMEWIN_CF_MOVEABLE, 0 },
  { MULTIEDIT_CreateIndirect, 	"Multiedit", 	ID_MULTIEDIT_0, 20, 	20, 	260, 	160, 	0, 0x0, 0 },
};


//对话框窗口回调函数
static void _cbDialog(WM_MESSAGE * pMsg) 
{
	WM_HWIN hItem;
	int     NCode;
	int     Id;
	switch (pMsg->MsgId) 
	{
		case WM_INIT_DIALOG:
			
			//初始化FRAMEWIN
			hItem = pMsg->hWin;
			FRAMEWIN_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
			FRAMEWIN_SetTitleHeight(hItem, 30);
			FRAMEWIN_SetFont(hItem,&XBF16_Font);
			FRAMEWIN_SetTextColor(hItem, GUI_RED);
			FRAMEWIN_SetText(hItem, "普中STM32XBF汉字显示");
		
			//初始化MULTIEDIT
			hItem = WM_GetDialogItem(pMsg->hWin, ID_MULTIEDIT_0);
			MULTIEDIT_SetTextColor(hItem,MULTIEDIT_CI_EDIT,GUI_BLACK);
			MULTIEDIT_SetBkColor(hItem,MULTIEDIT_CI_EDIT,GUI_LIGHTCYAN);
			MULTIEDIT_SetFont(hItem,&XBF12_Font);
			MULTIEDIT_SetText(hItem,MultiEdit_String);
		
			break;
		case WM_NOTIFY_PARENT:
			Id = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;
			switch(Id) 
			{
				case ID_MULTIEDIT_0: 
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
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

void STemWin_XBFFont_Demo(void) 
{
	#if GUI_SUPPORT_MEMDEV
		WM_SetCreateFlags(WM_CF_MEMDEV);
	#endif
	
	hDialog = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
	GUI_Exec();
	while(1)
	{
		GUI_Delay(100);
	}
}

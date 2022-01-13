#include "MyFrameWinDLG.h"
#include "FRAMEWIN.h"
#include "string.h"
#include "GUI.h"
#include "DIALOG.h"

#define SPEED 1200
#define MSG_CHANGE_MAIN_TEXT (WM_USER+0)

static FRAMEWIN_Handle 	_hFrame;
static WM_CALLBACK*		_pcbOldFrame;
static char				_acMainText[100];
static int				_LockClose=1;

//改变背景窗口中显示的字符
static void _ChangeMainText(char* pStr,int Delay)
{
	WM_MESSAGE Message;
	Message.MsgId=MSG_CHANGE_MAIN_TEXT;
	Message.Data.p=pStr;
	GUI_Delay(Delay);
	WM_SendMessage(WM_HBKWIN,&Message);
	WM_InvalidateWindow(WM_HBKWIN);
	GUI_Delay(Delay/3);
}

//子窗口回调函数
static void _cbChild(WM_MESSAGE *pMsg)
{
	WM_HWIN hWin=(FRAMEWIN_Handle)(pMsg->hWin);
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(GUI_WHITE);
			GUI_SetColor(GUI_BLACK);
			GUI_SetFont(&GUI_FontComic24B_ASCII);
			GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
			GUI_Clear();
			GUI_DispStringHCenterAt("Client window",WM_GetWindowSizeX(hWin)/2,WM_GetWindowSizeY(hWin)/2);
			break;
		default:
			WM_DefaultProc(pMsg);
	}
}

//FrameWin控件回调函数
static void _cbFrame(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_NOTIFY_PARENT:
			if(pMsg->Data.v==WM_NOTIFICATION_RELEASED)
			{
				int Id=WM_GetId(pMsg->hWinSrc);
				if(Id==GUI_ID_CLOSE)
				{
					if(_LockClose) return;
					_hFrame = 0;
				}
			}
			break;	
	}
	if(_pcbOldFrame) (*_pcbOldFrame)(pMsg);
}

//背景窗口WM_HBKWIN回调函数
static void _cbBkWindow(WM_MESSAGE* pMsg)
{
	switch(pMsg->MsgId)
	{
		case MSG_CHANGE_MAIN_TEXT:
			strcpy(_acMainText,pMsg->Data.p);
			WM_InvalidateWindow(pMsg->hWin);
			break;
		case WM_PAINT:
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			GUI_SetColor(GUI_WHITE);
			GUI_SetFont(&GUI_Font24_ASCII);
			GUI_DispStringHCenterAt("WIDGET_FrameWin - Sample",160,5);
			GUI_SetFont(&GUI_Font8x16);
			GUI_DispStringHCenterAt(_acMainText,160,40);
			GUI_SetFont(&GUI_Font6x8);
			GUI_DispStringHCenterAt("The function FRAMEWIN_Create creates both the\n"
                              "frame window and the client window.", 160, 190);
			break;
		default:
			WM_DefaultProc(pMsg);
	}
}

static void _DemoFramewin(void)
{
	int i;
	char acInfoText[]="-- sec to play with window";
	WM_HWIN hChild;
	//为背景设置回调函数
	WM_SetCallback(WM_HBKWIN,_cbBkWindow);
	//创建一个框架窗口
	_ChangeMainText("FRAMEWIN_Create",SPEED);
	_hFrame = FRAMEWIN_Create("Frame window",0,WM_CF_SHOW,30,60,260,125);
	FRAMEWIN_SetTitleHeight(_hFrame,20);
	FRAMEWIN_SetFont(_hFrame,&GUI_Font16B_ASCII);
	//为框架窗口设置回调函数
	_pcbOldFrame=WM_SetCallback(_hFrame,_cbFrame);
	//获取子窗口的句柄
	hChild=WM_GetClientWindow(_hFrame);
	//设置子窗口的回调函数
	WM_SetCallback(hChild,_cbChild);
	//设置框架窗口为可移动的
	FRAMEWIN_SetMoveable(_hFrame,1);
	//给框架窗口创建几个按钮
	FRAMEWIN_AddCloseButton(_hFrame,FRAMEWIN_BUTTON_RIGHT,0);
	FRAMEWIN_AddMaxButton(_hFrame,FRAMEWIN_BUTTON_RIGHT,2);
	FRAMEWIN_AddMinButton(_hFrame,FRAMEWIN_BUTTON_RIGHT,2);
	
	//修改框架窗口的属性
	_ChangeMainText("FRAMEWIN_SetActive",SPEED);
	FRAMEWIN_SetActive(_hFrame,1);
	_ChangeMainText("FRAMEWIN_SetFont",SPEED);
	FRAMEWIN_SetFont(_hFrame,&GUI_Font24_ASCII);
	FRAMEWIN_SetTitleHeight(_hFrame,25);
	_ChangeMainText("FRAMEWIN_SetTextColor",SPEED);
	FRAMEWIN_SetTextColor(_hFrame,GUI_YELLOW);
	_ChangeMainText("FRAMEWIN_SetTextAlign",SPEED);
	FRAMEWIN_SetTextAlign(_hFrame,GUI_TA_HCENTER);
	_ChangeMainText("FRAMEWIN_Minimize",SPEED);
	FRAMEWIN_Minimize(_hFrame);
	_ChangeMainText("FRAMEWIN_Maxmize",SPEED);
	FRAMEWIN_Maximize(_hFrame);
	_ChangeMainText("FRAMEWIN_Restore",SPEED);
	FRAMEWIN_Restore(_hFrame);
	_ChangeMainText("FRAMEWIN_SetTitleVis",SPEED);
	for(i=0;i<5;i++)
	{
		FRAMEWIN_SetTitleVis(_hFrame,0);
		GUI_Delay(200);
		FRAMEWIN_SetTitleVis(_hFrame,1);
		GUI_Delay(200);
	}
	
	_LockClose=0;
	for(i=250;(i>0)&& _hFrame;i--)
	{
		acInfoText[0]='0'+((i+9)/100);
		acInfoText[1]='0'+(((i+9)/10)%10);
		_ChangeMainText(acInfoText,0);
		GUI_Delay(100);
	}
	if(_hFrame)
	{
		_ChangeMainText("FRAMEWIN_Delete",SPEED);
		FRAMEWIN_Delete(_hFrame);
	}else
	{
		_ChangeMainText("",50);
	}
}

void FrameWin_Demo(void)
{
	WM_EnableMemdev(WM_HBKWIN);
	while(1)
	{
		_DemoFramewin();
	}
}


void STemWin_FrameWin_Test(void)
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
	FrameWin_Demo();
}













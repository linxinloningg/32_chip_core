#include "WM_cat_demo.h"
#include "GUI.h"
#include "WM.h"
#include "FRAMEWIN.h"
#include "BUTTON.h"


static WM_HWIN _hWin1;
static WM_HWIN _hWin2;
static WM_HWIN _hBut1;
static WM_HWIN _hBut2;
static int     _PaintCount1;
static int     _PaintCount2;

static GUI_COLOR _aColors[]={
GUI_BLUE, GUI_YELLOW, GUI_RED, GUI_LIGHTCYAN, GUI_DARKRED, GUI_DARKRED
};


//背景窗口回调函数
static void _cbBkWin(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			GUI_SetColor(0x0060FF);
			GUI_DispStringAt("PaintCount (Early):",0,0);
			GUI_DispDecAt(_PaintCount1,120,0,5);
			GUI_SetColor(0x00FFC0);
			GUI_DispStringAt("PaintCount (Late):",0,12);
			GUI_DispDecAt(_PaintCount2,120,12,5);
			break;
		case WM_NOTIFY_PARENT:
			if(pMsg->Data.v == WM_NOTIFICATION_RELEASED)  //按钮被释放时
			{
				if(pMsg->hWinSrc == _hBut1)	//button1被释放
				{
					WM_InvalidateWindow(_hWin1); 	//窗口1失效
					WM_InvalidateWindow(_hWin2);	//窗口2失效		
				}else if(pMsg->hWinSrc == _hBut2)	//button2被释放
				{
					_PaintCount1 = 0;
					_PaintCount2 = 0;
					WM_InvalidateWindow(pMsg->hWin); //主窗口失效
				}
			}
			break;
			default:
				WM_DefaultProc(pMsg);	
	}
}


//顶层窗口回调函数
static void _cbTop(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(GUI_MAGENTA);
			GUI_Clear();
			break;
		default:
			WM_DefaultProc(pMsg);
	}
}

//框架窗口1回调函数
static void _cbFrameWin1(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(_aColors[_PaintCount1 % 6]); //设置背景颜色
			GUI_Clear();
			GUI_SetColor(0x0060FF);
			GUI_FillCircle(25,25,15);
			GUI_SetColor(GUI_BLACK);
			GUI_DrawCircle(25,25,15);
			_PaintCount1++;
			WM_InvalidateWindow(WM_HBKWIN); //背景窗口失效
			break;
		default:
			WM_DefaultProc(pMsg);
	}
}

//框架窗口2回调函数
static void _cbFrameWin2(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(_aColors[_PaintCount2 % 6]);
			GUI_Clear();
			GUI_SetColor(0x00FFC0);
			GUI_FillCircle(25,25,15);
			GUI_SetColor(GUI_BLACK);
			GUI_DrawCircle(25,25,15);
			_PaintCount2++;
			WM_InvalidateWindow(WM_HBKWIN);
			break;
		default:
			WM_DefaultProc(pMsg);
	}		
}



void STemWin_LateClipping_Test(void)
{
	WM_HWIN hWin0;
	WM_HWIN	hWin1;
	WM_HWIN hWin2;
	WM_HWIN	hFrame1;
	WM_HWIN hFrame2;
	WM_HWIN	hClient1;
	WM_HWIN hClient2;
	
	WM_SetCallback(WM_HBKWIN,_cbBkWin);
	hFrame1 = FRAMEWIN_CreateEx(10,30,140,140,0,WM_CF_SHOW,FRAMEWIN_CF_MOVEABLE,0,"Early Clipping",_cbFrameWin1); //创建框架窗口1,可移动
	hFrame2 = FRAMEWIN_CreateEx(170,30,140,140,0,WM_CF_SHOW,FRAMEWIN_CF_MOVEABLE,0,"Late Clipping",_cbFrameWin2); //创建框架窗口2,可移动
	hClient1= WM_GetClientWindow(hFrame1); 	//返回框架1的客户端窗口句柄
	hClient2= WM_GetClientWindow(hFrame2);	//返回框架2的客户端窗口句柄
	_hWin1 = WM_CreateWindowAsChild(0,0,WM_GetWindowSizeX(hClient1),WM_GetWindowSizeY(hClient1),hClient1,WM_CF_SHOW,_cbFrameWin1,0);
	_hWin2 = WM_CreateWindowAsChild(0,0,WM_GetWindowSizeX(hClient2),WM_GetWindowSizeY(hClient2),hClient2,WM_CF_SHOW|WM_CF_LATE_CLIP,_cbFrameWin2,0);
	_hBut1 = BUTTON_CreateEx(10,210,140,20,0,WM_CF_SHOW,0,1);	//创建按钮1
	_hBut2 = BUTTON_CreateEx(170,210,140,20,0,WM_CF_SHOW,0,2);	//创建按钮2
	hWin0 = FRAMEWIN_CreateEx(60,80,40,40,	0, 	WM_CF_SHOW|WM_CF_STAYONTOP,FRAMEWIN_CF_MOVEABLE,0,"Top 0",_cbTop);
	hWin1 = FRAMEWIN_CreateEx(220,80,40,40,	0,	WM_CF_SHOW|WM_CF_STAYONTOP,FRAMEWIN_CF_MOVEABLE,0,"Top 1",_cbTop);
	hWin2 = FRAMEWIN_CreateEx(140,170,40,40,0, 	WM_CF_SHOW|WM_CF_STAYONTOP,FRAMEWIN_CF_MOVEABLE,0,"Top 2",_cbTop);
	
	FRAMEWIN_SetResizeable(hWin0,1);  	//窗口设置为可缩放
	FRAMEWIN_SetResizeable(hWin1,1);	//窗口设置为可缩放
	FRAMEWIN_SetResizeable(hWin2,1);    //窗口设置为可缩放
	
	BUTTON_SetText(_hBut1,"Invalidate");	//设置按钮的名字
	BUTTON_SetText(_hBut2,"Reset counts");	
	while(1)
	{
		GUI_Delay(50);
	}
}

void STemWIN_WM_Cat_Test(void)   
{
	STemWin_LateClipping_Test();
	while(1)
	{
		GUI_Delay(10);
	}
}


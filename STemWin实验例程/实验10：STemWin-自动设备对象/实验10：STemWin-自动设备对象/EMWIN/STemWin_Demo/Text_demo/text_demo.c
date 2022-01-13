#include "text_demo.h"
#include "GUI.h"


void STemWIN_Text_Test(void)   //STEMWIN文本显示测试程序
{
	int i;
	char text[]="Hello!";
	GUI_RECT Rect={0,200,30,240};  //定义矩形显示区域
	//定义换行模式
	GUI_WRAPMODE aWm[]={GUI_WRAPMODE_NONE,  //不换行
						GUI_WRAPMODE_CHAR,  //以字符换行
						GUI_WRAPMODE_WORD};  //以字换行
	

	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font24_ASCII);
	GUI_SetColor(GUI_YELLOW);
	GUI_DispString("Hello World!\n");

	GUI_SetFont(&GUI_Font8x16);
	GUI_SetPenSize(10);
	GUI_SetColor(GUI_RED);
	GUI_DrawLine(100,50,300,130);
	GUI_DrawLine(100,130,300,50);

	GUI_SetBkColor(GUI_BLACK);
	GUI_SetColor(GUI_WHITE);
	GUI_SetTextMode(GUI_TM_NORMAL);  //正常模式
	GUI_DispStringHCenterAt("GUI_TM_NORMAL",200,50);
	
	GUI_SetTextMode(GUI_TM_REV);  //反转文本
	GUI_DispStringHCenterAt("GUI_TM_REV",200,66);
	
	GUI_SetTextMode(GUI_TM_TRANS);//透明文本
	GUI_DispStringHCenterAt("GUI_TM_TRANS",200,82);

	GUI_SetTextMode(GUI_TM_XOR);  //异或文本
	GUI_DispStringHCenterAt("GUI_TM_XOR",200,98);
	
	GUI_SetTextMode(GUI_TM_REV|GUI_TM_TRANS);  //反转+透明
	GUI_DispStringHCenterAt("GUI_TM_REV|GUI_TM_TRANS",200,114);

	GUI_SetTextMode(GUI_TM_TRANS);  //透明
	for(i=0;i<3;i++)
	{
		GUI_SetColor(GUI_BLACK);	
		GUI_FillRectEx(&Rect);
		GUI_SetColor(GUI_WHITE);
		//在指定的区域内显示字符串，并进行换行模式的切换
		GUI_DispStringInRectWrap(text,&Rect,GUI_TA_LEFT,aWm[i]);
		Rect.x0+=40;
		Rect.x1+=40;
	}

}


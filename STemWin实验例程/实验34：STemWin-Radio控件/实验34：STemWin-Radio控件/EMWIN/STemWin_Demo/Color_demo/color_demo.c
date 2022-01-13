#include "color_demo.h"
#include "GUI.h"
#include "math.h"
#include "stdlib.h"

#define X_START 60
#define Y_START 40   //颜色条开始坐标

typedef struct
{
	int numbar;  //颜色条数量
	GUI_COLOR color;  //颜色
	const char *s;   //颜色字符
}BAR_DATA;

static const BAR_DATA abardata[]=
{
	{2,GUI_RED,"RED"},
	{2,GUI_GREEN,"GREEN"},
	{2,GUI_BLUE,"BLUE"},
	{1,GUI_WHITE,"WHITE"},
	{2,GUI_YELLOW,"YELLOW"},
	{2,GUI_CYAN,"CYAN"},
	{2,GUI_MAGENTA,"MAGENTA"}
}; 
static const int acolorstart[]={GUI_BLACK,GUI_WHITE};



void STemWin_ColorBar_Test(void)   //颜色条显示测试
{
	GUI_RECT Rect;
	int ystep;
	int i,j;
	int xsize,ysize;
	int numbar;    //总共的颜色条数  2+2+2+1+2+2+2=13
	int numcolor;  //总共的颜色数  7
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetColor(GUI_YELLOW);
	GUI_SetFont(&GUI_Font24_ASCII);
	GUI_SetTextMode(GUI_TM_TRANS);  //设置透明
	GUI_DispStringHCenterAt("PRECHIN ColorBar Test",200,0);

	//获取显示尺寸
	xsize=LCD_GetXSize();
	ysize=LCD_GetYSize();

	//获取颜色的数量
	numcolor=GUI_COUNTOF(abardata);
	for(i=0,numbar=0;i<numcolor;i++)
	{
		numbar+=abardata[i].numbar;
	}
	ystep=(ysize-Y_START)/numbar;  //每一条颜色的Y值

	//显示文本
	Rect.x0=0;
	Rect.x1=X_START-1;
	Rect.y0=Y_START;
	GUI_SetFont(&GUI_Font8x16);
	for(i=0;i<numcolor;i++)
	{
		Rect.y1=Rect.y0+ystep*abardata[i].numbar-1;
		GUI_DispStringInRect(abardata[i].s,&Rect,GUI_TA_LEFT|GUI_TA_VCENTER); //显示文本
		Rect.y0=Rect.y1+1;
	}
	
	//显示颜色条
	Rect.x0=X_START;
	Rect.x1=xsize-1;
	Rect.y0=Y_START;
	for(i=0;i<numcolor;i++)
	{
		for(j=0;j<abardata[i].numbar;j++)
		{
			Rect.y1=Rect.y0+ystep-1;
			//显示颜色 水平梯度填充的颜色
			GUI_DrawGradientH(Rect.x0,Rect.y0,Rect.x1,Rect.y1,acolorstart[j],abardata[i].color);
			Rect.y0=Rect.y1+1;
		}
	}
}

void STemWIN_Color_Test(void)   
{
	STemWin_ColorBar_Test();
	while(1)
	{
		GUI_Delay(10);
	}
}


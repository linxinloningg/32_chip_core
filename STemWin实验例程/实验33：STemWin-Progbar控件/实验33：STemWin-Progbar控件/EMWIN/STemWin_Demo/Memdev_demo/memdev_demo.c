#include "memdev_demo.h"
#include "GUI.h"
#include "led.h"


static GUI_RECT Rect = {0, 150, 400, 210}; 
static GUI_RECT Rect1 = {40, 150, 120, 210};

//未使用存储设备绘制
static void _Draw(int Delay) 
{
	GUI_SetPenSize(5);
	GUI_SetColor(GUI_RED);
	GUI_DrawLine(Rect1.x0 + 3, Rect1.y0 + 3, Rect1.x1 - 3, Rect1.y1 - 3);
	GUI_Delay(Delay);
	GUI_SetColor(GUI_GREEN);
	GUI_DrawLine(Rect1.x0 + 3, Rect1.y1 - 3, Rect1.x1 - 3, Rect1.y0 + 3);
	GUI_Delay(Delay);
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_FontComic24B_ASCII);
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_DispStringInRect("Closed", &Rect1, GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_Delay(Delay);
}


//使用存储设备绘制
void STemWin_MemDev_Test(void) 
{
	GUI_MEMDEV_Handle hMem;
	int i;

	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetColor(GUI_YELLOW);
	GUI_SetFont(&GUI_Font24_ASCII);
	GUI_DispStringHCenterAt("MEMDEV_MemDev - Sample", 200, 0);
	GUI_SetFont(&GUI_Font20_ASCII);
	GUI_DispStringHCenterAt("Shows the advantage of using a\nmemorydevice", 200,50);
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_DispStringHCenterAt("Draws the picture\nwithout a\nmemory device", 100, 100);
	GUI_DispStringHCenterAt("Draws the picture\nusing a\nmemory device", 300, 100);
  

	
	hMem = GUI_MEMDEV_Create(Rect.x0, Rect.y0, Rect.x1 - Rect.x0, Rect.y1 - Rect.y0);	//创建存储设备
	GUI_MEMDEV_Select(hMem);	//选择hMem作为绘制操作目标的存储设备
	_Draw(0);  					//执行要显示的内容复制到存储设备中
	GUI_MEMDEV_Select(0);  		//选中LCD
	while (1) 
	{
		for (i = 0; i < 3; i++) 
		{
			GUI_Delay(250);
			led1=!led1;
			GUI_ClearRect(LCD_GetXSize() - Rect.x1, Rect.y0, LCD_GetXSize(), Rect.y1);  //清除选中的屏幕
			GUI_Delay(250);
			GUI_MEMDEV_CopyToLCDAt(hMem, LCD_GetXSize() - Rect.x1+120, Rect.y0); //将存储设备的内容从内存复制到LCD的指定位置
		}
		GUI_Delay(500);  
	
		//未使用存储设备
		_Draw(400);
		GUI_Delay(400);
		GUI_ClearRect(0, 150, 479, 210);
	}
}

void STemWIN_Memdev_Test(void)   
{
	STemWin_MemDev_Test();
	while(1)
	{
		GUI_Delay(10);
	}

}


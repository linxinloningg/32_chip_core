#include "snake.h"
#include "gui.h"

u8 key;
//游戏界面
//一共12个汉字，成绩，关卡，上，下，左，右，开始，暂停
void show(void)
{
	//画围墙 厚度为5
	//构建一个((150/5)*(230/5))=(30*46)的游戏区域 墙壁厚为5 蛇体为5
  	
	GUI_Line(0, 161, 240, 161,BLACK);
		//画按键 长64 高32 

	GUI_Box(88,192,152,224,GREEN);
	Test_Show_CH_Font16(112,200,4,RED);//上
	GUI_Box(88,266,152,298,GREEN);
	Test_Show_CH_Font16(112,274,5,RED);//下
	GUI_Box(19,229,83,261,GREEN);
	Test_Show_CH_Font16(43,237,6,RED);//左
	GUI_Box(157,229,221,261,GREEN);
	Test_Show_CH_Font16(181,237,7,RED);//右
	//成绩
	Test_Show_CH_Font16(0,165,0,RED);
	Test_Show_CH_Font16(16,165,1,RED);
	GUI_Show12ASCII(32,165,":",RED,GRAY);
	GUI_Show12ASCII(40,165,"  0",RED,GRAY);	
	//生命
	Test_Show_CH_Font16(184,165,2,RED);
	Test_Show_CH_Font16(200,165,3,RED);
	GUI_Show12ASCII(216,165,":",RED,GRAY);
	GUI_Show12ASCII(224,165," ",RED,GRAY);	
	//开始
	GUI_Box(1,286,65,318,GREEN);
	Test_Show_CH_Font16(17,294,8,RED);
	Test_Show_CH_Font16(33,294,9,RED);
	//暂停
	GUI_Box(174,286,238,318,GREEN);
	Test_Show_CH_Font16(190,294,10,RED);
	Test_Show_CH_Font16(204,294,11,RED);
	//游戏名字
	GUI_Show12ASCII(98,237,"Snaker",RED,GRAY);

	GUI_Show12ASCII(10,320,"K_UP:    UP",RED,GRAY);
	GUI_Show12ASCII(10,340,"K_DOWN:  DOWN",RED,GRAY);
	GUI_Show12ASCII(10,360,"K_LEFT:  LEFT",RED,GRAY);
	GUI_Show12ASCII(10,380,"K_RIGHT: RIGHT",RED,GRAY);
}

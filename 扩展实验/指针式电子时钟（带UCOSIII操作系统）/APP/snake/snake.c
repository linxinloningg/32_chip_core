#include "snake.h"
#include "tftlcd.h"
#include "picture.h"
#include "ascii.h"

u8 key;


void LCD_ShowPictureEx(u16 x, u16 y, u16 wide, u16 high)
{
	u16 temp = 0;
	long tmp=0,num=0;
	LCD_Set_Window(x, y, x+wide-1, y+high-1);
	num = wide * high*2 ;
	do
	{  
		temp = pic[tmp + 1];
		temp = temp << 8;
		temp = temp | pic[tmp];
		LCD_WriteData_Color(temp);//逐点显示
		tmp += 2;
	}
	while(tmp < num);	
}

//在指定位置 显示1个16*16的汉字
//(x,y):汉字显示的位置
//index:tfont数组里面的第几个汉字
//color:这个汉字的颜色
void Test_Show_CH_Font16(u16 x,u16 y,u8 index,u16 color)
{   			    
	u8 temp,t,t1;
	u16 y0=y;				   
    for(t=0;t<32;t++)//每个16*16的汉字点阵 有32个字节
    {   
		if(t<16)temp=tfont16[index*2][t];      //前16个字节
		else temp=tfont16[index*2+1][t-16];    //后16个字节	                          
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)LCD_DrawFRONT_COLOR(x,y,color);//画实心点
			else LCD_DrawFRONT_COLOR(x,y,GBLUE);   //画空白点（使用背景色）
			temp<<=1;
			y++;
			if((y-y0)==16)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}

//在指定位置 显示1个24*24的汉字
//(x,y):汉字显示的位置
//index:tfont数组里面的第几个汉字
//color:这个汉字的颜色
void Test_Show_CH_Font24(u16 x,u16 y,u8 index,u16 color)
{   			    
	u8 temp,t,t1;
	u16 y0=y;				   
    for(t=0;t<72;t++)//每个24*24的汉字点阵 有72个字节
    {   
		if(t<24)temp=tfont24[index*3][t];           //前24个字节
		else if(t<48)temp=tfont24[index*3+1][t-24]; //中24个字节	                          
        else temp=tfont24[index*3+2][t-48];         //后24个字节
	    for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)LCD_DrawFRONT_COLOR(x,y,color);//画实心点
			else LCD_DrawFRONT_COLOR(x,y,GRAY);   //画空白点（使用背景色）
			temp<<=1;
			y++;
			if((y-y0)==24)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}


//游戏界面
//一共12个汉字，成绩，关卡，上，下，左，右，开始，暂停
void show(void)
{
	//画围墙 厚度为5
	//构建一个((150/5)*(230/5))=(30*46)的游戏区域 墙壁厚为5 蛇体为5
  	
	LCD_DrawLine_Color(0, 161, 240, 161,BLACK);
	LCD_DrawLine_Color(240, 0, 240, 161,BLACK);

	
	//成绩
	Test_Show_CH_Font16(0,165,0,RED);
	Test_Show_CH_Font16(16,165,1,RED);
	FRONT_COLOR=RED;
	BACK_COLOR=GRAY;
	LCD_ShowString(32,165,tftlcd_data.width,tftlcd_data.height,16,":");
	LCD_ShowString(40,165,tftlcd_data.width,tftlcd_data.height,16,"  0");	
	//生命
	Test_Show_CH_Font16(184,165,2,RED);
	Test_Show_CH_Font16(200,165,3,RED);
	LCD_ShowString(216,165,tftlcd_data.width,tftlcd_data.height,16,":");
	LCD_ShowString(224,165,tftlcd_data.width,tftlcd_data.height,16," ");	
	
	//游戏名字
	LCD_ShowString(98,237,tftlcd_data.width,tftlcd_data.height,16,"Snaker");

	LCD_ShowString(10,320,tftlcd_data.width,tftlcd_data.height,16,"K_UP:    UP");
	LCD_ShowString(10,340,tftlcd_data.width,tftlcd_data.height,16,"K_DOWN:  DOWN");
	LCD_ShowString(10,360,tftlcd_data.width,tftlcd_data.height,16,"K_LEFT:  LEFT");
	LCD_ShowString(10,380,tftlcd_data.width,tftlcd_data.height,16,"K_RIGHT: RIGHT");
}

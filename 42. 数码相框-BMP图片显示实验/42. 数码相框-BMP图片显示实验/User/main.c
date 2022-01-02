

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 
#include "sd.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "key.h"
#include "font_show.h"
#include "bmp.h"


FileNameTypeDef filename[30];

int main()
{
	u8 i=0,t;
	u8 key;
    u8 j, k;
    u8 dat[7] = {"0:/图片"};  //要显示的图片的文件地址
    u8 picFile[30];
	u8 num[6];
    u32 sdCapacity, free;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();				//初始化EN25QXX
	
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	FRONT_COLOR=RED;//设置字体为红色 
	LCD_ShowFont16Char(10,10,"图片显示实验");
	
	while(SD_Init()!=0)
	{	
		LCD_ShowFont12Char(10,30,"SD Card Error!");
		
	}
	LCD_ShowFont12Char(10,30,"SD Card OK!    ");
	
	FATFS_Init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	f_mount(fs[1],"1:",1); 				//挂载FLASH.
	
	while(FATFS_GetFree("0", &sdCapacity, &free) != 0)
	{
		LCD_ShowFont12Char(10, 50, "SD Card FAT ERROR!");
	}
	
	sdCapacity >>= 10;                         //将KB转换为MB
	num[0] = (sdCapacity /10000 % 10) + '0';
	num[1] = (sdCapacity /1000 % 10) + '0';
	num[2] = (sdCapacity /100 % 10) + '0';
	num[3] = (sdCapacity /10 % 10) + '0';
	num[4] = (sdCapacity % 10) + '0';
	
	LCD_ShowFont12Char(10, 50, "SD Card FAT OK!  ");
    LCD_ShowFont12Char(10, 70, "SD card total memory:     MB");
	LCD_ShowFont12Char(178, 70, num);
	
	/* 显示SD卡空余空间 */
	free >>= 10;         //将单位转换为MB
	num[0] = (free /10000 % 10) + '0';
	num[1] = (free /1000 % 10) + '0';
	num[2] = (free /100 % 10) + '0';
	num[3] = (free /10 % 10) + '0';
	num[4] = (free % 10) + '0';

    LCD_ShowFont12Char(10, 100, "SD card free memory:     MB");
	LCD_ShowFont12Char(170, 100, num);
    LCD_ShowFont12Char(10, 120, "KEY_DOWN：下一张图片！");
	
	/*  扫描文件地址里面所有的文件 */
    FATFS_ScanFiles(dat, filename);

	while(1)
	{
		while(1)
		{
			key=KEY_Scan(0);
			if(key==KEY_DOWN)
			{
				break;
			}
			t++;
			if(t%20==0)
			{
				led1=!led1;
			}
			delay_ms(10);
		}
		while(1)
		{
			/* 设置刷屏方向，FAT32文件存放BMP图片是从最后开始存 */
		#ifdef TFTLCD_HX8357D
            LCD_WriteCmd(0x26); //01
        	LCD_WriteData(0x01);
            LCD_WriteCmd(0x36);
    		LCD_WriteData(0x8c);
		#endif
			
		#ifdef TFTLCD_HX8352C
            LCD_WriteCmd(0x36);   //设置彩屏显示方向的寄存器
			LCD_WriteData(0x06);
		#endif
			
		#ifdef TFTLCD_ILI9341
            LCD_WriteCmd(0x26); //01
        	LCD_WriteData(0x01);
            LCD_WriteCmd(0x36);
    		LCD_WriteData(0x8c);
		#endif
				
		#ifdef TFTLCD_R61509V
			LCD_WriteCmd(0x0400);
			LCD_WriteData(0xe200);
		#endif
		
		#ifdef TFTLCD_R61509V3
			LCD_WriteCmd(0x0400);
			LCD_WriteData(0xe200);
		#endif
		
		#ifdef TFTLCD_R61509VN
			LCD_WriteCmd(0x0400);
			LCD_WriteData(0xe200);
		#endif
		
		#ifdef TFTLCD_ILI9325
			LCD_WriteCmd(0x0001);   
			LCD_WriteData(0x0100);
			LCD_WriteCmd(0x0003);   //设置彩屏显示方向的寄存器
			LCD_WriteData(0x1000);
		#endif
		
		#ifdef TFTLCD_NT5510
            
		#endif
			
			/* 判断是否是BMP图片文件 */
			if((filename[i].type[1] == 'b') && (filename[i].type[2] == 'm') &&
               (filename[i].type[3] == 'p')) 
			{
				/* 处理文件路径,先添加文件路径 */
				k = 0;
				while(*(dat + k) != '\0')
				{
					*(picFile + k) = *(dat + k);
					k++;
				}

                /* 路径之后加上一斜杠 */
				*(picFile + k) = '/';
				k++;

                /* 添加文件名字名字 */
				j = 0;
				while(filename[i].name[j] != '\0')
				{
					*(picFile + k) = filename[i].name[j];
					k++;
					j++;	
				}

                /* 添加文件后缀 */
				j = 0;
				while(filename[i].type[j] != '\0')
				{
					*(picFile + k) = filename[i].type[j];
					k++;
					j++;	
				}

                /* 文件最后添加一个结束符号 */
                *(picFile + k) = '\0';

				/* 显示图片 */
			 	BMP_ShowPicture(picFile);
                i++;
				break;
			}
			i++;
            if(i > 30)
            {
                i = 0;
				
            }		
		}
		
	}
}

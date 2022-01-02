/* 本程序使用的是RFID-RC522射频模块设计的一个门禁系统，当感应卡放到射频模块区域内会感应到
	卡，如果卡序列号和程序设计一致就会认为是正确开锁，D2指示灯亮，LCD上显示开锁，5秒钟以后
	自动关锁，D2指示灯灭。当卡错误时候不会显示，D2也不会亮。卡的序列号是唯一的。
	
	管脚接线图：
	RST---PF4
	MISO---PF3
	MOSI---PF2
	SCK---PF1
	NSS(SDA)--PF0
	
	*/

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "RC522.h"
#include "time.h"


unsigned char data1[16] = {0x12,0x34,0x56,0x78,0xED,0xCB,0xA9,0x87,0x12,0x34,0x56,0x78,0x01,0xFE,0x01,0xFE};
//M1卡的某一块写为如下格式，则该块为钱包，可接收扣款和充值命令
//4字节金额（低字节在前）＋4字节金额取反＋4字节金额＋1字节块地址＋1字节块地址取反＋1字节块地址＋1字节块地址取反 
unsigned char data2[4]  = {0,0,0,0x01};
unsigned char DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 


unsigned char g_ucTempbuf[20];

int main()
{
	unsigned char status,i;
	unsigned int temp;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	
	
	FRONT_COLOR=GREEN;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"RFID-RC522 Test");
	FRONT_COLOR=RED;
	LCD_ShowString(10,110,tftlcd_data.width,tftlcd_data.height,16,"Close Door...");
	
	RC522_Init();
	PcdReset();
 	PcdAntennaOff(); 
	delay_ms(10);
 	PcdAntennaOn();
	delay_ms(10);
	
	TIM4_Init(1000,7199);
	printf("Start \r\n");
	
	while ( 1 )
     {   
		 status = PcdRequest(PICC_REQALL, g_ucTempbuf);//寻卡
         if (status != MI_OK)
         {    
		     PcdReset();
		     PcdAntennaOff(); 
		     PcdAntennaOn(); 
			 continue;
         }		     
		printf("卡的类型:");
	    for(i=0;i<2;i++)
		{
			temp=g_ucTempbuf[i];
			printf("%X",temp);
			
		}	
         status = PcdAnticoll(g_ucTempbuf);//防冲撞
         if(status != MI_OK)
         {    continue;    }
		   
		////////以下为超级终端打印出的内容////////////////////////
	
		printf("卡序列号：");	//超级终端显示,
		for(i=0;i<4;i++)
		{
			temp=g_ucTempbuf[i];
			printf("%X",temp);
			
		}

		if(g_ucTempbuf[0]==0xd4&&g_ucTempbuf[1]==0xd5&&g_ucTempbuf[2]==0x34&&g_ucTempbuf[3]==0x00)
		{
			led2=0;
			FRONT_COLOR=RED;
			LCD_ShowString(10,110,tftlcd_data.width,tftlcd_data.height,16,"Open Door...  ");//开门
		}
		else
		{	
			led2=1;	
			FRONT_COLOR=RED;
			LCD_ShowString(10,110,tftlcd_data.width,tftlcd_data.height,16,"               ");
		}

		///////////////////////////////////////////////////////////

	     status = PcdSelect(g_ucTempbuf);//选定卡片
	     if (status != MI_OK)
	     {    continue;    }
	     
	     status = PcdAuthState(PICC_AUTHENT1A, 1, DefaultKey, g_ucTempbuf);//验证卡片密码
	     if (status != MI_OK)
	     {    continue;    }
	     
	     status = PcdWrite(1, data1);//写块
         if (status != MI_OK)
         {    continue;    }

		while(1)
		{
         	status = PcdRequest(PICC_REQALL, g_ucTempbuf);//寻卡
         	if (status != MI_OK)
         	{   
 		 	
			     PcdReset();
			     PcdAntennaOff(); 
			     PcdAntennaOn(); 
			  	continue;
         	}
			 status = PcdAnticoll(g_ucTempbuf);//防冲撞
	         if (status != MI_OK)
	         {    continue;    }

			status = PcdSelect(g_ucTempbuf);//选定卡片
	         if (status != MI_OK)
	         {    continue;    }
	         
	         status = PcdAuthState(PICC_AUTHENT1A, 1, DefaultKey, g_ucTempbuf);//验证卡片密码
	         if (status != MI_OK)
	         {    continue;    }


	         status = PcdValue(PICC_DECREMENT,1,data2);//扣款
	         if (status != MI_OK)
	         {    continue;    }
			 
	         status = PcdBakValue(1, 2);//块备份
	         if (status != MI_OK)
	         {    continue;    }
	         
	         status = PcdRead(2, g_ucTempbuf);//读块
	         if (status != MI_OK)
	         {    continue;    }
        	printf("卡读块：");	//超级终端显示,
         	for(i=0;i<16;i++)
			{
				temp=g_ucTempbuf[i];
				printf("%X",temp);
				
			}

			printf("\n");				 		         
			PcdHalt();
		}	
    }
}

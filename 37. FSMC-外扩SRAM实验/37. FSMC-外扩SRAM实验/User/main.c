

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "sram.h" 


u8 text_buf[]="www.prechin.net";
#define TEXT_LEN sizeof(text_buf)
	

//外部内存测试(最大支持1M字节内存测试)	    
void ExSRAM_Cap_Test(u16 x,u16 y)
{
    u8 writeData = 0xf0, readData;
	u16 cap=0;
    u32 addr;
	
	addr = 1024; //从1KB位置开始算起
	
	LCD_ShowString(x,y,239,y+16,16,"ExSRAM Cap:   0KB"); 
	
	while(1)
	{
		FSMC_SRAM_WriteBuffer(&writeData, addr, 1);
		FSMC_SRAM_ReadBuffer(&readData,addr,1);
		
		/* 查看读取到的数据是否跟写入数据一样 */
        if(readData == writeData)
        {
            cap++;
            addr += 1024;
            readData = 0;
            if(addr > 1024 * 1024) //SRAM容量最大为1MB
            {
                break;
            }    
        }
        else
        {
            break;
        }     
	}
	LCD_ShowxNum(x+11*8,y,cap,4,16,0);//显示内存容量 
	printf("SRAM容量为：%dKB\r\n",cap);
}

int main()
{
	u8 i=0;
	u8 key;
	u8 read_buf[TEXT_LEN];
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	FSMC_SRAM_Init();
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"ExSRAM Test");
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_UP:Write   K_DOWN:Read");
	
	FRONT_COLOR=RED;
	ExSRAM_Cap_Test(10,110); 
	LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,16,"Write:");
	LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"Read :");
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			FSMC_SRAM_WriteBuffer(text_buf,0,TEXT_LEN);
			printf("写入的数据是：%s\r\n",text_buf);
			LCD_ShowString(10+6*8,130,tftlcd_data.width,tftlcd_data.height,16,(u8 *)text_buf);
		}
		if(key==KEY_DOWN)
		{
			FSMC_SRAM_ReadBuffer(read_buf,0,TEXT_LEN);
			printf("读取的数据是：%s\r\n",read_buf);
			LCD_ShowString(10+6*8,150,tftlcd_data.width,tftlcd_data.height,16,read_buf);
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		delay_ms(10);
			
	}
	
}

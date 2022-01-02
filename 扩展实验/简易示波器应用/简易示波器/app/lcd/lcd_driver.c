#include "lcd_driver.h"
#include "delay.h"

u16 tft_id;
u16 POINT_COLOR=0x0000;
/****************************************************************************
*函数名：TFT_GPIO_Config
*输  入：无
*输  出：无
*功  能：初始化TFT的IO口。
****************************************************************************/	  

void TFT_GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|RCC_APB2Periph_GPIOG,ENABLE);//使能PORTD,E,G时钟
	
 	//PORTD复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure); 
  	 
	//PORTE复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure);    	    	 											 

   	//	//PORTG12复用推挽输出 A10	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_12;	 //	//PORTG复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure);
}

/****************************************************************************
* Function Name  : TFT_FSMC_Config
* Description    : 初始化FSMC
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

void TFT_FSMC_Config(void)
{
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  	FSMC_NORSRAMTimingInitTypeDef  FSMC_ReadTimingInitStructure; 
	FSMC_NORSRAMTimingInitTypeDef  FSMC_WriteTimingInitStructure;
	
  	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);	//使能FSMC时钟

	FSMC_ReadTimingInitStructure.FSMC_AddressSetupTime = 0x01;	 //地址建立时间（ADDSET）为2个HCLK 1/36M=27ns
  	FSMC_ReadTimingInitStructure.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（ADDHLD）模式A未用到	
  	FSMC_ReadTimingInitStructure.FSMC_DataSetupTime = 0x0f;		 // 数据保存时间为16个HCLK,因为液晶驱动IC的读数据的时候，速度不能太快，尤其对1289这个IC。
  	FSMC_ReadTimingInitStructure.FSMC_BusTurnAroundDuration = 0x00;
  	FSMC_ReadTimingInitStructure.FSMC_CLKDivision = 0x00;
  	FSMC_ReadTimingInitStructure.FSMC_DataLatency = 0x00;
  	FSMC_ReadTimingInitStructure.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 


	FSMC_WriteTimingInitStructure.FSMC_AddressSetupTime = 0x15;	 //地址建立时间（ADDSET）为16个HCLK  
  	FSMC_WriteTimingInitStructure.FSMC_AddressHoldTime = 0x15;	 //地址保持时间		
  	FSMC_WriteTimingInitStructure.FSMC_DataSetupTime = 0x05;		 //数据保存时间为6个HCLK	
  	FSMC_WriteTimingInitStructure.FSMC_BusTurnAroundDuration = 0x00;
  	FSMC_WriteTimingInitStructure.FSMC_CLKDivision = 0x00;
  	FSMC_WriteTimingInitStructure.FSMC_DataLatency = 0x00;
  	FSMC_WriteTimingInitStructure.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A  


  	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//  这里我们使用NE4 ，也就对应BTCR[6],[7]。
  	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; // 不复用数据地址
  	FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;// FSMC_MemoryType_SRAM;  //SRAM   
  	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;//存储器数据宽度为16bit   
  	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
  	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable; 
  	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;   
  	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;  
  	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;	//  存储器写使能
  	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;   
  	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable; // 读写使用不同的时序
  	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable; 
  	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &FSMC_ReadTimingInitStructure; //读写时序
  	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &FSMC_WriteTimingInitStructure;  //写时序

  	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //初始化FSMC配置

 	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  // 使能BANK1	
}

/****************************************************************************
* Function Name  : TFT_WriteCmd
* Description    : LCD写入命令
* Input          : cmd：写入的16位命令
* Output         : None
* Return         : None
****************************************************************************/

void TFT_WriteCmd(uint16_t cmd)
{
	TFT->TFT_CMD=(cmd>>8)<<1;
	TFT->TFT_CMD=(cmd&0xff)<<1;
}

/****************************************************************************
* Function Name  : TFT_WriteData
* Description    : LCD写入数据
* Input          : dat：写入的16位数据
* Output         : None
* Return         : None
****************************************************************************/

void TFT_WriteData(u16 dat)
{
	TFT->TFT_DATA=(dat>>8)<<1;
	TFT->TFT_DATA=(dat&0xff)<<1;
}

u32 LCD_RGBColor_Change(u16 color)
{
	u8 r,g,b=0;
	
	r=(color>>11)&0x1f;
	g=(color>>5)&0x3f;
	b=color&0x1f;
	
	return ((r<<13)|(g<<6)|(b<<1));
}

void TFT_WriteData_Color(u16 color)
{
	u32 recolor=0;
	recolor=LCD_RGBColor_Change(color);
	TFT->TFT_DATA=(recolor>>9);
	TFT->TFT_DATA=recolor;
}

/****************************************************************************
* Function Name  : TFT_Init
* Description    : 初始化LCD屏
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

void TFT_Init(void)
{
	uint16_t i;

	TFT_GPIO_Config();
	TFT_FSMC_Config();
	delay_ms(50); 



	TFT_WriteCmd(0x0000);
	TFT_WriteCmd(0x0000);
	delay_ms(10);
	TFT_WriteCmd(0x0000);
	TFT_WriteCmd(0x0000);
	TFT_WriteCmd(0x0000);
	TFT_WriteCmd(0x0000);
	TFT_WriteCmd(0x00A4);TFT_WriteData(0x0001);
	delay_ms(10);

	TFT_WriteCmd(0x0060);TFT_WriteData(0x2700);
	TFT_WriteCmd(0x0008);TFT_WriteData(0x0808);

	//----------- Adjust the Gamma Curve ----------/
	TFT_WriteCmd(0x0300);TFT_WriteData(0x0109);  // Gamma Control
	TFT_WriteCmd(0x0301);TFT_WriteData(0x7E0A); 
	TFT_WriteCmd(0x0302);TFT_WriteData(0x0704); 
	TFT_WriteCmd(0x0303);TFT_WriteData(0x0911); 
	TFT_WriteCmd(0x0304);TFT_WriteData(0x2100); 
	TFT_WriteCmd(0x0305);TFT_WriteData(0x1109); 
	TFT_WriteCmd(0x0306);TFT_WriteData(0x7407); 
	TFT_WriteCmd(0x0307);TFT_WriteData(0x0A0E); 
	TFT_WriteCmd(0x0308);TFT_WriteData(0x0901); 
	TFT_WriteCmd(0x0309);TFT_WriteData(0x0021);

	TFT_WriteCmd(0x0010);TFT_WriteData(0x0010);  // Frame frequence
	TFT_WriteCmd(0x0011);TFT_WriteData(0x0202);  // 
	TFT_WriteCmd(0x0012);TFT_WriteData(0x0300); 
	TFT_WriteCmd(0x0013);TFT_WriteData(0x0007);
	delay_ms(10);
	// -------------- Power On sequence -------------//
	TFT_WriteCmd(0x0100);TFT_WriteData(0x0000);  // VGH/VGL 6/-3
	TFT_WriteCmd(0x0101);TFT_WriteData(0x0007);  // VCI1
	TFT_WriteCmd(0x0102);TFT_WriteData(0x0000);  // VREG1
	TFT_WriteCmd(0x0103);TFT_WriteData(0x0000);  // VDV 
	TFT_WriteCmd(0x0280);TFT_WriteData(0x0000);  // VCM
	delay_ms(200);//delay_ms 200ms
	TFT_WriteCmd(0x0100);TFT_WriteData(0x0330);  // VGH/VGL 6/-3
	TFT_WriteCmd(0x0101);TFT_WriteData(0x0247);  // VCI1
	delay_ms(50);//delay_ms 50ms
	TFT_WriteCmd(0x0102);TFT_WriteData(0xD1B0);  // VREG1
	delay_ms(50);//delay_ms 50ms
	TFT_WriteCmd(0x0103);TFT_WriteData(0x1000);  // VDV          //0900
	TFT_WriteCmd(0x0280);TFT_WriteData(0xC600);  // VCM          //chenyu 0xc600

	delay_ms(200);

	TFT_WriteCmd(0x0001);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0002);TFT_WriteData(0x0200);
	TFT_WriteCmd(0x0003);TFT_WriteData(0x1038);
	TFT_WriteCmd(0x0009);TFT_WriteData(0x0001);
	TFT_WriteCmd(0x000A);TFT_WriteData(0x0008);
	TFT_WriteCmd(0x000C);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x000D);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x000E);TFT_WriteData(0x0030);
	TFT_WriteCmd(0x0020);TFT_WriteData(0x0000);//H Start
	TFT_WriteCmd(0x0021);TFT_WriteData(0x0000);//V Start
	TFT_WriteCmd(0x0029);TFT_WriteData(0x0052);
	TFT_WriteCmd(0x0050);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0051);TFT_WriteData(0x00EF);
	TFT_WriteCmd(0x0052);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0053);TFT_WriteData(0x013F);
	TFT_WriteCmd(0x0061);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x006A);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0080);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0081);TFT_WriteData(0x0000);
	TFT_WriteCmd(0x0082);TFT_WriteData(0x005F);
	TFT_WriteCmd(0x0093);TFT_WriteData(0x0507);

	TFT_WriteCmd(0x0007);TFT_WriteData(0x0100);//DISPLAY ON
	delay_ms(50);
	TFT_WriteCmd(0x0022);//GRAM Data Write
}

/****************************************************************************
* Function Name  : TFT_SetWindow
* Description    : 设置要操作的窗口范围
* Input          : xStart：窗口起始X坐标
*                * yStart：窗口起始Y坐标
*                * xEnd：窗口结束X坐标
*                * yEnd：窗口结束Y坐标
* Output         : None
* Return         : None
****************************************************************************/

void TFT_SetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{

	TFT_WriteCmd(0x0212);   
	TFT_WriteData(xStart);
	TFT_WriteCmd(0x0213);  
	TFT_WriteData(xEnd);
	TFT_WriteCmd(0x0210);   
	TFT_WriteData(yStart);
	TFT_WriteCmd(0x0211);   
	TFT_WriteData(yEnd);

	TFT_WriteCmd(0x0201);   
	TFT_WriteData(xStart);
	TFT_WriteCmd(0x0200);   
	TFT_WriteData(yStart);	
	TFT_WriteCmd(0x0202);
}

/****************************************************************************
* Function Name  : TFT_ClearScreen
* Description    : 将LCD清屏成相应的颜色
* Input          : color：清屏颜色
* Output         : None
* Return         : None
****************************************************************************/
	  
void TFT_ClearScreen(uint16_t color)
{
 	uint16_t i, j ;

	TFT_SetWindow(0, 0, TFT_XMAX-1, TFT_YMAX-1);	 //作用区域
  	for(i=0; i<TFT_XMAX; i++)
	{
		for (j=0; j<TFT_YMAX; j++)
		{
			TFT_WriteData_Color(color);
		}
	}
}


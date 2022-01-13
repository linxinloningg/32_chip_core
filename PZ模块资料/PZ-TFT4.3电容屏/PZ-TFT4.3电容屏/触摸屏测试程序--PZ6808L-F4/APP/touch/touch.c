#include "touch.h"
#include "24cxx.h"
#include "SysTick.h"
#include "tftlcd.h"
#include "ft6236.h"
#include "gt5663.h"

#define TOUCH_AdjDelay500ms() delay_ms(500)

TouchTypeDef TouchData;         //定义用来存储读取到的数据
static PosTypeDef TouchAdj;     //定义一阵数据用来保存校正因数



void TOUCH_Init(void)
{
#ifdef TFTLCD_HX8352C
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOB,C,F时钟

	//GPIOB1,2初始化设置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;//PB1/PB2 设置为上拉输入
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//输入模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;//PB0设置为推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;//PC13设置为推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出模式
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PF11设置推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出模式
	GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化
	
	AT24CXX_Init();		//初始化24CXX
	AT24CXX_Read(TOUCH_ADJ_ADDR,&TouchAdj.posState,sizeof(TouchAdj));
	if(TouchAdj.posState != TOUCH_ADJ_OK)
    {
        TOUCH_Adjust(); //校正   
    }
#endif
	
#ifndef TFTLCD_ILI9488
	FT5206_Init();	
#endif
	
	
	
#ifdef TFTLCD_NT35510
	GT5663_Init();
#endif	
	
}


//SPI写数据
//向触摸屏IC写入1byte数据    
//num:要写入的数据
void TOUCH_Write_Byte(u8 num)    
{  
	u8 count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)TDIN=1;  
		else TDIN=0;   
		num<<=1;    
		TCLK=0; 
		delay_us(1);
		TCLK=1;		//上升沿有效	        
	}		 			    
}

//SPI读数据 
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据	   
u16 TOUCH_Read_AD(u8 CMD)	  
{ 	 
	u8 count=0; 	  
	u16 Num=0; 
	TCLK=0;		//先拉低时钟 	 
	TDIN=0; 	//拉低数据线
	TCS=0; 		//选中触摸屏IC
	TOUCH_Write_Byte(CMD);//发送命令字
	delay_us(6);//ADS7846的转换时间最长为6us
	TCLK=0; 	     	    
	delay_us(1);    	   
	TCLK=1;		//给1个时钟，清除BUSY
	delay_us(1);    
	TCLK=0; 	     	    
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				  
		Num<<=1; 	 
		TCLK=0;	//下降沿有效  	    	   
		delay_us(1);    
 		TCLK=1;
 		if(DOUT)Num++; 		 
	}  	
	Num>>=4;   	//只有高12位有效.
	TCS=1;		//释放片选	 
	return(Num);   
}

uint16_t TOUCH_ReadData(uint8_t cmd)
{
    uint8_t i, j;
    uint16_t readValue[TOUCH_READ_TIMES], value;
    uint32_t totalValue;

    /* 读取TOUCH_READ_TIMES次触摸值 */
    for(i=0; i<TOUCH_READ_TIMES; i++)
    {   /* 打开片选 */
        TCS=0;
        /* 在差分模式下，XPT2046转换需要24个时钟，8个时钟输入命令，之后1个时钟去除 */
        /* 忙信号，接着输出12位转换结果，剩下3个时钟是忽略位 */    
        readValue[i]=TOUCH_Read_AD(cmd); // 发送命令，选择X轴或者Y轴 
        
        TCS=1;
    }

    /* 滤波处理 */
    /* 首先从大到小排序 */
    for(i=0; i<(TOUCH_READ_TIMES - 1); i++)
    {
        for(j=i+1; j<TOUCH_READ_TIMES; j++)
        {
            /* 采样值从大到小排序排序 */
            if(readValue[i] < readValue[j])
            {
                value = readValue[i];
				readValue[i] = readValue[j];
				readValue[j] = value;
            }   
        }       
    }
   
    /* 去掉最大值，去掉最小值，求平均值 */
    j = TOUCH_READ_TIMES - 1;
    totalValue = 0;
    for(i=1; i<j; i++)     //求y的全部值
    {
        totalValue += readValue[i];
    }
    value = totalValue / (TOUCH_READ_TIMES - 2);
      
    return value;
}

uint8_t TOUCH_ReadXY(uint16_t *xValue, uint16_t *yValue)
{   
    uint16_t xValue1, yValue1, xValue2, yValue2;

    xValue1 = TOUCH_ReadData(TOUCH_X_CMD);
    yValue1 = TOUCH_ReadData(TOUCH_Y_CMD);
    xValue2 = TOUCH_ReadData(TOUCH_X_CMD);
    yValue2 = TOUCH_ReadData(TOUCH_Y_CMD);
    
    /* 查看两个点之间的只采样值差距 */
    if(xValue1 > xValue2)
    {
        *xValue = xValue1 - xValue2;
    }
    else
    {
        *xValue = xValue2 - xValue1;
    }

    if(yValue1 > yValue2)
    {
        *yValue = yValue1 - yValue2;
    }
    else
    {
        *yValue = yValue2 - yValue1;
    }

    /* 判断采样差值是否在可控范围内 */
	if((*xValue > TOUCH_MAX+0) || (*yValue > TOUCH_MAX+0))  
	{
		return 0xFF;
	}

    /* 求平均值 */
    *xValue = (xValue1 + xValue2) / 2;
    *yValue = (yValue1 + yValue2) / 2;

    /* 判断得到的值，是否在取值范围之内 */
    if((*xValue > TOUCH_X_MAX+0) || (*xValue < TOUCH_X_MIN) 
       || (*yValue > TOUCH_Y_MAX+0) || (*yValue < TOUCH_Y_MIN))
    {                   
        return 0xFF;
    }
 
    return 0; 
}

uint8_t TOUCH_ReadAdjust(uint16_t x, uint16_t y, uint16_t *xValue, uint16_t *yValue)
{
    uint8_t i;
    uint32_t timeCont;

    /* 读取校正点的坐标 */
    LCD_Clear(BACK_COLOR);
    LCD_DrowSign(x, y, RED);
    i = 0;
    while(1)
    {
        if(!TOUCH_ReadXY(xValue, yValue))
        {
            i++;
            if(i > 10)         //延时一下，以读取最佳值
            {
                LCD_DrowSign(x, y, BACK_COLOR);
                return 0;
            }
               
        }
		timeCont++;
        /* 超时退出 */
        if(timeCont > 0xFFFFFFFE)
        {   
                LCD_DrowSign(x, y, BACK_COLOR); 
                return 0xFF;
        } 
    }       
}

void TOUCH_Adjust(void)
{
#ifdef TFTLCD_HX8352C     
	uint16_t px[2], py[2], xPot[4], yPot[4];
    float xFactor, yFactor;

    /* 读取第一个点 */
    if(TOUCH_ReadAdjust(LCD_ADJX_MIN, LCD_ADJY_MIN, &xPot[0], &yPot[0]))
    {
        return;
    }   
    TOUCH_AdjDelay500ms();

    /* 读取第二个点 */
    if(TOUCH_ReadAdjust(LCD_ADJX_MIN, LCD_ADJY_MAX, &xPot[1], &yPot[1]))
    {
        return;
    }   
    TOUCH_AdjDelay500ms();

    /* 读取第三个点 */
    if(TOUCH_ReadAdjust(LCD_ADJX_MAX, LCD_ADJY_MIN, &xPot[2], &yPot[2]))
    {
        return;
    }   
    TOUCH_AdjDelay500ms();

    /* 读取第四个点 */
    if(TOUCH_ReadAdjust(LCD_ADJX_MAX, LCD_ADJY_MAX, &xPot[3], &yPot[3]))
    {
        return;
    }   
    TOUCH_AdjDelay500ms();
    
    /* 处理读取到的四个点的数据，整合成对角的两个点 */
    px[0] = (xPot[0] + xPot[1]) / 2;
    py[0] = (yPot[0] + yPot[2]) / 2;
    px[1] = (xPot[3] + xPot[2]) / 2;
    py[1] = (yPot[3] + yPot[1]) / 2;

    /* 求出比例因数 */
    xFactor = (float)LCD_ADJ_X / (px[1] - px[0]);
    yFactor = (float)LCD_ADJ_Y / (py[1] - py[0]);  
    
    /* 求出偏移量 */
    TouchAdj.xOffset = (int16_t)LCD_ADJX_MAX - ((float)px[1] * xFactor);
    TouchAdj.yOffset = (int16_t)LCD_ADJY_MAX - ((float)py[1] * yFactor);

    /* 将比例因数进行数据处理，然后保存 */
    TouchAdj.xFactor = xFactor ;
    TouchAdj.yFactor = yFactor ;
    
    TouchAdj.posState = TOUCH_ADJ_OK;
    AT24CXX_Write(TOUCH_ADJ_ADDR, &TouchAdj.posState, sizeof(TouchAdj)); 
#endif

#ifdef TFTLCD_ILI9488 
	return; //电容触摸屏不需要校准
#endif

#ifdef TFTLCD_NT35510
	return; //电容触摸屏不需要校准
#endif

}

//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
uint8_t TOUCH_Scan(void)
{
#ifdef TFTLCD_HX8352C
//    if(PEN == 0)   //查看是否有触摸
    {
        if(TOUCH_ReadXY(&TouchData.x, &TouchData.y)) //没有触摸
        {
            return 0;    
        }
        /* 根据物理坐标值，计算出彩屏坐标值 */
        TouchData.lcdx[0] = TouchData.x * TouchAdj.xFactor + TouchAdj.xOffset;
        TouchData.lcdy[0] = TouchData.y * TouchAdj.yFactor + TouchAdj.yOffset;
        
        /* 查看彩屏坐标值是否超过彩屏大小 */
        if(TouchData.lcdx[0] > tftlcd_data.width)
        {
            TouchData.lcdx[0] = tftlcd_data.width;
        }
        if(TouchData.lcdy[0] > tftlcd_data.height)
        {
            TouchData.lcdy[0] = tftlcd_data.height;
        }
        return 1; 
    }
 //   return 0xFF;
#endif
	
#ifdef TFTLCD_ILI9488
	return FT5206_Scan(0);	
#endif

#ifdef TFTLCD_NT35510
	return GT5663_Scan(0);
#endif
	
}








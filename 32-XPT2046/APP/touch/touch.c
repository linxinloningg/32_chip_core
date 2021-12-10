#include "touch.h"
#include "SysTick.h"
#include "tftlcd.h"
#include "spi.h"
#include "usart.h"

#define TOUCH_AdjDelay500ms() delay_ms(500)

TouchTypeDef TouchData;         //定义用来存储读取到的数据
static PosTypeDef TouchAdj;     //定义一阵数据用来保存校正因数
         

void TOUCH_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    /* SPI的IO口和SPI外设打开时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    /* TOUCH-CS的IO口设置 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* TOUCH-PEN的IO口设置 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    SPI1_Init();
	if(TouchAdj.posState != TOUCH_ADJ_OK)
    {
        TOUCH_Adjust(); //校正   
    }
}



uint16_t TOUCH_ReadData(uint8_t cmd)
{
    uint8_t i, j;
    uint16_t readValue[TOUCH_READ_TIMES], value;
    uint32_t totalValue;
	
	/* SPI的速度不宜过快 */
    SPI1_SetSpeed(SPI_BaudRatePrescaler_16);
	
    /* 读取TOUCH_READ_TIMES次触摸值 */
    for(i=0; i<TOUCH_READ_TIMES; i++)
    {   /* 打开片选 */
        TCS=0;
        /* 在差分模式下，XPT2046转换需要24个时钟，8个时钟输入命令，之后1个时钟去除 */
        /* 忙信号，接着输出12位转换结果，剩下3个时钟是忽略位 */    
        SPI1_ReadWriteByte(cmd); // 发送命令，选择X轴或者Y轴 
        
        /* 读取数据 */
        readValue[i] = SPI1_ReadWriteByte(0xFF);
        readValue[i] <<= 8;
        readValue[i] |= SPI1_ReadWriteByte(0xFF);
        
        /* 将数据处理，读取到的AD值的只有12位，最低三位无用 */
        readValue[i] >>= 3;
        
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
void TOUCH_Adjust(void)
{
    float xFactor, yFactor;
    /* 求出比例因数 */
    xFactor = (float)LCD_ADJ_X / (TOUCH_X_MAX - TOUCH_X_MIN);
    yFactor = (float)LCD_ADJ_Y / (TOUCH_Y_MAX - TOUCH_Y_MIN);  
    
    /* 求出偏移量 */
    TouchAdj.xOffset = (int16_t)LCD_ADJX_MAX - ((float)TOUCH_X_MAX * xFactor);
    TouchAdj.yOffset = (int16_t)LCD_ADJY_MAX - ((float)TOUCH_Y_MAX * yFactor);

    /* 将比例因数进行数据处理，然后保存 */
    TouchAdj.xFactor = xFactor ;
    TouchAdj.yFactor = yFactor ;
    
    TouchAdj.posState = TOUCH_ADJ_OK;           
}


uint8_t TOUCH_Scan(void)
{
    
    //if(PEN == 0)   //查看是否有触摸
    {
        if(TOUCH_ReadXY(&TouchData.x, &TouchData.y)) //没有触摸
        {
            return 0xFF;    
        }
        /* 根据物理坐标值，计算出彩屏坐标值 */
        TouchData.lcdx = TouchData.x * TouchAdj.xFactor + TouchAdj.xOffset;
        TouchData.lcdy = TouchData.y * TouchAdj.yFactor + TouchAdj.yOffset;
        
        /* 查看彩屏坐标值是否超过彩屏大小 */
        if(TouchData.lcdx > tftlcd_data.width)
        {
            TouchData.lcdx = tftlcd_data.width;
        }
        if(TouchData.lcdy > tftlcd_data.height)
        {
            TouchData.lcdy = tftlcd_data.height;
        }
        return 0; 
    }
 //   return 0xFF;       
}










#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "adc.h"
#include "tftlcd.h"



void ADC_DataPros(void)
{
	float vol;
	u16 adc_vol=0;
	u8 adc_buf[5];
	
	vol=(float)ADC_ConvertValue[0]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,80,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[1]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,100,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[2]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,120,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[3]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,140,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[4]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,160,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[5]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,180,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[6]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,200,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[7]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,220,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[8]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,240,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
	vol=(float)ADC_ConvertValue[9]*(3.3/4096);
	adc_vol=vol*100;
	adc_buf[0]=adc_vol/100+0x30;
	adc_buf[1]='.';
	adc_buf[2]=adc_vol%100/10+0x30;
	adc_buf[3]=adc_vol%100%10+0x30;
	adc_buf[4]='V';
	LCD_ShowString(10+10*8,260,tftlcd_data.width,tftlcd_data.height,16,adc_buf);
	
}

/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	USART1_Init(9600);
	LED_Init();
	TFTLCD_Init();
	ADCx_Init();
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ADC_DMA Test");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.cn");
	LCD_ShowString(10,80,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH0:");
	LCD_ShowString(10,100,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH1:");
	LCD_ShowString(10,120,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH2:");
	LCD_ShowString(10,140,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH3:");
	LCD_ShowString(10,160,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH4:");
	LCD_ShowString(10,180,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH10:");
	LCD_ShowString(10,200,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH11:");
	LCD_ShowString(10,220,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH12:");
	LCD_ShowString(10,240,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH13:");
	LCD_ShowString(10,260,tftlcd_data.width,tftlcd_data.height,16,"ADC_CH14:");
	
	FRONT_COLOR=RED;
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			led2=!led2;
		}
		
		if(i%50==0)
		{
			ADC_DataPros();
		}
		delay_ms(10);	
	}
}

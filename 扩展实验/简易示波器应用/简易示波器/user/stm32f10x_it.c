#include "stm32f10x_it.h"
#include "gui.h"
#include "system.h"
#include "SysTick.h"
#include "tim.h"
#include "adc.h"
#include "button.h"
#include "led.h"
u8 frequency_flag = 0;
long int shao_miao_shu_du = 0;
u8 num_shao_miao = 8;
u8 mode = 0;
u8 num_fu_du =7;
u8 ad_flag = 1;
float gao_pin_palus = 0;
u16 vcc_div = 0;
u16 vpp;
				
void set_io0(void)					  										
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);	
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);
}

void set_io1(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);     
}

void set_io2(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     
}

void set_io3(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io4(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io5(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io6(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io7(void)					  										
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io8(void)					  										
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io9(void)					  										
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io10(void)					  										
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_SetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}

void set_io11(void)					  										
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);	

	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);			   //  GPIO_SetBits
	GPIO_SetBits(GPIOA,GPIO_Pin_6);     	   //GPIO_ResetBits
}



void lcd_huadian(u16 a,u16 b,u16 color)
{							    
	GUI_Dot(a,210-b,color);
}

void lcd_huaxian(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	GUI_Line(x1,210-y1,x2,210-y2,color);	
}

void hua_wang(void)
{
	u8 index_y = 0;
	u8 index_hang = 0;	

    FRONT_COLOR = YELLOW;
	LCD_DrawRectangleex(0,9,250,210,FRONT_COLOR);
	GUI_Line(0,110,250,110,FRONT_COLOR);
	GUI_Line(125,9,125,210,FRONT_COLOR);

	FRONT_COLOR = GRAY;		
	for(index_hang = 0;index_hang<250;index_hang = index_hang + 25)
	{
		for(index_y = 0;index_y<200;index_y = index_y +5)
		{
			lcd_huadian(index_hang,index_y,FRONT_COLOR);	
		}
	}
	
	for(index_hang = 0;index_hang<200;index_hang = index_hang + 25)
	{
		for(index_y = 0;index_y<250;index_y = index_y +5)
		{
			lcd_huadian(index_y,index_hang,FRONT_COLOR);	
		}
	}

	FRONT_COLOR=RED;
}

void set_background(void)
{
	FRONT_COLOR = YELLOW;
    LCD_Clear(BLUE);
	
	LCD_DrawRectangleex(0,9,250,210,FRONT_COLOR);
	GUI_Line(0,110,250,110,FRONT_COLOR);
	GUI_Line(125,9,125,210,FRONT_COLOR);
	FRONT_COLOR=RED;
	GUI_Box(260,10,260+57,210,YELLOW);
	LCD_DrawRectangleex(259,9,260+58,211,GREEN);
	GUI_Show12ASCII(0,224,"www.prechin.com",FRONT_COLOR,WHITE);
	GUI_Show12ASCII(204,224,"mv",FRONT_COLOR,WHITE);	
	GUI_Show12ASCII(132,224,"vpp=",FRONT_COLOR,WHITE);
	GUI_Show12ASCII(260,10,"us/div:",FRONT_COLOR,WHITE);
	GUI_Show12ASCII(260,90,"mv/div:",FRONT_COLOR,WHITE);
	
	GUI_Show12ASCII(260,140,"PA2:",FRONT_COLOR,YELLOW);	
	GUI_Show12ASCII(260,160,"ADC1_In",FRONT_COLOR,YELLOW);
}

void key_init(void)
{
	EXTI_InitTypeDef   EXTI_InitTypeStruct;

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);	 //K_UP
	EXTI_InitTypeStruct.EXTI_Line = EXTI_Line0;
	EXTI_InitTypeStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitTypeStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitTypeStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitTypeStruct);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource3);	   //K_DOWN
	EXTI_InitTypeStruct.EXTI_Line = EXTI_Line3;
	EXTI_InitTypeStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitTypeStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitTypeStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitTypeStruct);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource4);	  //K_RIGHT
	EXTI_InitTypeStruct.EXTI_Line = EXTI_Line4;
	EXTI_InitTypeStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitTypeStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitTypeStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitTypeStruct);
}

void EXTI0_IRQHandler(void)
{
	u16 yan_se1;
	delay_ms(10);
	yan_se1 = FRONT_COLOR;
	FRONT_COLOR=RED;
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0))
	{
		mode++;
		led0=0;
		if(mode == 2)mode = 0;
		if(mode == 0)
		{
			GUI_Show12ASCII(260,224,"f",FRONT_COLOR,WHITE);
		}
		else GUI_Show12ASCII(260,224,"v",FRONT_COLOR,WHITE);
	}
	EXTI_ClearITPendingBit(EXTI_Line0);
	FRONT_COLOR = yan_se1;
}	   

void EXTI3_IRQHandler(void)
{
	delay_ms(10);
	led1=0;
	if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3)==0)
	{
		if(mode == 0)
		{
			num_shao_miao++;
			if(num_shao_miao == 22)num_shao_miao = 1;
		}
		else 
		{	
			num_fu_du++;
			if(num_fu_du==12)num_fu_du=1;
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line3);
}

void EXTI4_IRQHandler(void)
{
	delay_ms(10);
	led2=0;
	if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4)==0)
	{
		if(mode == 0)
		{
			num_shao_miao--;
			if(num_shao_miao == 0)num_shao_miao = 21;
		}
		else 
		{
					
			num_fu_du--;
			if(num_fu_du==0)num_fu_du=11;
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line4);
}

void TIM2_IRQHandler(void)
{
	u16 temple;
	u16 yan_se;
	u8 shao_miao_shu_du_buf[8],vcc_div_buf[8];
	if(TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
		TIM_Cmd(TIM3,DISABLE);
		TIM_Cmd(TIM2,DISABLE);

   		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		temple = TIM_GetCounter(TIM3);
		frequency = 65536*count+ temple;
		frequency = frequency - frequency*(130.10/1000000);

		switch(num_shao_miao)
		{
			case 1:shao_miao_shu_du = 347;gao_pin_palus = 1;break;
			case 2:shao_miao_shu_du = 694;gao_pin_palus = 2;break;
			case 3:shao_miao_shu_du = 1736;gao_pin_palus = 5;break;
			case 4:shao_miao_shu_du = 3472;gao_pin_palus = 10;break;
			case 5:shao_miao_shu_du = 6944;gao_pin_palus = 20;break;
			case 6:shao_miao_shu_du = 17361;gao_pin_palus = 50;break;
			case 7:shao_miao_shu_du = 34722;gao_pin_palus = 100;break;	  
			case 8:shao_miao_shu_du = 50;break;		  //·Ö½çµã£¬
			case 9:shao_miao_shu_du = 100;break;
			case 10:shao_miao_shu_du = 200;break;
			case 11:shao_miao_shu_du = 500;break;
			case 12:shao_miao_shu_du = 1000;break;
			case 13:shao_miao_shu_du = 2000;break;
			case 14:shao_miao_shu_du = 5000;break;
			case 15:shao_miao_shu_du = 10000;break;
			case 16:shao_miao_shu_du = 20000;break;
			case 17:shao_miao_shu_du = 50000;break;
			case 18:shao_miao_shu_du = 100000;break;
			case 19:shao_miao_shu_du = 200000;break;
			case 20:shao_miao_shu_du = 500000;break;
			case 21:shao_miao_shu_du = 1000000;break;


			default :break;
		}
		switch(num_fu_du)
		{
			case 1:vcc_div=1000;set_io1();break;
			case 2:vcc_div=950;set_io2();break;
			case 3:vcc_div=900;set_io3();break;
			case 4:vcc_div=800;set_io4();break;
			case 5:vcc_div=700;set_io5();break;
			case 6:vcc_div=600;set_io6();break;
			case 7:vcc_div=500;set_io7();break;
			case 8:vcc_div=400;set_io8();break;
			case 9:vcc_div=300;set_io9();break;
			case 10:vcc_div=200;set_io10();break;
			case 11:vcc_div=100;set_io11();break;
			default :break;
		}

		shao_miao_shu_du_buf[0]=shao_miao_shu_du/1000000+0x30;
		shao_miao_shu_du_buf[1]=shao_miao_shu_du%1000000/100000+0x30;
		shao_miao_shu_du_buf[2]=shao_miao_shu_du%1000000%100000/10000+0x30;
		shao_miao_shu_du_buf[3]=shao_miao_shu_du%1000000%100000%10000/1000+0x30;
		shao_miao_shu_du_buf[4]=shao_miao_shu_du%1000000%100000%10000%1000/100+0x30;
		shao_miao_shu_du_buf[5]=shao_miao_shu_du%1000000%100000%10000%1000%100/10+0x30;
		shao_miao_shu_du_buf[6]=shao_miao_shu_du%1000000%100000%10000%1000%100%10+0x30;
		shao_miao_shu_du_buf[7]='\0';

		vcc_div_buf[0]=vcc_div/1000000+0x30;
		vcc_div_buf[1]=vcc_div%1000000/100000+0x30;
		vcc_div_buf[2]=vcc_div%1000000%100000/10000+0x30;
		vcc_div_buf[3]=vcc_div%1000000%100000%10000/1000+0x30;
		vcc_div_buf[4]=vcc_div%1000000%100000%10000%1000/100+0x30;
		vcc_div_buf[5]=vcc_div%1000000%100000%10000%1000%100/10+0x30;
		vcc_div_buf[6]=vcc_div%1000000%100000%10000%1000%100%10+0x30;
		vcc_div_buf[7]='\0';
		yan_se = FRONT_COLOR;
		FRONT_COLOR=RED;
		if(frequency>20000)
		{
			frequency_flag = 1;	
		}
		else
		{
			frequency_flag = 0;			
		}

		if(num_shao_miao>7)
		{
			GUI_Show12ASCII(260,10,"us/div:",FRONT_COLOR,WHITE);
			GUI_Show12ASCII(260,26,shao_miao_shu_du_buf,FRONT_COLOR,WHITE);
		}
		else
		{
			GUI_Show12ASCII(260,10,"ns/div:",FRONT_COLOR,WHITE);
			GUI_Show12ASCII(260,26,shao_miao_shu_du_buf,FRONT_COLOR,WHITE);
		}
		GUI_Show12ASCII(260,106,vcc_div_buf,FRONT_COLOR,WHITE);				
		FRONT_COLOR=yan_se;

		count = 0;
		TIM_SetCounter(TIM2,0);
		TIM_SetCounter(TIM3,0);

		TIM_Cmd(TIM2,ENABLE);
     	TIM_Cmd(TIM3,ENABLE);
		led0=!led0;	  
	}
}

void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update))
	{
   		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		count++;
		
	}
}

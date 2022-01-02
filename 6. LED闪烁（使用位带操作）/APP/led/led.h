#ifndef _led_H
#define _led_H

#include "system.h"

/*  LED时钟端口、引脚定义 */
#define LED_PORT 			GPIOC   
#define LED_PIN 			(GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7)
#define LED_PORT_RCC		RCC_APB2Periph_GPIOC


#define led1 PCout(0)  	//D1指示灯连接的是PC0管脚
#define led2 PCout(1)  	//D2指示灯连接的是PC1管脚
#define led3 PCout(2)	//D3指示灯连接的是PC2管脚



void LED_Init(void);


#endif

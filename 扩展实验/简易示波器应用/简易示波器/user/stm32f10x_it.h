
#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"


extern u8 frequency_flag;
extern long int shao_miao_shu_du;
extern u8 num_shao_miao;
extern u8 mode;
extern u8 num_fu_du;
extern u8 ad_flag;
extern u16 vpp;
extern float gao_pin_palus;
extern u16 vcc_div;

void lcd_huadian(u16 a,u16 b,u16 color);
void lcd_huaxian(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);
void hua_wang(void);
void set_background(void);
void key_init(void);
void set_io0(void);
void set_io1(void);
void set_io2(void);
void set_io3(void);
void set_io4(void);
void set_io5(void);
void set_io6(void);
void set_io7(void);
void set_io8(void);
void set_io9(void);
void set_io10(void);
void set_io11(void);
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);


#ifdef __cplusplus
}
#endif

#endif /* __STM32F10x_IT_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

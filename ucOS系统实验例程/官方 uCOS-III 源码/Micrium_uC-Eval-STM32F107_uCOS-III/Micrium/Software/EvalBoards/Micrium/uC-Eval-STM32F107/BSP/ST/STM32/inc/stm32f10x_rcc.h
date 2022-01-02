/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : stm32f10x_rcc.h
* Author             : MCD Application Team
* Version            : V2.1.0RC2
* Date               : 03/13/2009
* Description        : This file contains all the functions prototypes for the
*                      RCC firmware library.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F10x_RCC_H
#define __STM32F10x_RCC_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_map.h"

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  u32 SYSCLK_Frequency;
  u32 HCLK_Frequency;
  u32 PCLK1_Frequency;
  u32 PCLK2_Frequency;
  u32 ADCCLK_Frequency;
}RCC_ClocksTypeDef;

/* Exported constants --------------------------------------------------------*/
/* HSE configuration */
#define RCC_HSE_OFF                      ((u32)0x00000000)
#define RCC_HSE_ON                       ((u32)0x00010000)
#define RCC_HSE_Bypass                   ((u32)0x00040000)

#define IS_RCC_HSE(HSE) (((HSE) == RCC_HSE_OFF) || ((HSE) == RCC_HSE_ON) || \
                         ((HSE) == RCC_HSE_Bypass))
    
/* PLL entry clock source */
#define RCC_PLLSource_HSI_Div2           ((u32)0x00000000)
#define RCC_PLLSource_HSE_Div1           ((u32)0x00010000)
#define RCC_PLLSource_HSE_Div2           ((u32)0x00030000)

#define IS_RCC_PLL_SOURCE(SOURCE) (((SOURCE) == RCC_PLLSource_HSI_Div2) || \
                                   ((SOURCE) == RCC_PLLSource_HSE_Div1) || \
                                   ((SOURCE) == RCC_PLLSource_HSE_Div2))

/* PLL1 entry clock source (only for STM32 connectivity line devices) */
#define RCC_PLL1Source_HSI_Div2           ((u32)0x00000000)
#define RCC_PLL1Source_PREDIV1            ((u32)0x00010000)

#define IS_RCC_PLL1_SOURCE(SOURCE) (((SOURCE) == RCC_PLL1Source_HSI_Div2) || \
                                    ((SOURCE) == RCC_PLL1Source_PREDIV1))

/* PLL multiplication factor */
#define RCC_PLLMul_2                     ((u32)0x00000000)
#define RCC_PLLMul_3                     ((u32)0x00040000)
#define RCC_PLLMul_4                     ((u32)0x00080000)
#define RCC_PLLMul_5                     ((u32)0x000C0000)
#define RCC_PLLMul_6                     ((u32)0x00100000)
#define RCC_PLLMul_7                     ((u32)0x00140000)
#define RCC_PLLMul_8                     ((u32)0x00180000)
#define RCC_PLLMul_9                     ((u32)0x001C0000)
#define RCC_PLLMul_10                    ((u32)0x00200000)
#define RCC_PLLMul_11                    ((u32)0x00240000)
#define RCC_PLLMul_12                    ((u32)0x00280000)
#define RCC_PLLMul_13                    ((u32)0x002C0000)
#define RCC_PLLMul_14                    ((u32)0x00300000)
#define RCC_PLLMul_15                    ((u32)0x00340000)
#define RCC_PLLMul_16                    ((u32)0x00380000)

#define IS_RCC_PLL_MUL(MUL) (((MUL) == RCC_PLLMul_2) || ((MUL) == RCC_PLLMul_3)   || \
                             ((MUL) == RCC_PLLMul_4) || ((MUL) == RCC_PLLMul_5)   || \
                             ((MUL) == RCC_PLLMul_6) || ((MUL) == RCC_PLLMul_7)   || \
                             ((MUL) == RCC_PLLMul_8) || ((MUL) == RCC_PLLMul_9)   || \
                             ((MUL) == RCC_PLLMul_10) || ((MUL) == RCC_PLLMul_11) || \
                             ((MUL) == RCC_PLLMul_12) || ((MUL) == RCC_PLLMul_13) || \
                             ((MUL) == RCC_PLLMul_14) || ((MUL) == RCC_PLLMul_15) || \
                             ((MUL) == RCC_PLLMul_16))

/* PLL1 multiplication factor (only for STM32 connectivity line devices) */
#define RCC_PLL1Mul_4                    ((u32)0x00080000)
#define RCC_PLL1Mul_5                    ((u32)0x000C0000)
#define RCC_PLL1Mul_6                    ((u32)0x00100000)
#define RCC_PLL1Mul_7                    ((u32)0x00140000)
#define RCC_PLL1Mul_8                    ((u32)0x00180000)
#define RCC_PLL1Mul_9                    ((u32)0x001C0000)
#define RCC_PLL1Mul_6_5                  ((u32)0x00340000)

#define IS_RCC_PLL1_MUL(MUL) (((MUL) == RCC_PLL1Mul_4) || ((MUL) == RCC_PLL1Mul_5) || \
                              ((MUL) == RCC_PLL1Mul_6) || ((MUL) == RCC_PLL1Mul_7) || \
                              ((MUL) == RCC_PLL1Mul_8) || ((MUL) == RCC_PLL1Mul_9) || \
                              ((MUL) == RCC_PLL1Mul_6_5))

/* PREDIV1 division factor (only for STM32 connectivity line devices) */
#define  RCC_PREDIV1_Div1                ((u32)0x00000000)
#define  RCC_PREDIV1_Div2                ((u32)0x00000001)
#define  RCC_PREDIV1_Div3                ((u32)0x00000002)
#define  RCC_PREDIV1_Div4                ((u32)0x00000003)
#define  RCC_PREDIV1_Div5                ((u32)0x00000004)
#define  RCC_PREDIV1_Div6                ((u32)0x00000005)
#define  RCC_PREDIV1_Div7                ((u32)0x00000006)
#define  RCC_PREDIV1_Div8                ((u32)0x00000007)
#define  RCC_PREDIV1_Div9                ((u32)0x00000008)
#define  RCC_PREDIV1_Div10               ((u32)0x00000009)
#define  RCC_PREDIV1_Div11               ((u32)0x0000000A)
#define  RCC_PREDIV1_Div12               ((u32)0x0000000B)
#define  RCC_PREDIV1_Div13               ((u32)0x0000000C)
#define  RCC_PREDIV1_Div14               ((u32)0x0000000D)
#define  RCC_PREDIV1_Div15               ((u32)0x0000000E)
#define  RCC_PREDIV1_Div16               ((u32)0x0000000F)

#define IS_RCC_PREDIV1(PREDIV1) (((PREDIV1) == RCC_PREDIV1_Div1) || ((PREDIV1) == RCC_PREDIV1_Div2) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div3) || ((PREDIV1) == RCC_PREDIV1_Div4) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div5) || ((PREDIV1) == RCC_PREDIV1_Div6) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div7) || ((PREDIV1) == RCC_PREDIV1_Div8) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div9) || ((PREDIV1) == RCC_PREDIV1_Div10) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div11) || ((PREDIV1) == RCC_PREDIV1_Div12) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div13) || ((PREDIV1) == RCC_PREDIV1_Div14) || \
                                 ((PREDIV1) == RCC_PREDIV1_Div15) || ((PREDIV1) == RCC_PREDIV1_Div16))

/* PREDIV1 clock source (only for STM32 connectivity line devices) */
#define  RCC_PREDIV1_Source_HSE          ((u32)0x00000000) 
#define  RCC_PREDIV1_Source_PLL2         ((u32)0x00010000) 

#define IS_RCC_PREDIV1_SOURCE(SOURCE) (((SOURCE) == RCC_PREDIV1_Source_HSE) || \
                                       ((SOURCE) == RCC_PREDIV1_Source_PLL2)) 

/* PREDIV2 division factor (only for STM32 connectivity line devices) */
#define  RCC_PREDIV2_Div1                ((u32)0x00000000)
#define  RCC_PREDIV2_Div2                ((u32)0x00000010)
#define  RCC_PREDIV2_Div3                ((u32)0x00000020)
#define  RCC_PREDIV2_Div4                ((u32)0x00000030)
#define  RCC_PREDIV2_Div5                ((u32)0x00000040)
#define  RCC_PREDIV2_Div6                ((u32)0x00000050)
#define  RCC_PREDIV2_Div7                ((u32)0x00000060)
#define  RCC_PREDIV2_Div8                ((u32)0x00000070)
#define  RCC_PREDIV2_Div9                ((u32)0x00000080)
#define  RCC_PREDIV2_Div10               ((u32)0x00000090)
#define  RCC_PREDIV2_Div11               ((u32)0x000000A0)
#define  RCC_PREDIV2_Div12               ((u32)0x000000B0)
#define  RCC_PREDIV2_Div13               ((u32)0x000000C0)
#define  RCC_PREDIV2_Div14               ((u32)0x000000D0)
#define  RCC_PREDIV2_Div15               ((u32)0x000000E0)
#define  RCC_PREDIV2_Div16               ((u32)0x000000F0)

#define IS_RCC_PREDIV2(PREDIV2) (((PREDIV2) == RCC_PREDIV2_Div1) || ((PREDIV2) == RCC_PREDIV2_Div2) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div3) || ((PREDIV2) == RCC_PREDIV2_Div4) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div5) || ((PREDIV2) == RCC_PREDIV2_Div6) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div7) || ((PREDIV2) == RCC_PREDIV2_Div8) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div9) || ((PREDIV2) == RCC_PREDIV2_Div10) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div11) || ((PREDIV2) == RCC_PREDIV2_Div12) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div13) || ((PREDIV2) == RCC_PREDIV2_Div14) || \
                                 ((PREDIV2) == RCC_PREDIV2_Div15) || ((PREDIV2) == RCC_PREDIV2_Div16))

/* PLL2 multiplication factor (only for STM32 connectivity line devices) */
#define  RCC_PLL2Mul_8                   ((u32)0x00000600)
#define  RCC_PLL2Mul_9                   ((u32)0x00000700)
#define  RCC_PLL2Mul_10                  ((u32)0x00000800)
#define  RCC_PLL2Mul_11                  ((u32)0x00000900)
#define  RCC_PLL2Mul_12                  ((u32)0x00000A00)
#define  RCC_PLL2Mul_13                  ((u32)0x00000B00)
#define  RCC_PLL2Mul_14                  ((u32)0x00000C00)
#define  RCC_PLL2Mul_16                  ((u32)0x00000E00)
#define  RCC_PLL2Mul_20                  ((u32)0x00000F00)

#define IS_RCC_PLL2_MUL(MUL) (((MUL) == RCC_PLL2Mul_8) || ((MUL) == RCC_PLL2Mul_9)  || \
                              ((MUL) == RCC_PLL2Mul_10) || ((MUL) == RCC_PLL2Mul_11) || \
                              ((MUL) == RCC_PLL2Mul_12) || ((MUL) == RCC_PLL2Mul_13) || \
                              ((MUL) == RCC_PLL2Mul_14) || ((MUL) == RCC_PLL2Mul_16) || \
                              ((MUL) == RCC_PLL2Mul_20))

/* PLL3 multiplication factor (only for STM32 connectivity line devices) */
#define  RCC_PLL3Mul_8                   ((u32)0x00006000)
#define  RCC_PLL3Mul_9                   ((u32)0x00007000)
#define  RCC_PLL3Mul_10                  ((u32)0x00008000)
#define  RCC_PLL3Mul_11                  ((u32)0x00009000)
#define  RCC_PLL3Mul_12                  ((u32)0x0000A000)
#define  RCC_PLL3Mul_13                  ((u32)0x0000B000)
#define  RCC_PLL3Mul_14                  ((u32)0x0000C000)
#define  RCC_PLL3Mul_16                  ((u32)0x0000E000)
#define  RCC_PLL3Mul_20                  ((u32)0x0000F000)

#define IS_RCC_PLL3_MUL(MUL) (((MUL) == RCC_PLL3Mul_8) || ((MUL) == RCC_PLL3Mul_9)  || \
                              ((MUL) == RCC_PLL3Mul_10) || ((MUL) == RCC_PLL3Mul_11) || \
                              ((MUL) == RCC_PLL3Mul_12) || ((MUL) == RCC_PLL3Mul_13) || \
                              ((MUL) == RCC_PLL3Mul_14) || ((MUL) == RCC_PLL3Mul_16) || \
                              ((MUL) == RCC_PLL3Mul_20))

/* System clock source */
#define RCC_SYSCLKSource_HSI             ((u32)0x00000000)
#define RCC_SYSCLKSource_HSE             ((u32)0x00000001)
#define RCC_SYSCLKSource_PLLCLK          ((u32)0x00000002)
#define RCC_SYSCLKSource_PLL1CLK         ((u32)0x00000002) /* Only for STM32 connectivity line devices */

#define IS_RCC_SYSCLK_SOURCE(SOURCE) (((SOURCE) == RCC_SYSCLKSource_HSI) || \
                                      ((SOURCE) == RCC_SYSCLKSource_HSE) || \
                                      ((SOURCE) == RCC_SYSCLKSource_PLLCLK))

/* AHB clock source */
#define RCC_SYSCLK_Div1                  ((u32)0x00000000)
#define RCC_SYSCLK_Div2                  ((u32)0x00000080)
#define RCC_SYSCLK_Div4                  ((u32)0x00000090)
#define RCC_SYSCLK_Div8                  ((u32)0x000000A0)
#define RCC_SYSCLK_Div16                 ((u32)0x000000B0)
#define RCC_SYSCLK_Div64                 ((u32)0x000000C0)
#define RCC_SYSCLK_Div128                ((u32)0x000000D0)
#define RCC_SYSCLK_Div256                ((u32)0x000000E0)
#define RCC_SYSCLK_Div512                ((u32)0x000000F0)

#define IS_RCC_HCLK(HCLK) (((HCLK) == RCC_SYSCLK_Div1) || ((HCLK) == RCC_SYSCLK_Div2) || \
                           ((HCLK) == RCC_SYSCLK_Div4) || ((HCLK) == RCC_SYSCLK_Div8) || \
                           ((HCLK) == RCC_SYSCLK_Div16) || ((HCLK) == RCC_SYSCLK_Div64) || \
                           ((HCLK) == RCC_SYSCLK_Div128) || ((HCLK) == RCC_SYSCLK_Div256) || \
                           ((HCLK) == RCC_SYSCLK_Div512))

/* APB1/APB2 clock source */
#define RCC_HCLK_Div1                    ((u32)0x00000000)
#define RCC_HCLK_Div2                    ((u32)0x00000400)
#define RCC_HCLK_Div4                    ((u32)0x00000500)
#define RCC_HCLK_Div8                    ((u32)0x00000600)
#define RCC_HCLK_Div16                   ((u32)0x00000700)

#define IS_RCC_PCLK(PCLK) (((PCLK) == RCC_HCLK_Div1) || ((PCLK) == RCC_HCLK_Div2) || \
                           ((PCLK) == RCC_HCLK_Div4) || ((PCLK) == RCC_HCLK_Div8) || \
                           ((PCLK) == RCC_HCLK_Div16))

/* RCC Interrupt source */
#define RCC_IT_LSIRDY                    ((u8)0x01)
#define RCC_IT_LSERDY                    ((u8)0x02)
#define RCC_IT_HSIRDY                    ((u8)0x04)
#define RCC_IT_HSERDY                    ((u8)0x08)
#define RCC_IT_PLLRDY                    ((u8)0x10)
#define RCC_IT_PLL1RDY                   ((u8)0x10) /* Only for STM32 connectivity line devices */
#define RCC_IT_PLL2RDY                   ((u8)0x20) /* Only for STM32 connectivity line devices */
#define RCC_IT_PLL3RDY                   ((u8)0x40) /* Only for STM32 connectivity line devices */
#define RCC_IT_CSS                       ((u8)0x80)

#define IS_RCC_IT(IT) ((((IT) & (u8)0x80) == 0x00) && ((IT) != 0x00))
#define IS_RCC_GET_IT(IT) (((IT) == RCC_IT_LSIRDY) || ((IT) == RCC_IT_LSERDY) || \
                           ((IT) == RCC_IT_HSIRDY) || ((IT) == RCC_IT_HSERDY) || \
                           ((IT) == RCC_IT_PLLRDY) || ((IT) == RCC_IT_CSS) || \
                           ((IT) == RCC_IT_PLL2RDY) || ((IT) == RCC_IT_PLL3RDY))
#define IS_RCC_CLEAR_IT(IT) ((((IT) & (u8)0x00) == 0x00) && ((IT) != 0x00))

/* USB clock source */
#define RCC_USBCLKSource_PLLCLK_1Div5    ((u8)0x00)
#define RCC_USBCLKSource_PLLCLK_Div1     ((u8)0x01)

#define IS_RCC_USBCLK_SOURCE(SOURCE) (((SOURCE) == RCC_USBCLKSource_PLLCLK_1Div5) || \
                                      ((SOURCE) == RCC_USBCLKSource_PLLCLK_Div1))

/* USB OTG FS clock source (only for STM32 connectivity line devices) */
#define RCC_OTGFSCLKSource_PLL1VCO_Div3   ((u8)0x00)
#define RCC_OTGFSCLKSource_PLL1VCO_Div2   ((u8)0x01)

#define IS_RCC_OTGFSCLK_SOURCE(SOURCE) (((SOURCE) == RCC_OTGFSCLKSource_PLL1VCO_Div3) || \
                                        ((SOURCE) == RCC_OTGFSCLKSource_PLL1VCO_Div2))

/* ADC clock source */
#define RCC_PCLK2_Div2                   ((u32)0x00000000)
#define RCC_PCLK2_Div4                   ((u32)0x00004000)
#define RCC_PCLK2_Div6                   ((u32)0x00008000)
#define RCC_PCLK2_Div8                   ((u32)0x0000C000)

#define IS_RCC_ADCCLK(ADCCLK) (((ADCCLK) == RCC_PCLK2_Div2) || ((ADCCLK) == RCC_PCLK2_Div4) || \
                               ((ADCCLK) == RCC_PCLK2_Div6) || ((ADCCLK) == RCC_PCLK2_Div8))

/* I2S2 clock source (only for STM32 connectivity line devices) */
#define RCC_I2S2CLKSource_SYSCLK         ((u8)0x00)
#define RCC_I2S2CLKSource_PLL3_VCO       ((u8)0x01)

#define IS_RCC_I2S2CLK_SOURCE(SOURCE) (((SOURCE) == RCC_I2S2CLKSource_SYSCLK) || \
                                       ((SOURCE) == RCC_I2S2CLKSource_PLL3_VCO))

/* I2S3 clock source (only for STM32 connectivity line devices) */
#define RCC_I2S3CLKSource_SYSCLK         ((u8)0x00)
#define RCC_I2S3CLKSource_PLL3_VCO       ((u8)0x01)

#define IS_RCC_I2S3CLK_SOURCE(SOURCE) (((SOURCE) == RCC_I2S3CLKSource_SYSCLK) || \
                                       ((SOURCE) == RCC_I2S3CLKSource_PLL3_VCO))    

/* LSE configuration */
#define RCC_LSE_OFF                      ((u8)0x00)
#define RCC_LSE_ON                       ((u8)0x01)
#define RCC_LSE_Bypass                   ((u8)0x04)

#define IS_RCC_LSE(LSE) (((LSE) == RCC_LSE_OFF) || ((LSE) == RCC_LSE_ON) || \
                         ((LSE) == RCC_LSE_Bypass))

/* RTC clock source */
#define RCC_RTCCLKSource_LSE             ((u32)0x00000100)
#define RCC_RTCCLKSource_LSI             ((u32)0x00000200)
#define RCC_RTCCLKSource_HSE_Div128      ((u32)0x00000300)

#define IS_RCC_RTCCLK_SOURCE(SOURCE) (((SOURCE) == RCC_RTCCLKSource_LSE) || \
                                      ((SOURCE) == RCC_RTCCLKSource_LSI) || \
                                      ((SOURCE) == RCC_RTCCLKSource_HSE_Div128))

/* AHB peripheral */
#define RCC_AHBPeriph_DMA1               ((u32)0x00000001)
#define RCC_AHBPeriph_DMA2               ((u32)0x00000002)
#define RCC_AHBPeriph_SRAM               ((u32)0x00000004)
#define RCC_AHBPeriph_FLITF              ((u32)0x00000010)
#define RCC_AHBPeriph_CRC                ((u32)0x00000040)
#define RCC_AHBPeriph_FSMC               ((u32)0x00000100)
#define RCC_AHBPeriph_SDIO               ((u32)0x00000400)
#define RCC_AHBPeriph_OTG_FS             ((u32)0x00001000) /* Only for STM32 connectivity line devices */
#define RCC_AHBPeriph_ETH_MAC            ((u32)0x00004000) /* Only for STM32 connectivity line devices */
#define RCC_AHBPeriph_ETH_MAC_Tx         ((u32)0x00008000) /* Only for STM32 connectivity line devices */
#define RCC_AHBPeriph_ETH_MAC_Rx         ((u32)0x00010000) /* Only for STM32 connectivity line devices */

#define IS_RCC_AHB_PERIPH(PERIPH) ((((PERIPH) & 0xFFFE2AA8) == 0x00) && ((PERIPH) != 0x00))
#define IS_RCC_AHB_PERIPH_RESET(PERIPH) ((((PERIPH) & 0xFFFFAFFF) == 0x00) && ((PERIPH) != 0x00))

/* APB2 peripheral */
#define RCC_APB2Periph_AFIO              ((u32)0x00000001)
#define RCC_APB2Periph_GPIOA             ((u32)0x00000004)
#define RCC_APB2Periph_GPIOB             ((u32)0x00000008)
#define RCC_APB2Periph_GPIOC             ((u32)0x00000010)
#define RCC_APB2Periph_GPIOD             ((u32)0x00000020)
#define RCC_APB2Periph_GPIOE             ((u32)0x00000040)
#define RCC_APB2Periph_GPIOF             ((u32)0x00000080)
#define RCC_APB2Periph_GPIOG             ((u32)0x00000100)
#define RCC_APB2Periph_ADC1              ((u32)0x00000200)
#define RCC_APB2Periph_ADC2              ((u32)0x00000400)
#define RCC_APB2Periph_TIM1              ((u32)0x00000800)
#define RCC_APB2Periph_SPI1              ((u32)0x00001000)
#define RCC_APB2Periph_TIM8              ((u32)0x00002000)
#define RCC_APB2Periph_USART1            ((u32)0x00004000)
#define RCC_APB2Periph_ADC3              ((u32)0x00008000)

#define IS_RCC_APB2_PERIPH(PERIPH) ((((PERIPH) & 0xFFFF0002) == 0x00) && ((PERIPH) != 0x00))

/* APB1 peripheral */
#define RCC_APB1Periph_TIM2              ((u32)0x00000001)
#define RCC_APB1Periph_TIM3              ((u32)0x00000002)
#define RCC_APB1Periph_TIM4              ((u32)0x00000004)
#define RCC_APB1Periph_TIM5              ((u32)0x00000008)
#define RCC_APB1Periph_TIM6              ((u32)0x00000010)
#define RCC_APB1Periph_TIM7              ((u32)0x00000020)
#define RCC_APB1Periph_WWDG              ((u32)0x00000800)
#define RCC_APB1Periph_SPI2              ((u32)0x00004000)
#define RCC_APB1Periph_SPI3              ((u32)0x00008000)
#define RCC_APB1Periph_USART2            ((u32)0x00020000)
#define RCC_APB1Periph_USART3            ((u32)0x00040000)
#define RCC_APB1Periph_UART4             ((u32)0x00080000)
#define RCC_APB1Periph_UART5             ((u32)0x00100000)
#define RCC_APB1Periph_I2C1              ((u32)0x00200000)
#define RCC_APB1Periph_I2C2              ((u32)0x00400000)
#define RCC_APB1Periph_USB               ((u32)0x00800000)
#define RCC_APB1Periph_CAN1              ((u32)0x02000000)
#define RCC_APB1Periph_CAN2              ((u32)0x04000000) /* Only for STM32 connectivity line devices */
#define RCC_APB1Periph_BKP               ((u32)0x08000000)
#define RCC_APB1Periph_PWR               ((u32)0x10000000)
#define RCC_APB1Periph_DAC               ((u32)0x20000000)

#define IS_RCC_APB1_PERIPH(PERIPH) ((((PERIPH) & 0xC10137C0) == 0x00) && ((PERIPH) != 0x00))

/* Clock source to output on MCO pin */
#define RCC_MCO_NoClock                  ((u8)0x00)
#define RCC_MCO_SYSCLK                   ((u8)0x04)
#define RCC_MCO_HSI                      ((u8)0x05)
#define RCC_MCO_HSE                      ((u8)0x06)
#define RCC_MCO_PLLCLK_Div2              ((u8)0x07)
#define RCC_MCO_PLL1CLK_Div2             ((u8)0x07) /* Only for STM32 connectivity line devices */
#define RCC_MCO_PLL2CLK                  ((u8)0x08) /* Only for STM32 connectivity line devices */
#define RCC_MCO_PLL3CLK_Div2             ((u8)0x09) /* Only for STM32 connectivity line devices */
#define RCC_MCO_XT1                      ((u8)0x0A) /* Only for STM32 connectivity line devices */
#define RCC_MCO_PLL3CLK                  ((u8)0x0B) /* Only for STM32 connectivity line devices */

#define IS_RCC_MCO(MCO) (((MCO) == RCC_MCO_NoClock) || ((MCO) == RCC_MCO_HSI) || \
                         ((MCO) == RCC_MCO_SYSCLK)  || ((MCO) == RCC_MCO_HSE) || \
                         ((MCO) == RCC_MCO_PLLCLK_Div2) || ((MCO) == RCC_MCO_PLL2CLK) || \
                         ((MCO) == RCC_MCO_PLL3CLK_Div2) || ((MCO) == RCC_MCO_XT1) || \
                         ((MCO) == RCC_MCO_PLL3CLK))

/* RCC Flag */
#define RCC_FLAG_HSIRDY                  ((u8)0x21)
#define RCC_FLAG_HSERDY                  ((u8)0x31)
#define RCC_FLAG_PLLRDY                  ((u8)0x39)
#define RCC_FLAG_PLL1RDY                 ((u8)0x39) /* Only for STM32 connectivity line devices */
#define RCC_FLAG_PLL2RDY                 ((u8)0x3B) /* Only for STM32 connectivity line devices */
#define RCC_FLAG_PLL3RDY                 ((u8)0x3D) /* Only for STM32 connectivity line devices */
#define RCC_FLAG_LSERDY                  ((u8)0x41)
#define RCC_FLAG_LSIRDY                  ((u8)0x61)
#define RCC_FLAG_PINRST                  ((u8)0x7A)
#define RCC_FLAG_PORRST                  ((u8)0x7B)
#define RCC_FLAG_SFTRST                  ((u8)0x7C)
#define RCC_FLAG_IWDGRST                 ((u8)0x7D)
#define RCC_FLAG_WWDGRST                 ((u8)0x7E)
#define RCC_FLAG_LPWRRST                 ((u8)0x7F)

#define IS_RCC_FLAG(FLAG) (((FLAG) == RCC_FLAG_HSIRDY) || ((FLAG) == RCC_FLAG_HSERDY) || \
                           ((FLAG) == RCC_FLAG_PLLRDY) || ((FLAG) == RCC_FLAG_LSERDY) || \
                           ((FLAG) == RCC_FLAG_PLL2RDY) || ((FLAG) == RCC_FLAG_PLL3RDY) || \
                           ((FLAG) == RCC_FLAG_LSIRDY) || ((FLAG) == RCC_FLAG_PINRST) || \
                           ((FLAG) == RCC_FLAG_PORRST) || ((FLAG) == RCC_FLAG_SFTRST) || \
                           ((FLAG) == RCC_FLAG_IWDGRST)|| ((FLAG) == RCC_FLAG_WWDGRST)|| \
                           ((FLAG) == RCC_FLAG_LPWRRST))

#define IS_RCC_CALIBRATION_VALUE(VALUE) ((VALUE) <= 0x1F)

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void RCC_DeInit(void);
void RCC_HSEConfig(u32 RCC_HSE);
ErrorStatus RCC_WaitForHSEStartUp(void);
void RCC_AdjustHSICalibrationValue(u8 HSICalibrationValue);
void RCC_HSICmd(FunctionalState NewState);
void RCC_PLLConfig(u32 RCC_PLLSource, u32 RCC_PLLMul);
void RCC_PLLCmd(FunctionalState NewState);
void RCC_PREDIV1Config(u32 RCC_PREDIV1_Source, u32 RCC_PREDIV1_Div); /* Only for STM32 connectivity line devices */                               
void RCC_PREDIV2Config(u32 RCC_PREDIV2_Div);                         /* Only for STM32 connectivity line devices */
void RCC_PLL1Config(u32 RCC_PLL1Source, u32 RCC_PLL1Mul);            /* Only for STM32 connectivity line devices */
void RCC_PLL1Cmd(FunctionalState NewState);                          /* Only for STM32 connectivity line devices */
void RCC_PLL2Config(u32 RCC_PLL2Mul);                                /* Only for STM32 connectivity line devices */
void RCC_PLL2Cmd(FunctionalState NewState);                          /* Only for STM32 connectivity line devices */
void RCC_PLL3Config(u32 RCC_PLL3Mul);                                /* Only for STM32 connectivity line devices */
void RCC_PLL3Cmd(FunctionalState NewState);                          /* Only for STM32 connectivity line devices */
void RCC_SYSCLKConfig(u32 RCC_SYSCLKSource);
u8 RCC_GetSYSCLKSource(void);
void RCC_HCLKConfig(u32 RCC_SYSCLK);
void RCC_PCLK1Config(u32 RCC_HCLK);
void RCC_PCLK2Config(u32 RCC_HCLK);
void RCC_ITConfig(u8 RCC_IT, FunctionalState NewState);
void RCC_USBCLKConfig(u32 RCC_USBCLKSource);
void RCC_ADCCLKConfig(u32 RCC_PCLK2);
void RCC_OTGFSCLKConfig(u32 RCC_OTGFSCLKSource); /* Only for STM32 connectivity line devices */
void RCC_I2S2CLKConfig(u32 RCC_I2S2CLKSource);   /* Only for STM32 connectivity line devices */                                  
void RCC_I2S3CLKConfig(u32 RCC_I2S3CLKSource);   /* Only for STM32 connectivity line devices */
void RCC_LSEConfig(u8 RCC_LSE);
void RCC_LSICmd(FunctionalState NewState);
void RCC_RTCCLKConfig(u32 RCC_RTCCLKSource);
void RCC_RTCCLKCmd(FunctionalState NewState);
void RCC_GetClocksFreq(RCC_ClocksTypeDef* RCC_Clocks);
void RCC_AHBPeriphClockCmd(u32 RCC_AHBPeriph, FunctionalState NewState);
void RCC_APB2PeriphClockCmd(u32 RCC_APB2Periph, FunctionalState NewState);
void RCC_APB1PeriphClockCmd(u32 RCC_APB1Periph, FunctionalState NewState);
void RCC_AHBPeriphResetCmd(u32 RCC_AHBPeriph, FunctionalState NewState); /* Only for STM32 connectivity line devices */
void RCC_APB2PeriphResetCmd(u32 RCC_APB2Periph, FunctionalState NewState);
void RCC_APB1PeriphResetCmd(u32 RCC_APB1Periph, FunctionalState NewState);
void RCC_BackupResetCmd(FunctionalState NewState);
void RCC_ClockSecuritySystemCmd(FunctionalState NewState);
void RCC_MCOConfig(u8 RCC_MCO);
FlagStatus RCC_GetFlagStatus(u8 RCC_FLAG);
void RCC_ClearFlag(void);
ITStatus RCC_GetITStatus(u8 RCC_IT);
void RCC_ClearITPendingBit(u8 RCC_IT);

#endif /* __STM32F10x_RCC_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

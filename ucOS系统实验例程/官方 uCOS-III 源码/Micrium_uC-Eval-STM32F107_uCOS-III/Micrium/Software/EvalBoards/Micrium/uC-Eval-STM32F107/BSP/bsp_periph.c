/*
*********************************************************************************************************
*                                     MICIRUM BOARD SUPPORT PACKAGE
*
*                            (c) Copyright 2007-2008; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        BOARD SUPPORT PACKAGE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                     Micrium uC-Eval-STM32F107 
*                                        Evaluation Board
*
* Filename      : bsp_periph.c
* Version       : V1.00
* Programmer(s) : EHS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  BSP_PERIPH_MODULE
#include <bsp.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  BSP_PERIPH_REG_RCC_BASE                 0x40021000
#define  BSP_PERIPH_REG_RCC_CFGR            (*(volatile  CPU_INT32U *)(BSP_PERIPH_REG_RCC_BASE + 0x004))
#define  BSP_PERIPH_REG_RCC_AHBENR          (*(volatile  CPU_INT32U *)(BSP_PERIPH_REG_RCC_BASE + 0x014))
#define  BSP_PERIPH_REG_RCC_APB2ENR         (*(volatile  CPU_INT32U *)(BSP_PERIPH_REG_RCC_BASE + 0x018))
#define  BSP_PERIPH_REG_RCC_APB1ENR         (*(volatile  CPU_INT32U *)(BSP_PERIPH_REG_RCC_BASE + 0x01C))


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         BSP_PeriphClkFreqGet()
*
* Description : Get clock frequency of a peripheral.
*
* Argument(s) : pwr_clk_id      Power/clock ID.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  BSP_PeriphClkFreqGet (CPU_DATA  pwr_clk_id)
{
    CPU_INT32U  clk_freq;
    CPU_INT32U  clk_div;


    clk_freq = BSP_CPU_ClkFreq();

    switch (pwr_clk_id) {
        case BSP_PERIPH_ID_DMA1:
        case BSP_PERIPH_ID_DMA2:
        case BSP_PERIPH_ID_SRAM:
        case BSP_PERIPH_ID_FLITF:
        case BSP_PERIPH_ID_CRC:
        case BSP_PERIPH_ID_OTGFS:
        case BSP_PERIPH_ID_ETHMAC:
        case BSP_PERIPH_ID_ETHMACTX:   
             return (clk_freq);

        case BSP_PERIPH_ID_AFIO:
        case BSP_PERIPH_ID_IOPA:
        case BSP_PERIPH_ID_IOPB:
        case BSP_PERIPH_ID_IOPC:
        case BSP_PERIPH_ID_IOPD:
        case BSP_PERIPH_ID_IOPE:
        case BSP_PERIPH_ID_ADC1:
        case BSP_PERIPH_ID_ADC2:
        case BSP_PERIPH_ID_TIM1:
        case BSP_PERIPH_ID_SPI1:
        case BSP_PERIPH_ID_USART1:
             clk_div = (BSP_PERIPH_REG_RCC_CFGR & 0x00003800) >> 11;
             if (clk_div < 4) {
                 return (clk_freq);
             }
             clk_div   = (clk_div - 3) << 1;
             clk_freq /= clk_div;
             return (clk_freq);

        case BSP_PERIPH_ID_TIM2:
        case BSP_PERIPH_ID_TIM3:
        case BSP_PERIPH_ID_TIM4:
        case BSP_PERIPH_ID_TIM5:
        case BSP_PERIPH_ID_TIM6:
        case BSP_PERIPH_ID_TIM7:
        case BSP_PERIPH_ID_WWDG:
        case BSP_PERIPH_ID_SPI2:
        case BSP_PERIPH_ID_SPI3:
        case BSP_PERIPH_ID_USART2:
        case BSP_PERIPH_ID_USART3:
        case BSP_PERIPH_ID_USART4:
        case BSP_PERIPH_ID_USART5:
        case BSP_PERIPH_ID_I2C1:
        case BSP_PERIPH_ID_I2C2:
        case BSP_PERIPH_ID_CAN1:
        case BSP_PERIPH_ID_CAN2:
        case BSP_PERIPH_ID_BKP:
        case BSP_PERIPH_ID_PWR:
        case BSP_PERIPH_ID_DAC:
             clk_div = (BSP_PERIPH_REG_RCC_CFGR & 0x00000700) >> 8;
             if (clk_div < 4) {
                 return (clk_freq);
             }
             clk_div   = (clk_div - 3) << 1;
             clk_freq /= clk_div;
             return (clk_freq);
    }

    return ((CPU_INT32U)0);
}


/*
*********************************************************************************************************
*                                             BSP_PeriphEn()
*
* Description : Enable clock for peripheral.
*
* Argument(s) : pwr_clk_id      Power/clock ID.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_PeriphEn (CPU_DATA  pwr_clk_id)
{
    switch (pwr_clk_id) {
        case BSP_PERIPH_ID_DMA1:
        case BSP_PERIPH_ID_DMA2:
        case BSP_PERIPH_ID_SRAM:
        case BSP_PERIPH_ID_FLITF:
        case BSP_PERIPH_ID_CRC:
        case BSP_PERIPH_ID_OTGFS:
        case BSP_PERIPH_ID_ETHMAC:
        case BSP_PERIPH_ID_ETHMACTX:        
             BSP_PERIPH_REG_RCC_AHBENR |= DEF_BIT(pwr_clk_id);
             break;

        case BSP_PERIPH_ID_AFIO:
        case BSP_PERIPH_ID_IOPA:
        case BSP_PERIPH_ID_IOPB:
        case BSP_PERIPH_ID_IOPC:
        case BSP_PERIPH_ID_IOPD:
        case BSP_PERIPH_ID_IOPE:        
        case BSP_PERIPH_ID_ADC1:
        case BSP_PERIPH_ID_ADC2:
        case BSP_PERIPH_ID_TIM1:
        case BSP_PERIPH_ID_SPI1:
        case BSP_PERIPH_ID_USART1:
             BSP_PERIPH_REG_RCC_APB2ENR |= DEF_BIT(pwr_clk_id - 32);
             break;

        case BSP_PERIPH_ID_TIM2:
        case BSP_PERIPH_ID_TIM3:
        case BSP_PERIPH_ID_TIM4:
        case BSP_PERIPH_ID_TIM5:
        case BSP_PERIPH_ID_TIM6:
        case BSP_PERIPH_ID_TIM7:
        case BSP_PERIPH_ID_WWDG:
        case BSP_PERIPH_ID_SPI2:
        case BSP_PERIPH_ID_SPI3:
        case BSP_PERIPH_ID_USART2:
        case BSP_PERIPH_ID_USART3:
        case BSP_PERIPH_ID_USART4:
        case BSP_PERIPH_ID_USART5:
        case BSP_PERIPH_ID_I2C1:
        case BSP_PERIPH_ID_I2C2:
        case BSP_PERIPH_ID_CAN1:
        case BSP_PERIPH_ID_CAN2:        
        case BSP_PERIPH_ID_BKP:
        case BSP_PERIPH_ID_PWR:
        case BSP_PERIPH_ID_DAC:
             BSP_PERIPH_REG_RCC_APB1ENR |= DEF_BIT(pwr_clk_id - 64);
             break;
    }
}


/*
*********************************************************************************************************
*                                             BSP_PeriphDis()
*
* Description : Disable clock for peripheral.
*
* Argument(s) : pwr_clk_id      Power/clock ID.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_PeriphDis (CPU_DATA  pwr_clk_id)
{
    switch (pwr_clk_id) {
        case BSP_PERIPH_ID_DMA1:
        case BSP_PERIPH_ID_DMA2:
        case BSP_PERIPH_ID_SRAM:
        case BSP_PERIPH_ID_FLITF:
        case BSP_PERIPH_ID_CRC:
        case BSP_PERIPH_ID_OTGFS:
        case BSP_PERIPH_ID_ETHMAC:
        case BSP_PERIPH_ID_ETHMACTX:  
             BSP_PERIPH_REG_RCC_AHBENR &= ~DEF_BIT(pwr_clk_id);
             break;

        case BSP_PERIPH_ID_AFIO:
        case BSP_PERIPH_ID_IOPA:
        case BSP_PERIPH_ID_IOPB:
        case BSP_PERIPH_ID_IOPC:
        case BSP_PERIPH_ID_IOPD:
        case BSP_PERIPH_ID_IOPE:
        case BSP_PERIPH_ID_ADC1:
        case BSP_PERIPH_ID_ADC2:
        case BSP_PERIPH_ID_TIM1:
        case BSP_PERIPH_ID_SPI1:
        case BSP_PERIPH_ID_USART1:
             BSP_PERIPH_REG_RCC_APB2ENR &= ~DEF_BIT(pwr_clk_id - 32);
             break;

        case BSP_PERIPH_ID_TIM2:
        case BSP_PERIPH_ID_TIM3:
        case BSP_PERIPH_ID_TIM4:
        case BSP_PERIPH_ID_TIM5:
        case BSP_PERIPH_ID_TIM6:
        case BSP_PERIPH_ID_TIM7:
        case BSP_PERIPH_ID_WWDG:
        case BSP_PERIPH_ID_SPI2:
        case BSP_PERIPH_ID_SPI3:
        case BSP_PERIPH_ID_USART2:
        case BSP_PERIPH_ID_USART3:
        case BSP_PERIPH_ID_USART4:
        case BSP_PERIPH_ID_USART5:
        case BSP_PERIPH_ID_I2C1:
        case BSP_PERIPH_ID_I2C2:
        case BSP_PERIPH_ID_CAN1:
        case BSP_PERIPH_ID_CAN2:        
        case BSP_PERIPH_ID_BKP:
        case BSP_PERIPH_ID_PWR:
        case BSP_PERIPH_ID_DAC:
             BSP_PERIPH_REG_RCC_APB1ENR &= ~DEF_BIT(pwr_clk_id - 64);
             break;
    }
}

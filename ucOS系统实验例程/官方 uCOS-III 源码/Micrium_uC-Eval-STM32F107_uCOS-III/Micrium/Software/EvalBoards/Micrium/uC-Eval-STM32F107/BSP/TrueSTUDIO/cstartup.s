/**
  ******************************************************************************
  * @file      startup_stm32f10x_cl.s
  * @author    MCD Application Team
  * @version   V3.5.0
  * @date      11-March-2011
  * @brief     STM32F10x Connectivity line Devices vector table for Atollic
  *            toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR
  *                  address.
  *                - Configure the clock system
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M3 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGESWITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

  .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

.global	g_pfnVectors
.global	Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word	_sidata
/* start address for the .data section. defined in linker script */
.word	_sdata
/* end address for the .data section. defined in linker script */
.word	_edata
/* start address for the .bss section. defined in linker script */
.word	_sbss
/* end address for the .bss section. defined in linker script */
.word	_ebss

.equ  BootRAM, 0xF1E0F85F
/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section	.text.Reset_Handler
	.weak	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:

/* Copy the data segment initializers from flash to SRAM */
  movs	r1, #0
  b	LoopCopyDataInit

CopyDataInit:
	ldr	r3, =_sidata
	ldr	r3, [r3, r1]
	str	r3, [r0, r1]
	adds	r1, r1, #4

LoopCopyDataInit:
	ldr	r0, =_sdata
	ldr	r3, =_edata
	adds	r2, r0, r1
	cmp	r2, r3
	bcc	CopyDataInit
	ldr	r2, =_sbss
	b	LoopFillZerobss

/* Zero fill the bss segment. */
FillZerobss:
	movs	r3, #0
	str	r3, [r2], #4

LoopFillZerobss:
	ldr	r3, = _ebss
	cmp	r2, r3
	bcc	FillZerobss

/* Call the clock system intitialization function.*/
  	@bl  SystemInit
/* Call static constructors */
    bl __libc_init_array
/* Call the application's entry point.*/
	bl	main
	bx	lr
.size	Reset_Handler, .-Reset_Handler

/******************************************************************************
*
* The minimal vector table for a Cortex M3.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
 	.section	.isr_vector,"a",%progbits
	.type	g_pfnVectors, %object
	.size	g_pfnVectors, .-g_pfnVectors

    .extern  BSP_IntHandlerWWDG
    .extern  BSP_IntHandlerPVD
    .extern  BSP_IntHandlerTAMPER
    .extern  BSP_IntHandlerRTC
    .extern  BSP_IntHandlerFLASH
    .extern  BSP_IntHandlerRCC
    .extern  BSP_IntHandlerEXTI0
    .extern  BSP_IntHandlerEXTI1
    .extern  BSP_IntHandlerEXTI2
    .extern  BSP_IntHandlerEXTI3
    .extern  BSP_IntHandlerEXTI4
    .extern  BSP_IntHandlerDMA1_CH1
    .extern  BSP_IntHandlerDMA1_CH2
    .extern  BSP_IntHandlerDMA1_CH3
    .extern  BSP_IntHandlerDMA1_CH4
    .extern  BSP_IntHandlerDMA1_CH5

    .extern  BSP_IntHandlerDMA1_CH6
    .extern  BSP_IntHandlerDMA1_CH7
    .extern  BSP_IntHandlerADC1_2
    .extern  BSP_IntHandlerCAN1_TX
    .extern  BSP_IntHandlerCAN1_RX0
    .extern  BSP_IntHandlerCAN1_RX1
    .extern  BSP_IntHandlerCAN1_SCE
    .extern  BSP_IntHandlerEXTI9_5
    .extern  BSP_IntHandlerTIM1_BRK
    .extern  BSP_IntHandlerTIM1_UP
    .extern  BSP_IntHandlerTIM1_TRG_COM
    .extern  BSP_IntHandlerTIM1_CC
    .extern  BSP_IntHandlerTIM2
    .extern  BSP_IntHandlerTIM3
    .extern  BSP_IntHandlerTIM4
    .extern  BSP_IntHandlerI2C1_EV
    .extern  BSP_IntHandlerI2C1_ER
    .extern  BSP_IntHandlerI2C2_EV
    .extern  BSP_IntHandlerI2C2_ER
    .extern  BSP_IntHandlerSPI1
    .extern  BSP_IntHandlerSPI2
    .extern  BSP_IntHandlerUSART1
    .extern  BSP_IntHandlerUSART2
    .extern  BSP_IntHandlerUSART3
    .extern  BSP_IntHandlerEXTI15_10
    .extern  BSP_IntHandlerRTCAlarm
    .extern  BSP_IntHandlerUSBWakeUp

    .extern  BSP_IntHandlerTIM5
    .extern  BSP_IntHandlerSPI3
    .extern  BSP_IntHandlerUSART4
    .extern  BSP_IntHandlerUSART5
    .extern  BSP_IntHandlerTIM6
    .extern  BSP_IntHandlerTIM7
    .extern  BSP_IntHandlerDMA2_CH1
    .extern  BSP_IntHandlerDMA2_CH2
    .extern  BSP_IntHandlerDMA2_CH3
    .extern  BSP_IntHandlerDMA2_CH4
    .extern  BSP_IntHandlerDMA2_CH5
    .extern  BSP_IntHandlerETH
    .extern  BSP_IntHandlerETHWakeup
    .extern  BSP_IntHandlerCAN2_TX
    .extern  BSP_IntHandlerCAN2_RX0
    .extern  BSP_IntHandlerCAN2_RX1
    .extern  BSP_IntHandlerCAN2_SCE
    .extern  BSP_IntHandlerOTG

    .extern  OS_CPU_PendSVHandler
    .extern  OS_CPU_SysTickHandler

g_pfnVectors:
	.word	_estack                       @ Top of Stack
	.word	Reset_Handler                 @ Reset Handler
	.word	App_NMI_ISR                   @ NMI Handler
	.word	App_Fault_ISR                 @ Hard Fault Handler
	.word	App_MemFault_ISR              @ MPU Fault Handler
	.word	App_BusFault_ISR              @ Bus Fault Handler
	.word	App_UsageFault_ISR            @ Usage Fault Handler
	.word	App_Spurious_ISR              @ Reserved
	.word	App_Spurious_ISR              @ Reserved
	.word	App_Spurious_ISR              @ Reserved
	.word	App_Spurious_ISR              @ Reserved
	.word	App_Spurious_ISR              @ SVCall Handler
	.word	App_Spurious_ISR              @ Debug Monitor Handler
	.word	App_Spurious_ISR              @ Reserved
	.word	OS_CPU_PendSVHandler          @ PendSV Handler
	.word	OS_CPU_SysTickHandler         @ SysTick Handler

                                                      @ External Interrupts
    .word   BSP_IntHandlerWWDG            @ 16, INTISR[  0]  Window Watchdog.
    .word   BSP_IntHandlerPVD             @ 17, INTISR[  1]  PVD through EXTI Line Detection.
    .word   BSP_IntHandlerTAMPER          @ 18, INTISR[  2]  Tamper Interrupt.
    .word   BSP_IntHandlerRTC             @ 19, INTISR[  3]  RTC Global Interrupt.
    .word   BSP_IntHandlerFLASH           @ 20, INTISR[  4]  FLASH Global Interrupt.
    .word   BSP_IntHandlerRCC             @ 21, INTISR[  5]  RCC Global Interrupt.
    .word   BSP_IntHandlerEXTI0           @ 22, INTISR[  6]  EXTI Line0 Interrupt.
    .word   BSP_IntHandlerEXTI1           @ 23, INTISR[  7]  EXTI Line1 Interrupt.
    .word   BSP_IntHandlerEXTI2           @ 24, INTISR[  8]  EXTI Line2 Interrupt.
    .word   BSP_IntHandlerEXTI3           @ 25, INTISR[  9]  EXTI Line3 Interrupt.
    .word   BSP_IntHandlerEXTI4           @ 26, INTISR[ 10]  EXTI Line4 Interrupt.
    .word   BSP_IntHandlerDMA1_CH1        @ 27, INTISR[ 11]  DMA Channel1 Global Interrupt.
    .word   BSP_IntHandlerDMA1_CH2        @ 28, INTISR[ 12]  DMA Channel2 Global Interrupt.
    .word   BSP_IntHandlerDMA1_CH3        @ 29, INTISR[ 13]  DMA Channel3 Global Interrupt.
    .word   BSP_IntHandlerDMA1_CH4        @ 30, INTISR[ 14]  DMA Channel4 Global Interrupt.
    .word   BSP_IntHandlerDMA1_CH5        @ 31, INTISR[ 15]  DMA Channel5 Global Interrupt.

    .word   BSP_IntHandlerDMA1_CH6        @ 32, INTISR[ 16]  DMA Channel6 Global Interrupt.
    .word   BSP_IntHandlerDMA1_CH7        @ 33, INTISR[ 17]  DMA Channel7 Global Interrupt.
    .word   BSP_IntHandlerADC1_2          @ 34, INTISR[ 18]  ADC1 & ADC2 Global Interrupt.
    .word   BSP_IntHandlerCAN1_TX         @ 35, INTISR[ 19]  USB High Prio / CAN TX  Interrupts.
    .word   BSP_IntHandlerCAN1_RX0        @ 36, INTISR[ 20]  USB Low  Prio / CAN RX0 Interrupts.
    .word   BSP_IntHandlerCAN1_RX1        @ 37, INTISR[ 21]  CAN RX1 Interrupt.
    .word   BSP_IntHandlerCAN1_SCE        @ 38, INTISR[ 22]  CAN SCE Interrupt.
    .word   BSP_IntHandlerEXTI9_5         @ 39, INTISR[ 23]  EXTI Line[9:5] Interrupt.
    .word   BSP_IntHandlerTIM1_BRK        @ 40, INTISR[ 24]  TIM1 Break  Interrupt.
    .word   BSP_IntHandlerTIM1_UP         @ 41, INTISR[ 25]  TIM1 Update Interrupt.
    .word   BSP_IntHandlerTIM1_TRG_COM    @ 42, INTISR[ 26]  TIM1 Trig & Commutation Interrupts.
    .word   BSP_IntHandlerTIM1_CC         @ 43, INTISR[ 27]  TIM1 Capture Compare Interrupt.
    .word   BSP_IntHandlerTIM2            @ 44, INTISR[ 28]  TIM2 Global Interrupt.
    .word   BSP_IntHandlerTIM3            @ 45, INTISR[ 29]  TIM3 Global Interrupt.
    .word   BSP_IntHandlerTIM4            @ 46, INTISR[ 30]  TIM4 Global Interrupt.
    .word   BSP_IntHandlerI2C1_EV         @ 47, INTISR[ 31]  I2C1 Event  Interrupt.
    .word   BSP_IntHandlerI2C1_ER         @ 48, INTISR[ 32]  I2C1 Error  Interrupt.
    .word   BSP_IntHandlerI2C2_EV         @ 49, INTISR[ 33]  I2C2 Event  Interrupt.
    .word   BSP_IntHandlerI2C2_ER         @ 50, INTISR[ 34]  I2C2 Error  Interrupt.
    .word   BSP_IntHandlerSPI1            @ 51, INTISR[ 35]  SPI1 Global Interrupt.
    .word   BSP_IntHandlerSPI2            @ 52, INTISR[ 36]  SPI2 Global Interrupt.
    .word   BSP_IntHandlerUSART1          @ 53, INTISR[ 37]  USART1 Global Interrupt.
    .word   BSP_IntHandlerUSART2          @ 54, INTISR[ 38]  USART2 Global Interrupt.
    .word   BSP_IntHandlerUSART3          @ 55, INTISR[ 39]  USART3 Global Interrupt.
    .word   BSP_IntHandlerEXTI15_10       @ 56, INTISR[ 40]  EXTI Line [15:10] Interrupts.
    .word   BSP_IntHandlerRTCAlarm        @ 57, INTISR[ 41]  RTC Alarm EXT Line Interrupt.
    .word   BSP_IntHandlerUSBWakeUp       @ 58, INTISR[ 42]  USB Wakeup from Suspend EXTI Int.

    .word   App_Reserved_ISR              @ 59, INTISR[ 43]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 60, INTISR[ 44]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 61, INTISR[ 45]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 62, INTISR[ 46]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 63, INTISR[ 47]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 64, INTISR[ 48]  USB Wakeup from Suspend EXTI Int.
    .word   App_Reserved_ISR              @ 65, INTISR[ 49]  USB Wakeup from Suspend EXTI Int.

    .word   BSP_IntHandlerTIM5            @ 66, INTISR[ 50]  TIM5 global Interrupt.
    .word   BSP_IntHandlerSPI3            @ 67, INTISR[ 51]  SPI3 global Interrupt.
    .word   BSP_IntHandlerUSART4          @ 68, INTISR[ 52]  UART4 global Interrupt.
    .word   BSP_IntHandlerUSART5          @ 69, INTISR[ 53]  UART5 global Interrupt.
    .word   BSP_IntHandlerTIM6            @ 70, INTISR[ 54]  TIM6 global Interrupt.
    .word   BSP_IntHandlerTIM7            @ 71, INTISR[ 55]  TIM7 global Interrupt.
    .word   BSP_IntHandlerDMA2_CH1        @ 72, INTISR[ 56]  DMA2 Channel 1 global Interrupt.
    .word   BSP_IntHandlerDMA2_CH2        @ 73, INTISR[ 57]  DMA2 Channel 2 global Interrupt.
    .word   BSP_IntHandlerDMA2_CH3        @ 74, INTISR[ 58]  DMA2 Channel 3 global Interrupt.
    .word   BSP_IntHandlerDMA2_CH4        @ 75, INTISR[ 59]  DMA2 Channel 4 global Interrupt.
    .word   BSP_IntHandlerDMA2_CH5        @ 76, INTISR[ 60]  DMA2 Channel 5 global Interrupt.
    .word   BSP_IntHandlerETH             @ 77, INTISR[ 61]  ETH global Interrupt.
    .word   BSP_IntHandlerETHWakeup       @ 78, INTISR[ 62]  ETH WakeUp from EXTI line Int.
    .word   BSP_IntHandlerCAN2_TX         @ 79, INTISR[ 63]  CAN2 TX Interrupts.
    .word   BSP_IntHandlerCAN2_RX0        @ 80, INTISR[ 64]  CAN2 RX0 Interrupts.
    .word   BSP_IntHandlerCAN2_RX1        @ 81, INTISR[ 65]  CAN2 RX1 Interrupt.
    .word   BSP_IntHandlerCAN2_SCE        @ 82, INTISR[ 66]  CAN2 SCE Interrupt.
    .word   BSP_IntHandlerOTG             @ 83, INTISR[ 67]  OTG global Interrupt.
/*
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word BootRAM      @0x1E0. This is for boot in RAM mode for
                         STM32F10x Connectivity line Devices. */

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/

    .section	.text.App_NMI_ISR
App_NMI_ISR:
	b	App_NMI_ISR
	.size	App_NMI_ISR, .-App_NMI_ISR


    .section	.text.App_Fault_ISR
App_Fault_ISR:
	b	App_Fault_ISR
	.size	App_Fault_ISR, .-App_Fault_ISR


    .section	.text.App_MemFault_ISR
App_MemFault_ISR:
	b	App_MemFault_ISR
	.size	App_MemFault_ISR, .-App_MemFault_ISR


    .section	.text.App_BusFault_ISR
App_BusFault_ISR:
	b	App_BusFault_ISR
	.size	App_BusFault_ISR, .-App_BusFault_ISR


    .section	.text.App_UsageFault_ISR
App_UsageFault_ISR:
	b	App_UsageFault_ISR
	.size	App_UsageFault_ISR, .-App_UsageFault_ISR

    .section	.text.App_Spurious_ISR
App_Spurious_ISR:
	b	App_Spurious_ISR
	.size	App_Spurious_ISR, .-App_Spurious_ISR


    .section	.text.App_Reserved_ISR
App_Reserved_ISR:
	b	App_Reserved_ISR
	.size	App_Reserved_ISR, .-App_Reserved_ISR

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

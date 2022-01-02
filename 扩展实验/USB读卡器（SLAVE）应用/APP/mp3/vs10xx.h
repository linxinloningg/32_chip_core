#ifndef __VS10XX_H
#define __VS10XX_H

#include "system.h"

#define VS_WRITE_COMMAND 	0x02
#define VS_READ_COMMAND 	0x03

/* VS10XX寄存器定义 */
#define SCI_MODE        	0x00   
#define SCI_STATUS      	0x01   
#define SCI_BASS        	0x02   
#define SCI_CLOCKF      	0x03   
#define SCI_DECODE_TIME 	0x04   
#define SCI_AUDATA      	0x05   
#define SCI_WRAM        	0x06   
#define SCI_WRAMADDR    	0x07   
#define SCI_HDAT0       	0x08   
#define SCI_HDAT1       	0x09 
  
#define SCI_AIADDR      	0x0a   
#define SCI_VOL         	0x0b   
#define SCI_AICTRL0     	0x0c   
#define SCI_AICTRL1     	0x0d   
#define SCI_AICTRL2     	0x0e   
#define SCI_AICTRL3     	0x0f   
		 
/* VS_RST */
#define VS_RST_SET GPIO_SetBits(GPIOG, GPIO_Pin_8)
#define VS_RST_CLR GPIO_ResetBits(GPIOG, GPIO_Pin_8)

/* VS_XDCS */
#define VS_XDCS_SET GPIO_SetBits(GPIOG, GPIO_Pin_6)
#define VS_XDCS_CLR GPIO_ResetBits(GPIOG, GPIO_Pin_6)

/* VS_CS */
#define VS_XCS_SET GPIO_SetBits(GPIOE, GPIO_Pin_6)
#define VS_XCS_CLR {GPIO_ResetBits(GPIOE, GPIO_Pin_6); GPIO_SetBits(GPIOD, GPIO_Pin_6);}

/* VS_DREQ */
#define VS_DREQ GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_7)

/* 定义全局变量 */
void VS10XX_Config(void);

int8_t VS10XX_WriteCmd(uint8_t addr, uint16_t cmd);
uint16_t VS10XX_ReadData(uint8_t addr);

int8_t VS10XX_HardwareReset(void);
int8_t VS10XX_SoftReset(void);
uint16_t VS10XX_RAM_Test(void);
void VS10XX_SineTest(void);
uint8_t VS10XX_SendMusicData(uint8_t *dat);


#endif

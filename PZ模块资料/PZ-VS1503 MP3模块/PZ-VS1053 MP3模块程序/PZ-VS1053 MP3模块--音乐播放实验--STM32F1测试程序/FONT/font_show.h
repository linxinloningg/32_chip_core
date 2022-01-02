#ifndef _font_show_H
#define _font_show_H


#include "system.h"


/* 字库文件地址 */
#define GUI_ASCII_FILE  "系统/FONT/ASCII（8X16）.DZK"
#define GUI_12CHAR_FILE "系统/FONT/12号字体（16X16）.DZK"
#define GUI_16CHAR_FILE "系统/FONT/16号字体（24X21）.DZK"


/* 设置字库地址 */
#define GUI_FLASH_ASCII_ADDR     6112846  //ASCII字库首地址（6114304 - 1456 - 2）
#define GUI_FLASH_12CHAR_ADDR    6114304  //12号字库首地址（6880386 - 766080 - 2）
#define GUI_FLASH_16CHAR_ADDR    6880386  //16号字库首地址（8388608 - 1508220 - 2）

/* 更新字库选择项 */
#define GUI_UPDATE_ASCII         0x01     
#define GUI_UPDATE_12CHAR        0x02
#define GUI_UPDATE_16CHAR        0x04
#define GUI_UPDATE_ALL           0x07


void FontUpdate(uint8_t updateState);
void LCD_ShowFont12Char(uint16_t x, uint16_t y, uint8_t *ch);
void LCD_ShowFont16Char(uint16_t x, uint16_t y, uint8_t *ch);




#endif

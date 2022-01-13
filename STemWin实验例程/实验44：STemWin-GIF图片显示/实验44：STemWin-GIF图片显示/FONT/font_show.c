#include "font_show.h"
#include "ff.h" 
#include "flash.h"
#include "tftlcd.h" 
#include "malloc.h"




void FontUpdate(uint8_t updateState)
{
    FRESULT res;
	FIL fsrc;
	UINT  br;
    uint32_t wordAddr, i, j;

#ifdef _malloc_H	
    uint8_t *p;
    p = mymalloc(0,4096);                  //开辟一个内存块
    if(p == 0)
    {
        return;
    }
#else
	uint8_t buffer[512];
#endif    	

    /* 更新ASCII字库 */
    if((updateState & GUI_UPDATE_ASCII) == GUI_UPDATE_ASCII)
    {
        /* 设置写入起始地址 */
        wordAddr = GUI_FLASH_ASCII_ADDR;
        j = 0;

        /* 打开读取文件 */
        res = f_open(&fsrc, GUI_ASCII_FILE, FA_READ);	
    	if(res == FR_OK)  //打开成功
        { 
         	for (;;)      //开始读取数据
         	{       
#ifdef _malloc_H	
                res = f_read(&fsrc, p, 4096, &br);
    
                /* 将读取到的数据写入FLASH */
                EN25QXX_Write(p, wordAddr, br);
          	    wordAddr += br;   //写入地址增加

#else
                res = f_read(&fsrc, buffer, sizeof(buffer), &br);
    
                /* 将读取到的数据写入FLASH */
                EN25QXX_Write(buffer, wordAddr, br);
          	    wordAddr += br;   //写入地址增加
#endif
                j += br;
                i = j * 100 / 1456;
                LCD_Fill(0, 80, i, 90, RED);    

                if (res || br == 0)
    			{
    				break;    // error or eof 
    			}
            } 
        }
    	 
        f_close(&fsrc);  //不论是打开，还是新建文件，一定记得关闭
    }

    /* 更新12号汉字库 */
    if((updateState & GUI_UPDATE_12CHAR) == GUI_UPDATE_12CHAR)
    {
        wordAddr = GUI_FLASH_12CHAR_ADDR;
        j = 0;

        res = f_open(&fsrc, GUI_12CHAR_FILE, FA_READ);
    	
    	if(res == FR_OK) 
        {  
         	for (;;)  
         	{      
#ifdef _malloc_H	
                res = f_read(&fsrc, p, 4096, &br);
    
                /* 将读取到的数据写入FLASH */
                EN25QXX_Write(p, wordAddr, br);
          	    wordAddr += br;   //写入地址增加
#else
                res = f_read(&fsrc, buffer, sizeof(buffer), &br);
    
                EN25QXX_Write(buffer, wordAddr, br);
          	    wordAddr += br;
#endif
                j += br;
                i = j * 100 / 766080;
                LCD_Fill(0, 95, i, 105, RED);    

    
                if (res || br == 0)
    			{
    				break;    // error or eof 
    			}
            } 
        }
        f_close(&fsrc);  //不论是打开，还是新建文件，一定记得关闭
    }
    
    /* 更新16号汉字库 */
    if((updateState & GUI_UPDATE_16CHAR) == GUI_UPDATE_16CHAR)
    {
        
        wordAddr = GUI_FLASH_16CHAR_ADDR;
        j = 0;

        res = f_open(&fsrc, GUI_16CHAR_FILE, FA_READ);	
    	if(res == FR_OK) 
        { 
         	for (;;)  
         	{       
#ifdef _malloc_H	
                res = f_read(&fsrc, p, 4096, &br);
    
                /* 将读取到的数据写入FLASH */
                EN25QXX_Write(p, wordAddr, br);
          	    wordAddr += br;   //写入地址增加
#else
                res = f_read(&fsrc, buffer, sizeof(buffer), &br);
    
                EN25QXX_Write(buffer, wordAddr, br);
          	    wordAddr += br;
#endif
                j += br;
                i = j * 100 / 1508220;
                LCD_Fill(0, 110, i, 120, RED);    

    
                if (res || br == 0)
    			{
    				break;    // error or eof 
    			}
            } 
        }
    	 
        f_close(&fsrc);  //不论是打开，还是新建文件，一定记得关闭
    }
#ifdef _malloc_H	
    myfree(0,p);
#endif
} 


void LCD_ShowFont12Char(uint16_t x, uint16_t y, uint8_t *ch)
{
    uint8_t i, j, color, buf[32];
    uint16_t asc;
    uint32_t wordAddr = 0;

    while(*ch != '\0')
    {
        /*显示字母，ASCII编码 */
        if(*ch < 0x80)  //ASCII码从0~127
        {
            /* 在字库中的ASCII码是从空格开始的也就是32开始的，所以减去32 */
    		wordAddr = *ch - 32;
            wordAddr *= 16;
            wordAddr += GUI_FLASH_ASCII_ADDR;
            
            /* 读取FLASH中该字的字模 */
            EN25QXX_Read(buf, wordAddr, 16);
            
            /* 显示该文字 */		
            LCD_Set_Window(x, y, x+7, y+15);           //字宽*高为：8*16
    		for (j=0; j<16; j++) //每个字模一共有16个字节
    		{
    			color = buf[j];
    			for (i=0; i<8; i++) 
    			{
    				if ((color&0x80) == 0x80)
    				{
    					LCD_WriteData_Color(FRONT_COLOR);
    				} 						
    				else
    				{
    					LCD_WriteData_Color(BACK_COLOR);
    				} 	
    				color <<= 1;
    			}
    		}
    
    		ch++;    //指针指向下一个字
    		
            /* 屏幕坐标处理 */
            x += 8;
            if(x > tftlcd_data.width -8)   //TFT_XMAX -8
            {
                x = 0;
                y += 16;    
            }            
        }
        /* 显示汉字，GBK编码 */
        else
        {
            /* 将汉字编码转换成在FLASH中的地址 */
            asc = *ch - 0x81;     //高字节是表示分区，分区是从0x81到0xFE,所以转换成地址-0x80
            wordAddr = asc * 190; //每个分区一共有190个字
    
            asc = *(ch + 1); //低字节代表每个字在每个分区的位置，它是从0x40到0xFF
            if(asc < 0x7F)   //在0x7F位置有个空位，但是我们取模不留空，所以大于0x7F之后多减一
            {
                asc -= 0x40;
            }
            else
            {
                asc -= 0x41;
            }
            
            wordAddr += asc; //求出在GBK中是第几个字
            wordAddr *= 32;  //将字位置转换位FLASH地址
            wordAddr += GUI_FLASH_12CHAR_ADDR; //加上首地址
    
            /* 读取FLASH中该字的字模 */
            EN25QXX_Read(buf, wordAddr, 32);
    
            /* 在彩屏上面显示 */
            LCD_Set_Window(x, y, x+15, y+15);
            for(i=0; i<32; i++)
            {
                 
                color = buf[i];            
                for(j=0; j<8; j++) 
        		{
        			if((color & 0x80) == 0x80)
        			{
        				LCD_WriteData_Color(FRONT_COLOR);
        			} 						
        			else
        			{
        				LCD_WriteData_Color(BACK_COLOR);
        			} 
        			color <<= 1;
        		}//for(j=0;j<8;j++)结束
            }
    
            /* 屏幕坐标处理 */
            x += 16;
            if(x > tftlcd_data.width -15)   //TFT_XMAX -15
            {
                x = 0;
                y += 16;    
            }
    
            /* 写下一个字，每个汉字占两个字节所以+2 */
            ch += 2;             
        }
    }    
}


void LCD_ShowFont16Char(uint16_t x, uint16_t y, uint8_t *cn )
{   
    uint8_t i, j, color, buf[63];
    uint16_t asc;
    uint32_t wordAddr = 0;    
    while(*cn != '\0')
    {  
        /* 将汉字编码转换成在FLASH中的地址 */
        asc = *cn - 0x81;     //高字节是表示分区，分区是从0x81到0xFE,所以转换成地址-0x80
        wordAddr = asc * 190; //每个分区一共有190个字

        asc = *(cn + 1); //低字节代表每个字在每个分区的位置，它是从0x40到0xFF
        if(asc < 0x7F)   //在0x7F位置有个空位，但是我们取模不留空，所以大于0x7F之后多减一
        {
            asc -= 0x40;
        }
        else
        {
            asc -= 0x41;
        }
        
        wordAddr += asc; //求出在GBK中是第几个字
        wordAddr *= 63;  //将字位置转换位FLASH地址
        wordAddr += GUI_FLASH_16CHAR_ADDR; //加上首地址

        /* 读取FLASH中该字的字模 */
        EN25QXX_Read(buf, wordAddr, 63);

        /* 在彩屏上面显示 */
        LCD_Set_Window(x, y, x+23, y+20);
        for(i=0; i<63; i++)
        {
            color = buf[i];            
            for(j=0; j<8; j++) 
    		{
    			if((color & 0x80) == 0x80)
    			{
    				LCD_WriteData_Color(FRONT_COLOR);
    			} 						
    			else
    			{
    				LCD_WriteData_Color(BACK_COLOR);
    			} 
    			color <<= 1;
    		}//for(j=0;j<8;j++)结束
        }

        /* 屏幕坐标处理 */
        x += 21;
        if(x > tftlcd_data.width -21)   //TFT_XMAX -21
        {
            x = 0;
            y += 21;    
        }

        /* 写下一个字，每个汉字占两个字节所以+2 */
        cn += 2;      
    }    
}

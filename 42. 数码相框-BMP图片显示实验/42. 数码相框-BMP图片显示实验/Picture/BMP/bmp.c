#include "bmp.h"
#include "ff.h"
#include "tftlcd.h"


/****************************************************************************
* Function Name  : BMP_ReadHeader
* Description    : 将读取到的数组函数转换位BPM文件信息结构体类型。由于在内存
*                * 上面数组的存储方式与结构体不同，所以要转换，而且SD读取到的
*                * 文件信息是小端模式。高位是低字节，低位是高字节，跟我们常用
*                * 的正好相反所以将数据转换过来。
* Input          : header：要转换的数组
*                * bmp：转换成的结构体
* Output         : None
* Return         : None
****************************************************************************/

void BMP_ReadHeader(uint8_t *header, BMP_HeaderTypeDef *bmp)
{

	bmp->fileHeader.bfType = ((*header) << 8) | (*(header + 1));
	header += 2;
	
	bmp->fileHeader.bfSize = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                         ((*(header + 1)) << 8) | (*header);
	header += 8;

	bmp->fileHeader.bfOffBits = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                            ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.bitSize = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                          ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biWidth = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                          ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biHeight = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                           ((*(header + 1)) << 8) | (*header);
	header += 6;

	bmp->infoHeader.biBitCount = ((*(header + 1)) << 8) | (*header);
	                         
	header += 2;

	bmp->infoHeader.biCompression = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                                ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biSizeImage = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                              ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biXPelsPerMeter = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                                  ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biYPelsPerMeter = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
	                                  ((*(header + 1)) << 8) | (*header);
}


/****************************************************************************
* Function Name  : BMP_ShowPicture
* Description    : 显示BMP格式的图片
* Input          : dir：要显示的图片路径和名字
* Output         : None
* Return         : None
****************************************************************************/

uint8_t buffer[1024];

void BMP_ShowPicture(uint8_t *dir)
{
	FRESULT res;
	FIL fsrc;
	UINT  br;

	uint8_t rgb;
	BMP_HeaderTypeDef bmpHeader;

	uint16_t a, x, y, color, width, xCount;
	float xRatio, yRatio;

	/* 将屏幕刷黑色 */
	LCD_Clear(BLACK);
    
	/* 打开要读取的文件 */
	res = f_open(&fsrc, (const TCHAR*)dir, FA_READ);
		
	if(res == FR_OK)   //打开成功
    { 
		/* 读取BMP文件的文件信息 */
        res = f_read(&fsrc, buffer, sizeof(buffer), &br);
		
		/* 将数组里面的数据放入到结构数组中，并排序好 */
		BMP_ReadHeader(buffer, &bmpHeader);

		/* 判断图片的大小超过TFT的大小的话，进行缩小 */
		if(((tftlcd_data.width + 1) < bmpHeader.infoHeader.biWidth) ||
		   ((tftlcd_data.height + 1) < bmpHeader.infoHeader.biHeight))
		{
			/* 求出缩小比例 */
			xRatio = (float)(tftlcd_data.width + 1) / (float)bmpHeader.infoHeader.biWidth;
			yRatio = (float)(tftlcd_data.height + 1) / (float)bmpHeader.infoHeader.biHeight;
	
	        /* 如果只有一边超出TFT屏的大小的话，不超出部分不进行缩小 */
			if(xRatio > 1)
			{
				xRatio = 1;
			}
			if(yRatio > 1)
			{
				yRatio = 1;
			}
		}
		else //如果图片大小小于TFT屏
		{
			xRatio = 1;
			yRatio = 1;
		}

		/* BMP图片的宽像素数据必须是4的倍数，如果不是那么将其补齐 */
		/* 所以这里是判断BMP图片的横坐标跳转数据。 */
		if(bmpHeader.infoHeader.biWidth % 4)
		{
			width = bmpHeader.infoHeader.biWidth * 3 / 4;
			width += 1;
			width *= 4;	
		}		
		else
		{
			width = bmpHeader.infoHeader.biWidth * 3;	
		}

		/* 初始化应用的值 */
		x = 0; 
		y = 0;
		rgb = 0;
		xCount = 0;
		a = bmpHeader.fileHeader.bfOffBits;    //去掉文件信息才开始是像素数据
     	while(1)  
     	{      
			/* SD卡读取一次数据的长度 */
      	  	while(a < 1024)
			{				
				/* 将读取到的24位色转换为16位色 */
				switch (rgb) 
				{
					case 0:				  
						color = buffer[a] >> 3; //B
						break ;	   
					case 1: 	 
						color += ((uint16_t)buffer[a] << 3) & 0X07E0;//G
						break;	  
					case 2 : 
						color += ((uint16_t)buffer[a] << 8) & 0XF800;//R	  
						break ;
                    default:
                        break;			
				}
				a++;
				rgb++;

				/* 如果读取完一个像素点，就写到TFT屏上面 */
				if(rgb == 3)
				{
				    /* 设置要写入的点 */
					LCD_Set_Window(((float)x * xRatio + 0.5), ((float)y * yRatio + 0.5), 
					              ((float)x * xRatio + 0.5), ((float)y * yRatio + 0.5));
					LCD_WriteData_Color(color);

					rgb =0;
					x++;       //X坐标+1					
				}

				/* 计数一共读取了多少像素值 */
				xCount++;
				if(xCount >= width)	   //如果等于一行的像素了，那么换行显示
				{	
					xCount = 0;
					x = 0;
					y++;
				}				
			}

			/* 继续读取图片数据 */
			res = f_read(&fsrc, buffer, sizeof(buffer), &br);
			a = 0;

			/* 判断手否读取完结，若完结跳出循环 */
            if (res || br < sizeof(buffer))
			{
				break;    // error or eof 
			}

        } 
    }
	 
    f_close(&fsrc);  //不论是打开，还是新建文件，一定记得关闭
}




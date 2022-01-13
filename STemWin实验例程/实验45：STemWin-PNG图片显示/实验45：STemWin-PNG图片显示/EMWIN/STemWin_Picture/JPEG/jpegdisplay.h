#ifndef _JPEGDISPLAY_H
#define _JPEGDISPLAY_H
#include "system.h"



//使用GUI_JPEG_Draw()函数绘制BMP图片的话
//图片是加载到RAM中的，因此不能大于JEGPMEMORYSIZE
//注意：显示BMP图片时内存申请使用的SRAM的内存申请函数，因此
//JPEGMEMORYSIZE不能大于我们给SRAM空间-EMWIN分配的内存池大小
#define JPEGMEMORYSIZE	500*1024	//图片大小不大于500kb

//绘制无需加载到RAM中的BMP图片时，图片每行的字节数
#define JPEGPERLINESIZE	2*1024		

int displyjpeg(u8 *JPEGFileName,u8 mode,u32 x,u32 y,int member,int denom);
int displayjpegex(u8 *JPEGFileName,u8 mode,u32 x,u32 y,int member,int denom);
void jpegdisplay_demo(void);
#endif

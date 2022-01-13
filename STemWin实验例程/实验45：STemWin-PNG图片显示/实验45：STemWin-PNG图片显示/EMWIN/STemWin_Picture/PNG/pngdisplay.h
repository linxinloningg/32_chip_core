#ifndef _PNGDISPLAY_H
#define _PNGDISPLAY_H
#include "system.h"



//使用GUI_PNG_Draw()函数绘制PNG图片的话
//图片是加载到RAM中的，因此不能大于PNGMEMORYSIZE
//注意：显示PNG图片时内存申请使用的SRAM的内存申请函数，因此
//PNGMEMORYSIZE不能大于我们给SRAM分配的内存空间-EMWIN分配的内存池大小
#define PNGMEMORYSIZE	300*1024	//图片大小不大于300kb

//绘制无需加载到RAM中的PNG图片时，图片每行的字节数
#define PNGPERLINESIZE	5*1024	

int displaypng(char *PNGFileName,u8 mode,u32 x,u32 y);
int displaypngex(char *PNGFileName,u8 mode,u32 x,u32 y);
void pngdisplay_demo(void);
#endif

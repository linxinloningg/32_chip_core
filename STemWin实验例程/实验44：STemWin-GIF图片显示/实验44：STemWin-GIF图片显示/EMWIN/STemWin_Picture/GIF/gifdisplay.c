#include "gifdisplay.h"
#include "EmWinHZFont.h"
#include "GUI.h"
#include "malloc.h"
#include "ff.h"
#include "tftlcd.h"
#include "includes.h"					


static FIL GIFFile;
static char gifBuffer[GIFPERLINESIZE];
/*******************************************************************
*
*       Static functions
*
********************************************************************
*/
/*********************************************************************
*
*       GifGetData
*
* Function description
*   This routine is called by GUI_GIF_DrawEx(). The routine is responsible
*   for setting the data pointer to a valid data location with at least
*   one valid byte.
*
* Parameters:
*   p           - Pointer to application defined data.
*   NumBytesReq - Number of bytes requested.
*   ppData      - Pointer to data pointer. This pointer should be set to
*                 a valid location.
*   StartOfFile - If this flag is 1, the data pointer should be set to the
*                 beginning of the data stream.
*
* Return value:
*   Number of data bytes available.
*/
static int GifGetData(void * p, const U8 ** ppData, unsigned NumBytesReq, U32 Off) 
{
	static int readaddress=0;
	FIL * phFile;
	UINT NumBytesRead;

	CPU_SR_ALLOC();
	
	phFile = (FIL *)p;
	
	if (NumBytesReq > sizeof(gifBuffer)) 
	{
		NumBytesReq = sizeof(gifBuffer);
	}

	//移动指针到应该读取的位置
	if(Off == 1) readaddress = 0;
	else readaddress=Off;
	
	OS_CRITICAL_ENTER();	//临界区
	f_lseek(phFile,readaddress); 
	
	//读取数据到缓冲区中
	f_read(phFile,gifBuffer,NumBytesReq,&NumBytesRead);
	
		
	OS_CRITICAL_EXIT();	//退出临界区
	
	*ppData = (U8 *)gifBuffer;
	return NumBytesRead;//返回读取到的字节数
}

//在指定位置显示加载到RAM中的GIF图片
//GIFFileName:图片在SD卡或者其他存储设备中的路径(需文件系统支持！)
//mode:显示模式
//		0 在指定位置显示，有参数x,y确定显示位置
//		1 在LCD中间显示图片，当选择此模式的时候参数x,y无效。
//x:图片左上角在LCD中的x轴位置(当参数mode为1时，此参数无效)
//y:图片左上角在LCD中的y轴位置(当参数mode为1时，此参数无效)
//member:  缩放比例的分子项
//denom:缩放比例的分母项
//返回值:0 显示正常,其他 失败
int displaygif(char *GIFFileName,u8 mode,u32 x,u32 y,int member,int denom)
{
	int i;
	u16 bread;
	char *gifbuffer;
	char result;
	int XSize,YSize;
	GUI_GIF_INFO GifInfo;
	GUI_GIF_IMAGE_INFO ImageInfo;
	float Xflag,Yflag;
	
	CPU_SR_ALLOC();

	result = f_open(&GIFFile,(const TCHAR*)GIFFileName,FA_READ);	//打开文件
	//文件打开错误或者文件大于JPEGMEMORYSIZE
	if((result != FR_OK) || (GIFFile.fsize>GIFMEMORYSIZE)) 	return 1;
	
	gifbuffer=mymalloc(SRAMEX,GIFFile.fsize);
	if(gifbuffer == NULL) return 2;
	
	OS_CRITICAL_ENTER();	//临界区
		
	result = f_read(&GIFFile,gifbuffer,GIFFile.fsize,(UINT *)&bread); //读取数据
	if(result != FR_OK) return 3;
	
	OS_CRITICAL_EXIT();//退出临界区
	
	GUI_GIF_GetInfo(gifbuffer,GIFFile.fsize,&GifInfo);	//获取GIF图片信息
	
	XSize = GifInfo.xSize;	//获取GIF图片的X轴大小
	YSize = GifInfo.ySize;	//获取GIF图片的Y轴大小
	switch(mode)
	{
		case 0:	//在指定位置显示图片
			if((member == 1) && (denom == 1)) //无需缩放，直接绘制
			{
				//在指定位置显示JPEG图片
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSub(gifbuffer,GIFFile.fsize,x,y,i);
					GUI_GIF_GetImageInfo(gifbuffer,GIFFile.fsize,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}else //否则图片需要缩放
			{
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubScaled(gifbuffer,GIFFile.fsize,x,y,i,member,denom);
					GUI_GIF_GetImageInfo(gifbuffer,GIFFile.fsize,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}
			break;
		case 1:	//在LCD中间显示图片
			if((member == 1) && (denom == 1)) //无需缩放，直接绘制
			{
				//在LCD中间显示图片
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSub(gifbuffer,GIFFile.fsize,(tftlcd_data.width-XSize)/2-1,(tftlcd_data.height-YSize)/2-1,i);
					GUI_GIF_GetImageInfo(gifbuffer,GIFFile.fsize,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}else //否则图片需要缩放
			{
				Xflag = (float)XSize*((float)member/(float)denom);
				Yflag = (float)YSize*((float)member/(float)denom);
				XSize = (tftlcd_data.width-(int)Xflag)/2-1;
				YSize = (tftlcd_data.height-(int)Yflag)/2-1;
				
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubScaled(gifbuffer,GIFFile.fsize,XSize,YSize,i,member,denom);
					GUI_GIF_GetImageInfo(gifbuffer,GIFFile.fsize,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}
			break;
	}
	f_close(&GIFFile);		//关闭JPEGFile文件
	myfree(SRAMEX,gifbuffer);	//释放内存
	return 0;
}

//在指定位置显示无需加载到RAM中的GIF图片(需文件系统支持！对于小RAM，推荐使用此方法！)
//GIFFileName:图片在SD卡或者其他存储设备中的路径
//mode:显示模式
//		0 在指定位置显示，有参数x,y确定显示位置
//		1 在LCD中间显示图片，当选择此模式的时候参数x,y无效。
//x:图片左上角在LCD中的x轴位置(当参数mode为1时，此参数无效)
//y:图片左上角在LCD中的y轴位置(当参数mode为1时，此参数无效)
//member:  缩放比例的分子项
//denom:缩放比例的分母项
//返回值:0 显示正常,其他 失败
int displaygifex(char *GIFFileName,u8 mode,u32 x,u32 y,int member,int denom)
{
	char result;
	int i;
	int XSize,YSize;
	float Xflag,Yflag;
	GUI_GIF_INFO GifInfo;
	GUI_GIF_IMAGE_INFO ImageInfo;

	result = f_open(&GIFFile,(const TCHAR*)GIFFileName,FA_READ);	//打开文件
	//文件打开错误
	if(result != FR_OK) 	return 1;
		
	GUI_GIF_GetInfoEx(GifGetData,&GIFFile,&GifInfo);
	XSize = GifInfo.xSize;	//GIF图片X大小
	YSize = GifInfo.ySize;	//GIF图片Y大小
	switch(mode)
	{
		case 0:	//在指定位置显示图片
			if((member == 1) && (denom == 1)) //无需缩放，直接绘制
			{
				//在指定位置显示BMP图片
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubEx(GifGetData,&GIFFile,x,y,i);
					GUI_GIF_GetImageInfoEx(GifGetData,&GIFFile,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}else //否则图片需要缩放
			{
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubScaledEx(GifGetData,&GIFFile,x,y,i,member,denom);
					GUI_GIF_GetImageInfoEx(GifGetData,&GIFFile,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}
			break;
		case 1:	//在LCD中间显示图片
			if((member == 1) && (denom == 1)) //无需缩放，直接绘制
			{
				//在LCD中间显示图片
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubEx(GifGetData,&GIFFile,(tftlcd_data.width-XSize)/2-1,(tftlcd_data.height-YSize)/2-1,i);
					GUI_GIF_GetImageInfoEx(GifGetData,&GIFFile,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}else //否则图片需要缩放
			{
				Xflag = (float)XSize*((float)member/(float)denom);
				Yflag = (float)YSize*((float)member/(float)denom);
				XSize = (tftlcd_data.width-(int)Xflag)/2-1;
				YSize = (tftlcd_data.height-(int)Yflag)/2-1;
				
				for(i=0;i<GifInfo.NumImages;i++)
				{
					GUI_GIF_DrawSubScaledEx(GifGetData,&GIFFile,XSize,YSize,i,member,denom);
					GUI_GIF_GetImageInfoEx(GifGetData,&GIFFile,&ImageInfo,i);
					GUI_Delay(ImageInfo.Delay ? ImageInfo.Delay*10:100);//延时
				}
			}
			break;
	}

	
	f_close(&GIFFile);		//关闭GIFFile文件
	return 0;
}	

//GIF图片显示例程
void gifdisplay_demo(void)
{
	
	GUI_SetBkColor(GUI_BLUE);
	GUI_SetFont(&GUI_FontHZ16);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
	while(1)
	{
		GUI_DispStringAt("Display GIF Picture",0,0);
		displaygif("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",0,0,0,1,1);
		GUI_Delay(1000);
		GUI_Clear();
	
		GUI_DispStringAt("Display GIF Picture 2/1",0,0);
		displaygif("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",1,0,0,2,1);	
		GUI_Delay(1000);
		GUI_Clear();
		
		GUI_DispStringAt("Display GIF Picture 1/2",0,0);
		displaygif("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",1,0,0,1,2);
		GUI_Delay(1000);
		GUI_Clear();
		
//		GUI_DispStringAt("Display GIFEX Picture",0,0);
//		displaygifex("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",0,0,0,1,1);
//		GUI_Delay(1000);
//		GUI_Clear();
//		
//		GUI_DispStringAt("Display GIFEX Picture 2/1",0,0);
//		displaygifex("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",1,0,0,2,1);	
//		GUI_Delay(1000);
//		GUI_Clear();
//	
//		GUI_DispStringAt("Display GIFEX Picture 1/2",0,0);
//		displaygifex("0:/EMWINPICTURE/PICTURE/GIF/cat.gif",1,0,0,1,2);
//		GUI_Delay(1000);
//		GUI_Clear();
	}
}
	
















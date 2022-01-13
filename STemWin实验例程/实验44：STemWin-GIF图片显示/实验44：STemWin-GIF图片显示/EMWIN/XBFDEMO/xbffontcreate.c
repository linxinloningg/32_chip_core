#include "xbffontcreate.h"
#include "ff.h"
#include "includes.h"					//ucos 使用	  

//定义字体
GUI_FONT XBF12_Font;
GUI_FONT XBF16_Font;
GUI_FONT XBF24_Font;
GUI_FONT XBF36_Font;

GUI_XBF_DATA	XBF12_Data;
GUI_XBF_DATA	XBF16_Data;
GUI_XBF_DATA	XBF24_Data;
GUI_XBF_DATA	XBF36_Data;

FIL XBF16FontFile;
FIL XBF12FontFile;
FIL XBF24FontFile;
FIL XBF36FontFile;

//回调函数，用来获取字体数据
//参数：Off:		在XBF中偏移(位置)
//		NumBytes:	要读取的字节数
//		pVoid:	要读取的文件
//		pBuff:	读取到的数据的缓冲区
//返回值:0 成功，1 失败
static int _cbGetData(U32 Off, U16 NumBytes, void * pVoid, void * pBuffer) 
{
	int result;
	u16 bread; 
	FIL *hFile;

	CPU_SR_ALLOC();


	hFile = (FIL*)pVoid; 
	
	//设置在文件中的偏移(位置)
	result = f_lseek(hFile,Off);
	if(result != FR_OK)	return 1; //返回错误

	//读取字体数据
	OS_CRITICAL_ENTER();	//临界区
		
	result = f_read(hFile,pBuffer,NumBytes,(UINT *)&bread); //读取数据
	

	OS_CRITICAL_EXIT();		//退出临界区
	
	if(result != FR_OK) return 1; //返回错误
	return 0; 
}

//创建XBF12字体，共EMWIN使用
//fxpath:XBF字体文件路径
//返回值:0，成功；1，失败
u8 Create_XBF12(u8 *fxpath) 
{
	int result;
	result = f_open(&XBF12FontFile,(const TCHAR*)fxpath,FA_READ);	//打开字库文件
	
	if(result != FR_OK) return 1;
	//创建XBF16字体
	GUI_XBF_CreateFont(	&XBF12_Font,    //指向GUI_FONT结构
						&XBF12_Data, 	//指向GUI_XBF_DATA结构
						GUI_XBF_TYPE_PROP_AA4_EXT,//要创建的字体类型
						_cbGetData,   	//回调函数
						&XBF12FontFile);  //窗体给回调函数_cbGetData的参数
	return 0;
}

//创建XBF16字体，共EMWIN使用
//fxpath:XBF字体文件路径
//返回值:0，成功；1，失败
u8 Create_XBF16(u8 *fxpath) 
{
	int result;
	result = f_open(&XBF16FontFile,(const TCHAR*)fxpath,FA_READ);	//打开字库文件
	
	if(result != FR_OK) return 1;
	//创建XBF16字体
	GUI_XBF_CreateFont(	&XBF16_Font,    //指向GUI_FONT结构
						&XBF16_Data, 	//指向GUI_XBF_DATA结构
						GUI_XBF_TYPE_PROP_AA4_EXT,//要创建的字体类型
						_cbGetData,   	//回调函数
						&XBF16FontFile);  //窗体给回调函数_cbGetData的参数
	return 0;
}

//创建XBF24字体，共EMWIN使用
//fxpath:XBF字体文件路径
//返回值:0，成功；1，失败
u8 Create_XBF24(u8 *fxpath) 
{
	int result;
	result = f_open(&XBF24FontFile,(const TCHAR*)fxpath,FA_READ);	//打开字库文件
	if(result != FR_OK) return 1;
	//创建XBF16字体
	GUI_XBF_CreateFont(	&XBF24_Font,    //指向GUI_FONT结构
						&XBF24_Data, 	//指向GUI_XBF_DATA结构
						GUI_XBF_TYPE_PROP_AA4_EXT,//要创建的字体类型
						_cbGetData,   	//回调函数
						&XBF24FontFile);  //窗体给回调函数_cbGetData的参数
	return 0;
}

//创建XBF36字体，共EMWIN使用
//fxpath:XBF字体文件路径
//返回值:0，成功；1，失败
u8 Create_XBF36(u8 *fxpath) 
{
	int result;
	result = f_open(&XBF36FontFile,(const TCHAR*)fxpath,FA_READ);	//打开字库文件
	if(result != FR_OK) return 1;	
	//创建XBF16字体
	GUI_XBF_CreateFont(	&XBF36_Font,    //指向GUI_FONT结构
						&XBF36_Data, 	//指向GUI_XBF_DATA结构
						GUI_XBF_TYPE_PROP_AA4_EXT,//要创建的字体类型
						_cbGetData,   	//回调函数
						&XBF36FontFile);  //窗体给回调函数_cbGetData的参数
	return 0;
}

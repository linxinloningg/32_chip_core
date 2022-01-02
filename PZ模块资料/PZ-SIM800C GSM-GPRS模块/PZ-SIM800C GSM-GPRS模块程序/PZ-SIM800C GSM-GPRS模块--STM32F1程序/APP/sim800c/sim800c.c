#include "sim800c.h"
#include "usart.h"		
#include "SysTick.h"
#include "led.h"   	 
#include "key.h"	 	 	 	 	 
#include "tftlcd.h" 	  
#include "flash.h"	 
#include "touch.h" 	 
#include "malloc.h"
#include "string.h"    
#include "font_show.h"	
#include "usart2.h" 
#include "ff.h"
#include "time.h"



u8 Scan_Wtime = 0;//保存蓝牙扫描需要的时间  
u8 BT_Scan_mode=0;//蓝牙扫描设备模式标志



//ATK-SIM800C 各项测试(拨号测试、短信测试、GPRS测试、蓝牙测试)共用代码
//SIM800C发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//其他,期待应答结果的位置(str的位置)
u8* sim800c_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART2_RX_STA&0X8000)  //接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

//向SIM800C发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim800c_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while((USART2->SR&0X40)==0);//等待上一次数据发送完成  
		USART2->DR=(u32)cmd;
	}else usart2_printf("%s\r\n",cmd);  //发送命令
	
	if(waittime==1100)              //11s后读回串口数据(蓝牙测试用到)
	{
		Scan_Wtime = 11;            //需要定时的时间
		TIM7_SetARR(10000-1);       //产生1S定时中断
	}
	if(ack&&waittime)		        //需要等待应答
	{
		while(--waittime)	        //等待倒计时
		{ 
			if(BT_Scan_mode)        //蓝牙扫描模式下
			{
				res=KEY_Scan(0);    //返回上一级
				if(res==KEY_DOWN) return 2;
			}
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(sim800c_check_cmd(ack))break;//得到有效数据 
				USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
}


//测试界面主UI
void sim800c_mtest_ui(u16 x,u16 y)
{
	u8 *p,*p1,*p2; 
	p=mymalloc(SRAMIN,50);//申请50个字节的内存
	LCD_Clear(WHITE);
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(10,y,"PZ-SIM800C GSM/GPRS测试程序");
	LCD_ShowFont12Char(x,y+25,"请选择:");
	LCD_ShowFont12Char(x,y+45,"K_RIGHT:拨号测试");
	LCD_ShowFont12Char(x,y+65,"K_DOWN:短信测试");
	LCD_ShowFont12Char(x,y+85,"K_LEFT:蓝牙测试");
	LCD_ShowFont12Char(x,y+105,"K_UP:GPRS测试");		
	FRONT_COLOR=BLUE; 	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CGMI","OK",200)==0)			 //查询制造商名称
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF+2),"\r\n");
		p1[0]=0;//加入结束符
		sprintf((char*)p,"制造商:%s",USART2_RX_BUF+2);
		LCD_ShowFont12Char(x,y+110+25,p);
		USART2_RX_STA=0;		
	} 
	if(sim800c_send_cmd("AT+CGMM","OK",200)==0)          //查询模块名字
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF+2),"\r\n"); 
		p1[0]=0;//加入结束符
		sprintf((char*)p,"模块型号:%s",USART2_RX_BUF+2);
		LCD_ShowFont12Char(x,y+130+25,p);
		USART2_RX_STA=0;		
	} 
	if(sim800c_send_cmd("AT+CGSN","OK",200)==0)          //查询产品序列号
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF+2),"\r\n");//查找回车
		p1[0]=0;//加入结束符 
		sprintf((char*)p,"序列号:%s",USART2_RX_BUF+2);
		LCD_ShowFont12Char(x,y+150+25,p);
		USART2_RX_STA=0;		
	}
	if(sim800c_send_cmd("AT+CNUM","+CNUM",200)==0)		 //查询本机号码
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+2),"\"");
		p2[0]=0;//加入结束符
		sprintf((char*)p,"本机号码:%s",p1+2);
		LCD_ShowFont12Char(x,y+170+25,p);
		USART2_RX_STA=0;		
	}
	myfree(SRAMIN,p); 
}

//NTP网络同步时间
void ntp_update(void)
{  
	sim800c_send_cmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK",200);//配置承载场景1
	sim800c_send_cmd("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",200);
	sim800c_send_cmd("AT+SAPBR=1,1",0,200);//激活一个GPRS上下文
	delay_ms(5);
	sim800c_send_cmd("AT+CNTPCID=1","OK",200);//设置CNTP使用的CID
	sim800c_send_cmd("AT+CNTP=\"202.120.2.101\",32","OK",200);//设置NTP服务器和本地时区(32时区 时间最准确)
	sim800c_send_cmd("AT+CNTP","+CNTP: 1",600);//同步网络时间
}

//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART2_RX_STA;
//     1,清零USART2_RX_STA;
void sim_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		          //接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART2_RX_BUF);	          //发送到串口
		if(mode)USART2_RX_STA=0;
	} 
}

//键盘码表
const u8* kbd_tbl1[13]={"1","2","3","4","5","6","7","8","9","*","0","#","DEL"};
const u8* kbd_tbl2[13]={"1","2","3","4","5","6","7","8","9",".","0","#","DEL"};
u8** kbd_tbl;
u8* kbd_fn_tbl[2];

//加载键盘界面（尺寸为240*140）
//x,y:界面起始坐标（320*240分辨率的时候，x必须为0）
void sim800c_load_keyboard(u16 x,u16 y,u8 **kbtbl)
{
	u16 i;
	FRONT_COLOR=RED;
	kbd_tbl=kbtbl;
	LCD_Fill(x,y,x+240,y+140,WHITE);
	LCD_DrawRectangle(x,y,x+240,y+140);						   
	LCD_DrawRectangle(x+80,y,x+160,y+140);	 
	LCD_DrawRectangle(x,y+28,x+240,y+56);
	LCD_DrawRectangle(x,y+84,x+240,y+112);
	FRONT_COLOR=BLUE;
	for(i=0;i<15;i++)
	{
		if(i<13)
			LCD_ShowFont12Char(x+(i%3)*80+30,y+6+28*(i/3),(u8*)kbd_tbl[i]);
		else 
			LCD_ShowFont12Char(x+(i%3)*80+30,y+6+28*(i/3),kbd_fn_tbl[i-13]);
	}  		 					   
}


//按键状态设置
//x,y:键盘坐标
//key:键值（0~8）
//sta:状态，0，松开；1，按下；
void sim800c_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>15)return;
	if(sta)LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,GREEN);
	else LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,WHITE); 
	if(j&&(i>3))
		LCD_ShowFont12Char(x+j*80+30,y+6+28*i,(u8*)kbd_fn_tbl[keyx-13]);
	else
		LCD_ShowFont12Char(x+j*80+30,y+6+28*i,(u8*)kbd_tbl[keyx]);			 		 
}

//得到触摸屏的输入
//x,y:键盘坐标
//返回值：按键键值（1~15有效；0,无效）
u8 sim800c_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x=0;//0,没有任何按键按下；1~15，1~15号按键按下
	u8 key=0;
	tp_dev.scan(0);	
	if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
	{	
		for(i=0;i<5;i++)
		{
			for(j=0;j<3;j++)
			{
			 	if(tp_dev.x[0]<(x+j*80+80)&&tp_dev.x[0]>(x+j*80)&&tp_dev.y[0]<(y+i*28+28)&&tp_dev.y[0]>(y+i*28))
				{	
					key=i*3+j+1;	 
					break;	 		   
				}
			}
			if(key)
			{	   
				if(key_x==key)key=0;
				else 
				{
					sim800c_key_staset(x,y,key_x-1,0);
					key_x=key;
					sim800c_key_staset(x,y,key_x-1,1);
				}
				break;
			}
		}  
	}else if(key_x) 
	{
		sim800c_key_staset(x,y,key_x-1,0);
		key_x=0;
	} 
	return key; 
}

//GSM信息显示(信号质量,电池电量,日期时间)
//返回值:0,正常
//其他,错误代码
u8 sim800c_gsminfo_show(u16 x,u16 y)
{
	u8 *p,*p1,*p2;
	u8 res=0;
	p=mymalloc(SRAMIN,50);//申请50个字节的内存
	FRONT_COLOR=BLUE; 	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CPIN?","OK",200))res|=1<<0;	//查询SIM卡是否在位 
	USART2_RX_STA=0;  
	if(sim800c_send_cmd("AT+COPS?","OK",200)==0)		//查询运营商名字
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),"\""); 
		if(p1)//有有效数据
		{
			p2=(u8*)strstr((const char*)(p1+1),"\"");
			p2[0]=0;//加入结束符			
			sprintf((char*)p,"运营商:%s",p1+1);
			LCD_ShowFont12Char(x,y,p);
		} 
		USART2_RX_STA=0;		
	}else res|=1<<1;
	if(sim800c_send_cmd("AT+CSQ","+CSQ:",200)==0)		//查询信号质量
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),":");
		p2=(u8*)strstr((const char*)(p1),",");
		p2[0]=0;//加入结束符
		sprintf((char*)p,"信号质量:%s",p1+2);
		LCD_ShowFont12Char(x,y+20,p);
		USART2_RX_STA=0;		
	}else res|=1<<2;
	if(sim800c_send_cmd("AT+CBC","+CBC:",200)==0)		//查询电池电量
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+1),",");
		p2[0]=0;p2[5]=0;//加入结束符
		sprintf((char*)p,"电池电量:%s%%  %smV",p1+1,p2+1);
		LCD_ShowFont12Char(x,y+40,p);
		USART2_RX_STA=0;		
	}else res|=1<<3; 
	if(sim800c_send_cmd("AT+CCLK?","+CCLK:",200)==0)	//查询电池电量
	{ 
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),"\"");
		p2=(u8*)strstr((const char*)(p1+1),":");
		p2[3]=0;//加入结束符
		sprintf((char*)p,"日期时间:%s",p1+1);
		LCD_ShowFont12Char(x,y+60,p);
		USART2_RX_STA=0;		
	}else res|=1<<4; 
	myfree(SRAMIN,p); 
	return res;
}

//SIM800C拨号测试
//用于拨打电话和接听电话
//返回值:0,正常
//    其他,错误代码
u8 sim800c_call_test(void)
{
	u8 key;
	u16 lenx;
	u8 callbuf[20]; 
	u8 pohnenumlen=0;//号码长度,最大15个数 
	u8 *p,*p1,*p2;
	u8 oldmode=0;
	u8 cmode=0;	//模式
				//0:等待拨号
				//1:拨号中
	            //2:通话中
				//3:接收到来电 
	LCD_Clear(WHITE);
	//if(sim800c_send_cmd("AT+CTTSRING=0","OK",200))return 1;	//设置TTS来电设置 0：来电有铃声 1：；来电没有铃声
	//if(sim800c_send_cmd("AT+CTTSPARAM=20,0,50,70,0","OK",200))return 1;//设置TTS声音大小、语调配置
	if(sim800c_send_cmd("AT+CLIP=1","OK",200))return 1;	//设置来电显示 
	if(sim800c_send_cmd("AT+COLP=1","OK",200))return 2;	//设置被叫号码显示 
 	p1=mymalloc(SRAMIN,20);								//申请20直接用于存放号码
	if(p1==NULL)return 2;	
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(10,30,"PZ-SIM800C GSM/GPRS拨号测试");
	LCD_ShowFont12Char(40,70,"请拨号:");		   
	kbd_fn_tbl[0]="拨号";
	kbd_fn_tbl[1]="返回"; 
	sim800c_load_keyboard(0,180,(u8**)kbd_tbl1);
	FRONT_COLOR=BLUE; 
	while(1)
	{
		delay_ms(10);
		if(USART2_RX_STA&0X8000)		//接收到数据
		{
			sim_at_response(0);
			if(cmode==1||cmode==2)
			{
				if(cmode==1)if(sim800c_check_cmd("+COLP:"))cmode=2;	//拨号成功
				if(sim800c_check_cmd("NO CARRIER"))cmode=0;	//拨号失败
				if(sim800c_check_cmd("NO ANSWER"))cmode=0;	//拨号失败
				if(sim800c_check_cmd("ERROR"))cmode=0;		//拨号失败
			}
			if(sim800c_check_cmd("+CLIP:"))//接收到来电
			{
				cmode=3;
				p=sim800c_check_cmd("+CLIP:");
				p+=8;
				p2=(u8*)strstr((const char *)p,"\"");
				p2[0]=0;//添加结束符 
				strcpy((char*)p1,(char*)p);
			}
			USART2_RX_STA=0;
		}
		key=sim800c_get_keynum(0,180);
		if(key)
		{ 
			if(key<13)
			{
				if(cmode==0&&pohnenumlen<15)
				{ 
					callbuf[pohnenumlen++]=kbd_tbl[key-1][0];
					usart2_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
					delay_ms(55);//延时
					usart2_printf("AT+CTTS=2,\"%c\"\r\n",kbd_tbl[key-1][0]); //TTS语音		
				}else if(cmode==2)//通话中
				{ 
					usart2_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]);
					delay_ms(100);
					usart2_printf("AT+VTS=%c\r\n",kbd_tbl[key-1][0]); 
					LCD_ShowChar(40+56,90,kbd_tbl[key-1][0],16,0);
				}
			}else
			{
				if(key==13)if(pohnenumlen&&cmode==0)pohnenumlen--;//删除
				if(key==14)//执行拨号
				{
					if(cmode==0)//拨号模式
					{
						callbuf[pohnenumlen]=0;			//最后加入结束符 
						delay_ms(200);
						usart2_printf("ATD%s;\r\n",callbuf);//拨号
						cmode=1;						//拨号中模式
					}else 
					{
						sim800c_send_cmd("ATH","OK",200);//挂机
						cmode=0;
					}
				}
				if(key==15)
				{
					if(cmode==3)//接收到来电
					{
						sim800c_send_cmd("ATA","OK",200);//发送应答指令
						LCD_ShowFont12Char(40+56,70,callbuf);
						cmode=2;
					}else
					{
						sim800c_send_cmd("ATH",0,0);//不管有没有在通话,都结束通话
						break;//退出循环
					}
				}
			} 
			if(cmode==0)//只有在等待拨号模式有效
			{
				callbuf[pohnenumlen]=0; 
				LCD_Fill(40+56,70,239,70+16,WHITE);
				LCD_ShowFont12Char(40+56,70,callbuf);	
			}				
		}
		if(oldmode!=cmode)//模式变化了
		{
			switch(cmode)
			{
				case 0: 
					kbd_fn_tbl[0]="拨号";
					kbd_fn_tbl[1]="返回"; 
					FRONT_COLOR=RED;
					LCD_ShowFont12Char(40,70,"请拨号:"); 
					LCD_Fill(40+56,70,239,70+16,WHITE);
					if(pohnenumlen)
					{
						FRONT_COLOR=BLUE;
						LCD_ShowFont12Char(40+56,70,callbuf);
					}
					break;
				case 1:
					FRONT_COLOR=RED;
					LCD_ShowFont12Char(40,70,"拨号中:"); 
					pohnenumlen=0;
				case 2:
					FRONT_COLOR=RED;
					if(cmode==2)
						LCD_ShowFont12Char(40,70,"通话中:");
					kbd_fn_tbl[0]="挂断";
					kbd_fn_tbl[1]="返回"; 	
					break;
				case 3:
					FRONT_COLOR=RED;
					LCD_ShowFont12Char(40,70,"有来电:");
					FRONT_COLOR=BLUE;
					LCD_ShowFont12Char(40+56,70,p1);
					kbd_fn_tbl[0]="挂断";
					kbd_fn_tbl[1]="接听"; 
					break;				
			}
			if(cmode==2)
				LCD_ShowFont12Char(40,90,"DTMF音:");//通话中,可以通过键盘输入DTMF音
			else LCD_Fill(40,90,120,90+16,WHITE);
			sim800c_load_keyboard(0,180,(u8**)kbd_tbl1);		//显示键盘 
			oldmode=cmode; 
		}
		if((lenx%50)==0)led1=!led1; 	    				 
		lenx++;	 
	} 
	myfree(SRAMIN,p1);
	return 0;
}


//将1个字符转换为16进制数字
//chr:字符,0~9/A~F/a~F
//返回值:chr对应的16进制数值
u8 sim800c_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}

//将1个16进制数字转换为字符
//hex:16进制数字,0~15;
//返回值:字符
u8 sim800c_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}
//unicode gbk 转换函数
//src:输入字符串
//dst:输出(uni2gbk时为gbk内码,gbk2uni时,为unicode字符串)
//mode:0,unicode到gbk转换;
//     1,gbk到unicode转换;
void sim800c_unigbk_exchange(u8 *src,u8 *dst,u8 mode)
{
	u16 temp; 
	u8 buf[2];
	if(mode)//gbk 2 unicode
	{
		while(*src!=0)
		{
			if(*src<0X81)	//非汉字
			{
				temp=(u16)ff_convert((WCHAR)*src,1);
				src++;
			}else 			//汉字,占2个字节
			{
				buf[1]=*src++;
				buf[0]=*src++; 
				temp=(u16)ff_convert((WCHAR)*(u16*)buf,1); 
			}
			*dst++=sim800c_hex2chr((temp>>12)&0X0F);
			*dst++=sim800c_hex2chr((temp>>8)&0X0F);
			*dst++=sim800c_hex2chr((temp>>4)&0X0F);
			*dst++=sim800c_hex2chr(temp&0X0F);
		}
	}else	//unicode 2 gbk
	{ 
		while(*src!=0)
		{
			buf[1]=sim800c_chr2hex(*src++)*16;
			buf[1]+=sim800c_chr2hex(*src++);
			buf[0]=sim800c_chr2hex(*src++)*16;
			buf[0]+=sim800c_chr2hex(*src++);
 			temp=(u16)ff_convert((WCHAR)*(u16*)buf,0);
			if(temp<0X80){*dst=temp;dst++;}
			else {*(u16*)dst=swap16(temp);dst+=2;}
		} 
	}
	*dst=0;//添加结束符
} 

//SIM800C读短信测试
void sim800c_sms_read_test(void)
{ 
	u8 *p,*p1,*p2;
	u8 timex=0;
	u8 msgindex[3];
	u8 msglen=0;
	u8 msgmaxnum=0;		//短信最大条数
	u8 key=0;
	u8 smsreadsta=0;	//是否在短信显示状态
	p=mymalloc(SRAMIN,200);//申请200个字节的内存
	LCD_Clear(WHITE); 
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(10,30,"PZ-SIM800C 读短信测试");
	LCD_ShowFont12Char(30,50,"读取:     总信息:");	
	kbd_fn_tbl[0]="读取";
	kbd_fn_tbl[1]="返回"; 
	sim800c_load_keyboard(0,180,(u8**)kbd_tbl1);//显示键盘  
	while(1)
	{
		key=sim800c_get_keynum(0,180);
		if(key)
		{  
			if(smsreadsta)
			{
				LCD_Fill(30,75,239,179,WHITE);//清除显示的短信内容
				usart2_printf("AT+CTTS=0\r\n");
				smsreadsta=0;
			}
			if(key<10||key==11)
			{
				if(msglen<2)
				{ 
					msgindex[msglen++]=kbd_tbl[key-1][0];
					usart2_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
				} 
				if(msglen==2)
				{
					key=(msgindex[0]-'0')*10+msgindex[1]-'0';
					if(key>msgmaxnum)
					{
						msgindex[0]=msgmaxnum/10+'0';
						msgindex[1]=msgmaxnum%10+'0';					
					}
				}				
			}else
			{ 
				if(key==10||key==12) usart2_printf("AT+CTTS=0\r\n");
				if(key==13)
				{usart2_printf("AT+CTTS=0\r\n");if(msglen)msglen--;}//删除  
				if(key==14&&msglen)//执行读取短信
				{ 
					LCD_Fill(30,75,239,179,WHITE);//清除之前的显示	   	 
					sprintf((char*)p,"AT+CMGR=%s",msgindex);
					if(sim800c_send_cmd(p,"+CMGR:",200)==0)//读取短信
					{
						FRONT_COLOR=RED;
						LCD_ShowFont12Char(30,75,"状态:");	
						LCD_ShowFont12Char(30+75,75,"来自:");	
						LCD_ShowFont12Char(30,90,"接收时间:");
						LCD_ShowFont12Char(30,105,"内容:");
						FRONT_COLOR=BLUE;
						if(strstr((const char*)(USART2_RX_BUF),"UNREAD")==0)
							LCD_ShowFont12Char(30+40,75,"已读");
						else 
							LCD_ShowFont12Char(30+40,75,"未读");
						p1=(u8*)strstr((const char*)(USART2_RX_BUF),",");
						p2=(u8*)strstr((const char*)(p1+2),"\"");
						p2[0]=0;//加入结束符
						sim800c_unigbk_exchange(p1+2,p,0);			//将unicode字符转换为gbk码 
						LCD_ShowFont12Char(30+75+40,75,p);//显示电话号码	
						p1=(u8*)strstr((const char*)(p2+1),"/");
						p2=(u8*)strstr((const char*)(p1),"+");
						p2[0]=0;//加入结束符
						LCD_ShowFont12Char(30+54+15,90,p1-2);//显示接收时间		  
						p1=(u8*)strstr((const char*)(p2+1),"\r");	//寻找回车符
						sim800c_unigbk_exchange(p1+2,p,0);			//将unicode字符转换为gbk码
						LCD_ShowFont12Char(30+40,105,p);//显示短信内容			
						usart2_printf("AT+CTTS=2,\"%s\"\r\n",p);        //TTS读取短信 ASCII模式
						smsreadsta=1;								//标记有显示短信内容 
					}else
					{
						LCD_ShowFont12Char(30,75,"无短信内容!!!请检查!!");
						usart2_printf("AT+CTTS=2,\"无短信内容请检查\"\r\n");
						delay_ms(1000);
						delay_ms(1000);
						LCD_Fill(30,75,239,75+16,WHITE);//清除显示
					}	  
					USART2_RX_STA=0;
				}
				if(key==15){usart2_printf("AT+CTTS=0\r\n"); break;}
			} 
			msgindex[msglen]=0; 
			LCD_Fill(30+40,50,86,50+16,WHITE);
			LCD_ShowFont12Char(30+40,50,msgindex);	
		}
		if(timex==0)		//2.5秒左右更新一次
		{
			if(sim800c_send_cmd("AT+CPMS?","+CPMS:",200)==0)	//查询优选消息存储器
			{ 
				p1=(u8*)strstr((const char*)(USART2_RX_BUF),","); 
				p2=(u8*)strstr((const char*)(p1+1),",");
				p2[0]='/'; 
				if(p2[3]==',')//小于64K SIM卡，最多存储几十条短信
				{
					msgmaxnum=(p2[1]-'0')*10+p2[2]-'0'; //获取最大存储短信条数
					p2[3]=0;
				}else //如果是64K SIM卡，则能存储100条以上的信息
				{
					msgmaxnum=(p2[1]-'0')*100+(p2[2]-'0')*10+p2[3]-'0';//获取最大存储短信条数
					p2[4]=0;
				}
				sprintf((char*)p,"%s",p1+1);
				LCD_ShowFont12Char(30+17*8,50,p);	
				USART2_RX_STA=0;		
			}
		}	
		if((timex%20)==0)led1=!led1;//200ms闪烁 
		timex++;
		delay_ms(10);
		if(USART2_RX_STA&0X8000)sim_at_response(1);//检查从GSM模块接收到的数据 
	}
	myfree(SRAMIN,p); 
}

//sms测试主界面
void sim800c_sms_ui(u16 x,u16 y)
{ 
	LCD_Clear(WHITE);
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(10,y,"PZ-SIM800C GSM/GPRS短信测试");
	LCD_ShowFont12Char(x,y+40,"请选择:");
	LCD_ShowFont12Char(x,y+60,"K_RIGHT:读短信测试");
	LCD_ShowFont12Char(x,y+80,"K_DOWN:发短信测试");
	LCD_ShowFont12Char(x,y+100,"K_UP:返回上级菜单");
}


//测试短信发送内容
const u8* sim800c_test_msg="您好，这是一条测试短信，由PZ-SIM800C GSM/GPRS模块发送";
//SIM800C发短信测试 
void sim800c_sms_send_test(void)
{
	u8 *p,*p1,*p2;
	u8 phonebuf[20]; 		//号码缓存
	u8 pohnenumlen=0;		//号码长度,最大15个数 
	u8 timex=0;
	u8 key=0;
	u8 smssendsta=0;		//短信发送状态,0,等待发送;1,发送失败;2,发送成功 
	p=mymalloc(SRAMIN,100);	//申请100个字节的内存,用于存放电话号码的unicode字符串
	p1=mymalloc(SRAMIN,300);//申请300个字节的内存,用于存放短信的unicode字符串
	p2=mymalloc(SRAMIN,100);//申请100个字节的内存 存放：AT+CMGS=p1 
	LCD_Clear(WHITE);  
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(30,30,"PZ-SIM800C 发短信测试");
	LCD_ShowFont12Char(30,50,"发送给:");
	LCD_ShowFont12Char(30,70,"状态:");
	LCD_ShowFont12Char(30,90,"内容:");	 
	FRONT_COLOR=BLUE;
	LCD_ShowFont12Char(30+40,70,"等待发送");//显示状态	
	LCD_ShowFont12Char(30+40,90,(u8*)sim800c_test_msg);//显示短信内容
			
	kbd_fn_tbl[0]="发送";
	kbd_fn_tbl[1]="返回"; 
	sim800c_load_keyboard(0,180,(u8**)kbd_tbl1);//显示键盘 
	while(1)
	{
		key=sim800c_get_keynum(0,180);
		if(key)
		{   
			if(smssendsta)
			{
				smssendsta=0;
				LCD_ShowFont12Char(30+40,70,"等待发送");//显示状态	
			}
			if(key<10||key==11)
			{
				if(pohnenumlen<15)
				{ 
					phonebuf[pohnenumlen++]=kbd_tbl[key-1][0];
					usart2_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
				}
			}else
			{
				if(key==13)if(pohnenumlen)pohnenumlen--;//删除  
				if(key==14&&pohnenumlen)				//执行发送短信
				{  
					LCD_ShowFont12Char(30+40,70,"正在发送");//显示正在发送
								
					smssendsta=1;		 
					sim800c_unigbk_exchange(phonebuf,p,1);				//将电话号码转换为unicode字符串
					sim800c_unigbk_exchange((u8*)sim800c_test_msg,p1,1);//将短信内容转换为unicode字符串.
					sprintf((char*)p2,"AT+CMGS=\"%s\"",p); 
					if(sim800c_send_cmd(p2,">",200)==0)					//发送短信命令+电话号码
					{ 		 				 													 
						usart2_printf("%s",p1);		 						//发送短信内容到GSM模块 
 						if(sim800c_send_cmd((u8*)0X1A,"+CMGS:",1000)==0)smssendsta=2;//发送结束符,等待发送完成(最长等待10秒钟,因为短信长了的话,等待时间会长一些)
					}  
					if(smssendsta==1)
						LCD_ShowFont12Char(30+40,70,"发送失败");//显示状态	
					else 
						LCD_ShowFont12Char(30+40,70,"发送成功");
					USART2_RX_STA=0;
				}
				if(key==15)break;
			} 
			phonebuf[pohnenumlen]=0; 
			LCD_Fill(30+54,50,239,50+16,WHITE);
			LCD_ShowFont12Char(30+54,50,phonebuf); 	
		}
		if((timex%20)==0)led1=!led1;//200ms闪烁 
		timex++;
		delay_ms(10);
		if(USART2_RX_STA&0X8000)sim_at_response(1);//检查从GSM模块接收到的数据 
	}
	myfree(SRAMIN,p);
	myfree(SRAMIN,p1);
	myfree(SRAMIN,p2); 
} 

//SIM800C短信测试
//用于读短信或者发短信
//返回值:0,正常
//其他,错误代码
u8 sim800c_sms_test(void)
{
	u8 key;
	u8 timex=0;
	if(sim800c_send_cmd("AT+CMGF=1","OK",200))return 1;			//设置文本模式 
	if(sim800c_send_cmd("AT+CSCS=\"UCS2\"","OK",200))return 2;	//设置TE字符集为UCS2 
	if(sim800c_send_cmd("AT+CSMP=17,0,2,25","OK",200))return 3;	//设置短消息文本模式参数 
//	if(sim800c_send_cmd("AT+CTTSPARAM=5,0,51,61,0","OK",200))return 1;//设置TTS声音大小、语调配置
	sim800c_sms_ui(40,30);
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_RIGHT)
		{ 
			sim800c_sms_read_test();
			sim800c_sms_ui(40,30);
			timex=0;
		}else if(key==KEY_DOWN)
		{ 
			sim800c_sms_send_test();
			sim800c_sms_ui(40,30);
			timex=0;			
		}else if(key==KEY_UP)break;
		timex++;
		if(timex==20)
		{
			timex=0;
			led1=!led1;
		}
		delay_ms(10);
		sim_at_response(1);										//检查GSM模块发送过来的数据,及时上传给电脑
	} 
	sim800c_send_cmd("AT+CSCS=\"GSM\"","OK",200);				//设置默认的GSM 7位缺省字符集
	return 0;
} 



//gprs测试主界面
void sim800c_gprs_ui(void)
{
	LCD_Clear(WHITE);  
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(30,30,"PZ-SIM800C GPRS通信测试");
	LCD_ShowFont12Char(30,50,"K_UP:连接方式切换");
	LCD_ShowFont12Char(30,90,"连接方式:");//连接方式通过KEY_UP设置(TCP/UDP)
	LCD_ShowFont12Char(30,110,"IP地址:");//IP地址可以键盘设置
	LCD_ShowFont12Char(30,130,"端口:");//端口固定为8086		
	kbd_fn_tbl[0]="连接";
	kbd_fn_tbl[1]="返回"; 
	sim800c_load_keyboard(0,180,(u8**)kbd_tbl2);//显示键盘 
} 



const u8 *modetbl[2]={"TCP","UDP"};//连接模式

//tcp/udp测试
//带心跳功能,以维持连接
//mode:0:TCP测试;1,UDP测试)
//ipaddr:ip地址
//port:端口 
void sim800c_tcpudp_test(u8 mode,u8* ipaddr,u8* port)
{ 
	u8 *p,*p1,*p2,*p3;
	u8 key;
	u16 timex=0;
	u8 count=0;
	const u8* cnttbl[3]={"正在连接","连接成功","连接关闭"};
	u8 connectsta=0;			//0,正在连接;1,连接成功;2,连接关闭; 
	u8 hbeaterrcnt=0;			//心跳错误计数器,连续5次心跳信号无应答,则重新连接
	u8 oldsta=0XFF;
	p=mymalloc(SRAMIN,100);		//申请100字节内存
	p1=mymalloc(SRAMIN,100);	//申请100字节内存
	LCD_Clear(WHITE);  
	FRONT_COLOR=RED; 
	if(mode)
		LCD_ShowFont12Char(30,30,"PZ-SIM800C UDP连接测试");
	else 
		LCD_ShowFont12Char(30,30,"PZ-SIM800C TCP连接测试");
	LCD_ShowFont12Char(10,50,"K_UP:退出测试  K_RIGHT:发送数据");	
	sprintf((char*)p,"IP地址:%s 端口:%s",ipaddr,port);
	LCD_ShowFont12Char(10,80,p);//显示IP地址和端口
	LCD_ShowFont12Char(10,110,"状态:"); //连接状态
	LCD_ShowFont12Char(10,130,"发送数据:"); 			 	
	LCD_ShowFont12Char(10,170,"接收数据:");	
	FRONT_COLOR=BLUE;
	USART2_RX_STA=0;
	sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);
	if(sim800c_send_cmd(p,"OK",500))return;		//发起连接
	while(1)
	{ 
		key=KEY_Scan(0);
		if(key==KEY_UP)//退出测试		 
		{  
			sim800c_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//关闭连接
			sim800c_send_cmd("AT+CIPSHUT","SHUT OK",500);		//关闭移动场景 
			break;												 
		}else if(key==KEY_RIGHT&(hbeaterrcnt==0))				//发送数据(心跳正常时发送)
		{
			LCD_ShowFont12Char(30+30,110,"数据发送中...");//提示数据发送中
		
			if(sim800c_send_cmd("AT+CIPSEND",">",500)==0)		//发送数据
			{ 
 				printf("CIPSEND DATA:%s\r\n",p1);	 			//发送数据打印到串口
				usart2_printf("%s\r\n",p1);
				delay_ms(10);
				if(sim800c_send_cmd((u8*)0X1A,"SEND OK",1000)==0)
					LCD_ShowFont12Char(30+30,110,"数据发送成功!");//最长等待10s
				else 
					LCD_ShowFont12Char(30+30,110,"数据发送失败!");
				delay_ms(1000); 
			}else sim800c_send_cmd((u8*)0X1B,0,0);	//ESC,取消发送 
			oldsta=0XFF;			
		}
		if((timex%20)==0)
		{
			led1=!led1;
			count++;	
			if(connectsta==2||hbeaterrcnt>8)//连接中断了,或者连续8次心跳没有正确发送成功,则重新连接
			{
				sim800c_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//关闭连接
				sim800c_send_cmd("AT+CIPSHUT","SHUT OK",500);		//关闭移动场景 
				sim800c_send_cmd(p,"OK",500);						//尝试重新连接
				connectsta=0;	
 				hbeaterrcnt=0;
			}
			sprintf((char*)p1,"PZ-SIM800C %s测试 %d  ",modetbl[mode],count);
			LCD_ShowFont12Char(30+54,130,p1);
		}
		if(connectsta==0&&(timex%200)==0)//连接还没建立的时候,每2秒查询一次CIPSTATUS.
		{
			sim800c_send_cmd("AT+CIPSTATUS","OK",500);	//查询连接状态
			if(strstr((const char*)USART2_RX_BUF,"CLOSED"))connectsta=2;
			if(strstr((const char*)USART2_RX_BUF,"CONNECT OK"))connectsta=1;
		}
		if(connectsta==1&&timex>=600)//连接正常的时候,每6秒发送一次心跳
		{
			timex=0;
			if(sim800c_send_cmd("AT+CIPSEND",">",200)==0)//发送数据
			{
				sim800c_send_cmd((u8*)0X00,0,0);	//发送数据:0X00  
				delay_ms(20);						//必须加延时
				sim800c_send_cmd((u8*)0X1A,0,0);	//CTRL+Z,结束数据发送,启动一次传输	
			}else sim800c_send_cmd((u8*)0X1B,0,0);	//ESC,取消发送 		
				
			hbeaterrcnt++; 
			printf("hbeaterrcnt:%d\r\n",hbeaterrcnt);//方便调试代码
		} 
		delay_ms(10);
		if(USART2_RX_STA&0X8000)		//接收到一次数据了
		{ 
			USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;	//添加结束符 
			printf("%s",USART2_RX_BUF);				//发送到串口  
			if(hbeaterrcnt)							//需要检测心跳应答
			{
				if(strstr((const char*)USART2_RX_BUF,"SEND OK"))hbeaterrcnt=0;//心跳正常
			}				
			p2=(u8*)strstr((const char*)USART2_RX_BUF,"+IPD");
			if(p2)//接收到TCP/UDP数据
			{
				p3=(u8*)strstr((const char*)p2,",");
				p2=(u8*)strstr((const char*)p2,":");
				p2[0]=0;//加入结束符
				sprintf((char*)p1,"收到%s字节,内容如下",p3+1);//接收到的字节数
				LCD_Fill(30+54,170,239,200,WHITE);
				FRONT_COLOR=BRED;
				LCD_ShowFont12Char(30+54,170,p1);//显示接收到的数据长度
				FRONT_COLOR=BLUE;
				LCD_Fill(30,190,210,319,WHITE);
				LCD_ShowFont12Char(30,190,p2+1);//显示接收到的数据 
			}
			USART2_RX_STA=0;
		}
		if(oldsta!=connectsta)
		{
			oldsta=connectsta;
			LCD_Fill(30+30,110,239,80+12,WHITE);
			LCD_ShowFont12Char(30+30,110,(u8*)cnttbl[connectsta]); //更新状态
		} 
		timex++; 
	} 
	myfree(SRAMIN,p);
	myfree(SRAMIN,p1);
}

//sim800c GPRS测试
//用于测试TCP/UDP连接
//返回值:0,正常
//其他,错误代码
u8 sim800c_gprs_test(void)
{
	const u8 *port="8086";	//端口固定为8086,当你的电脑8086端口被其他程序占用的时候,请修改为其他空闲端口
	u8 mode=0;				//0,TCP连接;1,UDP连接
	u8 key;
	u8 timex=0; 
	u8 ipbuf[16]; 		//IP缓存
	u8 iplen=0;			//IP长度 
	sim800c_gprs_ui();	//加载主界面
	LCD_ShowFont12Char(30+72,90,(u8*)modetbl[mode]);//显示连接方式
	LCD_ShowFont12Char(30+40,130,(u8*)port);//显示端口 
			
 	sim800c_send_cmd("AT+CIPCLOSE=1","CLOSE OK",100);	//关闭连接
	sim800c_send_cmd("AT+CIPSHUT","SHUT OK",100);		//关闭移动场景 
	if(sim800c_send_cmd("AT+CGCLASS=\"B\"","OK",1000))return 1;				//设置GPRS移动台类别为B,支持包交换和数据交换 
	if(sim800c_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",1000))return 2;//设置PDP上下文,互联网接协议,接入点等信息
	if(sim800c_send_cmd("AT+CGATT=1","OK",500))return 3;					//附着GPRS业务
	if(sim800c_send_cmd("AT+CIPCSGP=1,\"CMNET\"","OK",500))return 4;	 	//设置为GPRS连接模式
	if(sim800c_send_cmd("AT+CIPHEAD=1","OK",500))return 5;	 				//设置接收数据显示IP头(方便判断数据来源)
	ipbuf[0]=0; 		
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)		 
		{  
			mode=!mode;		//连接模式切换
			LCD_ShowFont12Char(30+72,90,(u8*)modetbl[mode]);//显示连接模式
		} 
		key=sim800c_get_keynum(0,180);
		if(key)
		{   
			if(key<12)
			{
				if(iplen<15)
				{ 
					ipbuf[iplen++]=kbd_tbl[key-1][0];
					usart2_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
				}
			}else
			{
				if(key==13)if(iplen)iplen--;	//删除  
				if(key==14&&iplen)				//执行GPRS连接
				{    
					sim800c_tcpudp_test(mode,ipbuf,(u8*)port);
					sim800c_gprs_ui();			//加载主界面
					LCD_ShowFont12Char(30+72,90,(u8*)modetbl[mode]);//显示连接模式
					LCD_ShowFont12Char(30+40,130,(u8*)port); //显示端口         	
					USART2_RX_STA=0;
				}
				if(key==15)break;
			} 
			ipbuf[iplen]=0; 
			LCD_Fill(30+56,110,239,110+16,WHITE);
			LCD_ShowFont12Char(30+56,110,ipbuf);//显示IP地址		 	
		} 
		timex++;
		if(timex==20)
		{
			timex=0;
			led1=!led1;
		}
		delay_ms(10);
		sim_at_response(1);//检查GSM模块发送过来的数据,及时上传给电脑
	}
	return 0;
} 



//蓝牙SPP测试主界面
void sim800c_spp_ui(u16 x,u16 y)
{
	LCD_Clear(WHITE);
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(40,y,"PZ-SIM800C 蓝牙测试");
	LCD_ShowFont12Char(x,y+40,"请选择:");
	LCD_ShowFont12Char(x,y+60,"K_LEFT:发起配对请求模式");
	LCD_ShowFont12Char(x,y+80,"K_RIGHT:接收配对请求模式");
	LCD_ShowFont12Char(x,y+100,"K_DOWN:返回上一级");  
}


//接收SIM800C返回数据（蓝牙测试模式下使用）
//request:期待接收命令字符串
//waittimg:等待时间(单位：10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim800c_wait_request(u8 *request ,u16 waittime)
{
	 u8 res = 1;
	 u8 key;
	 if(request && waittime)
	 {
	    while(--waittime)
		{   
		   key=KEY_Scan(0);
		   if(key==KEY_DOWN) return 2;//返回上一级
		   delay_ms(10);
		   if(USART2_RX_STA &0x8000)//接收到期待的应答结果
		   {
			   if(sim800c_check_cmd(request)) break;//得到有效数据
			   USART2_RX_STA=0;
		   }
		}
		if(waittime==0)res=0;
	 }
	 return res;
}


//SIM800C蓝牙连接模式测试
//用于模式连接和串口透传  
//mode(模式):0,接收配对模式
//           1,寻找设备模式
//返回值:0,正常
//      其他,错误代码
u8 sim800c_spp_mode(u8 mode)
{
	u8 *p1,*p2;
	u8 key;
	u8 timex=1;
	u8 sendcnt=0;	
	u8 sendbuf[20];
	u8 res;
	
	LCD_Clear(WHITE);
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(40,20,"PZ-SIM800C 蓝牙测试");
	LCD_ShowFont12Char(40,60,"K_DOWN:返回上一级");
	if(mode==0)//接收配对模式
	{
		do
		{ 
			LCD_ShowFont12Char(40,30+60,"等待连接请求...");
			res = sim800c_wait_request("+BTPAIRING:",600);            //等待手机端蓝牙连接请求 6s
			if(res==1)                                                //手机端连接请求
			{
				 delay_ms(10);
				 sim800c_send_cmd("AT+BTPAIR=1,1","BTPAIR:",500);     //响应连接
			}
			else if(res==2) return 0;                                 //按键返回上一级
			LCD_ShowFont12Char(40,30+60,"                 ");
			delay_ms(50);
			led1=!led1;    	
	   }while(strstr((const char*)USART2_RX_BUF,"+BTPAIR: 1")==NULL); //判断是否匹配成功
		USART2_RX_STA=0;
	}
	else if(mode==1)//寻找设备模式
	{ 
		BT_Scan_mode=1;                                                //设置蓝牙设备扫描标志
		do
		{ 
			LCD_ShowFont12Char(40,30+60,"搜索设备中...");
			res=sim800c_send_cmd("AT+BTSCAN=1,11","+BTSCAN: 1",1100);//搜索附近的蓝牙设备,搜索11s(重新配置定时器分频系数，定时为1S中断，蓝牙扫描结束后重新配置为10ms定时中断)
			if(res==2){BT_Scan_mode=0;return 0;}                     //按键返回上一级
			LCD_ShowFont12Char(40,30+60,"             ");
			delay_ms(100);
			led1=!led1;			
		}while(strstr((const char*)USART2_RX_BUF,"+BTSCAN: 0")==NULL);//判断是否扫描到设备
		USART2_RX_STA=0;
		do
		{ 
			LCD_ShowFont12Char(40,30+60,"发现设备");
			res= sim800c_send_cmd("AT+BTPAIR=0,1","+BTPAIRING:",400);//连接搜索到的第一个设备
			if(res==2){BT_Scan_mode=0;return 0;}                     //按键返回上一级
			delay_ms(100);
			LCD_ShowFont12Char(40,30+60,"正在连接中.....");
			sim800c_send_cmd("AT+BTPAIR=1,1","BTPAIR:",500);         //响应连接，如果手机长期不确认匹配，SIM800C端蓝牙 30S后才会上报配对失败
		}while(strstr((const char*)USART2_RX_BUF,"+BTPAIR: 1")==NULL);//判断是否匹配成功
		USART2_RX_STA=0;
		BT_Scan_mode=0;                                               //清除蓝牙设备扫描标志
	}
	LCD_ShowFont12Char(40,30+60,"蓝牙连接成功         ");
	do
	{
		if(!led1)
			LCD_ShowFont12Char(40,120,"等待SPP连接");
		else 
			LCD_ShowFont12Char(40,120,"            ");
		res = sim800c_wait_request("SPP",120);                        //等待手机端SPP连接请求
		if(res==2)return 0;                                           //按键返回上一级
		else if(res==1) break;                                        //SPP连接成功
		led1=!led1;
	}while(1);
	if(!sim800c_send_cmd("AT+BTACPT=1","+BTCONNECT:",300))           //应答手机端spp连接请求 3S
	{
		LCD_ShowFont12Char(40,120,"SPP连接成功");
		delay_ms(1000);
		LCD_ShowFont12Char(40,120,"             ");
		LCD_ShowFont12Char(10,140,"发送数据:");
		LCD_ShowFont12Char(10,200,"接收数据:");
	}
	else  
	{
		LCD_ShowFont12Char(40,120,"SPP连接失败");
		return 0;//若一段时间内没有应答，则需要重新连接蓝牙!
	}
	while(1)
	{
		key = KEY_Scan(0);
		if(key == KEY_DOWN)  break;               //按键返回上一级
		if(USART2_RX_STA&0x8000)
		{  
			USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
			USART2_RX_STA=0;
			p1 =(u8*)strstr((const char*)USART2_RX_BUF,"DATA: ");
			if(p1!=NULL)
			{
				p2 = (u8*)strstr((const char *)p1,"\x0d\x0a"); 
				if(p2!= NULL) 
				{
					p2 =(u8*)strstr((const char *)p1,",");
					p1 =(u8*)strstr((const char *)p2+1,",");
				// printf("接收到的数据是：");
				// printf("%s\r\n",p1+1);                                //打印到串口
					LCD_Fill(90,200,320,480,WHITE);                       //清除显示
					LCD_ShowString(90,200,150,119,16,(u8*)(p1+1));        //显示接收到的数据
		
				}
			}
			else 
			{
				p1 =(u8*)strstr((const char*)USART2_RX_BUF,"+BTDISCONN: ");//判断是否断开连接
				if(p1!=NULL)
				{
					LCD_ShowFont12Char(40,30+60,"蓝牙连接断开         ");  
					LCD_Fill(10,140,240,320,WHITE);                       //清屏
					delay_ms(1000);
					delay_ms(1000);
					delay_ms(1000);
					break;                                                //退出
				}						 
			}
		}
		timex++;
		if(timex%50==0)
		{  
			timex=0;
			sim800c_send_cmd("AT+BTSPPSEND",">",100);                    //发送数据
			sprintf((char*)sendbuf,"Bluetooth test %d \r\n\32",sendcnt);
			sendcnt++;
			if(sendcnt>99) sendcnt = 0;
			res = sim800c_send_cmd((u8*)sendbuf,"OK",100);               //发送数据
			if(res==0)
			{
				LCD_ShowString(90,140,209,119,16,(u8*)sendbuf);            //显示发送的数据
				led1=!led1;
			}
			else
			{
				LCD_ShowFont12Char(40,30+60,"蓝牙连接断开         ");
				LCD_Fill(10,140,240,320,WHITE);                        //清屏
				delay_ms(1000);
				delay_ms(1000);
				delay_ms(1000);
				break;                                                //退出
			}
		}
		delay_ms(10);	 
	}
	return 0;
}

//SIM800C蓝牙连接模式选择
//返回值:0,正常
//     其他,错误代码
u8 sim800c_spp_test(void)
{
	 u8 key;
	 u8 timex=0;
	 if(sim800c_send_cmd("ATE1","OK",200))          //打开回显失败
	 {
		 //printf("打开回显失败");
		  return 1;
	 }
	 delay_ms(10);
	 if(sim800c_send_cmd("AT+BTPOWER=1","AT",300))	//打开蓝牙电源 不判断OK，因为电源原本开启再发送打开的话会返回error
     {
		  sim800c_send_cmd("ATE0","OK",200);        //关闭回显功能
	      return 1;
	 }
	 delay_ms(10);
	 sim800c_spp_ui(40,30);
	 while(1)
	 {
		  key=KEY_Scan(0);
		  if(key==KEY_DOWN)
          {
			  sim800c_send_cmd("ATE0","OK",300);         //关闭回显功能
   			  break;
		  }
		  else if(key==K_LEFT)
		  {  			 
			  sim800c_spp_mode(1);//寻找设备模式
			  sim800c_spp_ui(40,30);
			  sim800c_send_cmd("AT+BTUNPAIR=0","AT",120);//删除配对信息				 
			  timex=0;
		  }
		  else if(key==K_RIGHT)
		  {
			  sim800c_spp_mode(0);//接收配对模式
			  sim800c_spp_ui(40,30);
			  sim800c_send_cmd("AT+BTUNPAIR=0","AT",120);//删除配对信息				 
			  timex=0;  
		  }
		  timex++;
		  if(timex==20)
		  {
			  timex=0;
			  led1=!led1;
		  }
		  delay_ms(10);
		  //sim_at_response(1);                          //检查GSM模块发送过来的数据,及时上传给电脑
			
	 }
	 return 0;
}


//SIM800C主测试程序
void sim800c_test(void)
{
	u8 key=0; 
	u8 timex=0;
	u8 sim_ready=0;
	FRONT_COLOR=RED;
	LCD_ShowFont12Char(10,30,"PZ-SIM800C GSM/GPRS测试程序");
	while(sim800c_send_cmd("AT","OK",100))//检测是否应答AT指令 
	{
		LCD_ShowFont12Char(40,55,"未检测到模块!!!");
		delay_ms(800);
		LCD_Fill(40,55,200,55+16,WHITE);
		LCD_ShowFont12Char(40,55,"尝试连接模块...");
		delay_ms(400);  
	} 	 
	LCD_Fill(40,55,200,55+16,WHITE);
	key+=sim800c_send_cmd("ATE0","OK",200);//不回显
	sim800c_mtest_ui(40,20);
	ntp_update();//网络同步时间
	while(1)
	{
		delay_ms(10); 
		sim_at_response(1);//检查GSM模块发送过来的数据,及时上传给电脑
		if(sim_ready)//SIM卡就绪.
		{
			key=KEY_Scan(0); 
			if(key)
			{
				switch(key)
				{
					case KEY_RIGHT:
						sim800c_call_test();//拨号测试
						break;
					case KEY_DOWN:
						sim800c_sms_test();	//短信测试
						break;
					case KEY_UP:
						sim800c_gprs_test();//GPRS测试
						break;
					case KEY_LEFT:
						sim800c_spp_test();//蓝牙spp测试
					  break;
						
				}
				sim800c_mtest_ui(40,30);
				timex=0;
			} 			
		}
		if(timex==0)		//2.5秒左右更新一次
		{
			if(sim800c_gsminfo_show(40,225)==0)sim_ready=1;
			else sim_ready=0;
		}	
		if((timex%20)==0)led1=!led1;//200ms闪烁 
		timex++;	 
	} 	
}



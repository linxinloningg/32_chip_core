#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "key.h"
#include "usart.h"
#include "tftlcd.h"

#include "rtc.h" 
#include "adc.h"
#include "adc_temp.h"
#include "enc28j60.h"
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "timer.h"				   
#include "math.h" 	
#include "string.h"


void uip_polling(void);	 												
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])	 		  


//数字->字符串转换函数
//将num数字(位数为len)转为字符串,存放在buf里面
//num:数字,整形
//buf:字符串缓存
//len:长度
void num2str(u16 num,u8 *buf,u8 len)
{
	u8 i;
	for(i=0;i<len;i++)
	{
		buf[i]=(num/LCD_Pow(10,len-i-1))%10+'0';
	}
}
//获取STM32内部温度传感器的温度
//temp:存放温度字符串的首地址.如"28.3";
//temp,最少得有5个字节的空间!
void get_temperature(u8 *temp)
{			  
	u16 t;
	float temperate;		   
	temperate=Get_ADC_Temp_Value(ADC_Channel_16,10);			 
	temperate=temperate*(3.3/4096);			    											    
	temperate=(1.43-temperate)/0.0043+25;	//计算出当前温度值
	t=temperate*10;//得到温度
	num2str(t/10,temp,2);							   
	temp[2]='.';temp[3]=t%10+'0';temp[4]=0;	//最后添加结束符
}
//获取RTC时间
//time:存放时间字符串,形如:"2012-09-27 12:33"
//time,最少得有17个字节的空间!
void get_time(u8 *time)
{	
	RTC_Get();
	time[4]='-';time[7]='-';time[10]=' ';
	time[13]=':';time[16]=0;			//最后添加结束符
	num2str(calendar.w_year,time,4);	//年份->字符串
	num2str(calendar.w_month,time+5,2); //月份->字符串	 
	num2str(calendar.w_date,time+8,2); 	//日期->字符串
	num2str(calendar.hour,time+11,2); 	//小时->字符串
	num2str(calendar.min,time+14,2); 	//分钟->字符串								   	  									  
}

int main()
{
	u8 key;
	u8 tcnt=0;
	u8 tcp_server_tsta=0XFF;
	u8 tcp_client_tsta=0XFF;
	uip_ipaddr_t ipaddr;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	KEY_Init();				
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	
	FRONT_COLOR=RED;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ENC28J60 Ethernet Test!");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.com");
	
	RTC_Init();
	ADCx_Init();	
	ADC_Temp_Init();
	while(tapdev_init())	//初始化ENC28J60错误
	{								   
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"ENC28J60 Init Error!");	 
		printf("ENC28J60 Init Error!\r\n");
	}
	led1=0;
	led2=0;
	printf("ENC28J60 Init OK!\r\n");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"                    ");
	uip_init();				//uIP初始化
	  	   						  	 
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_RIGHT:Server Send Msg");
	LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"K_LEFT:Client Send Msg");
	LCD_ShowString(10,110,tftlcd_data.width,tftlcd_data.height,16,"IP:192.168.1.16");
	LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,16,"MASK:255.255.255.0");
	LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"GATEWAY:192.168.1.1");
	
	LCD_ShowString(10,200,tftlcd_data.width,tftlcd_data.height,16,"TCP RX:");	   						  	 
	LCD_ShowString(10,220,tftlcd_data.width,tftlcd_data.height,16,"TCP TX:");	
	   						  	   						  	 
	LCD_ShowString(10,270,tftlcd_data.width,tftlcd_data.height,16,"TCP RX:");	   						  	 
	LCD_ShowString(10,290,tftlcd_data.width,tftlcd_data.height,16,"TCP TX:");
	
	FRONT_COLOR=BLUE;
	uip_ipaddr(ipaddr, 192,168,1,16);	//设置本地设置IP地址
	uip_sethostaddr(ipaddr);					    
	uip_ipaddr(ipaddr, 192,168,1,1); 	//设置网关IP地址(其实就是你路由器的IP地址)
	uip_setdraddr(ipaddr);						 
	uip_ipaddr(ipaddr, 255,255,255,0);	//设置网络掩码
	uip_setnetmask(ipaddr);

	uip_listen(HTONS(1200));			//监听1200端口,用于TCP Server
	uip_listen(HTONS(80));				//监听80端口,用于Web Server
  	tcp_client_reconnect();	   		    //尝试连接到TCP Server端,用于TCP Client
	
	while (1)
	{
		uip_polling();	//处理uip事件，必须插入到用户程序的循环体中 

		key=KEY_Scan(0);
		if(tcp_server_tsta!=tcp_server_sta)//TCP Server状态改变
		{															 
			if(tcp_server_sta&(1<<7))LCD_ShowString(10,180,200,16,16,"TCP Server Connected   ");
			else LCD_ShowString(10,180,200,16,16,"TCP Server Disconnected");
 			if(tcp_server_sta&(1<<6))	//收到新数据
			{
				LCD_Fill(86,200,240,216,WHITE);	//清除之前显示
				LCD_ShowString(86,200,154,16,16,tcp_server_databuf);
    			printf("TCP Server RX:%s\r\n",tcp_server_databuf);//打印数据
				tcp_server_sta&=~(1<<6);		//标记数据已经被处理	
			}
			tcp_server_tsta=tcp_server_sta;
		}
		if(key==KEY_RIGHT)//TCP Server 请求发送数据
		{
			if(tcp_server_sta&(1<<7))	//连接还存在
			{
				sprintf((char*)tcp_server_databuf,"TCP Server OK %d\r\n",tcnt);	 
				LCD_Fill(86,220,240,236,WHITE);		//清除之前显示
				LCD_ShowString(86,220,154,16,16,tcp_server_databuf);//显示当前发送数据
				tcp_server_sta|=1<<5;//标记有数据需要发送
				tcnt++;
			}
		}
		if(tcp_client_tsta!=tcp_client_sta)//TCP Client状态改变
		{															 
			if(tcp_client_sta&(1<<7))LCD_ShowString(10,250,200,16,16,"TCP Client Connected   ");
			else LCD_ShowString(10,250,200,16,16,"TCP Client Disconnected");
 			if(tcp_client_sta&(1<<6))	//收到新数据
			{
				LCD_Fill(86,270,240,286,WHITE);	//清除之前显示
				LCD_ShowString(86,270,154,16,16,tcp_client_databuf);
    			printf("TCP Client RX:%s\r\n",tcp_client_databuf);//打印数据
				tcp_client_sta&=~(1<<6);		//标记数据已经被处理	
			}
			tcp_client_tsta=tcp_client_sta;
		}
		if(key==KEY_LEFT)//TCP Client 请求发送数据
		{
			if(tcp_client_sta&(1<<7))	//连接还存在
			{
				sprintf((char*)tcp_client_databuf,"TCP Client OK %d\r\n",tcnt);	 
				LCD_Fill(86,290,240,306,WHITE);		//清除之前显示
				LCD_ShowString(86,290,154,16,16,tcp_client_databuf);//显示当前发送数据
				tcp_client_sta|=1<<5;//标记有数据需要发送
				tcnt++;
			}
		}
		
		delay_ms(1);   
	}  
}

//uip事件处理函数
//必须将该函数插入用户主循环,循环调用.

void uip_polling(void)
{
	u8 i;
	static struct timer periodic_timer, arp_timer;
	static u8 timer_ok=0;	 
	if(timer_ok==0)//仅初始化一次
	{
		timer_ok = 1;
		timer_set(&periodic_timer,CLOCK_SECOND/2);  //创建1个0.5秒的定时器 
		timer_set(&arp_timer,CLOCK_SECOND*10);	   	//创建1个10秒的定时器 
	}				 
	uip_len=tapdev_read();	//从网络设备读取一个IP包,得到数据长度.uip_len在uip.c中定义
	if(uip_len>0) 			//有数据
	{   
		//处理IP数据包(只有校验通过的IP包才会被接收) 
		if(BUF->type == htons(UIP_ETHTYPE_IP))//是否是IP包? 
		{
			uip_arp_ipin();	//去除以太网头结构，更新ARP表
			uip_input();   	//IP包处理
			//当上面的函数执行后，如果需要发送数据，则全局变量 uip_len > 0
			//需要发送的数据在uip_buf, 长度是uip_len  (这是2个全局变量)		    
			if(uip_len>0)//需要回应数据
			{
				uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();//发送数据到以太网
			}
		}else if (BUF->type==htons(UIP_ETHTYPE_ARP))//处理arp报文,是否是ARP请求包?
		{
			uip_arp_arpin();
 			//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len(这是2个全局变量)
 			if(uip_len>0)tapdev_send();//需要发送数据,则通过tapdev_send发送	 
		}
	}else if(timer_expired(&periodic_timer))	//0.5秒定时器超时
	{
		timer_reset(&periodic_timer);		//复位0.5秒定时器 
		//轮流处理每个TCP连接, UIP_CONNS缺省是40个  
		for(i=0;i<UIP_CONNS;i++)
		{
			uip_periodic(i);	//处理TCP通信事件  
	 		//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
	 		if(uip_len>0)
			{
				uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();//发送数据到以太网
			}
		}
#if UIP_UDP	//UIP_UDP 
		//轮流处理每个UDP连接, UIP_UDP_CONNS缺省是10个
		for(i=0;i<UIP_UDP_CONNS;i++)
		{
			uip_udp_periodic(i);	//处理UDP通信事件
	 		//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
			if(uip_len > 0)
			{
				uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();//发送数据到以太网
			}
		}
#endif 
		//每隔10秒调用1次ARP定时器函数 用于定期ARP处理,ARP表10秒更新一次，旧的条目会被抛弃
		if(timer_expired(&arp_timer))
		{
			timer_reset(&arp_timer);
			uip_arp_timer();
		}
	}
}

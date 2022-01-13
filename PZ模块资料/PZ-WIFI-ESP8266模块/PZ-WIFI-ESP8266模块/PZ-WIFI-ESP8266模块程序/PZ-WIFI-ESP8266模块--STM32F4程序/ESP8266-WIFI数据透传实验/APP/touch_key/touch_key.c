#include "touch_key.h"
#include "SysTick.h"
#include "usart.h"

#define Touch_ARR_MAX_VAL 0xffffffff  //最大的ARR值(TIM2是32位定时器)	
u16 touch_default_val=0;  //为按下触摸按键时的值


void TIM2_CH1_Input_Init(u32 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//使能TIM2时钟
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_TIM2);//管脚复用
	
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5;//管脚设置
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF; //复用输出模式
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;//速度为100M
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;//浮空
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化结构体
	
	
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);	
	
	TIM_ICInitStructure.TIM_Channel=TIM_Channel_1; //通道1
	TIM_ICInitStructure.TIM_ICFilter=0x00;  //滤波
	TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;//捕获极性
	TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1; //分频系数
	TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;//直接映射到TI1
	TIM_ICInit(TIM2,&TIM_ICInitStructure);
		
	TIM_Cmd(TIM2,ENABLE); //使能定时器
}

//触摸按键复位 先放电然后充电并释放计时器内的值
void Touch_Reset(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  //PA5
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //下拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA5
	 
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);//输出0,放电

	delay_ms(5);
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1|TIM_IT_Update); //清除中断标志
	TIM_SetCounter(TIM2,0);		//归0
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PA5
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//不带上下拉 
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA5
}

//返回捕获高电平值
u16 Touch_Get_Val(void)
{
	Touch_Reset();
	while(TIM_GetFlagStatus(TIM2,TIM_FLAG_CC1)==0) //等待捕获到高电平标志
	{
		if(TIM_GetCounter(TIM2)>Touch_ARR_MAX_VAL-500)  //超时了直接返回CNT值
		{
			return TIM_GetCounter(TIM2);
		}
	}
	return TIM_GetCapture1(TIM2); //返回捕获高电平值
}

//触摸按键初始化
u8 Touch_Key_Init(u8 psc)  
{
	u8 i;
	u16 buf[10];
	u8 j;
	u16 temp;
	TIM2_CH1_Input_Init(Touch_ARR_MAX_VAL,psc);
	
	for(i=0;i<10;i++) //读取10次为按下时候的触摸值
	{
		buf[i]=Touch_Get_Val();
		delay_ms(10);
	}
	
	for(i=0;i<9;i++)   //从小到大排序
	{
		for(j=i+1;j<10;j++)
		{
			if(buf[i]>buf[j])
			{
				temp=buf[i];
				buf[j]=buf[j];
				buf[j]=temp;
			}
		}
	}
	
	temp=0;
	for(i=2;i<8;i++)  //取中间6个数值求和 取其平均数
	{
		temp+=buf[i];
	}
	touch_default_val=temp/6;
	printf("touch_default_val=%d \r\n",touch_default_val);
	if(touch_default_val>Touch_ARR_MAX_VAL/2)
	{
		return 1;//初始化遇到超过Touch_ARR_MAX_VAL/2的数值,不正常!
	}
	return 0;
}

//读取n次,取最大值
//n：连续获取的次数
//返回值：n次读数里面读到的最大读数值
u16 Touch_Get_MaxVal(u8 n)
{
	u16 temp=0;
	u16 res=0; 
	while(n--)
	{
		temp=Touch_Get_Val();//得到一次值
		if(temp>res)res=temp;
	}
	return res;
}  

//扫描触摸按键
//mode:0,不支持连续触发(按下一次必须松开才能按下一次);1,支持连续触发(可以一直按下)
//返回值:0,没有按下;1,有按下;										  
#define TOUCH_GATE_VAL 	100	//触摸的门限值,也就是必须大于tpad_default_val+TOUCH_GATE_VAL,才认为是有效触摸.
u8 Touch_Key_Scan(u8 mode)
{
	static u8 keyen=0;	//0,可以开始检测;>0,还不能开始检测	 
	u8 res=0;
	u8 sample=3;		//默认采样次数为3次	 
	u16 rval;
	if(mode)
	{
		sample=6;	//支持连按的时候，设置采样次数为6次
		keyen=0;	//支持连按	  
	}
	rval=Touch_Get_MaxVal(sample); 
	if(rval>(touch_default_val+TOUCH_GATE_VAL)&&rval<(10*touch_default_val))//大于touch_default_val+TPAD_GATE_VAL,且小于10倍touch_default_val,则有效
	{							 
		if((keyen==0)&&(rval>(touch_default_val+TOUCH_GATE_VAL)))	//大于touch_default_val+TOUCH_GATE_VAL,有效
		{
			res=1;
		}	   
		printf("触摸后捕获高电平值为：%d\r\n",rval);		     	    					   
		keyen=3;				//至少要再过3次之后才能按键有效   
	} 
	if(keyen)keyen--;		   							   		     	    					   
	return res;
}	 

#include "system.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "includes.h"



//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		512
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);


//任务优先级
#define LED1_TASK_PRIO		4
//任务堆栈大小	
#define LED1_STK_SIZE 		128
//任务控制块
OS_TCB Led1TaskTCB;
//任务堆栈	
CPU_STK LED1_TASK_STK[LED1_STK_SIZE];
void led1_task(void *p_arg);


//任务优先级
#define LED2_TASK_PRIO		4
//任务堆栈大小	
#define LED2_STK_SIZE 		512
//任务控制块
OS_TCB Led2TaskTCB;
//任务堆栈	
CPU_STK LED2_TASK_STK[LED2_STK_SIZE];
void led2_task(void *p_arg);

//任务优先级
#define LED3_TASK_PRIO		4
//任务堆栈大小	
#define LED3_STK_SIZE 		512
//任务控制块
OS_TCB Led3TaskTCB;
//任务堆栈	
CPU_STK LED3_TASK_STK[LED3_STK_SIZE];
void led3_task(void *p_arg);


int main()
{  	
	OS_ERR err;
	
	LED_Init();
	KEY_Init();
	USART1_Init(9600);
	
	OSInit(&err);		//初始化UCOSIII
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值 
	OSStart(&err);  //开启UCOSIII
	while(1);
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	
	CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
	
	cpu_clk_freq = BSP_CPU_ClkFreq();                           /* Determine SysTick reference freq.                    */
    cnts = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;        /* Determine nbr SysTick increments                     */
    OS_CPU_SysTickInit(cnts);                                   /* Init uC/OS periodic time src (SysTick).              */

    Mem_Init();       
	
	
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//进入临界区
	
	//创建LED1任务
	OSTaskCreate((OS_TCB 	* )&Led1TaskTCB,		
				 (CPU_CHAR	* )"led1 task", 		
                 (OS_TASK_PTR )led1_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED1_TASK_PRIO,     
                 (CPU_STK   * )&LED1_TASK_STK[0],	
                 (CPU_STK_SIZE)LED1_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED1_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
				 
	//创建任务
	OSTaskCreate((OS_TCB 	* )&Led2TaskTCB,		
				 (CPU_CHAR	* )"led2 task", 		
                 (OS_TASK_PTR )led2_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED2_TASK_PRIO,     
                 (CPU_STK   * )&LED2_TASK_STK[0],	
                 (CPU_STK_SIZE)LED2_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED2_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
				 
	//创建任务
	OSTaskCreate((OS_TCB 	* )&Led3TaskTCB,		
				 (CPU_CHAR	* )"led3 task", 		
                 (OS_TASK_PTR )led3_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED3_TASK_PRIO,     
                 (CPU_STK   * )&LED3_TASK_STK[0],	
                 (CPU_STK_SIZE)LED3_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED3_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
				 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//退出临界区
}

//led1任务函数
void led1_task(void *p_arg)
{
	OS_ERR err;
	OS_REG      value;
	p_arg = p_arg;
	while(1)
	{
		led1=!led1;
		
		value = OSTaskRegGet ( 0, 0, & err );              //获取自身任务寄存器值
		
		if ( value < 10 )                                  //如果任务寄存器值<10
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );          //继续累加任务寄存器值
		}		
		else                                               //如果累加到10
		{
			OSTaskRegSet ( 0, 0, 0, & err );                 //将任务寄存器值归0
			
			OSTaskResume ( & Led2TaskTCB, & err );        //恢复 LED1 任务
			 
			OSTaskResume ( & Led3TaskTCB, & err );        //恢复 LED2 任务
			
		}
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_DLY,&err); //延时1s
	}
}

//任务处理函数
void led2_task(void *p_arg)
{
	OS_ERR err;
	OS_REG      value;
	
	p_arg = p_arg;
		
	while(1)
	{
		led2=!led2;
		value = OSTaskRegGet ( 0, 0, & err );            //获取自身任务寄存器值
		
		if ( value < 5 )                                 //如果任务寄存器值<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //继续累加任务寄存器值
		}
		else                                             //如果累加到5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //将任务寄存器值归0
			
			OSTaskSuspend ( 0, & err );                    //挂起自身
			
		}
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_DLY,&err);
		
	}
}


//任务处理函数
void led3_task(void *p_arg)
{
	OS_ERR err;
	OS_REG      value;

	p_arg = p_arg;
				
	while(1)
	{
		led3=!led3;
		
		value = OSTaskRegGet ( 0, 0, & err );            //获取自身任务寄存器值
		
		if ( value < 5 )                                 //如果任务寄存器值<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //继续累加任务寄存器值
		}
		else                                             //如果累加到5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //将任务寄存器值归零
			
			OSTaskSuspend ( 0, & err );                    //挂起自身
			
		}
		
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_DLY,&err);	                  
		
	}
}


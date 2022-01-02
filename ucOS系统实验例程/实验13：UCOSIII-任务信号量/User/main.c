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
#define TASKSEMPOST_TASK_PRIO		6
//任务堆栈大小	
#define TASKSEMPOST_STK_SIZE 		128
//任务控制块
OS_TCB TASKSEMPOSTTaskTCB;
//任务堆栈	
CPU_STK TASKSEMPOST_TASK_STK[TASKSEMPOST_STK_SIZE];
void TaskSemPost_task(void *p_arg);


//任务优先级
#define TASKSEMPEND_TASK_PRIO		7
//任务堆栈大小	
#define TASKSEMPEND_STK_SIZE 		128
//任务控制块
OS_TCB TASKSEMPENDTaskTCB;
//任务堆栈	
CPU_STK TASKSEMPEND_TASK_STK[TASKSEMPEND_STK_SIZE];
void TaskSemPend_task(void *p_arg);

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
	OSTaskCreate((OS_TCB 	* )&TASKSEMPOSTTaskTCB,		
				 (CPU_CHAR	* )"TaskSemPost_task", 		
                 (OS_TASK_PTR )TaskSemPost_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )TASKSEMPOST_TASK_PRIO,     
                 (CPU_STK   * )&TASKSEMPOST_TASK_STK[0],	
                 (CPU_STK_SIZE)TASKSEMPOST_STK_SIZE/10,	
                 (CPU_STK_SIZE)TASKSEMPOST_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
		
	//创建任务
	OSTaskCreate((OS_TCB 	* )&TASKSEMPENDTaskTCB,		
				 (CPU_CHAR	* )"TaskSemPend_task", 		
                 (OS_TASK_PTR )TaskSemPend_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )TASKSEMPEND_TASK_PRIO,     
                 (CPU_STK   * )&TASKSEMPEND_TASK_STK[0],	
                 (CPU_STK_SIZE)TASKSEMPEND_STK_SIZE/10,	
                 (CPU_STK_SIZE)TASKSEMPEND_STK_SIZE,		
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
	p_arg = p_arg;
	while(1)
	{
		led1=!led1;
		OSTimeDlyHMSM(0,0,0,200,OS_OPT_TIME_DLY,&err); //延时200ms
	}
}

//任务处理函数
void TaskSemPost_task(void *p_arg)
{
	OS_ERR err;

	p_arg = p_arg;

			
	while(1)
	{
		
		if(KEY_Scan(0)==KEY_UP)
		{
			/* 发布任务信号量 */
			OSTaskSemPost((OS_TCB  *)&TASKSEMPENDTaskTCB,           //目标任务
							(OS_OPT   )OS_OPT_POST_NONE,        //没选项要求
							(OS_ERR  *)&err);
		}
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_DLY,&err);
		
	}
}


//任务处理函数
void TaskSemPend_task(void *p_arg)
{
	OS_ERR err;
	CPU_TS         ts;
	CPU_INT32U     cpu_clk_freq;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;
	
	cpu_clk_freq = BSP_CPU_ClkFreq();        //获取CPU时钟，时间戳是以该时钟计数
				
	while(1)
	{
		//阻塞任务，直到K_UP被单击 
		OSTaskSemPend ((OS_TICK   )0,                     //无期限等待
						(OS_OPT    )OS_OPT_PEND_BLOCKING,  //如果信号量不可用就等待
						(CPU_TS   *)&ts,                   //获取信号量被发布的时间戳
						(OS_ERR   *)&err);                 //返回错误类型
		
		ts = OS_TS_GET() - ts;                //计算信号量从发布到接收的时间差
		led1=!led1;
		
		OS_CRITICAL_ENTER();	//进入临界区
		printf("任务信号量从被发布到被接收的时间差是%d us\r\n",ts /(cpu_clk_freq/1000000));
		OS_CRITICAL_EXIT();	//退出临界区
	
	}
}





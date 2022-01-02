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
#define MULTIPOST_TASK_PRIO		6
//任务堆栈大小	
#define MULTIPOST_STK_SIZE 		128
//任务控制块
OS_TCB MULTIPOSTTaskTCB;
//任务堆栈	
CPU_STK MULTIPOST_TASK_STK[MULTIPOST_STK_SIZE];
void MultiPost_task(void *p_arg);


//任务优先级
#define MULTIPEND_TASK_PRIO		7
//任务堆栈大小	
#define MULTIPEND_STK_SIZE 		128
//任务控制块
OS_TCB MULTIPENDTaskTCB;
//任务堆栈	
CPU_STK MULTIPEND_TASK_STK[MULTIPEND_STK_SIZE];
void MultiPend_task(void *p_arg);


OS_SEM my_sem;
OS_Q my_q;
OS_PEND_DATA multi_pend_data[2];

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
	
	OSSemCreate(&my_sem,"my_sem",0,&err);  
	
	OSQCreate(&my_q,"my_q",10,&err);
	
	//初始化要等待的多个内核对象
	multi_pend_data[0].PendObjPtr=(OS_PEND_OBJ *)&my_sem;
	multi_pend_data[1].PendObjPtr=(OS_PEND_OBJ *)&my_q;
	
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
	OSTaskCreate((OS_TCB 	* )&MULTIPOSTTaskTCB,		
				 (CPU_CHAR	* )"MultiPost_task", 		
                 (OS_TASK_PTR )MultiPost_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MULTIPOST_TASK_PRIO,     
                 (CPU_STK   * )&MULTIPOST_TASK_STK[0],	
                 (CPU_STK_SIZE)MULTIPOST_STK_SIZE/10,	
                 (CPU_STK_SIZE)MULTIPOST_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
		
	//创建任务
	OSTaskCreate((OS_TCB 	* )&MULTIPENDTaskTCB,		
				 (CPU_CHAR	* )"MultiPend_task", 		
                 (OS_TASK_PTR )MultiPend_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MULTIPEND_TASK_PRIO,     
                 (CPU_STK   * )&MULTIPEND_TASK_STK[0],	
                 (CPU_STK_SIZE)MULTIPEND_STK_SIZE/10,	
                 (CPU_STK_SIZE)MULTIPEND_STK_SIZE,		
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

void MyTmrCallback(OS_TMR *p_tmr,void *p_arg)
{
	OS_ERR err;
	
	if(KEY_Scan(0)==KEY_UP)
	{
		OS_SemPost(&my_sem,OS_OPT_POST_ALL,0,&err);
		
	}
}

//任务处理函数
void MultiPost_task(void *p_arg)
{
	OS_ERR err;

	OS_TMR my_tmr;

	p_arg = p_arg;
	
	OSTmrCreate(&my_tmr,"my_tmr",0,1,OS_OPT_TMR_PERIODIC,(void *)MyTmrCallback,"Time Over!",&err);
	OSTmrStart(&my_tmr,&err);
			
	while(1)
	{
		
		OSQPost(&my_q,
				"www.prechin.cn",
				sizeof("www.prechin.cn"),
				OS_OPT_POST_FIFO | OS_OPT_POST_ALL,
				&err
				);
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_DLY,&err);
		
	}
}


//任务处理函数
void MultiPend_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;
				
	while(1)
	{
		OSPendMulti(multi_pend_data,2,0,OS_OPT_PEND_BLOCKING,&err);
		
		if(multi_pend_data[0].RdyObjPtr==multi_pend_data[0].PendObjPtr)
		{
			OS_CRITICAL_ENTER();	//进入临界区
			printf("按键K_UP被按下，多值信号量被发送\r\n");
			OS_CRITICAL_EXIT();	//退出临界区
		}
		
		if(multi_pend_data[1].RdyObjPtr==multi_pend_data[1].PendObjPtr)
		{
			OS_CRITICAL_ENTER();	//进入临界区
			printf("接收到一条消息，消息内容为：%s,消息长度为：%d字节\r\n",multi_pend_data[1].RdyMsgPtr,multi_pend_data[1].RdyMsgSize);
			OS_CRITICAL_EXIT();	//退出临界区
		}
	
	}
}





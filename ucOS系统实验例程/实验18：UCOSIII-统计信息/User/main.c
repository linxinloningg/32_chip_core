#include "system.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "exti.h"
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
#define USART_TASK_PRIO		5
//任务堆栈大小	
#define USART_STK_SIZE 		512
//任务控制块
OS_TCB USARTTaskTCB;
//任务堆栈	
CPU_STK USART_TASK_STK[USART_STK_SIZE];
void Usart_task(void *p_arg);


//任务优先级
#define KEY_TASK_PRIO		5
//任务堆栈大小	
#define KEY_STK_SIZE 		512
//任务控制块
OS_TCB KEYTaskTCB;
//任务堆栈	
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
void Key_task(void *p_arg);



OS_MEM mymem;
u8 ucArray [ 70 ] [ 4 ];   //声明内存分区大小


int main()
{  	
	OS_ERR err;
	
	LED_Init();
	KEY_Init();
	My_EXTI_Init();
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
	
	//创建内存管理对象 mymem 
	OSMemCreate ((OS_MEM      *)&mymem,             //指向内存管理对象
				(CPU_CHAR    *)"mymem",   //命名内存管理对象
				(void        *)ucArray,          //内存分区的首地址
				(OS_MEM_QTY   )70,               //内存分区中内存块数目
				(OS_MEM_SIZE  )4,                //内存块的字节数目
				(OS_ERR      *)&err);            //返回错误类型		
				
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
	OSTaskCreate((OS_TCB 	* )&USARTTaskTCB,		
				 (CPU_CHAR	* )"Usart task", 		
                 (OS_TASK_PTR )Usart_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )USART_TASK_PRIO,     
                 (CPU_STK   * )&USART_TASK_STK[0],	
                 (CPU_STK_SIZE)USART_STK_SIZE/10,	
                 (CPU_STK_SIZE)USART_STK_SIZE,		
                 (OS_MSG_QTY  )50,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
		
	//创建任务
	OSTaskCreate((OS_TCB 	* )&KEYTaskTCB,		
				 (CPU_CHAR	* )"Key task", 		
                 (OS_TASK_PTR )Key_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )KEY_TASK_PRIO,     
                 (CPU_STK   * )&KEY_TASK_STK[0],	
                 (CPU_STK_SIZE)KEY_STK_SIZE/10,	
                 (CPU_STK_SIZE)KEY_STK_SIZE,		
                 (OS_MSG_QTY  )50,					
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
void Usart_task(void *p_arg)
{
	OS_ERR err;
	OS_MSG_SIZE    msg_size;
	char * pMsg;
	
	CPU_SR_ALLOC();
	
	p_arg = p_arg;
		
	while(1)
	{
		//阻塞任务，等待任务消息 */
		pMsg = OSTaskQPend ((OS_TICK        )0,                    //无期限等待
							(OS_OPT         )OS_OPT_PEND_BLOCKING, //没有消息就阻塞任务
							(OS_MSG_SIZE   *)&msg_size,            //返回消息长度
							(CPU_TS        *)0,                    //返回消息被发布的时间戳
							(OS_ERR        *)&err);                //返回错误类型

		OS_CRITICAL_ENTER();                //进入临界段，避免串口打印被打断

		printf ( "%c", * pMsg );            //打印消息内容

		OS_CRITICAL_EXIT();              //退出临界段
		
		//退还内存块
		OSMemPut ((OS_MEM  *)&mymem,              //指向内存管理对象
							(void    *)pMsg,    //内存块的首地址
							(OS_ERR  *)&err);	//返回错误类型
		
	
		
	}
}


//任务处理函数
void Key_task(void *p_arg)
{
	OS_ERR         err;
	CPU_TS_TMR     ts_int;
	CPU_INT16U     version;
	CPU_INT32U     cpu_clk_freq;
	CPU_SR_ALLOC();
	
	p_arg = p_arg;
	
	version = OSVersion(&err);            //获取uC/OS版本号
	
	cpu_clk_freq = BSP_CPU_ClkFreq();     //获取CPU时钟，时间戳是以该时钟计数
	
	while(1)
	{
		//阻塞任务，直到K_UP被单击
		OSTaskSemPend ((OS_TICK   )0,                     //无期限等待
						(OS_OPT    )OS_OPT_PEND_BLOCKING,  //如果信号量不可用就等待
						(CPU_TS   *)0,                     //获取信号量被发布的时间戳
						(OS_ERR   *)&err);                 //返回错误类型
		
		ts_int = CPU_IntDisMeasMaxGet ();                 //获取最大关中断时间

		OS_CRITICAL_ENTER();                              //进入临界段，避免串口打印被打断

		printf ( "\r\nuC/OS版本号：V%d.%02d.%02d\r\n",
				version / 10000, version % 10000 / 100, version % 100 );
    
		printf ( "CPU主频：%d MHz\r\n", cpu_clk_freq / 1000000 );  
		
		printf ( "最大中断时间：%d us\r\n", 
				ts_int / ( cpu_clk_freq / 1000000 ) ); 

		printf ( "最大锁调度器时间：%d us\r\n", 
		         OSSchedLockTimeMax / ( cpu_clk_freq / 1000000 ) );		

		printf ( "任务切换总次数：%d\r\n", OSTaskCtxSwCtr ); 	
		
		printf ( "CPU使用率：%d.%d%%\r\n",
				OSStatTaskCPUUsage / 100, OSStatTaskCPUUsage % 100 );  
		
		printf ( "CPU最大使用率：%d.%d%%\r\n", 
		         OSStatTaskCPUUsageMax / 100, OSStatTaskCPUUsageMax % 100 );

		printf ( "串口任务的CPU使用率：%d.%d%%\r\n", 
		         USARTTaskTCB.CPUUsage / 100, USARTTaskTCB.CPUUsage % 100 );
						 
		printf ( "串口任务的CPU最大使用率：%d.%d%%\r\n", 
		         USARTTaskTCB.CPUUsageMax / 100, USARTTaskTCB.CPUUsageMax % 100 ); 

		printf ( "按键任务的CPU使用率：%d.%d%%\r\n", 
		         KEYTaskTCB.CPUUsage / 100, KEYTaskTCB.CPUUsage % 100 );  
						 
		printf ( "按键任务的CPU最大使用率：%d.%d%%    \r\n", 
		         KEYTaskTCB.CPUUsageMax / 100, KEYTaskTCB.CPUUsageMax % 100 ); 
		
		printf ( "串口任务的已用和空闲堆栈大小分别为：%d,%d\r\n", 
		         USARTTaskTCB.StkUsed, USARTTaskTCB.StkFree ); 
		
		printf ( "按键任务的已用和空闲堆栈大小分别为：%d,%d\r\n", 
		         KEYTaskTCB.StkUsed, KEYTaskTCB.StkFree ); 
		
		
		OS_CRITICAL_EXIT();                               //退出临界段
		
	}
}


#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "time.h"
#include "key.h"
#include "touch.h"
#include "snake.h"
#include "rtc.h"
#include "stdlib.h"


#define SNAKE_Max_Long 50//蛇的最大长度 
u8 pause=0;
u8 start=0;

//蛇结构体
struct Snake
{
	s16 X[SNAKE_Max_Long];
	s16 Y[SNAKE_Max_Long];
	u8 Long;//蛇的长度
	u8 Life;//蛇的生命 0活着 1死亡
	u8 Direction;//蛇移动的方向
}snake;

//食物结构体
struct Food
{
	u8 X;//食物横坐标
	u8 Y;//食物纵坐标
	u8 Yes;//判断是否要出现食物的变量 0有食物 1需要出现食物
}food;

//游戏等级分数
struct Game
{
	u16 Score;//分数
	u8 Life;//游戏等级	
}game;

void touch(void)
{	
	key=KEY_Scan(0);	
	
	if(key==KEY_UP&&snake.Direction!=4)		//上
	{
		snake.Direction=3;
	} 
	if(key==KEY_DOWN&&snake.Direction!=3)	   //下
	{
		snake.Direction=4;
	} 
	if(key==KEY_LEFT&&snake.Direction!=1)	   //左
	{
		snake.Direction=2;
	} 
	if(key==KEY_RIGHT&&snake.Direction!=2)	  //右
	{
		snake.Direction=1;
	}
}
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)!= RESET)
	{
		touch();
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);	
	}
}  


//游戏结束
void gameover()
{
	start=0;//停止游戏
	Test_Show_CH_Font24(80,65,0,RED);		
	Test_Show_CH_Font24(104,65,1,RED);		
	Test_Show_CH_Font24(128,65,2,RED);		
	Test_Show_CH_Font24(152,65,3,RED);		
	Test_Show_CH_Font24(176,65,4,RED);		
	FRONT_COLOR=BLACK;
	BACK_COLOR=GRAY;
	LCD_ShowString(224,165,tftlcd_data.width,tftlcd_data.height,16,"0");//显示生命值	
}

//玩游戏
void play()
{
	u16 i,n;//i蛇的关节数 n用来判断食物和蛇的身体是否重合
	u8 life_buf[2];
	u8 socre_buf[4];
	snake.Long=2;//定义蛇的长度
	snake.Life=0;//蛇还活着
	snake.Direction=1;//蛇的起始方向定义为右
	game.Score=0;//分数为0
	game.Life=4;//蛇的生命值
	food.Yes=1;//出现新食物
	snake.X[0]=12;snake.Y[0]=24;
	snake.X[1]=12;snake.Y[1]=24;

	while(1)
	{
			if(food.Yes==1)//出现新的食物
			{
				while(1)
				{
						//在设定的区域内显示食物		
						//food.X=12+rand()%(240/12)*12;
						//food.Y=12+rand()%(160/12)*12;
						srand(calendar.sec);//添加随机种子 采用的RTC时钟
						food.X=12+rand()%(228/12)*12;
						food.Y=12+rand()%(148/12)*12;
						for(n=0;n<snake.Long;n++)
						{
							if(food.X==snake.X[n]&&food.Y==snake.Y[n])
								break;
						}
						if(n==snake.Long)
						food.Yes=0;	
						break;
					}
			}
				
				if(food.Yes==0)//有食物就要显示
				{	
					LCD_Fill(food.X,food.Y,food.X+10,food.Y+10,RED);
				}
				//取得需要重新画的蛇的节数
				for(i=snake.Long-1;i>0;i--)
				{
					snake.X[i]=snake.X[i-1];
					snake.Y[i]=snake.Y[i-1];
				}
				//通过按键来设置蛇的运动方向
				switch(snake.Direction)
				{
					case 1:snake.X[0]+=12;break;//向右运动
					case 2:snake.X[0]-=12;break;//向左运动
					case 3:snake.Y[0]-=12;break;//向上运动
					case 4:snake.Y[0]+=12;break;//向下运动
				}
				for(i=0;i<snake.Long;i++)//画出蛇	
				LCD_Fill(snake.X[i],snake.Y[i],snake.X[i]+10,snake.Y[i]+10,BLUE);//画蛇身体
				while(pause==1){};
				delay_ms(500);//延时
				LCD_Fill(snake.X[snake.Long-1],snake.Y[snake.Long-1],snake.X[snake.Long-1]+10,snake.Y[snake.Long-1]+10,GRAY);//消除蛇身		
						
					
				//判断是否撞墙
				if(snake.X[0]<0||snake.X[0]>240||snake.Y[0]<0||snake.Y[0]>150)
					snake.Life=1;//蛇死掉了
		
				//当蛇的身体超过3节后判断蛇自身的碰撞
				for(i=3;i<snake.Long;i++)
				{
					if(snake.X[i]==snake.X[0]&&snake.Y[i]==snake.Y[0])//自身的任一坐标值与蛇头坐标相等就认为是自身碰撞
						game.Life-=1;
				}
				if(snake.Life==1||game.Life==0)//以上两种判断以后如果蛇死掉了跳出内循环，重新开始
				{
					gameover();
					break;
				}	
				//判断蛇是否吃到了食物
				if(snake.X[0]==food.X&&snake.Y[0]==food.Y)
				{
					LCD_Fill(food.X,food.Y,food.X+10,food.Y+10,GRAY);//把吃到的食物消除
					if(!((snake.Long==SNAKE_Max_Long)&&(snake.Long==SNAKE_Max_Long)))
					snake.Long++;//蛇的身体长一节
					game.Score+=10;
					socre_buf[0]=game.Score/100+0x30;
					socre_buf[1]=game.Score%100/10+0x30;
					socre_buf[2]=game.Score%100%10+0x30;
					socre_buf[3]='\0';
					FRONT_COLOR=BLACK;
					BACK_COLOR=GRAY;
		
					LCD_ShowString(40,165,tftlcd_data.width,tftlcd_data.height,16,socre_buf);//显示成绩	
					food.Yes=1;//需要重新显示食物
				}
				life_buf[0]=game.Life%10+0x30;
				life_buf[1]='\0';
			
				LCD_ShowString(224,165,tftlcd_data.width,tftlcd_data.height,16,life_buf);//显示生命值	
		}	
}

int main()
{
	u8 i;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	
	TOUCH_Init();
	LCD_ShowPictureEx(0, 0, 240, 320); 
	while(TOUCH_Scan() == 0xff); //等待按下触摸
	TIM3_Init(50,7199);//启动定时器
	RTC_Init();
	LCD_Clear(GRAY);
	show();//画游戏界面
	play();//玩游戏
}

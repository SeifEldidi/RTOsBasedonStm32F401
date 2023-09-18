/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Seif ELdidi
 * @brief          : Main program body
 ******************************************************************************
 */
#include "core/CortexM4_core_Systick.h"
#include "core/CortexM4_core_NVIC.h"
#include "HAL/Hal_Usart.h"
#include "HAL/Hal_RCC.h"
#include <stdint.h>

#include "../RTOS/RTOS.h"

uint32_t * Ptr = NULL;
extern int32_t  IdleTaskTime;
extern uint32_t OS_TICK ;
volatile uint8_t ISRData ;
pQueue MyQueue = NULL;
pSemaphore SemaphoreO = NULL;
pEventGroup Event1 = NULL;
pOsTimer    Timer1 = NULL;
uint32_t Task1Counter   = 0;
uint32_t Task2Counter   = 0;
uint32_t Task3Counter   = 0;
uint32_t Task2Periodic  = 0;
uint32_t TaskRecieve    = 0;
TaskHandle_t Task1Handler = NULL;
TaskHandle_t Task3Handler = NULL;


void ISR_Task()
{
	ISRData = USART1->DR;
	OsQueueSendIsr(MyQueue,(void*)&ISRData);
}

void SystemInit()
{
	/*-----------84Mhz Clock Setting-----*/
	RCC_Config_t RCC_Config = {
		.MSTR_CLK_SRC = SW_CLK_PLLO,
		.PLL_SRC = PLL_HSE,
		.PLL_M = 25,
		.PLL_N = 336,
		.PLL_P = PLLP_4,
		.PLL_Q = PLLQ_7_S,
		.AHB_PRE = AHBDIV_1,
		.APB1_PRE = APB_AHB_2,
		.APB2_PRE = APB_AHB_1,
	};
	/* SystemClock Init */
	HAL_RCC_Init(&RCC_Config);
}

void PeriphInit()
{
	USART_Config Usart = {
			.Instance = USART1,
			.BaudRate = 115200,
			.Data_Size = USART_DATA_SIZE_8,
			.Mode = USART_TX_RX_MODE,
			.TX_RX_Interrupts = INT_RX,
			.Rx_Callback = ISR_Task,
			.No_StopBits = USART_STOPBITS_1,
			.Parity = USART_NPARITY,
	};
	GPIO_InitStruct GPIOX = {
			.Pin   = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2,
			.Speed = GPIO_Speed_25MHz,
			.mode  = GPIO_MODE_OUTPUT_PP,
			.pull  = GPIO_NOPULL,
	};
	HAL_GPIO_Init(GPIOA,&GPIOX);
	xHAL_UsartInit(&Usart);
}

void Timer1Callback()
{
	xHAL_UsartLogInfo(USART1, "TimerCallback .....! \n\r");
}


void Task1(void)
{
	while(1)
	{
		Task1Counter++;
		if(Task1Counter == 1)
			TimerStart(Timer1);
		EventGroupSetBits(Event1 ,(1<<4)|(1<<3));
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_0);
		//OsQueueRecieve(0,&TaskRecieve,OS_QUEUE_BLOCK);
		OsDelay(MsToTicks(125));
	}
}

void Task2()
{
	while(1)
	{
		Task2Counter++;
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
		OsSemaphoreObtain(SemaphoreO ,OS_SEMAPHORE_BLOCK );
		xHAL_UsartLogInfo(USART1, "Hello From Task 2.....! \n\r");
		OsSemaphoreRelease(SemaphoreO);
		OsDelay(MsToTicks(125));
	}
}

void Task3()
{
	uint32_t LTask3_Counter = 0;
	uint32_t LTask3_Counter1 = 0;
	uint8_t  Data;
	while(1)
	{
		LTask3_Counter1 ++;
		LTask3_Counter ++;
		Task3Counter++;
		/*----Release Mutex with Ownership---*/
		OsQueueRecieve(MyQueue,&Data,OS_QUEUE_BLOCK);
		if(Data =='a')
		{
			Data = '\0';
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_2);
		}
		//OsQueueSend(0,&Task1Counter,OS_QUEUE_BLOCK);
//		OsDelay(MsToTicks(1500));
	}
}

void Task4()
{
	//uint8_t Del_Flag = 0;
	uint8_t  Data = 'A';
	while(1)
	{
		//OsQueueSend(MyQueue,&Data,OS_QUEUE_BLOCK);
		EventGroupWaitBits(Event1,(1<<4)|(1<<2),EVENT_WAIT_OR,OS_EVENT_BLOCK);
//		if(Del_Flag == 0)
//		{
//			Del_Flag++;
//			OsKillTask(Task1Handler);
//		}
	}
}


int main(void)
{
	PeriphInit();
	OSKernelAddThread(Task1,1,1,OS_STACK_SIZE,&Task1Handler);
	OSKernelAddThread(Task2,2,1,OS_STACK_SIZE,NULL);
	OSKernelAddThread(Task3,3,1,OS_STACK_SIZE,&Task3Handler);
	OSKernelAddThread(Task4,4,1,OS_STACK_SIZE,NULL);
	Timer1 = TimerCreate(OS_TIMER_AUTOReload,MsToTicks(5000),Timer1Callback);
	SemaphoreO = OsSemaphoreCreateDyanmic(1,"UartSemaphore",NULL);
	MyQueue = QueueCreateDynamic(20,"Queue0",sizeof(int));
	Event1  = EventGroupCreate("Event1");
	OsKernelStart();
    /* Loop forever */
	while(1)
	{

	}
}

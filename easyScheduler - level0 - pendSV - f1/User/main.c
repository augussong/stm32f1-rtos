/**
  ******************************************************************************
  */ 
	
#include "stm32f10x.h"
#include "peripheral.h"

//ջ��С
//˼��1����Сջ�ռ�8*4Byte��ΪʲôOS_EXCEPT_STK_SIZE��17��
/*
	TASK_1_STK������������UINT32�ͣ�һ��Ԫ�ؾͿ��Ա���һ���Ĵ����������UCHAR����Ȼ��Ҫ32+1Byte��
	�����һ��Ԫ������ΪTask_Create()���õ�--ptr�Ĳ��������Ծ����ȼ�1�ٸ�ֵ���������β���ض����һ��0Ԫ��
	��������Ԫ����ʵҲ����������Ϊջ���������
*/
//˼��2��ΪʲôTASK_1_STK_SIZE��TASK_2_STK_SIZE��19������������˵��17��
/*
	TASK_2_STK_SIZE����һ����ʱ�����������ڲ�����һ����ʱ������
	������ǡ������ʱ�������л�������ʱ������ַ�ͱ�������ռ��ջ�ռ�
*/
//˼��3��ΪʲôTASK_1_STK_SIZE��Ϊ17��Ȼ����ȷ����������TASK_2_STK_SIZEΪ17ȴ���У�
/*
  ����������ã��������ڱ���ʱ��task2ջ��ַ������task1���棬task2ջ�ռ䲻���Ļ����������ջ��Ȼ��ѹ��task1ջ��Χ��
  ����Խ����ʵ��¹����쳣�������task1ջ�ռ䲻���Ļ�Ҳ��Խ�磬������Ӱ�쵽task2��ջ��
	���task1��ջ��ַǰ����ڴ�ռ�ǡ��û��ʹ�ã���Ȼ������������
*/
//˼��4������task1����ʱ��Ϊʲôtask2���ǳ����������ǵ����������printf�������ε���
/*
	��ԭ������ٽ��������⣬��תIO����ԭ�Ӳ�������whileѭ���´���ʻ��ڷ�ת���̽���һ���ʱ�����л���
	�ڷ�תIOǰ��ֱ�ر����жϺͿ������жϿ��Խ�����⣬���Խ�������ٽ��������
*/
#define OS_EXCEPT_STK_SIZE 17
#define TASK_1_STK_SIZE 19
#define TASK_2_STK_SIZE 19

//���Ͷ������ӳ��
typedef unsigned int OS_STK;
typedef unsigned int OS_U32;
typedef void (*OS_TASK)(void *);//����ָ��
typedef struct OS_TCB
{
  OS_STK *StkAddr;//��ջ����ַ��ָ��
}OS_TCB,*OS_TCBP;

OS_TCBP g_OS_Tcb_CurP;//��ǰ������ƿ�ָ�� 
OS_TCBP g_OS_Tcb_HighRdyP;//����������ƿ�ָ��

//�쳣ջ����ջ��ַ
static OS_STK OS_CPU_ExceptStk[OS_EXCEPT_STK_SIZE];
OS_STK *g_OS_CPU_ExceptStkBase;

//����ջ������ƿ�
static OS_STK TASK_1_STK[TASK_1_STK_SIZE];
static OS_STK TASK_2_STK[TASK_2_STK_SIZE];
static OS_TCB TCB_1;
static OS_TCB TCB_2;

//��ຯ������
extern void OSStart_Asm(void);//OS����
extern void OSCtxSw(void);//�������л�
extern OS_U32 OS_CPU_SR_Save(void);    //����CPU�Ĵ���
extern void OS_CPU_SR_Restore(OS_U32); //�ָ�CPU�Ĵ���

//�ٽ�������
#define OS_USE_CRITICAL OS_U32 cpu_sr; //�ñ������ڱ���PRIMASKֵ
#define OS_ENTER_CRITICAL()    \
  {                            \
    cpu_sr = OS_CPU_SR_Save(); \
  } 																	//�����ٽ�����ע�⣺�ô��벻��Ƕ��ʹ�ã�
#define OS_EXIT_CRITICAL()     \
  {                            \
    OS_CPU_SR_Restore(cpu_sr); \
  }                                   //�˳��ٽ���
	
#define OS_PendSV_Trigger() OSCtxSw() //����PendSV�жϣ������������л���
	
//��ʱ����
extern vu32 sysCnt;
void delay(u32 cnt)
{
	vu32 old;
	old=sysCnt;//���������Ҫ3��������ִ�У�����ԭ�Ӳ��������ܻᱻ��ϣ���˲���ȫ��˼����1����������ԭ�Ӳ����𣿣�
	while(1)
	{
		if(sysCnt-old<cnt)
			continue;
		else
			break;
	}
}

//����1
void task1(void* para) 
{ 
//	OS_USE_CRITICAL;
	para=para;
	while(1)
	{
//		OS_ENTER_CRITICAL();
		LED1_TOGGLE;
//    printf("123"); 
//		OS_EXIT_CRITICAL();
//		delay(200);
	}
}

//����2
void task2(void* para)
{
//	OS_USE_CRITICAL;
	para=para;
	while(1)
	{
//		OS_ENTER_CRITICAL();
		LED2_TOGGLE;
//    printf("abc");
//		OS_EXIT_CRITICAL();
		delay(250);
	}
}

//�������
void Task_Switch(void)
{
	//���ľ�����������ƿ�
  if(g_OS_Tcb_CurP == &TCB_1)
    g_OS_Tcb_HighRdyP=&TCB_2;
  else
    g_OS_Tcb_HighRdyP=&TCB_1;
	//�������л�
  OSCtxSw();
}

//�������񣨳�ʼ��ջ+��ʼ��������ƿ飩
void Task_Create(OS_TCB *tcb,OS_TASK task,OS_STK *stk)
{
    OS_STK  *p_stk;
    p_stk      = stk;
		//R13ΪSPָ�룬����Ҫ����
    *(--p_stk) = (OS_STK)0x01000000uL;								//xPSR����ʼֵ
    *(--p_stk) = (OS_STK)task;												// Entry Point����ڵ�ַ���������ַ
    *(--p_stk) = (OS_STK)0xFFFFFFFFuL;								// R14 (LR) (init value will cause fault if ever used)���洢���ص�ַ������Ӧ�÷��ء�
    *(--p_stk) = (OS_STK)0x12121212uL;								// R12�������ͨ�üĴ����������ʼ�����Ա�Ž��г�ʼ��ֻ�Ƿ���ʶ��
    *(--p_stk) = (OS_STK)0x03030303uL;								// R3
    *(--p_stk) = (OS_STK)0x02020202uL;								// R2
    *(--p_stk) = (OS_STK)0x01010101uL;								// R1
    *(--p_stk) = (OS_STK)0x00000000uL;								// R0
    
    *(--p_stk) = (OS_STK)0x11111111uL;								// R11
    *(--p_stk) = (OS_STK)0x10101010uL;								// R10
    *(--p_stk) = (OS_STK)0x09090909uL;								// R9
    *(--p_stk) = (OS_STK)0x08080808uL;								// R8
    *(--p_stk) = (OS_STK)0x07070707uL;								// R7
    *(--p_stk) = (OS_STK)0x06060606uL;								// R6
    *(--p_stk) = (OS_STK)0x05050505uL;								// R5
    *(--p_stk) = (OS_STK)0x04040404uL;								// R4
    
    tcb->StkAddr=p_stk;//��ʼ����ջ����ַ��ֵ��������ƿ�
}

//Systick���Ķ�ʱ����ʼ��
void SysTick_Init(void)
{
	/* SystemFrequency / 1000    1ms�ж�һ��
	 * SystemFrequency / 100000	 10us�ж�һ��
	 */
	if (SysTick_Config(SystemCoreClock / 1000))	// ST3.5.0��汾
	{ 
		/* Capture error */ 
		while (1);
	}
	//����SysTick
	SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}


int main(void)
{
	//��ʼ��LED, USART1��SysTick
	LED_GPIO_Config();
	USART1_Config();
	SysTick_Init();   

	//ջ��ַָ��ջ�ף�ARM������������ջ������ջ���ڸߵ�ַ��
	g_OS_CPU_ExceptStkBase = OS_CPU_ExceptStk + OS_EXCEPT_STK_SIZE;

	//����2������
	Task_Create(&TCB_1,task1,&TASK_1_STK[TASK_1_STK_SIZE - 1]);
	Task_Create(&TCB_2,task2,&TASK_2_STK[TASK_2_STK_SIZE - 1]);
	
	//������1��Ϊ����̬
	g_OS_Tcb_HighRdyP=&TCB_1;

	//OS����
	OSStart_Asm(); 
		
	while(1)
	{}
}

/*********************************************END OF FILE**********************/

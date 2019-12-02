/**
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "peripheral.h"

//ջ��С
#define TASK_IDLE_STK_SIZE  32
#define OS_EXCEPT_STK_SIZE  32
#define TASK_1_STK_SIZE     32
#define TASK_2_STK_SIZE     32

#define OS_TASK_MAX_NUM         64       //���������
#define OS_TICKS_PER_SECOND     1000 //ϵͳ����Ƶ��
#define OS_RDY_TBL_SIZE         ((OS_TASK_MAX_NUM) / 8) //�������С
#define OS_TASK_IDLE_PRIO       OS_TASK_MAX_NUM - 1

//���Ͷ������ӳ��
typedef signed    char  OS_S8;
typedef signed    short OS_S16;
typedef signed    int   OS_S32;
typedef unsigned  char  OS_U8;
typedef unsigned  short OS_U16;
typedef unsigned  int   OS_U32;
typedef unsigned  int   OS_STK;
typedef void (*OS_TASK)(void *); //����ָ��

//����״̬ö��
typedef enum OS_TASK_STA
{
  TASK_READY,
  TASK_DELAY,
} OS_TASK_STA;

//������ƿ�
typedef struct OS_TCB
{
  OS_STK          *StkAddr;   //��ջָ�루ջ����ַ���ı���������ջָ�������
  OS_U32          TimeDly;    //��ʱ������
  OS_TASK_STA     State;      //����״̬
  OS_U8           OSTCBPrio;
  OS_U8           OSTCBX;     /* Bit position in group  corresponding to task priority   */
  OS_U8           OSTCBY;     /* Index into ready table corresponding to task priority   */
  OS_U8           OSTCBBitX;  /* Bit mask to access bit position in ready table          */
  OS_U8           OSTCBBitY;  /* Bit mask to access bit position in ready group          */
	struct OS_TCB   *OSTCBNext;
	struct OS_TCB   *OSTCBPrev;
} OS_TCB, *OS_TCBP;

//���ȼ�λͼ��
OS_U8 OSRdyGrp;
OS_U8 OSRdyTbl[OS_RDY_TBL_SIZE];

OS_U32  OS_TimeTick;                      //���ļ���
OS_U8   g_Prio_Cur;                       //��ǰ��ѡ����������ȼ����������е���������ȼ���
OS_U8   g_Prio_HighRdy;                   //���о�����������ߵ����ȼ�
OS_TCBP g_OS_Tcb_CurP;                    //��ǰ��ѡ��������ƿ�ָ�루�������е������������ƿ�ָ�룩
OS_TCBP g_OS_Tcb_HighRdyP;                //���о���������������ȼ���������ƿ�ָ��
OS_TCBP OS_TCB_TABLE[OS_TASK_MAX_NUM];    //������ƿ��
OS_TCBP OSTCBList = NULL;                 //TCB��ͷ

//�쳣ջ����ջ��ַ
static OS_STK OS_CPU_ExceptStk[OS_EXCEPT_STK_SIZE];
OS_STK *g_OS_CPU_ExceptStkBase;

//����ջ������ƿ�
static OS_STK TASK_IDLE_STK[TASK_IDLE_STK_SIZE];
static OS_STK TASK_1_STK[TASK_1_STK_SIZE];
static OS_STK TASK_2_STK[TASK_2_STK_SIZE];
static OS_TCB TCB_IDLE;
static OS_TCB TCB_1;
static OS_TCB TCB_2;

//��ຯ������
extern void OSStart_Asm(void);         //OS����
extern void OSCtxSw(void);             //�������л�
extern void   OS_CPU_SR_Restore(OS_U32);  //�ָ�CPU�Ĵ���
extern OS_U32 OS_CPU_SR_Save(void);       //����CPU�Ĵ���
extern OS_U8 	OS_GetHighRdyPrio(void);
	
#define OS_USE_CRITICAL OS_U32 cpu_sr; //�ñ������ڱ���PRIMASKֵ
#define OS_ENTER_CRITICAL()    \
  {                            \
    cpu_sr = OS_CPU_SR_Save(); \
  }                                   //�����ٽ�����ע�⣺�ô��벻��Ƕ��ʹ�ã�
#define OS_EXIT_CRITICAL()     \
  {                            \
    OS_CPU_SR_Restore(cpu_sr); \
  }                                   //�˳��ٽ���
	
#define OS_PendSV_Trigger() OSCtxSw() //����PendSV�жϣ������������л���

//�����л���ʹ�����ȼ�λͼ�����������ȼ�����
//ע�⣺ֻ���л������������е�������ȼ����񣬻�û�����������л�
//���⣺���жϺ��ж�Ƕ���У��Ƿ���Ҫͳ��Ƕ�ײ���
void OS_Task_Switch(void)
{
  OS_S32 i;
	OS_U8 highRdyPrio;
  OS_USE_CRITICAL

  OS_ENTER_CRITICAL(); 
  highRdyPrio = OS_GetHighRdyPrio();// ������ȼ���ߵ�����
  g_OS_Tcb_HighRdyP = OS_TCB_TABLE[highRdyPrio];
  g_Prio_HighRdy = i;
  OS_EXIT_CRITICAL();
}

//��ʱ�������������趨��ʱ���ġ��������񡢣�
void OS_TimeDly(OS_U32 ticks)
{
	OS_U8 y;
  OS_USE_CRITICAL;

  OS_ENTER_CRITICAL();

	y            =  g_OS_Tcb_CurP->OSTCBY;        /* ��λ��ǰ���ȼ�λ                                 */
	OSRdyTbl[y] &= ~g_OS_Tcb_CurP->OSTCBBitX;
	if (OSRdyTbl[y] == 0) {
		OSRdyGrp &= ~g_OS_Tcb_CurP->OSTCBBitY;
	}

  g_OS_Tcb_CurP->State = TASK_DELAY;
  g_OS_Tcb_CurP->TimeDly = ticks;

  OS_EXIT_CRITICAL();

  OS_Task_Switch();
  OS_PendSV_Trigger();
}

//����1
void task1(void *para)
{
  OS_USE_CRITICAL;
  para = para;
  while (1)
  {
    OS_ENTER_CRITICAL();
    LED1_TOGGLE;
    printf("123");
    OS_EXIT_CRITICAL();
    OS_TimeDly(200);
  }
}

//����2
void task2(void *para)
{
  OS_USE_CRITICAL;
  para = para;
  while (1)
  {
	  OS_ENTER_CRITICAL();
    LED2_TOGGLE;
    printf("abc");
	  OS_EXIT_CRITICAL();
    OS_TimeDly(400);
  }
}

//��������
void OS_Task_Idle(void *para)
{
  while (1)
  {
    __ASM("WFE");//����˯��̬�����͹��ģ�Ҳ����������������л�������һ�������CPU�����ʺܵ͵�����£�while����л��ظ����������Ƿ��б�Ҫ����
								 //����˯��̬�ᵼ��ʵʱ�Խ��ͣ��������ʵʱ�ԣ��ɲο�uCOS�����κ��ж��˳�ʱ����OSIntExit()������ʵʱ�Ի���Ӹߣ�������systick�жϿ������������л��������κ��ж϶��������л�����
								 //������ú���Դ����ʾ����������Ҫ��¼�ж�Ƕ�ײ�����ͬʱOSIntExit()�е��������л�����OSIntCtxSw()����ͨ��OSCtxSw()�����ϻ��������𣬻�ʹ�����һ�����ӡ�
    OS_Task_Switch();
    OS_PendSV_Trigger();
  }
}

//��ʼ������ջ
OS_STK* OSTaskStkInit(OS_TASK task, OS_STK *stk)
{
  OS_STK *p_stk;

  p_stk = stk;
  p_stk = (OS_STK *)((OS_STK)(p_stk) & 0xFFFFFFF8u); //�ֽڶ���

  *(--p_stk) = (OS_STK)0x01000000uL; //xPSR
  *(--p_stk) = (OS_STK)task;         // Entry Point
  *(--p_stk) = (OS_STK)0xFFFFFFFFuL; // R14 (LR)����ʼ״̬��û�ط������ˣ������ff��Ҳ������һ��End�����ַ����
  *(--p_stk) = (OS_STK)0x12121212uL; // R12
  *(--p_stk) = (OS_STK)0x03030303uL; // R3
  *(--p_stk) = (OS_STK)0x02020202uL; // R2
  *(--p_stk) = (OS_STK)0x01010101uL; // R1
  *(--p_stk) = (OS_STK)0x00000000uL; // R0 �����Ҫ���������R0��Ϊ��һ�������������޸����Ｔ��

  *(--p_stk) = (OS_STK)0x11111111uL; // R11
  *(--p_stk) = (OS_STK)0x10101010uL; // R10
  *(--p_stk) = (OS_STK)0x09090909uL; // R9
  *(--p_stk) = (OS_STK)0x08080808uL; // R8
  *(--p_stk) = (OS_STK)0x07070707uL; // R7
  *(--p_stk) = (OS_STK)0x06060606uL; // R6
  *(--p_stk) = (OS_STK)0x05050505uL; // R5
  *(--p_stk) = (OS_STK)0x04040404uL; // R4

  return (p_stk);
}

//��ʼ��TCB
OS_U8 OS_TCBInit(OS_TCB *ptcb, OS_STK *ptos, OS_U8 prio)
{
  OS_U8 err = 0;

  ptcb->StkAddr    = ptos;   
  ptcb->TimeDly    = 0;       
  ptcb->State      = TASK_READY;

  ptcb->OSTCBY     = (OS_U8)(prio >> 3);
  ptcb->OSTCBX     = (OS_U8)(prio & 0x07);
  ptcb->OSTCBBitY  = (OS_U8)(1 << ptcb->OSTCBY);
  ptcb->OSTCBBitX  = (OS_U8)(1 << ptcb->OSTCBX);

  ptcb->OSTCBNext  = OSTCBList;			//��ɫ����һ���ڵ�ʵ��������һ�������������������ȴ�������������ǿ�������������һ���ڵ�һ���ǿ�������
  ptcb->OSTCBPrev  = (OS_TCB *)0;
  if (OSTCBList != (OS_TCB *)0) {
    OSTCBList->OSTCBPrev = ptcb;
  }
  OSTCBList        = ptcb;

  ptcb->OSTCBPrio  = prio;

  OS_TCB_TABLE[prio] = ptcb;
  OSRdyGrp               |= ptcb->OSTCBBitY;         /* Make task ready to run                   */
  OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;

  return err;
}

//��������
void OS_Task_Create(OS_TCB *ptcb, OS_TASK task, OS_STK *stk, OS_U8 prio)
{
  OS_USE_CRITICAL
  OS_STK *psp;

  if (prio >= OS_TASK_MAX_NUM)
    return;

  OS_ENTER_CRITICAL();

  psp = OSTaskStkInit(task, stk);
  OS_TCBInit(ptcb, psp, prio);

  OS_EXIT_CRITICAL();
}

//OS��ʼ��
void OS_Init(void)
{
  int i;
  //�趨�쳣ջ��ַ��ARM�����ݼ�ջ
  g_OS_CPU_ExceptStkBase = OS_CPU_ExceptStk + OS_EXCEPT_STK_SIZE;
  //�����ж�
  __ASM("CPSID   I");
  //��ʼ��������ƿ�����
  for (i = 0; i < OS_TASK_MAX_NUM; i++)
    OS_TCB_TABLE[i] = 0;
  OS_TimeTick = 0;
  //����һ����������
  OS_Task_Create(&TCB_IDLE, OS_Task_Idle, &TASK_IDLE_STK[TASK_IDLE_STK_SIZE - 1], OS_TASK_IDLE_PRIO);
}

//OS����
void OS_Start(void)
{
  //�����л�
  OS_Task_Switch();
  //��ʼ��systick��ʱ��
  if (SysTick_Config(SystemCoreClock / OS_TICKS_PER_SECOND))
    while (1)
      ;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
  //OS��������ʼ��PendSV����ʼ��PSP��MSP���趨PendSV������־��
  OSStart_Asm();
}

int main(void)
{
  //��ʼ��LED��USART1
  LED_GPIO_Config();
  USART1_Config();

  OS_Init();

  //����2������
  OS_Task_Create(&TCB_1, task1, &TASK_1_STK[TASK_1_STK_SIZE - 1], 10);
  OS_Task_Create(&TCB_2, task2, &TASK_2_STK[TASK_2_STK_SIZE - 1], 20);

  //OS����
  OS_Start();

  while (1)
  {
  }
}

//systick�жϷ��������������������ʱ���ļ�1��
//����ĵ��Ⱥ����ڿ��������ﴥ����uCOSҲ��
void SysTick_Handler(void)
{
  OS_TCBP tcb_p;
  OS_USE_CRITICAL

  OS_ENTER_CRITICAL();
  ++OS_TimeTick;
  //����������Ľ�������1
  //�÷����������죬�ο�uCOS������һ������ֱ����������NULL��������ƿ�
  tcb_p = OSTCBList;
  while (tcb_p->OSTCBPrio != OS_TASK_IDLE_PRIO)
  {
    if (tcb_p->State == TASK_DELAY)
    {
      --tcb_p->TimeDly;	//˼����Ϊʲô���ú��Լ�����ǰ�Լ�Ч�ʸ��ߣ����豣�渱����
      if (tcb_p->TimeDly == 0)
			{
        tcb_p->State = TASK_READY;
				OSRdyGrp               |= tcb_p->OSTCBBitY;         /* Make task ready to run                   */
				OSRdyTbl[tcb_p->OSTCBY] |= tcb_p->OSTCBBitX;
			}
    }
    tcb_p = tcb_p->OSTCBNext;
  }
  
  OS_EXIT_CRITICAL();
}

/*********************************************END OF FILE**********************/

/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   ����led
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� iSO-MINI STM32 ������ 
  * ��̳    :http://www.chuxue123.com
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */ 
	
#include "stm32f10x.h"
#include "bsp_led.h"
#include "scheduler.h"

//������Сջ
//17�ĺ��壺һ����16���Ĵ�����Ҫ���棨����8�����Զ����棩������ARM����ջ���������ͣ������Ҫ����һ������
//17����Сջ����? �����ڴ������齫�����������Ϊʲô���������������?
#define MIN_STACK_SIZE	27

#define HW32_REG(ADDRESS)  (*((volatile unsigned long  *)(ADDRESS)))

uint32_t  curr_task=0;      //��ǰִ������
uint32_t  next_task=1;      //��һ������
uint32_t  task0_stack[MIN_STACK_SIZE];	//����0��ջ��

uint32_t  task1_stack[MIN_STACK_SIZE];	//����1��ջ
uint32_t  PSP_array[4];			//PSP���飬�洢ջ��ַ

u8 task0_handle=1;					//������޹�
u8 task1_handle=1;					//������޹�

int add(int x)
{
	int y=x;
	return y+x;
}

//����0
void task0(void) 
{ 
    while(1)
    {
        if(task0_handle==1)
        {
            LED2_TOGGLE;
            task0_handle=0;
            task1_handle=1;
        }
    }
}

//����1
uint32_t temp1,temp2,temp3,temp4;
void task1(void)
{
//		int a[3];		
		int b=4;	
		int d=b-4;	
		int e=d+2;
    while(1)
    {
        if(task1_handle==1)
        {
						d=add(b);
//						b=d;
            LED1_TOGGLE;
            task1_handle=0;
            task0_handle=1;
        }
    }
}


//Systick���Ķ�ʱ����ʼ��
void SysTick_Init(void)
{
	/* SystemFrequency / 1000    1ms�ж�һ��
	 * SystemFrequency / 100000	 10us�ж�һ��
	 * SystemFrequency / 1000000 1us�ж�һ��
	 */
//	if (SysTick_Config(SystemFrequency / 100000))	// ST3.0.0��汾
	if (SysTick_Config(SystemCoreClock / 1000))	// ST3.5.0��汾
	{ 
		/* Capture error */ 
		while (1);
	}
		// �رյδ�ʱ��  
//	SysTick->CTRL &= ~ SysTick_CTRL_ENABLE_Msk;
	SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}

int temp=0;
#define DEBUG 0
int main(void)
{
		//��ʼ��PendSV��LED
    SetPendSVPro();
    LED_GPIO_Config();
    
#if !DEBUG
//		/*��ʼ�������ջ*/
//		//PSP_array�д洢��Ϊtask0_stack������׵�ַ����task0_stack[0]��ַ
		PSP_array[0] = ((unsigned int) task0_stack);
	
    //��ʼ��ջ���ݣ�ֻ��Ҫ��ʼ����ڵ�ַҲ����PCָ���ָ��ֵ��xPSR�Ĵ������ɣ�����ͨ�üĴ�����λ�������ʼ����
	  //14��15���壺����PendSV�жϺ���Զ�����8����Ҫ�Ĵ��������ȱ������xPSR��PC�Ĵ������ų���16����ջ��Ӧ����14��15��
		//Ϊʲô14��PC,15��xPSR�����Ȩ��ָ�ϲ�һ�£����Ǻͦ�C/OSϵͳһ�£��ѵ���Ȩ��ָ�ϳ����ˣ�
		//����δ���ʣ���ַ����ֵ���ı��������������ܲ��Ὣ�䰲�Ž�ջ���棬����ֱ�ӷ���ͨ�üĴ������㡣
		//��˴���һ�����⣬��������ڽ�whileǰ��������Ҫ���ʵı�������������ջ�仯������PSPָ������������ǰ���Ѿ���ָ����ջ��
		//����ʼ����ʱ�������½��������ᵼ��ջ�仯�����ʹ��ջ�����14��15λ����ᱻ�������ǣ���ô�µ�ջָ��λ�����ȷ���Ӷ���������⣿
    HW32_REG((PSP_array[0] + 14*sizeof(uint32_t) )) = (unsigned long) task0; /* PC */
    HW32_REG((PSP_array[0] + 15*sizeof(uint32_t) )) = 0x01000000;            /* xPSR */
    
    PSP_array[1] = ((unsigned int) task1_stack);
    HW32_REG((PSP_array[1] + 14*sizeof(uint32_t))) = (unsigned long) task1; /* PC */
    HW32_REG((PSP_array[1] + 15*sizeof(uint32_t))) = 0x01000000;            /* xPSR */    
#else
		//PSP_array�д洢��Ϊtask0_stack�����β��ַ-16*4����task0_stack[1023-16]��ַ
    PSP_array[0] = ((unsigned int) task0_stack)+sizeof(task0_stack) ;
    //��ʼ��ջ���ݣ�ֻ��Ҫ��ʼ����ڵ�ַҲ����PCָ���ָ��ֵ��xPSR�Ĵ������ɣ�����ͨ�üĴ�����λ�������ʼ����
	  //14��15���壺����PendSV�жϺ���Զ�����8����Ҫ�Ĵ��������ȱ������xPSR��PC�Ĵ������ų���16����ջ��Ӧ����14��15��
    HW32_REG((PSP_array[0] - 2 )) = (unsigned long) task0; /* PC */
    HW32_REG((PSP_array[0] - 1 )) = 0x01000000;            /* xPSR */
    
    PSP_array[1] = ((unsigned int) task1_stack)+sizeof(task1_stack) ;
    HW32_REG((PSP_array[1] - 2 )) = (unsigned long) task1; /* PC */
    HW32_REG((PSP_array[1] - 1 )) = 0x01000000;            /* xPSR */   
#endif		
     
    /* ����PSPָ������0��ջ��ջ����Ӧ���ǵװɣ���Ȼ�Ǹߵ�ַ�� */
#if !DEBUG
    __set_PSP((PSP_array[curr_task] + (MIN_STACK_SIZE-1)*sizeof(uint32_t))); 
#else
		__set_PSP((PSP_array[curr_task = 0,curr_task])); 
#endif

		//��ʼ�����Ķ�ʱ��
    SysTick_Init();   
		
    /* ʹ�ö�ջָ�룬����Ȩ��״̬ */
    __set_CONTROL(0x3);//ʹ���û����߳�ģʽ+ʹ��PSP��ջָ�룬Ϊʲô���ܷ���SysTick��ʼ������ǰ��
    
    /* �ı�CONTROL��ִ��ISB (architectural recommendation) */
//    __ISB();

    /* ��������0 */
    task0();  
		
    while(1)
		{
			;
		}
}


/*********************************************END OF FILE**********************/

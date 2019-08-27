#include "scheduler.h"

extern uint32_t  curr_task,next_task,PSP_array[4];

//�趨PendSV���ȼ�����ʼ��PendSV�жϣ�
__asm void SetPendSVPro(void)
{
//		NVIC_SYSPRI14   EQU     0xE000ED22
//		NVIC_PENDSV_PRI EQU           0xFF
    
    LDR     R1, =NVIC_PENDSV_PRI    
    LDR     R0, =NVIC_SYSPRI14    
    STRB    R1, [R0]
    BX      LR
}

//����PendSV�ж�
__asm void TriggerPendSV(void)
{
//		NVIC_INT_CTRL   EQU     0xE000ED04                              
//		NVIC_PENDSVSET  EQU     0x10000000                              

    LDR     R0, =NVIC_INT_CTRL                                 
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR
}

//PendSV�жϷ�����
__asm void PendSV_Handler(void)
{ 
		/*
		***�﷨����**
	{}���б�
	LDR �Ӵ洢���м����ֵ�һ���Ĵ�����
	MRS <Rn>, <SReg> ;�������⹦�ܼĴ�����ֵ��Rn
	MSR <Sreg>,<Rn> ;�洢Rn ��ֵ�����⹦�ܼĴ���
	STMDB Rd!, {�Ĵ����б�}   �洢����ֵ� Rd ����ÿ��һ����ǰRd �Լ�һ�Σ�16λ��ȣ�����ѹջЧ����
	LDMIA Rd!, {�Ĵ����б�}   �� Rd ����ȡ����֡�ÿ��һ���ֺ�Rd ����һ�Σ�16λ��ȣ����Ƴ�ջЧ����
	LSL������ָ��
	#��������
	[Rd]:���ѰַRd
	[Rd,xx]��Rdƫ��xx
	STR����һ���Ĵ������ִ洢���洢���У����������ֵ�Ҳ�����ָ��
	BX������ָ��
	LR�����ӼĴ����������ӳ��򷵻ص�ַ
		*/

    // ���浱ǰ����ļĴ�������
    MRS    R0, PSP     // �õ�PSP  R0 = PSP ��
                       // xPSR, PC, LR, R12, R0-R3���Զ�����
    STMDB  R0!,{R4-R11}// ����PUSH��Ч����������PUSH�����PUSHֻ����MSP
											
    // ������һ�����������
    LDR    R1,=__cpp(&curr_task)
    LDR    R3,=__cpp(&PSP_array)
    LDR    R4,=__cpp(&next_task)
    LDR    R4,[R4]     // �õ���һ�������ID
    STR    R4,[R1]     // ���� curr_task = next_task��R4��ֵ��R1
    LDR    R0,[R3, R4, LSL #2] // ��PSP_array�л�ȡPSP��ֵ��LSL����ָ�R4����2bit��Ҳ��������4������Ϊ�ø�������uint32�ͣ�����ƫ��n������ʵ��Ӧƫ��4*n�ֽڣ���������R4ΪR3��ƫ������Ҳ����
    LDMIA  R0!,{R4-R11}// �������ջ�е���ֵ���ص�R4-R11�У�Ӧ�����ȳ�R4

    MSR    PSP, R0     // ����PSPָ�������

    BX     LR          // ���ء�return
                       // xPSR, PC, LR, R12, R0-R3���Զ��Ļָ�
}

//SysTick�жϷ�����
void SysTick_Handler(void)
{
	static unsigned int cnt=0;
	if(++cnt%100==0)
	{
    if(curr_task==0)
        next_task=1;
    else
        next_task=0;
    TriggerPendSV();
	}
}


void TaskSwitch(void)
{
	
	
}

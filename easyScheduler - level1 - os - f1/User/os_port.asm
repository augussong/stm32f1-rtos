;�õ��Ļ���﷨
;EQU �滻��䣬����#define
;EXTERN �������
;EXPORT ������䣻
;LDR    load register װ����䣬����mov
;CBZ    �Ƚϣ�������Ϊ0 ��ת��
;STR    ��һ���Ĵ������ִ洢���洢����
;MSR    ����Ĵ�����д�룬����PSP
;CPSIE  ʹ��PRIMASK
;CPSID  ʧ��PRIMASK
;SUB    ��
;ADD    ��
;STM    �洢���ɼĴ����е��ֵ�һƬ�����ĵ�ַ�ռ��У�������
;LDM    ��һƬ�����ĵ�ַ�ռ��м��ض���ֵ����ɼĴ���
;B      ������ת��
;BL     ��ת������LR�Ĵ��������ں������أ�����
;LR     ���ӼĴ�������PCָ����ת������ǰ���洢PCָ���ֵ
;END    ������Խ���
;MRS/MSR    ���ڶ�/д���⹦�ܼĴ�����PRIMASK��FAULTMASK��SP����ͨ�üĴ�����
;=      αָ����������ڵ�ַ��һ������
;����ָ�����CortexȨ��ָ�ϣ�ע�⣺ָ�֧�ֺ�׺���������Ҳ�����ȫһ�µ�ָ�����ע����û�н���ƥ���ָ�

NVIC_INT_CTRL   EQU     0xE000ED04				; �жϿ��Ƽ�״̬�Ĵ���Interrupt control state register.����28bit��1����PendSV�жϣ�
NVIC_SYSPRI14   EQU     0xE000ED22				; PendSV�����ȼ��Ĵ��� (priority 14).
NVIC_PENDSV_PRI EQU           0xFF				; PendSV���ȼ�ֵPendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000				; дNVIC_PENDSVSET��NVIC_INT_CTRL���ɴ���PendSV�ж�Value to trigger PendSV exception.

	;��֪�������
    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB	;THUMBָ���
    REQUIRE8
    PRESERVE8
 
  ;�ⲿ��������
  EXTERN  g_OS_CPU_ExceptStkBase
  EXTERN  g_OS_Tcb_CurP
  EXTERN  g_OS_Tcb_HighRdyP
	  
  ;�����ӿڵ���
  EXPORT OSCtxSw
  EXPORT OSStart_Asm
  EXPORT PendSV_Handler
  EXPORT OS_CPU_SR_Save
  EXPORT OS_CPU_SR_Restore
	  
OS_CPU_SR_Save
    MRS     R0, PRIMASK ; PRIMASK��1bit�ж�����Ĵ�������1ʱ�رճ�FAULT��NMI�������жϣ��൱�ڱ����жϿ���״̬
    CPSID   I           ; �ر������ж�
    BX      LR          ; �������ء�

OS_CPU_SR_Restore
    MSR     PRIMASK, R0
    BX      LR
	
;�������л�������PendSV�жϣ�
;˼����uCOS��OSCtxSw��OSIntCtxSw��ʲô�ã�����Ϊʲô�������������������ȴ�������أ�ʲô����������ϻ��������أ�
OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                  ;NVIC_INT_CTRL��R0
    LDR     R1, =NVIC_PENDSVSET                 ;NVIC_PENDSVSET��R1
    STR     R1, [R0]                            ;R1��ֵ�洢��R0�����ַָ���λ����ȥ�����Ѱַ��
    BX      LR                                  ;�������أ�PendSV�жϱ����ң�ʹ�ܣ����ȴ�������Enable interrupts at processor level

;OS��������ʼ��PendSV����ʼ��PSP��MSP���趨PendSV������־��
OSStart_Asm
    LDR     R0, =NVIC_SYSPRI14                  ; ����PendSV�쳣�����ȼ�����ʼ����Set the PendSV exception priority
    LDR     R1, =NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                              ; PSPָ���ʼ��Ϊ0��Set the PSP to 0 for initial context switch call
    MSR     PSP, R0

    LDR     R0, =g_OS_CPU_ExceptStkBase         ; MSPָ���ʼ��Ϊg_OS_CPU_ExceptStkBase����Initialize the MSP to the OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1    

    LDR     R0, =NVIC_INT_CTRL                  ; ��OSCtxSw�������͡�Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                   ; ʹ���жϡ�Enable interrupts at processor level

OSStartHang
    B       OSStartHang                         ; һ����ѭ������Ӧ��һֱ����ִ�С�Should never get here
    
    

PendSV_Handler
    CPSID   I                                   ; �ر�ȫ���жϡ�Prevent interruption during context switch
    MRS     R0, PSP                             ; ��ȡPSP��R0��PSP is process stack pointer
    CBZ     R0, OS_CPU_PendSVHandler_nosave     ; �ж�R0�ǲ���0����0�Ļ���ת��nosave��ǩȥ����Ϊ��0�����״�ִ��
                                                ;�����豣�����ģ�ֱ���л����ļ��ɡ�Skip register save the first time
   ;��������
    SUBS    R0, R0, #0x20                       ; R0��0x20=32���͵�ַƫ��8���Ĵ���*4�ֽڳ��ȣ��Ա���ʣ���8���Ĵ�����R0 = R0 - 0x20; Save remaining regs r4-11 on process stack
    STM     R0, {R4-R11}                        ; ��R4-R11�����ݴ洢��R0����ǿ�ʼ�ĵ�ַ��ȥ��ÿ�δ洢�ᵼ�µ�ַ+4����R0�����䡣˵�����ڽ��ж�ʱPSPָ����Զ�������ջƫ�ƣ��������ǵ�������ƿ����ջָ����������Զ��仯�����Ҫ�ֶ���������һ��

	  LDR     R1, =g_OS_Tcb_CurP                  ; ��g_OS_Tcb_CurP��ַ����R1��g_OS_Tcb_CurPָ�����һ��TCB�ṹ
    LDR     R1, [R1]                            ; ���Ѱַ����ȡTCB�ṹ���ýṹ��ĵ�һ���������Ǵ�ջָ�루ջ����ַ���ı����������ʵ�Ǵ��Ĵ�ջָ��ı���OSTCBCur->OSTCBStkPtr = SP;
    STR     R0, [R1]                            ; ��ȡ��ջָ��ı�����������ǰPSPָ����ȥ�����ı�����ɡ�R0 is SP of process being switched out

    ;�л�����                                   ; At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave
	  LDR     R0, =g_OS_Tcb_CurP                  ; ��ȡ�洢��ǰ������ƿ��ַ�ı�����OSTCBCur  = OSTCBHighRdy;  ��ע�⡰��ַ�������ֵ�λ�ã�
    LDR     R1, =g_OS_Tcb_HighRdyP              ; ��ȡ�洢����������ƿ��ַ�ı���
    LDR     R2, [R1]                            ; ��ȡ����������ƿ��ַ������R2
    STR     R2, [R0]                            ; ������������ƿ��ַ��ֵ����ǰ������ƿ飨ˢ�µ�ǰ����

	  LDR     R0, [R2]                            ; ��ȡջ����ַ�����ؽ�R0��R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
  
    LDM     R0, {R4-R11}                        ; ��R0�����ַ��ʼ���������ζ���8���Ĵ���R4-R11��ÿ��һ�Σ���ַ��+4����R0���䣩��Restore r4-11 from new process stack
    ADDS    R0, R0, #0x20                       ; R0����0x20=32����Ϊǰ��ָ���8���Ĵ�����1���Ĵ���ռ4Byte��
            
	  MSR     PSP, R0                             ; �µ�R0���µ�ջ����ַ������PSPָ�롣Load PSP with new process SP		
    ORR     LR, LR, #0x04                       ; ��Ҫ��ȷ���쳣���غ�ʹ��PSPָ�룬������MSPָ�롣LR��0x04�������洢��LR�С���CM3��Ӧ�쳣��LR�������½��ͣ���������ͨ���ӳ��򷵻�ֵ����ϸ���Ͳμ���Ȩ��ָ�ϡ��ھ���Լǰ10ҳ���ݡ�Ensure exception return uses process stack
    
    CPSIE   I                                   ; �����ж�
    BX      LR                                  ; �����ӳ���Exception return will restore remaining context
  
    END
#include "vector_handlers.h"

#include <stdlib.h>
#include "riscv-csr.h"

/* from freeRTOS  task.h definition */

extern void vTaskSwitchContext(void);

#define PRINT_CAUSE_ON_REGS(cause)                                                                                     \
	do                                                                                                                 \
	{                                                                                                                  \
		for (int i = 0; i < 10; i++)                                                                                   \
		{                                                                                                              \
			asm volatile("li x6,0xc1a0");                                                                              \
			asm volatile("li x7,0xc1a0");                                                                              \
			asm volatile("li x8,0xc1a0");                                                                              \
			asm volatile("mv x9,%0" : : "r"(cause));                                                                   \
			asm volatile("mv x10,%0" : : "r"(cause));                                                                  \
			asm volatile("li x11,0xc1a0");                                                                             \
			asm volatile("li x12,0xc1a0");                                                                             \
		}                                                                                                              \
	} while (0)

/*they overwrite the weak definitions*/
void freertos_risc_v_application_exception_handler(uint32_t mcause){
    
	// Known exceptions
    
	exit(1);
    csr_write_mcause(0);
    return ;
}


void freertos_risc_v_application_interrupt_handler(uint32_t int_number) {
    
    switch (int_number&0xff)
	{
	case RISCV_INT_MTI_CODE:
		break;
	case RISCV_INT_MSI_CODE:
        csr_clr_bits_mip(MIP_MSI_BIT_MASK);
		vTaskSwitchContext();
        /*for comet simulator, clean the mip*/
		asm volatile("li t0, %0" : : "i"(0));
		asm volatile("la t1, 0x50"); 
		asm volatile("sw t0,0(t1)");
	    break;
	default:
	    PRINT_CAUSE_ON_REGS(int_number);
	    exit(1);
		break;
	}
    csr_write_mcause(0);
    return;
}
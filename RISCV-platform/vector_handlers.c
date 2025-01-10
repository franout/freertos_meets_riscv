#include "vector_handlers.h"

#include <stdlib.h>
#include "riscv-csr.h"
#include "FreeRTOS.h"

/* from freeRTOS  task.h definition */

extern void vTaskSwitchContext(void);
extern BaseType_t xTaskIncrementTick( void );
extern uint64_t *pullMachineTimerCompareRegister;
extern uint64_t *pullNextTime; 
extern uint64_t uxTimerIncrementsForOneTick;

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
	// TO BE IMPLEMENTED 
	PRINT_CAUSE_ON_REGS(mcause);
	exit(1);
    csr_write_mcause(0);
    return ;
}


void freertos_risc_v_application_interrupt_handler(uint32_t int_number) {
    
    switch (int_number&0xff)
	{
	case RISCV_INT_MTI_CODE:
		
		
      #if (__riscv_xlen == 32)
        // 32-bit RISC-V: Update the 64-bit mtimer compare value in two 32-bit writes.
        uint32_t lowWordNextTime = (uint32_t)(*pullNextTime);          // Low 32 bits of ullNextTime
        uint32_t highWordNextTime = (uint32_t)(*pullNextTime >> 32);   // High 32 bits of ullNextTime
        uint32_t tempLowWord = 0xFFFFFFFF;                            // Temporary value for low word
        uint32_t timerIncrementLow = (uint32_t)uxTimerIncrementsForOneTick;

        // Write the low word with a temporary value to ensure atomicity
        ((uint32_t *)pullMachineTimerCompareRegister)[0] = tempLowWord;

        // Write the high word of ullNextTime
        ((uint32_t *)pullMachineTimerCompareRegister)[1] = highWordNextTime;

        // Write the low word of ullNextTime
        ((uint32_t *)pullMachineTimerCompareRegister)[0] = lowWordNextTime;

        // Calculate the new ullNextTime
        uint32_t newLowWord = lowWordNextTime + timerIncrementLow;
        uint32_t overflow = (newLowWord < lowWordNextTime) ? 1 : 0;    // Check for overflow
        uint32_t newHighWord = highWordNextTime + overflow;

        // Store the updated ullNextTime
        ((uint32_t *)pullNextTime)[0] = newLowWord;
        ((uint32_t *)pullNextTime)[1] = newHighWord;

    #elif (__riscv_xlen == 64)
        // 64-bit RISC-V: Update the 64-bit mtimer compare value in a single write.
        uint64_t nextTime = *pullNextTime;

        // Write ullNextTime to the compare register
        *pullMachineTimerCompareRegister = nextTime;

        // Calculate the new ullNextTime
        uint64_t newNextTime = nextTime + uxTimerIncrementsForOneTick;

        // Store the updated ullNextTime
        *pullNextTime = newNextTime;

    #endif

 	/*If a non-zero value is returned then a context switch is required */

	if (!xTaskIncrementTick()) {
		vTaskSwitchContext();
	}
	
    	
		break;
	case RISCV_INT_MSI_CODE:
    #ifdef __COMET_SIMULATOR__
        /*for comet simulator, clean the mip*/
		asm volatile("li t0, %0" : : "i"(0));
		asm volatile("la t1, 0x50"); 
		asm volatile("sw t0,0(t1)");
	#endif
		vTaskSwitchContext();
	    break;
	default:
	    PRINT_CAUSE_ON_REGS(int_number);
	    exit(1);
		break;
	}
    csr_write_mcause(0);
    return;
}
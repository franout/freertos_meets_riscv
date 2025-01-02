#ifndef __VECTOR_HANDLERS_H
#define __VECTOR_HANDLERS_H

#include <stdint.h>
#include "riscv-interrupts.h"

void freertos_risc_v_application_exception_handler(uint32_t mcause);
void freertos_risc_v_application_interrupt_handler(uint32_t int_number);

#endif /*__VECTOR_HANDLERS_H*/
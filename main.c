/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * This is a simple main that will start the FreeRTOS-Kernel and run a periodic task
 * that only delays if compiled with the template port, this project will do nothing.
 * For more information on getting started please look here:
 * https://freertos.org/FreeRTOS-quick-start-guide.html
 */

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>


#ifdef __COMET_SIMULATOR__
#include <stdlib.h>

/* currently, Comet breaks on ecall*/
#define END() 		do { asm volatile ("ecall"); }while(0)
#endif /*__COMET_SIMULATOR__*/


/*
*********************************************************************************************************
*                                           LOCAL DEFINES
*********************************************************************************************************
*/
#define PRINT_END()                                                                                                    \
	do                                                                                                                 \
	{                                                                                                                  \
		for (int i = 0; i < 10; i++)                                                                                   \
		{                                                                                                              \
			asm volatile("li x6,0xc1a0");                                                                              \
			asm volatile("li x7,0xE41d");                                                                              \
			asm volatile("li x8,0xc1a0");                                                                              \
			asm volatile("li x9,0xE41d");                                                                              \
			asm volatile("li x10,0xc1a0");                                                                             \
			asm volatile("li x11,0xE41d");                                                                             \
			asm volatile("li x12,0xc1a0");                                                                             \
			asm volatile("li x13,0xE41d");                                                                             \
			asm volatile("li x14,0xc1a0");                                                                             \
			asm volatile("li x15,0xE41d");                                                                             \
			asm volatile("li x16,0xc1a0");                                                                             \
			asm volatile("li x17,0xE41d");                                                                             \
			asm volatile("li x18,0xc1a0");                                                                             \
			asm volatile("li x19,0xE41d");                                                                             \
			asm volatile("li x20,0xc1a0");                                                                             \
			asm volatile("li x21,0xE41d");                                                                             \
		}                                                                                                              \
	} while (0)

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
/*-----------------------------------------------------------*/

static void exampleTask(void *parameters) __attribute__((noreturn));
static void exampleTask2(void *parameters) __attribute__((noreturn));

/*-----------------------------------------------------------*/

static void exampleTask(void *parameters)
{
    /* Unused parameters. */
    (void)parameters;
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
    volatile int my_dummy_counter = 0;
    for (;;)
    {
        /* Example Task Code */
        my_dummy_counter=(my_dummy_counter++)<<2;
        vTaskDelay(xDelay); /* block for 500 ms*/
    }
}

static void exampleTask2(void *parameters)
{
    /* Unused parameters. */
    
    const TickType_t xFrequency = 100;
    TickType_t xLastWakeTime;
    //const TickType_t xDelay = 4 / portTICK_PERIOD_MS;
    volatile int a=1;
    volatile int b=0;
    volatile int c;
    xLastWakeTime = xTaskGetTickCount ();
    for (;;)
    {
        c=a*b;
        a++;
        b++;
        xTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}
/*-----------------------------------------------------------*/

volatile unsigned int period = 0 ;
volatile unsigned int counter = 0;
/*it cointans the stop conditions*/
void vApplicationIdleHook(void)
{
#ifdef __COMET_SIMULATOR__
	counter++;
    if (counter<=1) {
        period++;
        if (period >= MAX_HYPERPERIOD_REPS)
	    {	
		PRINT_END();
        END();
	    }
    }
	/*the clean up counter must be cleaned when the tick is incremented*/
#endif /*__COMET_SIMULATOR__*/
    
}

void vApplicationTickHook( void ){
    /* clean up the counter for the hyperperiod counter in the idle task */
    if (counter >= 1 ) {
        counter = 0 ;
    }
    
}

extern void freertos_risc_v_trap_handler(void);

static StaticTask_t exampleTaskTCB;
static StackType_t exampleTaskStack[configMINIMAL_STACK_SIZE*2];

static StaticTask_t exampleTaskTCB2;
static StackType_t exampleTaskStack2[configMINIMAL_STACK_SIZE*2];

__attribute__((optimize("O0"))) int main(void)
{
    BaseType_t xReturned = pdPASS;
    TaskHandle_t xHandle = NULL;
    /**************************************************************
    *****************                              *****************
    *****************           INT settings       *****************
    *****************                              *****************
    ***************************************************************/
    // Global interrupt disable
    csr_clr_bits_mstatus(MSTATUS_MIE_BIT_MASK);
    portDISABLE_INTERRUPTS();
    
    // Setup the IRQ handler entry point, set the software mode 
    csr_write_mtvec((uint_xlen_t) freertos_risc_v_trap_handler );
    
    /* dynamic task*/
    xReturned = xTaskCreate(exampleTask,
                            "example",
                            configMINIMAL_STACK_SIZE,
                            NULL,
                            configMAX_PRIORITIES - 2U,
                            xHandle);

    if (xReturned != pdPASS ){
        PRINT_END();
        END();
    }
    /* static task*/
    xHandle = xTaskCreateStatic(exampleTask2,
                            "example2",
                            configMINIMAL_STACK_SIZE,
                            NULL,
                            //configMAX_PRIORITIES - 3U,
                            2,
                            exampleTaskStack2,
                            &(exampleTaskTCB2));
    if (xHandle == NULL){
        PRINT_CAUSE_ON_REGS(xReturned);
        END();
    }
    /* Start the scheduler. */
    vTaskStartScheduler();

    for (;;)
    {
        /* Should not reach here. */
    }
    return 0;
}
/*-----------------------------------------------------------*/

#if (configCHECK_FOR_STACK_OVERFLOW > 0)

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    /* Check pcTaskName for the name of the offending task,
     * or pxCurrentTCB if pcTaskName has itself been corrupted. */
    (void)xTask;
    (void)pcTaskName;
}

#endif /* #if ( configCHECK_FOR_STACK_OVERFLOW > 0 ) */
/*-----------------------------------------------------------*/

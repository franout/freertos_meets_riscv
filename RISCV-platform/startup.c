/*
   Simple C++ startup routine to setup CRT
   SPDX-License-Identifier: Unlicense

   (https://five-embeddev.com/ | http://www.shincbm.com/) 

*/

#include <stdint.h>
#include <string.h>

// Generic C function pointer.
typedef void(*function_t)(void) ;

// These symbols are defined by the linker script.
// See linker.lds
extern uint8_t __bss_start;
extern uint8_t __bss_end;
extern const uint8_t segment_data_source_start;
extern uint8_t segment_data_target_start;
extern uint8_t segment_data_target_end;

extern function_t __init_array_start;
extern function_t __init_array_end;
extern function_t __fini_array_start;
extern function_t __fini_array_end;

// This function will be placed by the linker script according to the section
// Raw function 'called' by the CPU with no runtime.
extern void _start(void)  __attribute__ ((naked, section(".text.init.enter")));

// Entry and exit points as C functions.
extern void _crt0(void) __attribute__ ((noreturn));
void _Exit(int exit_code) __attribute__ ((noreturn,noinline));

// Standard entry point, no arguments.
extern int main(void);

// The linker script will place this in the reset entry point.
// It will be 'called' with no stack or C runtime configuration.
// NOTE - this only supports a single hart.
// tp will not be initialized
void _start(void) {
    // Setup SP and GP
    // The locations are defined in the linker script
    __asm__ volatile  (
        ".option push;"
        // The 'norelax' option is critical here.
        // Without 'norelax' the global pointer will
        // be loaded relative to the global pointer!
         ".option norelax;"
        "la    gp, __global_pointer$;"
        ".option pop;"
        "la    sp, _sp;"
        "jal   zero, _crt0;"
        :  /* output: none %0 */
        : /* input: none */
        : /* clobbers: none */); 
    // This point will not be executed, _start() will be called with no return.
}

// At this point we have a stack and global poiner, but no access to global variables.
void _crt0(void) {

    // Init memory regions
    // Clear the .bss section (global variables with no initial values)
    memset((void*) &__bss_start,
           0, 
           (&__bss_end - &__bss_start));

    // Initialize the .data section (global variables with initial values)
    memcpy((void*)&segment_data_target_start,
           (const void*)&segment_data_source_start,
           (&segment_data_target_end - &segment_data_target_start));

    // Call constructors
    for (const function_t* entry=&__init_array_start; 
         entry < &__init_array_end;
         ++entry) {
        (*entry)();
    }

    int rc = main();

    // Call destructors
    for (const function_t* entry=&__fini_array_start; 
         entry < &__fini_array_end;
         ++entry) {
        (*entry)();
    }
    /*SHOULD NEVER REACH HERE*/
    while(1);

}


/* Copyright (c) 2020 SiFive Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
OUTPUT_ARCH("riscv")

/* Default Linker Script
 *
 * This is the default linker script for all Freedom Metal applications.
 */

ENTRY(_start)

MEMORY
{
    romram (iarwx) : ORIGIN = 0x00030000 , LENGTH = 1M
    stack_zone : ORIGIN = 0x00030000 + 1M - 32k - 4k, LENGTH = 32k
    heap_zone : ORIGIN = 0x00030000 + 1M - 4k , LENGTH = 4k 
}

PHDRS
{
    romram PT_LOAD;
    ram_init PT_LOAD;
    stack_zone PT_DYNAMIC ;
    heap_zone PT_DYNAMIC;
    tls PT_TLS;
    text PT_LOAD;
    lim_init PT_LOAD;
}

SECTIONS
{
    /* Each hart is allocated its own stack of size __stack_size. This value
     * can be overriden at build-time by adding the following to CFLAGS:
     *
     *     -Xlinker --defsym=__stack_size=0xf00
     *
     * where 0xf00 can be replaced with a multiple of 16 of your choice.
     *
     * __stack_size is PROVIDE-ed as a symbol so that initialization code
     * initializes the stack pointers for each hart at the right offset from
     * the _sp symbol.
     */
    __stack_size = DEFINED(__stack_size) ? __stack_size : 0x20000;

    PROVIDE(__stack_size = __stack_size);

    /* The size of the heap can be overriden at build-time by adding the
     * following to CFLAGS:
     *
     *     -Xlinker --defsym=__heap_size=0xf00
     *
     * where 0xf00 can be replaced with the value of your choice.
     *
     * Altertatively, the heap can be grown to fill the entire remaining region
     * of RAM by adding the following to CFLAGS:
     *
     *     -Xlinker --defsym=__heap_max=1
     *
     * Note that depending on the memory layout, the bitness (32/64bit) of the
     * target, and the code model in use, this might cause a relocation error.
     */
    __heap_size = DEFINED(__heap_size) ? __heap_size : 0x800;
    PROVIDE(__heap_size = __heap_size);

    /* The boot hart sets which hart runs the pre-main initialization routines,
     * including copying .data into RAM, zeroing the BSS region, running
     * constructors, etc. After initialization, the boot hart is also the only
     * hart which runs application code unless the application overrides the
     * secondary_main() function to start execution on secondary harts.
     */
    PROVIDE(__boot_hart = 0);

    /* The chicken bit is used by pre-main initialization to enable/disable
     * certain core features */
    PROVIDE(__chicken_bit = 1);

    /* The memory_ecc_scrub bit is used by _entry code to enable/disable
     * memories scrubbing to zero  */
    PROVIDE(__eccscrub_bit = 0);

    /* ROM SECTION
     *
     * The following sections contain data which lives in read-only memory, if
     * such memory is present in the design, for the entire duration of program
     * execution.
     */

    .init : {
        /* The _enter symbol is placed in the .text.init.enter section
         * and must be placed at the beginning of the program */
        KEEP (*(.text.init.enter))
        KEEP (*(.text.init.*))
        KEEP (*(SORT_NONE(.init)))
        KEEP (*(.text.libgloss.start))
    } >romram :romram

    .fini : {
        KEEP (*(SORT_NONE(.fini)))
    } >romram :romram

    .preinit_array : ALIGN(8) {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >romram :romram

    .init_array : ALIGN(8) {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
        PROVIDE_HIDDEN ( constructors_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)));
        KEEP (*(.init_array));
        PROVIDE_HIDDEN ( constructors_end = .);
    } >romram :romram

    .fini_array : ALIGN(8) {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
        PROVIDE_HIDDEN ( destructors_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)));
        KEEP (*(.fini_array));
        PROVIDE_HIDDEN ( destructors_end = .);
    } >romram :romram

 

    .ctors : {
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        KEEP (*(.ctors .ctors.*))
    } >romram :romram

    .dtors : {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        KEEP (*(.dtors .dtors.*))
    } >romram : romram

    .rodata : {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(8);
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
    } >romram :romram



    /* TEXT SECTION
     *
     * The following section contains the code of the program, excluding
     * everything that's been allocated into the ITIM/LIM already
     */

    .text : {
        *(.text.unlikely .text.unlikely.*)
        *(.text.startup .text.startup.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    } >romram :text

    /* RAM SECTION
     *
     * The following sections contain data which is copied from read-only
     * memory into a read-write-capable memory such as data tightly-integrated
     * memory (DTIM) or another main memory, as well as the BSS, stack, and
     * heap.
     *
     * You might notice that .data, .tdata, .tbss, .tbss_space, and .bss all
     * have an apparently unnecessary ALIGN at their top. This is because
     * the implementation of _start in Freedom Metal libgloss depends on the
     * ADDR and LOADADDR being 8-byte aligned.
     */


    .data : ALIGN(8) {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(.sdata .sdata.* .sdata2.*)
        *(.gnu.linkonce.s.*)
    } >romram AT>romram :ram_init

    .tdata : ALIGN(8) {
        PROVIDE( __tls_base = . );
	*(.tdata .tdata.* .gnu.linkonce.td.*)
    } >romram AT>romram  :ram_init

    PROVIDE( __tdata_source = LOADADDR(.tdata) );
    PROVIDE( __tdata_size = SIZEOF(.tdata) );

    PROVIDE( segment_data_source_start = LOADADDR(.data) );
    PROVIDE( segment_data_target_start = ADDR(.data) );
    PROVIDE( segment_data_target_end = ADDR(.tdata) + SIZEOF(.tdata) );

    .tbss : ALIGN(8) {
	*(.tbss .tbss.* .gnu.linkonce.tb.*)
	*(.tcommon .tcommon.*)
	PROVIDE( __tls_end = . );
    } >romram AT>romram :romram
    PROVIDE( __tbss_size = SIZEOF(.tbss) );
    PROVIDE( __tls_size = __tls_end - __tls_base );

    .tbss_space : ALIGN(8) {
	. = . + __tbss_size;
    } >romram :romram

    .bss (NOLOAD): ALIGN(8) {
        PROVIDE( __bss_start = .);
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        PROVIDE( __bss_end = .);
        PROVIDE( _end = . );
    } >romram :romram

    PROVIDE( segment_tbss_source_start = LOADADDR(.tbss) );
    PROVIDE( segment_tbss_target_start = ADDR(.tbss) );
    PROVIDE( segment_tbss_target_end = ADDR(.bss) + SIZEOF(.bss) );
 
    .stack (NOLOAD) : ALIGN(16) {
        
        . = ORIGIN(stack_zone);
        PROVIDE(segment_stack_begin = .);
        . += __stack_size; /* Hart 0 */
        PROVIDE( _sp = . );
        PROVIDE(segment_stack_end = .);
    } >stack_zone :stack_zone
    

    .heap (NOLOAD) : ALIGN(8) {
        . = ORIGIN(heap_zone);
        PROVIDE( __heap_start = . );
        . += __heap_size;
        PROVIDE( __heap_end = . );
    } >heap_zone :heap_zone

    /* C++ exception handling information is
     * not useful with our current runtime environment,
     * and it consumes flash space. Discard it until
     * we have something that can use it
     
    /DISCARD/ : {
	*(.eh_frame .eh_frame.*)
    }*/
}
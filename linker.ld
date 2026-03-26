/* linker.ld - Linker script for i386 kernel */

/* Entry point */
ENTRY(_start)

/* Sections */
SECTIONS
{
    /* Kernel starts at 1MB */
    . = 0x00100000;

    /* Multiboot header first */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    /* Read-only data */
    .text ALIGN(4K) : {
        *(.text)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data ALIGN(4K) : {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
}

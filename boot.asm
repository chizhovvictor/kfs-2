; boot.asm - Multiboot header and kernel entry point
; This file sets up the multiboot header for GRUB and calls the kernel main function

; Multiboot header constants
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_PAGE_ALIGN    equ 1 << 0
MULTIBOOT_MEMORY_INFO   equ 1 << 1
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; Kernel stack size (16 KB)
KERNEL_STACK_SIZE equ 0x4000

section .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb KERNEL_STACK_SIZE
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Set up the stack
    mov esp, stack_top

    ; Push multiboot information
    push ebx    ; Multiboot info structure pointer
    push eax    ; Multiboot magic number

    ; Call the kernel main function
    call kernel_main

    ; Hang if kernel_main returns
    cli
.hang:
    hlt
    jmp .hang

; boot.asm - Multiboot header and kernel entry point
; This file sets up the multiboot header for GRUB and calls the kernel main function

; Multiboot header constants
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_PAGE_ALIGN    equ 1 << 0
MULTIBOOT_MEMORY_INFO   equ 1 << 1
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; GDT placement requirements
GDT_PHYS_ADDR           equ 0x00000800
GDT_ENTRY_COUNT         equ 7
GDT_SIZE_BYTES          equ GDT_ENTRY_COUNT * 8

; Segment selectors
KERNEL_CODE_SEL         equ 0x08
KERNEL_DATA_SEL         equ 0x10
KERNEL_STACK_SEL        equ 0x18
USER_CODE_SEL           equ 0x20
USER_DATA_SEL           equ 0x28
USER_STACK_SEL          equ 0x30

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

gdt_template:
    ; Null descriptor
    dq 0x0000000000000000
    ; Kernel code (base=0, limit=4GB)
    dq 0x00CF9A000000FFFF
    ; Kernel data
    dq 0x00CF92000000FFFF
    ; Kernel stack (expand-down, writable)
    dq 0x00CF96000000FFFF
    ; User code (DPL=3)
    dq 0x00CFFA000000FFFF
    ; User data (DPL=3)
    dq 0x00CFF2000000FFFF
    ; User stack (expand-down, writable, DPL=3)
    dq 0x00CFF6000000FFFF

gdt_descriptor:
    dw GDT_SIZE_BYTES - 1
    dd GDT_PHYS_ADDR

_start:
    ; Copy required GDT to physical address 0x800
    cld
    mov esi, gdt_template
    mov edi, GDT_PHYS_ADDR
    mov ecx, GDT_SIZE_BYTES / 4
    rep movsd

    ; Load new GDT and reload segment registers
    cli
    lgdt [gdt_descriptor]
    jmp KERNEL_CODE_SEL:.reload_cs

.reload_cs:
    mov ax, KERNEL_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ax, KERNEL_STACK_SEL
    mov ss, ax

    ; Set up the kernel stack with the new stack segment
    mov esp, stack_top

    ; Call the kernel main function
    call kernel_main

    ; Hang if kernel_main returns
    cli
.hang:
    hlt
    jmp .hang

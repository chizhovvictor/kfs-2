# KFS-2 - 42 Defense README

Minimal x86 kernel booted by GRUB. This step includes a custom Global Descriptor Table (GDT) at address 0x00000800 and human-friendly debug output through printk.

## Subject Checklist

- Create a Global Descriptor Table: done
- GDT contains:
  - Kernel code: done
  - Kernel data: done
  - Kernel stack: done
  - User code: done
  - User data: done
  - User stack: done
- Declare/load GDT for CPU startup path (via LGDT): done
- GDT base address is 0x00000800: done
- Add tool to print kernel stack in human-friendly way: done
- Total work under 10 MB: done

## Build And Run (Linux)

### Dependencies

```bash
sudo apt-get install nasm gcc grub-common xorriso qemu-system-x86
```

### Commands

```bash
make re
make run
```

Output artifact:

- kfs-2.iso

GRUB menu entry:

- KFS-2

## Project Layout

```text
kfs-2/
|- boot.asm
|- kernel.c
|- kernel.h
|- linker.ld
|- Makefile
`- README.md
```

## Runtime Flow

1. BIOS/UEFI starts GRUB from the ISO.
2. GRUB executes multiboot loading for /boot/kernel.bin.
3. Control jumps to _start in boot.asm.
4. _start copies the GDT to physical address 0x00000800.
5. _start executes LGDT, reloads segment registers, and sets SS/ESP for kernel stack.
6. _start calls kernel_main.
7. kernel_main prints basic text and dumps the kernel stack.
8. Kernel enters an infinite hlt loop.

## GDT Details

GDT is built as 7 descriptors:

1. Null descriptor
2. Kernel code segment (selector 0x08)
3. Kernel data segment (selector 0x10)
4. Kernel stack segment (selector 0x18)
5. User code segment (selector 0x20)
6. User data segment (selector 0x28)
7. User stack segment (selector 0x30)

All segments use base 0 and 4 GB limit (32-bit protected mode layout).

## Kernel Stack Dump Tool

The stack dump tool prints:

- Current ESP and EBP
- Table with index, memory address, and 32-bit value for N words from current ESP

This is used as a printk-like debug aid for defense: you can show concrete stack content live during boot.

## Memory Map Output

Memory regions from Multiboot are printed in compact rows suitable for VGA 80x25:

- index
- base low 32 bits
- length low 32 bits
- type (usable, reserved, acpi, nvs, bad)

If a region has non-zero high 32-bit words, an extra line is printed with those high values.

## Memory Notes

- VGA text buffer: 0x000B8000
- Kernel link base: 0x00100000
- GDT location: 0x00000800
- Kernel stack reserved in .bss (16 KiB)

## 30-Second Defense Pitch (EN)

GRUB loads a multiboot-compatible kernel and jumps to _start. In assembly, I copy my custom GDT to 0x00000800, load it with LGDT, reload all segment registers, and initialize kernel stack segment plus ESP. Then I call kernel_main, which writes to VGA memory and runs a human-readable kernel stack dump. The kernel stays in a safe hlt loop. This satisfies the required GDT layout and debugging output for stack inspection.

## Короткий текст для защиты (RU)

GRUB загружает multiboot-совместимое ядро и передает управление в _start. В boot.asm я размещаю GDT по адресу 0x00000800, загружаю ее через LGDT, перезагружаю сегментные регистры и настраиваю сегмент стека ядра с ESP. После этого вызывается kernel_main, который пишет в VGA-память и выводит человекочитаемый дамп стека ядра. Далее ядро уходит в бесконечный hlt-цикл. Так выполняются требования по составу GDT и инструменту просмотра стека.

## Notes

If evaluator asks about BIOS wording: technically CPU uses GDTR and segment selectors in protected mode. In this project, declaring GDT is done by loading GDTR with LGDT early in boot path.

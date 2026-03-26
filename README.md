# KFS-2 - Kernel From Scratch

A simple x86 kernel that boots with GRUB and displays "42" on the screen.

## Requirements

- `nasm` - Netwide Assembler for compiling assembly code
- `gcc` - GNU C Compiler with i386 support
- `ld` - GNU Linker
- `grub-mkrescue` - GRUB bootable ISO creator
- `xorriso` - ISO filesystem tool (dependency for grub-mkrescue)
- `qemu-system-i386` (optional) - For testing the kernel

### Installation on macOS

```bash
brew install nasm
brew install i686-elf-gcc
brew install xorriso
brew install grub
brew install qemu
```

Note: You may need to use `i686-elf-gcc` and `i686-elf-ld` on macOS. Update the Makefile if needed.

### Installation on Linux

```bash
sudo apt-get install nasm gcc grub-common xorriso qemu-system-x86
```

## Project Structure

```
kfs-2/
├── boot.asm        # Assembly bootloader with multiboot header
├── kernel.c        # Main kernel code in C
├── kernel.h        # Kernel header file
├── linker.ld       # Linker script for i386
├── Makefile        # Build system
└── README.md       # This file
```

## Building

To build the kernel and create a bootable ISO:

```bash
make
```

This will:
1. Compile `boot.asm` to an object file
2. Compile `kernel.c` to an object file
3. Link both objects using `linker.ld` to create `kernel.bin`
4. Create a bootable ISO with GRUB

## Running

### With QEMU

```bash
make run
```

### With other virtual machines

Use the generated `kfs-2.iso` file with:
- VirtualBox
- VMware
- Real hardware (burn to CD/USB)

## Cleaning

To remove all build artifacts:

```bash
make clean
```

To rebuild everything:

```bash
make re
```

## Features

- Multiboot compliant (bootable with GRUB)
- Protected mode i386 kernel
- VGA text mode output
- Displays "42" on screen

## Implementation Details

### boot.asm
- Contains multiboot header for GRUB
- Sets up kernel stack
- Calls kernel_main function

### kernel.c
- Implements VGA text mode driver
- Provides screen manipulation functions
- Displays "42" on screen at position (0,0)

### linker.ld
- Maps kernel to 1MB memory address
- Organizes sections (.text, .data, .bss, .rodata)
- Sets proper alignment for sections

## Defense Notes (Detailed Runtime Explanation)

This section explains exactly what happens from boot to the `"42"` output.

### 1. End-to-end boot flow

1. BIOS/UEFI loads GRUB from the ISO image.
2. GRUB reads `grub.cfg` and executes `multiboot /boot/kernel.bin`.
3. GRUB loads `kernel.bin` into memory and jumps to the linker entry symbol `_start`.
4. `_start` (in `boot.asm`) sets up the stack and calls the C entry point.
5. `kernel_main` (in `kernel.c`) clears VGA text memory and writes `"42"` at `(0,0)`.
6. Kernel enters an infinite `hlt` loop.

### 2. Multiboot header in `boot.asm`

The multiboot header is required so GRUB recognizes the binary as bootable.

- `MULTIBOOT_HEADER_MAGIC = 0x1BADB002`
- `MULTIBOOT_HEADER_FLAGS = PAGE_ALIGN | MEMORY_INFO`
- `MULTIBOOT_CHECKSUM = -(MAGIC + FLAGS)`

GRUB validates these 3 double-words. If valid, it loads and starts your kernel.

### 3. CPU state expected from GRUB

When GRUB jumps to your kernel entry:

- `EAX = 0x2BADB002` (multiboot magic runtime value)
- `EBX = pointer to multiboot info structure`

In `_start`, your code does:

- `mov esp, stack_top` to initialize a valid kernel stack.
- `push ebx` then `push eax` to pass multiboot values on stack.
- `call kernel_main` to transfer control to C code.

### 4. Stack trace at kernel entry

After `call kernel_main`, stack layout is conceptually:

```text
[ESP + 0]  return address to boot.asm
[ESP + 4]  multiboot magic (EAX value pushed)
[ESP + 8]  multiboot info pointer (EBX value pushed)
```

Current `kernel_main` implementation does not consume these arguments, which is fine for this minimal milestone.

### 5. Why `linker.ld` is mandatory

Project requirement says you must create your own linker script (not host default `.ld`).

Your `linker.ld` does exactly that:

- `ENTRY(_start)` defines the first executed symbol.
- `. = 0x00100000` places kernel at 1 MiB.
- Places sections in explicit order:
	- `.multiboot`
	- `.text`
	- `.rodata`
	- `.data`
	- `.bss`
- Uses `ALIGN(4K)` for deterministic page-friendly layout.

This satisfies the requirement: you use `ld` binary, but with your custom linker script.

### 6. Memory map (simplified)

```text
0x000B8000             VGA text buffer (80x25, 2 bytes per cell)
0x00100000             Kernel load base (from linker.ld)
0x00100000 + sections  .multiboot, .text, .rodata, .data, .bss
within .bss            kernel stack (16 KiB reserved by resb)
```

### 7. VGA output path (`kernel.c`)

The kernel writes directly to VGA memory `0xB8000`:

- Each screen cell is 16 bits:
	- low 8 bits: ASCII character
	- high 8 bits: color attribute
- `clear_screen()` fills the entire 80x25 buffer with spaces.
- `print_at("42", 0, 0, color)` writes:
	- `'4'` to first cell
	- `'2'` to second cell

Result: `42` appears in top-left corner.

### 8. Why infinite `hlt` loop is used

After initialization, kernel executes:

```c
while (1) {
		__asm__ __volatile__("hlt");
}
```

This prevents returning into invalid context and keeps CPU idle safely.

### 9. Build flags and why they matter

Important freestanding flags used in `Makefile`:

- `-ffreestanding`: compile as freestanding environment.
- `-fno-builtin`: do not replace calls with compiler builtins.
- `-fno-exceptions`: disable exception runtime model.
- `-fno-stack-protector`: avoid host stack canary runtime dependency.
- `-nostdlib -nodefaultlibs -nostartfiles`: no host CRT or standard libs.
- `-Wall -Wextra -Werror`: strict warning policy.

This ensures host libraries are not linked into kernel image.

### 10. Quick oral defense summary (30 seconds)

"GRUB loads my multiboot-compliant kernel and jumps to `_start`. In assembly, I set a
16 KiB stack, preserve multiboot values, and call `kernel_main` in C. The kernel writes
directly to VGA memory at `0xB8000`, clears screen, then prints `42`. Linking is done
with my own `linker.ld` at 1 MiB, with explicit section layout and no host libraries,
using `-nostdlib` and `-nodefaultlibs`."

## Defense Notes (RU)

Этот раздел дублирует пояснения для защиты проекта на русском языке.

### 1. Полный путь загрузки

1. BIOS/UEFI запускает GRUB с ISO.
2. GRUB читает `grub.cfg` и выполняет `multiboot /boot/kernel.bin`.
3. GRUB загружает `kernel.bin` в память и передает управление в `_start`.
4. `_start` в `boot.asm` инициализирует стек и вызывает C-функцию ядра.
5. `kernel_main` в `kernel.c` очищает VGA-экран и пишет `"42"` в `(0,0)`.
6. Затем ядро уходит в бесконечный цикл `hlt`.

### 2. Multiboot header в `boot.asm`

Чтобы GRUB распознал бинарник как загрузочный, в ядре есть multiboot header:

- `MULTIBOOT_HEADER_MAGIC = 0x1BADB002`
- `MULTIBOOT_HEADER_FLAGS = PAGE_ALIGN | MEMORY_INFO`
- `MULTIBOOT_CHECKSUM = -(MAGIC + FLAGS)`

GRUB проверяет эти 3 значения и, если они корректны, запускает ядро.

### 3. Состояние CPU при входе в ядро

Перед прыжком в `_start` GRUB передает:

- `EAX = 0x2BADB002` (магическое значение multiboot)
- `EBX = указатель на структуру multiboot info`

Дальше в `_start` выполняется:

- `mov esp, stack_top` - установка валидного стека ядра.
- `push ebx`, затем `push eax` - передача multiboot-данных через стек.
- `call kernel_main` - переход в C-код.

### 4. Схема стека на входе в `kernel_main`

Сразу после `call kernel_main` стек выглядит так:

```text
[ESP + 0]  адрес возврата в boot.asm
[ESP + 4]  multiboot magic (бывший EAX)
[ESP + 8]  указатель на multiboot info (бывший EBX)
```

В текущей минимальной версии `kernel_main` эти аргументы не использует, и для этого этапа это нормально.

### 5. Почему обязателен свой `linker.ld`

По условию проекта нельзя использовать host linker script. Нужно писать свой.

Ваш `linker.ld` делает именно это:

- `ENTRY(_start)` задает точку входа.
- `. = 0x00100000` размещает ядро с адреса 1 MiB.
- Секции раскладываются явно: `.multiboot`, `.text`, `.rodata`, `.data`, `.bss`.
- `ALIGN(4K)` дает предсказуемую page-aligned раскладку.

Это полностью соответствует требованию: используется `ld` как бинарник, но со своим `.ld` файлом.

### 6. Карта памяти (упрощенно)

```text
0x000B8000             VGA text buffer (80x25, 2 байта на символ)
0x00100000             База загрузки ядра (из linker.ld)
0x00100000 + sections  .multiboot, .text, .rodata, .data, .bss
внутри .bss            стек ядра (16 KiB через resb)
```

### 7. Как выводится `42` в `kernel.c`

Ядро пишет напрямую в видеопамять `0xB8000`:

- Каждая ячейка экрана: 16 бит.
- Младшие 8 бит: ASCII символ.
- Старшие 8 бит: атрибут цвета.
- `clear_screen()` заполняет все 80x25 пробелами.
- `print_at("42", 0, 0, color)` записывает `'4'` и `'2'` в первые две ячейки.

Результат: `42` в левом верхнем углу.

### 8. Зачем бесконечный `hlt` цикл

После инициализации выполняется:

```c
while (1) {
	__asm__ __volatile__("hlt");
}
```

Так ядро не возвращается в недопустимый контекст и безопасно держит CPU в idle.

### 9. Флаги сборки и их смысл

Ключевые freestanding-флаги из `Makefile`:

- `-ffreestanding` - компиляция для freestanding среды.
- `-fno-builtin` - не подменять функции на builtin-версии компилятора.
- `-fno-exceptions` - отключить модель исключений.
- `-fno-stack-protector` - убрать зависимость от host stack canary runtime.
- `-nostdlib -nodefaultlibs -nostartfiles` - не использовать host CRT и стандартные библиотеки.
- `-Wall -Wextra -Werror` - строгая политика предупреждений.

Это гарантирует, что ядро не линковается с библиотеками хостовой системы.

### 10. Короткий текст для устной защиты (30 секунд)

"GRUB загружает мое multiboot-совместимое ядро и передает управление в `_start`.
В `boot.asm` я поднимаю стек 16 KiB, сохраняю multiboot-значения и вызываю `kernel_main`.
Далее ядро напрямую пишет в VGA-память `0xB8000`: очищает экран и выводит `42`.
Линковка выполняется через мой собственный `linker.ld` с базой 1 MiB и явной раскладкой
секций, без каких-либо библиотек хоста благодаря `-nostdlib` и `-nodefaultlibs`."

## License

42 School Project

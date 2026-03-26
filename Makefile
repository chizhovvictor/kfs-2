# Makefile for KFS-1 kernel

# Compiler and tools
ASM = nasm
CC = gcc
LD = ld

# Directories
BUILD_DIR = build
ISO_DIR = iso
GRUB_DIR = $(ISO_DIR)/boot/grub

# Output files
KERNEL = $(BUILD_DIR)/kernel.bin
ISO = kfs-1.iso

# Source files
ASM_SRC = boot.asm
C_SRC = kernel.c
LINKER_SCRIPT = linker.ld

# Object files
ASM_OBJ = $(BUILD_DIR)/boot.o
C_OBJ = $(BUILD_DIR)/kernel.o
OBJS = $(ASM_OBJ) $(C_OBJ)

# Compiler flags
ASM_FLAGS = -f elf32
CC_FLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra -Werror \
           -fno-builtin -fno-exceptions -fno-stack-protector \
           -nostdlib -nostartfiles -nodefaultlibs
LD_FLAGS = -m elf_i386 -T $(LINKER_SCRIPT) -nostdlib

# Default target
all: $(ISO)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GRUB_DIR):
	mkdir -p $(GRUB_DIR)

# Compile assembly
$(ASM_OBJ): $(ASM_SRC) | $(BUILD_DIR)
	$(ASM) $(ASM_FLAGS) $(ASM_SRC) -o $(ASM_OBJ)

# Compile C
$(C_OBJ): $(C_SRC) kernel.h | $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(C_SRC) -o $(C_OBJ)

# Link kernel
$(KERNEL): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(LD_FLAGS) $(OBJS) -o $(KERNEL)

# Create GRUB config
$(GRUB_DIR)/grub.cfg: | $(GRUB_DIR)
	@echo "menuentry \"KFS-1\" {" > $(GRUB_DIR)/grub.cfg
	@echo "    multiboot /boot/kernel.bin" >> $(GRUB_DIR)/grub.cfg
	@echo "}" >> $(GRUB_DIR)/grub.cfg

# Copy kernel to ISO directory
$(ISO_DIR)/boot/kernel.bin: $(KERNEL) | $(GRUB_DIR)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin

# Create ISO
$(ISO): $(ISO_DIR)/boot/kernel.bin $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)
	@echo "ISO created: $(ISO)"

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO)

# Rebuild everything
re: clean all

# Run with QEMU (optional)
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

.PHONY: all clean re run

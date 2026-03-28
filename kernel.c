/* kernel.c - Main kernel code */

#include "kernel.h"
#include <stdarg.h>

/* VGA text mode buffer */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA color codes */
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Multiboot info flags */
#define MBI_FLAG_MEM_MAP (1u << 6)

/* Global text cursor for printk-like output */
static int g_cursor_pos = 0;

/* GDTR layout in 32-bit protected mode */
struct gdtr32 {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
} __attribute__((packed));



/* Create VGA entry with character and color */
static inline unsigned short vga_entry(unsigned char ch, unsigned char color) {
    return (unsigned short) ch | (unsigned short) color << 8;
}

/* Create color byte from foreground and background */
static inline unsigned char vga_color(unsigned char fg, unsigned char bg) {
    return fg | bg << 4;
}

/* Write a single character and keep cursor state */
static void put_char(char ch, unsigned char color) {
    unsigned short *vga_buffer = (unsigned short *) VGA_MEMORY;

    if (ch == '\n') {
        g_cursor_pos = (g_cursor_pos / VGA_WIDTH + 1) * VGA_WIDTH;
    } else {
        vga_buffer[g_cursor_pos++] = vga_entry((unsigned char) ch, color);
    }

    if (g_cursor_pos >= VGA_WIDTH * VGA_HEIGHT) {
        g_cursor_pos = 0;
    }
}

/* Print a 32-bit value as hexadecimal */
static void print_hex32(uint32_t value) {
    static const char hex[] = "0123456789ABCDEF";
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    put_char('0', color);
    put_char('x', color);
    for (int shift = 28; shift >= 0; shift -= 4) {
        put_char(hex[(value >> shift) & 0xF], color);
    }
}

static void print_u32(uint32_t value);

/* Print signed 32-bit integer in decimal */
static void print_i32(int32_t value) {
    if (value < 0) {
        put_char('-', vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print_u32((uint32_t) (-value));
        return;
    }
    print_u32((uint32_t) value);
}

/* Print 32-bit unsigned integer in decimal */
static void print_u32(uint32_t value) {
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    char buffer[10];
    int index = 0;

    if (value == 0) {
        put_char('0', color);
        return;
    }

    while (value > 0 && index < 10) {
        buffer[index++] = (char) ('0' + (value % 10));
        value /= 10;
    }

    while (index > 0) {
        put_char(buffer[--index], color);
    }
}

/* Minimal kernel printf for early debugging */
// void printk(const char *fmt, ...) {
//     va_list args;

//     va_start(args, fmt);

//     for (uint32_t i = 0; fmt[i] != '\0'; i++) {
//         if (fmt[i] != '%') {
//             put_char(fmt[i], vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
//             continue;
//         }

//         i++;
//         if (fmt[i] == '\0') {
//             break;
//         }

//         if (fmt[i] == '%') {
//             put_char('%', vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
//         } else if (fmt[i] == 'c') {
//             put_char((char) va_arg(args, int), vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
//         } else if (fmt[i] == 's') {
//             const char *str = va_arg(args, const char *);
//             if (str == 0) {
//                 print("(null)");
//             } else {
//                 print(str);
//             }
//         } else if (fmt[i] == 'u') {
//             print_u32(va_arg(args, uint32_t));
//         } else if (fmt[i] == 'd') {
//             print_i32(va_arg(args, int32_t));
//         } else if (fmt[i] == 'x') {
//             print_hex32(va_arg(args, uint32_t));
//         } else {
//             put_char('%', vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
//             put_char(fmt[i], vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
//         }
//     }

//     va_end(args);
// }

static const char *mmap_type_to_string(uint32_t type) {
    if (type == 1) {
        return "usable";
    }
    if (type == 3) {
        return "acpi";
    }
    if (type == 4) {
        return "nvs";
    }
    if (type == 5) {
        return "bad";
    }
    return "reserved";
}

/* Basic string length helper for freestanding kernel */
uint32_t strlen(const char *str) {
    uint32_t len = 0;

    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/* Clear the screen */
void clear_screen(void) {
    unsigned short *vga_buffer = (unsigned short *) VGA_MEMORY;
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', color);
    }

    g_cursor_pos = 0;
}

/* Print a string at specific position */
void print_at(const char *str, int x, int y, unsigned char color) {
    unsigned short *vga_buffer = (unsigned short *) VGA_MEMORY;
    int pos = y * VGA_WIDTH + x;
    
    for (int i = 0; str[i] != '\0'; i++) {
        vga_buffer[pos + i] = vga_entry(str[i], color);
    }
}

/* Print a string at current position */
void print(const char *str) {
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i], color);
    }
}

/* Print current kernel stack in a compact, human-friendly table */
void dump_kernel_stack(uint32_t words) {
    uint32_t *esp_ptr;
    uint32_t *ebp_ptr;

    __asm__ __volatile__("mov %%esp, %0" : "=r" (esp_ptr));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (ebp_ptr));

    printk("\n== STACK ==\n");
    printk("ESP=%x EBP=%x\n", (uint32_t) esp_ptr, (uint32_t) ebp_ptr);
    printk("#  addr        value\n");

    for (uint32_t i = 0; i < words; i++) {
        printk("%u  %x  %x\n", i, (uint32_t) (esp_ptr + i), esp_ptr[i]);
    }
}

/* Print memory map from Multiboot in a readable row format */
static void dump_memory_map(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    printk("\n== MEMORY MAP ==\n");

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Bad multiboot magic: %x\n", multiboot_magic);
        return;
    }

    struct multiboot_info *mbi = (struct multiboot_info *) multiboot_info_addr;

    if ((mbi->flags & MBI_FLAG_MEM_MAP) == 0) {
        printk("No mmap in multiboot info\n");
        return;
    }

    printk("idx  base_hi   base_lo   len_hi    len_lo    type\n");

    uint32_t index = 0;
    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
    uint32_t mmap_ptr = mbi->mmap_addr;

    while (mmap_ptr < mmap_end) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *) mmap_ptr;

        printk("%u    %x %x %x %x %s\n",
               index++,
               entry->addr_high,
               entry->addr_low,
               entry->len_high,
               entry->len_low,
               mmap_type_to_string(entry->type));

        mmap_ptr += entry->size + sizeof(entry->size);
    }
}


/* Print current GDTR values to verify GDT base address */
static void dump_gdt_info(void) {
    struct gdtr32 gdtr;

    __asm__ __volatile__("sgdt %0" : "=m" (gdtr));

    printk("GDTR.base=%x GDTR.limit=%x\n", gdtr.base, (uint32_t) gdtr.limit);
}

/* Kernel main function */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    /* Clear the screen */
    clear_screen();

    printk("KFS GDT ready\n");
    dump_gdt_info();
    dump_kernel_stack(8);
    dump_memory_map(multiboot_magic, multiboot_info_addr);
    
    /* Hang forever */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

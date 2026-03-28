/* kernel.c - Main kernel code */

#include "kernel.h"

/* VGA text mode buffer */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA color codes */
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15

/* Global text cursor for printk-like output */
static int g_cursor_pos = 0;

/* GDTR layout in 32-bit protected mode */
struct gdtr32 {
    uint16_t limit;
    uint32_t base;
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

/* Print an unsigned integer in decimal */
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

/* Print current kernel stack in a readable tabular form */
void dump_kernel_stack(uint32_t words) {
    uint32_t *esp_ptr;
    uint32_t *ebp_ptr;

    __asm__ __volatile__("mov %%esp, %0" : "=r" (esp_ptr));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (ebp_ptr));

    print("\n=== KERNEL STACK DUMP ===\n");
    print("ESP=");
    print_hex32((uint32_t) esp_ptr);
    print(" EBP=");
    print_hex32((uint32_t) ebp_ptr);
    print("\nIdx Addr         Value\n");

    for (uint32_t i = 0; i < words; i++) {
        print_u32(i);
        print("   ");
        print_hex32((uint32_t) (esp_ptr + i));
        print("   ");
        print_hex32(esp_ptr[i]);
        print("\n");
    }
}

/* Print current GDTR values to verify GDT base address */
static void dump_gdt_info(void) {
    struct gdtr32 gdtr;

    __asm__ __volatile__("sgdt %0" : "=m" (gdtr));

    print("GDTR.base=");
    print_hex32(gdtr.base);
    print(" GDTR.limit=");
    print_hex32((uint32_t) gdtr.limit);
    print("\n");
}

/* Kernel main function */
void kernel_main() {
    /* Clear the screen */
    clear_screen();
    
    /* Display "42" on the screen */
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_at("42", 0, 0, color);

    /* Keep the first line for "42" and continue logs from line 2 */
    g_cursor_pos = VGA_WIDTH;

    print("KFS GDT ready\n");
    dump_gdt_info();
    dump_kernel_stack(12);
    
    /* Hang forever */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

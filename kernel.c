/* kernel.c - Main kernel code */

#include "kernel.h"

/* VGA text mode buffer */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA color codes */
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15

/* Create VGA entry with character and color */
static inline unsigned short vga_entry(unsigned char ch, unsigned char color) {
    return (unsigned short) ch | (unsigned short) color << 8;
}

/* Create color byte from foreground and background */
static inline unsigned char vga_color(unsigned char fg, unsigned char bg) {
    return fg | bg << 4;
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
    unsigned short *vga_buffer = (unsigned short *) VGA_MEMORY;
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    static int pos = 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            pos = (pos / VGA_WIDTH + 1) * VGA_WIDTH;
        } else {
            vga_buffer[pos++] = vga_entry(str[i], color);
        }
        
        if (pos >= VGA_WIDTH * VGA_HEIGHT) {
            pos = 0;
        }
    }
}

/* Kernel main function */
void kernel_main() {
    /* Clear the screen */
    clear_screen();
    
    /* Display "42" on the screen */
    unsigned char color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_at("42", 0, 0, color);
    
    /* Hang forever */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

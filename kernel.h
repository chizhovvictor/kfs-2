/* kernel.h - Kernel header file */

#ifndef KERNEL_H
#define KERNEL_H

/* Basic types */
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
typedef int            int32_t;
typedef short          int16_t;
typedef char           int8_t;

/* Kernel functions */
uint32_t strlen(const char *str);
void clear_screen(void);
// void print(const char *str);
// void printk(const char *fmt, ...);
// void print_at(const char *str, int x, int y, unsigned char color);
void dump_kernel_stack(uint32_t words);
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr);


#endif /* KERNEL_H */

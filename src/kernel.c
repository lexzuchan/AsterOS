#include <stdint.h>
#include <stddef.h>

/* AsterOS: tiny 32-bit educational kernel. Hardware: BIOS/GRUB, VGA text, PS/2. */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define INPUT_MAX 78
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u

static volatile uint16_t *const vga = (uint16_t *)0xB8000;
static unsigned row = 0, col = 0;
static uint8_t fg = 15, bg = 1;
static char input[INPUT_MAX + 1];
static size_t input_len = 0;
static uint32_t detected_memory_kb = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}
static size_t str_len(const char *s) { size_t n=0; while (s[n]) ++n; return n; }
static int str_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static int starts_with(const char *s, const char *prefix) {
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}
static uint16_t cell(char c) {
    return ((uint16_t)((bg << 4) | fg) << 8) | (uint8_t)c;
}
static void scroll(void) {
    if (row < VGA_HEIGHT) return;
    for (unsigned y = 1; y < VGA_HEIGHT; ++y)
        for (unsigned x = 0; x < VGA_WIDTH; ++x)
            vga[(y-1)*VGA_WIDTH+x] = vga[y*VGA_WIDTH+x];
    for (unsigned x = 0; x < VGA_WIDTH; ++x)
        vga[(VGA_HEIGHT-1)*VGA_WIDTH+x] = cell(' ');
    row = VGA_HEIGHT - 1;
}
static void clear(void) {
    for (unsigned i=0; i<VGA_WIDTH*VGA_HEIGHT; ++i) vga[i] = cell(' ');
    row = 0; col = 0;
}
static void putc(char c) {
    if (c == '\n') { col = 0; ++row; scroll(); return; }
    if (c == '\r') { col = 0; return; }
    if (c == '\b') {
        if (col > 0) --col;
        else if (row > 0) { --row; col = VGA_WIDTH-1; }
        vga[row*VGA_WIDTH+col] = cell(' ');
        return;
    }
    vga[row*VGA_WIDTH+col] = cell(c);
    if (++col >= VGA_WIDTH) { col = 0; ++row; scroll(); }
}
static void print(const char *s) { while (*s) putc(*s++); }
static void print_uint(uint32_t n) {
    char buf[11]; unsigned i=0;
    do { buf[i++] = '0' + n%10; n/=10; } while(n);
    while(i) putc(buf[--i]);
}
static void prompt(void) { print("\naster> "); }

/* US keyboard scancode set 1, suitable for common VM PS/2 emulation. */
static const char keymap[128] = {
    [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
    [12]='-',[13]='=',[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',
    [23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[30]='a',[31]='s',[32]='d',[33]='f',
    [34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',
    [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',
    [53]='/',[57]=' '
};
static const char shifted[128] = {
    [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',
    [12]='_',[13]='+',[16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',[21]='Y',[22]='U',
    [23]='I',[24]='O',[25]='P',[26]='{',[27]='}',[30]='A',[31]='S',[32]='D',[33]='F',
    [34]='G',[35]='H',[36]='J',[37]='K',[38]='L',[39]=':',[40]='"',[41]='~',[43]='|',
    [44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',[49]='N',[50]='M',[51]='<',[52]='>',
    [53]='?',[57]=' '
};
static int shift_down = 0, caps_lock = 0, extended = 0;
static char keyboard_poll(void) {
    if (!(inb(0x64) & 1)) return 0;
    uint8_t sc = inb(0x60);
    if (sc == 0xE0 || sc == 0xE1) { extended = 1; return 0; }
    if (extended) { extended = 0; return 0; }
    if (sc == 0x2A || sc == 0x36) { shift_down = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift_down = 0; return 0; }
    if (sc & 0x80) return 0;
    if (sc == 0x3A) { caps_lock = !caps_lock; return 0; }
    if (sc == 0x1C) return '\n';
    if (sc == 0x0E) return '\b';
    if (sc >= 128) return 0;
    char ch = shift_down ? shifted[sc] : keymap[sc];
    if (caps_lock && ch >= 'a' && ch <= 'z') ch -= 32;
    else if (caps_lock && ch >= 'A' && ch <= 'Z') ch += 32;
    return ch;
}

static void reboot(void) {
    /* Wait until i8042 input buffer is empty, then pulse CPU reset. */
    for (unsigned i=0; i<100000; ++i) {
        if (!(inb(0x64) & 2)) { outb(0x64, 0xFE); break; }
    }
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}
static void halt(void) {
    print("\nSystem halted. Power off the virtual machine.\n");
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}
static void execute(const char *cmd) {
    if (!*cmd) return;
    if (str_eq(cmd,"help")) {
        print("Commands:\n  help     Show commands\n  clear    Clear screen\n");
        print("  about    System info\n  mem      Show detected RAM\n");
        print("  echo X   Print text\n  color N  Change foreground (0-15)\n");
        print("  reboot   Restart VM\n  halt     Stop CPU\n");
    } else if (str_eq(cmd,"clear")) {
        clear(); print("AsterOS / terminal ready\n");
    } else if (str_eq(cmd,"about")) {
        print("AsterOS 0.1 | x86 32-bit | GRUB Multiboot | VGA/PS2\n");
        print("A minimal learning OS, not a Linux/Windows replacement.\n");
    } else if (str_eq(cmd,"mem")) {
        print("Usable memory reported above 1 MiB: ");
        print_uint(detected_memory_kb / 1024); print(" MiB (approx.)\n");
    } else if (starts_with(cmd,"echo ")) {
        print(cmd + 5); putc('\n');
    } else if (starts_with(cmd,"color ") && str_len(cmd)==7 && cmd[6]>='0' && cmd[6]<='9') {
        fg = (uint8_t)(cmd[6]-'0'); print("Color changed.\n");
    } else if (starts_with(cmd,"color ") && str_len(cmd)==8 && cmd[6]=='1' && cmd[7]>='0' && cmd[7]<='5') {
        fg = (uint8_t)(10+cmd[7]-'0'); print("Color changed.\n");
    } else if (str_eq(cmd,"reboot")) {
        reboot();
    } else if (str_eq(cmd,"halt")) {
        halt();
    } else {
        print("Unknown command: "); print(cmd); print(" (try 'help')\n");
    }
}

/* Multiboot information first words: flags, mem_lower, mem_upper. */
struct multiboot_info_prefix { uint32_t flags, mem_lower, mem_upper; };
void kernel_main(uint32_t magic, uint32_t mbi_ptr) {
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi_ptr) {
        const struct multiboot_info_prefix *mbi = (const void *)(uintptr_t)mbi_ptr;
        if (mbi->flags & 1u) detected_memory_kb = mbi->mem_upper;
    }
    clear();
    print("===============================================================================\n");
    print("   ASTER OS                                     version 0.1  |  32-bit x86\n");
    print("===============================================================================\n\n");
    print("Welcome to AsterOS!  A small operating system running inside your VM.\n");
    print("Type 'help' for commands. Keyboard: US layout, PS/2.\n");
    prompt();
    for (;;) {
        char c = keyboard_poll();
        if (!c) continue;
        if (c == '\n') {
            putc('\n'); input[input_len] = '\0';
            execute(input); input_len = 0; prompt();
        } else if (c == '\b') {
            if (input_len) { --input_len; putc('\b'); }
        } else if (c >= 32 && c <= 126 && input_len < INPUT_MAX) {
            input[input_len++] = c; putc(c);
        }
    }
}

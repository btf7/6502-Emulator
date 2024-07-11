#define FLAG_NEGATIVE  0b10000000
#define FLAG_OVERFLOW  0b01000000
#define FLAG_IGNORE    0b00110000
// #define FLAG_BREAK     0b00010000 // This is 1 when pushed from software, 0 when pushed from hardware. This will never be pushed from hardware
#define FLAG_DECIMAL   0b00001000
#define FLAG_INTERRUPT 0b00000100
#define FLAG_ZERO      0b00000010
#define FLAG_CARRY     0b00000001
#include <stdint.h>

void pushStack(uint8_t val);
uint8_t getStatus(void);
uint16_t readWord(uint16_t pointer);
void writeByte(uint16_t pointer, uint8_t byte);
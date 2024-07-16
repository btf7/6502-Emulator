#include <stdint.h>
#include <stdbool.h>

extern uint8_t mem[];
extern uint16_t PC;
extern uint8_t SP; // Grows down
extern uint8_t AC;
extern uint8_t X;
extern uint8_t Y;

extern bool negativeFlag;
extern bool overflowFlag;
extern bool decimalFlag;
extern bool interruptFlag;
extern bool zeroFlag;
extern bool carryFlag;

extern bool drawQueued;

void readFile(const char* fileName);
void runInstruction(void);
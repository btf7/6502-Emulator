#include <stdint.h>

uint16_t readWord(uint16_t pointer);

void ADC(uint16_t pointer);
void AND(uint16_t pointer);
void ASL(uint16_t pointer);
void ASLA(void);
void BCC(uint16_t pointer);
void BCS(uint16_t pointer);
void BEQ(uint16_t pointer);
void BIT(uint16_t pointer);
void BMI(uint16_t pointer);
void BNE(uint16_t pointer);
void BPL(uint16_t pointer);
void BRK(void);
void BVC(uint16_t pointer);
void BVS(uint16_t pointer);
void CLC(void);
void CLD(void);
void CLI(void);
void CLV(void);
void CMP(uint16_t pointer);
void CPX(uint16_t pointer);
void CPY(uint16_t pointer);
void DEC(uint16_t pointer);
void DEX(void);
void DEY(void);
void EOR(uint16_t pointer);
void INC(uint16_t pointer);
void INX(void);
void INY(void);
void JMP(uint16_t pointer);
void JSR(uint16_t pointer);
void LDA(uint16_t pointer);
void LDX(uint16_t pointer);
void LDY(uint16_t pointer);
void LSR(uint16_t pointer);
void LSRA(void);
void NOP(void);
void ORA(uint16_t pointer);
void PHA(void);
void PHP(void);
void PLA(void);
void PLP(void);
void ROL(uint16_t pointer);
void ROLA(void);
void ROR(uint16_t pointer);
void RORA(void);
void RTI(void);
void RTS(void);
void SBC(uint16_t pointer);
void SEC(void);
void SED(void);
void SEI(void);
void STA(uint16_t pointer);
void STX(uint16_t pointer);
void STY(uint16_t pointer);
void TAX(void);
void TAY(void);
void TSX(void);
void TXA(void);
void TXS(void);
void TYA(void);
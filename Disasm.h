#ifndef X86EMU_DISASM
#define X86EMU_DISASM

#include <inttypes.h>
#include <string>

// forward declarations
class CpuInterface;
class Memory;

class Disasm
{
public:
    // constructor & destructor
    Disasm(CpuInterface& cpu, Memory& memory);
    ~Disasm() = default;

    // public methods
    std::string Process();

private:
    static const char* s_regName16[];
    static const char* s_regName8l[];
    static const char* s_regName8h[];
    static const char* s_regName8[];
    static const char* s_sregName[];
    static const char  s_hexDigits[];
    static const int   s_modRmInstLen[256];

    CpuInterface& m_cpu;
    uint8_t*      m_memory;

    // private methods
    std::string Reg16ToStr(uint8_t reg);
    std::string Reg8LowToStr(uint8_t reg);
    std::string Reg8HighToStr(uint8_t reg);

    std::string Hex16(uint16_t value);
    std::string Hex8(uint8_t value);
    std::string Dec8(int8_t value);

    std::string Disp16(uint8_t* ip);
    std::string Disp8(uint8_t* ip);
    std::string Imm16(uint8_t* ip);
    std::string Imm8(uint8_t* ip);

    std::string ModRmReg16(uint8_t* ip);
    std::string ModRmReg8(uint8_t* ip);
    std::string ModRmSReg(uint8_t* ip);
    std::string ModRm(uint8_t* ip);

    std::string DumpMem(uint16_t segment, uint16_t offset, int length);
};

#endif /* X86EMU_DISASM */

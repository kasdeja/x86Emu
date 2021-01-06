#ifndef X86EMU_CPU_MODEL_1
#define X86EMU_CPU_MODEL_1

#include<inttypes.h>

namespace CpuModel1
{

class Cpu
{
public:
    enum Register
    {
        AX = 0x00,
        CX = 0x01,
        BX = 0x02,
        DX = 0x03,
        SP = 0x04,
        BP = 0x05,
        SI = 0x06,
        DI = 0x07,

        AH = 0x80,
        CH = 0x81,
        BH = 0x82,
        DH = 0x83,

        ES = 0x08,
        CS = 0x09,
        SS = 0x0a,
        DS = 0x0b,
        IP = 0x0c,

        FLAGS = 0x0d
    };

    // constructor & destructor
    Cpu(uint8_t* memory);
    ~Cpu();

    // public methods
    void SetReg(Register reg, uint32_t value);
    void Run(int nCycles);

private:
    uint16_t m_register[16];
    uint8_t* m_memory;

    void ExecuteInstruction();
};

} // CpuModel1

#endif /* X86EMU_CPU_MODEL_1 */

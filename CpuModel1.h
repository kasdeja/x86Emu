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
        DX = 0x02,
        BX = 0x03,
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
    enum State
    {
        InvalidOp       = 1,
        SegmentOverride = 2
    };

    uint16_t m_register[16];
    uint16_t m_segment;
    uint16_t m_stackSegment;
    uint8_t* m_memory;
    uint32_t m_state;

    uint16_t* Reg(uint8_t *ip);
    uint16_t* SReg(uint8_t *ip);
    uint16_t* Mem16(uint16_t segment, uint16_t offset);
    uint8_t*  Mem8(uint16_t segment, uint16_t offset);
    void      Push16(uint16_t value);
    uint16_t  Pop16();
    uint16_t  LoadU16(uint8_t* ip);
    int8_t    LoadS8(uint8_t* ip);
    int       ModRm(uint8_t* ip, uint16_t **op);
    void      ExecuteInstruction();
};

} // CpuModel1

#endif /* X86EMU_CPU_MODEL_1 */

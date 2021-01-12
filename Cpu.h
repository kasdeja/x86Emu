#ifndef X86EMU_CPU
#define X86EMU_CPU

#include <inttypes.h>
#include <memory>
#include "CpuInterface.h"

// forward declarations
class Memory;

class Cpu : public CpuInterface
{
public:
    // constructor & destructor
    Cpu(Memory& memory);
    ~Cpu();

    // public methods
    void SetReg16(Reg16 reg, uint16_t value) override;
    void SetReg8(Reg8 reg, uint8_t value) override;

    uint16_t GetReg16(Reg16 reg) override;
    uint8_t GetReg8(Reg8 reg) override;

    void Run(int nCycles) override;

private:
    static uint16_t s_modRmInstLen[256];

    enum State
    {
        InvalidOp       = 1,
        SegmentOverride = 2
    };

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

        ES = 0x08,
        CS = 0x09,
        SS = 0x0a,
        DS = 0x0b,
        IP = 0x0c,

        FLAG = 0x0d
    };

    enum Flag
    {
        CF      =  0, //1,     // bit  0
        Always1 =  1, //2,     // bit  1
        PF      =  2, //4,     // bit  2
        AF      =  4, //16,    // bit  4
        ZF      =  6, //64,    // bit  6
        SF      =  7, //128,   // bit  7
        TF      =  8, //256,   // bit  8
        IF      =  9, //512,   // bit  9
        DF      = 10, //1024,  // bit 10
        OF      = 11, //2048,  // bit 11
        NT      = 14, //16384, // bit 14
        Always0 = 15  //32768  // bit 15
    };

    uint16_t    m_register[16];
    std::size_t m_segmentBase;
    std::size_t m_stackSegmentBase;
    uint8_t*    m_memory;
    uint32_t    m_state;
    uint32_t    m_lastResult;

    uint16_t* ModRmToReg(uint8_t modrm);
    uint16_t* ModRmToSReg(uint8_t modrm);
    uint16_t  Disp16(uint8_t* ip);
    int8_t    Disp8(uint8_t* ip);
    uint16_t  Imm16(uint8_t* ip);
    uint16_t  Load16(std::size_t linearAddr);
    uint8_t   Load8(std::size_t linearAddr);
    void      Store16(std::size_t linearAddr, uint16_t value);
    void      Store8(std::size_t linearAddr, uint8_t value);
    void      Push16(uint16_t value);
    uint16_t  Pop16();
    void      RecalcFlags();

    uint16_t  ModRmLoad16(uint8_t *ip);
    uint8_t   ModRmLoad8(uint8_t *ip);
    void      ModRmStore16(uint8_t *ip, uint16_t value);
    void      ModRmStore8(uint8_t *ip, uint8_t value);

    template<typename F> void ModRmModifyOp16(uint8_t *ip, uint16_t regValue, F&& f);
    template<typename F> void ModRmModifyOp8(uint8_t *ip, uint8_t regValue, F&& f);

    void      ExecuteInstruction();
};

#endif /* X86EMU_CPU */

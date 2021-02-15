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
    void SetReg16(Register16 reg, uint16_t value) override;
    void SetReg8(Register8 reg, uint8_t value) override;

    uint16_t GetReg16(Register16 reg) override;
    uint8_t GetReg8(Register8 reg) override;

    Memory& GetMem() override;

    void Run(int nCycles) override;

private:
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

    struct Flag
    {
        enum Bit
        {
            CF_bit      = 0,     // 1
            Always1_bit = 1,     // 2
            PF_bit      = 2,     // 4
            AF_bit      = 4,     // 16
            ZF_bit      = 6,     // 64
            SF_bit      = 7,     // 128
            TF_bit      = 8,     // 256
            IF_bit      = 9,     // 512
            DF_bit      = 10,    // 1024
            OF_bit      = 11,    // 2048
            NT_bit      = 14,    // 16384
            Always0_bit = 15     // 32768
        };

        enum Mask
        {
            CF_mask      = 1,     // bit 0
            Always1_mask = 2,     // bit 1
            PF_mask      = 4,     // bit 2
            AF_mask      = 16,    // bit 4
            ZF_mask      = 64,    // bit 6
            SF_mask      = 128,   // bit 7
            TF_mask      = 256,   // bit 8
            IF_mask      = 512,   // bit 9
            DF_mask      = 1024,  // bit 10
            OF_mask      = 2048,  // bit 11
            NT_mask      = 16384, // bit 14
            Always0_mask = 32768  // bit 15
        };
    };

    struct Aux
    {
        enum Bit
        {
            CF_bit  = 31,
            PO_bit  = 30,
            PDB_bit = 8,
            AF_bit  = 3
        };

        enum Mask
        {
            CF_mask  = 0x80000000,
            PO_mask  = 0x40000000,
            PDB_mask = 0x0000ff00,
            AF_mask  = 0x00000008,
            SFD_mask = 0x00000001
        };
    };

    static uint16_t s_modRmInstLen[256];

    uint16_t    m_register[16];
    uint8_t*    m_memory;
    std::size_t m_segmentBase;
    std::size_t m_stackSegmentBase;
    uint32_t    m_state;

    int         m_result;
    int         m_auxbits;

    Memory&     m_rMemory;

    uint16_t* Reg16(uint8_t modrm);
    uint8_t*  Reg8(uint8_t modrm);
    uint16_t* SReg(uint8_t modrm);

    uint16_t  Disp16(uint8_t* ip);
    int8_t    Disp8(uint8_t* ip);
    uint16_t  Imm16(uint8_t* ip);

    uint16_t  Load16(std::size_t linearAddr);
    uint8_t   Load8(std::size_t linearAddr);
    void      Store16(std::size_t linearAddr, uint16_t value);
    void      Store8(std::size_t linearAddr, uint8_t value);

    void      Push16(uint16_t value);
    uint16_t  Pop16();

    bool GetCF();
    bool GetPF();
    bool GetAF();
    bool GetZF();
    bool GetSF();
    bool GetOF();

    void SetOF_CF(bool of, bool cf);
    void SetCF(bool val);
    void SetPF(bool val);
    void SetAF(bool val);
    void SetZF(bool val);
    void SetSF(bool val);
    void SetOF(bool val);

    void SetSubFlags16(uint16_t op1, uint16_t op2, uint16_t result);
    void SetSubFlags8(uint8_t op1, uint8_t op2, uint8_t result);
    void SetAddFlags16(uint16_t op1, uint16_t op2, uint16_t result);
    void SetAddFlags8(uint8_t op1, uint8_t op2, uint8_t result);
    void SetLogicFlags16(uint16_t result);
    void SetLogicFlags8(uint8_t result);

    void RecalcFlags();
    void RestoreLazyFlags();

    uint16_t  ModRmLoad16(uint8_t *ip);
    uint8_t   ModRmLoad8(uint8_t *ip);

    uint16_t  ModRmLoadNoDisp16(uint8_t *ip);
    uint8_t   ModRmLoadNoDisp8(uint8_t *ip);

    void      ModRmStore16(uint8_t *ip, uint16_t value);
    void      ModRmStore8(uint8_t *ip, uint8_t value);

    template<typename F> void ModRmLoadOp16(uint8_t *ip, F&& f);
    template<typename F> void ModRmLoadOp8(uint8_t *ip, F&& f);

    template<typename F> void ModRmModifyOp16(uint8_t *ip, F&& f);
    template<typename F> void ModRmModifyOp8(uint8_t *ip, F&& f);

    template<typename F> void ModRmModifyOpNoReg16(uint8_t *ip, F&& f);
    template<typename F> void ModRmModifyOpNoReg8(uint8_t *ip, F&& f);

    template<typename F> void ModRmModifyOpNoDisp16(uint8_t *ip, F&& f);
    template<typename F> void ModRmModifyOpNoDisp8(uint8_t *ip, F&& f);

    void HandleREPNE(uint8_t opcode);
    void Handle8xCommon(uint8_t* ip, uint16_t op2);
    void Handle80h(uint8_t* ip);
    void Handle81h(uint8_t* ip);
    void Handle83h(uint8_t* ip);
    void HandleF6h(uint8_t* ip);
    void HandleF7h(uint8_t* ip);
    void HandleShift16(uint8_t* ip, uint8_t shift);
    void HandleShift8(uint8_t* ip, uint8_t shift);

    void ExecuteInstruction();
};

#endif /* X86EMU_CPU */

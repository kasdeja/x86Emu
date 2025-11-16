#ifndef X86EMU_CPU_INTERFACE
#define X86EMU_CPU_INTERFACE

#include <inttypes.h>
#include <functional>

// forward declarations
class Memory;

class CpuInterface
{
public:
    enum Register16
    {
        AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS, IP
    };

    enum Register8
    {
        AL, BL, CL, DL, AH, BH, CH, DH
    };

    enum Flag
    {
        CF, PF, AF, ZF, SF, TF, IF, DF, OF, NT
    };

    virtual void SetReg16(Register16 reg, uint16_t value) = 0;
    virtual void SetReg8(Register8 reg, uint8_t value) = 0;

    virtual uint16_t GetReg16(Register16 reg) = 0;
    virtual uint8_t GetReg8(Register8 reg) = 0;

    virtual void SetFlag(Flag flag, bool value) = 0;
    virtual bool GetFlag(Flag flag) = 0;

    virtual Memory& GetMem() = 0;

    virtual bool Run(int nCycles) = 0;
    virtual void Interrupt(int num) = 0;
    virtual bool HardwareInterrupt(int num) = 0;

    std::function<void     (int intNo)>                               onInterrupt;
    std::function<uint32_t (uint16_t port, int size)>                 onPortRead;
    std::function<void     (uint16_t port, int size, uint32_t value)> onPortWrite;
    std::function<uint8_t  (uint32_t addr)>                           onVgaMemRead;
    std::function<void     (uint32_t addr, uint8_t value)>            onVgaMemWrite;
};

#endif /* X86EMU_CPU_INTERFACE */

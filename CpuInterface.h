#ifndef X86EMU_CPU_INTERFACE
#define X86EMU_CPU_INTERFACE

#include <inttypes.h>
#include <functional>

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

    virtual void SetReg16(Register16 reg, uint16_t value) = 0;
    virtual void SetReg8(Register8 reg, uint8_t value) = 0;

    virtual uint16_t GetReg16(Register16 reg) = 0;
    virtual uint8_t GetReg8(Register8 reg) = 0;

    virtual void Run(int nCycles) = 0;

    std::function<void (CpuInterface *cpu, int irq)> onSoftIrq;
};

#endif /* X86EMU_CPU_INTERFACE */

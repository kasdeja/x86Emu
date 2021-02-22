#include "CpuInterface.h"
#include "Memory.h"
#include "Bios.h"

// constructor & destructor
Bios::Bios(Memory& memory)
    : m_memory(memory.GetMem())
{
}

Bios::~Bios()
{
}

// public methods
void Bios::Int1Ah(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Read system-timer time count
        {
            uint32_t tickCount = 12 * 60 * 60 * 18.2;

            cpu->SetReg16(CpuInterface::CX, tickCount >> 16);
            cpu->SetReg16(CpuInterface::DX, tickCount &  0xffff);
            cpu->SetReg8(CpuInterface::AL, 0);

            break;
        }

        default:
            printf("Bios::Int1Ah() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

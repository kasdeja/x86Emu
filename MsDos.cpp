#include <stdio.h>
#include "MsDos.h"
#include "CpuInterface.h"

// constructor & destructor
MsDos::MsDos(uint8_t* memory)
    : m_memory(memory)
{
}

MsDos::~MsDos()
{
}

// public methods
void MsDos::Int21h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x25: // Get interrupt vector
        {
            uint8_t intr = cpu->GetReg8(CpuInterface::AL);

            *reinterpret_cast<uint16_t *>(m_memory + intr * 4)     = cpu->GetReg16(CpuInterface::DS);
            *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2) = cpu->GetReg16(CpuInterface::DX);
            break;
        }

        case 0x30:  // DOS Version
        {
            // DOS 5.0
            cpu->SetReg8(CpuInterface::AL, 5);
            cpu->SetReg8(CpuInterface::AH, 0);

            cpu->SetReg16(CpuInterface::BX, 0);
            cpu->SetReg16(CpuInterface::CX, 0);
            break;
        }

        case 0x35: // Get interrupt vector
        {
            uint8_t intr = cpu->GetReg8(CpuInterface::AL);

            cpu->SetReg16(CpuInterface::ES, *reinterpret_cast<uint16_t *>(m_memory + intr * 4));
            cpu->SetReg16(CpuInterface::BX, *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2));

            break;
        }

        default:
            printf("MsDos::Int21h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

#include <stdio.h>
#include <algorithm>
#include "CpuModel1.h"

using namespace CpuModel1;

// constructor & destructor
Cpu::Cpu(uint8_t *memory)
    : m_memory(memory)
{
    std::fill(m_register, m_register + 16, 0);
}

Cpu::~Cpu()
{
}

// public methods
void Cpu::SetReg(Register reg, uint32_t value)
{
    uint32_t idx = static_cast<uint32_t>(reg);

    if (idx & 0x80)
    {
        m_register[idx & 0x0f] = value;
    }
    else
    {
        m_register[idx] = value;
    }
}

void Cpu::Run(int nCycles)
{
    for(int n = 0; n < nCycles; n++)
    {
        ExecuteInstruction();
    }
}

void Cpu::ExecuteInstruction()
{
    uint8_t* ip = m_memory + m_register[Register::CS] * 16 + m_register[Register::IP];

    printf("%04x:%04x | ", m_register[Register::CS], m_register[Register::IP]);
    for(int n = 0; n < 8; n++)
        printf("%02x ", ip[n]);
    printf("\n");

}

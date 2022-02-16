#include "Pic.h"
#include "CpuInterface.h"

// constructor & destructor
Pic::Pic(CpuInterface& cpu)
    : m_cpu(cpu)
    , m_interruptInService(-1)
{
}

Pic::~Pic()
{
}

// public methods
uint8_t Pic::PortRead(uint16_t port)
{
    return 0;
}

void Pic::PortWrite(uint16_t port, uint8_t value)
{
    if (port == 0x20 && value == 0x20)
    {
        m_interruptInService = -1;
    }
}

void Pic::Interrupt(int num)
{
    m_interrupt[num].raised = true;
}

void Pic::HandleInterrupts()
{
    if (m_interruptInService != -1)
        return;

    HandleInterrupt(0);  // timer interrupt
    HandleInterrupt(1);  // keyboard interrupt
}

// private methods
void Pic::HandleInterrupt(int num)
{
    auto& interrupt = m_interrupt[num];

    if (!interrupt.raised || interrupt.masked)
        return;

    if (m_cpu.HardwareInterrupt(num + 8))
    {
        m_interruptInService = num;
        interrupt.raised = false;
    }
}


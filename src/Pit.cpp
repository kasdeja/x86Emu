#include <stdio.h>
#include "Pic.h"
#include "Pit.h"

// constructor & destructor
Pit::Pit(Pic& pic)
    : m_pic(pic)
{
}

Pit::~Pit()
{
}

// public methods
uint8_t Pit::PortRead(uint16_t port)
{
    printf("Unhandled read port = 0x%04x\n", port);
    return 0;
}

void Pit::PortWrite(uint16_t port, uint8_t value)
{
    if (port == 0x43)
    {
        m_cmd = value;
        return;
    }

    PitChannel& channel = m_channel[port - 0x40];

    uint8_t accessMode = (m_cmd >> 5) & 3;

    if (accessMode == 3)
    {
        if (channel.byteNumber == 0)
        {
            channel.divisorReg = (channel.divisorReg & 0xff00) | value;
            channel.byteNumber = 1;
        }
        else
        {
            channel.divisorReg = (channel.divisorReg & 0x00ff) | (value << 8);
            channel.byteNumber = 0;
            channel.divisor    = (channel.divisorReg > 0) ? channel.divisorReg : 65536;
            channel.tickCount  = channel.divisor;
        }
    }
}

void Pit::Process(int64_t nsec)
{
    double ticks = 1191181.6666 / 1000000000.0 * nsec;

    // Process timer interrupt channel.
    m_channel[0].tickCount -= ticks;

    if (m_channel[0].tickCount < 0)
    {
        m_channel[0].tickCount += m_channel[0].divisor;
        m_pic.Interrupt(0);
    }
}

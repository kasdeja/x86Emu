#include <stdio.h>
#include <algorithm>
#include "Pic.h"
#include "Pit.h"

// constructor & destructor
Pit::Pit(Pic& pic)
    : m_pic(pic)
    , m_cmd(0)
    , m_latch(0)
{
}

Pit::~Pit()
{
}

// public methods
uint8_t Pit::PortRead(uint16_t port)
{
    PitChannel& channel = m_channel[port - 0x40];

    uint8_t result = m_latch & 0xff;
    m_latch >>= 8;

    printf("Handled read port = 0x%04x value 0x%02x\n", port, result);

    return result;
}

void Pit::PortWrite(uint16_t port, uint8_t value)
{
    printf("Handled read write = 0x%04x value 0x%02x\n", port, value);
    if (port == 0x43)
    {
        m_cmd = value;

        uint8_t accessMode = (m_cmd >> 4) & 3;
        int     channel = m_cmd >> 6;

        if (accessMode == 0 && channel < 3)
        {
            m_latch = std::min(65535.0, m_channel[channel].tickCount);
        }

        return;
    }

    PitChannel& channel = m_channel[port - 0x40];

    uint8_t accessMode = (m_cmd >> 4) & 3;

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

    // PC Speaker
    m_channel[2].tickCount -= ticks;

    if (m_channel[2].tickCount < 0)
    {
        m_channel[2].tickCount += m_channel[2].divisor;
    }
}

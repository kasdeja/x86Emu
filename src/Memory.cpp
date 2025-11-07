#include <string.h>
#include "Memory.h"

Memory::Memory(uint32_t ramSizeKb)
{
    uint32_t ramSize = ((ramSizeKb + 63) & 0xffffff80) * 1024;

    m_memory    = new uint8_t[ramSize + 256 * 1024];
    m_vgaMemory = m_memory + ramSize;

    m_memorySize    = ramSize;
    m_vgaMemorySize = 256 * 1024;

    ::memset(m_memory, 0, ramSize + 256 * 1024);

    // Fill interrupt table with pseudo vectors and install handler for hardware interrupts.
    for(int n = 0; n < 256; n++)
    {
        reinterpret_cast<uint32_t *>(m_memory)[n] = 0;// 0xabcd1000 + n * 16;
    }

    // 50              push ax
    // b0 20           mov al, 0x20
    // e6 20           out 0x20, al
    // 58              pop ax
    // cf              iret
    static uint8_t defaultIrqHandler[7] = { 0x50, 0xb0, 0x20, 0xe6, 0x20, 0x58, 0xcf };

    // f4 hlt
    static uint8_t defaultIntHandler[1] = { 0xcf };

    ::memcpy(m_memory + 0xfff00, defaultIrqHandler, 7);
    ::memcpy(m_memory + 0xfff10, defaultIntHandler, 1);

    // Fill interrupt table with pseudo vectors and install handler for hardware interrupts.
    //for(int n = 0; n < 256; n++)
    //{
    //    reinterpret_cast<uint32_t *>(m_memory)[n] = 0xfff00010;
    //}

    for(int n = 8; n < 16; n++)
    {
        reinterpret_cast<uint32_t *>(m_memory)[n] = 0xfff00000;
    }
}

Memory::~Memory()
{
    delete m_memory;
}

uint8_t* Memory::GetMem()
{
    return m_memory;
}

uint8_t* Memory::GetVgaMem()
{
    return m_vgaMemory;
}

uint32_t Memory::GetMemSize()
{
    return m_memorySize;
}

uint32_t Memory::GetVgaMemSize()
{
    return m_vgaMemorySize;
}

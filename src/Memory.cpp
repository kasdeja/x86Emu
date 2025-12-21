#include <string.h>
#include "Memory.h"

Memory::Memory(uint32_t ramSizeKb)
{
    uint32_t ramSize = ramSizeKb * 1024;

    m_memory    = new uint8_t[ramSize + 256 * 1024];
    m_vgaMemory = m_memory + ramSize;

    m_memorySize    = ramSize;
    m_vgaMemorySize = 256 * 1024;

    ::memset(m_memory, 0, ramSize + 256 * 1024);

    // Clear interrupt table
    for(int n = 0; n < 256; n++)
    {
        reinterpret_cast<uint32_t *>(m_memory)[n] = 0;// 0xabcd1000 + n * 16;
    }

    // 50              push ax
    // b0 20           mov al, 0x20
    // e6 20           out 0x20, al
    // 58              pop ax
    // cf              iret
    static uint8_t defaultIrqHandler[7]   = { 0x50, 0xb0, 0x20, 0xe6, 0x20, 0x58, 0xcf };

    // 50              push ax
    // b0 ff           mov al, 0xff
    // e6 68           out 0x68, al
    // b0 20           mov al, 0x20
    // e6 20           out 0x20, al
    // 58              pop ax
    // cf              iret
    static uint8_t keyboardIrqHandler[11] = { 0x50, 0xb0, 0xff, 0xe6, 0x68, 0xb0, 0x20, 0xe6, 0x20, 0x58, 0xcf };

    // f4 hlt
    static uint8_t defaultIntHandler[1] = { 0xcf };

    ::memcpy(m_memory + 0xfff00, defaultIrqHandler, 7);
    ::memcpy(m_memory + 0xfff10, defaultIntHandler, 1);
    ::memcpy(m_memory + 0xfff20, keyboardIrqHandler, 11);

    // Bios Data Area (BDA)
    *reinterpret_cast<uint16_t *>(m_memory + 0x413) = 639;
    *reinterpret_cast<uint16_t *>(m_memory + 0x463) = 0x3d4;

    // Fill interrupt table with pseudo vectors and install handler for hardware interrupts.
    for(int n = 8; n < 16; n++)
    {
        reinterpret_cast<uint32_t *>(m_memory)[n] = 0xfff00000;
    }

    // Install keyboard IRQ handler
    reinterpret_cast<uint32_t *>(m_memory)[9] = 0xfff20000;
}

Memory::~Memory()
{
    delete [] m_memory;
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

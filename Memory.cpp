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

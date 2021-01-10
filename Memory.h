#ifndef X86EMU_MEMORY
#define X86EMU_MEMORY

#include <inttypes.h>
#include <memory>

class Memory
{
public:
    static constexpr uint32_t kMaxRealModeMemory = 1088;    // 1024 + 64 kB

    // constructor & destructor
    Memory(uint32_t ramSizeKb);
    ~Memory();

    // public methods
    uint8_t* GetMem();
    uint8_t* GetVgaMem();

    uint32_t GetMemSize();
    uint32_t GetVgaMemSize();

private:
    uint8_t* m_memory;
    uint8_t* m_vgaMemory;
    uint32_t m_memorySize;
    uint32_t m_vgaMemorySize;
};

#endif /* X86EMU_MEMORY */

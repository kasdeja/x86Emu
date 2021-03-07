#ifndef X86EMU_BIOS
#define X86EMU_BIOS

#include <inttypes.h>
#include <string>
#include <vector>

// forward declarations
class CpuInterface;
class Memory;

class Bios
{
public:
    // constructor & destructor
    Bios(Memory& memory);
    ~Bios();

    // public methods
    void Int10h(CpuInterface *cpu);
    void Int11h(CpuInterface *cpu);
    void Int1Ah(CpuInterface *cpu);

private:
    uint8_t* m_memory;

    uint8_t m_cursorX;
    uint8_t m_cursorY;
};

#endif /* X86EMU_BIOS */

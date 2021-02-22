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
    void Int1Ah(CpuInterface *cpu);

private:
    uint8_t* m_memory;
};

#endif /* X86EMU_BIOS */

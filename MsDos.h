#ifndef X86EMU_MS_DOS
#define X86EMU_MS_DOS

#include <inttypes.h>

// forward declaration
class CpuInterface;

class MsDos
{
public:
    // constructor & destructor
    MsDos(uint8_t* memory);
    ~MsDos();

    void Int21h(CpuInterface *cpu);

private:
    uint8_t*  m_memory;
};

#endif /* X86EMU_MS_DOS */

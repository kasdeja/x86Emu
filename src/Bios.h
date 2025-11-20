#ifndef X86EMU_BIOS
#define X86EMU_BIOS

#include <inttypes.h>
#include <string>
#include <vector>
#include <queue>

// forward declarations
class CpuInterface;
class Memory;
class Vga;

class Bios
{
public:
    // constructor & destructor
    Bios(Memory& memory, Vga& vga);
    ~Bios();

    // public methods
    void Int10h(CpuInterface *cpu);
    void Int11h(CpuInterface *cpu);
    void Int16h(CpuInterface *cpu);
    void Int1Ah(CpuInterface *cpu);

    void    AddKey(uint8_t key);
    uint8_t ShowKey();
    uint8_t GetKey();
    bool    HasKey();

private:
    uint8_t* m_memory;
    Vga&     m_vga;

    uint8_t m_cursorX;
    uint8_t m_cursorY;

    std::queue<uint8_t> m_keys;
};

#endif /* X86EMU_BIOS */

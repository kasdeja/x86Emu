#ifndef X86EMU_MEMORY_VIEW
#define X86EMU_MEMORY_VIEW

class Memory;
class Vga;

class MemoryView
{
public:
    // constructor & destructor
    MemoryView(Memory *memory, Vga *vga);
    ~MemoryView();

    // public methods
    void DrawRamDump(uint8_t* pixels, int width, int height, int stride);

private:
    Memory *m_memory;
    Vga    *m_vga;
};

#endif /* X86EMU_MEMORY_VIEW */

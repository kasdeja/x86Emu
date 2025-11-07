#ifndef X86EMU_SDL_INTERFACE
#define X86EMU_SDL_INTERFACE

class Vga;
class MemoryView;

class SDLInterface
{
public:
    // constructor & destructor
    SDLInterface(Vga *vga, MemoryView *memoryView);
    ~SDLInterface();

    // public methods
    bool Initialize();
    void MainLoop();

private:
    Vga *m_vga;
    MemoryView *m_memoryView;
};

#endif /* X86EMU_SDL_INTERFACE */

#ifndef X86EMU_SDL_INTERFACE
#define X86EMU_SDL_INTERFACE

class Vga;

class SDLInterface
{
public:
    // constructor & destructor
    SDLInterface(Vga *vga);
    ~SDLInterface();

    // public methods
    bool Initialize();
    void MainLoop();

private:
    Vga *m_vga;
};

#endif /* X86EMU_SDL_INTERFACE */

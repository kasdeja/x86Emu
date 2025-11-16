#ifndef X86EMU_SDL_INTERFACE
#define X86EMU_SDL_INTERFACE

#include <inttypes.h>
#include <atomic>
#include <functional>

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
    void StopMainLoop();

    std::function<void (uint8_t scancode)>  onKeyEvent;

private:
    std::atomic<bool> m_running;
    Vga*              m_vga;
    MemoryView*       m_memoryView;
};

#endif /* X86EMU_SDL_INTERFACE */

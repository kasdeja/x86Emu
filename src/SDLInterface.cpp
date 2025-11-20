#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "SDLInterface.h"
#include "MemoryView.h"
#include "Vga.h"

// constructor & destructor
SDLInterface::SDLInterface(Vga *vga, MemoryView *memoryView)
    : m_vga(vga), m_memoryView(memoryView)
{
}

SDLInterface::~SDLInterface()
{
    m_vga = nullptr;
    m_memoryView = nullptr;
}

// public methods
bool SDLInterface::Initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void SDLInterface::MainLoop()
{
    SDL_Window *window =
        SDL_CreateWindow(
            "SDL Test",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1440, 1080,
            //1600, 1200,
            //1920, 1440,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    SDL_Window *mvWindow = nullptr;
    SDL_Surface *mvSurface = nullptr;

    if (m_memoryView)
    {
        mvWindow =
            SDL_CreateWindow(
                "Memory View",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                1344, 1088,
                SDL_WINDOW_SHOWN
            );

        mvSurface = SDL_GetWindowSurface(mvWindow);
    }

    printf("w %d h %d pitch %d\n", surface->w, surface->h, surface->pitch);
    printf("bits per pixel %d\n", surface->format->BitsPerPixel);
    printf("bytes per pixel %d\n", surface->format->BytesPerPixel);
    printf("rmask 0x%08x\n", surface->format->Rmask);
    printf("gmask 0x%08x\n", surface->format->Gmask);
    printf("bmask 0x%08x\n", surface->format->Bmask);

    // Main loop
    SDL_Event event;

    m_running = true;

    while(m_running)
    {
        // Process events
        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    {
                        uint8_t keycode = 0;
                        bool    extended = false;

                        switch(event.key.keysym.scancode)
                        {
                            case SDL_SCANCODE_A:            keycode = 0x1e;                  break;
                            case SDL_SCANCODE_B:            keycode = 0x30;                  break;
                            case SDL_SCANCODE_C:            keycode = 0x2e;                  break;
                            case SDL_SCANCODE_D:            keycode = 0x20;                  break;
                            case SDL_SCANCODE_E:            keycode = 0x12;                  break;
                            case SDL_SCANCODE_F:            keycode = 0x21;                  break;
                            case SDL_SCANCODE_G:            keycode = 0x22;                  break;
                            case SDL_SCANCODE_H:            keycode = 0x23;                  break;
                            case SDL_SCANCODE_I:            keycode = 0x17;                  break;
                            case SDL_SCANCODE_J:            keycode = 0x24;                  break;
                            case SDL_SCANCODE_K:            keycode = 0x25;                  break;
                            case SDL_SCANCODE_L:            keycode = 0x26;                  break;
                            case SDL_SCANCODE_M:            keycode = 0x32;                  break;
                            case SDL_SCANCODE_N:            keycode = 0x31;                  break;
                            case SDL_SCANCODE_O:            keycode = 0x18;                  break;
                            case SDL_SCANCODE_P:            keycode = 0x19;                  break;
                            case SDL_SCANCODE_Q:            keycode = 0x10;                  break;
                            case SDL_SCANCODE_R:            keycode = 0x13;                  break;
                            case SDL_SCANCODE_S:            keycode = 0x1f;                  break;
                            case SDL_SCANCODE_T:            keycode = 0x14;                  break;
                            case SDL_SCANCODE_U:            keycode = 0x16;                  break;
                            case SDL_SCANCODE_V:            keycode = 0x2f;                  break;
                            case SDL_SCANCODE_W:            keycode = 0x11;                  break;
                            case SDL_SCANCODE_X:            keycode = 0x2d;                  break;
                            case SDL_SCANCODE_Y:            keycode = 0x15;                  break;
                            case SDL_SCANCODE_Z:            keycode = 0x2c;                  break;

                            case SDL_SCANCODE_1:            keycode = 0x02;                  break;
                            case SDL_SCANCODE_2:            keycode = 0x03;                  break;
                            case SDL_SCANCODE_3:            keycode = 0x04;                  break;
                            case SDL_SCANCODE_4:            keycode = 0x05;                  break;
                            case SDL_SCANCODE_5:            keycode = 0x06;                  break;
                            case SDL_SCANCODE_6:            keycode = 0x07;                  break;
                            case SDL_SCANCODE_7:            keycode = 0x08;                  break;
                            case SDL_SCANCODE_8:            keycode = 0x09;                  break;
                            case SDL_SCANCODE_9:            keycode = 0x0a;                  break;
                            case SDL_SCANCODE_0:            keycode = 0x0b;                  break;

                            case SDL_SCANCODE_RETURN:       keycode = 0x1c;                  break;
                            case SDL_SCANCODE_ESCAPE:       keycode = 0x01;                  break;
                            case SDL_SCANCODE_BACKSPACE:    keycode = 0x0e;                  break;
                            case SDL_SCANCODE_TAB:          keycode = 0x0f;                  break;
                            case SDL_SCANCODE_SPACE:        keycode = 0x39;                  break;

                            case SDL_SCANCODE_MINUS:        keycode = 0x0c;                  break;
                            case SDL_SCANCODE_EQUALS:       keycode = 0x0d;                  break;
                            case SDL_SCANCODE_LEFTBRACKET:  keycode = 0x1a;                  break;
                            case SDL_SCANCODE_RIGHTBRACKET: keycode = 0x1b;                  break;
                            case SDL_SCANCODE_BACKSLASH:    keycode = 0x2b;                  break;
                            case SDL_SCANCODE_SEMICOLON:    keycode = 0x27;                  break;
                            case SDL_SCANCODE_APOSTROPHE:   keycode = 0x28;                  break;
                            case SDL_SCANCODE_GRAVE:        keycode = 0x29;                  break;
                            case SDL_SCANCODE_COMMA:        keycode = 0x33;                  break;
                            case SDL_SCANCODE_PERIOD:       keycode = 0x34;                  break;
                            case SDL_SCANCODE_SLASH:        keycode = 0x35;                  break;

                            case SDL_SCANCODE_CAPSLOCK:     keycode = 0x3a;                  break;

                            case SDL_SCANCODE_F1:           keycode = 0x3b;                  break;
                            case SDL_SCANCODE_F2:           keycode = 0x3c;                  break;
                            case SDL_SCANCODE_F3:           keycode = 0x3d;                  break;
                            case SDL_SCANCODE_F4:           keycode = 0x3e;                  break;
                            case SDL_SCANCODE_F5:           keycode = 0x3f;                  break;
                            case SDL_SCANCODE_F6:           keycode = 0x40;                  break;
                            case SDL_SCANCODE_F7:           keycode = 0x41;                  break;
                            case SDL_SCANCODE_F8:           keycode = 0x42;                  break;
                            case SDL_SCANCODE_F9:           keycode = 0x43;                  break;
                            case SDL_SCANCODE_F10:          keycode = 0x44;                  break;
                            case SDL_SCANCODE_F11:          keycode = 0x57;                  break;
                            case SDL_SCANCODE_F12:          keycode = 0x58;                  break;

                            case SDL_SCANCODE_INSERT:       keycode = 0x52; extended = true; break;
                            case SDL_SCANCODE_HOME:         keycode = 0x47; extended = true; break;
                            case SDL_SCANCODE_PAGEUP:       keycode = 0x49; extended = true; break;
                            case SDL_SCANCODE_DELETE:       keycode = 0x53; extended = true; break;
                            case SDL_SCANCODE_END:          keycode = 0x4f; extended = true; break;
                            case SDL_SCANCODE_PAGEDOWN:     keycode = 0x51; extended = true; break;
                            case SDL_SCANCODE_RIGHT:        keycode = 0x4d; extended = true; break;
                            case SDL_SCANCODE_LEFT:         keycode = 0x4b; extended = true; break;
                            case SDL_SCANCODE_DOWN:         keycode = 0x50; extended = true; break;
                            case SDL_SCANCODE_UP:           keycode = 0x48; extended = true; break;

                            case SDL_SCANCODE_LCTRL:        keycode = 0x1d;                  break;
                            case SDL_SCANCODE_LSHIFT:       keycode = 0x2a;                  break;
                            case SDL_SCANCODE_LALT:         keycode = 0x38;                  break;
                            case SDL_SCANCODE_RCTRL:        keycode = 0x1d; extended = true; break;
                            case SDL_SCANCODE_RSHIFT:       keycode = 0x36;                  break;
                            case SDL_SCANCODE_RALT:         keycode = 0x38; extended = true; break;
                            default:                        keycode = 0;                     break;
                        }

                        if (keycode == 0)
                        {
                            break;
                        }

                        if (event.key.state == SDL_PRESSED)
                        {
                            printf("Key press detected, keysym %d\n", event.key.keysym.scancode);
                        }
                        else
                        {
                            printf("Key release detected, keysym %d\n", event.key.keysym.scancode);
                            keycode |= 0x080;
                        }

                        if (onKeyEvent && keycode != 0)
                        {
                            if (extended)
                            {
                                onKeyEvent(0xe0);
                            }

                            onKeyEvent(keycode);
                        }
                    }
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        surface = SDL_GetWindowSurface(window);

                        printf("window resized to %d, %d\n", event.window.data1, event.window.data2);
                    }
                    break;

                case SDL_QUIT:
                    printf("quit\n");
                    m_running = false;
                    break;
            }
        }

        m_vga->DrawScreenFiltered(reinterpret_cast<uint8_t *>(surface->pixels), surface->w, surface->h, surface->pitch);
        SDL_UpdateWindowSurface(window);

        if (m_memoryView)
        {
            m_memoryView->DrawRamDump(reinterpret_cast<uint8_t *>(mvSurface->pixels), mvSurface->w, mvSurface->h, mvSurface->pitch);
            SDL_UpdateWindowSurface(mvWindow);
        }

        // Do some heavy work
        ::usleep(10 * 1000);
    }

    SDL_DestroyWindow(window);

    if (mvWindow)
        SDL_DestroyWindow(mvWindow);
}

void SDLInterface::StopMainLoop()
{
    m_running = false;
}

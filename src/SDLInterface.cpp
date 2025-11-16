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
                            case SDL_SCANCODE_SPACE:  keycode = 0x39;                  break;
                            case SDL_SCANCODE_ESCAPE: keycode = 0x01;                  break;
                            case SDL_SCANCODE_RIGHT:  keycode = 0x4d; extended = true; break;
                            case SDL_SCANCODE_LEFT:   keycode = 0x4b; extended = true; break;
                            case SDL_SCANCODE_DOWN:   keycode = 0x50; extended = true; break;
                            case SDL_SCANCODE_UP:     keycode = 0x48; extended = true; break;
                            default:                  keycode = 0;                     break;
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

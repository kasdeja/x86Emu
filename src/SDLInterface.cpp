#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "SDLInterface.h"
#include "Vga.h"

// constructor & destructor
SDLInterface::SDLInterface(Vga *vga)
    : m_vga(vga)
{
}

SDLInterface::~SDLInterface()
{
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

    printf("w %d h %d pitch %d\n", surface->w, surface->h, surface->pitch);
    printf("bits per pixel %d\n", surface->format->BitsPerPixel);
    printf("bytes per pixel %d\n", surface->format->BytesPerPixel);
    printf("rmask 0x%08x\n", surface->format->Rmask);
    printf("gmask 0x%08x\n", surface->format->Gmask);
    printf("bmask 0x%08x\n", surface->format->Bmask);

    // Main loop
    SDL_Event event;
    bool      running = true;

    while(running)
    {
        // Process events
        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        surface = SDL_GetWindowSurface(window);

                        printf("window resized to %d, %d\n", event.window.data1, event.window.data2);
                    }
                    break;

                case SDL_QUIT:
                    printf("quit\n");
                    running = false;
                    break;
            }
        }

        m_vga->DrawScreenFiltered(reinterpret_cast<uint8_t *>(surface->pixels), surface->w, surface->h, surface->pitch);
        SDL_UpdateWindowSurface(window);

        // Do some heavy work
        ::usleep(10 * 1000);
    }

    SDL_DestroyWindow(window);
}


#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>
#include "Vga.h"
#include "Memory.h"

void FillMemoryWithTestContent(Memory &memory)
{
    uint8_t* videoMem     = memory.GetVgaMem();
    uint8_t* videoMemText = memory.GetVgaMem() + 0x18000;

    // Text mode sample content
    const char *str = "This is sample text. Hello World! :)";

    for(int n = 0; n < ::strlen(str); n++)
    {
        videoMemText[2 * n] = str[n];
        videoMemText[2 * n + 1] = 15 + (3 << 4);

        videoMemText[2 * n + 160] = str[n];
        videoMemText[2 * n + 161] = 15;
    }

    for(int n = 0; n < 256; n++)
        videoMemText[2 * n + 160 * 3] = n;

    for(int n = 0; n < 256; n++)
    {
        videoMemText[2 * n + 160 * 12] = n;
        videoMemText[2 * n + 160 * 12 + 1] =  (1 << 4) + 7;
    }

    // Mode 13h sample content
    for(int x = 0; x < 320; x++)
    {
        videoMem[x] = 7;
        videoMem[x + 199 * 320] = 7;
    }

    for(int y = 0; y < 200; y++)
    {
        videoMem[y * 320] = 7;
        videoMem[y * 320 + 319] = 7;
    }

    for(int n = 0; n < 256; n++)
    {
        int x = (n % 64) * 4 + 2;
        int y = (n / 64) * 4 + 2;

        int addr = y * 320 + x;

        videoMem[addr +   0] = videoMem[addr +   1] = videoMem[addr +   2] = n;
        videoMem[addr + 320] = videoMem[addr + 321] = videoMem[addr + 322] = n;
        videoMem[addr + 640] = videoMem[addr + 641] = videoMem[addr + 642] = n;
    }

    for(int x = 0; x < 256; x++)
    {
        for(int y = 0; y < 32; y++)
            videoMem[x + (y + 32) * 320] = x;
    }

}

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

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

    // Initialize VGA
    Memory memory(1024);
    Vga    vga(memory);

    FillMemoryWithTestContent(memory);

    // Main loop
    SDL_Event event;
    bool      running = true;

    int cnt = 0;

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

                        printf("window resized to %d, %d\n",
                            event.window.data1,
                            event.window.data2);
                    }
                    break;

                case SDL_QUIT:
                    printf("quit\n");
                    running = false;
                    break;
            }
        }

        if (cnt < 500)
        {
            if (cnt == 0)
                vga.SetMode(Vga::Mode::Text);

            vga.DrawScreenFiltered(reinterpret_cast<uint8_t *>(surface->pixels), surface->w, surface->h - 1, surface->pitch);
        }
        else
        {
            if (cnt == 500)
                vga.SetMode(Vga::Mode::Mode13h);

            vga.DrawScreenFiltered(reinterpret_cast<uint8_t *>(surface->pixels), surface->w, surface->h - 1, surface->pitch);
        }

        if (cnt++ >= 1000)
            cnt = 0;

        SDL_UpdateWindowSurface(window);

        // Do some heavy work
        ::usleep(10 * 1000);
    }

    SDL_DestroyWindow(window);

    return 0;
}

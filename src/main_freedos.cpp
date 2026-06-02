#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <thread>
#include <atomic>
#include <unistd.h>
#include "Memory.h"
#include "MemoryView.h"
#include "Vga.h"
#include "Bios.h"
#include "Cpu.h"
#include "Pic.h"
#include "Pit.h"
#include "Keyboard.h"
#include "SDLInterface.h"

int main(int argc, char **argv)
{
    printf("x86emu v0.1\n\n");

    // Initialize emulator
    Memory*       memory     = new Memory(4096);
    Vga*          vga        = new Vga(*memory);
    MemoryView*   memoryView = nullptr; // new MemoryView(memory, vga);
    Bios*         bios       = new Bios(*memory, *vga);
    CpuInterface* cpu        = new Cpu(*memory);
    Pic*          pic        = new Pic(*cpu);
    Pit*          pit        = new Pit(*pic);
    Keyboard*     keyboard   = new Keyboard;
    SDLInterface* sdl        = new SDLInterface(vga, memoryView);

    pic->onAck = [keyboard](int irqNo)
        {
            if (irqNo == 1)
            {
                keyboard->RemoveKey();
            }
        };

    cpu->onInterrupt =
        [cpu, bios](int intNo)
        {
            if (intNo == 0x10)
            {
                bios->Int10h(cpu);
            }
            else if (intNo == 0x11)
            {
                bios->Int11h(cpu);
            }
            else if (intNo == 0x12)
            {
                bios->Int12h(cpu);
            }
            else if (intNo == 0x13)
            {
                bios->Int13h(cpu);
            }
            else if (intNo == 0x14)
            {
                // Do nothing (serial port)
            }
            else if (intNo == 0x15)
            {
                // Do nothing
            }
            else if (intNo == 0x16)
            {
                bios->Int16h(cpu);
            }
            else if (intNo == 0x1a)
            {
                bios->Int1Ah(cpu);
            }
            else
            {
                cpu->Interrupt(intNo);
            }
        };

    cpu->onPortRead =
        [vga, pic, pit, keyboard](uint16_t port, int size) -> uint32_t
        {
            //printf("read port = 0x%04x, size = %d\n", port, size);
            switch(port)
            {
                case 0x20: case 0x21:
                    return pic->PortRead(port);

                case 0x40: case 0x41:
                case 0x42: case 0x43:
                    return pit->PortRead(port);

                case 0x60:
                    {
                        uint8_t key = keyboard->GetKey();
                        printf("onPortRead() got key %02x\n", key);
                        return key;
                    }

                case 0x3c5:
                case 0x3c9:
                case 0x3cf:
                case 0x3d5:
                case 0x3da:
                    return vga->PortRead(port);

                case 0x61:
                case 0x388: // Adlib Address / Status, ignore
                case 0x389: // Adlib Data port, ignore
                    return 0;

                case 0x201: // Joystick, ignore
                    return 0xff;

                default:
                    printf("Unhandled read port = 0x%04x, size = %d\n", port, size);
                    return 0;
            }
        };

    cpu->onPortWrite =
        [vga, pic, pit, bios, keyboard](uint16_t port, int size, uint32_t value)
        {
            //printf("write port = 0x%04x, size = %d, value = %d (0x%04x)\n", port, size, value, value);
            switch(port)
            {
                case 0x20: case 0x21:
                    pic->PortWrite(port, value);
                    break;

                case 0x68:
                    printf("onPortWrite() got write on pseudoport 0x68\n");
                    if (keyboard->HasKey())
                    {
                        bios->AddKey(keyboard->GetKey());
                    }
                    break;

                case 0x40: case 0x41:
                case 0x42: case 0x43:
                    pit->PortWrite(port, value);
                    break;

                case 0x3c4: case 0x3c5:
                case 0x3ce: case 0x3cf:
                case 0x3d4: case 0x3d5:
                    if (size == 2)
                    {
                        vga->PortWrite(port, value & 0xff);
                        vga->PortWrite(port + 1, (value >> 8) & 0xff);
                    }
                    else
                    {
                        vga->PortWrite(port, value);
                    }
                    break;

                case 0x3c7:
                case 0x3c8:
                case 0x3c9:
                    vga->PortWrite(port, value);
                    break;

                case 0x61:
                case 0x201:
                    break;

                default:
                    printf("Unhandled write port = 0x%04x, size = %d, value = %d (0x%04x)\n", port, size, value, value);
                    break;
            }
        };

    cpu->onVgaMemRead  = [vga](uint32_t addr) { return vga->MemRead(addr); };
    cpu->onVgaMemWrite = [vga](uint32_t addr, uint8_t value) { vga->MemWrite(addr, value); };

    cpu->SetReg16(CpuInterface::IP, 0x7c00);

    bios->OpenDrive(0, "freedos/x86BOOT.img");
    bios->LoadMBR(0);

    auto runEmulator =
        [cpu, pic, pit, keyboard](int64_t usec, int64_t instructionsPerSecond) -> bool
        {
            constexpr int64_t batchSize = 100;
            int64_t instructionsToExecute = (instructionsPerSecond * usec) / 1000000;

            while(instructionsToExecute > 0)
            {
                int64_t itr = std::min(batchSize, instructionsToExecute);

                if (keyboard->HasKey() && !pic->IsInService(1))
                {
                    pic->Interrupt(1);
                }

                if (!cpu->Run(itr))
                {
                    return false;
                }

                pit->Process((1000000000 * itr) / instructionsPerSecond);
                pic->HandleInterrupts();

                instructionsToExecute -= itr;
            }

            return true;
        };

    // Start main loop
    std::atomic<bool> running;
    std::thread       thread;

    if (sdl->Initialize())
    {
        sdl->onKeyEvent = [keyboard](uint8_t scancode) { keyboard->AddKey(scancode); };
        running = true;

        thread = std::thread(
            [&running, cpu, runEmulator, sdl]
            {
                printf("Running...\n");
                while(running)
                {
                    if (!runEmulator(5000, 4000000))
                    {
                        sdl->StopMainLoop();
                        break;
                    }

                    ::usleep(5000);
                }
                printf("Finished...\n");
            });

        sdl->MainLoop();
        running = false;

        if (thread.joinable())
            thread.join();
    }

    delete sdl;
    delete cpu;
    delete bios;
    delete memoryView;
    delete vga;
    delete memory;

    return 0;
}

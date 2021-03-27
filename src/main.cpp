#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <thread>
#include <atomic>
#include "Memory.h"
#include "Vga.h"
#include "Bios.h"
#include "Dos.h"
#include "Cpu.h"
#include "SDLInterface.h"

int main(int argc, char **argv)
{
    printf("x86emu v0.1\n\n");

    // Initialize emulator
    Memory*       memory = new Memory(4096);
    Vga*          vga    = new Vga(*memory);
    Bios*         bios   = new Bios(*memory, *vga);
    Dos*          dos    = new Dos(*memory);
    CpuInterface* cpu    = new Cpu(*memory);
    SDLInterface* sdl    = new SDLInterface(vga);

    uint16_t envSeg   = 0x0ff0;
    uint16_t pspSeg   = 0x1000;
    uint16_t imageSeg = 0x1010;

    auto imageInfo = dos->LoadExeFromFile(0x1010, "wolf/WOLF3D.EXE");

    uint16_t nextSeg = 0xa000;// imageSeg + (imageInfo.imageSize >> 4);

    dos->BuildEnv(envSeg, { "PATH=C:\\" }); // { "PATH=C:\\", "PROMPT=$P$G" }
    dos->BuildPsp(pspSeg, envSeg, nextSeg, "C:\\WOLF\\WOLF3D.EXE");
    dos->SetPspSeg(pspSeg);

    cpu->onSoftIrq =
        [dos, bios](CpuInterface *cpu, int irq)
        {
            if (irq == 0x21)
            {
                dos->Int21h(cpu);
            }
            else if (irq == 0x10)
            {
                bios->Int10h(cpu);
            }
            else if (irq == 0x11)
            {
                bios->Int11h(cpu);
            }
            else if (irq == 0x1a)
            {
                bios->Int1Ah(cpu);
            }
            else
            {
                cpu->Interrupt(irq);
            }
        };

    cpu->SetReg16(CpuInterface::CS, imageInfo.initCS);
    cpu->SetReg16(CpuInterface::IP, imageInfo.initIP);
    cpu->SetReg16(CpuInterface::SS, imageInfo.initSS);
    cpu->SetReg16(CpuInterface::SP, imageInfo.initSP);

    cpu->SetReg16(CpuInterface::DS, pspSeg);
    cpu->SetReg16(CpuInterface::ES, pspSeg);

    cpu->SetReg16(CpuInterface::AX, 2);

    // Start main loop
    std::atomic<bool> running;
    std::thread       thread;

    if (sdl->Initialize())
    {
        running = true;

        thread = std::thread(
            [&running, cpu]
            {
                printf("Running...\n");
                cpu->Run(32768);
            });

        sdl->MainLoop();

        if (thread.joinable())
            thread.join();
    }

    delete sdl;
    delete cpu;
    delete dos;
    delete bios;
    delete vga;
    delete memory;

    return 0;
}

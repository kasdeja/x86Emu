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
#include "Pic.h"
#include "Pit.h"
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
    Pic*          pic    = new Pic(*cpu);
    Pit*          pit    = new Pit(*pic);
    SDLInterface* sdl    = new SDLInterface(vga);

    uint16_t envSeg   = 0x0ff0;
    uint16_t pspSeg   = 0x1000;
    uint16_t imageSeg = 0x1010;

    auto imageInfo = dos->LoadExeFromFile(0x1010, "wolf/WOLF3D.EXE");

    uint16_t nextSeg = 0xa000;// imageSeg + (imageInfo.imageSize >> 4);

    dos->BuildEnv(envSeg, { "PATH=C:\\" }); // { "PATH=C:\\", "PROMPT=$P$G" }
    dos->BuildPsp(pspSeg, envSeg, nextSeg, "C:\\WOLF\\WOLF3D.EXE");
    dos->SetPspSeg(pspSeg);
    dos->SetCwd("./wolf");

    cpu->onSoftIrq =
        [dos, bios](CpuInterface *cpu, int irq)
        {
            if (irq == 0x21)
            {
                dos->Int21h(cpu);
            }
            else if (irq == 0x2f) // XMS interrupt
            {
                // Do nothing
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

    cpu->onPortRead =
        [vga, pic, pit](CpuInterface *cpu, uint16_t port, int size) -> uint32_t
        {
            switch(port)
            {
                case 0x20: case 0x21:
                    return pic->PortRead(port);

                case 0x40: case 0x41:
                case 0x42: case 0x43:
                    return pit->PortRead(port);

                case 0x3c5:
                case 0x3c9:
                case 0x3cf:
                case 0x3d5:
                case 0x3da:
                    return vga->PortRead(port);

                default:
                    printf("Unhandled read port = 0x%04x, size = %d\n", port, size);
                    return 0;
            }
        };

    cpu->onPortWrite =
        [vga, pic, pit](CpuInterface *cpu, uint16_t port, int size, uint32_t value)
        {
            switch(port)
            {
                case 0x20: case 0x21:
                    pic->PortWrite(port, value);
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

                default:
                    printf("Unhandled write port = 0x%04x, size = %d, value = %d (0x%04x)\n", port, size, value, value);
                    break;
            }
        };

    vga->onPlaneModeChange =
        [cpu](bool chain4, uint8_t planeMask)
        {
            cpu->VgaPlaneMode(chain4, planeMask);
        };

    cpu->SetReg16(CpuInterface::CS, imageInfo.initCS);
    cpu->SetReg16(CpuInterface::IP, imageInfo.initIP);
    cpu->SetReg16(CpuInterface::SS, imageInfo.initSS);
    cpu->SetReg16(CpuInterface::SP, imageInfo.initSP);
    cpu->SetReg16(CpuInterface::DS, pspSeg);
    cpu->SetReg16(CpuInterface::ES, pspSeg);
    cpu->SetReg16(CpuInterface::AX, 2);

    auto runEmulator =
        [cpu, pic, pit](int64_t usec)
        {
            int64_t nps = 1000000000;               // nanoseconds per second
            int64_t ips = 200000;                   // instructions per second
            int     ite = (ips * usec) / 1000000;   // instructions to execute

            for(int n = 0; n < ite; n += 5000)
            {
                int itr = ite - n;

                if (itr > 5000)
                    itr = 5000;

                cpu->Run(itr);
                pit->Process((nps * itr) / ips);
                pic->HandleInterrupts();
            }
        };

    // Start main loop
    std::atomic<bool> running;
    std::thread       thread;

    if (sdl->Initialize())
    {
        running = true;

        thread = std::thread(
            [&running, cpu, runEmulator]
            {
                printf("Running...\n");
                runEmulator(10 * 1000 * 1000);   // 10 sec
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

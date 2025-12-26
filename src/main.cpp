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
#include "Dos.h"
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
    MemoryView*   memoryView = new MemoryView(memory, vga);
    Bios*         bios       = new Bios(*memory, *vga);
    Dos*          dos        = new Dos(*memory, *bios);
    CpuInterface* cpu        = new Cpu(*memory);
    Pic*          pic        = new Pic(*cpu);
    Pit*          pit        = new Pit(*pic);
    Keyboard*     keyboard   = new Keyboard;
    SDLInterface* sdl        = new SDLInterface(vga, memoryView);

    uint16_t envSeg   = 0x07ca;
    uint16_t pspSeg   = 0x0814;
    uint16_t imageSeg = 0x0824;
    uint16_t nextSeg  = 0x9fff;

    std::string game = (argc > 1) ? argv[1] : "wolf";
    std::string gameCwd, gameImg, gameExe;

    if (game == "wolf")
    {
        gameCwd = "./games/wolf";
        gameImg = "./games/wolf/wolf3d.exe";
        gameExe = "C:\\WOLF\\WOLF3D.EXE";
    }
    else if (game == "monkey")
    {
        gameCwd = "./games/monkey";
        gameImg = "./games/monkey/monkey.exe";
        gameExe = "C:\\MONKEY\\MONKEY.EXE";
    }
    else if (game == "lotus3")
    {
        gameCwd = "./games/lotus3";
        gameImg = "./games/lotus3/lotus.exe";
        gameExe = "C:\\LOTUS3\\LOTUS.EXE";
    }
    else if (game == "tyrian")
    {
        gameCwd = "./games/tyrian";
        gameImg = "./games/tyrian/tyrian.exe";
        gameExe = "C:\\TYRIAN\\TYRIAN.EXE";
    }
    else if (game == "another")
    {
        gameCwd = "./games/another";
        gameImg = "./games/another/another.exe";
        gameExe = "C:\\ANOTHER\\ANOTHER.EXE";
    }
    else if (game == "prehist2")
    {
        gameCwd = "./games/prehist2";
        gameImg = "./games/prehist2/pre2.exe";
        gameExe = "C:\\PREHIST2\\PRE2.EXE";
    }
    else if (game == "priv")
    {
        gameCwd = "./games/priv";
        gameImg = "./games/priv/priv.exe";
        gameExe = "C:\\PRIV\\PRIV.EXE";
    }
    else if (game == "tie")
    {
        gameCwd = "./games/tie";
        gameImg = "./games/tie/tie.exe";
        gameExe = "C:\\TIE\\TIE.EXE";
    }
    else if (game == "wc1")
    {
        gameCwd = "./games/wc1";
        gameImg = "./games/wc1/wc.exe";
        gameExe = "C:\\WC1\\wc.EXE";
    }
    else if (game == "xwing")
    {
        gameCwd = "./games/xwing";
        gameImg = "./games/xwing/xwing.exe";
        gameExe = "C:\\XWING\\XWING.EXE";
    }

    dos->SetCDrive("./games");
    dos->SetCwd(gameCwd);

    dos->BuildEnv(envSeg, gameExe, { "PATH=C:\\" });
    auto imageInfo = dos->LoadExeFromFile(imageSeg, gameImg.c_str());
    dos->BuildPsp(pspSeg, envSeg, nextSeg, "\r\r\r\r\r\r\r\r");
    dos->SetPspSeg(pspSeg);

    pic->onAck = [keyboard](int irqNo)
        {
            if (irqNo == 1)
            {
                keyboard->RemoveKey();
            }
        };

    cpu->onInterrupt =
        [cpu, dos, bios](int intNo)
        {
            if (intNo == 0x01) // Single step / int 21 alias??? WTF?
            {
                dos->Int21h(cpu);
            }
            else if (intNo == 0x10)
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
            else if (intNo == 0x21)
            {
                dos->Int21h(cpu);
            }
            else if (intNo == 0x2f) // XMS interrupt
            {
                // Do nothing
            }
            else if (intNo == 0x33) // Mouse
            {
                // Do nothing
            }
            else if (intNo == 0x74) // ???
            {
                // Do nothing
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
                case 0x201: // Joystick, ignore
                case 0x388: // Adlib Address / Status, ignore
                case 0x389: // Adlib Data port, ignore
                    return 0;

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

    cpu->SetReg16(CpuInterface::CS, imageInfo.initCS);
    cpu->SetReg16(CpuInterface::IP, imageInfo.initIP);
    cpu->SetReg16(CpuInterface::SS, imageInfo.initSS);
    cpu->SetReg16(CpuInterface::SP, imageInfo.initSP);
    cpu->SetReg16(CpuInterface::DS, pspSeg);
    cpu->SetReg16(CpuInterface::ES, pspSeg);
    cpu->SetReg16(CpuInterface::AX, 0); // was 2, why?

    cpu->SetReg16(CpuInterface::CX, 0xff);
    cpu->SetReg16(CpuInterface::SI, 0x00);
    cpu->SetReg16(CpuInterface::DI, 0x80);
    cpu->SetReg16(CpuInterface::BP, 0x91C);

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
    delete dos;
    delete bios;
    delete memoryView;
    delete vga;
    delete memory;

    return 0;
}

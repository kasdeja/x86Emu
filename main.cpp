#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include "Memory.h"
#include "CpuModel1.h"
#include "MsDos.h"

int main(int argc, char **argv)
{
    printf("x86emu v0.1\n\n");

    Memory*       memory = new Memory(4096);
    MsDos*        dos    = new MsDos(*memory);
    CpuInterface* cpu    = new CpuModel1::Cpu(*memory);

    uint16_t envSeg   = 0x0ff0;
    uint16_t pspSeg   = 0x1000;
    uint16_t imageSeg = 0x1010;

    auto imageInfo = dos->LoadExeFromFile(0x1010, "wolf/WOLF3D.EXE");

    uint16_t nextSeg = imageSeg + (imageInfo.imageSize >> 4);

    dos->BuildEnv(envSeg, { "PATH=C:\\" });
    dos->BuildPsp(pspSeg, envSeg, nextSeg, "C:\\WOLF\\WOLF3D.EXE");

    cpu->onSoftIrq =
        [dos](CpuInterface *cpu, int irq)
        {
            if (irq == 0x21)
            {
                dos->Int21h(cpu);
            }
        };

    cpu->SetReg16(CpuInterface::CS, imageInfo.initCS);
    cpu->SetReg16(CpuInterface::IP, imageInfo.initIP);
    cpu->SetReg16(CpuInterface::SS, imageInfo.initSS);
    cpu->SetReg16(CpuInterface::SP, imageInfo.initSP);

    cpu->SetReg16(CpuInterface::DS, pspSeg);
    cpu->SetReg16(CpuInterface::ES, pspSeg);

    cpu->SetReg16(CpuInterface::AX, 2);

    printf("Running...\n");
    cpu->Run(64);

    delete cpu;
    delete dos;
    delete memory;

    return 0;
}

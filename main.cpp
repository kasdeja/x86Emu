#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include "CpuModel1.h"
#include "LoadExe.h"
#include "MsDos.h"

int main(int argc, char **argv)
{
    printf("x86emu v0.1\n\n");

    constexpr uint32_t kMaxDosRamSize = 1024 * 1024 + 65536;

    uint8_t* memory = new uint8_t[kMaxDosRamSize];
    std::fill(memory, memory + kMaxDosRamSize, 0);

    auto result = LoadExe(memory).FromFile(0x1010, "wolf/WOLF3D.EXE");

    // PSP
    //  +00      2  wInt20       INT 20H instruction (cd 20) (old way to exit)
    //  +02      2  wNextSeg     Segment addr just beyond end of program image
    //  +04      1  res1         (reserved)
    //  +05      5  abDispatch   FAR CALL to DOS function dispatcher (obs)
    //  +0a      4  pfTerminate  terminate address.       See INT 22H
    //  +0e      4  pfCtlBrk     Ctrl-Break handler address   INT 23H
    //  +12      4  pfCritErr    Critical Error handler addr  INT 24H
    //  +16     22  res2         DOS reserved area
    //           2  wParentPsp   ◄undoc► segment of parent's PSP
    //  +2c      2  wEnvSeg      segment address of DOS environment
    //  +2e     46  res3         DOS reserved area (handle table, et al.)
    //  +5c     16  rFCB_1       an unopened FCB for 1st cmd parameter
    //  +6c     20  rFCB_2       an unopened FCB for 2nd cmd parameter
    //  +80      1  bCmdTailLen  count of characters in command tail at 81H (also
    //                            default setting for the DTA)
    //  +81    127  abCmdTail    characters from DOS command line
    uint8_t* psp = memory + 0x10000;
    uint8_t* env = memory + 0x0ff00;

    const char *wolfPath = "C:\\WOLF\\WOLF3D.EXE";

    psp[0] = 0xcd;
    psp[1] = 0x20;
    *reinterpret_cast<uint16_t *>(psp + 2)    = 0x1010 + (result.imageSize >> 4);
    *reinterpret_cast<uint16_t *>(psp + 0x2c) = 0x0ff0;

    psp[0x80] = strlen(wolfPath);
    strcpy(reinterpret_cast<char *>(psp + 0x81), wolfPath);
    strcpy(reinterpret_cast<char *>(env), "PATH=C:\\");

    MsDos        *msDos = new MsDos(memory);
    CpuInterface *cpu   = new CpuModel1::Cpu(memory);

    cpu->onSoftIrq =
        [msDos](CpuInterface *cpu, int irq)
        {
            if (irq == 0x21)
            {
                msDos->Int21h(cpu);
            }
        };

    cpu->SetReg16(CpuInterface::CS, result.initCS);
    cpu->SetReg16(CpuInterface::IP, result.initIP);
    cpu->SetReg16(CpuInterface::SS, result.initSS);
    cpu->SetReg16(CpuInterface::SP, result.initSP);

    cpu->SetReg16(CpuInterface::DS, 0x1000);
    cpu->SetReg16(CpuInterface::ES, 0x1000);

    cpu->SetReg16(CpuInterface::AX, 2);

    printf("Running...\n");
    cpu->Run(64);

    delete cpu;
    delete msDos;
    delete memory;

    return 0;
}

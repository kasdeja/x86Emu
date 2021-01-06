#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "CpuModel1.h"
#include "LoadExe.h"

using namespace CpuModel1;

int main(int argc, char **argv)
{
    printf("x86emu v0.1\n\n");

    constexpr uint32_t kMaxDosRamSize = 1024 * 1024 + 65536;

    uint8_t* memory = new uint8_t[kMaxDosRamSize];
    ::memset(memory, 0, kMaxDosRamSize);

    auto result = LoadExe(memory).FromFile(0x1010, "wolf/WOLF3D.EXE");

    Cpu cpu(memory);

    cpu.SetReg(Cpu::Register::CS, result.initCS);
    cpu.SetReg(Cpu::Register::IP, result.initIP);
    cpu.SetReg(Cpu::Register::SS, result.initSS);
    cpu.SetReg(Cpu::Register::SP, result.initSP);

    cpu.SetReg(Cpu::Register::DS, 0x1000);
    cpu.SetReg(Cpu::Register::ES, 0x1000);

    cpu.SetReg(Cpu::Register::AX, 2);

    printf("Running...\n");
    cpu.Run(1);

    return 0;
}

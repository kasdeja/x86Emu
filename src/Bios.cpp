#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "CpuInterface.h"
#include "Memory.h"
#include "Vga.h"
#include "Bios.h"

#ifndef _WIN32
#define O_BINARY 0
#endif

const uint16_t s_biosKeyMapping[80][4] = {
//    Normal  Shifted w/Ctrl  w/Alt
    { 0x1E61, 0x1E41, 0x1E01, 0x1E00 },
    { 0x3062, 0x3042, 0x3002, 0x3000 },
    { 0x2E63, 0x2E42, 0x2E03, 0x2E00 },
    { 0x2064, 0x2044, 0x2004, 0x2000 },
    { 0x1265, 0x1245, 0x1205, 0x1200 },
    { 0x2166, 0x2146, 0x2106, 0x2100 },
    { 0x2267, 0x2247, 0x2207, 0x2200 },
    { 0x2368, 0x2348, 0x2308, 0x2300 },
    { 0x1769, 0x1749, 0x1709, 0x1700 },
    { 0x246A, 0x244A, 0x240A, 0x2400 },
    { 0x256B, 0x254B, 0x250B, 0x2500 },
    { 0x266C, 0x264C, 0x260C, 0x2600 },
    { 0x326D, 0x324D, 0x320D, 0x3200 },
    { 0x316E, 0x314E, 0x310E, 0x3100 },
    { 0x186F, 0x184F, 0x180F, 0x1800 },
    { 0x1970, 0x1950, 0x1910, 0x1900 },
    { 0x1071, 0x1051, 0x1011, 0x1000 },
    { 0x1372, 0x1352, 0x1312, 0x1300 },
    { 0x1F73, 0x1F53, 0x1F13, 0x1F00 },
    { 0x1474, 0x1454, 0x1414, 0x1400 },
    { 0x1675, 0x1655, 0x1615, 0x1600 },
    { 0x2F76, 0x2F56, 0x2F16, 0x2F00 },
    { 0x1177, 0x1157, 0x1117, 0x1100 },
    { 0x2D78, 0x2D58, 0x2D18, 0x2D00 },
    { 0x1579, 0x1559, 0x1519, 0x1500 },
    { 0x2C7A, 0x2C5A, 0x2C1A, 0x2C00 },
    { 0x0231, 0x0221, 0x7800, 0xFFFF },
    { 0x0332, 0x0340, 0x0300, 0x7900 },
    { 0x0433, 0x0423, 0x7A00, 0xFFFF },
    { 0x0534, 0x0524, 0x7B00, 0xFFFF },
    { 0x0635, 0x0625, 0x7C00, 0xFFFF },
    { 0x0736, 0x075E, 0x071E, 0x7D00 },
    { 0x0837, 0x0826, 0x7E00, 0xFFFF },
    { 0x0938, 0x092A, 0x7F00, 0xFFFF },
    { 0x0A39, 0x0A28, 0x8000, 0xFFFF },
    { 0x0B30, 0x0B29, 0x8100, 0xFFFF },
    { 0x0C2D, 0x0C5F, 0x0C1F, 0x8200 },
    { 0x0D3D, 0x0D2B, 0x8300, 0xFFFF },
    { 0x1A5B, 0x1A7B, 0x1A1B, 0x1A00 },
    { 0x1B5D, 0x1B7D, 0x1B1D, 0x1B00 },
    { 0x273B, 0x273A, 0x2700, 0xFFFF },
    { 0x2827, 0x2822, 0xFFFF, 0xFFFF },
    { 0x2960, 0x297E, 0xFFFF, 0xFFFF },
    { 0x2B5C, 0x2B7C, 0x2B1C, 0x2600 },
    { 0x332C, 0x333C, 0xFFFF, 0xFFFF },
    { 0x342E, 0x343E, 0xFFFF, 0xFFFF },
    { 0x352F, 0x353F, 0xFFFF, 0xFFFF },
    { 0x3B00, 0x5400, 0x5E00, 0x6800 },
    { 0x3C00, 0x5500, 0x5F00, 0x6900 },
    { 0x3D00, 0x5600, 0x6000, 0x6A00 },
    { 0x3E00, 0x5700, 0x6100, 0x6B00 },
    { 0x3F00, 0x5800, 0x6200, 0x6C00 },
    { 0x4000, 0x5900, 0x6300, 0x6D00 },
    { 0x4100, 0x5A00, 0x6400, 0x6E00 },
    { 0x4200, 0x5B00, 0x6500, 0x6F00 },
    { 0x4300, 0x5C00, 0x6600, 0x7000 },
    { 0x4400, 0x5D00, 0x6700, 0x7100 },
    { 0x8500, 0x8700, 0x8900, 0x8B00 },
    { 0x8600, 0x8800, 0x8A00, 0x8C00 },
    { 0x0E08, 0x0E08, 0x0E7F, 0x0E00 },
    { 0x5300, 0x532E, 0x9300, 0xA300 },
    { 0x5000, 0x5032, 0x9100, 0xA000 },
    { 0x4F00, 0x4F31, 0x7500, 0x9F00 },
    { 0x1C0D, 0x1C0D, 0x1C0A, 0xA600 },
    { 0x011B, 0x011B, 0x011B, 0x0100 },
    { 0x4700, 0x4737, 0x7700, 0x9700 },
    { 0x5200, 0x5230, 0x9200, 0xA200 },
    { 0xFFFF, 0x4C35, 0x8F00, 0xFFFF },
    { 0x372A, 0x9600, 0x3700, 0xFFFF },
    { 0x4A2D, 0x4A2D, 0x8E00, 0x4A00 },
    { 0x4E2B, 0x4E2B, 0x4E00, 0xFFFF },
    { 0x352F, 0x352F, 0x9500, 0xA400 },
    { 0x4B00, 0x4B34, 0x7300, 0x9B00 },
    { 0x5100, 0x5133, 0x7600, 0xA100 },
    { 0x4900, 0x4939, 0x8400, 0x9900 },
    { 0xFFFF, 0xFFFF, 0x7200, 0xFFFF },
    { 0x4D00, 0x4D36, 0x7400, 0x9D00 },
    { 0x3920, 0x3920, 0x3920, 0x3920 },
    { 0x0F09, 0x0F00, 0x9400, 0xA500 },
    { 0x4800, 0x4838, 0x8D00, 0x9800 }
};

// constructor & destructor
Bios::Bios(Memory& memory, Vga& vga)
    : m_memory(memory.GetMem())
    , m_vga   (vga)
{
    // BIOS Data Area
    m_memory[0x44a] = 80; // number of columns
    m_memory[0x484] = 24; // number of rows - 1

    m_memory[0x462] = 0;  // active display page number
    m_memory[0x449] = 3;  // current video number

    m_cursorX = 0;
    m_cursorY = 0;

    m_extendedKey = false;
    m_shiftPressed = false;
    m_ctrlPressed = false;
    m_altPressed = false;
    m_capsPressed = false;
    m_scanCode = 0;
}

Bios::~Bios()
{
}

// public methods
void Bios::Int10h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Set video mode
        {
            uint8_t mode = cpu->GetReg8(CpuInterface::AL);

            printf("Bios::Int10h() function 0x%02x - setting video mode to 0x%02x\n",
                func, mode);

            if (mode == 0x13)
            {
                m_vga.SetMode(Vga::Mode::Mode13h);
            }
            else if (mode == 0x03)
            {
                m_vga.SetMode(Vga::Mode::Text);
            }

            break;
        }

        case 0x01: //  Set Cursor Type
        {
            printf("Bios::Int10h() function 0x%02x - set cursor type start %d, end %d\n",
                func,
                cpu->GetReg8(CpuInterface::CH),
                cpu->GetReg8(CpuInterface::CL));
            break;
        }

        case 0x02: // Set cursor position
        {
            uint8_t page = cpu->GetReg8(CpuInterface::BH);
            uint8_t row  = cpu->GetReg8(CpuInterface::DH);
            uint8_t col  = cpu->GetReg8(CpuInterface::DL);

            SetCursorPos(col, row);
            break;
        }

        case 0x03: // Read cursor position and size
        {
            cpu->SetReg8(CpuInterface::CH, 13);
            cpu->SetReg8(CpuInterface::CL, 14);
            cpu->SetReg8(CpuInterface::DH, m_cursorY);
            cpu->SetReg8(CpuInterface::DL, m_cursorX);
            break;
        }

        case 0x06: // Scroll Window Up
        {
            ScrollWindow(
                cpu->GetReg8(CpuInterface::CL),
                cpu->GetReg8(CpuInterface::CH),
                cpu->GetReg8(CpuInterface::DL),
                cpu->GetReg8(CpuInterface::DH),
                cpu->GetReg8(CpuInterface::AL),
                cpu->GetReg8(CpuInterface::BH));
            break;
        }

        case 0x08: // Read character and attribute at cursor
        {
            uint32_t addr = 0x18000 + m_cursorY * 160 + m_cursorX * 2;

            cpu->SetReg8(CpuInterface::AL, m_vga.MemRead(addr));
            cpu->SetReg8(CpuInterface::AH, m_vga.MemRead(addr + 1));
            break;
        }

        case 0x09: // Write Character and Attribute at Cursor Position
        {
            char ch   = cpu->GetReg8(CpuInterface::AL);
            char attr = cpu->GetReg8(CpuInterface::BL);
            int  cnt  = cpu->GetReg16(CpuInterface::CX);

            uint32_t addr = 0x18000 + m_cursorY * 160 + m_cursorX * 2;

            m_vga.MemWrite(addr, ch);
            m_vga.MemWrite(addr + 1, attr);
            break;
        }

        case 0x0e: // Write Text
        {
            char ch = cpu->GetReg8(CpuInterface::AL);

            if (ch == 0x07)
            {
                // do nothing
            }
            else if (ch == 0x08)
            {
                if (m_cursorX > 0)
                {
                    m_cursorX--;
                }
            }
            else if (ch == 0x0a)
            {
                m_cursorX = 0;
            }
            else if (ch == 0x0d)
            {
                m_cursorX = 0;
                m_cursorY++;
            }
            else
            {
                m_vga.MemWrite(0x18000 + m_cursorY * 160 + m_cursorX * 2, ch);
                m_vga.MemWrite(0x18000 + m_cursorY * 160 + m_cursorX * 2 + 1, 0x07);
                m_cursorX++;
            }

            if (m_cursorX >= 80)
            {
                m_cursorX = 0;
                m_cursorY++;
            }

            if (m_cursorY > 24)
            {
                m_cursorY = 24;
                ScrollWindow(0, 0, 79, 24, 1, 7);
            }

            m_vga.SetCursorPos(m_cursorX, m_cursorY);
            break;
        }

        case 0x0f: // Get current video mode
        {
            uint8_t currentMode = 3; // 80x25 color
            uint8_t columns     = 80;

            cpu->SetReg16(CpuInterface::AX, (columns << 8) | currentMode);
            cpu->SetReg8(CpuInterface::BH, 0);
            break;
        }

        case 0x10: // Palette functions
        {
            uint8_t subService = cpu->GetReg8(CpuInterface::AL);

            if (subService == 0x12)
            {
                uint16_t idx   = cpu->GetReg16(CpuInterface::BX);
                uint16_t count = cpu->GetReg16(CpuInterface::CX);

                std::size_t linearAddr = cpu->GetReg16(CpuInterface::ES) * 16 + cpu->GetReg16(CpuInterface::DX);

                for(int n = 0; n < count; n++, idx++)
                {
                    m_vga.PortWrite(0x3c8, n);
                    m_vga.PortWrite(0x3c9, m_memory[linearAddr++]);
                    m_vga.PortWrite(0x3c9, m_memory[linearAddr++]);
                    m_vga.PortWrite(0x3c9, m_memory[linearAddr++]);
                }
            }
            else
            {
                printf("Bios::Int10h() function 0x%02x subservice 0x%02x not implemented yet!\n", func, subService);
            }

            break;
        }

        case 0x12: // Alternate select
        {
            uint8_t subService = cpu->GetReg8(CpuInterface::BL);

            if (subService == 0x10)
            {
                cpu->SetReg16(CpuInterface::BX, 0x0003); // color + 256k of VRAM
                cpu->SetReg16(CpuInterface::CX, 0x0000);
            }
            else
            {
                printf("Bios::Int10h() function 0x%02x subservice 0x%02x not implemented yet!\n", func, subService);
            }

            break;
        }

        case 0x1a: // Video Display Combination
        {
            uint8_t subService = cpu->GetReg8(CpuInterface::AL);

            if (subService == 0x00)
            {
                cpu->SetReg8(CpuInterface::AL, 0x1a);
                cpu->SetReg16(CpuInterface::BX, 0x0008);
            }
            else
            {
                printf("Bios::Int10h() function 0x%02x subservice 0x%02x not implemented yet!\n", func, subService);
            }

            break;
        }

        case 0x1b: // Get VGA Functionality and State Info
        {
            // FIXME: mostly workaround
            uint8_t *data = reinterpret_cast<uint8_t *>(m_memory) + cpu->GetReg16(CpuInterface::ES) * 16 + cpu->GetReg16(CpuInterface::DI);

            data[4] = 3;
            data[5] = 80;
            data[7] = 0;
            data[8] = 16;
            data[9] = 0;
            data[34] = 19;
            data[42] = 2;

            cpu->SetReg8(CpuInterface::AL, 0x1b);
            break;
        }

        default:
            printf("Bios::Int10h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

void Bios::Int11h(CpuInterface* cpu)
{
//                            Equipment code (AX)
//
//   F E D C B A 9 8  7 6 5 4 3 2 1 0
//   x x . . . . . .  . . . . . . . .  Number of printers installed
//   . . x . . . . .  . . . . . . . .  Internal modem installed
//   . . . x . . . .  . . . . . . . .  Game adapter installed? (always 1 on PCJr)
//   . . . . x x x .  . . . . . . . .  Number of RS-232 ports
//   . . . . . . . x  . . . . . . . .  Reserved
//   . . . . . . . .  x x . . . . . .  Number of diskettes - 1 (i.e. 0=1 disk)
//   . . . . . . . .  . . x x . . . .  Initial video mode (see below)
//   . . . . . . . .  . . . . x . . .  Reserved
//   . . . . . . . .  . . . . . x . .  Reserved
//   . . . . . . . .  . . . . . . x .  Math coprocessor installed?
//   . . . . . . . .  . . . . . . . x  1=diskettes present; 0=no disks present
//
//                          Initial video mode
//
//                     Value                 Meaning
//                      00                   Reserved
//                      01                   40 x 25 Color
//                      10                   80 x 25 Color
//                      11                   80 x 25 Monochrome

    cpu->SetReg16(CpuInterface::AX, (2 << 9) | (1 << 6) | (2 << 4));
}

void Bios::Int12h(CpuInterface* cpu)
{
    cpu->SetReg16(CpuInterface::AX, 639);
}

void Bios::Int13h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Reset Disk System
        {
            int drive = cpu->GetReg8(CpuInterface::DL);

            if (m_driveInfo.find(drive) == m_driveInfo.end() || m_driveInfo[drive].fd == -1)
            {
                cpu->SetReg16(CpuInterface::AX, 0x0c00);
                cpu->SetFlag(CpuInterface::CF, true);
            }
            else
            {
                cpu->SetFlag(CpuInterface::CF, false);
            }

            break;
        }

        case 0x02: // Read Disk Sectors
        case 0x03: // Write Disk Sectors
        {
            int drive = cpu->GetReg8(CpuInterface::DL);

            if (m_driveInfo.find(drive) == m_driveInfo.end() || m_driveInfo[drive].fd == -1)
            {
                printf("Bios::Int13h() function 0x%02x drive 0x%2x not found\n", func, drive);

                cpu->SetReg16(CpuInterface::AX, 0x0c00);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            DriveInfo& driveInfo = m_driveInfo[drive];

            int      fd        = driveInfo.fd;
            uint32_t bytes     = cpu->GetReg8(CpuInterface::AL) * 512;
            char*    dstBuffer = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::ES) * 16 + cpu->GetReg16(CpuInterface::BX);

            uint32_t lba = CHStoLBA(
                driveInfo,
                cpu->GetReg8(CpuInterface::CH),
                cpu->GetReg8(CpuInterface::DH),
                cpu->GetReg8(CpuInterface::CL));

            ::lseek(fd, lba * 512, SEEK_SET);

            if (func == 0x02)
            {
                uint32_t bytesRead = ::read(fd, dstBuffer, bytes);

                printf("Bios::Int13h() function 0x%02x lba %d\n", func, lba);
                printf("Bios::Int13h() function 0x%02x read %d bytes from fd %d (requested %d)\n", func, bytesRead, fd, bytes);
            }
            else if (func == 0x03)
            {
                uint32_t bytesWrite = ::write(fd, dstBuffer, bytes);

                printf("Bios::Int13h() function 0x%02x lba %d\n", func, lba);
                printf("Bios::Int13h() function 0x%02x write %d bytes from fd %d (requested %d)\n", func, bytesWrite, fd, bytes);
            }

            cpu->SetReg8(CpuInterface::AH, 0);
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x08: // Read Drive Parameters
        {
            int drive = cpu->GetReg8(CpuInterface::DL);

            if (m_driveInfo.find(drive) == m_driveInfo.end() || m_driveInfo[drive].fd == -1)
            {
                printf("Bios::Int13h() function 0x%02x drive 0x%2x not found\n", func, drive);

                cpu->SetReg8(CpuInterface::BL, 0);
                cpu->SetReg8(CpuInterface::CH, 0);
                cpu->SetReg8(CpuInterface::CL, 0);
                cpu->SetReg8(CpuInterface::DH, 0);
                cpu->SetReg8(CpuInterface::DL, 0);  // number of drives
                cpu->SetReg8(CpuInterface::AH, 0);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            DriveInfo& driveInfo = m_driveInfo[drive];

            if (driveInfo.isFloppy)
            {
                cpu->SetReg8(CpuInterface::BL, 4);       // 1.44MB floppy
                cpu->SetReg16(CpuInterface::ES, 0xf000); // Disk Base Table, empty currently
                cpu->SetReg16(CpuInterface::DI, 0x0000);
            }

            cpu->SetReg8(CpuInterface::CH, driveInfo.nCylinders - 1); // max cylinder
            cpu->SetReg8(CpuInterface::CL, driveInfo.nSectors);       // sectors per track
            cpu->SetReg8(CpuInterface::DH, driveInfo.nHeads - 1);     // max head

            int driveCnt = 0;

            for(auto const& it: m_driveInfo)
            {
                if (it.second.isFloppy == driveInfo.isFloppy)
                {
                    driveCnt++;
                }
            }

            printf("Bios::Int13h() function 0x%02x drive 0x%2x cyls %d heads %d sectors %d ndrives %d\n",
                func, drive, driveInfo.nCylinders, driveInfo.nHeads, driveInfo.nSectors, driveCnt);

            cpu->SetReg8(CpuInterface::DL, driveCnt); // number of floppies / hard disk
            cpu->SetReg8(CpuInterface::AH, 0);
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x15:
        {
            int drive = cpu->GetReg8(CpuInterface::DL);

            if (m_driveInfo.find(drive) == m_driveInfo.end() || m_driveInfo[drive].fd == -1)
            {
                printf("Bios::Int13h() function 0x%02x drive 0x%2x not found\n", func, drive);

                cpu->SetReg8(CpuInterface::AH, 0);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            DriveInfo& driveInfo = m_driveInfo[drive];

            if (driveInfo.isFloppy)
            {
                cpu->SetReg8(CpuInterface::AH, 1);
            }
            else
            {
                uint32_t nTotalSectors = driveInfo.nCylinders * driveInfo.nHeads * driveInfo.nSectors;

                cpu->SetReg16(CpuInterface::CX, nTotalSectors >> 16);
                cpu->SetReg16(CpuInterface::DX, nTotalSectors & 0xffff);
            }

            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x41: // Check EDD extension present
        {
            printf("Bios::Int13h() function 0x%02x EDD extension not supported!\n", func);
            cpu->SetReg8(CpuInterface::AH, 1);
            cpu->SetFlag(CpuInterface::CF, true);
            break;
        }

        default:
            printf("Bios::Int13h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

void Bios::Int16h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Read key press
        {
            ProcessKeys();

            if (!m_processedKeys.empty())
            {
                uint16_t key = m_processedKeys.front();
                m_processedKeys.pop();

                printf("Bios::Int16h() function 0x%02x read key 0x%2x\n", func, key);
                cpu->SetReg16(CpuInterface::AX, key);
            }
            else
            {
                cpu->SetReg16(CpuInterface::AX, 0); // no scancode
            }
            break;
        }

        case 0x01: // Get the State of the keyboard buffer
        {
            ProcessKeys();

            if (!m_processedKeys.empty())
            {
                uint16_t key = m_processedKeys.front();

                printf("Bios::Int16h() function 0x%02x get key 0x%2x\n", func, key);
                cpu->SetReg16(CpuInterface::AX, key);
                cpu->SetFlag(CpuInterface::ZF, false);
            }
            else
            {
                //printf("Bios::Int16h() function 0x%02x no key\n", func);
                cpu->SetReg16(CpuInterface::AX, 0); // no scancode
                cpu->SetFlag(CpuInterface::ZF, true);
            }
            break;
        }

        default:
            printf("Bios::Int16h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

void Bios::Int1Ah(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Read system-timer time count
        {
            uint32_t tickCount = 12 * 60 * 60 * 18.2;

            cpu->SetReg16(CpuInterface::CX, tickCount >> 16);
            cpu->SetReg16(CpuInterface::DX, tickCount &  0xffff);
            cpu->SetReg8(CpuInterface::AL, 0);

            break;
        }

        default:
            printf("Bios::Int1Ah() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

void Bios::AddKey(uint8_t key)
{
    if (key != 0)
    {
        m_keys.push(key);
    }
}

uint8_t Bios::ShowKey()
{
    if (!m_keys.empty())
    {
        return m_keys.front();
    }

    return 0;
}

uint8_t Bios::GetKey()
{
    uint8_t key = 0;

    if (!m_keys.empty())
    {
        key = m_keys.front();
        m_keys.pop();
    }

    return key;
}

bool Bios::HasKey()
{
    return !m_keys.empty();
}

bool Bios::LoadMBR(int drive)
{
    if (m_driveInfo.find(drive) == m_driveInfo.end() || m_driveInfo[drive].fd == -1)
    {
        return false;
    }

    int fd = m_driveInfo[drive].fd;

    ::lseek(fd, 0, SEEK_SET);
    int bytesRead = ::read(fd, m_memory + 0x7c00, 512) == 512;

    return bytesRead > 0;
}

bool Bios::OpenDrive(int drive, const std::string &fileName, int nCylinders, int nHeads, int nSectors)
{
    // Open drive image
    int fd = ::open(fileName.c_str(), O_RDWR | O_BINARY);

    if (fd < 0)
    {
        return false;
    }

    // Close current drive image file if exists
    if (m_driveInfo.find(drive) != m_driveInfo.end() && m_driveInfo[drive].fd != -1)
    {
        ::close(m_driveInfo[drive].fd);
    }

    // Create / update new drive info
    m_driveInfo[drive] = {
        .fd = fd,
        .nHeads = nHeads,
        .nSectors = nSectors,
        .nCylinders = nCylinders,
        .isFloppy = drive < 0x80
    };

    return true;
}

bool Bios::OpenFloppyDrive(int drive, const std::string &fileName)
{
    if (drive >= 0x80)
    {
        return false;
    }

    return OpenDrive(drive, fileName, 80, 2, 18);
}

void Bios::CloseDrive(int drive)
{
    if (m_driveInfo.find(drive) != m_driveInfo.end())
    {
        int& fd = m_driveInfo[drive].fd;

        if (fd != -1)
        {
            ::close(fd);
            fd = -1;
        }
    }
}

void Bios::ProcessKeys()
{
    uint8_t  key;
    uint16_t retCode;
    int      idx, column;

    for(;;)
    {
        if (HasKey())
        {
            key = GetKey();
            printf("Bios::ProcessKeys() scancode %2x\n", key);
            m_scanCode = (m_scanCode << 8) | key;

            if (key == 0xe0)
            {
                continue;
            }
        }
        else
        {
            break;
        }

        bool     released = (m_scanCode & 0x80) != 0;
        uint16_t scanCode = m_scanCode & 0xff7f;

        m_scanCode = 0;

        if (scanCode == 0x1d || scanCode == 0xe01d)
        {
            m_ctrlPressed = !released;
            printf("Bios::ProcessKeys() ctrl pressed = %d\n", m_ctrlPressed);
            continue;
        }
        else if (scanCode == 0x2a || scanCode == 0x36)
        {
            m_shiftPressed = !released;
            printf("Bios::ProcessKeys() shift pressed = %d\n", m_shiftPressed);
            continue;
        }
        else if (scanCode == 0x38 || scanCode == 0xe038)
        {
            m_altPressed = !released;
            printf("Bios::ProcessKeys() alt pressed = %d\n", m_altPressed);
            continue;
        }
        else if (scanCode == 0x3a)
        {
            m_capsPressed = !released;
            printf("Bios::ProcessKeys() caps pressed = %d\n", m_capsPressed);
            continue;
        }

        if (released)
        {
            printf("Bios::ProcessKeys() key released\n");
            continue;
        }

        if ((scanCode >> 8) == 0xe0)
        {
            printf("Bios::ProcessKeys() extended code - omitting\n");
            continue;
        }

        if (m_capsPressed ^ m_shiftPressed)
        {
            column = 1;
        }
        else if (m_ctrlPressed)
        {
            column = 2;
        }
        else if (m_altPressed)
        {
            column = 3;
        }
        else
        {
            column = 0;
        }

        for(idx = 0; idx < 80; idx++)
        {
            if ((s_biosKeyMapping[idx][0] >> 8) == scanCode)
            {
                break;
            }
        }

        printf("Bios::ProcessKeys() scancode %04x idx %d\n", scanCode, idx);

        if (idx >= 80)
        {
            continue;
        }

        retCode = s_biosKeyMapping[idx][column];
        printf("Bios::ProcessKeys() retcode %04x\n", retCode);

        if (retCode == 0xffff)
        {
            continue;
        }
        else
        {
            m_processedKeys.push(retCode);
        }
    }
}

void Bios::SetCursorPos(uint8_t x, uint8_t y)
{
    m_cursorX = x;
    m_cursorY = y;
    m_memory[0x450] = x;
    m_memory[0x451] = y;
    m_vga.SetCursorPos(x, y);
}

void Bios::ScrollWindow(int x1, int y1, int x2, int y2, int nLines, uint8_t attr)
{
    if (x1 > x2 || y1 > y2)
    {
        return;
    }

    auto clamp = [](int val, int max) {
        if (val < 0)
        {
            return 0;
        }
        else if (val > max)
        {
            return max;
        }

        return val;
    };

    x1 = clamp(x1, 79);
    x2 = clamp(x2, 79);
    y1 = clamp(y1, 24);
    y2 = clamp(y2, 24);

    int height = y2 - y1 + 1;

    if (nLines >= height || nLines == 0)
    {
        // Clear window
        for(int y = y1; y <= y2; y++)
        {
            for(int x = x1; x <= x2; x++)
            {
                uint32_t addr = 0x18000 + y * 160 + x * 2;
                m_vga.MemWrite(addr, 32);
                m_vga.MemWrite(addr + 1, attr);
            }
        }
    }
    else
    {
        uint32_t lineOffset = nLines * 160;

        // Scroll window up
        for(int y = y1; y <= (24 - nLines); y++)
        {
            for(int x = x1; x <= x2; x++)
            {
                uint32_t addr = 0x18000 + y * 160 + 2 * x;

                m_vga.MemWrite(addr, m_vga.MemRead(addr + lineOffset));
                m_vga.MemWrite(addr + 1, m_vga.MemRead(addr + lineOffset + 1));
            }
        }

        // Clear last line
        for(int y = (24 - nLines + 1); y <= y2; y++)
        {
            for(int x = x1; x <= x2; x++)
            {
                uint32_t addr = 0x18000 + y * 160 + x * 2;
                m_vga.MemWrite(addr, 32);
                m_vga.MemWrite(addr + 1, attr);
            }
        }
    }
}

uint32_t Bios::CHStoLBA(const DriveInfo& driveInfo, uint32_t cylReg,  uint32_t head, uint32_t cylSectorReg)
{
    uint32_t cyl    = cylReg + ((cylSectorReg & 0xc0) << 2);
    uint32_t sector = cylSectorReg & 0x3f;

    return (cyl * driveInfo.nHeads + head) * driveInfo.nSectors + sector - 1;
}

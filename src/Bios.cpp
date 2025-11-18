#include "CpuInterface.h"
#include "Memory.h"
#include "Vga.h"
#include "Bios.h"

// constructor & destructor
Bios::Bios(Memory& memory, Vga& vga)
    : m_memory(memory.GetMem())
    , m_vga   (vga)
{
    // BIOS Data Area
    m_memory[0x484] = 24; // number of rows on screen - 1

    m_cursorX = 0;
    m_cursorY = 0;
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

        case 0x02: // Set cursor position
        {
            uint8_t page = cpu->GetReg8(CpuInterface::BH);
            uint8_t row  = cpu->GetReg8(CpuInterface::DH);
            uint8_t col  = cpu->GetReg8(CpuInterface::DL);

            m_cursorX = col;
            m_cursorY = row;

            printf("Bios::Int10h() function 0x%02x - setting cursor position to %d, %d (page %d)\n",
                   func, row, col, page);
            break;
        }

        case 0x03: // Read cursor position and size
        {
            cpu->SetReg8(CpuInterface::CH, 11);
            cpu->SetReg8(CpuInterface::CL, 12);
            cpu->SetReg8(CpuInterface::DH, m_cursorY);
            cpu->SetReg8(CpuInterface::DL, m_cursorX);
            break;
        }

        case 0x08: // Read character and attribute at cursor
        {
            // Always return space with black background and white foreground
            cpu->SetReg16(CpuInterface::AX, 0x0720);
            break;
        }

        case 0x0f: // Get current video mode
        {
            uint32_t tickCount = 12 * 60 * 60 * 18.2;

            uint8_t currentMode = 3; // 80x25 color
            uint8_t columns     = 80;

            cpu->SetReg16(CpuInterface::AX, (columns << 8) | currentMode);
            cpu->SetReg8(CpuInterface::BH, 0);

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
        };

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

void Bios::Int16h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x00: // Read key press
        case 0x01: // Get the State of the keyboard buffer
        {
            cpu->SetReg16(CpuInterface::AX, 0); // no scancode
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

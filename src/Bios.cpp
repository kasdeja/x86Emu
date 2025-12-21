#include "CpuInterface.h"
#include "Memory.h"
#include "Vga.h"
#include "Bios.h"

namespace
{

uint8_t KeyToChar(uint8_t key)
{
    switch(key & 0x7f)
    {
        case 0x1E: return 0x61;     // A
        case 0x30: return 0x62;     // B
        case 0x2E: return 0x63;     // C
        case 0x20: return 0x64;     // D
        case 0x12: return 0x65;     // E
        case 0x21: return 0x66;     // F
        case 0x22: return 0x67;     // G
        case 0x23: return 0x68;     // H
        case 0x17: return 0x69;     // I
        case 0x24: return 0x6A;     // J
        case 0x25: return 0x6B;     // K
        case 0x26: return 0x6C;     // L
        case 0x32: return 0x6D;     // M
        case 0x31: return 0x6E;     // N
        case 0x18: return 0x6F;     // O
        case 0x19: return 0x70;     // P
        case 0x10: return 0x71;     // Q
        case 0x13: return 0x72;     // R
        case 0x1F: return 0x73;     // S
        case 0x14: return 0x74;     // T
        case 0x16: return 0x75;     // U
        case 0x2F: return 0x76;     // V
        case 0x11: return 0x77;     // W
        case 0x2D: return 0x78;     // X
        case 0x15: return 0x79;     // Y
        case 0x2C: return 0x7A;     // Z
        case 0x02: return 0x31;     // 1
        case 0x03: return 0x32;     // 2
        case 0x04: return 0x33;     // 3
        case 0x05: return 0x34;     // 4
        case 0x06: return 0x35;     // 5
        case 0x07: return 0x36;     // 6
        case 0x08: return 0x37;     // 7
        case 0x09: return 0x38;     // 8
        case 0x0A: return 0x39;     // 9
        case 0x0B: return 0x30;     // 0
        case 0x0C: return 0x2D;     // -
        case 0x0D: return 0x3D;     // =
        case 0x1A: return 0x5B;     // [
        case 0x1B: return 0x5D;     // ]
        case 0x27: return 0x3B;     // ;
        case 0x28: return 0x27;     // '
        case 0x29: return 0x60;     // `
        case 0x2B: return 0x5C;     // Backslash
        case 0x33: return 0x2C;     // ,
        case 0x34: return 0x2E;     // .
        case 0x35: return 0x2F;     // /
        case 0x0E: return 0x08;     // BackSpace
        case 0x1C: return 0x0D;     // Enter
        case 0x01: return 0x1B;     // Esc
        case 0x39: return 0x20;     // SpaceBar
        case 0x0F: return 0x09;     // Tab

        default: return 0;
    }
}

}

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
        {
            if (HasKey())
            {
                uint16_t key = GetKey();

                printf("Bios::Int16h() function 0x%02x read key 0x%2x\n", func, key);
                cpu->SetReg16(CpuInterface::AX, (key << 8) | KeyToChar(key));
            }
            else
            {
                cpu->SetReg16(CpuInterface::AX, 0); // no scancode
            }
            break;
        }

        case 0x01: // Get the State of the keyboard buffer
        {
            uint16_t key = ShowKey();

            if (HasKey())
            {
                printf("Bios::Int16h() function 0x%02x get key 0x%2x\n", func, key);
                cpu->SetReg16(CpuInterface::AX, (key << 8) | KeyToChar(key));
                cpu->SetFlag(CpuInterface::ZF, false);
            }
            else
            {
                printf("Bios::Int16h() function 0x%02x no key\n", func, key);
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


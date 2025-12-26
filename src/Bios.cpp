#include "CpuInterface.h"
#include "Memory.h"
#include "Vga.h"
#include "Bios.h"

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
    m_memory[0x484] = 24; // number of rows on screen - 1

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
                printf("Bios::Int16h() function 0x%02x no key\n", func);
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

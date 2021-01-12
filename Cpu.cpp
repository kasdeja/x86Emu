#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <string>
#include "Cpu.h"
#include "Memory.h"

#define DEBUG

#ifdef DEBUG
#define DebugPrintf(...) printf(__VA_ARGS__)
static const char* s_regName16[]   = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static const char* s_regName8l[]   = { "al", "cl", "dl", "bl" };
static const char* s_regName8h[]   = { "ah", "ch", "dh", "bh" };
static const char* s_sregName[]    = { "es", "cs", "ss", "ds" };
static const char  s_hexDigits[]   = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const char* Reg16ToStr(uint8_t reg)
{
    return s_regName16[reg];
}

const char* Reg8LowToStr(uint8_t reg)
{
    return s_regName8l[reg];
}

const char* Reg8HighToStr(uint8_t reg)
{
    return s_regName8h[reg];
}

std::string Hex16(uint8_t *ip)
{
    uint16_t value = *reinterpret_cast<uint16_t *>(ip);
    char    str[5];

    str[0] = s_hexDigits[(value >> 12) & 0x0f];
    str[1] = s_hexDigits[(value >>  8) & 0x0f];
    str[2] = s_hexDigits[(value >>  4) & 0x0f];
    str[3] = s_hexDigits[(value      ) & 0x0f];
    str[4] = 0;

    return str;
}

std::string Hex8(uint8_t *ip)
{
    uint8_t value = *ip;
    char   str[3];

    str[0] = s_hexDigits[(value >>  4) & 0x0f];
    str[1] = s_hexDigits[(value      ) & 0x0f];
    str[2] = 0;

    return str;
}

std::string Dec16(uint8_t *ip)
{
    return std::to_string(*reinterpret_cast<uint16_t *>(ip));
}

std::string Dec8(uint8_t *ip)
{
    char value = *reinterpret_cast<char *>(ip);

    return (value < 0) ? "- " + std::to_string(-value) : "+ " + std::to_string(-value);
}

std::string ModRmToStr(uint8_t *ip)
{
    std::string op;
    uint8_t     modRm = *ip;

    switch(((modRm >> 3) & 0x18) + (modRm & 0x07))
    {
        // mod 00
        case 0:  op = "[bx + si]";               break;
        case 1:  op = "[bx + di]";               break;
        case 2:  op = "[bp + si]";               break;
        case 3:  op = "[bp + di]";               break;
        case 4:  op = "[si]";                    break;
        case 5:  op = "[di]";                    break;
        case 6:  op = "[" + Hex16(ip + 1) + "]"; break;
        case 7:  op = "[bx]";                    break;

        // mod 01
        case 8:  op = "[bx + si " + Dec8(ip + 1) + "]"; break;
        case 9:  op = "[bx + di " + Dec8(ip + 1) + "]"; break;
        case 10: op = "[bp + si " + Dec8(ip + 1) + "]"; break;
        case 11: op = "[bp + di " + Dec8(ip + 1) + "]"; break;
        case 12: op = "[si "      + Dec8(ip + 1) + "]"; break;
        case 13: op = "[si "      + Dec8(ip + 1) + "]"; break;
        case 14: op = "[bp "      + Dec8(ip + 1) + "]"; break;
        case 15: op = "[bx "      + Dec8(ip + 1) + "]"; break;

        // mod 10
        case 16: op = "[bx + si + " + Hex16(ip + 1) + "]"; break;
        case 17: op = "[bx + di + " + Hex16(ip + 1) + "]"; break;
        case 18: op = "[bp + si + " + Hex16(ip + 1) + "]"; break;
        case 19: op = "[bp + di + " + Hex16(ip + 1) + "]"; break;
        case 20: op = "[si + "      + Hex16(ip + 1) + "]"; break;
        case 21: op = "[si + "      + Hex16(ip + 1) + "]"; break;
        case 22: op = "[bp + "      + Hex16(ip + 1) + "]"; break;
        case 23: op = "[bx + "      + Hex16(ip + 1) + "]"; break;

        // mod 11
        case 24: op = "ax"; break;
        case 25: op = "cx"; break;
        case 26: op = "dx"; break;
        case 27: op = "bx"; break;
        case 28: op = "sp"; break;
        case 29: op = "bp"; break;
        case 30: op = "si"; break;
        case 31: op = "di"; break;
    }

    return op;
}

void DumpMem(uint8_t *base, uint16_t segment, uint16_t offset, int length)
{
    uint8_t* mem = base + segment * 16 + offset;

    printf("%04x:%04x | ", segment, offset);

    for(int n = 0; n < length; n++)
        printf("%02x ", mem[n]);

    printf("\n");
}

void DumpInstImpl(uint8_t *base, uint16_t segment, uint16_t offset, int length, const char* fmt, ...)
{
    uint16_t offsetFix = offset - length;
    uint8_t* mem = base + segment * 16 + offsetFix;
    int      n;
    va_list  args;

    printf("%04x:%04x | ", segment, offsetFix);

    for(n = 0; n < length; n++)
        printf("%02x ", mem[n]);

    for(; n < 8; n++)
        printf("   ");

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

#define DumpInst(length, fmt, ...) \
    DumpInstImpl(m_memory, m_register[Register::CS], m_register[Register::IP], length, fmt, __VA_ARGS__)
#else
#define DebugPrintf(...)
#define DumpMem(...)
#define DumpInst(...)
#endif

#define ModRm_Case(v) case (v+0x00): case (v+0x08): case (v+0x10): case (v+0x18): case (v+0x20): case (v+0x28): case (v+0x30): case (v+0x38)

uint16_t Cpu::s_modRmInstLen[256] =
  { 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

// constructor & destructor
Cpu::Cpu(Memory& memory)
    : m_memory(memory.GetMem())
{
    std::fill(m_register, m_register + 16, 0);
    m_register[Register::FLAG] = (1 << Flag::IF) | (1 << Flag::Always1);

    m_lastResult = 0;
}

Cpu::~Cpu()
{
}

// public methods
void Cpu::SetReg16(Reg16 reg, uint16_t value)
{
    switch(reg)
    {
        case Reg16::AX: m_register[Register::AX] = value; break;
        case Reg16::BX: m_register[Register::BX] = value; break;
        case Reg16::CX: m_register[Register::CX] = value; break;
        case Reg16::DX: m_register[Register::DX] = value; break;
        case Reg16::SI: m_register[Register::SI] = value; break;
        case Reg16::DI: m_register[Register::DI] = value; break;
        case Reg16::BP: m_register[Register::BP] = value; break;
        case Reg16::SP: m_register[Register::SP] = value; break;
        case Reg16::CS: m_register[Register::CS] = value; break;
        case Reg16::DS: m_register[Register::DS] = value; break;
        case Reg16::ES: m_register[Register::ES] = value; break;
        case Reg16::SS: m_register[Register::SS] = value; break;
        case Reg16::IP: m_register[Register::IP] = value; break;
    }
}

void Cpu::SetReg8(Reg8 reg, uint8_t value)
{
    switch(reg)
    {
        case Reg8::AL: m_register[Register::AX] &= 0xff00; m_register[Register::AX] |= value; break;
        case Reg8::BL: m_register[Register::BX] &= 0xff00; m_register[Register::BX] |= value; break;
        case Reg8::CL: m_register[Register::CX] &= 0xff00; m_register[Register::CX] |= value; break;
        case Reg8::DL: m_register[Register::DX] &= 0xff00; m_register[Register::DX] |= value; break;

        case Reg8::AH: m_register[Register::AX] &= 0x00ff; m_register[Register::AX] |= value << 8; break;
        case Reg8::BH: m_register[Register::BX] &= 0x00ff; m_register[Register::BX] |= value << 8; break;
        case Reg8::CH: m_register[Register::CX] &= 0x00ff; m_register[Register::CX] |= value << 8; break;
        case Reg8::DH: m_register[Register::DX] &= 0x00ff; m_register[Register::DX] |= value << 8; break;
    }
}

uint16_t Cpu::GetReg16(Reg16 reg)
{
    switch(reg)
    {
        case Reg16::AX: return m_register[Register::AX];
        case Reg16::BX: return m_register[Register::BX];
        case Reg16::CX: return m_register[Register::CX];
        case Reg16::DX: return m_register[Register::DX];
        case Reg16::SI: return m_register[Register::SI];
        case Reg16::DI: return m_register[Register::DI];
        case Reg16::BP: return m_register[Register::BP];
        case Reg16::SP: return m_register[Register::SP];
        case Reg16::CS: return m_register[Register::CS];
        case Reg16::DS: return m_register[Register::DS];
        case Reg16::ES: return m_register[Register::ES];
        case Reg16::SS: return m_register[Register::SS];
        case Reg16::IP: return m_register[Register::IP];
    }

    return 0;
}

uint8_t Cpu::GetReg8(Reg8 reg)
{
    switch(reg)
    {
        case Reg8::AL: return m_register[Register::AX];
        case Reg8::BL: return m_register[Register::BX];
        case Reg8::CL: return m_register[Register::CX];
        case Reg8::DL: return m_register[Register::DX];

        case Reg8::AH: return m_register[Register::AX] >> 8;
        case Reg8::BH: return m_register[Register::BX] >> 8;
        case Reg8::CH: return m_register[Register::CX] >> 8;
        case Reg8::DH: return m_register[Register::DX] >> 8;
    }

    return 0;
}

void Cpu::Run(int nCycles)
{
    m_state            = 0;
    m_segmentBase      = m_register[Register::DS] * 16;
    m_stackSegmentBase = m_register[Register::SS] * 16;

    for(int n = 0; n < nCycles; n++)
    {
        printf("AX %04x BX %04x CX %04x DX %04x SI %04x DI %04x SP %04x BP %04x CS %04x DS %04x ES %04x SS %04x  ",
            m_register[Register::AX],
            m_register[Register::BX],
            m_register[Register::CX],
            m_register[Register::DX],
            m_register[Register::SI],
            m_register[Register::DI],
            m_register[Register::SP],
            m_register[Register::BP],
            m_register[Register::CS],
            m_register[Register::DS],
            m_register[Register::ES],
            m_register[Register::SS]);

        ExecuteInstruction();

        if (m_state & State::InvalidOp)
            break;
    }
}

inline uint16_t* Cpu::ModRmToReg(uint8_t modrm)
{
    return &m_register[(modrm >> 3) & 0x07];
}

inline uint16_t* Cpu::ModRmToSReg(uint8_t modrm)
{
    return &m_register[Register::ES + ((modrm >> 3) & 0x07)];
}

inline uint16_t Cpu::Disp16(uint8_t* ip)
{
    return *reinterpret_cast<uint16_t *>(ip);
}

inline int8_t Cpu::Disp8(uint8_t* ip)
{
    return *reinterpret_cast<char *>(ip);
}

inline uint16_t Cpu::Imm16(uint8_t* ip)
{
    return *reinterpret_cast<uint16_t *>(ip);
}

inline uint16_t Cpu::Load16(std::size_t linearAddr)
{
    return *reinterpret_cast<uint16_t *>(m_memory + linearAddr);
}

inline uint8_t Cpu::Load8(std::size_t linearAddr)
{
    return *reinterpret_cast<uint8_t *>(m_memory + linearAddr);
}

inline void Cpu::Store16(std::size_t linearAddr, uint16_t value)
{
    *reinterpret_cast<uint16_t *>(m_memory + linearAddr) = value;
}

inline void Cpu::Store8(std::size_t linearAddr, uint8_t value)
{
    *reinterpret_cast<uint8_t *>(m_memory + linearAddr) = value;
}

inline void Cpu::Push16(uint16_t value)
{
    m_register[Register::SP] -= 2;
    Store16(m_register[Register::SS] * 16 + m_register[Register::SP], value);
}

inline uint16_t Cpu::Pop16()
{
    uint16_t value = Load16(m_register[Register::SS] * 16 + m_register[Register::SP]);
    m_register[Register::SP] += 2;
    return value;
}

inline void Cpu::RecalcFlags()
{
    m_register[Register::FLAG] &=
        ~((1 << Flag::CF) | (1 << Flag::PF) | (1 << Flag::CF) | (1 << Flag::ZF) | (1 << Flag::SF));

    m_register[Register::FLAG] |=
        (((m_lastResult >> 16) & 1) << Flag::CF) | ((m_lastResult & 1) << Flag::PF) |
        ((m_lastResult == 0) ? (1 << Flag::ZF) : 0) | (((m_lastResult >> 15) & 1) << Flag::SF);
}

inline uint16_t Cpu::ModRmLoad16(uint8_t *ip)
{
    uint8_t modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI]);
        ModRm_Case(0x01): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI]);
        ModRm_Case(0x02): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI]);
        ModRm_Case(0x03): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI]);
        ModRm_Case(0x04): return Load16(m_segmentBase      + m_register[Register::SI]);
        ModRm_Case(0x05): return Load16(m_segmentBase      + m_register[Register::DI]);
        ModRm_Case(0x06): return Load16(m_segmentBase      + Disp16(ip));
        ModRm_Case(0x07): return Load16(m_segmentBase      + m_register[Register::BX]);
        // mod 01
        ModRm_Case(0x40): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x41): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x42): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x43): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x44): return Load16(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip));
        ModRm_Case(0x45): return Load16(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip));
        ModRm_Case(0x46): return Load16(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip));
        ModRm_Case(0x47): return Load16(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip));
        // mod 10
        ModRm_Case(0x80): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x81): return Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x82): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x83): return Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x84): return Load16(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip));
        ModRm_Case(0x85): return Load16(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip));
        ModRm_Case(0x86): return Load16(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip));
        ModRm_Case(0x87): return Load16(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip));
        // mod 11
        ModRm_Case(0xc0): return m_register[0];
        ModRm_Case(0xc1): return m_register[1];
        ModRm_Case(0xc2): return m_register[2];
        ModRm_Case(0xc3): return m_register[3];
        ModRm_Case(0xc4): return m_register[4];
        ModRm_Case(0xc5): return m_register[5];
        ModRm_Case(0xc6): return m_register[6];
        ModRm_Case(0xc7): return m_register[7];
    }

    return 0;
}

inline uint8_t Cpu::ModRmLoad8(uint8_t *ip)
{
    uint8_t modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI]);
        ModRm_Case(0x01): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI]);
        ModRm_Case(0x02): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI]);
        ModRm_Case(0x03): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI]);
        ModRm_Case(0x04): return Load8(m_segmentBase      + m_register[Register::SI]);
        ModRm_Case(0x05): return Load8(m_segmentBase      + m_register[Register::DI]);
        ModRm_Case(0x06): return Load8(m_segmentBase      + Disp16(ip));
        ModRm_Case(0x07): return Load8(m_segmentBase      + m_register[Register::BX]);
        // mod 01
        ModRm_Case(0x40): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x41): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x42): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x43): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x44): return Load8(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip));
        ModRm_Case(0x45): return Load8(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip));
        ModRm_Case(0x46): return Load8(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip));
        ModRm_Case(0x47): return Load8(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip));
        // mod 10
        ModRm_Case(0x80): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x81): return Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x82): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x83): return Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x84): return Load8(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip));
        ModRm_Case(0x85): return Load8(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip));
        ModRm_Case(0x86): return Load8(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip));
        ModRm_Case(0x87): return Load8(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip));
        // mod 11
        ModRm_Case(0xc0): return m_register[0];
        ModRm_Case(0xc1): return m_register[1];
        ModRm_Case(0xc2): return m_register[2];
        ModRm_Case(0xc3): return m_register[3];
        ModRm_Case(0xc4): return m_register[4];
        ModRm_Case(0xc5): return m_register[5];
        ModRm_Case(0xc6): return m_register[6];
        ModRm_Case(0xc7): return m_register[7];
    }

    return 0;
}

inline void Cpu::ModRmStore16(uint8_t *ip, uint16_t value)
{
    uint8_t modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI], value); break;
        ModRm_Case(0x01): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI], value); break;
        ModRm_Case(0x02): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI], value); break;
        ModRm_Case(0x03): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI], value); break;
        ModRm_Case(0x04): Store16(m_segmentBase      + m_register[Register::SI], value);                            break;
        ModRm_Case(0x05): Store16(m_segmentBase      + m_register[Register::DI], value);                            break;
        ModRm_Case(0x06): Store16(m_segmentBase      + Disp16(ip)              , value);                            break;
        ModRm_Case(0x07): Store16(m_segmentBase      + m_register[Register::BX], value);                            break;
        // mod 01
        ModRm_Case(0x40): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip), value);  break;
        ModRm_Case(0x41): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip), value);  break;
        ModRm_Case(0x42): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip), value);  break;
        ModRm_Case(0x43): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip), value);  break;
        ModRm_Case(0x44): Store16(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip), value);  break;
        ModRm_Case(0x45): Store16(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip), value);  break;
        ModRm_Case(0x46): Store16(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip), value);  break;
        ModRm_Case(0x47): Store16(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip), value);  break;
        // mod 10
        ModRm_Case(0x80): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip), value); break;
        ModRm_Case(0x81): Store16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip), value); break;
        ModRm_Case(0x82): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip), value); break;
        ModRm_Case(0x83): Store16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip), value); break;
        ModRm_Case(0x84): Store16(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip), value); break;
        ModRm_Case(0x85): Store16(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip), value); break;
        ModRm_Case(0x86): Store16(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip), value); break;
        ModRm_Case(0x87): Store16(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip), value); break;
        // mod 11
        ModRm_Case(0xc0): m_register[0] = value; break;
        ModRm_Case(0xc1): m_register[1] = value; break;
        ModRm_Case(0xc2): m_register[2] = value; break;
        ModRm_Case(0xc3): m_register[3] = value; break;
        ModRm_Case(0xc4): m_register[4] = value; break;
        ModRm_Case(0xc5): m_register[5] = value; break;
        ModRm_Case(0xc6): m_register[6] = value; break;
        ModRm_Case(0xc7): m_register[7] = value; break;
    }
}

inline void Cpu::ModRmStore8(uint8_t *ip, uint8_t value)
{
    uint8_t modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI], value); break;
        ModRm_Case(0x01): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI], value); break;
        ModRm_Case(0x02): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI], value); break;
        ModRm_Case(0x03): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI], value); break;
        ModRm_Case(0x04): Store8(m_segmentBase      + m_register[Register::SI], value);                            break;
        ModRm_Case(0x05): Store8(m_segmentBase      + m_register[Register::DI], value);                            break;
        ModRm_Case(0x06): Store8(m_segmentBase      + Disp16(ip)              , value);                            break;
        ModRm_Case(0x07): Store8(m_segmentBase      + m_register[Register::BX], value);                            break;
        // mod 01
        ModRm_Case(0x40): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip), value);  break;
        ModRm_Case(0x41): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip), value);  break;
        ModRm_Case(0x42): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip), value);  break;
        ModRm_Case(0x43): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip), value);  break;
        ModRm_Case(0x44): Store8(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip), value);  break;
        ModRm_Case(0x45): Store8(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip), value);  break;
        ModRm_Case(0x46): Store8(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip), value);  break;
        ModRm_Case(0x47): Store8(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip), value);  break;
        // mod 10
        ModRm_Case(0x80): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip), value); break;
        ModRm_Case(0x81): Store8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip), value); break;
        ModRm_Case(0x82): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip), value); break;
        ModRm_Case(0x83): Store8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip), value); break;
        ModRm_Case(0x84): Store8(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip), value); break;
        ModRm_Case(0x85): Store8(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip), value); break;
        ModRm_Case(0x86): Store8(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip), value); break;
        ModRm_Case(0x87): Store8(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip), value); break;
        // mod 11
        ModRm_Case(0xc0): m_register[0] = (m_register[0] & 0xff00) | value;        break;
        ModRm_Case(0xc1): m_register[1] = (m_register[1] & 0xff00) | value;        break;
        ModRm_Case(0xc2): m_register[2] = (m_register[2] & 0xff00) | value;        break;
        ModRm_Case(0xc3): m_register[3] = (m_register[3] & 0xff00) | value;        break;
        ModRm_Case(0xc4): m_register[0] = (m_register[0] & 0x00ff) | (value << 8); break;
        ModRm_Case(0xc5): m_register[1] = (m_register[1] & 0x00ff) | (value << 8); break;
        ModRm_Case(0xc6): m_register[2] = (m_register[2] & 0x00ff) | (value << 8); break;
        ModRm_Case(0xc7): m_register[3] = (m_register[3] & 0x00ff) | (value << 8); break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOp16(uint8_t *ip, uint16_t regValue, F&& f)
{
    std::size_t ea;
    uint8_t     modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); m_lastResult = f(Load16(ea), regValue); Store16(ea, m_lastResult); break;
        // mod 11
        ModRm_Case(0xc0): m_lastResult = f(m_register[0], regValue); m_register[0] = m_lastResult; break;
        ModRm_Case(0xc1): m_lastResult = f(m_register[1], regValue); m_register[1] = m_lastResult; break;
        ModRm_Case(0xc2): m_lastResult = f(m_register[2], regValue); m_register[2] = m_lastResult; break;
        ModRm_Case(0xc3): m_lastResult = f(m_register[3], regValue); m_register[3] = m_lastResult; break;
        ModRm_Case(0xc4): m_lastResult = f(m_register[4], regValue); m_register[4] = m_lastResult; break;
        ModRm_Case(0xc5): m_lastResult = f(m_register[5], regValue); m_register[5] = m_lastResult; break;
        ModRm_Case(0xc6): m_lastResult = f(m_register[6], regValue); m_register[6] = m_lastResult; break;
        ModRm_Case(0xc7): m_lastResult = f(m_register[7], regValue); m_register[7] = m_lastResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOp8(uint8_t *ip, uint8_t regValue, F&& f)
{
    std::size_t ea;
    uint8_t     modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); m_lastResult = f(Load8(ea), regValue); Store8(ea, m_lastResult); break;
        // mod 11
        ModRm_Case(0xc0): m_lastResult = f(m_register[0] & 0xff, regValue); m_register[0] = (m_register[0] & 0xff00) | m_lastResult;        break;
        ModRm_Case(0xc1): m_lastResult = f(m_register[1] & 0xff, regValue); m_register[1] = (m_register[1] & 0xff00) | m_lastResult;        break;
        ModRm_Case(0xc2): m_lastResult = f(m_register[2] & 0xff, regValue); m_register[2] = (m_register[2] & 0xff00) | m_lastResult;        break;
        ModRm_Case(0xc3): m_lastResult = f(m_register[3] & 0xff, regValue); m_register[3] = (m_register[3] & 0xff00) | m_lastResult;        break;
        ModRm_Case(0xc4): m_lastResult = f(m_register[0] >> 8,   regValue); m_register[0] = (m_register[0] & 0x00ff) | (m_lastResult << 8); break;
        ModRm_Case(0xc5): m_lastResult = f(m_register[1] >> 8,   regValue); m_register[1] = (m_register[1] & 0x00ff) | (m_lastResult << 8); break;
        ModRm_Case(0xc6): m_lastResult = f(m_register[2] >> 8,   regValue); m_register[2] = (m_register[2] & 0x00ff) | (m_lastResult << 8); break;
        ModRm_Case(0xc7): m_lastResult = f(m_register[3] >> 8,   regValue); m_register[3] = (m_register[3] & 0x00ff) | (m_lastResult << 8); break;
    }
}

void Cpu::ExecuteInstruction()
{
    uint16_t opcode, length, offset, value;
    uint16_t *reg;
    uint8_t  *ip;

    do
    {
        ip = m_memory + m_register[Register::CS] * 16 + m_register[Register::IP];
        opcode = *ip++;

        switch(opcode)
        {
            case 0x06: // push es
                Push16(m_register[Register::ES]);
                m_register[Register::IP] += 1;
                DumpInst(1, "push es", 0);
                break;

            case 0x0e: // push cs
                Push16(m_register[Register::CS]);
                m_register[Register::IP] += 1;
                DumpInst(1, "push cs", 0);
                break;

            case 0x16: // push ss
                Push16(m_register[Register::SS]);
                m_register[Register::IP] += 1;
                DumpInst(1, "push ss", 0);
                break;

            case 0x1e: // push ds
                Push16(m_register[Register::DS]);
                m_register[Register::IP] += 1;
                DumpInst(1, "push ds", 0);
                break;

            case 0x1f: // pop ds
                m_register[Register::DS] = Pop16();
                m_register[Register::IP] += 1;
                DumpInst(1, "pop ds", 0);
                break;

            case 0x26: // prefix - ES override
                m_segmentBase      = m_register[Register::ES] * 16;
                m_stackSegmentBase = m_register[Register::ES] * 16;
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "ES:", 0);
                continue;

            case 0x2e: // prefix - CS override
                m_segmentBase      = m_register[Register::CS] * 16;
                m_stackSegmentBase = m_register[Register::CS] * 16;
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "CS:", 0);
                continue;

//             case 0x30: // xor r/m8, r8
//                 break;

            case 0x31: // xor r/m16, r16
                value = *ModRmToReg(*ip);
                ModRmModifyOp16(ip, value, [](uint16_t op1, uint16_t op2) { return op1 ^ op2; });
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "xor %s, %s", ModRmToStr(ip).c_str(), s_regName16[(*ip >> 3) & 0x07]);
                break;

//             case 0x32: // xor r8, r/m8
//                 break;

            case 0x33: // xor r16, r/m16
                reg = ModRmToReg(*ip);
                m_lastResult = *reg = *reg ^ ModRmLoad16(ip);
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "xor %s, %s", s_regName16[(*ip >> 3) & 0x07], ModRmToStr(ip).c_str());
                break;

            case 0x36: // prefix - SS override
                m_segmentBase      = m_register[Register::SS] * 16;
                m_stackSegmentBase = m_register[Register::SS] * 16;
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "SS:", 0);
                continue;

            case 0x3e: // prefix - DS override
                m_segmentBase      = m_register[Register::DS] * 16;
                m_stackSegmentBase = m_register[Register::DS] * 16;
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "DS:", 0);
                continue;

            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:
                Push16(m_register[opcode - 0x50]);
                m_register[Register::IP] += 1;

                DumpInst(1, "push %s", Reg16ToStr(opcode - 0x50));
                break;

            case 0x58: case 0x59: case 0x5a: case 0x5b:
            case 0x5c: case 0x5d: case 0x5e: case 0x5f:
                m_register[opcode - 0x58] = Pop16();
                m_register[Register::IP] += 1;

                DumpInst(1, "pop %s", Reg16ToStr(opcode - 0x58));
                break;

            case 0x89: // mov r/m16, r16
                value = *ModRmToReg(*ip);
                ModRmStore16(ip, value);
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "mov %s, %s", ModRmToStr(ip).c_str(), s_regName16[(*ip >> 3) & 0x07]);
                break;

            case 0x8b: // mov r16, r/m16
                *ModRmToReg(*ip) = ModRmLoad16(ip);
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "mov %s, %s", s_regName16[(*ip >> 3) & 0x07], ModRmToStr(ip).c_str());
                break;

            case 0x8c: // mov r/m16, Sreg,
                value = *ModRmToSReg(*ip);
                ModRmStore16(ip, value);
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "mov %s, %s", ModRmToStr(ip).c_str(), s_sregName[(*ip >> 3) & 0x07]);
                break;

            case 0x8e: // mov Sreg, r/m16
                *ModRmToSReg(*ip) = ModRmLoad16(ip);
                m_register[Register::IP] += s_modRmInstLen[*ip];

                DumpInst(s_modRmInstLen[*ip], "mov %s, %s", s_sregName[(*ip >> 3) & 0x07], ModRmToStr(ip).c_str());
                break;

            case 0x9c: // pushf
                RecalcFlags();
                Push16(m_register[Register::FLAG]);
                m_register[Register::IP] += 1;

                DumpInst(1, "pushf", 0);
                break;

            case 0x9d: // popf
                value = Pop16();
                value |= 1 << Flag::Always1;
                value &= ~(1 << Flag::Always0);
                m_register[Register::FLAG] = value;
                m_register[Register::IP] += 1;

                DumpInst(1, "popf", 0);
                break;

            case 0xa3: // mov moffs16, ax
                Store16(m_segmentBase + Disp16(ip), m_register[Register::AX]);
                m_register[Register::IP] += 3;

                DumpInst(3, "mov [%04x], ax", offset);
                break;

            case 0xb0: case 0xb1: case 0xb2: case 0xb3: // mov reg8, imm8     (reg8 = al, cl, dl, bl)
                reg  = &m_register[opcode - 0xb0];
                *reg = (*reg & 0xff00) | *ip;
                m_register[Register::IP] += 2;

                DumpInst(2, "mov %s, 0x%02x", Reg8LowToStr(opcode - 0xb0), *ip);
                break;

            case 0xb4: case 0xb5: case 0xb6: case 0xb7: // mov reg8, imm8     (reg8 = ah, ch, dh, bh)
                reg = &m_register[opcode - 0xb4];
                *reg = (*reg & 0x00ff) | (*ip << 8);
                m_register[Register::IP] += 2;

                DumpInst(2, "mov %s, 0x%02x", Reg8HighToStr(opcode - 0xb4), *ip);
                break;

            case 0xb8: case 0xb9: case 0xba: case 0xbb: // mov reg16, imm16
            case 0xbc: case 0xbd: case 0xbe: case 0xbf:
                m_register[opcode - 0xb8] = Imm16(ip);
                m_register[Register::IP] += 3;

                DumpInst(3, "mov %s, 0x%04x", Reg16ToStr(opcode - 0xb8), Imm16(ip));
                break;

            case 0xe8: // call rel16
                offset = Disp16(ip);
                m_register[Register::IP] += 3;
                DumpInst(3, "call %s0x%04x", (offset & 0x8000) ? "-" : "", offset & 0x7fff);
                Push16(m_register[Register::IP]);
                m_register[Register::IP] += offset;
                break;

            case 0xc3:
                DumpInst(0, "ret", 0);
                m_register[Register::IP] = Pop16();
                break;

            case 0xcd:
                m_register[Register::IP] += 2;
                DumpInst(2, "int 0x%02x", *ip);
                onSoftIrq(this, *ip);
                break;

            default:
                DumpMem(m_memory, m_register[Register::CS], m_register[Register::IP], 8);
                DebugPrintf("Invalid opcode 0x%02x\n", *(ip - 1));
                m_state |= State::InvalidOp;
                break;
        }

        if (m_state)
        {
            if (m_state & State::InvalidOp)
                break;

            if (m_state & State::SegmentOverride)
            {
                m_segmentBase      = m_register[Register::DS] * 16;
                m_stackSegmentBase = m_register[Register::SS] * 16;
            }

            m_state = 0;
        }

    }while(false);
}

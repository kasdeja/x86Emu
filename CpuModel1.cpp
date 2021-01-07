#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <string>
#include "CpuModel1.h"

#define DEBUG

#ifdef DEBUG
#define DebugPrintf(...) printf(__VA_ARGS__)
static const char* s_regName16[]   = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static const char* s_regName8l[]   = { "al", "cl", "dl", "bl" };
static const char* s_regName8h[]   = { "ah", "ch", "dh", "bh" };
static const char* s_sregName[]    = { "es", "cs", "ss", "ds" };
static const char  s_hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

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

using namespace CpuModel1;

// constructor & destructor
Cpu::Cpu(uint8_t *memory)
    : m_memory(memory)
{
    std::fill(m_register, m_register + 16, 0);
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
    m_state        = 0;
    m_segment      = m_register[Register::DS];
    m_stackSegment = m_register[Register::SS];

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

inline uint16_t* Cpu::Reg(uint8_t *ip)
{
    uint8_t modRm = *ip;
    return &m_register[(modRm >> 3) & 0x07];
}

inline uint16_t* Cpu::SReg(uint8_t *ip)
{
    uint8_t modRm = *ip;
    return &m_register[Register::ES + ((modRm >> 3) & 0x07)];
}

uint16_t* Cpu::Mem16(uint16_t segment, uint16_t offset)
{
    return reinterpret_cast<uint16_t *>(m_memory + segment * 16 + offset);
}

uint8_t* Cpu::Mem8(uint16_t segment, uint16_t offset)
{
    return m_memory + segment * 16 + offset;
}

void Cpu::Push16(uint16_t value)
{
    m_register[Register::SP] -= 2;
    *Mem16(m_stackSegment, m_register[Register::SP]) = value;
}

uint16_t Cpu::Pop16()
{
    uint16_t value = *Mem16(m_stackSegment, m_register[Register::SP]);
    m_register[Register::SP] += 2;
    return value;
}

uint16_t Cpu::LoadU16(uint8_t* ip)
{
    return *reinterpret_cast<uint16_t *>(ip);
}

int8_t Cpu::LoadS8(uint8_t* ip)
{
    return *reinterpret_cast<char *>(ip);
}

inline int Cpu::ModRm(uint8_t *ip, uint16_t **op)
{
    uint8_t modRm = *ip;

    switch(((modRm >> 3) & 0x18) + (modRm & 0x07))
    {
        // mod 00
        case 0:  *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::SI]); return 1;
        case 1:  *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::DI]); return 1;
        case 2:  *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::SI]); return 1;
        case 3:  *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::DI]); return 1;
        case 4:  *op = Mem16(m_segment,      m_register[Register::SI]);                            return 1;
        case 5:  *op = Mem16(m_segment,      m_register[Register::DI]);                            return 1;
        case 6:  *op = Mem16(m_segment,      LoadU16(ip + 1));                                     return 3;
        case 7:  *op = Mem16(m_segment,      m_register[Register::BX]);                            return 1;

        // mod 01
        case 8:  *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::SI] + LoadS8(ip + 1)); return 2;
        case 9:  *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::DI] + LoadS8(ip + 1)); return 2;
        case 10: *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::SI] + LoadS8(ip + 1)); return 2;
        case 11: *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::DI] + LoadS8(ip + 1)); return 2;
        case 12: *op = Mem16(m_segment,      m_register[Register::SI]                            + LoadS8(ip + 1)); return 2;
        case 13: *op = Mem16(m_segment,      m_register[Register::DI]                            + LoadS8(ip + 1)); return 2;
        case 14: *op = Mem16(m_segment,      m_register[Register::BP]                            + LoadS8(ip + 1)); return 2;
        case 15: *op = Mem16(m_segment,      m_register[Register::BX]                            + LoadS8(ip + 1)); return 2;

        // mod 10
        case 16: *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::SI] + LoadU16(ip + 1)); return 3;
        case 17: *op = Mem16(m_segment,      m_register[Register::BX] + m_register[Register::DI] + LoadU16(ip + 1)); return 3;
        case 18: *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::SI] + LoadU16(ip + 1)); return 3;
        case 19: *op = Mem16(m_stackSegment, m_register[Register::BP] + m_register[Register::DI] + LoadU16(ip + 1)); return 3;
        case 20: *op = Mem16(m_segment,      m_register[Register::SI]                            + LoadU16(ip + 1)); return 3;
        case 21: *op = Mem16(m_segment,      m_register[Register::DI]                            + LoadU16(ip + 1)); return 3;
        case 22: *op = Mem16(m_segment,      m_register[Register::BP]                            + LoadU16(ip + 1)); return 3;
        case 23: *op = Mem16(m_segment,      m_register[Register::BX]                            + LoadU16(ip + 1)); return 3;

        // mod 11
        case 24: *op = &m_register[0]; return 1;
        case 25: *op = &m_register[1]; return 1;
        case 26: *op = &m_register[2]; return 1;
        case 27: *op = &m_register[3]; return 1;
        case 28: *op = &m_register[4]; return 1;
        case 29: *op = &m_register[5]; return 1;
        case 30: *op = &m_register[6]; return 1;
        case 31: *op = &m_register[7]; return 1;
    }

    return 0;
}

void Cpu::ExecuteInstruction()
{
    uint16_t opcode, length, offset;
    uint16_t *op1, *op2, *reg;
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
                m_segment      = m_register[Register::ES];
                m_stackSegment = m_register[Register::ES];
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "ES:", 0);
                continue;

            case 0x2e: // prefix - CS override
                m_segment      = m_register[Register::CS];
                m_stackSegment = m_register[Register::CS];
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "CS:", 0);
                continue;

            case 0x36: // prefix - SS override
                m_segment      = m_register[Register::SS];
                m_stackSegment = m_register[Register::SS];
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "SS:", 0);
                continue;

            case 0x3e: // prefix - DS override
                m_segment      = m_register[Register::DS];
                m_stackSegment = m_register[Register::DS];
                m_register[Register::IP] += 1;
                m_state |= State::SegmentOverride;

                DumpInst(1, "DS:", 0);
                continue;

            case 0x89: // mov r/m16, r16
                length = ModRm(ip, &op1);
                op2    = Reg(ip);
                *op1   = *op2;
                m_register[Register::IP] += 1 + length;

                DumpInst(1 + length, "mov %s, %s", ModRmToStr(ip).c_str(), s_regName16[(*ip >> 3) & 0x07]);
                break;

            case 0x8b: // mov r16, r/m16
                length = ModRm(ip, &op1);
                op2    = Reg(ip);
                *op2   = *op1;
                m_register[Register::IP] += 1 + length;

                DumpInst(1 + length, "mov %s, %s", s_regName16[(*ip >> 3) & 0x07], ModRmToStr(ip).c_str());
                break;

            case 0x8c: // mov r/m16, Sreg,
                length = ModRm(ip, &op1);
                op2    = SReg(ip);
                *op1   = *op2;
                m_register[Register::IP] += 1 + length;

                DumpInst(1 + length, "mov %s, %s", ModRmToStr(ip).c_str(), s_sregName[(*ip >> 3) & 0x07]);
                break;

            case 0x8e: // mov Sreg, r/m16
                length = ModRm(ip, &op1);
                op2    = SReg(ip);
                *op2   = *op1;
                m_register[Register::IP] += 1 + length;

                DumpInst(1 + length, "mov %s, %s", s_sregName[(*ip >> 3) & 0x07], ModRmToStr(ip).c_str());
                break;

            case 0xa3: // mov moffs16, ax
                offset = LoadU16(ip);
                *Mem16(m_segment, offset) = m_register[Register::AX];
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
                m_register[opcode - 0xb8] = LoadU16(ip);
                m_register[Register::IP] += 3;

                DumpInst(3, "mov %s, 0x%04x", Reg16ToStr(opcode - 0xb8), LoadU16(ip));
                break;

            case 0xe8: // call rel16
                offset = LoadU16(ip);
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
                m_segment      = m_register[Register::DS];
                m_stackSegment = m_register[Register::SS];
            }

            m_state = 0;
        }

    }while(false);
}

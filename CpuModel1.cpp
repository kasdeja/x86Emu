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

uint8_t* Cpu::GetMem()
{
    return m_memory;
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
                onSoftIrq(this, *ip);

                DumpInst(2, "int 0x%02x", *ip);
                break;

            case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:            case 0x07:
            case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:            case 0x0f:
            case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:            case 0x17:
            case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:
            case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:            case 0x27:
            case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d:            case 0x2f:
            case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:            case 0x37:
            case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d:            case 0x3f:
            case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
            case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
            case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
            case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
            case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
                                  case 0x8a:                       case 0x8d:            case 0x8f:
            case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
            case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            case 0xa0: case 0xa1: case 0xa2:            case 0xa4: case 0xa5: case 0xa6: case 0xa7:
            case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
            // 0xb0
            // 0xb8
            case 0xc0: case 0xc1: case 0xc2:            case 0xc4: case 0xc5: case 0xc6: case 0xc7:
            case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc:            case 0xce: case 0xcf:
            case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
            case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
            case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
                       case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
            case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
            case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
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

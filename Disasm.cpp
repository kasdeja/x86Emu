#include "Disasm.h"
#include "CpuInterface.h"
#include "Memory.h"

const char* Disasm::s_regName16[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char* Disasm::s_regName8l[] = { "al", "cl", "dl", "bl" };
const char* Disasm::s_regName8h[] = { "ah", "ch", "dh", "bh" };
const char* Disasm::s_regName8[]  = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const char* Disasm::s_sregName[]  = { "es", "cs", "ss", "ds" };
const char  Disasm::s_hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const int Disasm::s_modRmInstLen[256] = {
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
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
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

// constructor
Disasm::Disasm(CpuInterface& cpu, Memory& memory)
    : m_cpu   (cpu)
    , m_memory(memory.GetMem())
{
}

// private methods
inline std::string Disasm::Reg16ToStr(uint8_t reg)
{
    return s_regName16[reg];
}

inline std::string Disasm::Reg8LowToStr(uint8_t reg)
{
    return s_regName8l[reg];
}

inline std::string Disasm::Reg8HighToStr(uint8_t reg)
{
    return s_regName8h[reg];
}

inline std::string Disasm::Hex16(uint16_t value)
{
    char str[5];

    str[0] = s_hexDigits[(value >> 12) & 0x0f];
    str[1] = s_hexDigits[(value >>  8) & 0x0f];
    str[2] = s_hexDigits[(value >>  4) & 0x0f];
    str[3] = s_hexDigits[(value      ) & 0x0f];
    str[4] = 0;

    return str;
}

inline std::string Disasm::Hex8(uint8_t value)
{
    char str[3];

    str[0] = s_hexDigits[(value >>  4) & 0x0f];
    str[1] = s_hexDigits[(value      ) & 0x0f];
    str[2] = 0;

    return str;
}

inline std::string Disasm::Dec8(int8_t value)
{
    return (value < 0) ? "- " + std::to_string(-value) : "+ " + std::to_string(-value);
}

inline std::string Disasm::Disp16(uint8_t* ip)
{
    return "0x" + Hex16(*reinterpret_cast<uint16_t *>(ip));
}

inline std::string Disasm::Disp8(uint8_t* ip)
{
    char value = *ip;

    return (value < 0) ? "-0x" + Hex8(*ip & 0x7f) : "0x" + Hex8(*ip);
}

inline std::string Disasm::Imm16(uint8_t* ip)
{
    return "0x" + Hex16(*reinterpret_cast<uint16_t *>(ip));
}

inline std::string Disasm::Imm8(uint8_t* ip)
{
    return "0x" + Hex8(*ip);
}

std::string Disasm::DumpMem(uint16_t segment, uint16_t offset, int length)
{
    uint8_t* mem = m_memory + segment * 16 + offset;
    int      n;

    std::string result = Hex16(segment) + ":" + Hex16(offset) + " | ";

    for(n = 0; n < length; n++)
        result += Hex8(mem[n]) + " ";

    for(; n < 8; n++)
        result += "   ";

    return result;
}

inline std::string Disasm::ModRmReg16(uint8_t *ip)
{
    return s_regName16[(*ip >> 3) & 0x07];
}

inline std::string Disasm::ModRmReg8(uint8_t *ip)
{
    return s_regName8[(*ip >> 3) & 0x07];
}

inline std::string Disasm::ModRmSReg(uint8_t *ip)
{
    return s_sregName[(*ip >> 3) & 0x07];
}

std::string Disasm::ModRm(uint8_t *ip)
{
    std::string op;
    uint8_t     modRm = *ip++;
    int8_t*     disp8  = reinterpret_cast<int8_t *>(ip);
    uint16_t*   disp16 = reinterpret_cast<uint16_t *>(ip);

    switch(((modRm >> 3) & 0x18) + (modRm & 0x07))
    {
        // mod 00
        case 0:  op = "[bx + si]";               break;
        case 1:  op = "[bx + di]";               break;
        case 2:  op = "[bp + si]";               break;
        case 3:  op = "[bp + di]";               break;
        case 4:  op = "[si]";                    break;
        case 5:  op = "[di]";                    break;
        case 6:  op = "[" + Hex16(*disp16) + "]"; break;
        case 7:  op = "[bx]";                    break;

        // mod 01
        case 8:  op = "[bx + si " + Dec8(*disp8) + "]"; break;
        case 9:  op = "[bx + di " + Dec8(*disp8) + "]"; break;
        case 10: op = "[bp + si " + Dec8(*disp8) + "]"; break;
        case 11: op = "[bp + di " + Dec8(*disp8) + "]"; break;
        case 12: op = "[si "      + Dec8(*disp8) + "]"; break;
        case 13: op = "[si "      + Dec8(*disp8) + "]"; break;
        case 14: op = "[bp "      + Dec8(*disp8) + "]"; break;
        case 15: op = "[bx "      + Dec8(*disp8) + "]"; break;

        // mod 10
        case 16: op = "[bx + si + " + Hex16(*disp16) + "]"; break;
        case 17: op = "[bx + di + " + Hex16(*disp16) + "]"; break;
        case 18: op = "[bp + si + " + Hex16(*disp16) + "]"; break;
        case 19: op = "[bp + di + " + Hex16(*disp16) + "]"; break;
        case 20: op = "[si + "      + Hex16(*disp16) + "]"; break;
        case 21: op = "[si + "      + Hex16(*disp16) + "]"; break;
        case 22: op = "[bp + "      + Hex16(*disp16) + "]"; break;
        case 23: op = "[bx + "      + Hex16(*disp16) + "]"; break;

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

// public methods
std::string Disasm::Process()
{
    uint16_t segment = m_cpu.GetReg16(CpuInterface::CS);
    uint16_t offset  = m_cpu.GetReg16(CpuInterface::IP);

    uint8_t* ip = m_memory + segment * 16 + offset;
    uint8_t  opcode = *ip++;
    int      length = 0;

    std::string instr;

    switch(opcode)
    {
        case 0x06: // push es
            instr  = "push es";
            length = 1;
            break;

        case 0x08: // or r/m8, r8
            instr  = "or " + ModRm(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x09: // or r/m16, r16
            instr  = "or " + ModRm(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0a: // or r8, r/m8
            instr  = "or " + ModRmReg8(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0b: // or r16, r/m16
            instr  = "or " + ModRmReg16(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0e: // push cs
            instr  = "push cs";
            length = 1;
            break;

        case 0x16: // push ss
            instr  = "push ss";
            length = 1;
            break;

        case 0x1e: // push ds
            instr  = "push ds";
            length = 1;
            break;

        case 0x1f: // pop ds
            instr  = "pop ds";
            length = 1;
            break;

        case 0x26: // prefix - ES override
            instr  = "ES:";
            length = 1;
            break;

        case 0x2e: // prefix - CS override
            instr  = "CS:";
            length = 1;
            break;

        case 0x30: // xor r/m8, r8
            instr  = "xor " + ModRm(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x31: // xor r/m16, r16
            instr  = "xor " + ModRm(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x32: // xor r8, r/m8
            instr  = "xor " + ModRmReg8(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x33: // xor r16, r/m16
            instr  = "xor " + ModRmReg16(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x36: // prefix - SS override
            instr  = "SS:";
            length = 1;
            break;

        case 0x3e: // prefix - DS override
            instr  = "DS:";
            length = 1;
            break;

        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            instr  = "push " + Reg16ToStr(opcode - 0x50);
            length = 1;
            break;

        case 0x58: case 0x59: case 0x5a: case 0x5b:
        case 0x5c: case 0x5d: case 0x5e: case 0x5f:
            instr  = "pop " + Reg16ToStr(opcode - 0x58);
            length = 1;
            break;

        case 0x70: // jo rel8
            instr = "jo " + Disp8(ip);
            length = 2;
            break;

        case 0x71: // jno rel8
            instr = "jno " + Disp8(ip);
            length = 2;
            break;

        case 0x72: // jb rel8
            instr = "jb " + Disp8(ip);
            length = 2;
            break;

        case 0x73: // jnb rel8
            instr = "jnb " + Disp8(ip);
            length = 2;
            break;

        case 0x74: // je rel8
            instr = "je " + Disp8(ip);
            length = 2;
            break;

        case 0x75: // jne rel8
            instr = "jne " + Disp8(ip);
            length = 2;
            break;

        case 0x76: // jna rel8
            instr = "jna " + Disp8(ip);
            length = 2;
            break;

        case 0x77: // ja rel8
            instr = "ja " + Disp8(ip);
            length = 2;
            break;

        case 0x78: // js rel8
            instr = "js " + Disp8(ip);
            length = 2;
            break;

        case 0x79: // jns rel8
            instr = "jns " + Disp8(ip);
            length = 2;
            break;

        case 0x7a: // jp rel8
            instr = "jp " + Disp8(ip);
            length = 2;
            break;

        case 0x7b: // jnp rel8
            instr = "jnp " + Disp8(ip);
            length = 2;
            break;

        case 0x7c: // jl rel8
            instr = "jl " + Disp8(ip);
            length = 2;
            break;

        case 0x7d: // jnl rel8
            instr = "jnl " + Disp8(ip);
            length = 2;
            break;

        case 0x7e: // jle rel8
            instr = "jle " + Disp8(ip);
            length = 2;
            break;

        case 0x7f: // jg rel8
            instr = "jg " + Disp8(ip);
            length = 2;
            break;

        case 0x89: // mov r/m16, r16
            instr  = "mov " + ModRm(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8b: // mov r16, r/m16
            instr  = "mov " + ModRmReg16(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8c: // mov r/m16, Sreg
            instr = "mov " + ModRm(ip) + ", " + ModRmSReg(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8e: // mov Sreg, r/m16
            instr = "mov " + ModRmSReg(ip) + ", " + ModRm(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x9c: // pushf
            instr = "pushf";
            length = 1;
            break;

        case 0x9d: // popf
            instr = "popf";
            length = 1;
            break;

        case 0xa3: // mov moffs16, ax
            instr = "mov [" + Hex16(*reinterpret_cast<uint16_t *>(ip)) + "], ax";
            length = 3;
            break;

        case 0xb0: case 0xb1: case 0xb2: case 0xb3: // mov reg8, imm8     (reg8 = al, cl, dl, bl)
            instr = "mov " + Reg8LowToStr(opcode - 0xb0) + ", " + Imm8(ip);
            length = 2;
            break;

        case 0xb4: case 0xb5: case 0xb6: case 0xb7: // mov reg8, imm8     (reg8 = ah, ch, dh, bh)
            instr = "mov " + Reg8HighToStr(opcode - 0xb4) + ", " + Imm8(ip);
            length = 2;
            break;

        case 0xb8: case 0xb9: case 0xba: case 0xbb: // mov reg16, imm16
        case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            instr = "mov " + Reg16ToStr(opcode - 0xb8) + ", " + Imm16(ip);
            length = 3;
            break;

        case 0xe8: // call rel16
            instr = "call " + Disp16(ip);
            length = 3;
            break;

        case 0xc3:
            instr = "ret";
            length = 1;
            break;

        case 0xcd:
            instr = "int " + Imm8(ip);
            length = 2;
            break;

        default:
            instr = "";
            length = 8;
            break;
    }

    return DumpMem(segment, offset, length) + instr;
}

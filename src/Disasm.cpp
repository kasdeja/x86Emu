#include "Disasm.h"
#include "CpuInterface.h"
#include "Memory.h"

const char* Disasm::s_regName16[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char* Disasm::s_regName8l[] = { "al", "cl", "dl", "bl" };
const char* Disasm::s_regName8h[] = { "ah", "ch", "dh", "bh" };
const char* Disasm::s_regName8[]  = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const char* Disasm::s_sregName[]  = { "es", "cs", "ss", "ds" };
const char  Disasm::s_hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
const char* Disasm::s_arithOp[]   = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char* Disasm::s_shiftOp[]   = { "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" };


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
    return (value < 0) ? "- " + std::to_string(-value) : "+ " + std::to_string(value);
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

inline std::string Disasm::Rel16(uint8_t* ip, uint16_t offset)
{
    return "0x" + Hex16(*reinterpret_cast<uint16_t *>(ip) + offset);
}

inline std::string Disasm::Rel8(uint8_t* ip, uint16_t offset)
{
    char rel8 = *ip;

    return "0x" + Hex16(rel8 + offset);
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

std::string Disasm::ModRm16(uint8_t *ip)
{
    std::string op;
    uint8_t     modRm = *ip++;
    int8_t*     disp8  = reinterpret_cast<int8_t *>(ip);
    uint16_t*   disp16 = reinterpret_cast<uint16_t *>(ip);

    switch(((modRm >> 3) & 0x18) + (modRm & 0x07))
    {
        // mod 00
        case 0:  op = "word ptr [bx + si]";                break;
        case 1:  op = "word ptr [bx + di]";                break;
        case 2:  op = "word ptr [bp + si]";                break;
        case 3:  op = "word ptr [bp + di]";                break;
        case 4:  op = "word ptr [si]";                     break;
        case 5:  op = "word ptr [di]";                     break;
        case 6:  op = "word ptr [" + Hex16(*disp16) + "]"; break;
        case 7:  op = "word ptr [bx]";                     break;

        // mod 01
        case 8:  op = "word ptr [bx + si " + Dec8(*disp8) + "]"; break;
        case 9:  op = "word ptr [bx + di " + Dec8(*disp8) + "]"; break;
        case 10: op = "word ptr [bp + si " + Dec8(*disp8) + "]"; break;
        case 11: op = "word ptr [bp + di " + Dec8(*disp8) + "]"; break;
        case 12: op = "word ptr [si "      + Dec8(*disp8) + "]"; break;
        case 13: op = "word ptr [si "      + Dec8(*disp8) + "]"; break;
        case 14: op = "word ptr [bp "      + Dec8(*disp8) + "]"; break;
        case 15: op = "word ptr [bx "      + Dec8(*disp8) + "]"; break;

        // mod 10
        case 16: op = "word ptr [bx + si + " + Hex16(*disp16) + "]"; break;
        case 17: op = "word ptr [bx + di + " + Hex16(*disp16) + "]"; break;
        case 18: op = "word ptr [bp + si + " + Hex16(*disp16) + "]"; break;
        case 19: op = "word ptr [bp + di + " + Hex16(*disp16) + "]"; break;
        case 20: op = "word ptr [si + "      + Hex16(*disp16) + "]"; break;
        case 21: op = "word ptr [si + "      + Hex16(*disp16) + "]"; break;
        case 22: op = "word ptr [bp + "      + Hex16(*disp16) + "]"; break;
        case 23: op = "word ptr [bx + "      + Hex16(*disp16) + "]"; break;

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

std::string Disasm::ModRm8(uint8_t *ip)
{
    std::string op;
    uint8_t     modRm = *ip++;
    int8_t*     disp8  = reinterpret_cast<int8_t *>(ip);
    uint16_t*   disp16 = reinterpret_cast<uint16_t *>(ip);

    switch(((modRm >> 3) & 0x18) + (modRm & 0x07))
    {
        // mod 00
        case 0:  op = "byte ptr [bx + si]";                break;
        case 1:  op = "byte ptr [bx + di]";                break;
        case 2:  op = "byte ptr [bp + si]";                break;
        case 3:  op = "byte ptr [bp + di]";                break;
        case 4:  op = "byte ptr [si]";                     break;
        case 5:  op = "byte ptr [di]";                     break;
        case 6:  op = "byte ptr [" + Hex16(*disp16) + "]"; break;
        case 7:  op = "byte ptr [bx]";                     break;

        // mod 01
        case 8:  op = "byte ptr [bx + si " + Dec8(*disp8) + "]"; break;
        case 9:  op = "byte ptr [bx + di " + Dec8(*disp8) + "]"; break;
        case 10: op = "byte ptr [bp + si " + Dec8(*disp8) + "]"; break;
        case 11: op = "byte ptr [bp + di " + Dec8(*disp8) + "]"; break;
        case 12: op = "byte ptr [si "      + Dec8(*disp8) + "]"; break;
        case 13: op = "byte ptr [si "      + Dec8(*disp8) + "]"; break;
        case 14: op = "byte ptr [bp "      + Dec8(*disp8) + "]"; break;
        case 15: op = "byte ptr [bx "      + Dec8(*disp8) + "]"; break;

        // mod 10
        case 16: op = "byte ptr [bx + si + " + Hex16(*disp16) + "]"; break;
        case 17: op = "byte ptr [bx + di + " + Hex16(*disp16) + "]"; break;
        case 18: op = "byte ptr [bp + si + " + Hex16(*disp16) + "]"; break;
        case 19: op = "byte ptr [bp + di + " + Hex16(*disp16) + "]"; break;
        case 20: op = "byte ptr [si + "      + Hex16(*disp16) + "]"; break;
        case 21: op = "byte ptr [si + "      + Hex16(*disp16) + "]"; break;
        case 22: op = "byte ptr [bp + "      + Hex16(*disp16) + "]"; break;
        case 23: op = "byte ptr [bx + "      + Hex16(*disp16) + "]"; break;

        // mod 11
        case 24: op = "al"; break;
        case 25: op = "cl"; break;
        case 26: op = "dl"; break;
        case 27: op = "bl"; break;
        case 28: op = "ah"; break;
        case 29: op = "ch"; break;
        case 30: op = "dh"; break;
        case 31: op = "bh"; break;
    }

    return op;
}

std::string Disasm::Handle80h(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    return std::string(s_arithOp[op]) + " " + ModRm8(ip) + ", " + Imm8(ip + s_modRmInstLen[modrm] - 1);
}

std::string Disasm::Handle81h(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    return std::string(s_arithOp[op]) + " " + ModRm16(ip) + ", " + Imm16(ip + s_modRmInstLen[modrm] - 1);
}

std::string Disasm::Handle83h(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    return std::string(s_arithOp[op]) + " " + ModRm16(ip) + ", 0x" + Hex16(*reinterpret_cast<char*>(ip + s_modRmInstLen[modrm] - 1));
}

std::string Disasm::Handle8Fh(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    switch(op)
    {
        case 0:
            return std::string("pop ") + ModRm16(ip);

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return "Invalid Op";
    }

    return "";
}

std::string Disasm::HandleC6h(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    switch(op)
    {
        case 0:
            return std::string("mov ") + ModRm8(ip) + ", " + Imm8(ip + s_modRmInstLen[modrm] - 1);

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return "Invalid Op";
    }

    return "";
}

std::string Disasm::HandleC7h(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    switch(op)
    {
        case 0:
            return std::string("mov ") + ModRm16(ip) + ", " + Imm16(ip + s_modRmInstLen[modrm] - 1);

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return "Invalid Op";
    }

    return "";
}

std::string Disasm::HandleF6h(uint8_t* ip, int& length)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    length = s_modRmInstLen[modrm];

    switch(op)
    {
        case 0:
        case 1:
            length += 1;
            return std::string("test ") + ModRm8(ip) + ", " + Imm8(ip + s_modRmInstLen[modrm] - 1);

        case 2:
            return std::string("not ") + ModRm8(ip);

        case 3:
            return std::string("neg ") + ModRm8(ip);

        case 4:
            return std::string("mul ") + ModRm8(ip);

        case 5:
            return std::string("imul ") + ModRm8(ip);

        case 6:
            return std::string("div ") + ModRm8(ip);

        case 7:
            return std::string("idiv ") + ModRm8(ip);
    }

    return "";
}

std::string Disasm::HandleF7h(uint8_t* ip, int& length)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    length = s_modRmInstLen[modrm];

    switch(op)
    {
        case 0:
        case 1:
            length += 2;
            return std::string("test ") + ModRm16(ip) + ", " + Imm16(ip + s_modRmInstLen[modrm] - 1);

        case 2:
            return std::string("not ") + ModRm16(ip);

        case 3:
            return std::string("neg ") + ModRm16(ip);

        case 4:
            return std::string("mul ") + ModRm16(ip);

        case 5:
            return std::string("imul ") + ModRm16(ip);

        case 6:
            return std::string("div ") + ModRm16(ip);

        case 7:
            return std::string("idiv ") + ModRm16(ip);
    }

    return "";
}

std::string Disasm::HandleFEh(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    switch(op)
    {
        case 0:
            return std::string("inc ") + ModRm8(ip);

        case 1:
            return std::string("dec ") + ModRm8(ip);

        case 7:
            return "Invalid Op";
    }

    return "";
}

std::string Disasm::HandleFFh(uint8_t* ip)
{
    uint8_t modrm = *ip;
    uint8_t mod = modrm >> 6;
    uint8_t op  = (modrm >> 3) & 0x07;

    switch(op)
    {
        case 0:
            return std::string("inc ") + ModRm16(ip);

        case 1:
            return std::string("dec ") + ModRm16(ip);

        case 2:
            return std::string("call ") + ModRm16(ip);

        case 3:
            return std::string("call far ") + ModRm16(ip);

        case 4:
            return std::string("jmp ") + ModRm16(ip);

        case 5:
            return std::string("jmp far ") + ModRm16(ip);

        case 6:
            return std::string("push ") + ModRm16(ip);

        case 7:
            return "Invalid Op";
    }

    return "";
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
        case 0x00: // add r/m8, r8
            instr  = "add " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x01: // add r/m16, r16
            instr  = "add " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x02: // add r8, r/m8
            instr  = "add " + ModRmReg8(ip) + ", "  + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x03: // add r16, r/m16
            instr  = "add " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x04: // add al, imm8
            instr  = "add al, " + Imm8(ip);
            length = 2;
            break;

        case 0x05: // add ax, imm16
            instr  = "add ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x06: // push es
            instr  = "push es";
            length = 1;
            break;

        case 0x07: // pop es
            instr  = "pop es";
            length = 1;
            break;

        case 0x08: // or r/m8, r8
            instr  = "or " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x09: // or r/m16, r16
            instr  = "or " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0a: // or r8, r/m8
            instr  = "or " + ModRmReg8(ip) + ", " + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0b: // or r16, r/m16
            instr  = "or " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x0c: // or al, imm8
            instr  = "or al, " + Imm8(ip);
            length = 2;
            break;

        case 0x0d: // or ax, imm16
            instr  = "or ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x0e: // push cs
            instr  = "push cs";
            length = 1;
            break;

        case 0x10: // adc r/m8, r8
            instr  = "adc " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x11: // adc r/m16, r16
            instr  = "adc " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x12: // adc r8, r/m8
            instr  = "adc " + ModRmReg8(ip) + ", "  + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x13: // adc r16, r/m16
            instr  = "adc " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x14: // adc al, imm8
            instr  = "adc al, " + Imm8(ip);
            length = 2;
            break;

        case 0x15: // adc al, imm16
            instr  = "adc ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x16: // push ss
            instr  = "push ss";
            length = 1;
            break;

        case 0x17: // pop ss
            instr  = "pop ss";
            length = 1;
            break;

        case 0x18: // sbb r/m8, r8
            instr  = "sbb " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x19: // sbb r/m16, r16
            instr  = "sbb " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x1a: // sbb r8, r/m8
            instr  = "sbb " + ModRmReg8(ip) + ", "  + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x1b: // sbb r16, r/m16
            instr  = "sbb " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x1c: // sbb al, imm8
            instr  = "sbb al, " + Imm8(ip);
            length = 2;
            break;

        case 0x1d: // sbb al, imm16
            instr  = "sbb ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x1e: // push ds
            instr  = "push ds";
            length = 1;
            break;

        case 0x1f: // pop ds
            instr  = "pop ds";
            length = 1;
            break;

        case 0x20: // and r/m8, r8
            instr  = "and " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x21: // and r/m16, r16
            instr  = "and " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x22: // and r8, r/m8
            instr  = "and " + ModRmReg8(ip) + ", "  + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x23: // and r16, r/m16
            instr  = "and " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x24: // and al, imm8
            instr  = "and al, " + Imm8(ip);
            length = 2;
            break;

        case 0x25: // and ax, imm16
            instr  = "and ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x26: // prefix - ES override
            instr  = "ES:";
            length = 1;
            break;

        case 0x28: // sub r/m8, r8
            instr  = "sub " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x29: // sub r/m16, r16
            instr  = "sub " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x2a: // sub r8, r/m8
            instr  = "sub " + ModRmReg8(ip) + ", "  + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x2b: // sub r16, r/m16
            instr  = "sub " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x2c: // sub al, imm8
            instr  = "sub al, " + Imm8(ip);
            length = 2;
            break;

        case 0x2d: // sub ax, imm16
            instr  = "sub al, " + Imm16(ip);
            length = 3;
            break;

        case 0x2e: // prefix - CS override
            instr  = "CS:";
            length = 1;
            break;

        case 0x30: // xor r/m8, r8
            instr  = "xor " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x31: // xor r/m16, r16
            instr  = "xor " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x32: // xor r8, r/m8
            instr  = "xor " + ModRmReg8(ip) + ", " + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x33: // xor r16, r/m16
            instr  = "xor " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x34: // xor al, imm8
            instr  = "xor al, " + Imm8(ip);
            length = 2;
            break;

        case 0x35: // xor ax, imm16
            instr  = "xor ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x36: // prefix - SS override
            instr  = "SS:";
            length = 1;
            break;

        case 0x38: // cmp r/m8, r8
            instr  = "cmp " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x39: // cmp r/m16, r16
            instr  = "cmp " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x3a: // cmp r8, r/m8
            instr  = "cmp " + ModRmReg8(ip) + ", " + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x3b: // cmp r16, r/m16
            instr  = "cmp " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x3c: // cmp al, imm8
            instr  = "cmp al, " + Imm8(ip);
            length = 2;
            break;

        case 0x3d: // cmp ax, imm16
            instr  = "cmp ax, " + Imm16(ip);
            length = 3;
            break;

        case 0x3e: // prefix - DS override
            instr  = "DS:";
            length = 1;
            break;

        case 0x40: case 0x41: case 0x42: case 0x43:
        case 0x44: case 0x45: case 0x46: case 0x47:
            instr  = "inc " + Reg16ToStr(opcode - 0x40);
            length = 1;
            break;

        case 0x48: case 0x49: case 0x4a: case 0x4b:
        case 0x4c: case 0x4d: case 0x4e: case 0x4f:
            instr  = "dec " + Reg16ToStr(opcode - 0x48);
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

        case 0x68: // push imm16
            instr = "push " + Imm16(ip);
            length = 3;
            break;

        case 0x6a: // push imm8
            instr = "push " + Imm8(ip);
            length = 2;
            break;

        case 0x70: // jo rel8
            instr = "jo " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x71: // jno rel8
            instr = "jno " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x72: // jb rel8
            instr = "jb " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x73: // jnb rel8
            instr = "jnb " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x74: // je rel8
            instr = "je " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x75: // jne rel8
            instr = "jne " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x76: // jna rel8
            instr = "jna " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x77: // ja rel8
            instr = "ja " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x78: // js rel8
            instr = "js " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x79: // jns rel8
            instr = "jns " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7a: // jp rel8
            instr = "jp " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7b: // jnp rel8
            instr = "jnp " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7c: // jl rel8
            instr = "jl " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7d: // jnl rel8
            instr = "jnl " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7e: // jle rel8
            instr = "jle " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x7f: // jg rel8
            instr = "jg " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0x80:
            instr = Handle80h(ip);
            length = 3;
            break;

        case 0x81:
            instr = Handle81h(ip);
            length = 4;
            break;

        case 0x82:
            instr = Handle80h(ip);
            length = 3;
            break;

        case 0x83:
            instr = Handle83h(ip);
            length = 3;
            break;

        case 0x84: // test r/m8, r8
            instr  = "test " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x85: // test r/m16, r16
            instr  = "test " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x86: // xchg r/m8, r8
            instr  = "xchg " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x87: // xchg r/m16, r16
            instr  = "xchg " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x88: // mov r/m8, r8
            instr  = "mov " + ModRm8(ip) + ", " + ModRmReg8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x89: // mov r/m16, r16
            instr  = "mov " + ModRm16(ip) + ", " + ModRmReg16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8a: // mov r8, r/m8
            instr  = "mov " + ModRmReg8(ip) + ", " + ModRm8(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8b: // mov r16, r/m16
            instr  = "mov " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8c: // mov r/m16, Sreg
            instr = "mov " + ModRm16(ip) + ", " + ModRmSReg(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8d: // lea r16, m16
            instr = "lea " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8e: // mov Sreg, r/m16
            instr = "mov " + ModRmSReg(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x8f:
            instr = Handle8Fh(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
            instr  = "xchg ax, " + Reg16ToStr(opcode - 0x90);
            length = 1;
            break;

        case 0x98: // cbw
            instr = "cbw";
            length = 1;
            break;

        case 0x99: // cwd
            instr = "cwd";
            length = 1;
            break;

        case 0x9a: // call far ptr16:16
            instr  = "call " + Hex16(*reinterpret_cast<uint16_t *>(ip + 2)) + ":" + Hex16(*reinterpret_cast<uint16_t *>(ip));
            length = 5;
            break;

        case 0x9c: // pushf
            instr = "pushf";
            length = 1;
            break;

        case 0x9d: // popf
            instr = "popf";
            length = 1;
            break;

        case 0xa0: // mov al, moffs8
            instr = "mov al, byte ptr [" + Hex16(*reinterpret_cast<uint16_t *>(ip)) + "]";
            length = 3;
            break;

        case 0xa1: // mov ax, moffs16
            instr = "mov ax, word ptr [" + Hex16(*reinterpret_cast<uint16_t *>(ip)) + "]";
            length = 3;
            break;

        case 0xa2: // mov moffs8, al
            instr = "mov byte ptr [" + Hex16(*reinterpret_cast<uint16_t *>(ip)) + "], al";
            length = 3;
            break;

        case 0xa3: // mov moffs16, ax
            instr = "mov word ptr [" + Hex16(*reinterpret_cast<uint16_t *>(ip)) + "], ax";
            length = 3;
            break;

        case 0xa4: // movsb
            instr = "movsb";
            length = 1;
            break;

        case 0xa5: // movsw
            instr = "movsw";
            length = 1;
            break;

        case 0xa6: // cmpsb
            instr = "cmpsb";
            length = 1;
            break;

        case 0xa7: // cmpsw
            instr = "cmpsw";
            length = 1;
            break;

        case 0xa8: // test al, imm8
            instr  = "test al, " + Imm8(ip);
            length = 2;
            break;

        case 0xa9: // test ax, imm16
            instr  = "test ax, " + Imm16(ip);
            length = 3;
            break;

        case 0xaa: // stosb
            instr = "stosb";
            length = 1;
            break;

        case 0xab: // stosw
            instr = "stosw";
            length = 1;
            break;

        case 0xac: // lodsb
            instr = "lodsb";
            length = 1;
            break;

        case 0xad: // lodsw
            instr = "lodsw";
            length = 1;
            break;

        case 0xae: // scasb
            instr = "scasb";
            length = 1;
            break;

        case 0xaf: // scasw
            instr = "scasw";
            length = 1;
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

        case 0xc0: // shift8 imm8
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm8(ip) + ", " + Imm8(ip + s_modRmInstLen[*ip] - 1);
            length = s_modRmInstLen[*ip] + 1;
            break;

        case 0xc1: // shift16 imm8
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm16(ip) + ", " + Imm8(ip + s_modRmInstLen[*ip] - 1);
            length = s_modRmInstLen[*ip] + 1;
            break;

        case 0xc2: // ret imm16
            instr = "ret " + Imm16(ip);
            length = 3;
            break;

        case 0xc3: // ret
            instr = "ret";
            length = 1;
            break;

        case 0xc4: // les r16, m16:m16
            instr = "les " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0xc5: // lds r16, m16:m16
            instr = "lds " + ModRmReg16(ip) + ", " + ModRm16(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0xc6:
            instr = HandleC6h(ip);
            length = s_modRmInstLen[*ip] + 1;
            break;

        case 0xc7:
            instr = HandleC7h(ip);
            length = s_modRmInstLen[*ip] + 2;
            break;

        case 0xc9: // leave
            instr = "leave";
            length = 1;
            break;

        case 0xca: // retf imm16
            instr = "retf " + Imm16(ip);
            length = 3;
            break;

        case 0xcb: // retf
            instr = "retf";
            length = 1;
            break;

        case 0xcd: // int imm8
            instr = "int " + Imm8(ip);
            length = 2;
            // FP hack
            if (*ip > 0x30 && *ip < 0x3f)
            {
                length += 6;
            }
            break;

        case 0xcf: // iret
            instr = "iret";
            length = 1;
            break;

        case 0xd0:
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm8(ip) + ", 1";
            length = s_modRmInstLen[*ip];
            break;

        case 0xd1:
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm16(ip) + ", 1";
            length = s_modRmInstLen[*ip];
            break;

        case 0xd2:
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm8(ip) + ", cl";
            length = s_modRmInstLen[*ip];
            break;

        case 0xd3:
            instr = std::string(s_shiftOp[(*ip >> 3) & 0x07]) + " " + ModRm16(ip) + ", cl";
            length = s_modRmInstLen[*ip];
            break;

        case 0xd4: // aam
            instr = "aam";
            length = 2;
            break;

        // case 0xd5: // aad
        //     break;
        //
        // case 0xd6: // salc
        //     break;
        //
        // case 0xd7: // xlatb
        //     break;
        //
        // case 0xd8: case 0xd9: case 0xda: case 0xdb:
        // case 0xdc: case 0xdd: case 0xde: case 0xdf: // fpu
        //     break;

        case 0xe0: // loopnz rel8
            instr = "loopnz " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0xe1: // loopz rel8
            instr = "loopz " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0xe2: // loop rel8
            instr = "loop " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0xe3: // jcxz rel8
            instr = "jcxz " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0xe4: // in al, imm8
            instr = "in al, " + Imm8(ip);
            length = 2;
            break;

        case 0xe5: // in ax, imm8
            instr = "in ax, " + Imm8(ip);
            length = 2;
            break;

        case 0xe6: // out imm8, al
            instr = "out " + Imm8(ip) + ", al";
            length = 2;
            break;

        case 0xe7: // out imm8, ax
            instr = "out " + Imm8(ip) + ", ax";
            length = 2;
            break;

        case 0xe8: // call rel16
            instr = "call " + Rel16(ip, offset + 3);
            length = 3;
            break;

        case 0xe9: // jmp rel16
            instr = "jmp " + Rel16(ip, offset + 3);
            length = 3;
            break;

        case 0xeb: // jmp rel8
            instr = "jmp " + Rel8(ip, offset + 2);
            length = 2;
            break;

        case 0xec:
            instr = "in al, dx";
            length = 1;
            break;

        case 0xed:
            instr = "in ax, dx";
            length = 1;
            break;

        case 0xee:
            instr = "out dx, al";
            length = 1;
            break;

        case 0xef:
            instr = "out dx, ax";
            length = 1;
            break;

        case 0xf2:
            instr = "repne ";
            switch(*ip)
            {
                case 0xa6: instr += "cmpsb"; break;
                case 0xa7: instr += "cmpsw"; break;
                case 0xae: instr += "scasb"; break;
                case 0xaf: instr += "scasw"; break;
            }
            length = 2;
            break;

        case 0xf3:
            switch(*ip)
            {
                case 0x6c: instr = "rep insb";   break;
                case 0x6d: instr = "rep insw";   break;
                case 0x6e: instr = "rep outsb";  break;
                case 0x6f: instr = "rep outsw";  break;
                case 0xa4: instr = "rep movsb";  break;
                case 0xa5: instr = "rep movsw";  break;
                case 0xa6: instr = "repe cmpsb"; break;
                case 0xa7: instr = "repe cmpsw"; break;
                case 0xaa: instr = "rep stosb";  break;
                case 0xab: instr = "rep stosw";  break;
                case 0xac: instr = "rep lodsb";  break;
                case 0xad: instr = "rep lodsw";  break;
                case 0xae: instr = "repe scasb"; break;
                case 0xaf: instr = "repe scasw"; break;
            }
            length = 2;
            break;

        case 0xf5:
            instr = "cmc";
            length = 1;
            break;

        case 0xf6:
            instr = HandleF6h(ip, length);
            break;

        case 0xf7:
            instr = HandleF7h(ip, length);
            break;

        case 0xf8:
            instr = "clc";
            length = 1;
            break;

        case 0xf9:
            instr = "stc";
            length = 1;
            break;

        case 0xfa:
            instr = "cli";
            length = 1;
            break;

        case 0xfb:
            instr = "sti";
            length = 1;
            break;

        case 0xfc:
            instr = "cld";
            length = 1;
            break;

        case 0xfd:
            instr = "std";
            length = 1;
            break;

        case 0xfe:
            instr = HandleFEh(ip);
            length = s_modRmInstLen[*ip];
            break;

        case 0xff:
            instr = HandleFFh(ip);
            length = s_modRmInstLen[*ip];
            break;

        default:
            instr = "";
            length = 8;
            break;
    }

    return DumpMem(segment, offset, length) + instr;
}

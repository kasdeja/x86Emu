#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <string>
#include "Cpu.h"
#include "Memory.h"
#include "Disasm.h"

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
    : m_memory (memory.GetMem())
    , m_rMemory(memory)
{
    std::fill(m_register, m_register + 16, 0);
    m_register[Register::FLAG] = Flag::IF_mask | Flag::Always1_mask;

    m_result  = 0;
    m_auxbits = 0;
}

Cpu::~Cpu()
{
}

// public methods
void Cpu::SetReg16(Register16 reg, uint16_t value)
{
    switch(reg)
    {
        case Register16::AX: m_register[Register::AX] = value; break;
        case Register16::BX: m_register[Register::BX] = value; break;
        case Register16::CX: m_register[Register::CX] = value; break;
        case Register16::DX: m_register[Register::DX] = value; break;
        case Register16::SI: m_register[Register::SI] = value; break;
        case Register16::DI: m_register[Register::DI] = value; break;
        case Register16::BP: m_register[Register::BP] = value; break;
        case Register16::SP: m_register[Register::SP] = value; break;
        case Register16::CS: m_register[Register::CS] = value; break;
        case Register16::DS: m_register[Register::DS] = value; break;
        case Register16::ES: m_register[Register::ES] = value; break;
        case Register16::SS: m_register[Register::SS] = value; break;
        case Register16::IP: m_register[Register::IP] = value; break;
    }
}

void Cpu::SetReg8(Register8 reg, uint8_t value)
{
    switch(reg)
    {
        case Register8::AL: m_register[Register::AX] &= 0xff00; m_register[Register::AX] |= value; break;
        case Register8::BL: m_register[Register::BX] &= 0xff00; m_register[Register::BX] |= value; break;
        case Register8::CL: m_register[Register::CX] &= 0xff00; m_register[Register::CX] |= value; break;
        case Register8::DL: m_register[Register::DX] &= 0xff00; m_register[Register::DX] |= value; break;

        case Register8::AH: m_register[Register::AX] &= 0x00ff; m_register[Register::AX] |= value << 8; break;
        case Register8::BH: m_register[Register::BX] &= 0x00ff; m_register[Register::BX] |= value << 8; break;
        case Register8::CH: m_register[Register::CX] &= 0x00ff; m_register[Register::CX] |= value << 8; break;
        case Register8::DH: m_register[Register::DX] &= 0x00ff; m_register[Register::DX] |= value << 8; break;
    }
}

uint16_t Cpu::GetReg16(Register16 reg)
{
    switch(reg)
    {
        case Register16::AX: return m_register[Register::AX];
        case Register16::BX: return m_register[Register::BX];
        case Register16::CX: return m_register[Register::CX];
        case Register16::DX: return m_register[Register::DX];
        case Register16::SI: return m_register[Register::SI];
        case Register16::DI: return m_register[Register::DI];
        case Register16::BP: return m_register[Register::BP];
        case Register16::SP: return m_register[Register::SP];
        case Register16::CS: return m_register[Register::CS];
        case Register16::DS: return m_register[Register::DS];
        case Register16::ES: return m_register[Register::ES];
        case Register16::SS: return m_register[Register::SS];
        case Register16::IP: return m_register[Register::IP];
    }

    return 0;
}

uint8_t Cpu::GetReg8(Register8 reg)
{
    switch(reg)
    {
        case Register8::AL: return m_register[Register::AX];
        case Register8::BL: return m_register[Register::BX];
        case Register8::CL: return m_register[Register::CX];
        case Register8::DL: return m_register[Register::DX];

        case Register8::AH: return m_register[Register::AX] >> 8;
        case Register8::BH: return m_register[Register::BX] >> 8;
        case Register8::CH: return m_register[Register::CX] >> 8;
        case Register8::DH: return m_register[Register::DX] >> 8;
    }

    return 0;
}

void Cpu::SetFlag(CpuInterface::Flag flag, bool value)
{
    switch(flag)
    {
        case CpuInterface::CF: SetCF(value); break;
        case CpuInterface::PF: SetPF(value); break;
        case CpuInterface::AF: SetAF(value); break;
        case CpuInterface::ZF: SetZF(value); break;
        case CpuInterface::SF: SetSF(value); break;
        case CpuInterface::TF: m_register[Register::FLAG] &= Flag::TF_mask; m_register[Register::FLAG] |= value << Flag::TF_bit; break;
        case CpuInterface::IF: m_register[Register::FLAG] &= Flag::IF_mask; m_register[Register::FLAG] |= value << Flag::IF_bit; break;
        case CpuInterface::DF: m_register[Register::FLAG] &= Flag::DF_mask; m_register[Register::FLAG] |= value << Flag::DF_bit; break;
        case CpuInterface::OF: SetOF(value); break;
        case CpuInterface::NT: m_register[Register::FLAG] &= Flag::NT_mask; m_register[Register::FLAG] |= value << Flag::NT_bit; break;
    }
}

bool Cpu::GetFlag(CpuInterface::Flag flag)
{
    switch(flag)
    {
        case CpuInterface::CF: return GetCF();
        case CpuInterface::PF: return GetPF();
        case CpuInterface::AF: return GetAF();
        case CpuInterface::ZF: return GetZF();
        case CpuInterface::SF: return GetSF();
        case CpuInterface::TF: return (m_register[Register::FLAG] >> Flag::TF_bit) & 1;
        case CpuInterface::IF: return (m_register[Register::FLAG] >> Flag::IF_bit) & 1;
        case CpuInterface::DF: return (m_register[Register::FLAG] >> Flag::DF_bit) & 1;
        case CpuInterface::OF: return GetOF();
        case CpuInterface::NT: return (m_register[Register::FLAG] >> Flag::NT_bit) & 1;
    }

    return false;
}

Memory& Cpu::GetMem()
{
    return m_rMemory;
}

void Cpu::Run(int nCycles)
{
    m_state            = 0;
    m_segmentBase      = m_register[Register::DS] * 16;
    m_stackSegmentBase = m_register[Register::SS] * 16;

    for(int n = 0; n < nCycles; n++)
    {
        ExecuteInstruction();

        if (m_state & State::InvalidOp)
            break;
    }
}

// private methods
inline uint16_t* Cpu::Reg16(uint8_t modrm)
{
    return &m_register[(modrm >> 3) & 0x07];
}

inline uint8_t* Cpu::Reg8(uint8_t modrm)
{
    return reinterpret_cast<uint8_t *>(&m_register[(modrm >> 3) & 0x03]) + ((modrm >> 5) & 0x01);
}

inline uint16_t* Cpu::SReg(uint8_t modrm)
{
    return &m_register[Register::ES + ((modrm >> 3) & 0x07)];
}

inline uint16_t Cpu::Disp16(uint8_t* ip)
{
    return *reinterpret_cast<uint16_t *>(ip);
}

inline int8_t Cpu::Disp8(uint8_t* ip)
{
    return *reinterpret_cast<int8_t *>(ip);
}

inline uint16_t Cpu::Imm16(uint8_t* ip)
{
    return *reinterpret_cast<uint16_t *>(ip);
}

inline uint32_t Cpu::Load32(std::size_t linearAddr)
{
    printf("load %08lx val 0x%08x\n",
        linearAddr, *reinterpret_cast<uint32_t *>(m_memory + linearAddr));

    return *reinterpret_cast<uint32_t *>(m_memory + linearAddr);
}

inline uint16_t Cpu::Load16(std::size_t linearAddr)
{
    printf("load %08lx val 0x%04x\n",
        linearAddr, *reinterpret_cast<uint16_t *>(m_memory + linearAddr));

    return *reinterpret_cast<uint16_t *>(m_memory + linearAddr);
}

inline uint8_t Cpu::Load8(std::size_t linearAddr)
{
    printf("load %08lx val 0x%02x\n",
        linearAddr, *reinterpret_cast<uint8_t *>(m_memory + linearAddr));

    return *reinterpret_cast<uint8_t *>(m_memory + linearAddr);
}

inline void Cpu::Store16(std::size_t linearAddr, uint16_t value)
{
    printf("store %08lx val 0x%04x (was 0x%04x)\n",
        linearAddr, value, *reinterpret_cast<uint16_t *>(m_memory + linearAddr));

    *reinterpret_cast<uint16_t *>(m_memory + linearAddr) = value;
}

inline void Cpu::Store8(std::size_t linearAddr, uint8_t value)
{
    printf("store %08lx val 0x%02x (was 0x%02x)\n",
        linearAddr, value, *reinterpret_cast<uint8_t *>(m_memory + linearAddr));

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

inline bool Cpu::GetCF()
{
    return static_cast<uint32_t>(m_auxbits) >> Aux::CF_bit;
}

inline bool Cpu::GetPF()
{
    int tmp;

    tmp = m_result & 0xff;
    tmp ^= (m_auxbits >> Aux::PDB_bit) & 0xff;
    tmp = (tmp & (tmp >> 4)) & 0x0f;

    return (0x9669 >> tmp) & 1;
}

inline bool Cpu::GetAF()
{
    return (m_auxbits >> Aux::AF_bit) & 1;
}

inline bool Cpu::GetZF()
{
    return m_result == 0;
}

inline bool Cpu::GetSF()
{
    return (static_cast<uint32_t>(m_result) >> 31) ^ (m_auxbits & Aux::SFD_mask);
}

inline bool Cpu::GetOF()
{
    return ((m_auxbits + (1 << Aux::PO_bit)) >> Aux::CF_bit) & 1;
}

inline void Cpu::SetOF_CF(bool of, bool cf)
{
    int po = of ^ cf;

    m_auxbits &= ~(Aux::PO_mask | Aux::CF_mask);
    m_auxbits |= (cf << Aux::CF_bit) | (po << Aux::PO_bit);
}

inline void Cpu::SetCF(bool val)
{
    SetOF_CF(GetOF(), val);
}

inline void Cpu::SetPF(bool val)
{
    int pdb = (m_result & 0xff) ^ (!val);

    m_auxbits &= ~Aux::PDB_mask;
    m_auxbits |= pdb << Aux::PDB_bit;
}

inline void Cpu::SetAF(bool val)
{
    m_auxbits &= ~Aux::AF_mask;
    m_auxbits |= val << Aux::AF_bit;
}

inline void Cpu::SetZF(bool val)
{
    if (val)
    {
        m_auxbits ^= (m_result >> 31) & 1;
        m_auxbits ^= (m_result & 0xff) << Aux::PDB_bit;
        m_result = 0;
    }
    else
    {
        m_result |= 1 << 8;
    }
}

inline void Cpu::SetSF(bool val)
{
    m_auxbits ^= GetSF() ^ val;
}

inline void Cpu::SetOF(bool val)
{
    SetOF_CF(val, GetCF());
}

inline void Cpu::SetSubFlags16(uint16_t op1, uint16_t op2, uint16_t result)
{
    uint16_t carries = ((~op1) & op2) | (((~op1) ^ op2) & result);

    m_result  = static_cast<short>(result);
    m_auxbits = (carries << 16) | (carries & Aux::AF_mask);
}

inline void Cpu::SetSubFlags8(uint8_t op1, uint8_t op2, uint8_t result)
{
    uint8_t carries = ((~op1) & op2) | (((~op1) ^ op2) & result);

    m_result  = static_cast<char>(result);
    m_auxbits = (carries << 24) | (carries & Aux::AF_mask);
}

inline void Cpu::SetAddFlags16(uint16_t op1, uint16_t op2, uint16_t result)
{
    uint16_t carries = (op1 & op2) | ((op1 | op2) & (~result));

    m_result  = static_cast<short>(result);
    m_auxbits = (carries << 16) | (carries & Aux::AF_mask);
}

inline void Cpu::SetAddFlags8(uint8_t op1, uint8_t op2, uint8_t result)
{
    uint8_t carries = (op1 & op2) | ((op1 | op2) & (~result));

    m_result  = static_cast<char>(result);
    m_auxbits = (carries << 24) | (carries & Aux::AF_mask);
}

inline void Cpu::SetLogicFlags16(uint16_t result)
{
    m_result  = static_cast<short>(result);
    m_auxbits = 0;
}

inline void Cpu::SetLogicFlags8(uint8_t result)
{
    m_result  = static_cast<char>(result);
    m_auxbits = 0;
}

void Cpu::RecalcFlags()
{
    m_register[Register::FLAG] &=
        ~(Flag::CF_mask | Flag::PF_mask | Flag::AF_mask | Flag::ZF_mask | Flag::SF_mask | Flag::OF_mask);

    m_register[Register::FLAG] |=
        (GetCF() << Flag::CF_bit) |
        (GetPF() << Flag::PF_bit) |
        (GetAF() << Flag::AF_bit) |
        (GetZF() << Flag::ZF_bit) |
        (GetSF() << Flag::SF_bit) |
        (GetOF() << Flag::OF_bit);
}

void Cpu::RestoreLazyFlags()
{
    bool cf = (m_register[Register::FLAG] >> Flag::CF_bit) & 1;
    bool pf = (m_register[Register::FLAG] >> Flag::PF_bit) & 1;
    bool af = (m_register[Register::FLAG] >> Flag::AF_bit) & 1;
    bool zf = (m_register[Register::FLAG] >> Flag::ZF_bit) & 1;
    bool sf = (m_register[Register::FLAG] >> Flag::SF_bit) & 1;
    bool of = (m_register[Register::FLAG] >> Flag::OF_bit) & 1;

    bool po = of ^ cf;

    m_result = (!zf) << 8;
    m_auxbits =
        (cf << Aux::CF_bit) |
        (po << Aux::PO_bit) |
        ((!pf) << Aux::PDB_bit) |
        (af << Aux::AF_bit) |
        sf;
}

inline uint32_t Cpu::ModRmLoad32(uint8_t *ip)
{
    uint8_t modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI]);
        ModRm_Case(0x01): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI]);
        ModRm_Case(0x02): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI]);
        ModRm_Case(0x03): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI]);
        ModRm_Case(0x04): return Load32(m_segmentBase      + m_register[Register::SI]);
        ModRm_Case(0x05): return Load32(m_segmentBase      + m_register[Register::DI]);
        ModRm_Case(0x06): return Load32(m_segmentBase      + Disp16(ip));
        ModRm_Case(0x07): return Load32(m_segmentBase      + m_register[Register::BX]);
        // mod 01
        ModRm_Case(0x40): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x41): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x42): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip));
        ModRm_Case(0x43): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip));
        ModRm_Case(0x44): return Load32(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip));
        ModRm_Case(0x45): return Load32(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip));
        ModRm_Case(0x46): return Load32(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip));
        ModRm_Case(0x47): return Load32(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip));
        // mod 10
        ModRm_Case(0x80): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x81): return Load32(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x82): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip));
        ModRm_Case(0x83): return Load32(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip));
        ModRm_Case(0x84): return Load32(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip));
        ModRm_Case(0x85): return Load32(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip));
        ModRm_Case(0x86): return Load32(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip));
        ModRm_Case(0x87): return Load32(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip));
        // mod 11
        ModRm_Case(0xc0):
        ModRm_Case(0xc1):
        ModRm_Case(0xc2):
        ModRm_Case(0xc3):
        ModRm_Case(0xc4):
        ModRm_Case(0xc5):
        ModRm_Case(0xc6):
        ModRm_Case(0xc7): m_state |= State::InvalidOp; break;
    }

    return 0;
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
        ModRm_Case(0xc4): return m_register[0] >> 8;
        ModRm_Case(0xc5): return m_register[1] >> 8;
        ModRm_Case(0xc6): return m_register[2] >> 8;
        ModRm_Case(0xc7): return m_register[3] >> 8;
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
        ModRm_Case(0xc0): *reinterpret_cast<uint8_t *>(&m_register[0]) = value;       break;
        ModRm_Case(0xc1): *reinterpret_cast<uint8_t *>(&m_register[1]) = value;       break;
        ModRm_Case(0xc2): *reinterpret_cast<uint8_t *>(&m_register[2]) = value;       break;
        ModRm_Case(0xc3): *reinterpret_cast<uint8_t *>(&m_register[3]) = value;       break;
        ModRm_Case(0xc4): *(reinterpret_cast<uint8_t *>(&m_register[0]) + 1) = value; break;
        ModRm_Case(0xc5): *(reinterpret_cast<uint8_t *>(&m_register[1]) + 1) = value; break;
        ModRm_Case(0xc6): *(reinterpret_cast<uint8_t *>(&m_register[2]) + 1) = value; break;
        ModRm_Case(0xc7): *(reinterpret_cast<uint8_t *>(&m_register[3]) + 1) = value; break;
    }
}

template<typename F>
inline void Cpu::ModRmLoadOp16(uint8_t *ip, F&& f)  // op r16, r/m16
{
    std::size_t ea;
    uint16_t    op1, op2, opResult;
    uint8_t     modRm = *ip++;
    uint16_t    *reg;

    reg      = Reg16(modRm);
    op1      = *reg;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x01): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x02): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x03): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x04): op2 = Load16(m_segmentBase      + m_register[Register::SI]);                                         opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x05): op2 = Load16(m_segmentBase      + m_register[Register::DI]);                                         opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x06): op2 = Load16(m_segmentBase      + Disp16(ip));                                                       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x07): op2 = Load16(m_segmentBase      + m_register[Register::BX]);                                         opResult = f(op1, op2); *reg = opResult; break;
        // mod 01
        ModRm_Case(0x40): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x41): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x42): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x43): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x44): op2 = Load16(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x45): op2 = Load16(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x46): op2 = Load16(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x47): op2 = Load16(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        // mod 10
        ModRm_Case(0x80): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x81): op2 = Load16(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x82): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x83): op2 = Load16(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x84): op2 = Load16(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x85): op2 = Load16(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x86): op2 = Load16(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x87): op2 = Load16(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        // mod 11
        ModRm_Case(0xc0): op2 = m_register[0]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc1): op2 = m_register[1]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc2): op2 = m_register[2]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc3): op2 = m_register[3]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc4): op2 = m_register[4]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc5): op2 = m_register[5]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc6): op2 = m_register[6]; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc7): op2 = m_register[7]; opResult = f(op1, op2); *reg = opResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmLoadOp8(uint8_t *ip, F&& f)   // op r8, r/m8
{
    std::size_t ea;
    uint8_t     op1, op2, opResult;
    uint8_t     modRm = *ip++;
    uint8_t     *reg;

    reg = Reg8(modRm);
    op1 = *reg;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x01): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x02): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x03): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI]);              opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x04): op2 = Load8(m_segmentBase      + m_register[Register::SI]);                                         opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x05): op2 = Load8(m_segmentBase      + m_register[Register::DI]);                                         opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x06): op2 = Load8(m_segmentBase      + Disp16(ip));                                                       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x07): op2 = Load8(m_segmentBase      + m_register[Register::BX]);                                         opResult = f(op1, op2); *reg = opResult; break;
        // mod 01
        ModRm_Case(0x40): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x41): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x42): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x43): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x44): op2 = Load8(m_segmentBase      + m_register[Register::SI]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x45): op2 = Load8(m_segmentBase      + m_register[Register::DI]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x46): op2 = Load8(m_segmentBase      + m_register[Register::BP]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x47): op2 = Load8(m_segmentBase      + m_register[Register::BX]                            + Disp8(ip));  opResult = f(op1, op2); *reg = opResult; break;
        // mod 10
        ModRm_Case(0x80): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x81): op2 = Load8(m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x82): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x83): op2 = Load8(m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x84): op2 = Load8(m_segmentBase      + m_register[Register::SI]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x85): op2 = Load8(m_segmentBase      + m_register[Register::DI]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x86): op2 = Load8(m_segmentBase      + m_register[Register::BP]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0x87): op2 = Load8(m_segmentBase      + m_register[Register::BX]                            + Disp16(ip)); opResult = f(op1, op2); *reg = opResult; break;
        // mod 11
        ModRm_Case(0xc0): op2 = *reinterpret_cast<uint8_t *>(&m_register[0]);       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc1): op2 = *reinterpret_cast<uint8_t *>(&m_register[1]);       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc2): op2 = *reinterpret_cast<uint8_t *>(&m_register[2]);       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc3): op2 = *reinterpret_cast<uint8_t *>(&m_register[3]);       opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc4): op2 = *(reinterpret_cast<uint8_t *>(&m_register[0]) + 1); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc5): op2 = *(reinterpret_cast<uint8_t *>(&m_register[1]) + 1); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc6): op2 = *(reinterpret_cast<uint8_t *>(&m_register[2]) + 1); opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc7): op2 = *(reinterpret_cast<uint8_t *>(&m_register[3]) + 1); opResult = f(op1, op2); *reg = opResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOp16(uint8_t *ip, F&& f)    // op r/m16, r16
{
    std::size_t ea;
    uint16_t    op1, op2, opResult;
    uint8_t     modRm = *ip++;

    op2 = *Reg16(modRm);

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); op1 = Load16(ea); opResult = f(op1, op2); Store16(ea, opResult); break;
        // mod 11
        ModRm_Case(0xc0): op1 = m_register[0]; opResult = f(op1, op2); m_register[0] = opResult; break;
        ModRm_Case(0xc1): op1 = m_register[1]; opResult = f(op1, op2); m_register[1] = opResult; break;
        ModRm_Case(0xc2): op1 = m_register[2]; opResult = f(op1, op2); m_register[2] = opResult; break;
        ModRm_Case(0xc3): op1 = m_register[3]; opResult = f(op1, op2); m_register[3] = opResult; break;
        ModRm_Case(0xc4): op1 = m_register[4]; opResult = f(op1, op2); m_register[4] = opResult; break;
        ModRm_Case(0xc5): op1 = m_register[5]; opResult = f(op1, op2); m_register[5] = opResult; break;
        ModRm_Case(0xc6): op1 = m_register[6]; opResult = f(op1, op2); m_register[6] = opResult; break;
        ModRm_Case(0xc7): op1 = m_register[7]; opResult = f(op1, op2); m_register[7] = opResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOp8(uint8_t *ip, F&& f)     // op r/m8, r8
{
    std::size_t ea;
    uint8_t     op1, op2, opResult;
    uint8_t     modRm = *ip++;
    uint8_t     *reg;

    op2 = *Reg8(modRm);

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); op1 = Load8(ea); opResult = f(op1, op2); Store8(ea, opResult); break;
        // mod 11
        ModRm_Case(0xc0): reg = reinterpret_cast<uint8_t *>(&m_register[0]);     op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc1): reg = reinterpret_cast<uint8_t *>(&m_register[1]);     op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc2): reg = reinterpret_cast<uint8_t *>(&m_register[2]);     op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc3): reg = reinterpret_cast<uint8_t *>(&m_register[3]);     op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc4): reg = reinterpret_cast<uint8_t *>(&m_register[0]) + 1; op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc5): reg = reinterpret_cast<uint8_t *>(&m_register[1]) + 1; op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc6): reg = reinterpret_cast<uint8_t *>(&m_register[2]) + 1; op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
        ModRm_Case(0xc7): reg = reinterpret_cast<uint8_t *>(&m_register[3]) + 1; op1 = *reg; opResult = f(op1, op2); *reg = opResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOpNoReg16(uint8_t *ip, F&& f)    // op r/m16
{
    std::size_t ea;
    uint16_t    op, opResult;
    uint8_t     modRm = *ip++;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); op = Load16(ea); opResult = f(op); Store16(ea, opResult); break;
        // mod 11
        ModRm_Case(0xc0): op = m_register[0]; opResult = f(op); m_register[0] = opResult; break;
        ModRm_Case(0xc1): op = m_register[1]; opResult = f(op); m_register[1] = opResult; break;
        ModRm_Case(0xc2): op = m_register[2]; opResult = f(op); m_register[2] = opResult; break;
        ModRm_Case(0xc3): op = m_register[3]; opResult = f(op); m_register[3] = opResult; break;
        ModRm_Case(0xc4): op = m_register[4]; opResult = f(op); m_register[4] = opResult; break;
        ModRm_Case(0xc5): op = m_register[5]; opResult = f(op); m_register[5] = opResult; break;
        ModRm_Case(0xc6): op = m_register[6]; opResult = f(op); m_register[6] = opResult; break;
        ModRm_Case(0xc7): op = m_register[7]; opResult = f(op); m_register[7] = opResult; break;
    }
}

template<typename F>
inline void Cpu::ModRmModifyOpNoReg8(uint8_t *ip, F&& f)    // op r/m8
{
    std::size_t ea;
    uint8_t     op, opResult;
    uint8_t     modRm = *ip++;
    uint8_t     *reg;

    switch(modRm)
    {
        // mod 00
        ModRm_Case(0x00): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI];              op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x01): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI];              op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x02): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI];              op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x03): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI];              op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x04): ea = m_segmentBase      + m_register[Register::SI];                                         op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x05): ea = m_segmentBase      + m_register[Register::DI];                                         op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x06): ea = m_segmentBase      + Disp16(ip);                                                       op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x07): ea = m_segmentBase      + m_register[Register::BX];                                         op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        // mod 01
        ModRm_Case(0x40): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x41): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x42): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x43): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x44): ea = m_segmentBase      + m_register[Register::SI]                            + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x45): ea = m_segmentBase      + m_register[Register::DI]                            + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x46): ea = m_segmentBase      + m_register[Register::BP]                            + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x47): ea = m_segmentBase      + m_register[Register::BX]                            + Disp8(ip);  op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        // mod 10
        ModRm_Case(0x80): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::SI] + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x81): ea = m_segmentBase      + m_register[Register::BX] + m_register[Register::DI] + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x82): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::SI] + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x83): ea = m_stackSegmentBase + m_register[Register::BP] + m_register[Register::DI] + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x84): ea = m_segmentBase      + m_register[Register::SI]                            + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x85): ea = m_segmentBase      + m_register[Register::DI]                            + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x86): ea = m_segmentBase      + m_register[Register::BP]                            + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        ModRm_Case(0x87): ea = m_segmentBase      + m_register[Register::BX]                            + Disp16(ip); op = Load8(ea); opResult = f(op); Store8(ea, opResult); break;
        // mod 11
        ModRm_Case(0xc0): reg = reinterpret_cast<uint8_t *>(&m_register[0]);     op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc1): reg = reinterpret_cast<uint8_t *>(&m_register[1]);     op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc2): reg = reinterpret_cast<uint8_t *>(&m_register[2]);     op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc3): reg = reinterpret_cast<uint8_t *>(&m_register[3]);     op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc4): reg = reinterpret_cast<uint8_t *>(&m_register[0]) + 1; op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc5): reg = reinterpret_cast<uint8_t *>(&m_register[1]) + 1; op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc6): reg = reinterpret_cast<uint8_t *>(&m_register[2]) + 1; op = *reg; opResult = f(op); *reg = opResult; break;
        ModRm_Case(0xc7): reg = reinterpret_cast<uint8_t *>(&m_register[3]) + 1; op = *reg; opResult = f(op); *reg = opResult; break;
    }
}

void Cpu::HandleREPNE(uint8_t opcode)
{
    if (m_register[Register::CX] == 0)
        return;

    std::size_t dsBase = m_register[Register::DS] * 16;
    std::size_t esBase = m_register[Register::ES] * 16;
    short       delta  = (m_register[Register::FLAG] & Flag::DF_mask) ? -1 : 1;

    if (opcode == 0xa6) // repne cmpsb
    {
        uint8_t op1, op2, diff;

        while(m_register[Register::CX] > 0)
        {
            op1  = Load8(dsBase + m_register[Register::SI]);
            op2  = Load8(esBase + m_register[Register::DI]);
            diff = op1 - op2;

            m_register[Register::SI] += delta;
            m_register[Register::DI] += delta;
            m_register[Register::CX]--;

            if (diff == 0)
                break;
        }

        SetSubFlags8(op1, op2, diff);
    }
    else if (opcode == 0xa7) // repne cmpsw
    {
        uint16_t op1, op2, diff;

        delta <<= 1;

        while(m_register[Register::CX] > 0)
        {
            op1  = Load8(dsBase + m_register[Register::SI]);
            op2  = Load8(esBase + m_register[Register::DI]);
            diff = op1 - op2;

            m_register[Register::SI] += delta;
            m_register[Register::DI] += delta;
            m_register[Register::CX]--;

            if (diff == 0)
                break;
        }

        SetSubFlags16(op1, op2, diff);
    }
    else if (opcode == 0xae) // repne scasb
    {
        uint8_t op1 = m_register[Register::AX] & 0xff;
        uint8_t op2, diff;

        while(m_register[Register::CX] > 0)
        {
            op2  = Load8(esBase + m_register[Register::DI]);
            diff = op1 - op2;

            m_register[Register::DI] += delta;
            m_register[Register::CX]--;

            if (diff == 0)
                break;
        }

        SetSubFlags8(op1, op2, diff);
    }
    else if (opcode == 0xaf) // repne scasw
    {
        uint16_t op1 = m_register[Register::AX];
        uint16_t op2, diff;

        delta <<= 1;

        while(m_register[Register::CX] > 0)
        {
            op2  = Load16(esBase + m_register[Register::DI]);
            diff = op1 - op2;

            m_register[Register::DI] += delta;
            m_register[Register::CX]--;

            if (diff == 0)
                break;
        }

        SetSubFlags16(op1, op2, diff);
    }
}

void Cpu::HandleREP(uint8_t opcode)
{
    if (m_register[Register::CX] == 0)
        return;

    std::size_t dsBase = m_register[Register::DS] * 16;
    std::size_t esBase = m_register[Register::ES] * 16;
    short       delta  = (m_register[Register::FLAG] & Flag::DF_mask) ? -1 : 1;

    if (opcode == 0xa4) // rep movsb
    {
        while(m_register[Register::CX] > 0)
        {
            Store8(esBase + m_register[Register::DI], Load8(dsBase + m_register[Register::SI]));

            m_register[Register::SI] += delta;
            m_register[Register::DI] += delta;
            m_register[Register::CX]--;
        }
    }
    else if (opcode == 0xa5) // rep movsw
    {
        delta <<= 1;

        while(m_register[Register::CX] > 0)
        {
            Store16(esBase + m_register[Register::DI], Load16(dsBase + m_register[Register::SI]));

            m_register[Register::SI] += delta;
            m_register[Register::DI] += delta;
            m_register[Register::CX]--;
        }
    }
    else if (opcode == 0xaa) // rep stosb
    {
        uint8_t byte = m_register[Register::AX];

        while(m_register[Register::CX] > 0)
        {
            Store8(esBase + m_register[Register::DI], byte);

            m_register[Register::DI] += delta;
            m_register[Register::CX]--;
        }
    }
    else if (opcode == 0xab) // rep stosw
    {
        uint16_t word = m_register[Register::AX];

        delta <<= 1;

        while(m_register[Register::CX] > 0)
        {
            Store16(esBase + m_register[Register::DI], word);

            m_register[Register::DI] += delta;
            m_register[Register::CX]--;
        }
    }
    else
    {
        // 6c rep insb
        // 6d rep insw
        // 6e rep outsb
        // 6f rep outsw
        // ac rep lodsb
        // ad rep lodsw
        m_state |= State::InvalidOp;
    }
}

void Cpu::Handle8xCommon(uint8_t* ip, uint16_t op2)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // add r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 + op2;
                    SetAddFlags16(op1, op2, result);
                    return result;
                });
            break;

        case 1: // or r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 | op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 2: // adc r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 + op2 + static_cast<uint16_t>(GetCF());
                    SetAddFlags16(op1, op2, result);
                    return result;
                });
            break;

        case 3: // sbb r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 - op2 - static_cast<uint16_t>(GetCF());
                    SetSubFlags16(op1, op2, result);
                    return result;
                });
            break;

        case 4: // and r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 & op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 5: // sub r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 - op2;
                    SetSubFlags16(op1, op2, result);
                    return result;
                });
            break;

        case 6: // xor r/m16, imm16
            ModRmModifyOpNoReg16(ip,
                [this, op2](uint16_t op1)
                {
                    uint16_t result = op1 ^ op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 7: // cmp r/m16, imm16
            {
                uint16_t op1  = ModRmLoad16(ip);
                uint16_t diff = op1 - op2;

                SetSubFlags16(op1, op2, diff);
            }
            break;
    }
}

void Cpu::Handle80h(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t op2    = *(ip + s_modRmInstLen[modrm] - 1);
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // add r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 + op2;
                    SetAddFlags8(op1, op2, result);
                    return result;
                });
            break;

        case 1: // or r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 | op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 2: // adc r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 + op2 + static_cast<uint8_t>(GetCF());
                    SetAddFlags8(op1, op2, result);
                    return result;
                });
            break;

        case 3: // sbb r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 - op2 - static_cast<uint8_t>(GetCF());
                    SetSubFlags8(op1, op2, result);
                    return result;
                });
            break;

        case 4: // and r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 & op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 5: // sub r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 - op2;
                    SetSubFlags8(op1, op2, result);
                    return result;
                });
            break;

        case 6: // xor r/m8, imm8
            ModRmModifyOpNoReg8(ip,
                [this, op2](uint8_t op1)
                {
                    uint8_t result = op1 ^ op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;
                });
            break;

        case 7: // cmp r/m8, imm8
            {
                uint8_t op1  = ModRmLoad8(ip);
                uint8_t diff = op1 - op2;

                SetSubFlags8(op1, op2, diff);
            }
            break;
    }
}

void Cpu::Handle81h(uint8_t* ip)
{
    Handle8xCommon(ip, *reinterpret_cast<uint16_t *>(ip + s_modRmInstLen[*ip] - 1));
}

void Cpu::Handle83h(uint8_t* ip)
{
    Handle8xCommon(ip, static_cast<short>(*(ip + s_modRmInstLen[*ip] - 1)));
}

void Cpu::Handle8Fh(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    if (opcode == 0) // pop r/m16
    {
        ModRmStore16(ip, Pop16());
    }
    else
    {
        m_state |= State::InvalidOp;
    }
}

void Cpu::HandleC6h(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    if (opcode == 0) // mov r/m8, imm8
    {
        uint8_t value = *(ip + s_modRmInstLen[modrm] - 1);

        ModRmStore8(ip, value);
    }
    else
    {
        m_state |= State::InvalidOp;
    }
}

void Cpu::HandleC7h(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    if (opcode == 0) // mov r/m16, imm16
    {
        uint16_t value = *reinterpret_cast<uint16_t *>(ip + s_modRmInstLen[*ip] - 1);

        ModRmStore16(ip, value);
    }
    else
    {
        m_state |= State::InvalidOp;
    }
}

void Cpu::HandleF6h(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // test r/m8, imm8
        case 1:
            {
                uint8_t result = ModRmLoad8(ip) & *(ip + 1);

                SetLogicFlags8(result);
                m_register[Register::IP] += 1;
            }
            break;

        case 2: // not r/m8
            ModRmModifyOpNoReg8(ip,
                [this](uint8_t op)
                {
                    return ~op;
                });
            break;

        case 3: // neg r/m8
            ModRmModifyOpNoReg8(ip,
                [this](uint8_t op)
                {
                    uint8_t result = -op;
                    SetSubFlags8(0, op, result);
                    return result;
                });
            break;

        case 4: // mul r/m8
        case 5: // imul r/m8
        case 6: // div r/m8
        case 7: // idiv r/m8
            m_state |= State::InvalidOp;
            break;
    }
}

void Cpu::HandleF7h(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // test r/m16, imm16
        case 1:
            {
                uint16_t result = ModRmLoad16(ip) & Imm16(ip + 1);

                SetLogicFlags16(result);
                m_register[Register::IP] += 2;
            }
            break;

        case 2: // not r/m16
            ModRmModifyOpNoReg16(ip,
                [this](uint16_t op)
                {
                    return ~op;
                });
            break;

        case 3: // neg r/m16
            ModRmModifyOpNoReg16(ip,
                [this](uint16_t op)
                {
                    uint16_t result = -op;
                    SetSubFlags16(0, op, result);
                    return result;
                });
            break;

        case 4: // mul r/m16
        case 5: // imul r/m16
        case 6: // div r/m16
        case 7: // idiv r/m16
            m_state |= State::InvalidOp;
            break;
    }
}

void Cpu::HandleFFh(uint8_t* ip)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // inc r/m16
            ModRmModifyOpNoReg16(ip,
                [this](uint16_t op)
                {
                    return op + 1;
                });
            break;

        case 1: // dec r/m16
            ModRmModifyOpNoReg16(ip,
                [this](uint16_t op)
                {
                    return op - 1;
                });
            break;

        case 2: // call r/m16
            Push16(m_register[Register::IP]);
            m_register[Register::IP] = ModRmLoad16(ip);
            break;

        case 3: // call far r/m16
            {
                uint32_t segmentOffset = ModRmLoad32(ip);

                Push16(m_register[Register::CS]);
                Push16(m_register[Register::IP]);

                m_register[Register::CS] = segmentOffset >> 16;
                m_register[Register::IP] = segmentOffset &  0xffff;
            }
            break;

        case 4: // jmp r/m16
            m_register[Register::IP] = Load16(m_segmentBase + ModRmLoad16(ip));
            break;

        case 5: // jmp far r/m16
            {
                uint16_t offset = ModRmLoad16(ip);

                m_register[Register::CS] = Load16(m_segmentBase + offset + 2);
                m_register[Register::IP] = Load16(m_segmentBase + offset);
            }
            break;

        case 6: // push r/m16
            Push16(ModRmLoad16(ip));
            break;

        case 7:
            m_state |= State::InvalidOp;
            break;
    }
}

void Cpu::HandleShift16(uint8_t* ip, uint8_t shift)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // rol r/m16, x
        case 1: // ror r/m16, x
        case 2: // rcl r/m16, x
        case 3: // rcr r/m16, x
            m_state |= State::InvalidOp;
            break;

        case 4: // shl r/m16, x
            ModRmModifyOpNoReg16(ip,
                [this, shift](uint16_t op)
                {
                    if (shift == 0)
                        return op;

                    uint16_t result;
                    bool of, cf;

                    if (shift <= 16)
                    {
                        result = op << shift;
                        cf     = (result >> (16 - shift)) & 1;
                        of     = cf & (op >> 15);
                    }
                    else
                    {
                        result = 0;
                        cf     = 0;
                        of     = 0;
                    }

                    SetLogicFlags16(result);
                    SetOF_CF(of, cf);

                    return result;
                });
            break;

        case 5: // shr r/m16, x
            ModRmModifyOpNoReg16(ip,
                [this, shift](uint16_t op)
                {
                    if (shift == 0)
                        return op;

                    uint16_t result;
                    bool of, cf;

                    if (shift <= 16)
                    {
                        result = op >> shift;
                        cf     = (result >> (shift - 1)) & 1;
                        of     = (((result << 1) ^ result) >> 15) & 1;
                    }
                    else
                    {
                        result = 0;
                        cf     = 0;
                        of     = 0;
                    }

                    SetLogicFlags16(result);
                    SetOF_CF(of, cf);

                    return result;
                });
            break;

        case 6: // sal r/m16, x
        case 7: // sar r/m16, x
            m_state |= State::InvalidOp;
            break;
    }
}

void Cpu::HandleShift8(uint8_t* ip, uint8_t shift)
{
    uint8_t modrm  = *ip;
    uint8_t opcode = (modrm >> 3) & 0x07;

    switch(opcode)
    {
        case 0: // rol r/m8, x
        case 1: // ror r/m8, x
        case 2: // rcl r/m8, x
        case 3: // rcr r/m8, x
            m_state |= State::InvalidOp;
            break;

        case 4: // shl r/m8, x
            ModRmModifyOpNoReg8(ip,
                [this, shift](uint8_t op)
                {
                    if (shift == 0)
                        return op;

                    uint8_t result;
                    bool of, cf;

                    if (shift <= 8)
                    {
                        result = op << shift;
                        cf     = (result >> (8 - shift)) & 1;
                        of     = cf & (op >> 7);
                    }
                    else
                    {
                        result = 0;
                        cf     = 0;
                        of     = 0;
                    }

                    SetLogicFlags8(result);
                    SetOF_CF(of, cf);

                    return result;
                });
            break;

        case 5: // shr r/m8, x
            ModRmModifyOpNoReg8(ip,
                [this, shift](uint8_t op)
                {
                    if (shift == 0)
                        return op;

                    uint8_t result;
                    bool of, cf;

                    if (shift <= 8)
                    {
                        result = op >> shift;
                        cf     = (result >> (shift - 1)) & 1;
                        of     = (((result << 1) ^ result) >> 7) & 1;
                    }
                    else
                    {
                        result = 0;
                        cf     = 0;
                        of     = 0;
                    }

                    SetLogicFlags8(result);
                    SetOF_CF(of, cf);

                    return result;
                });
            break;

        case 6: // sal r/m16, x
        case 7: // sar r/m16, x
            m_state |= State::InvalidOp;
            break;
    }
}

void Cpu::ExecuteInstruction()
{
    uint16_t opcode, length, offset, value;
    uint16_t *reg;
    uint8_t  *ip;

    RestartDecoding:
    ip = m_memory + m_register[Register::CS] * 16 + m_register[Register::IP];
    opcode = *ip++;

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

    printf("%s\n", Disasm(*this, m_rMemory).Process().c_str());

    switch(opcode)
    {
        case 0x00: // add r/m8, r8
            ModRmModifyOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 + op2;
                    SetAddFlags8(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x01: // add r/m16, r16
            ModRmModifyOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 + op2;
                    SetAddFlags16(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x02: // add r8, r/m8
            ModRmLoadOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 + op2;
                    SetAddFlags8(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x03: // add r16, r/m16
            ModRmLoadOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 + op2;
                    SetAddFlags16(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x04: // add al, imm8
            {
                uint8_t op1 = m_register[Register::AX];
                uint8_t op2 = *ip;
                uint8_t result = op1 + op2;

                SetAddFlags8(op1, op2, result);
                m_register[Register::AX] = (m_register[Register::AX] & 0xff00) | result;
            }
            m_register[Register::IP] += 2;
            break;

        case 0x05: // add ax, imm16
            {
                uint16_t op1 = m_register[Register::AX];
                uint16_t op2 = *reinterpret_cast<uint16_t *>(ip);
                uint16_t result = op1 + op2;

                SetAddFlags16(op1, op2, result);
                m_register[Register::AX] = result;
            }
            m_register[Register::IP] += 3;
            break;

        case 0x06: // push es
            Push16(m_register[Register::ES]);
            m_register[Register::IP] += 1;
            break;

        case 0x07: // pop ds
            m_register[Register::ES] = Pop16();
            m_register[Register::IP] += 1;
            break;

        case 0x08: // or r/m8, r8
            ModRmModifyOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 | op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;

                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x09: // or r/m16, r16
            ModRmModifyOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 | op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;

                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x0a: // or r8, r/m8
            ModRmLoadOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 | op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;

                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x0b: // or r16, r/m16
            ModRmLoadOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 | op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;

                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x0e: // push cs
            Push16(m_register[Register::CS]);
            m_register[Register::IP] += 1;
            break;

        case 0x16: // push ss
            Push16(m_register[Register::SS]);
            m_register[Register::IP] += 1;
            break;

        case 0x1e: // push ds
            Push16(m_register[Register::DS]);
            m_register[Register::IP] += 1;
            break;

        case 0x1f: // pop ds
            m_register[Register::DS] = Pop16();
            m_register[Register::IP] += 1;
            m_segmentBase = m_register[Register::DS] * 16;
            break;

        case 0x24: // and al, imm8
            {
                uint8_t op1 = m_register[Register::AX];
                uint8_t op2 = *ip;
                uint8_t result = op1 & op2;

                SetLogicFlags8(result);
                m_register[Register::AX] = (m_register[Register::AX] & 0xff00) | result;
            }
            m_register[Register::IP] += 2;
            break;

        case 0x25: // and ax, imm16
            {
                uint16_t op1 = m_register[Register::AX];
                uint16_t op2 = *reinterpret_cast<uint16_t *>(ip);
                uint16_t result = op1 & op2;

                SetLogicFlags16(result);
                m_register[Register::AX] = result;
            }
            m_register[Register::IP] += 3;
            break;

        case 0x26: // prefix - ES override
            m_segmentBase      = m_register[Register::ES] * 16;
            m_stackSegmentBase = m_register[Register::ES] * 16;
            m_register[Register::IP] += 1;
            m_state |= State::SegmentOverride;
            goto RestartDecoding;// continue;

        case 0x28: // sub r/m8, r8
            ModRmModifyOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 - op2;
                    SetSubFlags8(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x29: // sub r/m16, r16
            ModRmModifyOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 - op2;
                    SetSubFlags16(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x2a: // sub r8, r/m8
            ModRmLoadOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 - op2;
                    SetSubFlags8(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x2b: // sub r16, r/m16
            ModRmLoadOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 - op2;
                    SetSubFlags16(op1, op2, result);
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x2c: // sub al, imm8
            {
                uint8_t op1 = m_register[Register::AX];
                uint8_t op2 = *ip;
                uint8_t result = op1 - op2;

                SetSubFlags8(op1, op2, result);
                m_register[Register::AX] = (m_register[Register::AX] & 0xff00) | result;
            }
            m_register[Register::IP] += 2;
            break;

        case 0x2d: // sub ax, imm16
            {
                uint16_t op1 = m_register[Register::AX];
                uint16_t op2 = *reinterpret_cast<uint16_t *>(ip);
                uint16_t result = op1 - op2;

                SetSubFlags16(op1, op2, result);
                m_register[Register::AX] = result;
            }
            m_register[Register::IP] += 3;
            break;

        case 0x2e: // prefix - CS override
            m_segmentBase      = m_register[Register::CS] * 16;
            m_stackSegmentBase = m_register[Register::CS] * 16;
            m_register[Register::IP] += 1;
            m_state |= State::SegmentOverride;
            goto RestartDecoding;// continue;

        case 0x30: // xor r/m8, r8
            ModRmModifyOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 ^ op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x31: // xor r/m16, r16
            ModRmModifyOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 ^ op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x32: // xor r8, r/m8
            ModRmLoadOp8(ip,
                [this](uint8_t op1, uint8_t op2)
                {
                    uint8_t result = op1 ^ op2;
                    m_result = static_cast<char>(result);
                    m_auxbits = 0;
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x33: // xor r16, r/m16
            ModRmLoadOp16(ip,
                [this](uint16_t op1, uint16_t op2)
                {
                    uint16_t result = op1 ^ op2;
                    m_result = static_cast<short>(result);
                    m_auxbits = 0;
                    return result;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x36: // prefix - SS override
            m_segmentBase      = m_register[Register::SS] * 16;
            m_stackSegmentBase = m_register[Register::SS] * 16;
            m_register[Register::IP] += 1;
            m_state |= State::SegmentOverride;
            goto RestartDecoding;

        case 0x38: // cmp r/m8, r8
            {
                uint8_t op1  = ModRmLoad8(ip);
                uint8_t op2  = *Reg8(*ip);
                uint8_t diff = op1 - op2;

                SetSubFlags8(op1, op2, diff);
            }
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x39: // cmp r/m16, r16
            {
                uint16_t op1  = ModRmLoad16(ip);
                uint16_t op2  = *Reg16(*ip);
                uint16_t diff = op1 - op2;

                SetSubFlags16(op1, op2, diff);
            }
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x3a: // cmp r8, r/m8
            {
                uint8_t op1  = *Reg8(*ip);
                uint8_t op2  = ModRmLoad8(ip);
                uint8_t diff = op1 - op2;

                SetSubFlags8(op1, op2, diff);
            }
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x3b: // cmp r16, r/m16
            {
                uint16_t op1  = *Reg16(*ip);
                uint16_t op2  = ModRmLoad16(ip);
                uint16_t diff = op1 - op2;

                SetSubFlags16(op1, op2, diff);
            }
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x3c: // cmp al, imm8
            {
                uint8_t op1  = m_register[Register::AX];
                uint8_t op2  = *ip;
                uint8_t diff = op1 - op2;

                SetSubFlags8(op1, op2, diff);
            }
            m_register[Register::IP] += 2;
            break;

        case 0x3d: // cmp ax, imm16
            {
                uint16_t op1  = m_register[Register::AX];
                uint16_t op2  = Imm16(ip);
                uint16_t diff = op1 - op2;

                SetSubFlags16(op1, op2, diff);
            }
            m_register[Register::IP] += 3;
            break;

        case 0x3e: // prefix - DS override
            m_segmentBase      = m_register[Register::DS] * 16;
            m_stackSegmentBase = m_register[Register::DS] * 16;
            m_register[Register::IP] += 1;
            m_state |= State::SegmentOverride;
            goto RestartDecoding;

        case 0x40: case 0x41: case 0x42: case 0x43: // inc reg16
        case 0x44: case 0x45: case 0x46: case 0x47:
            m_register[opcode - 0x40]++;
            m_register[Register::IP] += 1;
            break;

        case 0x48: case 0x49: case 0x4a: case 0x4b: // dec reg16
        case 0x4c: case 0x4d: case 0x4e: case 0x4f:
            m_register[opcode - 0x48]--;
            m_register[Register::IP] += 1;
            break;

        case 0x50: case 0x51: case 0x52: case 0x53: // push reg16
        case 0x54: case 0x55: case 0x56: case 0x57:
            Push16(m_register[opcode - 0x50]);
            m_register[Register::IP] += 1;
            break;

        case 0x58: case 0x59: case 0x5a: case 0x5b: // pop reg16
        case 0x5c: case 0x5d: case 0x5e: case 0x5f:
            m_register[opcode - 0x58] = Pop16();
            m_register[Register::IP] += 1;
            break;

        case 0x70: // jo rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x71: // jno rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x72: // jb rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetCF())
                m_register[Register::IP] += offset;
            break;

        case 0x73: // jnb rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetCF())
                m_register[Register::IP] += offset;
            break;

        case 0x74: // je rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetZF())
                m_register[Register::IP] += offset;
            break;

        case 0x75: // jne rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetZF())
                m_register[Register::IP] += offset;
            break;

        case 0x76: // jna rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetCF() || GetZF())
                m_register[Register::IP] += offset;
            break;

        case 0x77: // ja rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetCF() && !GetZF())
                m_register[Register::IP] += offset;
            break;

        case 0x78: // js rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetSF())
                m_register[Register::IP] += offset;
            break;

        case 0x79: // jns rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetSF())
                m_register[Register::IP] += offset;
            break;

        case 0x7a: // jp rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetPF())
                m_register[Register::IP] += offset;
            break;

        case 0x7b: // jnp rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetPF())
                m_register[Register::IP] += offset;
            break;

        case 0x7c: // jl rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetSF() != GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x7d: // jnl rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetSF() == GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x7e: // jle rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (GetZF() || GetSF() != GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x7f: // jg rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (!GetZF() && GetSF() == GetOF())
                m_register[Register::IP] += offset;
            break;

        case 0x80:
            Handle80h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 1;
            break;

        case 0x81:
            Handle81h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 2;
            break;

        case 0x82:
            Handle80h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 1;
            break;

        case 0x83:
            Handle83h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 1;
            break;

        case 0x86: // xchg r/m8, r8
            ModRmModifyOp8(ip,
                [this, ip](uint8_t op1, uint8_t op2)
                {
                    *Reg8(*ip) = op1;
                    return op2;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x87: // xchg r/m16, r16
            ModRmModifyOp16(ip,
                [this, ip](uint16_t op1, uint16_t op2)
                {
                    *Reg16(*ip) = op1;
                    return op2;
                });
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x88: // mov r/m8, r8
            ModRmStore8(ip, *Reg8(*ip));
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x89: // mov r/m16, r16
            ModRmStore16(ip, *Reg16(*ip));
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x8a: // mov r8, r/m8
            *Reg8(*ip) = ModRmLoad8(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x8b: // mov r16, r/m16
            *Reg16(*ip) = ModRmLoad16(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x8c: // mov r/m16, Sreg
            ModRmStore16(ip, *SReg(*ip));
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x8e: // mov Sreg, r/m16
            *SReg(*ip) = ModRmLoad16(ip);
            m_segmentBase      = m_register[Register::DS] * 16;
            m_stackSegmentBase = m_register[Register::SS] * 16;
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x8f:
            Handle8Fh(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
            std::swap(m_register[Register::AX], m_register[opcode - 0x90]);
            m_register[Register::IP] += 1;
            break;

        case 0x98: // cwd
            m_register[Register::AX] = static_cast<char>(m_register[Register::AX]);
            m_register[Register::IP] += 1;
            break;

        case 0x9c: // pushf
            RecalcFlags();
            Push16(m_register[Register::FLAG]);
            m_register[Register::IP] += 1;
            break;

        case 0x9d: // popf
            m_register[Register::FLAG] = (Pop16() & ~Flag::Always0_mask) | Flag::Always1_mask;
            m_register[Register::IP] += 1;
            RestoreLazyFlags();
            break;

        case 0xa0: // mov al, moffs8
            m_register[Register::AX] = (m_register[Register::AX] & 0xff00) | Load8(m_segmentBase + Disp16(ip));
            m_register[Register::IP] += 3;
            break;

        case 0xa1: // mov ax, moffs16
            m_register[Register::AX] = Load16(m_segmentBase + Disp16(ip));
            m_register[Register::IP] += 3;
            break;

        case 0xa2: // mov moffs8, al
            Store8(m_segmentBase + Disp16(ip), m_register[Register::AX]);
            m_register[Register::IP] += 3;
            break;

        case 0xa3: // mov moffs16, ax
            Store16(m_segmentBase + Disp16(ip), m_register[Register::AX]);
            m_register[Register::IP] += 3;
            break;

        case 0xac: // lodsb
            {
                m_register[Register::AX] =
                    (m_register[Register::AX] & 0xff00) | Load8(m_register[Register::DS] * 16 + m_register[Register::SI]);
                m_register[Register::SI] += (m_register[Register::FLAG] & Flag::DF_mask) ? -1 : 1;
            }
            m_register[Register::IP] += 1;
            break;

        case 0xad: // lodsw
            {
                m_register[Register::AX] = Load16(m_register[Register::DS] * 16 + m_register[Register::SI]);
                m_register[Register::SI] += (m_register[Register::FLAG] & Flag::DF_mask) ? -2 : 2;
            }
            m_register[Register::IP] += 1;
            break;

        case 0xb0: case 0xb1: case 0xb2: case 0xb3: // mov reg8, imm8 (reg8 = al, cl, dl, bl)
            reg  = &m_register[opcode - 0xb0];
            *reg = (*reg & 0xff00) | *ip;
            m_register[Register::IP] += 2;
            break;

        case 0xb4: case 0xb5: case 0xb6: case 0xb7: // mov reg8, imm8 (reg8 = ah, ch, dh, bh)
            reg = &m_register[opcode - 0xb4];
            *reg = (*reg & 0x00ff) | (*ip << 8);
            m_register[Register::IP] += 2;
            break;

        case 0xb8: case 0xb9: case 0xba: case 0xbb: // mov reg16, imm16
        case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            m_register[opcode - 0xb8] = Imm16(ip);
            m_register[Register::IP] += 3;
            break;

        case 0xc2: // ret imm16
            m_register[Register::IP] = Pop16();
            m_register[Register::SP] += Imm16(ip);
            break;

        case 0xc3: // ret
            m_register[Register::IP] = Pop16();
            break;

        case 0xc4: // les r16, off16
            offset = Disp16(ip + 1);
            *Reg16(*ip) = Load16(m_segmentBase + offset);
            m_register[Register::ES] = Load16(m_segmentBase + offset + 2);
            m_register[Register::IP] += 4;
            break;

        case 0xc6:
            HandleC6h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 1;
            break;

        case 0xc7:
            HandleC7h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip] + 2;
            break;

        case 0xca: // retf imm16
            m_register[Register::IP] = Pop16();
            m_register[Register::CS] = Pop16();
            m_register[Register::SP] += Imm16(ip);
            break;

        case 0xcb: // retf
            m_register[Register::IP] = Pop16();
            m_register[Register::CS] = Pop16();
            break;

        case 0xcd: // int imm8
            m_register[Register::IP] += 2;
            onSoftIrq(this, *ip);
            break;

        case 0xd0: // shift r/m8, 1
            HandleShift8(ip, 1);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xd1: // shift r/m16, 1
            HandleShift16(ip, 1);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xd2: // shift r/m8, cl
            HandleShift8(ip, m_register[Register::CX]);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xd3: // shift r/m16, cl
            HandleShift16(ip, m_register[Register::CX]);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xe3: // jcxz rel8
            offset = Disp8(ip);
            m_register[Register::IP] += 2;
            if (m_register[Register::CX] == 0)
                m_register[Register::IP] += offset;
            break;

        case 0xe8: // call rel16
            offset = Disp16(ip);
            m_register[Register::IP] += 3;
            Push16(m_register[Register::IP]);
            m_register[Register::IP] += offset;
            break;

        case 0xe9: // jmp rel16
            offset = Disp16(ip);
            m_register[Register::IP] += offset + 3;
            break;

        case 0xeb: // jmp rel8
            offset = Disp8(ip);
            m_register[Register::IP] += offset + 2;
            break;

        case 0xf2:
            HandleREPNE(*ip);
            m_register[Register::IP] += 2;
            break;

        case 0xf3:
            HandleREP(*ip);
            m_register[Register::IP] += 2;
            break;

        case 0xf6:
            HandleF6h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xf7:
            HandleF7h(ip);
            m_register[Register::IP] += s_modRmInstLen[*ip];
            break;

        case 0xfa: // cli
            m_register[Register::FLAG] &= ~Flag::IF_mask;
            m_register[Register::IP] += 1;
            break;

        case 0xfb: // sti
            m_register[Register::FLAG] |= Flag::IF_mask;
            m_register[Register::IP] += 1;
            break;

        case 0xfc: // cld
            m_register[Register::FLAG] &= ~Flag::DF_mask;
            m_register[Register::IP] += 1;
            break;

        case 0xfd: // std
            m_register[Register::FLAG] |= Flag::DF_mask;
            m_register[Register::IP] += 1;
            break;

        case 0xff:
            m_register[Register::IP] += s_modRmInstLen[*ip];
            HandleFFh(ip);
            break;

        default:
            printf("Invalid opcode 0x%02x\n", *(ip - 1));
            m_state |= State::InvalidOp;
            break;
    }

    if (m_state)
    {
        if (m_state & State::InvalidOp)
            return;

        if (m_state & State::SegmentOverride)
        {
            m_segmentBase      = m_register[Register::DS] * 16;
            m_stackSegmentBase = m_register[Register::SS] * 16;
        }

        m_state = 0;
    }
}

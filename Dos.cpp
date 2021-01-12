#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "CpuInterface.h"
#include "Memory.h"
#include "Dos.h"

// EXE header
//
//   Offset Size Contents
// -------------------------------------------------------------------------------
//    +0      2  wSignature   5a4dH .EXE file signature ('MZ')
//    +2      2  wPartPage    length of partial page at end (generally ignored)
//    +4      2  wPageCnt     length of image in 512-byte pages, incl. header
//    +6      2  wReloCnt     number of items in relocation table
//    +8      2  wHdrSize     size of header in 16-byte paragraphs
//   +0aH     2  wMinAlloc    minimum RAM needed above end of prog (paragraphs)
//   +0cH     2  wMaxAlloc    maximum RAM needed above end of prog (paragraphs)
//   +0eH     2  wInitSS      segment offset of stack segment (for setting SS)
//   +10H     2  wInitSP      value for SP register when started
//   +12H     2  wChkSum      file checksum (negative sum of all words in file)
//   +14H     2  wInitIP      value for IP register when started
//   +16H     2  wInitCS      segment offset of code segment (for setting CS)
//   +18H     2  wTablOff     file-offset of first relo item (often 001cH)
//   +1aH     2  wOverlayNo   overlay number (0 for base module)
struct ExeHeader
{
    uint16_t magic;
    uint16_t partialPageSize;
    uint16_t pageCnt;
    uint16_t relocCnt;
    uint16_t headerSize;
    uint16_t minAlloc;
    uint16_t maxAlloc;
    uint16_t initSS;
    uint16_t initSP;
    uint16_t checksum;
    uint16_t initIP;
    uint16_t initCS;
    uint16_t relocOffset;
    uint16_t overlayNo;
} __attribute__((packed));

struct RelocEntry
{
    uint16_t off;
    uint16_t seg;
} __attribute__((packed));


// PSP Header
//
//   Offset Size Contents
// -------------------------------------------------------------------------------
//    +0      2  wInt20       INT 20H instruction (cd 20) (old way to exit)
//    +2      2  wNextSeg     Segment addr just beyond end of program image
//    +4      1  res1         (reserved)
//    +5      5  abDispatch   FAR CALL to DOS function dispatcher (obs)
//   +0aH     4  pfTerminate  terminate address.       See INT 22H
//   +0eH     4  pfCtlBrk     Ctrl-Break handler address   INT 23H
//   +12H     4  pfCritErr    Critical Error handler addr  INT 24H
//   +16H    22  res2         DOS reserved area
//   +2cH     2  wEnvSeg      segment address of DOS environment
//   +2eH    46  res3         DOS reserved area (handle table, et al.)
//   +5cH    16  rFCB_1       an unopened FCB for 1st cmd parameter
//   +6cH    20  rFCB_2       an unopened FCB for 2nd cmd parameter
//   +80H     1  bCmdTailLen  count of characters in command tail at 81H (also
//                            default setting for the DTA)
//   +81H   127  abCmdTail    characters from DOS command line
struct PspHeader
{
    uint8_t  int20[2];
    uint16_t nextSeg;
    uint8_t  reserved1;
    uint8_t  abDispatch[5];
    uint16_t pfTerminate[2];
    uint16_t pfCtlBrk[2];
    uint16_t pfCritErr[2];
    uint8_t  reserved2[22];
    uint16_t envSeg;
    uint8_t  reserved3[46];
    uint8_t  fcb1[16];
    uint8_t  fcb2[16];
    uint8_t  cmdTailLength;
    char     cmdTail[127];
} __attribute__((packed));

// constructor & destructor
Dos::Dos(Memory& memory)
    : m_memory(memory.GetMem())
{
}

Dos::~Dos()
{
}

// public methods
void Dos::Int21h(CpuInterface* cpu)
{
    uint8_t func = cpu->GetReg8(CpuInterface::AH);

    switch(func)
    {
        case 0x25: // Get interrupt vector
        {
            uint8_t intr = cpu->GetReg8(CpuInterface::AL);

            *reinterpret_cast<uint16_t *>(m_memory + intr * 4)     = cpu->GetReg16(CpuInterface::DS);
            *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2) = cpu->GetReg16(CpuInterface::DX);
            break;
        }

        case 0x30:  // DOS Version
        {
            // DOS 5.0
            cpu->SetReg8(CpuInterface::AL, 5);
            cpu->SetReg8(CpuInterface::AH, 0);

            cpu->SetReg16(CpuInterface::BX, 0);
            cpu->SetReg16(CpuInterface::CX, 0);
            break;
        }

        case 0x35: // Get interrupt vector
        {
            uint8_t intr = cpu->GetReg8(CpuInterface::AL);

            cpu->SetReg16(CpuInterface::ES, *reinterpret_cast<uint16_t *>(m_memory + intr * 4));
            cpu->SetReg16(CpuInterface::BX, *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2));

            break;
        }

        default:
            printf("MsDos::Int21h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

uint16_t Dos::BuildEnv(uint16_t envSeg, const std::vector<std::string> &envVars)
{
    char*    env = reinterpret_cast<char *>(m_memory + envSeg * 16);
    uint32_t envSize = 1;

    for(auto const& var : envVars)
    {
        std::size_t len = var.size();

        ::strcpy(env, var.c_str());
        env[len + 2] = 0;
        envSize += len + 1;
    }

    return envSeg + (((envSize + 15) & (~15)) >> 4);
}

void Dos::BuildPsp(uint16_t pspSeg, uint16_t envSeg, uint16_t nextSeg, const std::string &cmd)
{
    PspHeader* psp = reinterpret_cast<PspHeader *>(m_memory + pspSeg * 16);

    ::memset(psp, 0, sizeof(PspHeader));

    psp->int20[0] = 0xcd;
    psp->int20[1] = 0x20;

    psp->nextSeg = nextSeg;
    psp->envSeg  = envSeg;

    psp->cmdTailLength = cmd.size();
    ::strcpy(psp->cmdTail, cmd.c_str());
}

Dos::ImageInfo Dos::LoadExeFromFile(uint16_t startSegment, const char *filename)
{
    ImageInfo result;

    // Open file
    int fd = ::open(filename, O_RDONLY);

    if (fd < 0)
    {
        printf("Dos::LoadExeFromFile() error: could not open file '%s'\n", filename);
        result.status = -1;
        return result;
    }

    // Read header
    ExeHeader header;

    ::read(fd, &header, sizeof(ExeHeader));

    printf("Dos::LoadExeFromFile() opened '%s'...\n", filename);
    printf("Header:\n");
    printf("    magic            '%c%c'\n",        header.magic & 0xff, header.magic >> 8);
    printf("    partialPageSize  %d\n",            header.partialPageSize);
    printf("    pageCnt          %d (%d bytes)\n", header.pageCnt, header.pageCnt * 512);
    printf("    relocCnt         %d\n",            header.relocCnt);
    printf("    headerSize       %d (%d bytes)\n", header.headerSize, header.headerSize * 16);
    printf("    minAlloc         %d (%d bytes)\n", header.minAlloc, header.minAlloc * 16);
    printf("    maxAlloc         %d (%d bytes)\n", header.maxAlloc, header.maxAlloc * 16);
    printf("    initSS           0x%04x\n",        header.initSS);
    printf("    initSP           0x%04x\n",        header.initSP);
    printf("    checksum         0x%04x\n",        header.checksum);
    printf("    initIP           0x%04x\n",        header.initIP);
    printf("    initCS           0x%04x\n",        header.initCS);
    printf("    relocOffset      0x%04x\n",        header.relocOffset);
    printf("    overlayNo        %d\n",            header.overlayNo);

    // Load image into memory
    int imageSize   = header.pageCnt * 512 - header.headerSize * 16;
    int minAlloc    = header.minAlloc * 16;
    int loadAddress = startSegment * 16;

    if (loadAddress + imageSize + minAlloc > 640 * 1024)
    {
        result.status = -1;
        return result;
    }

    ::lseek(fd, header.headerSize * 16, SEEK_SET);
    ::read(fd, m_memory + loadAddress, imageSize);

    printf("Dos::LoadExeFromFile() loaded %d bytes of program image\n", imageSize);

    // Handle relocations
    RelocEntry* relocTbl = new RelocEntry[header.relocCnt];

    ::lseek(fd, header.relocOffset, SEEK_SET);
    ::read(fd, reinterpret_cast<void *>(relocTbl), header.relocCnt * sizeof(RelocEntry));

    for(uint32_t n = 0; n < header.relocCnt; n++)
    {
        RelocEntry& reloc = relocTbl[n];
        uint16_t*   fixup = reinterpret_cast<uint16_t *>(m_memory + loadAddress + reloc.seg * 16 + reloc.off);

        //printf("%04x:%04x off %d value 0x%04x\n", reloc.seg, reloc.off, reloc.seg * 16 + reloc.off, *fixup);

        *fixup += startSegment;
    }

    // Close file
    ::close(fd);

    // Return entry point
    result.imageSize = imageSize;
    result.initCS = header.initCS + startSegment;
    result.initIP = header.initIP;
    result.initSS = header.initSS + startSegment;
    result.initSP = header.initSP;

    return result;
}

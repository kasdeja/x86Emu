#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "CpuInterface.h"
#include "Memory.h"
#include "Dos.h"

#ifndef _WIN32
#define O_BINARY 0
#endif

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
    , m_lastFd(4)
{
    m_fdMap[1] = 1;
    m_fdMap[2] = 1; //2;

    m_lastBlockSeg  = 0x0100;
    m_lastBlockSize = 0;
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
        case 0x08: // Character Input without Echo
            printf("MsDos::Int21h() function 0x08 - character input without echo\n");
            // FIXME: implement? shall block if nothing available (which may be hard)
            cpu->SetReg8(CpuInterface::AL, 32); // space
            break;

        case 0x0b: // Check Input Status
            printf("MsDos::Int21h() function 0x0b - check input status\n");
            // FIXME: implement?
            // 0x00 - no character, 0xff - at least one
            cpu->SetReg8(CpuInterface::AL, 0);
            break;

        case 0x19: // Get current default drive
            cpu->SetReg8(CpuInterface::AL, 2); // C:
            break;

        case 0x1a: // Get Disk Transfer Area address
        {
            m_dtaSeg = cpu->GetReg16(CpuInterface::DS);
            m_dtaOff = cpu->GetReg16(CpuInterface::DX);
            break;
        }

        case 0x2f: // Get Disk Transfer Area address
        {
            cpu->SetReg16(CpuInterface::ES, m_dtaSeg);
            cpu->SetReg16(CpuInterface::BX, m_dtaOff);
            break;
        }

        case 0x25: // Set interrupt vector
        {
            uint8_t intr = cpu->GetReg8(CpuInterface::AL);

            *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2) = cpu->GetReg16(CpuInterface::DS);
            *reinterpret_cast<uint16_t *>(m_memory + intr * 4)     = cpu->GetReg16(CpuInterface::DX);

            printf("MsDos::Int21h() function 0x%02x - setting interrupt vector %02xh to %04x:%04x\n",
                func, intr, cpu->GetReg16(CpuInterface::DS), cpu->GetReg16(CpuInterface::DX));
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

            cpu->SetReg16(CpuInterface::ES, *reinterpret_cast<uint16_t *>(m_memory + intr * 4 + 2));
            cpu->SetReg16(CpuInterface::BX, *reinterpret_cast<uint16_t *>(m_memory + intr * 4));
            break;
        }

        case 0x3b: // Set Current Directory
        {
            char*  path = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);
            printf("MsDos::Int21h() function 0x%02x path '%s' -> '%s' not implemented!\n", func, path, FixPath(path).c_str());
            break;
        }

        case 0x3d: // Open file
        {
            char*   path = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);
            uint8_t accessMode = cpu->GetReg8(CpuInterface::AL);

            printf("MsDos::Int21h() function 0x%02x path '%s' -> '%s' accessMode %d\n", func, path, FixPath(path).c_str(), accessMode);

            int fd = ::open(FixPath(path).c_str(), O_RDONLY | O_BINARY);

            if (fd != -1)
            {
                printf("MsDos::Int21h() function 0x%02x path '%s' opened fd %d\n", func, path, m_lastFd);
                m_fdMap[m_lastFd] = fd;

                cpu->SetReg16(CpuInterface::AX, m_lastFd);
                cpu->SetFlag(CpuInterface::CF, false);
                m_lastFd++;
            }
            else
            {
                printf("MsDos::Int21h() function 0x%02x path '%s' not found \n", func, path);
                cpu->SetReg16(CpuInterface::AX, 2);
                cpu->SetFlag(CpuInterface::CF, true);
            }

            break;
        }

        case 0x3e: // Close file
        {
            int dosFd = cpu->GetReg16(CpuInterface::BX);

            if (m_fdMap.find(dosFd) == m_fdMap.end())
            {
                printf("MsDos::Int21h() function 0x%02x invalid fd %d\n", func, dosFd);
                cpu->SetReg16(CpuInterface::AX, 2);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            cpu->SetFlag(CpuInterface::CF, false);
            printf("MsDos::Int21h() function 0x%02x close fd %d\n", func, dosFd);

            int fd = m_fdMap[dosFd];

            ::close(fd);
            m_fdMap.erase(dosFd);

            break;
        }

        case 0x3f: // Read from file
        {
            int dosFd = cpu->GetReg16(CpuInterface::BX);
            int bytes = cpu->GetReg16(CpuInterface::CX);

            char* dstBuffer = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);

            if (m_fdMap.find(dosFd) == m_fdMap.end())
            {
                printf("MsDos::Int21h() function 0x%02x invalid fd %d\n", func, dosFd);
                cpu->SetReg16(CpuInterface::AX, 2);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            int fd = m_fdMap[dosFd];
            int bytesRead = ::read(fd, dstBuffer, bytes);

            printf("MsDos::Int21h() function 0x%02x read %d bytes from fd %d (requested %d)\n", func, bytesRead, dosFd, bytes);

            cpu->SetReg16(CpuInterface::AX, bytesRead);
            cpu->SetFlag(CpuInterface::CF, false);

            break;
        }

        case 0x40: // Write to file
        {
            int dosFd = cpu->GetReg16(CpuInterface::BX);
            int bytes = cpu->GetReg16(CpuInterface::CX);

            char* srcBuffer = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);

            if (m_fdMap.find(dosFd) == m_fdMap.end())
            {
                printf("MsDos::Int21h() function 0x%02x invalid fd %d\n", func, dosFd);
                cpu->SetReg16(CpuInterface::AX, 2);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            int bytesWritten;

            if (dosFd != 1 && dosFd != 2)
            {
                int fd = m_fdMap[dosFd];

                bytesWritten = ::write(fd, srcBuffer, bytes);
                printf("MsDos::Int21h() function 0x%02x write %d bytes to fd %d\n", func, bytesWritten, dosFd);
            }
            else
            {
                char buff[1024];

                ::memcpy(buff, srcBuffer, bytes);
                buff[bytes] = 0;
                printf("MsDos::Int21h() function 0x%02x write %d bytes to fd %d '%s'\n", func, bytesWritten, dosFd, buff);

                bytesWritten = bytes;
            }

            cpu->SetReg16(CpuInterface::AX, bytesWritten);
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x42: // Move File Pointer Using Handle
        {
            int dosFd    = cpu->GetReg16(CpuInterface::BX);
            int seekType = cpu->GetReg8(CpuInterface::AL);
            int offset   = (cpu->GetReg16(CpuInterface::CX) << 16) + cpu->GetReg16(CpuInterface::DX);

            if (m_fdMap.find(dosFd) == m_fdMap.end())
            {
                printf("MsDos::Int21h() function 0x%02x invalid fd %d\n", func, dosFd);
                cpu->SetReg16(CpuInterface::AX, 2);
                cpu->SetFlag(CpuInterface::CF, true);
                break;
            }

            printf("MsDos::Int21h() function 0x%02x offset %d seekType %d fd %d\n", func, offset, seekType, dosFd);

            int fd = m_fdMap[dosFd];

                 if (seekType == 0)     offset = ::lseek(fd, offset, SEEK_SET);
            else if (seekType == 1)     offset = ::lseek(fd, offset, SEEK_CUR);
            else if (seekType == 2)     offset = ::lseek(fd, offset, SEEK_END);

            cpu->SetReg16(CpuInterface::DX, offset >> 16);
            cpu->SetReg16(CpuInterface::AX, offset & 0xffff);
            cpu->SetFlag(CpuInterface::CF, false);

            break;
        }

        case 0x43: // Get / Set File Attributes
        {
            char *path = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);

            printf("MsDos::Int21h() function 0x%02x path '%s'\n", func, path);

            cpu->SetReg16(CpuInterface::CX, 0);
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x44: // I/O Control for device (IOCTL)
        {
            uint8_t cmd = cpu->GetReg8(CpuInterface::AL);
            uint16_t fd = cpu->GetReg16(CpuInterface::BX);

            if (cmd == 0)
            {
                if (fd < 3)
                {
                    uint16_t devInfo = (1 << 7) | (fd == 0 ? 1 : 2);

                    cpu->SetReg16(CpuInterface::DX, devInfo);
                    cpu->SetFlag(CpuInterface::CF, false);
                }
                else if (m_fdMap.find(fd) != m_fdMap.end())
                {
                    cpu->SetReg16(CpuInterface::DX, 2);
                    cpu->SetFlag(CpuInterface::CF, false);
                }
                else
                {
                    cpu->SetReg16(CpuInterface::AX, 6);
                    cpu->SetFlag(CpuInterface::CF, true);

                    printf("MsDos::Int21h() ioctl %d unkown fd %d!\n", cmd, fd);
                }
            }
            else if (cmd == 1)
            {
                uint16_t devInfo = cpu->GetReg16(CpuInterface::DX);

                printf("MsDos::Int21h() ioctl %d set device info fd %d info %04x\n", cmd, fd, devInfo);
            }
            else
            {
                printf("MsDos::Int21h() ioctl %d not implemented for device, fd %d!\n", cmd, fd);
            }

            break;
        }

        case 0x47:
        {
            cpu->SetReg16(CpuInterface::AX, 0);
            cpu->SetFlag(CpuInterface::CF, false);

            std::string dosPath = DosPath(m_cwd);
            char* output = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::SI);

            ::memcpy(output, dosPath.c_str(), dosPath.size() + 1);
            printf("MsDos::Int21h() function 0x%02x get current path '%s' -> '%s'\n", func, m_cwd, dosPath.c_str());

            break;
        }

        case 0x48: // Allocate memory
        {
            uint16_t newSeg = m_lastBlockSeg + (m_lastBlockSize + 15) / 16;
            uint16_t maxAvail = 0x9fff - newSeg;
            uint16_t requested = cpu->GetReg16(CpuInterface::BX);

            printf("MsDos::Int21h() allocating %d bytes memory block, new segment 0x%04x\n", requested * 16, newSeg);

            if (requested > maxAvail)
            {
                printf("MsDos::Int21h() allocation failed, not enough memory\n");
                cpu->SetFlag(CpuInterface::CF, true);
                cpu->SetReg16(CpuInterface::AX, 8);
                cpu->SetReg16(CpuInterface::BX, maxAvail);
                break;
            }

            m_lastBlockSeg = newSeg;
            m_lastBlockSize = requested * 16;
            printf("MsDos::Int21h() allocated lastBlockSeg 0x%04x size %d\n", m_lastBlockSeg, m_lastBlockSize);

            cpu->SetReg16(CpuInterface::AX, newSeg);
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x49: // Release memory
        {
            uint16_t seg = cpu->GetReg16(CpuInterface::ES);

            printf("MsDos::Int21h() releasing %d bytes memory block, segment 0x%04x lastBlockSeg 0x%04x\n", m_lastBlockSize, seg, m_lastBlockSeg);

            if (seg != m_lastBlockSeg)
            {
                printf("MsDos::Int21h() releasing failed\n");
                cpu->SetFlag(CpuInterface::CF, true);
                cpu->SetReg16(CpuInterface::AX, 9);
                break;
            }

            m_lastBlockSize = 0;
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x4a: // Resize memory block
        {
            uint16_t seg     = cpu->GetReg16(CpuInterface::ES);
            uint32_t newSize = cpu->GetReg16(CpuInterface::BX) * 16;

            printf("MsDos::Int21h() resizing memory block 0x%04x to %d bytes\n", seg, newSize);

            if (seg != m_lastBlockSeg)
            {
                printf("MsDos::Int21h() resizing failed, could not identify resized block\n");
                cpu->SetFlag(CpuInterface::CF, true);
                cpu->SetReg16(CpuInterface::AX, 9);
                cpu->SetReg16(CpuInterface::BX, 0);
                break;
            }

            if ((seg * 16 + newSize) > 0x9fff0)
            {
                printf("MsDos::Int21h() resizing failed, insufficient memory\n");
                cpu->SetFlag(CpuInterface::CF, true);
                cpu->SetReg16(CpuInterface::AX, 8);
                cpu->SetReg16(CpuInterface::BX, 0x9fff - (seg + (m_lastBlockSize + 15) / 16));
                break;
            }

            m_lastBlockSize = newSize;
            cpu->SetFlag(CpuInterface::CF, false);
            break;
        }

        case 0x4b:
        {
            uint8_t cmd = cpu->GetReg8(CpuInterface::AL);

            uint16_t* paramBlk =
                reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(m_memory) +
                cpu->GetReg16(CpuInterface::ES) * 16 + cpu->GetReg16(CpuInterface::BX));

            char *fileName = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);

            printf("MsDos::Int21h() function 0x%02x loading file '%s' -> '%s'\n", func, fileName, FixPath(fileName).c_str());
            printf("MsDos::Int21h() function 0x%02x last block seg %04x size %d (%06x)\n", func, m_lastBlockSeg, m_lastBlockSize, m_lastBlockSize);
            printf("MsDos::Int21h() function 0x%02x AX %04x DS:DX %04x:%04x ES:BX %04x:%04x\n",
                   func, cpu->GetReg16(CpuInterface::AX),
                   cpu->GetReg16(CpuInterface::DS), cpu->GetReg16(CpuInterface::DX),
                   cpu->GetReg16(CpuInterface::ES), cpu->GetReg16(CpuInterface::BX));

            for(int n = 0; n < 7; n++)
            {
                printf("MsDos::Int21h() function 0x%02x off %02x val %04x\n", func, n * 2, paramBlk[n]);
            }

            if (cmd == 0x03)
            {
                ImageInfo imageInfo = LoadImage(*paramBlk, FixPath(fileName).c_str());

                if (imageInfo.status == -1)
                {
                    printf("MsDos::Int21h() function 0x%02x cmd %d loading failed\n", func, cmd);
                    cpu->SetReg16(CpuInterface::AX, 2);     // error code: File not found
                    cpu->SetFlag(CpuInterface::CF, true);
                    break;
                }

                m_lastBlockSeg = *paramBlk;
                m_lastBlockSize = imageInfo.imageSize;

                cpu->SetFlag(CpuInterface::CF, false);
            }
            else
            {
                printf("MsDos::Int21h() function 0x%02x cmd %d not implemented yet!\n", func, cmd);
            }

            break;
        }

        case 0x4e: // Find first matching file
        {
            char *path = reinterpret_cast<char *>(m_memory) + cpu->GetReg16(CpuInterface::DS) * 16 + cpu->GetReg16(CpuInterface::DX);

            printf("MsDos::Int21h() function 0x%02x path '%s'\n", func, path);

            if (!::strcmp(path, "*.WL6"))
            {
                cpu->SetFlag(CpuInterface::CF, false);
            }
            else
            {
                cpu->SetReg16(CpuInterface::AX, 2);     // error code: File not found
                cpu->SetFlag(CpuInterface::CF, true);
            }

            break;
        }

        case 0x4c: // Exit
            printf("MsDos::Int21h() function 0x%02x\n", func);
            cpu->Stop();
            break;

        default:
            printf("MsDos::Int21h() function 0x%02x not implemented yet!\n", func);
            break;
    }
}

uint16_t Dos::BuildEnv(uint16_t envSeg, std::string cmd, const std::vector<std::string> &envVars)
{
    char*    env = reinterpret_cast<char *>(m_memory + envSeg * 16);
    uint32_t envSize = 0;

    char *bk = env;

    for(auto const& var : envVars)
    {
        std::size_t len = var.size();

        ::strcpy(env, var.c_str());

        env     += len + 1;
        envSize += len + 1;
    }

    *env++ = 0;
    *env++ = 1;
    *env++ = 0;

    ::strcpy(env, cmd.c_str());
    envSize += cmd.size() + 1;

    // for(int n = 0; n < 256; n++)
    // {
    //     printf("%02x ", bk[n]);
    // }
    // printf("\n");
    // for(int n = 0; n < 256; n++)
    // {
    //     printf("%c ", bk[n]);
    // }
    // printf("\n");

    return envSeg + (((envSize + 16) & (~15)) >> 4);
}

void Dos::BuildPsp(uint16_t pspSeg, uint16_t envSeg, uint16_t nextSeg, const std::string &args)
{
    PspHeader* psp = reinterpret_cast<PspHeader *>(m_memory + pspSeg * 16);

    ::memset(psp, 0, sizeof(PspHeader));

    psp->int20[0] = 0xcd;
    psp->int20[1] = 0x20;

    psp->nextSeg = nextSeg;
    psp->envSeg  = envSeg;

    psp->cmdTailLength = args.size();
    ::strcpy(psp->cmdTail, args.c_str());
}

void Dos::SetPspSeg(uint16_t pspSeg)
{
   m_pspSeg = pspSeg;
   m_dtaSeg = pspSeg;
   m_dtaOff = 0x080;
}

void Dos::SetCwd(std::string const& cwd)
{
    m_cwd = cwd;
}

void Dos::SetCDrive(std::string const& cDrive)
{
    m_cDrive = cDrive;
}

Dos::ImageInfo Dos::LoadExeFromFile(uint16_t startSegment, const char *filename)
{
    ImageInfo result = LoadImage(startSegment, filename);

    // Set initial values for memory allocator;
    m_lastBlockSeg = startSegment - 0x10;
    m_lastBlockSize = result.imageSize + 0x100;

    return result;
}

// private methods
std::string Dos::FixPath(std::string const& dosPath)
{
    std::string path = dosPath;

    for(std::size_t n = 0; n < path.size(); n++)
    {
        char& ch = path[n];

        if (ch == '\\')
        {
            ch = '/';
        }
        else
        {
            ch = std::tolower(ch);
        }
    }

    if (path.substr(0, 2) == "c:")
    {
        return m_cDrive + path.substr(2);
    }
    else
    {
        return m_cwd + '/' + path;
    }
}

std::string Dos::DosPath(std::string const& path)
{
    std::size_t pos = 0;
    std::string result;

    if (path[0] == '.')
        pos++;

    if (path[pos] == '/')
        pos++;

    for(std::size_t n = pos; n < path.size(); n++)
    {
        char ch = path[n];

        if (ch == '/')
        {
            ch = '\\';
        }

        result += ch;
    }

    return result;
}

Dos::ImageInfo Dos::LoadImage(uint16_t startSegment, const char *filename)
{
    ImageInfo result;

    // Open file
    int fd = ::open(filename, O_RDONLY | O_BINARY);

    if (fd < 0)
    {
        printf("Dos::LoadImage() error: could not open file '%s'\n", filename);
        result.status = -1;
        return result;
    }

    // Read header
    ExeHeader header;

    ::read(fd, &header, sizeof(ExeHeader));

    printf("Dos::LoadImage() opened '%s'...\n", filename);
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

    printf("Dos::LoadImage() startseg %04x, loadAddress %08x, imageSize %08x, minalloc %08x, total %xd\n",
           startSegment, loadAddress, imageSize, minAlloc,
           loadAddress + imageSize + minAlloc);

    if (loadAddress + imageSize + minAlloc > 640 * 1024)
    {
        result.status = -1;
        return result;
    }

    ::lseek(fd, header.headerSize * 16, SEEK_SET);
    ::read(fd, m_memory + loadAddress, imageSize);

    printf("Dos::LoadImage() loaded %d bytes of program image\n", imageSize);

    // Handle relocations
    RelocEntry* relocTbl = new RelocEntry[header.relocCnt];

    ::lseek(fd, header.relocOffset, SEEK_SET);
    ::read(fd, reinterpret_cast<void *>(relocTbl), header.relocCnt * sizeof(RelocEntry));

    for(uint32_t n = 0; n < header.relocCnt; n++)
    {
        RelocEntry& reloc = relocTbl[n];
        uint16_t*   fixup = reinterpret_cast<uint16_t *>(m_memory + loadAddress + reloc.seg * 16 + reloc.off);

        *fixup += startSegment;
    }

    // Close file
    ::close(fd);

    // Return entry point
    result.status = 0;
    result.imageSize = imageSize + minAlloc;
    result.initCS = header.initCS + startSegment;
    result.initIP = header.initIP;
    result.initSS = header.initSS + startSegment;
    result.initSP = header.initSP;

    return result;
}

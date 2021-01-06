#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "LoadExe.h"

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
    uint16_t lastPageSize;
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

// constructor & destructor
LoadExe::LoadExe(uint8_t *memory, int memorySize)
    : m_memory      (memory)
    , m_memorySize  ((memorySize > 0) ? memorySize : 640 * 1024)
{
}

LoadExe::~LoadExe()
{
}

// public methods
LoadExe::Result LoadExe::FromFile(uint16_t startSegment, const char *filename)
{
    Result result;

    // Open file
    int fd = ::open(filename, O_RDONLY);

    if (fd < 0)
    {
        printf("LoadExe::FromFile() error: could not open file '%s'\n", filename);
        result.status = -1;
        return result;
    }

    // Read header
    ExeHeader header;

    ::read(fd, &header, sizeof(ExeHeader));

    printf("LoadExe::FromFile() opened '%s'...\n", filename);
    printf("Header:\n");
    printf("    magic         '%c%c'\n",        header.magic & 0xff, header.magic >> 8);
    printf("    lastPageSize  %d\n",            header.lastPageSize);
    printf("    pageCnt       %d (%d bytes)\n", header.pageCnt, header.pageCnt * 512);
    printf("    relocCnt      %d\n",            header.relocCnt);
    printf("    headerSize    %d (%d bytes)\n", header.headerSize, header.headerSize * 16);
    printf("    minAlloc      %d (%d bytes)\n", header.minAlloc, header.minAlloc * 16);
    printf("    maxAlloc      %d (%d bytes)\n", header.maxAlloc, header.maxAlloc * 16);
    printf("    initSS        0x%04x\n",        header.initSS);
    printf("    initSP        0x%04x\n",        header.initSP);
    printf("    checksum      0x%04x\n",        header.checksum);
    printf("    initIP        0x%04x\n",        header.initIP);
    printf("    initCS        0x%04x\n",        header.initCS);
    printf("    relocOffset   0x%04x\n",        header.relocOffset);
    printf("    overlayNo     %d\n",            header.overlayNo);

    // Load image into memory
    int imageSize   = header.pageCnt * 512 + header.lastPageSize;
    int minAlloc    = header.minAlloc * 16;
    int loadAddress = startSegment * 16;

    if (loadAddress + imageSize + minAlloc > m_memorySize)
    {
        result.status = -1;
        return result;
    }

    ::lseek(fd, header.headerSize * 16, SEEK_SET);
    ::read(fd, m_memory + loadAddress, imageSize);

    printf("LoadExe::FromFile() loaded %d bytes of program image\n", imageSize);

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
    result.initCS = header.initCS + startSegment;
    result.initIP = header.initIP;
    result.initSS = header.initSS + startSegment;
    result.initSP = header.initSP;

    return result;
}


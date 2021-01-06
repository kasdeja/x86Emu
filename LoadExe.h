#ifndef X86EMU_LOAD_EXE
#define X86EMU_LOAD_EXE

#include <inttypes.h>

class LoadExe
{
public:
    struct Result
    {
        int      status;
        uint32_t imageSize;
        uint16_t initCS;
        uint16_t initIP;
        uint16_t initSS;
        uint16_t initSP;
    };

    // constructor & destructor
    LoadExe(uint8_t *memory, int memorySize = -1);
    ~LoadExe();

    // public methods
    Result FromFile(uint16_t startSegment, const char *filename);

private:
    uint8_t* m_memory;
    uint32_t m_memorySize;
};

#endif /* X86EMU_LOAD_EXE */

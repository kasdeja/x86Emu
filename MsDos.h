#ifndef X86EMU_MS_DOS
#define X86EMU_MS_DOS

#include <inttypes.h>
#include <string>
#include <vector>

// forward declarations
class CpuInterface;
class Memory;

class MsDos
{
public:
    struct ImageInfo
    {
        int      status;
        uint32_t imageSize;
        uint16_t initCS;
        uint16_t initIP;
        uint16_t initSS;
        uint16_t initSP;
    };

    // constructor & destructor
    MsDos(Memory& memory);
    ~MsDos();

    void Int21h(CpuInterface *cpu);

    uint16_t  BuildEnv(uint16_t envSeg, const std::vector<std::string> &envVars);
    void      BuildPsp(uint16_t pspSeg, uint16_t envSeg, uint16_t nextSeg, const std::string &cmd);
    ImageInfo LoadExeFromFile(uint16_t startSegment, const char *filename);

private:
    uint8_t* m_memory;
};

#endif /* X86EMU_MS_DOS */

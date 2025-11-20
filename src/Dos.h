#ifndef X86EMU_DOS
#define X86EMU_DOS

#include <inttypes.h>
#include <string>
#include <vector>
#include <unordered_map>

// forward declarations
class CpuInterface;
class Memory;

class Dos
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
    Dos(Memory& memory);
    ~Dos();

    // public methods
    void Int21h(CpuInterface *cpu);

    uint16_t  BuildEnv(uint16_t envSeg, std::string cmd, const std::vector<std::string> &envVars);
    void      BuildPsp(uint16_t pspSeg, uint16_t envSeg, uint16_t nextSeg, const std::string &args);
    void      SetPspSeg(uint16_t pspSeg);
    void      SetCwd(std::string const& cwd);

    ImageInfo LoadExeFromFile(uint16_t startSegment, const char *filename);

private:
    uint8_t* m_memory;

    uint16_t m_pspSeg;
    uint16_t m_dtaSeg;
    uint16_t m_dtaOff;

    uint16_t m_lastBlockSeg;
    uint32_t m_lastBlockSize;

    uint16_t m_lastFd;
    std::string m_cwd;
    std::unordered_map<int, int> m_fdMap;

};

#endif /* X86EMU_DOS */

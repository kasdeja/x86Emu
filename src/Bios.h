#ifndef X86EMU_BIOS
#define X86EMU_BIOS

#include <inttypes.h>
#include <string>
#include <vector>
#include <queue>

// forward declarations
class CpuInterface;
class Memory;
class Vga;

class Bios
{
public:
    // constructor & destructor
    Bios(Memory& memory, Vga& vga);
    ~Bios();

    // public methods
    void Int10h(CpuInterface *cpu);
    void Int11h(CpuInterface *cpu);
    void Int12h(CpuInterface *cpu);
    void Int13h(CpuInterface *cpu);
    void Int16h(CpuInterface *cpu);
    void Int1Ah(CpuInterface *cpu);

    void    AddKey(uint8_t key);
    uint8_t ShowKey();
    uint8_t GetKey();
    bool    HasKey();

    bool LoadMBR(int drive);
    bool OpenDrive(int drive, const std::string &fileName);
    void CloseDrive(int drive);

private:
    struct DriveInfo
    {
        int fd;
        int nHeads;
        int nSectors;
    };

    uint8_t* m_memory;
    Vga&     m_vga;

    uint8_t m_cursorX;
    uint8_t m_cursorY;

    bool m_extendedKey;
    bool m_shiftPressed;
    bool m_ctrlPressed;
    bool m_altPressed;
    bool m_capsPressed;
    uint16_t m_scanCode;

    std::queue<uint8_t>  m_keys;
    std::queue<uint16_t> m_processedKeys;

    static constexpr int m_maxDrives = 4;
    DriveInfo m_driveInfo[4];

    void ProcessKeys();
};

#endif /* X86EMU_BIOS */

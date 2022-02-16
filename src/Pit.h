#ifndef X86EMU_PIT
#define X86EMU_PIT

#include <inttypes.h>

// forward declarations
class Pic;

class Pit
{
public:
    // constructor & destructor
    Pit(Pic& pic);
    ~Pit();

    // public methods
    uint8_t PortRead(uint16_t port);
    void    PortWrite(uint16_t port, uint8_t value);

    void Process(int64_t nsec);

private:
    struct PitChannel
    {
        uint16_t divisorReg;
        uint8_t  byteNumber;  // 0 - low byte, 1 - high byte

        double   tickCount;
        double   divisor;

        PitChannel()
            : divisorReg(0)
            , byteNumber(0)
            , tickCount (0)
            , divisor   (65536)
        {
        }
    };

    Pic&       m_pic;
    uint8_t    m_cmd;
    PitChannel m_channel[3];
};

#endif /* X86EMU_PIT */

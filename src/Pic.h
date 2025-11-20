#ifndef X86EMU_PIC
#define X86EMU_PIC

#include <inttypes.h>
#include <functional>

// forward declarations
class CpuInterface;

class Pic
{
public:
    // constructor & destructor
    Pic(CpuInterface& cpu);
    ~Pic();

    // public methods
    uint8_t PortRead(uint16_t port);
    void    PortWrite(uint16_t port, uint8_t value);

    void Interrupt(int num);
    void HandleInterrupts();
    bool IsInService(int num);

    std::function<void(int irqNo)> onAck;

private:
    struct InterruptState
    {
        bool raised;
        bool masked;

        InterruptState() : raised(false), masked(false) { }
    };

    CpuInterface& m_cpu;

    InterruptState m_interrupt[16];
    int m_interruptInService;

    // private methods
    void HandleInterrupt(int num);
};

#endif /* X86EMU_PIC */

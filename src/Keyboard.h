#ifndef X86EMU_KEYBOARD
#define X86EMU_KEYBOARD

#include <inttypes.h>
#include <mutex>
#include <queue>

class Keyboard
{
public:
    // public methods
    void    AddKey(uint8_t key);
    uint8_t GetKey();
    bool    HasKey();

private:
    std::mutex          m_mutex;
    std::queue<uint8_t> m_keys;
};

#endif /* X86EMU_KEYBOARD */

#ifndef X86EMU_VGA
#define X86EMU_VGA

#include <inttypes.h>
#include <immintrin.h>
#include <vector>

// forward declarations
class Memory;

class Vga
{
public:
    enum Mode
    {
        Text,
        Mode13h
    };

    // constructor & destructor
    Vga(Memory& memory);
    ~Vga();

    // public methods
    void SetMode(Mode mode);
    void DrawScreenFiltered(uint8_t* pixels, int width, int height, int stride);

private:
    struct FilterBank
    {
        std::vector<short> coeffs;
        std::vector<char>  incTbl;

        int I;
        int D;
        int bankLength;     // number of filters in bank
        int filterLength;   // number of filter taps
    };

    static const uint8_t s_defaultColorMap[256][3];
    static const uint8_t s_defaultFont[256 * 16];

    Memory&     m_rMemory;

    uint64_t    m_colorMap[256];
    int         m_luminance[64];
    uint8_t*    m_linear;
    uint8_t*    m_videoMem;
    uint8_t*    m_videoMemText;
    int         m_currentWidth;
    int         m_currentHeight;
    Mode        m_currentMode;
    FilterBank  m_hFilter;
    FilterBank  m_vFilter;
    __m128i*    m_linebuffer;
    __m128i*    m_pixelbuffer;

    // private methods
    FilterBank DesignFilter(int inputRate, int outputRate, int taps, double cutoff);

    void DrawMode13hLine8(short *pixel, int y);
    void DrawTextModeLine8(short *pixel, int y);

    void DrawTextModeScreenFiltered(uint8_t* pixels, int width, int height, int stride);
    void DrawMode13hScreenFiltered(uint8_t* pixels, int width, int height, int stride);
};

#endif /* X86EMU_VGA */

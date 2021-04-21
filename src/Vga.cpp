#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Memory.h"
#include "Vga.h"
#include "VgaTables.h"

#define MAX_H_CUTOFF 0.8
#define MAX_V_CUTOFF 0.85

namespace
{
    int gcd(int a, int b)
    {
        while(a != b)
        {
            if (a > b)
                a -= b;
            else if (b > a)
                b -= a;
        }

        return a;
    }

    double sinc(double x)
    {
        if (x == 0)
            return 1.0;
        else
            return sin(x * M_PI) / (x * M_PI);
    }

    void BlackmanNuttallWindow(double* window, int size)
    {
        double a0 = 0.3635819;
        double a1 = 0.4891775;
        double a2 = 0.1365995;
        double a3 = 0.0106411;

        for(unsigned int n = 0; n < size; n++)
            window[n] = a0 - a1 * cos((2 * M_PI * n) / (size - 1))
                           + a2 * cos((4 * M_PI * n) / (size - 1))
                           - a3 * cos((6 * M_PI * n) / (size - 1));
    }

} // anonymouse namespace

// constructor & destructor
Vga::Vga(Memory& memory)
    : m_rMemory(memory)
{
    // Initialize
    m_currentWidth  = 0;
    m_currentHeight = 0;
    m_currentMode   = Mode::Text;

    m_sequencerIdx    = 0;
    m_graphicsCtrlIdx = 0;
    m_crtCtrlIdx      = 0;

    for(int n = 0; n < 5; n++)
        m_sequencerReg[n] = 0;

    for(int n = 0; n < 9; n++)
        m_graphicsCtrlReg[n] = 0;

    for(int n = 0; n < 35; n++)
        m_crtCtrlReg[n] = 0;

    m_verticalRetraceCnt = 0;

    // Alloc memory
    m_linear       = reinterpret_cast<uint8_t *>(aligned_alloc(32,  64 * 1024));
    m_videoMem     = memory.GetVgaMem();
    m_videoMemText = memory.GetMem() + 0xb8000;

    m_linebuffer   = reinterpret_cast<__m128i *>(aligned_alloc(32, 2048 * 8 * 3 * sizeof(short)));
    m_pixelbuffer  = nullptr;

    // Setup conversion tables (gamma correct <-> linear)
    for(int n = 0; n < 64; n++)
    {
        double x = pow(n / 64.0, 2.2) * 32768;

        if (x > 32767.0)
            x = 32767.0;

        m_luminance[n] = x;
    }

    for(int n = 0; n < 65536; n++)
        m_linear[n] = 255;

    for(int n = 0; n < 4096; n++)
        m_linear[n] = 0;

    for(int n = 0; n < 4096; n++)
    {
        double x = pow(n / 4096.0, 1.0 / 2.2) * 255;

        if (x > 255.0)
            x = 255.0;

        m_linear[n + 1024] = x;
    }

    // Initialize colormap
    m_colorMapReadIdx  = 0;
    m_colorMapWriteIdx = 0;

    ::memcpy(m_vgaColorMap, s_defaultColorMap, 3 * 256);

    for(int n = 0; n < 256; n++)
    {
        m_colorMap[n] =
            (static_cast<uint64_t>(m_luminance[s_defaultColorMap[n][0]]) << 32) +
            (static_cast<uint64_t>(m_luminance[s_defaultColorMap[n][1]]) << 16) +
            (static_cast<uint64_t>(m_luminance[s_defaultColorMap[n][2]]));
    }

    // Fill video memory
    for(int n = 0; n < 4000; n++)
    {
        m_videoMemText[2 * n]     = 32;
        m_videoMemText[2 * n + 1] = 7;
    }

    ::memset(m_videoMem, 0, 256 * 1024);

    // Sample content
//     const char *str = "This is sample text. Hello World! :)";
//
//     for(int n = 0; n < ::strlen(str); n++)
//     {
//         m_videoMemText[2 * n] = str[n];
//         m_videoMemText[2 * n + 1] = 15 + (3 << 4);
//
//         m_videoMemText[2 * n + 160] = str[n];
//         m_videoMemText[2 * n + 161] = 7;
//     }
}

Vga::~Vga()
{
    ::free(m_linear);
    ::free(m_linebuffer);

    if (m_pixelbuffer)
        ::free(m_pixelbuffer);
}

// public methods
uint8_t Vga::PortRead(uint16_t port)
{
    if (port == 0x3c9)
    {
        uint8_t value = m_vgaColorMap[m_colorMapReadIdx / 3][m_colorMapReadIdx % 3];
        printf("VGA colormap idx read %d = %d\n", m_colorMapReadIdx, value);
        m_colorMapReadIdx = (m_colorMapReadIdx + 1) % 768;

        return value;
    }
    else if (port == 0x3c5 && m_sequencerIdx < 5) // Sequencer register write
    {
        printf("VGA ");

        switch(m_sequencerIdx)
        {
            case 0: printf("Reset");                break;
            case 1: printf("Clocking Mode");        break;
            case 2: printf("Plane Mask");           break;
            case 3: printf("Character Map Select"); break;
            case 4: printf("Memory Mode");          break;
        }

        printf(" register read  = 0x%02x\n", m_sequencerReg[m_sequencerIdx]);

        return m_sequencerReg[m_sequencerIdx];
    }
    else if (port == 0x3cf && m_graphicsCtrlIdx < 9) // Graphics Controller register write
    {
        printf("VGA ");

        switch(m_graphicsCtrlIdx)
        {
            case 0: printf("Set/Reset");        break;
            case 1: printf("Enable Set/Reset"); break;
            case 2: printf("Color Compare");    break;
            case 3: printf("Data Rotate");      break;
            case 4: printf("Read Map Select");  break;
            case 5: printf("Mode");             break;
            case 6: printf("Misc");             break;
            case 7: printf("Color Don't Care"); break;
            case 8: printf("Bit Mask");         break;
        }

        printf(" register read  = 0x%02x\n", m_graphicsCtrlReg[m_graphicsCtrlIdx]);

        return m_graphicsCtrlReg[m_graphicsCtrlIdx];
    }
    else if (port == 0x3d5 && m_crtCtrlIdx < 35) // CRT Controller register write
    {
        printf("VGA ");

        switch(m_crtCtrlIdx)
        {
            case  0: printf("Horizontal Total");                break;
            case  1: printf("Horizontal Displat Enable End");   break;
            case  2: printf("Start Horizontal Blanking");       break;
            case  3: printf("End Horizontal Blanking");         break;
            case  4: printf("Start Horizontal Retrace Pulse");  break;
            case  5: printf("End Horizontal Retrace");          break;
            case  6: printf("Vertical Total");                  break;
            case  7: printf("Overflow");                        break;
            case  8: printf("Preset Row Scan");                 break;
            case  9: printf("Maximum Scan Line");               break;
            case 10: printf("Cursor Start");                    break;
            case 11: printf("Cursor End");                      break;
            case 12: printf("Start Address High");              break;
            case 13: printf("Start Address Low");               break;
            case 14: printf("Cursor Location High");            break;
            case 15: printf("Cursor Location Low");             break;
            case 16: printf("Vertical Retrace Start");          break;
            case 17: printf("Vertical Retrace End");            break;
            case 18: printf("Vertical Display Enable End");     break;
            case 19: printf("Offset");                          break;
            case 20: printf("Underline Location");              break;
            case 21: printf("Start Vertical Blanking");         break;
            case 22: printf("End Vertical Blanking");           break;
            case 23: printf("CRTC Mode Control");               break;
            case 24: printf("Line Comparator Register");        break;

            default:
                printf("Unknown");
                break;
        }

        printf(" register read  = 0x%02x\n", m_crtCtrlReg[m_crtCtrlIdx]);

        return m_crtCtrlReg[m_crtCtrlIdx];
    }
    else if (port == 0x3da)
    {
        // TODO: stub
        m_verticalRetraceCnt++;

        if (m_verticalRetraceCnt >= 60)
            m_verticalRetraceCnt = 0;

        if (m_verticalRetraceCnt < 40)
        {
            return m_verticalRetraceCnt & 1;
        }
        else if (m_verticalRetraceCnt < 50)
        {
            return 0x08;
        }
        else
        {
            return 0x01; // no retrace, no display
        }
    }
    else
    {
        printf("Unhandled VGA read port = 0x%04x\n", port);
    }

    return 0;
}

void Vga::PortWrite(uint16_t port, uint8_t value)
{
    if (port == 0x3c7)
    {
        m_colorMapReadIdx = value * 3;
        printf("VGA colormap read idx %d / %d \n", m_colorMapReadIdx, value);
    }
    else if (port == 0x3c8)
    {
        m_colorMapWriteIdx = value * 3;
        printf("VGA colormap write idx %d / %d \n", m_colorMapWriteIdx, value);
    }
    else if (port == 0x3c9)
    {
        int idx = m_colorMapWriteIdx / 3;

        m_vgaColorMap[idx][m_colorMapWriteIdx % 3] = value;
        printf("VGA colormap idx %d write = %d\n", m_colorMapWriteIdx, value);
        m_colorMapWriteIdx = (m_colorMapWriteIdx + 1) % 768;

        m_colorMap[idx] =
            (static_cast<uint64_t>(m_luminance[m_vgaColorMap[idx][0]]) << 32) +
            (static_cast<uint64_t>(m_luminance[m_vgaColorMap[idx][1]]) << 16) +
            (static_cast<uint64_t>(m_luminance[m_vgaColorMap[idx][2]]));
    }
    else if (port == 0x3c4) // Sequencer register index
    {
        printf("VGA Sequencer register index = %d\n", value);
        m_sequencerIdx = value;
    }
    else if (port == 0x3ce) // Graphics Controller register index
    {
        printf("VGA Graphics Controller register index = %d\n", value);
        m_graphicsCtrlIdx = value;
    }
    else if (port == 0x3d4) // CRT Controller register index
    {
        printf("VGA CRT Controller register index = %d\n", value);
        m_crtCtrlIdx = value;
    }
    else if (port == 0x3c5 && m_sequencerIdx < 5) // Sequencer register write
    {
        printf("VGA ");

        switch(m_sequencerIdx)
        {
            case 0: printf("Reset");                break;
            case 1: printf("Clocking Mode");        break;
            case 2: printf("Plane Mask");           break;
            case 3: printf("Character Map Select"); break;
            case 4: printf("Memory Mode");          break;
        }

        printf(" register write = 0x%02x (was 0x%02x)\n", value, m_sequencerReg[m_sequencerIdx]);

        m_sequencerReg[m_sequencerIdx] = value;

        if (m_sequencerIdx == 4)
        {
            if (value & 8)
            {
                printf("VGA Chain 4 mode is active (linear framebuffer)\n");
            }
            else
            {
                printf("VGA Chain 4 mode is off (planar framebuffer)\n");

                if (value & 4)
                    printf("VGA Plane selected by Plane Mask Register\n");
            }
        }

        if (m_sequencerIdx == 2 || m_sequencerIdx == 4)
        {
            if (onPlaneModeChange)
                onPlaneModeChange((m_sequencerReg[4] & 8) != 0, m_sequencerReg[2]);
        }
    }
    else if (port == 0x3cf && m_graphicsCtrlIdx < 9) // Graphics Controller register write
    {
        printf("VGA ");

        switch(m_graphicsCtrlIdx)
        {
            case 0: printf("Set/Reset");        break;
            case 1: printf("Enable Set/Reset"); break;
            case 2: printf("Color Compare");    break;
            case 3: printf("Data Rotate");      break;
            case 4: printf("Read Map Select");  break;
            case 5: printf("Mode");             break;
            case 6: printf("Misc");             break;
            case 7: printf("Color Don't Care"); break;
            case 8: printf("Bit Mask");         break;
        }

        printf(" register write = 0x%02x (was 0x%02x)\n", value, m_graphicsCtrlReg[m_graphicsCtrlIdx]);

        m_graphicsCtrlReg[m_graphicsCtrlIdx] = value;
    }
    else if (port == 0x3d5 && m_crtCtrlIdx < 35) // CRT Controller register write
    {
        printf("VGA ");

        switch(m_crtCtrlIdx)
        {
            case  0: printf("Horizontal Total");                break;
            case  1: printf("Horizontal Displat Enable End");   break;
            case  2: printf("Start Horizontal Blanking");       break;
            case  3: printf("End Horizontal Blanking");         break;
            case  4: printf("Start Horizontal Retrace Pulse");  break;
            case  5: printf("End Horizontal Retrace");          break;
            case  6: printf("Vertical Total");                  break;
            case  7: printf("Overflow");                        break;
            case  8: printf("Preset Row Scan");                 break;
            case  9: printf("Maximum Scan Line");               break;
            case 10: printf("Cursor Start");                    break;
            case 11: printf("Cursor End");                      break;
            case 12: printf("Start Address High");              break;
            case 13: printf("Start Address Low");               break;
            case 14: printf("Cursor Location High");            break;
            case 15: printf("Cursor Location Low");             break;
            case 16: printf("Vertical Retrace Start");          break;
            case 17: printf("Vertical Retrace End");            break;
            case 18: printf("Vertical Display Enable End");     break;
            case 19: printf("Offset");                          break;
            case 20: printf("Underline Location");              break;
            case 21: printf("Start Vertical Blanking");         break;
            case 22: printf("End Vertical Blanking");           break;
            case 23: printf("CRTC Mode Control");               break;
            case 24: printf("Line Comparator Register");        break;

            default:
                printf("Unknown");
                break;
        }

        printf(" register write = 0x%02x (was 0x%02x)\n", value, m_crtCtrlReg[m_crtCtrlIdx]);

        m_crtCtrlReg[m_crtCtrlIdx] = value;

        if (m_crtCtrlIdx == 20 || m_crtCtrlIdx == 23)
        {
            int addresingMode = ((m_crtCtrlReg[20] >> 6) & 1) * 2 + ((m_crtCtrlReg[23] >> 6) & 1);

            switch(addresingMode)
            {
                case 0: printf("VGA Addressing Mode = word\n"); break;
                case 1: printf("VGA Addressing Mode = byte\n"); break;
                case 2: printf("VGA Addressing Mode = dword\n"); break;
                case 3: printf("VGA Addressing Mode = dword\n"); break;

                default:
                    break;
            }
        }
    }
    else
    {
        printf("Unhandled VGA write port = 0x%04x, value = 0x%02x\n", port, value);
    }
}

void Vga::SetMode(Vga::Mode mode)
{
//     unsigned char g_80x25_text[] =
//     {
//     /* MISC */
//         0x67,
//     /* SEQ */
//         0x03, 0x00, 0x03, 0x00, 0x02,
//     /* CRTC */
//         0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
//         0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
//         0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
//         0xFF,
//     /* GC */
//         0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
//         0xFF,
//     /* AC */
//         0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
//         0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
//         0x0C, 0x00, 0x0F, 0x08, 0x00
//     };
//
//     unsigned char g_320x200x256[] =
//     {
//     /* MISC */
//         0x63,
//     /* SEQ */
//         0x03, 0x01, 0x0F, 0x00, 0x0E,
//     /* CRTC */
//         0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
//         0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//         0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
//         0xFF,
//     /* GC */
//         0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
//         0xFF,
//     /* AC */
//         0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
//         0x41, 0x00, 0x0F, 0x00,	0x00
//     };

    m_currentMode   = mode;
    m_currentWidth  = 0;
    m_currentHeight = 0;

    if (m_currentMode == Vga::Mode13h)
    {
        static const uint8_t seqReg[5]   = { 0x03, 0x01, 0x0f, 0x00, 0x0e };
        static const uint8_t gcReg[9]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff };
        static const uint8_t crtcReg[25] =
            { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
              0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x9c, 0x0e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
              0xff };

        ::memcpy(m_sequencerReg,    seqReg,   5);
        ::memcpy(m_graphicsCtrlReg, gcReg,    9);
        ::memcpy(m_crtCtrlReg,      crtcReg, 25);
    }
}

void Vga::DrawScreenFiltered(uint8_t* pixels, int width, int height, int stride)
{
    if (m_currentMode == Mode::Mode13h)
        DrawMode13hScreenFiltered(pixels, width, height, stride);
    else
        DrawTextModeScreenFiltered(pixels, width, height, stride);
}

// private methods
Vga::FilterBank Vga::DesignFilter(int inputRate, int outputRate, int taps, double cutoff)
{
    std::vector<double> window, coeffs;
    FilterBank result;

    // Calc I & D factors
    int tmp = gcd(inputRate, outputRate);

    int I = outputRate / tmp;
    int D = inputRate  / tmp;

    result.I            = I;
    result.D            = D;
    result.filterLength = taps;
    result.bankLength   = I;

    // Calc coeffs
    int flen     = I * taps;
    int flenHalf = flen / 2;

    coeffs.resize(flen);
    window.resize(flen);

    BlackmanNuttallWindow(window.data(), flen);

    double sum = 0;

    for(int n = 0; n < flen; n++)
    {
        coeffs[n] = sinc(((double)(n - flenHalf) * cutoff) / I) * window[n];
        sum += coeffs[n];
    }

    double scaleFactor = 1.0 / sum * I;

    for(int n = 0; n < flen; n++)
        coeffs[n] *= scaleFactor;

    // Create filter bank by sampling larger filter kernel
    int firstTap = 0;
    int coeffIdx = 0;

    result.incTbl.resize(I);
    result.coeffs.resize(I * taps);

    for(int n = 0; n < I; n++)
    {
        result.incTbl[n] = 0;

        for(int m = 0; m < taps; m++)
        {
            result.coeffs[coeffIdx] = coeffs[firstTap + m * I] * 32768;
            coeffIdx++;
        }

        firstTap -= D;

        while(firstTap < 0)
        {
            firstTap += I;
            result.incTbl[n]++;
        }
    }

    return result;
}

void Vga::DrawMode13hLine8(short *pixel, int y)
{
    for(int n = 0; n < 96; n++)
        *pixel++ = 0;

    uint8_t* line = m_videoMem + y * 320;

    for(int n = 0; n < 320; n++)
    {
        uint64_t v[8];

        for(int m = 0; m < 8; m++)
            v[m] = m_colorMap[line[m * 320]];

        for(int m = 0; m < 8; m++)
        {
            short r = v[m] >> 32;
            short g = v[m] >> 16;
            short b = v[m];

            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;

            pixel[24] = r;
            pixel[25] = g;
            pixel[26] = b;

            pixel[48] = r;
            pixel[49] = g;
            pixel[50] = b;

            pixel[72] = r;
            pixel[73] = g;
            pixel[74] = b;

            pixel += 3;
        }

        pixel += 72;
        line += 1;
    }

    for(int n = 0; n < 96; n++)
        *pixel++ = 0;
}

void Vga::DrawTextModeLine8(short *pixel, int y)
{
    for(int n = 0; n < 96; n++)
        *pixel++ = 0;

    uint8_t* textLine   = m_videoMemText + (y >> 4) * 160;
    const uint64_t* font = reinterpret_cast<const uint64_t *>(s_defaultFont + (y & 15));

    for(int n = 0; n < 80; n++)
    {
        uint64_t ch   = font[textLine[n * 2] << 1];
        uint8_t  attr = textLine[n * 2 + 1];
        uint64_t v[2];

        v[0] = m_colorMap[attr >> 4];
        v[1] = m_colorMap[attr & 15];

        for(int m = 0; m < 8; m++)
        {
            uint64_t color[8];

            color[0] = v[(ch >>  7) & 1];
            color[1] = v[(ch >> 15) & 1];
            color[2] = v[(ch >> 23) & 1];
            color[3] = v[(ch >> 31) & 1];
            color[4] = v[(ch >> 39) & 1];
            color[5] = v[(ch >> 47) & 1];
            color[6] = v[(ch >> 55) & 1];
            color[7] = v[(ch >> 63) & 1];

            for(int k = 0; k < 8; k++)
            {
                *pixel++ = color[k] >> 32;
                *pixel++ = color[k] >> 16;
                *pixel++ = color[k];
            }

            for(int k = 0; k < 8; k++)
            {
                *pixel++ = color[k] >> 32;
                *pixel++ = color[k] >> 16;
                *pixel++ = color[k];
            }

            ch <<= 1;
        }

        for(int k = 0; k < 8; k++)
        {
            *pixel++ = v[0] >> 32;
            *pixel++ = v[0] >> 16;
            *pixel++ = v[0];
        }

        for(int k = 0; k < 8; k++)
        {
            *pixel++ = v[0] >> 32;
            *pixel++ = v[0] >> 16;
            *pixel++ = v[0];
        }
    }

    for(int n = 0; n < 96; n++)
        *pixel++ = 0;
}

void Vga::DrawMode13hScreenFiltered(uint8_t* pixels, int width, int height, int stride)
{
    uint8_t* linear   = m_linear;
    int      pstride  = ((width + 7) & (~7)) * 3;
    int      pstride8 = pstride >> 3;

    // Prepare filter banks
    if (m_currentWidth != width)
    {
        double hcf = width / 1280.0;

        if (hcf > MAX_H_CUTOFF)
            hcf = MAX_H_CUTOFF;

        m_hFilter      = DesignFilter(1280, width, 8, hcf);
        m_currentWidth = width;

        if (m_pixelbuffer)
            free(m_pixelbuffer);

        std::size_t pbSize = 216 * pstride * sizeof(short);

        m_pixelbuffer = reinterpret_cast<__m128i *>(aligned_alloc(32, pbSize));
        ::memset(m_pixelbuffer, 0, pbSize);
    }

    if (m_currentHeight != height)
    {
        double vcf = height / 800.0;

        if (vcf > MAX_V_CUTOFF)
            vcf = MAX_V_CUTOFF;

        m_vFilter = DesignFilter(800, height, 8, vcf);
        m_currentHeight = height;
    }

    // Scale content and draw
    __m128i cfp  = _mm_setzero_si128();
    __m128i fix  = _mm_setzero_si128();

    (reinterpret_cast<short *>(&fix))[0] = 1024;
    fix = _mm_broadcastw_epi16(fix);

    for(int y = 0; y < 200; y += 8)
    {
        __m128i* lb = m_linebuffer;
        short*   pb = reinterpret_cast<short *>(m_pixelbuffer) + (y + 1) * pstride;

        DrawMode13hLine8(reinterpret_cast<short *>(lb), y);

        short* coeffs = m_hFilter.coeffs.data();
        char*  incTbl = m_hFilter.incTbl.data();
        int    fb     = 0;

        for(int x = 0; x < width; x++)
        {
            __m128i a = _mm_setzero_si128();
            __m128i b = _mm_setzero_si128();
            __m128i c = _mm_setzero_si128();

            __m128i* lbt = lb;

            for(int m = 0; m < 8; m++)
            {
                (reinterpret_cast<short *>(&cfp))[0] = *coeffs++;

                cfp = _mm_broadcastw_epi16(cfp);

                a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, *lbt++));
                b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, *lbt++));
                c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, *lbt++));
            }

            lb += incTbl[fb++] * 3;

            if (fb >= m_hFilter.bankLength)
            {
                coeffs = m_hFilter.coeffs.data();
                fb = 0;
            }

            short* aa = reinterpret_cast<short *>(&a);
            short* bb = reinterpret_cast<short *>(&b);
            short* cc = reinterpret_cast<short *>(&c);

            short* pixel = pb;

            pixel[0] = aa[0];   pixel[1] = aa[1];   pixel[2] = aa[2];   pixel += pstride;
            pixel[0] = aa[3];   pixel[1] = aa[4];   pixel[2] = aa[5];   pixel += pstride;
            pixel[0] = aa[6];   pixel[1] = aa[7];   pixel[2] = bb[0];   pixel += pstride;
            pixel[0] = bb[1];   pixel[1] = bb[2];   pixel[2] = bb[3];   pixel += pstride;
            pixel[0] = bb[4];   pixel[1] = bb[5];   pixel[2] = bb[6];   pixel += pstride;
            pixel[0] = bb[7];   pixel[1] = cc[0];   pixel[2] = cc[1];   pixel += pstride;
            pixel[0] = cc[2];   pixel[1] = cc[3];   pixel[2] = cc[4];   pixel += pstride;
            pixel[0] = cc[5];   pixel[1] = cc[6];   pixel[2] = cc[7];

            pb += 3;
        }
    }

    short*   coeffs = m_vFilter.coeffs.data();
    char*    incTbl = m_vFilter.incTbl.data();
    int      fb     = 0;
    int      sy     = 0;

    for(int y = 0; y < height; y++)
    {
        uint32_t* pixel = reinterpret_cast<uint32_t *>(pixels + y * stride);

        __m128i* pb[4];

        pb[0] = m_pixelbuffer + ((sy + 1) >> 2) * pstride8;
        pb[1] = m_pixelbuffer + ((sy + 2) >> 2) * pstride8;
        pb[2] = m_pixelbuffer + ((sy + 3) >> 2) * pstride8;
        pb[3] = m_pixelbuffer + ((sy + 4) >> 2) * pstride8;

        for(int x = 0; x < width; x += 8)
        {
            __m128i a = _mm_setzero_si128();
            __m128i b = _mm_setzero_si128();
            __m128i c = _mm_setzero_si128();

            __m128i* pbt[4];
            int      xoff = (x >> 3) * 3;

            pbt[0] = pb[0] + xoff;
            pbt[1] = pb[1] + xoff;
            pbt[2] = pb[2] + xoff;
            pbt[3] = pb[3] + xoff;

            if (sy & 1)
            {
                for(int m = 0; m < 8; m += 2)
                {
                    (reinterpret_cast<short *>(&cfp))[0] = coeffs[m];
                    cfp = _mm_broadcastw_epi16(cfp);

                    __m128i* pbtt = pbt[m >> 1];

                    a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, pbtt[0]));
                    b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, pbtt[1]));
                    c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, pbtt[2]));
                }
            }
            else
            {
                for(int m = 1; m < 8; m += 2)
                {
                    (reinterpret_cast<short *>(&cfp))[0] = coeffs[m];
                    cfp = _mm_broadcastw_epi16(cfp);

                    __m128i* pbtt = pbt[m >> 1];

                    a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, pbtt[0]));
                    b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, pbtt[1]));
                    c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, pbtt[2]));
                }
            }

            a = _mm_add_epi16(a, fix);
            b = _mm_add_epi16(b, fix);
            c = _mm_add_epi16(c, fix);

            short* aa = reinterpret_cast<short *>(&a);
            short* bb = reinterpret_cast<short *>(&b);
            short* cc = reinterpret_cast<short *>(&c);

            *pixel++ =  (linear[aa[0]] << 16) +
                        (linear[aa[1]] << 8) +
                        (linear[aa[2]]);
            *pixel++ =  (linear[aa[3]] << 16) +
                        (linear[aa[4]] << 8) +
                        (linear[aa[5]]);
            *pixel++ =  (linear[aa[6]] << 16) +
                        (linear[aa[7]] << 8) +
                        (linear[bb[0]]);
            *pixel++ =  (linear[bb[1]] << 16) +
                        (linear[bb[2]] << 8) +
                        (linear[bb[3]]);
            *pixel++ =  (linear[bb[4]] << 16) +
                        (linear[bb[5]] << 8) +
                        (linear[bb[6]]);
            *pixel++ =  (linear[bb[7]] << 16) +
                        (linear[cc[0]] << 8) +
                        (linear[cc[1]]);
            *pixel++ =  (linear[cc[2]] << 16) +
                        (linear[cc[3]] << 8) +
                        (linear[cc[4]]);
            *pixel++ =  (linear[cc[5]] << 16) +
                        (linear[cc[6]] << 8) +
                        (linear[cc[7]]);
        }

        coeffs += 8;
        sy += incTbl[fb++];

        if (fb >= m_vFilter.bankLength)
        {
            coeffs = m_vFilter.coeffs.data();
            fb = 0;
        }
    }
}

void Vga::DrawTextModeScreenFiltered(uint8_t* pixels, int width, int height, int stride)
{
    uint8_t* linear   = m_linear;
    int      pstride  = ((width + 7) & (~7)) * 3;
    int      pstride8 = pstride >> 3;

    // Prepare filter banks
    if (m_currentWidth != width)
    {
        double hcf = width / 1440.0;

        if (hcf > MAX_H_CUTOFF)
            hcf = MAX_H_CUTOFF;

        m_hFilter      = DesignFilter(1440, width, 8, hcf);
        m_currentWidth = width;

        if (m_pixelbuffer)
            free(m_pixelbuffer);

        std::size_t pbSize = 416 * pstride * sizeof(short);

        m_pixelbuffer = reinterpret_cast<__m128i *>(aligned_alloc(32, pbSize));
        ::memset(m_pixelbuffer, 0, pbSize);
    }

    if (m_currentHeight != height)
    {
        double vcf = height / 800.0;

        if (vcf > MAX_V_CUTOFF)
            vcf = MAX_V_CUTOFF;

        m_vFilter = DesignFilter(800, height, 8, vcf);
        m_currentHeight = height;
    }

    // Scale content and draw
    __m128i cfp  = _mm_setzero_si128();
    __m128i fix  = _mm_setzero_si128();

    (reinterpret_cast<short *>(&fix))[0] = 1024;
    fix = _mm_broadcastw_epi16(fix);

    for(int y = 0; y < 400; y += 8)
    {
        __m128i* lb = m_linebuffer;
        short*   pb = reinterpret_cast<short *>(m_pixelbuffer) + (y + 2) * pstride;

        DrawTextModeLine8(reinterpret_cast<short *>(lb), y);

        short* coeffs = m_hFilter.coeffs.data();
        char*  incTbl = m_hFilter.incTbl.data();
        int    fb     = 0;

        for(int x = 0; x < width; x++)
        {
            __m128i a = _mm_setzero_si128();
            __m128i b = _mm_setzero_si128();
            __m128i c = _mm_setzero_si128();

            __m128i* lbt = lb;

            for(int m = 0; m < 8; m++)
            {
                (reinterpret_cast<short *>(&cfp))[0] = *coeffs++;

                cfp = _mm_broadcastw_epi16(cfp);

                a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, *lbt++));
                b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, *lbt++));
                c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, *lbt++));
            }

            lb += incTbl[fb++] * 3;

            if (fb >= m_hFilter.bankLength)
            {
                coeffs = m_hFilter.coeffs.data();
                fb = 0;
            }

            short* aa = reinterpret_cast<short *>(&a);
            short* bb = reinterpret_cast<short *>(&b);
            short* cc = reinterpret_cast<short *>(&c);

            short* pixel = pb;

            pixel[0] = aa[0];   pixel[1] = aa[1];   pixel[2] = aa[2];   pixel += pstride;
            pixel[0] = aa[3];   pixel[1] = aa[4];   pixel[2] = aa[5];   pixel += pstride;
            pixel[0] = aa[6];   pixel[1] = aa[7];   pixel[2] = bb[0];   pixel += pstride;
            pixel[0] = bb[1];   pixel[1] = bb[2];   pixel[2] = bb[3];   pixel += pstride;
            pixel[0] = bb[4];   pixel[1] = bb[5];   pixel[2] = bb[6];   pixel += pstride;
            pixel[0] = bb[7];   pixel[1] = cc[0];   pixel[2] = cc[1];   pixel += pstride;
            pixel[0] = cc[2];   pixel[1] = cc[3];   pixel[2] = cc[4];   pixel += pstride;
            pixel[0] = cc[5];   pixel[1] = cc[6];   pixel[2] = cc[7];

            pb += 3;
        }
    }

    short*   coeffs = m_vFilter.coeffs.data();
    char*    incTbl = m_vFilter.incTbl.data();
    int      fb     = 0;
    int      sy     = 0;

    for(int y = 0; y < height; y++)
    {
        uint32_t* pixel = reinterpret_cast<uint32_t *>(pixels + y * stride);
        __m128i*  pb = m_pixelbuffer + (sy >> 1) * pstride8;

        for(int x = 0; x < width; x += 8)
        {
            __m128i a = _mm_setzero_si128();
            __m128i b = _mm_setzero_si128();
            __m128i c = _mm_setzero_si128();

            __m128i* pbt = pb + (x >> 3) * 3;

            if (sy & 1)
            {
                for(int m = 0; m < 8; m += 2)
                {
                    (reinterpret_cast<short *>(&cfp))[0] = coeffs[m];
                    cfp = _mm_broadcastw_epi16(cfp);

                    a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, pbt[0]));
                    b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, pbt[1]));
                    c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, pbt[2]));

                    pbt += pstride8;
                }
            }
            else
            {
                for(int m = 1; m < 8; m += 2)
                {
                    (reinterpret_cast<short *>(&cfp))[0] = coeffs[m];
                    cfp = _mm_broadcastw_epi16(cfp);

                    a = _mm_add_epi16(a, _mm_mulhi_epi16(cfp, pbt[0]));
                    b = _mm_add_epi16(b, _mm_mulhi_epi16(cfp, pbt[1]));
                    c = _mm_add_epi16(c, _mm_mulhi_epi16(cfp, pbt[2]));

                    pbt += pstride8;
                }
            }

            a = _mm_add_epi16(a, fix);
            b = _mm_add_epi16(b, fix);
            c = _mm_add_epi16(c, fix);

            short* aa = reinterpret_cast<short *>(&a);
            short* bb = reinterpret_cast<short *>(&b);
            short* cc = reinterpret_cast<short *>(&c);

            *pixel++ =  (linear[aa[0]] << 16) +
                        (linear[aa[1]] << 8) +
                        (linear[aa[2]]);
            *pixel++ =  (linear[aa[3]] << 16) +
                        (linear[aa[4]] << 8) +
                        (linear[aa[5]]);
            *pixel++ =  (linear[aa[6]] << 16) +
                        (linear[aa[7]] << 8) +
                        (linear[bb[0]]);
            *pixel++ =  (linear[bb[1]] << 16) +
                        (linear[bb[2]] << 8) +
                        (linear[bb[3]]);
            *pixel++ =  (linear[bb[4]] << 16) +
                        (linear[bb[5]] << 8) +
                        (linear[bb[6]]);
            *pixel++ =  (linear[bb[7]] << 16) +
                        (linear[cc[0]] << 8) +
                        (linear[cc[1]]);
            *pixel++ =  (linear[cc[2]] << 16) +
                        (linear[cc[3]] << 8) +
                        (linear[cc[4]]);
            *pixel++ =  (linear[cc[5]] << 16) +
                        (linear[cc[6]] << 8) +
                        (linear[cc[7]]);
        }

        coeffs += 8;
        sy += incTbl[fb++];

        if (fb >= m_vFilter.bankLength)
        {
            coeffs = m_vFilter.coeffs.data();
            fb = 0;
        }
    }
}

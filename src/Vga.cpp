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
}

Vga::~Vga()
{
    ::free(m_linear);
    ::free(m_linebuffer);

    if (m_pixelbuffer)
        ::free(m_pixelbuffer);
}

// public methods
void Vga::SetMode(Vga::Mode mode)
{
    m_currentMode = mode;
    m_currentWidth  = 0;
    m_currentHeight = 0;
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

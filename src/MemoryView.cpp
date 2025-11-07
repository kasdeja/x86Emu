#include "Memory.h"
#include "MemoryView.h"
#include "Vga.h"

// constructor & destructor
MemoryView::MemoryView(Memory *memory, Vga *vga)
    : m_memory(memory), m_vga(vga)
{
}

MemoryView::~MemoryView()
{
    m_memory = nullptr;
    m_vga = nullptr;
}

// public methods
void MemoryView::DrawRamDump(uint8_t* pixels, int width, int height, int stride)
{
    int maxLines = 1024 + 64;
    int maxColumns = 1024;

    if (height < maxLines)
        maxLines = height;

    if (width < maxColumns)
        maxColumns = width;

    uint8_t *memory = m_memory->GetMem();
    uint8_t *vgaMemory = m_memory->GetVgaMem();
    uint8_t *colormap = m_vga ? m_vga->GetColorMap() : nullptr;

    for(int y = 0; y < maxLines; y++)
    {
        uint32_t *pixel = reinterpret_cast<uint32_t *>(pixels + y * stride);
        uint8_t  *data  = memory + y * 1024;

        int x = 0;

        for(; x < maxColumns; x++)
        {
            uint8_t byte = data[x];
            pixel[x] = (byte << 16) | (byte << 8) | byte;
        }

        if (x < width)
        {
            pixel[x++] = 0xffffff;
        }

        if (y < 800 && m_vga != nullptr)
        {
            int maxVgaColumns = 320;

            if ((width - x) < maxVgaColumns)
                maxVgaColumns = width - x;

            uint8_t *vgaData = vgaMemory + 320 * y;

            for(int n = 0; n < maxVgaColumns; n++)
            {
                uint8_t *vgaPixel = &colormap[*vgaData++ * 3];
                pixel[x++] = (vgaPixel[0] << 18) | (vgaPixel[1] << 10) | (vgaPixel[2] << 2);
            }
        }
    }
}

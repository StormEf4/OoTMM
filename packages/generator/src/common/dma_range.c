#include <combo.h>
#include <combo/dma.h>
#include <combo/io.h>
#include <combo/dma_range.h>

/*
 * Partial loads from Yaz0 compressed files (fork addition).
 *
 * DmaManagerRunRequest (dma.c) decompresses a compressed file whole into the destination, whatever size was asked,
 * so LoadFile() on part of a compressed file writes past the destination buffer. This streams the Yaz0 data from the
 * ROM through a 4 KiB window and keeps only the requested range. The cost is decompressing the file up to the end of
 * the range, so it is meant for one-time loads.
 */

#define YAZ0_WINDOW     0x1000
#define YAZ0_CHUNK      0x400
#define YAZ0_HEADER     0x10

typedef struct
{
    u32 rom;
    u32 romEnd;
    u8* chunk;
    u32 chunkPos;
    u32 chunkLen;
}
Yaz0Stream;

static u8 Yaz0Stream_Read(Yaz0Stream* s)
{
    u32 len;

    if (s->chunkPos >= s->chunkLen)
    {
        len = s->romEnd - s->rom;
        if (len > YAZ0_CHUNK)
            len = YAZ0_CHUNK;
        if (len == 0)
            return 0;
        /* ROM reads are done in even sizes; the chunk buffer has room for the padding */
        DMARomToRam(s->rom | PI_DOM1_ADDR2, s->chunk, (len + 1) & ~1);
        s->rom += len;
        s->chunkPos = 0;
        s->chunkLen = len;
    }
    return s->chunk[s->chunkPos++];
}

static int Yaz0_DecompressRange(const DmaEntry* e, void* dst, u32 offset, u32 size)
{
    Yaz0Stream s;
    u8* window;
    u8* out;
    u32 pos;
    u32 end;
    u32 src;
    u32 count;
    u8 group;
    u8 b1;
    u8 b2;
    u8 byte;
    int bit;

    window = malloc(YAZ0_WINDOW + YAZ0_CHUNK + 0x10);
    if (!window)
        return 0;

    s.rom = e->pstart + YAZ0_HEADER;
    s.romEnd = e->pend;
    s.chunk = (u8*)(((u32)window + YAZ0_WINDOW + 0xf) & ~0xf);
    s.chunkPos = 0;
    s.chunkLen = 0;

    out = dst;
    pos = 0;
    end = offset + size;
    while (pos < end)
    {
        group = Yaz0Stream_Read(&s);
        for (bit = 7; bit >= 0 && pos < end; --bit)
        {
            if (group & (1 << bit))
            {
                /* Literal */
                byte = Yaz0Stream_Read(&s);
                window[pos & (YAZ0_WINDOW - 1)] = byte;
                if (pos >= offset)
                    out[pos - offset] = byte;
                pos++;
            }
            else
            {
                /* Back reference: up to 0x1000 bytes back */
                b1 = Yaz0Stream_Read(&s);
                b2 = Yaz0Stream_Read(&s);
                src = pos - ((((u32)b1 & 0xf) << 8) | b2) - 1;
                count = b1 >> 4;
                if (count == 0)
                    count = Yaz0Stream_Read(&s) + 0x12;
                else
                    count += 2;

                while (count-- && pos < end)
                {
                    byte = window[src++ & (YAZ0_WINDOW - 1)];
                    window[pos & (YAZ0_WINDOW - 1)] = byte;
                    if (pos >= offset)
                        out[pos - offset] = byte;
                    pos++;
                }
            }
        }
    }

    free(window);

    /* The data was written by the CPU: flush it so the RSP and other DMA readers see it */
    osWritebackDCache(dst, size);
    return 1;
}

int comboDmaLoadFileRange(void* dst, u32 vrom, u32 offset, u32 size)
{
    DmaEntry e;

    if (!comboDmaLookup(&e, vrom))
        return 0;
    if (e.pend == 0xffffffff)
        return 0;
    if (offset + size > e.vend - e.vstart)
        return 0;

    /* Not compressed: the DMA manager loads ranges directly */
    if (e.pend == 0)
    {
        LoadFile(dst, e.vstart + offset, size);
        return 1;
    }

    return Yaz0_DecompressRange(&e, dst, offset, size);
}

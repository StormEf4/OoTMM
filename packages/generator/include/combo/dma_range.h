#ifndef COMBO_DMA_RANGE_H
#define COMBO_DMA_RANGE_H

#include <combo/types.h>

/*
 * Loads size bytes at offset into the file at vrom (a file start, native or foreign). Works on compressed files,
 * which the DMA manager can only load whole. Returns 1 on success.
 */
int comboDmaLoadFileRange(void* dst, u32 vrom, u32 offset, u32 size);

#endif

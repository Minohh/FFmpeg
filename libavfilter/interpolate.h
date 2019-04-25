/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * interpolate functions
 */

#ifndef AVFILTER_INTERPOLATE_H
#define AVFILTER_INTERPOLATE_H

#define USE_NEW_INTERFACES 1

#include "avfilter.h"
/*******************    interpolate      *********************/
#define INTERPOLATE_PARAMS uint32_t *dst, const uint8_t *src1, const uint8_t *src2, \
                         uint32_t *weights, const uint8_t *weight_table, int alpha

typedef void (*interpolate_line_fn)(INTERPOLATE_PARAMS);
typedef int (*check_weight_fn)(uint32_t *weights);

#if USE_NEW_INTERFACES
void init_interpolate_line_fn_list(interpolate_line_fn fn_list[], int depth);

void interpolate_32x32(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_16x16(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_8x8(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_4x4(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);

void interpolate_chroma_16x16(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_chroma_8x8(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_chroma_4x4(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);
void interpolate_chroma_2x2(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fn fn_list[]);

#else
void interpolate_32x32(INTERPOLATE_PARAMS, ptrdiff_t stride);
void interpolate_chroma_16x16(INTERPOLATE_PARAMS, ptrdiff_t stride);

#endif


void average_weight_luma(uint8_t *pixels, uint32_t *weighted_pixels, uint32_t *weights, int width, int height);
void average_weight_chroma(uint8_t *pixels, uint32_t *weighted_pixels, uint32_t *weights, int uv_width, int uv_height);

/*******************        SAD           ********************/

#define SAD_PARAMS const uint8_t *src1, ptrdiff_t stride1, \
                         const uint8_t *src2, ptrdiff_t stride2, \
                         ptrdiff_t block_size, uint64_t *sum

void ff_sad_c(SAD_PARAMS);

void ff_sad16_c(SAD_PARAMS);

typedef void (*sad_fn)(SAD_PARAMS);

void init_sad_fn_list(sad_fn sad_list[], int depth);

/********************   check weights     ********************/
#if USE_NEW_INTERFACES
int init_check_weight_fn(check_weight_fn *fn);
#else
int check_weight_4_pixels(uint32_t *weights);
#endif

#endif /* AVFILTER_INTERPOLATE_H */

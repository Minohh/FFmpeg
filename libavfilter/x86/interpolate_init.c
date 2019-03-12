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

#include "libavutil/cpu.h"
#include "libavutil/x86/cpu.h"
#include "libavfilter/interpolate.h"
/***********************  SAD functions    ***********************************/

/*****************************************************************************/
/*  x86 asm funcitons                                                        */
/*****************************************************************************/
#define SAD_FUNC(FUNC_NAME, ASM_FUNC_NAME, MMSIZE)                            \
void ASM_FUNC_NAME(SAD_PARAMS);                                               \
                                                                              \
static void FUNC_NAME(SAD_PARAMS) {                                           \
    uint64_t sad[MMSIZE / 8] = {0};                                           \
    ASM_FUNC_NAME(src1, stride1, src2, stride2, block_size, sad);             \
    for (int i = 0; i < MMSIZE / 8; i++)                                      \
        *sum += sad[i];                                                       \
}

#if HAVE_X86ASM
SAD_FUNC(sad_sse2, ff_sad_sse2, 16);
#if HAVE_AVX2_EXTERNAL
SAD_FUNC(sad_avx2, ff_sad_avx2, 32);
#endif
#endif

static void sad_32x32(SAD_PARAMS)
{
    sad_avx2(src1, stride1, src2, stride2, 32, sum);
}

static void sad_4_16x16(SAD_PARAMS)
{
    sad_sse2(src1                    , stride1, src2                    , stride2, 16, sum);
    sad_sse2(src1 + 16               , stride1, src2 + 16               , stride2, 16, sum);
    sad_sse2(src1 +      16 * stride1, stride1, src2 +      16 * stride2, stride2, 16, sum);
    sad_sse2(src1 + 16 + 16 * stride1, stride1, src2 + 16 + 16 * stride2, stride2, 16, sum);
}

static void sad_16x16(SAD_PARAMS)
{
    sad_sse2(src1, stride1, src2, stride2, 16, sum);
}

/***************************************************************************/
/*    c functions                                                          */
/***************************************************************************/
#define C_SAD_FUNC(SIZE)                                                    \
static void c_sad_ ## SIZE ## x ## SIZE (SAD_PARAMS)                        \
{                                                                           \
    int i,j;                                                                \
    *sum = 0;                                                               \
    for(j = 0; j < SIZE; j++)                                               \
        for(i = 0; i < SIZE; i++)                                           \
            *sum += FFABS(src1[i + j * stride1] - src2[i + j * stride2]);   \
                                                                            \
}                                                                           \

C_SAD_FUNC(32)
C_SAD_FUNC(16)
C_SAD_FUNC(8)
C_SAD_FUNC(4)

/***************************************************************************/
/*    init function                                                        */
/***************************************************************************/
void init_sad_fun_list(sad_fun sad_list[], int depth)
{
    sad_list[0] = c_sad_32x32;
    sad_list[1] = c_sad_16x16;
    sad_list[2] = c_sad_8x8;
    sad_list[3] = c_sad_4x4;
#if HAVE_X86ASM
    int cpu_flags = av_get_cpu_flags();
    if (depth == 8) {
#if HAVE_AVX2_EXTERNAL
        if (EXTERNAL_AVX2_FAST(cpu_flags))
            sad_list[0] = sad_32x32;
#endif
        if (EXTERNAL_SSE2(cpu_flags)) {
            sad_list[0] = sad_4_16x16;
            sad_list[1] = sad_16x16;
        }
    }
#endif
}


#if 0
/************** interpolate functions   **************************************/

/*****************************************************************************/
/*  x86 asm funcitons                                                        */
/*****************************************************************************/
#define INTERPOLATE_FUNC(FUNC_NAME, ASM_FUNC_NAME, MMSIZE)                            \
void ASM_FUNC_NAME(SAD_PARAMS);                                               \
                                                                              \
static void FUNC_NAME(SAD_PARAMS) {                                           \
    uint64_t sad[MMSIZE / 8] = {0};                                           \
    ASM_FUNC_NAME(src1, stride1, src2, stride2, block_size, sad);             \
    for (int i = 0; i < MMSIZE / 8; i++)                                      \
        *sum += sad[i];                                                       \
}

#if HAVE_X86ASM
INTERPOLATE_FUNC(interpolate_sse2, ff_interpolate_sse2, 16);
#if HAVE_AVX2_EXTERNAL
INTERPOLATE_FUNC(interpolate_avx2, ff_interpolate_avx2, 32);
#endif
#endif

static void interpolate_32x32(SAD_PARAMS)
{
    interpolate_avx2(src1, stride1, src2, stride2, 32, sum);
}

static void interpolate_4_16x16(SAD_PARAMS)
{
    sad_sse2(src1                    , stride1, src2                    , stride2, 16, sum);
    sad_sse2(src1 + 16               , stride1, src2 + 16               , stride2, 16, sum);
    sad_sse2(src1 +      16 * stride1, stride1, src2 +      16 * stride2, stride2, 16, sum);
    sad_sse2(src1 + 16 + 16 * stride1, stride1, src2 + 16 + 16 * stride2, stride2, 16, sum);
}

static void sad_16x16(SAD_PARAMS)
{
    sad_sse2(src1, stride1, src2, stride2, 16, sum);
}

/***************************************************************************/
/*    c functions                                                          */
/***************************************************************************/
#define C_SAD_FUNC(SIZE)                                                    \
static void c_sad_ ## SIZE ## x ## SIZE (SAD_PARAMS)                        \
{                                                                           \
    int i,j;                                                                \
    *sum = 0;                                                               \
    for(j = 0; j < SIZE; j++)                                               \
        for(i = 0; i < SIZE; i++)                                           \
            *sum += FFABS(src1[i + j * stride1] - src2[i + j * stride2]);   \
                                                                            \
}                                                                           \

C_SAD_FUNC(32)
C_SAD_FUNC(16)
C_SAD_FUNC(8)
C_SAD_FUNC(4)

/***************************************************************************/
/*    init function                                                        */
/***************************************************************************/
void init_sad_fun_list(sad_fun sad_list[], int depth)
{
    sad_list[0] = c_sad_32x32;
    sad_list[1] = c_sad_16x16;
    sad_list[2] = c_sad_8x8;
    sad_list[3] = c_sad_4x4;
#if HAVE_X86ASM
    int cpu_flags = av_get_cpu_flags();
    if (depth == 8) {
#if HAVE_AVX2_EXTERNAL
        if (EXTERNAL_AVX2_FAST(cpu_flags))
            sad_list[0] = sad_32x32;
#endif
        if (EXTERNAL_SSE2(cpu_flags)) {
            sad_list[0] = sad_4_16x16;
            sad_list[1] = sad_16x16;
        }
    }
#endif
}
#endif
void ff_interpolate_4_pixels_sse4(INTERPOLATE_PARAMS);

void interpolate_32x32(INTERPOLATE_PARAMS, ptrdiff_t stride)
{
    int x,y;
    int offset, offset_w;
    for(y = 0; y < 32; y++)
        for(x = 0; x < 32; x+=4){
            offset   = x + y * stride;
            offset_w = x + y * 32;
            ff_interpolate_4_pixels_sse4(dst + offset, src1 + offset, src2 + offset,
                    weights + offset, weight_table + offset_w, alpha);
        }
}

void ff_interpolate_chroma_4_pixels_sse4(INTERPOLATE_PARAMS);

void interpolate_chroma_16x16(INTERPOLATE_PARAMS, ptrdiff_t stride)
{
    int x,y;
    int offset, offset_w;
    for(y = 0; y < 16; y++)
        for(x = 0; x < 16; x+=4){
            offset   = x + y * stride;
            /*weight table is still 32x32*/
            offset_w = 2 * x + (2 * y) * 32;
            ff_interpolate_chroma_4_pixels_sse4(dst + offset, src1 + offset, src2 + offset,
                    weights+offset, weight_table + offset_w, alpha);
        }
}

void ff_divide_luma_4_pixels_sse4(uint8_t *Q, uint32_t *dividend, uint32_t *divisor);

void average_weight_luma(uint8_t *pixels, uint32_t *weighted_pixels, uint32_t *weights, int width, int height)
{
    int x,y;
    int index;
    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x+=4) {
            index = x + y * width;
            ff_divide_luma_4_pixels_sse4(pixels + index, weighted_pixels + index, weights + index);
        }
    }
}

void ff_divide_chroma_4_pixels_sse4(uint8_t *Q, uint32_t *dividend, uint32_t *divisor);

void average_weight_chroma(uint8_t *pixels, uint32_t *weighted_pixels, uint32_t *weights, int uv_width, int uv_height)
{
    int x,y;
    int index, weight_index;
    for(y = 0; y < uv_height; y++) {
        for(x = 0; x < uv_width; x+=4) {
            index = x + y * uv_width;
            weight_index = 2 * x + 2 * y * 2 * uv_width;
            ff_divide_chroma_4_pixels_sse4(pixels + index, weighted_pixels + index, weights + weight_index);
        }
    }
}
/***************************************************************************/
/*    check weight function                                                */
/***************************************************************************/
void ff_check_weight_4_pixels_sse4(uint32_t *weights, int *ret);

int check_weight_4_pixels(uint32_t *weights)
{
    int ret;
    ff_check_weight_4_pixels_sse4(weights, &ret);
    return ret;
}

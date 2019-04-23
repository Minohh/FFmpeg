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


#if USE_NEW_INTERFACES
/************** interpolate functions   **************************************/

/*****************************************************************************/
/*  x86 asm funcitons                                                        */
/*****************************************************************************/
#define INTERPOLATE_LINE_FUNC(FUNC_NAME, ASM_FUNC_NAME, MMSIZE, LINESIZE)     \
                                                                              \
static void FUNC_NAME(INTERPOLATE_PARAMS) {                                   \
    int32_t step_len = MMSIZE / 4;                                            \
    int32_t x;                                                                \
    for (x = 0; x < LINESIZE; x+=step_len) {                                  \
        ASM_FUNC_NAME(dst+x, src1+x, src2+x,                                  \
                weights+x, weight_table+x, alpha);                            \
    }                                                                         \
}                                                                             \

#if HAVE_X86ASM
void ff_interpolate_4_pixels_sse4 (INTERPOLATE_PARAMS);
INTERPOLATE_LINE_FUNC(interpolate_line_32_pixels_sse4, ff_interpolate_4_pixels_sse4, 16, 32);
INTERPOLATE_LINE_FUNC(interpolate_line_16_pixels_sse4, ff_interpolate_4_pixels_sse4, 16, 16);
INTERPOLATE_LINE_FUNC(interpolate_line_8_pixels_sse4, ff_interpolate_4_pixels_sse4, 16, 8);
INTERPOLATE_LINE_FUNC(interpolate_line_4_pixels_sse4, ff_interpolate_4_pixels_sse4, 16, 4);

#if HAVE_AVX2_EXTERNAL
void ff_interpolate_8_pixels_avx2 (INTERPOLATE_PARAMS);
INTERPOLATE_LINE_FUNC(interpolate_line_32_pixels_avx2, ff_interpolate_8_pixels_avx2, 32, 32);
INTERPOLATE_LINE_FUNC(interpolate_line_16_pixels_avx2, ff_interpolate_8_pixels_avx2, 32, 16);
INTERPOLATE_LINE_FUNC(interpolate_line_8_pixels_avx2, ff_interpolate_8_pixels_avx2, 32, 8);
#endif

#endif

/***************************************************************************/
/*    c functions                                                          */
/***************************************************************************/
#define C_INTERPOLATE_LINE_FUNC(LINESIZE)                                   \
static void c_interpolate_line_ ## LINESIZE ## _pixels (INTERPOLATE_PARAMS) \
{                                                                           \
    int x;                                                                  \
    for(x = 0; x < LINESIZE; x++) {                                         \
        *(weights+x) += *(weight_table+x);                                  \
        *(dst+x) += *(weight_table+x) * (                                   \
                         (alpha       * (*(src1+x))                         \
                        +(1024-alpha) * (*(src2+x))                         \
                       ) >> 10                                              \
                      );                                                    \
    }                                                                       \
}                                                                           \

C_INTERPOLATE_LINE_FUNC(32)
C_INTERPOLATE_LINE_FUNC(16)
C_INTERPOLATE_LINE_FUNC(8)
C_INTERPOLATE_LINE_FUNC(4)

/***************************************************************************/
/*    init function                                                        */
/***************************************************************************/
void init_interpolate_line_fun_list(interpolate_line_fun fun_list[], int depth)
{
    fun_list[0] = c_interpolate_line_32_pixels;
    fun_list[1] = c_interpolate_line_16_pixels;
    fun_list[2] = c_interpolate_line_8_pixels;
    fun_list[3] = c_interpolate_line_4_pixels;
#if HAVE_X86ASM
    int cpu_flags = av_get_cpu_flags();
    if (depth == 8) {
        if (EXTERNAL_SSE4(cpu_flags)) {
            fun_list[0] = interpolate_line_32_pixels_sse4;
            fun_list[1] = interpolate_line_16_pixels_sse4;
            fun_list[2] = interpolate_line_8_pixels_sse4;
            fun_list[3] = interpolate_line_4_pixels_sse4;
        }
#if HAVE_AVX2_EXTERNAL
        if (EXTERNAL_AVX2_FAST(cpu_flags))
            fun_list[0] = interpolate_line_32_pixels_avx2;
            fun_list[1] = interpolate_line_16_pixels_avx2;
            fun_list[2] = interpolate_line_8_pixels_avx2;
#endif
    }
#endif
}

void interpolate_32x32(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fun fun_list[])
{
    int y;
    int offset, offset_w;
    for(y = 0; y < 32; y++) {
        offset   = y * stride;
        offset_w = y * 32;
        fun_list[0](dst + offset, src1 + offset, src2 + offset,
                weights + offset, weight_table + offset_w, alpha);
    }
}

void interpolate_16x16(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fun fun_list[])
{
    int y;
    int offset, offset_w;
    for(y = 0; y < 16; y++) {
        offset   = y * stride;
        offset_w = y * 16;
        fun_list[1](dst + offset, src1 + offset, src2 + offset,
                weights + offset, weight_table + offset_w, alpha);
    }
}

void interpolate_8x8(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fun fun_list[])
{
    int y;
    int offset, offset_w;
    for(y = 0; y < 8; y++) {
        offset   = y * stride;
        offset_w = y * 8;
        fun_list[2](dst + offset, src1 + offset, src2 + offset,
                weights + offset, weight_table + offset_w, alpha);
    }
}

void interpolate_4x4(INTERPOLATE_PARAMS, ptrdiff_t stride, interpolate_line_fun fun_list[])
{
    int y;
    int offset, offset_w;
    for(y = 0; y < 4; y++) {
        offset   = y * stride;
        offset_w = y * 4;
        fun_list[0](dst + offset, src1 + offset, src2 + offset,
                weights + offset, weight_table + offset_w, alpha);
    }
}
#else
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
#endif

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

;*****************************************************************************
;* x86-optimized functions for motion interpolation
;*
;* Copyright (C) 2019 Minohh
;*
;* Based on scene_sad.asm, Copyright (C) 2018 Marton Balint
;*
;* This file is part of FFmpeg.
;*
;* FFmpeg is free software; you can redistribute it and/or
;* modify it under the terms of the GNU Lesser General Public
;* License as published by the Free Software Foundation; either
;* version 2.1 of the License, or (at your option) any later version.
;*
;* FFmpeg is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;* Lesser General Public License for more details.
;*
;* You should have received a copy of the GNU Lesser General Public
;* License along with FFmpeg; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
;******************************************************************************

%include "libavutil/x86/x86util.asm"
SECTION_RODATA 32
uv_shuf:      db 0x00, 0x80, 0x80, 0x80, 0x02, 0x80, 0x80, 0x80
              db 0x04, 0x80, 0x80, 0x80, 0x06, 0x80, 0x80, 0x80
              db 0x08, 0x80, 0x80, 0x80, 0x0a, 0x80, 0x80, 0x80
              db 0x0c, 0x80, 0x80, 0x80, 0x0e, 0x80, 0x80, 0x80
two_pof_10:   db 0xff, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00
              db 0xff, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00
              db 0xff, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00
              db 0xff, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00
luma_shuf:    db 0x00, 0x80, 0x80, 0x80, 0x01, 0x80, 0x80, 0x80
              db 0x02, 0x80, 0x80, 0x80, 0x03, 0x80, 0x80, 0x80
              db 0x04, 0x80, 0x80, 0x80, 0x05, 0x80, 0x80, 0x80
              db 0x06, 0x80, 0x80, 0x80, 0x07, 0x80, 0x80, 0x80
%include "reciprocal.asm"

SECTION .text

;******************************************************************************
;    sad
;******************************************************************************
%macro SAD 0
cglobal sad, 6, 6, 3, src1, stride1, src2, stride2, block_size, sad
    pxor       m2, m2

.nextrow:
    movu            m0, [src1q]
    movu            m1, [src2q]
    psadbw          m0, m1
    paddq           m2, m0

    add     src1q, stride1q
    add     src2q, stride2q
    sub     block_sizeq, 1
    jg .nextrow

    mov         r0q, sadq
    movu      [r0q], m2      ; sum
    REP_RET
%endmacro


INIT_XMM sse2
SAD

%if HAVE_AVX2_EXTERNAL

INIT_YMM avx2
SAD

%endif




;***********************************************************************************
;  interpolate
;***********************************************************************************
%macro INTERPOLATE_SSE4 0
cglobal interpolate_4_pixels, 6, 6, 6, dst, src1, src2, weights, weight_table, alpha
    pxor            m1, m1
    pxor            m2, m2

    movu            m3, [weight_tableq]
    punpcklbw       m3, m2
    punpcklwd       m3, m2
    mova            m4, m3
    movd            m1, alphad
    pshufd          m1, m1, 0
    pmulld          m3, m1
    pslld           m4, 10
    psubd           m4, m3
    psrld           m3, 10
    psrld           m4, 10
    mova            m5, m3
    paddd           m5, m4
    movu            m0, [weightsq]
    paddd           m5, m0
    movu            [weightsq], m5

    movu            m0, [src1q]
    movu            m1, [src2q]
    punpcklbw       m0, m2
    punpcklbw       m1, m2
    punpcklwd       m0, m1

    packusdw        m3, m2
    packusdw        m4, m2
    punpcklwd       m4, m3

    pmaddwd         m0, m4

    movu            m1, [dstq]
    paddd           m0, m1
    movu            [dstq], m0
    REP_RET
%endmacro

%macro INTERPOLATE_AVX2 0
cglobal interpolate_8_pixels, 6, 6, 7, dst, src1, src2, weights, weight_table, alpha
    pxor            m1, m1
    pxor            m2, m2

    movu            m3, [weight_tableq]
    vpermq          m3, m3, 0
    mova            m6, [luma_shuf]
    pshufb          m3, m3, m6
 
    mova            m4, m3
    
    movd           xm1, alphad
    vpbroadcastd    m1, xm1
    
    pmulld          m3, m1
    pslld           m4, 10
    psubd           m4, m3
    psrld           m3, 10
    psrld           m4, 10
    mova            m5, m3
    paddd           m5, m4
    movu            m0, [weightsq]
    paddd           m5, m0
    movu            [weightsq], m5

    movu            m0, [src1q]
    movu            m1, [src2q]
    vpermq          m0, m0, 0
    vpermq          m1, m1, 0
    pshufb          m0, m0, m6
    pshufb          m1, m1, m6
    pslld           m1, 16
    paddd           m0, m1

    pslld           m3, 16
    paddd           m4, m3

    pmaddwd         m0, m4

    movu            m1, [dstq]
    paddd           m0, m1
    movu            [dstq], m0
    REP_RET
%endmacro


%macro INTERPOLATE_CHROMA_SSE4 0
cglobal interpolate_chroma_4_pixels, 6, 6, 5, dst, src1, src2, weights, weight_table, alpha
    pxor            m1, m1
    pxor            m2, m2

    movu            m3, [weight_tableq]
    mova            m4, [uv_shuf]
    pshufb          m3, m4
    mova            m4, m3
    movd            m1, alphad
    pshufd          m1, m1, 0
    pmulld          m3, m1
    pslld           m4, 10
    psubd           m4, m3
    psrld           m3, 10
    psrld           m4, 10

    movu            m0, [src1q]
    movu            m1, [src2q]
    punpcklbw       m0, m2
    punpcklbw       m1, m2
    punpcklwd       m0, m1

    packusdw        m3, m2
    packusdw        m4, m2
    punpcklwd       m4, m3

    pmaddwd         m0, m4

    movu            m1, [dstq]
    paddd           m0, m1
    movu            [dstq], m0
    REP_RET
%endmacro


%macro INTERPOLATE_CHROMA_AVX2 0
cglobal interpolate_chroma_8_pixels, 6, 6, 7, dst, src1, src2, weights, weight_table, alpha
    pxor            m1, m1
    pxor            m2, m2

    movu            m3, [weight_tableq]
    vpermq          m3, m3, 01000100b
    mova            m5, [uv_shuf]
    pshufb          m3, m3, m5
 
    mova            m4, m3
    
    movd           xm1, alphad
    vpbroadcastd    m1, xm1
    
    pmulld          m3, m1
    pslld           m4, 10
    psubd           m4, m3
    psrld           m3, 10
    psrld           m4, 10

    mova            m6, [luma_shuf]
    movu            m0, [src1q]
    movu            m1, [src2q]
    vpermq          m0, m0, 0
    vpermq          m1, m1, 0
    pshufb          m0, m0, m6
    pshufb          m1, m1, m6
    pslld           m1, 16
    paddd           m0, m1

    pslld           m3, 16
    paddd           m4, m3

    pmaddwd         m0, m4

    movu            m1, [dstq]
    paddd           m0, m1
    movu            [dstq], m0
    REP_RET
%endmacro

;***********************************************************************************
;  division
;***********************************************************************************
%macro DIVIDE_LUMA 0
cglobal divide_luma_4_pixels, 3, 5, 3, dst, dividend, divisor, tmp, addr
    pxor            m0, m0
    pxor            m2, m2

    movu            m1, [dividendq]

;    mov           tmpq, [divisorq]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 0
;    
;    mov           tmpq, [divisorq+4]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 2
;    
;    mov           tmpq, [divisorq+8]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 4
;    
;    mov           tmpq, [divisorq+12]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 6

    xor           tmpq, tmpq
    mov          addrq, reciprocal
    mov           tmpd, [divisorq]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 0

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+4]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 2

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+8]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 4

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+12]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 6

    pmulld          m2, m1
    psrld           m1, 1
    paddd           m2, m1
    psrld           m2, 16

    packusdw        m2, m0
    packuswb        m2, m0

    movd         [dstq], m2

    REP_RET
%endmacro

%macro DIVIDE_CHROMA 0
cglobal divide_chroma_4_pixels, 3, 5, 3, dst, dividend, divisor, tmp, addr
    pxor            m0, m0
    pxor            m2, m2

    movu            m1, [dividendq]

;    mov           tmpq, [divisorq]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 0
;    
;    mov           tmpq, [divisorq+8]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 2
;    
;    mov           tmpq, [divisorq+16]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 4
;    
;    mov           tmpq, [divisorq+24]
;    shl           tmpq, 1
;    add           tmpq, reciprocal
;    pinsrw          m2, [tmpq], 6

    xor           tmpq, tmpq
    mov          addrq, reciprocal
    mov           tmpd, [divisorq]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 0

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+8]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 2

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+16]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 4

    mov          addrq, reciprocal
    mov           tmpd, [divisorq+24]
    lea          addrq, [addrq+2*tmpq]
    pinsrw          m2, [addrq], 6

    pmulld          m2, m1
    psrld           m1, 1
    paddd           m2, m1
    psrld           m2, 16

    packusdw        m2, m0
    packuswb        m2, m0

    movd         [dstq], m2

    REP_RET
%endmacro

;***********************************************************************************
;  check value (0/1024) 
;  if weight == 0 or weight > 1023 
;  then return 1
;  else return 0
;***********************************************************************************
%macro CHECK_WEIGHT 1
cglobal check_weight_%1_pixels, 2, 4, 3, weights, ret, one, tmp
    pxor              m0, m0
    xor             tmpd, tmpd
    mov             oned, 1
    mova              m1, [two_pof_10]

    movu              m2, [weightsq]
    pcmpeqd           m0, m2
    cmove           tmpd, oned
    pcmpgtd           m1, m2
    cmovg           tmpd, oned
    mov           [retq], tmpd

    REP_RET
%endmacro

INIT_XMM sse4
INTERPOLATE_SSE4
INTERPOLATE_CHROMA_SSE4
DIVIDE_LUMA
DIVIDE_CHROMA
CHECK_WEIGHT 4

%if HAVE_AVX2_EXTERNAL

INIT_YMM avx2
INTERPOLATE_AVX2
INTERPOLATE_CHROMA_AVX2
CHECK_WEIGHT 8

%endif

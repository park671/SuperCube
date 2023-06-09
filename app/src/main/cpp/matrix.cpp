//
// Created by youngpark on 2023/4/3.
//

#include "matrix.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>

void setIdentityM(float sm[], int smOffset) {
    for (int i = 0; i < 16; i++) {
        sm[smOffset + i] = 0;
    }
    for (int i = 0; i < 16; i += 5) {
        sm[smOffset + i] = 1.0f;
    }
}

float *getIdentityMatrix(int size, int offset) {
    float *matrix = (float *) malloc(sizeof(float) * size);
    setIdentityM(matrix, offset);
    return matrix;
}

float length(float x, float y, float z) {
    return (float) sqrt(x * x + y * y + z * z);
}

void translateM(float *tm, int tmOffset,
                float *m, int mOffset,
                float x, float y, float z) {
    for (int i = 0; i < 12; i++) {
        tm[tmOffset + i] = m[mOffset + i];
    }
    for (int i = 0; i < 4; i++) {
        int tmi = tmOffset + i;
        int mi = mOffset + i;
        tm[12 + tmi] = m[mi] * x + m[4 + mi] * y + m[8 + mi] * z +
                       m[12 + mi];
    }
}

void translateM(
        float *m, int mOffset,
        float x, float y, float z) {
    for (int i = 0; i < 4; i++) {
        int mi = mOffset + i;
        m[12 + mi] += m[mi] * x + m[4 + mi] * y + m[8 + mi] * z;
    }
}

void setLookAtM(float *rm, int rmOffset,
                float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ, float upX, float upY,
                float upZ) {

    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:

    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / length(fx, fy, fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // compute s = f x up (x means "cross product")
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // and normalize s
    float rls = 1.0f / length(sx, sy, sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    rm[rmOffset + 0] = sx;
    rm[rmOffset + 1] = ux;
    rm[rmOffset + 2] = -fx;
    rm[rmOffset + 3] = 0.0f;

    rm[rmOffset + 4] = sy;
    rm[rmOffset + 5] = uy;
    rm[rmOffset + 6] = -fy;
    rm[rmOffset + 7] = 0.0f;

    rm[rmOffset + 8] = sz;
    rm[rmOffset + 9] = uz;
    rm[rmOffset + 10] = -fz;
    rm[rmOffset + 11] = 0.0f;

    rm[rmOffset + 12] = 0.0f;
    rm[rmOffset + 13] = 0.0f;
    rm[rmOffset + 14] = 0.0f;
    rm[rmOffset + 15] = 1.0f;

    translateM(rm, rmOffset, -eyeX, -eyeY, -eyeZ);
}

void setRotateM(float *rm, int rmOffset,
                float a, float x, float y, float z) {
    rm[rmOffset + 3] = 0;
    rm[rmOffset + 7] = 0;
    rm[rmOffset + 11] = 0;
    rm[rmOffset + 12] = 0;
    rm[rmOffset + 13] = 0;
    rm[rmOffset + 14] = 0;
    rm[rmOffset + 15] = 1;
    a *= (float) (3.1415926535 / 180.0f);
    float s = (float) sin(a);
    float c = (float) cos(a);
    if (1.0f == x && 0.0f == y && 0.0f == z) {
        rm[rmOffset + 5] = c;
        rm[rmOffset + 10] = c;
        rm[rmOffset + 6] = s;
        rm[rmOffset + 9] = -s;
        rm[rmOffset + 1] = 0;
        rm[rmOffset + 2] = 0;
        rm[rmOffset + 4] = 0;
        rm[rmOffset + 8] = 0;
        rm[rmOffset + 0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        rm[rmOffset + 0] = c;
        rm[rmOffset + 10] = c;
        rm[rmOffset + 8] = s;
        rm[rmOffset + 2] = -s;
        rm[rmOffset + 1] = 0;
        rm[rmOffset + 4] = 0;
        rm[rmOffset + 6] = 0;
        rm[rmOffset + 9] = 0;
        rm[rmOffset + 5] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        rm[rmOffset + 0] = c;
        rm[rmOffset + 5] = c;
        rm[rmOffset + 1] = s;
        rm[rmOffset + 4] = -s;
        rm[rmOffset + 2] = 0;
        rm[rmOffset + 6] = 0;
        rm[rmOffset + 8] = 0;
        rm[rmOffset + 9] = 0;
        rm[rmOffset + 10] = 1;
    } else {
        float len = length(x, y, z);
        if (1.0f != len) {
            float recipLen = 1.0f / len;
            x *= recipLen;
            y *= recipLen;
            z *= recipLen;
        }
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        rm[rmOffset + 0] = x * x * nc + c;
        rm[rmOffset + 4] = xy * nc - zs;
        rm[rmOffset + 8] = zx * nc + ys;
        rm[rmOffset + 1] = xy * nc + zs;
        rm[rmOffset + 5] = y * y * nc + c;
        rm[rmOffset + 9] = yz * nc - xs;
        rm[rmOffset + 2] = zx * nc - ys;
        rm[rmOffset + 6] = yz * nc + xs;
        rm[rmOffset + 10] = z * z * nc + c;
    }
}

void matrix_multiply_4x4_neon(float32_t *A, float32_t *B, float32_t *C) {
    // these are the columns A
    float32x4_t A0;
    float32x4_t A1;
    float32x4_t A2;
    float32x4_t A3;

    // these are the columns B
    float32x4_t B0;
    float32x4_t B1;
    float32x4_t B2;
    float32x4_t B3;

    // these are the columns C
    float32x4_t C0;
    float32x4_t C1;
    float32x4_t C2;
    float32x4_t C3;

    A0 = vld1q_f32(A);
    A1 = vld1q_f32(A + 4);
    A2 = vld1q_f32(A + 8);
    A3 = vld1q_f32(A + 12);

    // Zero accumulators for C values
    C0 = vmovq_n_f32(0);
    C1 = vmovq_n_f32(0);
    C2 = vmovq_n_f32(0);
    C3 = vmovq_n_f32(0);

    // Multiply accumulate in 4x1 blocks, i.e. each column in C
    B0 = vld1q_f32(B);
    C0 = vfmaq_laneq_f32(C0, A0, B0, 0);
    C0 = vfmaq_laneq_f32(C0, A1, B0, 1);
    C0 = vfmaq_laneq_f32(C0, A2, B0, 2);
    C0 = vfmaq_laneq_f32(C0, A3, B0, 3);
    vst1q_f32(C, C0);

    B1 = vld1q_f32(B + 4);
    C1 = vfmaq_laneq_f32(C1, A0, B1, 0);
    C1 = vfmaq_laneq_f32(C1, A1, B1, 1);
    C1 = vfmaq_laneq_f32(C1, A2, B1, 2);
    C1 = vfmaq_laneq_f32(C1, A3, B1, 3);
    vst1q_f32(C + 4, C1);

    B2 = vld1q_f32(B + 8);
    C2 = vfmaq_laneq_f32(C2, A0, B2, 0);
    C2 = vfmaq_laneq_f32(C2, A1, B2, 1);
    C2 = vfmaq_laneq_f32(C2, A2, B2, 2);
    C2 = vfmaq_laneq_f32(C2, A3, B2, 3);
    vst1q_f32(C + 8, C2);

    B3 = vld1q_f32(B + 12);
    C3 = vfmaq_laneq_f32(C3, A0, B3, 0);
    C3 = vfmaq_laneq_f32(C3, A1, B3, 1);
    C3 = vfmaq_laneq_f32(C3, A2, B3, 2);
    C3 = vfmaq_laneq_f32(C3, A3, B3, 3);
    vst1q_f32(C + 12, C3);
}

void frustumM(float *m, int offset,
              float left, float right, float bottom, float top,
              float near, float far) {
    float r_width = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth = 1.0f / (near - far);
    float x = 2.0f * (near * r_width);
    float y = 2.0f * (near * r_height);
    float A = (right + left) * r_width;
    float B = (top + bottom) * r_height;
    float C = (far + near) * r_depth;
    float D = 2.0f * (far * near * r_depth);
    m[offset + 0] = x;
    m[offset + 5] = y;
    m[offset + 8] = A;
    m[offset + 9] = B;
    m[offset + 10] = C;
    m[offset + 14] = D;
    m[offset + 11] = -1.0f;
    m[offset + 1] = 0.0f;
    m[offset + 2] = 0.0f;
    m[offset + 3] = 0.0f;
    m[offset + 4] = 0.0f;
    m[offset + 6] = 0.0f;
    m[offset + 7] = 0.0f;
    m[offset + 12] = 0.0f;
    m[offset + 13] = 0.0f;
    m[offset + 15] = 0.0f;
}

void scaleM(float *sm, int smOffset,
            float *m, int mOffset,
            float x, float y, float z) {
    for (int i = 0; i < 4; i++) {
        int smi = smOffset + i;
        int mi = mOffset + i;
        sm[smi] = m[mi] * x;
        sm[4 + smi] = m[4 + mi] * y;
        sm[8 + smi] = m[8 + mi] * z;
        sm[12 + smi] = m[12 + mi];
    }
}

void scaleM(float* m, int mOffset,
            float x, float y, float z) {
    for (int i=0 ; i<4 ; i++) {
        int mi = mOffset + i;
        m[     mi] *= x;
        m[ 4 + mi] *= y;
        m[ 8 + mi] *= z;
    }
}

void rotateM(float *m, int mOffset,
             float a, float x, float y, float z) {
    float sTemp[16], result[16];
    setRotateM(sTemp, 0, a, x, y, z);
    matrix_multiply_4x4_neon(sTemp, m, result);
    memcpy(m, result, 16);
}
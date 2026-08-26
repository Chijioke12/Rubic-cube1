#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>

struct Vec3 {
    float x, y, z;
};

struct Matrix4 {
    float m[16];

    static Matrix4 identity() {
        Matrix4 res = {0};
        res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1.0f;
        return res;
    }

    static Matrix4 perspective(float fov, float aspect, float near, float far) {
        float tanHalfFov = tan(fov / 2.0f);
        Matrix4 res = {0};
        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(far + near) / (far - near);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * far * near) / (far - near);
        return res;
    }

    static Matrix4 translate(float x, float y, float z) {
        Matrix4 res = identity();
        res.m[12] = x; res.m[13] = y; res.m[14] = z;
        return res;
    }

    static Matrix4 rotateX(float angle) {
        Matrix4 res = identity();
        float s = sin(angle);
        float c = cos(angle);
        res.m[5] = c; res.m[6] = s;
        res.m[9] = -s; res.m[10] = c;
        return res;
    }

    static Matrix4 rotateY(float angle) {
        Matrix4 res = identity();
        float s = sin(angle);
        float c = cos(angle);
        res.m[0] = c; res.m[2] = -s;
        res.m[8] = s; res.m[10] = c;
        return res;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 res = {0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    res.m[i + j * 4] += m[i + k * 4] * other.m[k + j * 4];
                }
            }
        }
        return res;
    }
};

#endif

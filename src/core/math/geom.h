#pragma once
#include <cmath>
#include <algorithm>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float Distance(const Vector3& other) const { return (*this - other).Length(); }
};

struct Vector4 {
    float x, y, z, w;
};

struct Matrix4x4 {
    float matrix[16];
};

inline Vector3 CalcAngle(const Vector3& src, const Vector3& dst) {
    Vector3 angle;
    Vector3 delta = dst - src;
    float hyp = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    angle.x = std::atan2(delta.y, delta.x) * 180.0f / 3.14159265358979323846f;
    angle.y = std::atan2(delta.z, hyp) * 180.0f / 3.14159265358979323846f;
    angle.z = 0.0f;

    angle.x += 90.0f;
    if (angle.x > 360.0f) angle.x -= 360.0f;
    if (angle.x < 0.0f) angle.x += 360.0f;

    if (angle.y > 89.0f) angle.y = 89.0f;
    if (angle.y < -89.0f) angle.y = -89.0f;

    return angle;
}
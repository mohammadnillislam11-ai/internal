#pragma once
#include <cmath>

/**
 * @file math_vector.h
 * @brief Standard 2D/3D Vector and Geometry Math Library.
 */
struct Vec2 {
    float x{0.0f}, y{0.0f};
    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float f) const { return {x * f, y * f}; }
    float length() const { return std::sqrt(x * x + y * y); }
    float distance_to(const Vec2& o) const { return (*this - o).length(); }
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};
    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float f) const { return {x * f, y * f, z * f}; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float distance_to(const Vec3& o) const { return (*this - o).length(); }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
};

struct Matrix4x4 {
    float m[4][4]{};
};

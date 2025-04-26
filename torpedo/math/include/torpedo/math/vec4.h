#pragma once

#include "torpedo/math/vec3.h"

namespace tpd {
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct vec4_t {
        constexpr vec4_t() noexcept : data{ 0, 0, 0, 0 } {}
        constexpr vec4_t(const T x, const T y, const T z, const T w) noexcept : data{ x, y, z, w } {}

        template<typename R>
        constexpr vec4_t(const vec2_t<R>& vec, T z, T w) noexcept : data{ static_cast<T>(vec.x), static_cast<T>(vec.y), z, w } {}
        constexpr vec4_t(const vec2_t<T>& vec, const T z, const T w) noexcept : data{ vec.x, vec.y, z, w } {}

        template<typename R>
        constexpr vec4_t(const vec3_t<R>& v, T w) noexcept;
        constexpr vec4_t(const vec3_t<T>& v, const T w) noexcept : data{ v.x, v.y, v.z, w } {}

        template<typename R>
        constexpr explicit vec4_t(const vec4_t<R>& other) noexcept;
        constexpr vec4_t(const vec4_t& other) noexcept : data{ other.x, other.y, other.z, other.w } {}

        template<typename R>
        constexpr vec4_t& operator=(const vec4_t<R>& other) noexcept;
        constexpr vec4_t& operator=(const vec4_t&) noexcept = default;

        T& operator[](const std::size_t i) noexcept { return data[i]; }
        T operator[](const std::size_t i) const noexcept { return data[i]; }

        vec4_t& operator+=(const vec4_t& v) noexcept;
        vec4_t& operator-=(const vec4_t& v) noexcept;
        vec4_t& operator*=(const vec4_t& v) noexcept;

        vec4_t& operator+=(T scalar) noexcept;
        vec4_t& operator-=(T scalar) noexcept;
        vec4_t& operator*=(T scalar) noexcept;
        vec4_t& operator/=(T scalar) noexcept;

        union {
            T data[4];

            struct {
                T x;
                T y;
                T z;
                T w;
            };

            vec3_t<T> xyz;
        };
    };

    using vec4 = vec4_t<float>;
    using dvec4 = vec4_t<double>;
    using uvec4 = vec4_t<uint32_t>;
    using ivec4 = vec4_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T>
    constexpr T dot(const vec4_t<T>& v0, const vec4_t<T>& v1) noexcept;

    template<typename T>
    constexpr float norm(const vec4_t<T>& vec) noexcept;

    template<typename T>
    constexpr vec4_t<T> normalize(const vec4_t<T>& vec) noexcept;
} // namespace tpd::math

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec4_t<T>::vec4_t(const vec3_t<R>& v, const T w) noexcept
    : data{ static_cast<T>(v.x), static_cast<T>(v.y), static_cast<T>(v.z), w } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec4_t<T>::vec4_t(const vec4_t<R>& other) noexcept
    : data{ static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z), static_cast<T>(other.w) } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec4_t<T>& tpd::vec4_t<T>::operator=(const vec4_t<R>& other) noexcept {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    z = static_cast<T>(other.z);
    w = static_cast<T>(other.w);
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator+=(const vec4_t& v) noexcept {
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator-=(const vec4_t& v) noexcept {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator*=(const vec4_t& v) noexcept {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    w *= v.w;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    z += scalar;
    w += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    z -= scalar;
    w -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        // Let IEEE 754 handles inf and NaN
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
    } else {
        if (scalar == 0) [[unlikely]] {
            using limits = std::numeric_limits<T>;
            x = x == 0 ? limits::quiet_NaN() : x > 0 ? limits::max() : limits::min();
            y = y == 0 ? limits::quiet_NaN() : y > 0 ? limits::max() : limits::min();
            z = z == 0 ? limits::quiet_NaN() : z > 0 ? limits::max() : limits::min();
            w = w == 0 ? limits::quiet_NaN() : w > 0 ? limits::max() : limits::min();
        }
        else {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
        }
    }
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

template<typename T>
constexpr bool operator!=(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::vec4_t<T> operator+(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
}

template<typename T>
constexpr tpd::vec4_t<T> operator-(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
}

template<typename T>
constexpr tpd::vec4_t<T> operator-(const tpd::vec4_t<T>& vec) noexcept {
    return tpd::vec3_t<T>{ -vec.x, -vec.y, -vec.z, -vec.w };
}

template<typename T>
constexpr tpd::vec4_t<T> operator*(const tpd::vec4_t<T>& vec, T scalar) noexcept {
    return tpd::vec3_t<T>{ vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar };
}

template<typename T>
constexpr tpd::vec4_t<T> operator*(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
}

template<typename T>
constexpr tpd::vec4_t<T> operator/(const tpd::vec4_t<T>& vec, const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        // Let IEEE 754 handles inf and NaN
        T inv = static_cast<T>(1.0) / scalar;
        return vec * inv;
    } else {
        using limits = std::numeric_limits<T>;
        if (scalar == 0) [[unlikely]] {
            // Handle division by zero for integral types
            T x = vec.x != 0 ? vec.x > 0 ? limits::max() : limits::min() : limits::quiet_NaN();
            T y = vec.y != 0 ? vec.y > 0 ? limits::max() : limits::min() : limits::quiet_NaN();
            T z = vec.z != 0 ? vec.z > 0 ? limits::max() : limits::min() : limits::quiet_NaN();
            T w = vec.w != 0 ? vec.w > 0 ? limits::max() : limits::min() : limits::quiet_NaN();
            return tpd::vec4_t<T>{ x, y, z, w };
        }
        // Normal division for non-zero scalar
        return tpd::vec4_t<T>{ vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar };
    }
}

template<typename T>
constexpr T tpd::math::dot(const vec4_t<T>& v0, const vec4_t<T>& v1) noexcept {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

template<typename T>
constexpr float tpd::math::norm(const vec4_t<T>& vec) noexcept {
    return std::sqrtf(static_cast<float>(dot(vec, vec)));
}

template<typename T>
constexpr tpd::vec4_t<T> tpd::math::normalize(const vec4_t<T>& vec) noexcept {
    return vec / static_cast<T>(norm(vec));
}

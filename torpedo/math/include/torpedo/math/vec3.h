#pragma once

#include "torpedo/math/vec2.h"

namespace tpd {
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct vec3_t {
        constexpr vec3_t() noexcept : data{ 0, 0, 0 } {}
        constexpr vec3_t(const T x, const T y, const T z) noexcept : data{ x, y, z } {}

        template<typename R>
        constexpr vec3_t(const vec2_t<R>& vec, const T z) noexcept : data{ static_cast<T>(vec.x), static_cast<T>(vec.y), z } {}
        constexpr vec3_t(const vec2_t<T>& vec, const T z) noexcept : data{ vec.x, vec.y, z } {}

        template<typename R>
        constexpr explicit vec3_t(const vec3_t<R>& other) noexcept;
        constexpr vec3_t(const vec3_t& other) noexcept : data{ other.x, other.y, other.z } {}

        template<typename R>
        constexpr vec3_t& operator=(const vec3_t<R>& other) noexcept;
        constexpr vec3_t& operator=(const vec3_t&) noexcept = default;

        T& operator[](const std::size_t i) noexcept { return data[i]; }
        T operator[](const std::size_t i) const noexcept { return data[i]; }

        vec3_t& operator+=(const vec3_t& vec) noexcept;
        vec3_t& operator-=(const vec3_t& vec) noexcept;
        vec3_t& operator*=(const vec3_t& vec) noexcept;

        vec3_t& operator+=(T scalar) noexcept;
        vec3_t& operator-=(T scalar) noexcept;
        vec3_t& operator*=(T scalar) noexcept;
        vec3_t& operator/=(T scalar) noexcept;

        union {
            T data[3];

            struct {
                T x;
                T y;
                T z;
            };

            vec2_t<T> xy;
        };
    };

    using vec3 = vec3_t<float>;
    using dvec3 = vec3_t<double>;
    using uvec3 = vec3_t<uint32_t>;
    using ivec3 = vec3_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T>
    constexpr vec3_t<T> cross(const vec3_t<T>& v0, const vec3_t<T>& v1) noexcept;

    template<typename T>
    constexpr T dot(const vec3_t<T>& v0, const vec3_t<T>& v1) noexcept;

    template<typename T>
    constexpr float norm(const vec3_t<T>& vec) noexcept;

    template<typename T>
    constexpr vec3_t<T> normalize(const vec3_t<T>& vec) noexcept;
} // namespace tpd::math

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec3_t<T>::vec3_t(const vec3_t<R>& other) noexcept
    : data{ static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z) } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec3_t<T>& tpd::vec3_t<T>::operator=(const vec3_t<R>& other) noexcept {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    z = static_cast<T>(other.z);
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator+=(const vec3_t& vec) noexcept {
    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator-=(const vec3_t& vec) noexcept {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator*=(const vec3_t& vec) noexcept {
    x *= vec.x;
    y *= vec.y;
    z *= vec.z;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    z += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    z -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        // Let IEEE 754 handles inf and NaN
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
    } else {
        if (scalar == 0) [[unlikely]] {
            using limits = std::numeric_limits<T>;
            x = x == 0 ? limits::quiet_NaN() : x > 0 ? limits::max() : limits::min();
            y = y == 0 ? limits::quiet_NaN() : y > 0 ? limits::max() : limits::min();
            z = z == 0 ? limits::quiet_NaN() : z > 0 ? limits::max() : limits::min();
        }
        else {
            x /= scalar;
            y /= scalar;
            z /= scalar;
        }
    }
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template<typename T>
constexpr bool operator!=(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::vec3_t<T> operator+(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

template<typename T>
constexpr tpd::vec3_t<T> operator-(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

template<typename T>
constexpr tpd::vec3_t<T> operator-(const tpd::vec3_t<T>& vec) noexcept {
    return tpd::vec3_t<T>{ -vec.x, -vec.y, -vec.z };
}

template<typename T>
constexpr tpd::vec3_t<T> operator*(const tpd::vec3_t<T>& vec, const T scalar) noexcept {
    return tpd::vec3_t<T>{ vec.x * scalar, vec.y * scalar, vec.z * scalar };
}

template<typename T>
constexpr tpd::vec3_t<T> operator*(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return tpd::vec3_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

template<typename T>
constexpr tpd::vec3_t<T> operator/(const tpd::vec3_t<T>& vec, const T scalar) noexcept {
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
            return tpd::vec3_t<T>{ x, y, z };
        }
        // Normal division for non-zero scalar
        return tpd::vec3_t<T>{ vec.x / scalar, vec.y / scalar, vec.z / scalar };
    }
}

template<typename T>
constexpr tpd::vec3_t<T> tpd::math::cross(const vec3_t<T>& v0, const vec3_t<T>& v1) noexcept {
    return tpd::vec3_t<T> {
        v0.y * v1.z - v0.z * v1.y,
        v0.z * v1.x - v0.x * v1.z,
        v0.x * v1.y - v0.y * v1.x
    };
}

template<typename T>
constexpr T tpd::math::dot(const vec3_t<T>& v0, const vec3_t<T>& v1) noexcept {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

template<typename T>
constexpr float tpd::math::norm(const vec3_t<T>& vec) noexcept {
    return std::sqrtf(static_cast<float>(dot(vec, vec)));
}

template<typename T>
constexpr tpd::vec3_t<T> tpd::math::normalize(const vec3_t<T>& vec) noexcept {
    return vec / static_cast<T>(norm(vec));
}

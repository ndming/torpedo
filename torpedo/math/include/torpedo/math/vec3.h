#pragma once

#include "torpedo/math/vec2.h"

namespace tpd {
    template<typename T> requires (std::is_trivially_copyable_v<T>)
    struct vec3_t {
        constexpr vec3_t() : data{ 0, 0, 0 } {}
        constexpr vec3_t(const T x, const T y, const T z) : data{ x, y, z } {}

        template<typename R>
        constexpr vec3_t(const vec2_t<R>& v, const T z) : data{ static_cast<T>(v.x), static_cast<T>(v.y), z } {}
        constexpr vec3_t(const vec2_t<T>& v, const T z) : data{ v.x, v.y, z } {}

        template<typename R>
        constexpr explicit vec3_t(const vec3_t<R>& other);
        constexpr vec3_t(const vec3_t& other) : data{ other.x, other.y, other.z } {}

        template<typename R>
        vec3_t& operator=(const vec3_t<R>& other);
        constexpr vec3_t& operator=(const vec3_t&) noexcept = default;

        [[nodiscard]] constexpr float length() const noexcept;
        explicit operator bool() const noexcept { return x != 0.0 || y != 0.0 || z != 0.0; }

        T& operator[](const std::size_t i) noexcept { return data[i]; }
        T operator[](const std::size_t i) const noexcept { return data[i]; }

        vec3_t& operator+=(const vec3_t& v) noexcept;
        vec3_t& operator-=(const vec3_t& v) noexcept;
        vec3_t& operator*=(const vec3_t& v) noexcept;

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

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr vec3_t<T> normalize(const vec3_t<T>& v);

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr float dot(const vec3_t<T>& lhs, const vec3_t<T>& rhs);

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr vec3_t<T> cross(const vec3_t<T>& lhs, const vec3_t<T>& rhs);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
constexpr tpd::vec3_t<T>::vec3_t(const vec3_t<R>& other)
    : data{ static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z) } {}

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
tpd::vec3_t<T>& tpd::vec3_t<T>::operator=(const vec3_t<R>& other) {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    z = static_cast<T>(other.z);
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr float tpd::vec3_t<T>::length() const noexcept {
    return static_cast<float>(std::sqrt(x * x + y * y + z * z));
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator+=(const vec3_t& v) noexcept {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator-=(const vec3_t& v) noexcept {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator*=(const vec3_t& v) noexcept {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    z += scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    z -= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec3_t<T>& tpd::vec3_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
    } else {
        x /= scalar;
        y /= scalar;
        z /= scalar;
    }
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator==(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator!=(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator+(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator-(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator-(const tpd::vec3_t<T>& v) {
    return tpd::vec3_t<T>{ -v.x, -v.y, -v.z };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator*(const tpd::vec3_t<T>& v, T scalar) {
    return tpd::vec3_t<T>{ v.x * scalar, v.y * scalar, v.z * scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator*(const tpd::vec3_t<T>& lhs, const tpd::vec3_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> operator/(const tpd::vec3_t<T>& v, T scalar) {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        return v * inv;
    }
    return tpd::vec3_t<T>{ v.x / scalar, v.y / scalar, v.z / scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> tpd::normalize(const vec3_t<T>& v) {
    return v / v.length();
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr float tpd::dot(const vec3_t<T>& lhs, const vec3_t<T>& rhs) {
    return static_cast<float>(lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec3_t<T> tpd::cross(const vec3_t<T>& lhs, const vec3_t<T>& rhs) {
    return tpd::vec3_t<T> {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

#pragma once

#include "torpedo/math/vec3.h"

namespace tpd {
    template<typename T> requires (std::is_trivially_copyable_v<T>)
    struct vec4_t {
        constexpr vec4_t() : data{ 0, 0, 0, 0 } {}
        constexpr vec4_t(const T x, const T y, const T z, const T w) : data{ x, y, z, w } {}

        template<typename R>
        constexpr vec4_t(const vec2_t<R>& v, T z, T w) : data{ static_cast<T>(v.x), static_cast<T>(v.y), z, w } {}
        constexpr vec4_t(const vec2_t<T>& v, const T z, const T w) : data{ v.x, v.y, z, w } {}

        template<typename R>
        constexpr vec4_t(const vec3_t<R>& v, T w);
        constexpr vec4_t(const vec3_t<T>& v, const T w) : data{ v.x, v.y, v.z, w } {}

        template<typename R>
        constexpr explicit vec4_t(const vec4_t<R>& other);
        constexpr vec4_t(const vec4_t& other) : data{ other.x, other.y, other.z, other.w } {}

        template<typename R>
        vec4_t& operator=(const vec4_t<R>& other);
        constexpr vec4_t& operator=(const vec4_t&) noexcept = default;

        [[nodiscard]] constexpr float length() const noexcept;
        explicit operator bool() const noexcept { return x != 0.0 || y != 0.0 || z != 0.0 || w != 0.0; }

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

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr vec4_t<T> normalize(const vec4_t<T>& v);

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr float dot(const vec4_t<T>& lhs, const vec4_t<T>& rhs);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
constexpr tpd::vec4_t<T>::vec4_t(const vec3_t<R>& v, const T w)
    : data{ static_cast<T>(v.x), static_cast<T>(v.y), static_cast<T>(v.z), w } {}

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
constexpr tpd::vec4_t<T>::vec4_t(const vec4_t<R>& other)
    : data{ static_cast<T>(other.x), static_cast<T>(other.y), static_cast<T>(other.z), static_cast<T>(other.w) } {}

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
tpd::vec4_t<T>& tpd::vec4_t<T>::operator=(const vec4_t<R>& other) {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    z = static_cast<T>(other.z);
    w = static_cast<T>(other.w);
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr float tpd::vec4_t<T>::length() const noexcept {
    return static_cast<float>(std::sqrt(x * x + y * y + z * z + w * w));
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator+=(const vec4_t& v) noexcept {
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator-=(const vec4_t& v) noexcept {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator*=(const vec4_t& v) noexcept {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    w *= v.w;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    z += scalar;
    w += scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    z -= scalar;
    w -= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec4_t<T>& tpd::vec4_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
    } else {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
    }
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator==(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator!=(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator+(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator-(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator-(const tpd::vec4_t<T>& v) {
    return tpd::vec3_t<T>{ -v.x, -v.y, -v.z, -v.w };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator*(const tpd::vec4_t<T>& v, T scalar) {
    return tpd::vec3_t<T>{ v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator*(const tpd::vec4_t<T>& lhs, const tpd::vec4_t<T>& rhs) {
    return tpd::vec3_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> operator/(const tpd::vec4_t<T>& v, T scalar) {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        return v * inv;
    }
    return tpd::vec4_t<T>{ v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec4_t<T> tpd::normalize(const vec4_t<T>& v) {
    return v / v.length();
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr float tpd::dot(const vec4_t<T>& lhs, const vec4_t<T>& rhs) {
    return static_cast<float>(lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w);
}

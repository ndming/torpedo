#pragma once

#include <type_traits>

namespace tpd {
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct vec2_t {
        constexpr vec2_t() noexcept : data{ 0, 0 } {}
        constexpr vec2_t(const T x, const T y) noexcept : data{ x, y } {}

        template<typename R>
        constexpr explicit vec2_t(const vec2_t<R>& other) noexcept;
        constexpr vec2_t(const vec2_t& other) noexcept : data{ other.x, other.y } {}

        template<typename R>
        constexpr vec2_t& operator=(const vec2_t<R>& other) noexcept;
        constexpr vec2_t& operator=(const vec2_t&) noexcept = default;

        T& operator[](const std::size_t i) noexcept { return data[i]; }
        T operator[](const std::size_t i) const noexcept { return data[i]; }

        vec2_t& operator+=(const vec2_t& vec) noexcept;
        vec2_t& operator-=(const vec2_t& vec) noexcept;
        vec2_t& operator*=(const vec2_t& vec) noexcept;

        vec2_t& operator+=(T scalar) noexcept;
        vec2_t& operator-=(T scalar) noexcept;
        vec2_t& operator*=(T scalar) noexcept;
        vec2_t& operator/=(T scalar) noexcept;

        union {
            T data[2];

            struct {
                T x;
                T y;
            };
        };
    };

    using vec2 = vec2_t<float>;
    using dvec2 = vec2_t<double>;
    using uvec2 = vec2_t<uint32_t>;
    using ivec2 = vec2_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T>
    constexpr float dot(const vec2_t<T>& v0, const vec2_t<T>& v1) noexcept;

    template<typename T>
    constexpr float norm(const vec2_t<T>& vec) noexcept;

    template<typename T>
    constexpr vec2_t<T> normalize(const vec2_t<T>& vec) noexcept;
} // namespace tpd::math

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec2_t<T>::vec2_t(const vec2_t<R>& other) noexcept
    : data{ static_cast<T>(other.x), static_cast<T>(other.y) } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::vec2_t<T>& tpd::vec2_t<T>::operator=(const vec2_t<R>& other) noexcept {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator+=(const vec2_t& vec) noexcept {
    x += vec.x;
    y += vec.y;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator-=(const vec2_t& vec) noexcept {
    x -= vec.x;
    y -= vec.y;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator*=(const vec2_t& vec) noexcept {
    x *= vec.x;
    y *= vec.y;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        // Let IEEE 754 handles inf and NaN
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
    } else {
        if (scalar == 0) [[unlikely]] {
            using limits = std::numeric_limits<T>;
            x = x == 0 ? limits::quiet_NaN() : x > 0 ? limits::max() : limits::min();
            y = y == 0 ? limits::quiet_NaN() : y > 0 ? limits::max() : limits::min();
        }
        else {
            x /= scalar;
            y /= scalar;
        }
    }
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template<typename T>
constexpr bool operator!=(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::vec2_t<T> operator+(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return tpd::vec2_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y };
}

template<typename T>
constexpr tpd::vec2_t<T> operator-(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return tpd::vec2_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y };
}

template<typename T>
constexpr tpd::vec2_t<T> operator-(const tpd::vec2_t<T>& vec) noexcept {
    return tpd::vec2_t<T>{ -vec.x, -vec.y };
}

template<typename T>
constexpr tpd::vec2_t<T> operator*(const tpd::vec2_t<T>& vec, const T scalar) noexcept {
    return tpd::vec2_t<T>{ vec.x * scalar, vec.y * scalar };
}

template<typename T>
constexpr tpd::vec2_t<T> operator*(const T scalar, const tpd::vec2_t<T>& vec) noexcept {
    return vec * scalar;
}

template<typename T>
constexpr tpd::vec2_t<T> operator*(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return tpd::vec2_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y };
}

template<typename T>
constexpr tpd::vec2_t<T> operator/(const tpd::vec2_t<T>& vec, const T scalar) {
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
            return tpd::vec2_t<T>{ x, y };
        }
        // Normal division for non-zero scalar
        return tpd::vec2_t<T>{ vec.x / scalar, vec.y / scalar };
    }
}

template<typename T>
constexpr float tpd::math::dot(const vec2_t<T>& v0, const vec2_t<T>& v1) noexcept {
    return static_cast<float>(v0.x * v1.x + v0.y * v1.y);
}

template<typename T>
constexpr float tpd::math::norm(const vec2_t<T>& vec) noexcept {
    return std::sqrtf(dot(vec, vec));
}

template<typename T>
constexpr tpd::vec2_t<T> tpd::math::normalize(const vec2_t<T>& vec) noexcept {
    return vec / static_cast<T>(norm(vec));
}

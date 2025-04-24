#pragma once

#include <cmath>
#include <type_traits>

namespace tpd {
    template<typename T> requires (std::is_trivially_copyable_v<T>)
    struct vec2_t {
        constexpr vec2_t() : data{ 0, 0 } {}
        constexpr vec2_t(const T x, const T y) : data{ x, y } {}

        template<typename R>
        constexpr explicit vec2_t(const vec2_t<R>& other) : data{ static_cast<T>(other.x), static_cast<T>(other.y) } {}
        constexpr vec2_t(const vec2_t& other) : data{ other.x, other.y } {}

        template<typename R>
        vec2_t& operator=(const vec2_t<R>& other);
        constexpr vec2_t& operator=(const vec2_t&) noexcept = default;

        [[nodiscard]] constexpr float length() const noexcept { return static_cast<float>(std::sqrt(x * x + y * y)); }
        explicit operator bool() const noexcept { return x != 0.0 || y != 0.0; }

        T& operator[](const std::size_t i) noexcept { return data[i]; }
        T operator[](const std::size_t i) const noexcept { return data[i]; }

        vec2_t& operator+=(const vec2_t& v) noexcept;
        vec2_t& operator-=(const vec2_t& v) noexcept;
        vec2_t& operator*=(const vec2_t& v) noexcept;

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

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr vec2_t<T> normalize(const vec2_t<T>& v);

    template<typename T> requires (std::is_trivially_copyable_v<T>)
    constexpr float dot(const vec2_t<T>& lhs, const vec2_t<T>& rhs);
} // namespace tpd

template<typename T> requires (std::is_trivially_copyable_v<T>)
template<typename R>
tpd::vec2_t<T>& tpd::vec2_t<T>::operator=(const vec2_t<R>& other) {
    x = static_cast<T>(other.x);
    y = static_cast<T>(other.y);
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator+=(const vec2_t& v) noexcept {
    x += v.x;
    y += v.y;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator-=(const vec2_t& v) noexcept {
    x -= v.x;
    y -= v.y;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator*=(const vec2_t& v) noexcept {
    x *= v.x;
    y *= v.y;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator+=(const T scalar) noexcept {
    x += scalar;
    y += scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator-=(const T scalar) noexcept {
    x -= scalar;
    y -= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
tpd::vec2_t<T>& tpd::vec2_t<T>::operator/=(const T scalar) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        x *= inv;
        y *= inv;
    } else {
        x /= scalar;
        y /= scalar;
    }
    return *this;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator==(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr bool operator!=(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator+(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) {
    return tpd::vec2_t<T>{ lhs.x + rhs.x, lhs.y + rhs.y };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator-(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) {
    return tpd::vec2_t<T>{ lhs.x - rhs.x, lhs.y - rhs.y };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator-(const tpd::vec2_t<T>& v) {
    return tpd::vec2_t<T>{ -v.x, -v.y };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator*(const tpd::vec2_t<T>& v, T scalar) {
    return tpd::vec2_t<T>{ v.x * scalar, v.y * scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator*(const tpd::vec2_t<T>& lhs, const tpd::vec2_t<T>& rhs) {
    return tpd::vec2_t<T>{ lhs.x * rhs.x, lhs.y * rhs.y };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> operator/(const tpd::vec2_t<T>& v, T scalar) {
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1.0) / scalar;
        return v * inv;
    }
    return tpd::vec2_t<T>{ v.x / scalar, v.y / scalar };
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr tpd::vec2_t<T> tpd::normalize(const vec2_t<T>& v) {
    return v / v.length();
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
constexpr float tpd::dot(const vec2_t<T>& lhs, const vec2_t<T>& rhs) {
    return static_cast<float>(lhs.x * rhs.x + lhs.y * rhs.y);
}

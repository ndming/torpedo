#pragma once

#include "torpedo/math/mat3.h"
#include "torpedo/math/vec4.h"

namespace tpd {
    /**
     * Represents a 4x4 row-major matrix. Each index `i` into the matrix results in a `vec4_t<T>` vector at row `i`.
     *
     * @tparam T Numeric type of the matrix elements, must be an arithmetic type.
     */
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct mat4_t {
        constexpr explicit mat4_t(T val = 0) noexcept;
        constexpr explicit mat4_t(const mat3_t<T>& mat, const vec3_t<T>& vec, T r3c3 = 1) noexcept;
        constexpr mat4_t(const vec4_t<T>& row0, const vec4_t<T>& row1, const vec4_t<T>& row2, const vec4_t<T>& row3) noexcept;
        constexpr mat4_t(
            T r0c0, T r0c1, T r0c2, T r0c3,
            T r1c0, T r1c1, T r1c2, T r1c3,
            T r2c0, T r2c1, T r2c2, T r2c3,
            T r3c0, T r3c1, T r3c2, T r3c3) noexcept;

        template<typename R>
        constexpr mat4_t& operator=(const mat4_t<R>& other) noexcept;
        constexpr mat4_t& operator=(const mat4_t& other) noexcept = default;

        template<typename R>
        constexpr explicit mat4_t(const mat4_t<R>& other) noexcept : data{ other[0], other[1], other[2], other[3] } {}
        constexpr mat4_t(const mat4_t& other) noexcept = default;

        vec4_t<T>& operator[](const std::size_t i) noexcept { return data[i]; }
        const vec4_t<T>& operator[](const std::size_t i) const noexcept { return data[i]; }
        vec4_t<T> col(const std::size_t i) const noexcept { return vec4_t<T>{ data[0][i], data[1][i], data[2][i], data[3][i] }; };

        T& operator[](const std::size_t row, const std::size_t col) noexcept { return data[row][col]; }
        T operator[](const std::size_t row, const std::size_t col) const noexcept { return data[row][col]; }

        constexpr explicit operator mat3_t<T>() const noexcept;

        mat4_t& operator+=(const mat4_t& mat) noexcept;
        mat4_t& operator-=(const mat4_t& mat) noexcept;
        mat4_t& operator*=(const mat4_t& mat) noexcept;

        mat4_t& operator+=(T scalar) noexcept;
        mat4_t& operator-=(T scalar) noexcept;
        mat4_t& operator*=(T scalar) noexcept;
        mat4_t& operator/=(T scalar) noexcept;

        vec4_t<T> data[4];
    };

    using mat4 = mat4_t<float>;
    using dmat4 = mat4_t<double>;
    using umat4 = mat4_t<uint32_t>;
    using imat4 = mat4_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr T det(const mat4_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat4_t<T> inv(const mat4_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat4_t<T> mul(const mat4_t<T>& lhs, const mat4_t<T>& rhs) noexcept;

    template<typename T>
    constexpr mat4_t<T> transpose(const mat4_t<T>& mat) noexcept;
} // namespace tpd::math

namespace tpd::utils {
    template<typename T>
    [[nodiscard]] std::string toString(const mat4_t<T>& mat);
} // namespace tpd::utils

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat4_t<T>::mat4_t(const T val) noexcept
    : data{ { val, 0, 0, 0 }, { 0, val, 0, 0 }, { 0, 0, val, 0 }, { 0, 0, 0, val } } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat4_t<T>::mat4_t(const mat3_t<T>& mat, const vec3_t<T>& vec, const T r3c3) noexcept
    : mat4_t{ { mat[0], vec[0] }, { mat[1], vec[1] }, { mat[2], vec[2] }, { 0, 0, 0, r3c3 } } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat4_t<T>::mat4_t(const vec4_t<T>& row0, const vec4_t<T>& row1, const vec4_t<T>& row2, const vec4_t<T>& row3) noexcept
    : data{ row0, row1, row2, row3 } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat4_t<T>::mat4_t(
    const T r0c0, const T r0c1, const T r0c2, const T r0c3,
    const T r1c0, const T r1c1, const T r1c2, const T r1c3,
    const T r2c0, const T r2c1, const T r2c2, const T r2c3,
    const T r3c0, const T r3c1, const T r3c2, const T r3c3) noexcept
    : data{
        { r0c0, r0c1, r0c2, r0c3 },
        { r1c0, r1c1, r1c2, r1c3 },
        { r2c0, r2c1, r2c2, r2c3 },
        { r3c0, r3c1, r3c2, r3c3 } }
{}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::mat4_t<T>& tpd::mat4_t<T>::operator=(const mat4_t<R>& other) noexcept {
    data[0] = other[0];
    data[1] = other[1];
    data[2] = other[2];
    data[3] = other[3];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat4_t<T>::operator mat3_t<T>() const noexcept {
    return mat3_t<T>{
        data[0][0], data[0][1], data[0][2],
        data[1][0], data[1][1], data[1][2],
        data[2][0], data[2][1], data[2][2],
    };
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator+=(const mat4_t& mat) noexcept {
    data[0] += mat[0];
    data[1] += mat[1];
    data[2] += mat[2];
    data[3] += mat[3];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator-=(const mat4_t& mat) noexcept {
    data[0] -= mat[0];
    data[1] -= mat[1];
    data[2] -= mat[2];
    data[3] -= mat[3];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator*=(const mat4_t& mat) noexcept {
    data[0] *= mat[0];
    data[1] *= mat[1];
    data[2] *= mat[2];
    data[3] *= mat[3];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator+=(const T scalar) noexcept {
    data[0] += scalar;
    data[1] += scalar;
    data[2] += scalar;
    data[3] += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator-=(const T scalar) noexcept {
    data[0] -= scalar;
    data[1] -= scalar;
    data[2] -= scalar;
    data[3] -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator*=(const T scalar) noexcept {
    data[0] *= scalar;
    data[1] *= scalar;
    data[2] *= scalar;
    data[3] *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat4_t<T>& tpd::mat4_t<T>::operator/=(const T scalar) noexcept {
    data[0] /= scalar;
    data[1] /= scalar;
    data[2] /= scalar;
    data[3] /= scalar;
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::mat4_t<T>& lhs, const tpd::mat4_t<T>& rhs) noexcept {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2] && lhs[3] == rhs[3];
}

template<typename T>
constexpr bool operator!=(const tpd::mat4_t<T>& lhs, const tpd::mat4_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::mat4_t<T> operator+(const tpd::mat4_t<T>& lhs, const tpd::mat4_t<T>& rhs) noexcept {
    return tpd::mat4_t<T>{ lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2], lhs[3] + rhs[3] };
}

template<typename T>
constexpr tpd::mat4_t<T> operator-(const tpd::mat4_t<T>& lhs, const tpd::mat4_t<T>& rhs) noexcept {
    return tpd::mat4_t<T>{ lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2], lhs[3] - rhs[3] };
}

template<typename T>
constexpr tpd::mat4_t<T> operator-(const tpd::mat4_t<T>& mat) noexcept {
    return tpd::mat4_t<T>{ -mat[0], -mat[1], -mat[2], -mat[3] };
}

template<typename T>
constexpr tpd::mat4_t<T> operator*(const tpd::mat4_t<T>& lhs, const tpd::mat4_t<T>& rhs) noexcept {
    return tpd::mat4_t<T>{ lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2], lhs[3] * rhs[3] };
}

template<typename T>
constexpr tpd::mat4_t<T> operator*(const tpd::mat4_t<T>& mat, const T scalar) noexcept {
    return tpd::mat4_t<T>{ mat[0] * scalar, mat[1] * scalar, mat[2] * scalar, mat[3] * scalar };
}

template<typename T>
constexpr tpd::mat4_t<T> operator*(const T scalar, const tpd::mat4_t<T>& mat) noexcept {
    return mat * scalar;
}

template<typename T>
constexpr tpd::mat4_t<T> operator/(const tpd::mat4_t<T>& mat, const T scalar) {
    return tpd::mat4_t<T>{ mat[0] / scalar, mat[1] / scalar, mat[2] / scalar, mat[3] / scalar };
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr T tpd::math::det(const mat4_t<T>& mat) noexcept {
    // TODO: compute the determinant of mat4
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat4_t<T> tpd::math::inv(const mat4_t<T>& mat) noexcept {
    // TODO: compute the inverse of mat4
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat4_t<T> tpd::math::mul(const mat4_t<T>& lhs, const mat4_t<T>& rhs) noexcept {
    return mat4_t<T>{
        dot(lhs[0], rhs.col(0)), dot(lhs[0], rhs.col(1)), dot(lhs[0], rhs.col(2)), dot(lhs[0], rhs.col(3)),
        dot(lhs[1], rhs.col(0)), dot(lhs[1], rhs.col(1)), dot(lhs[1], rhs.col(2)), dot(lhs[1], rhs.col(3)),
        dot(lhs[2], rhs.col(0)), dot(lhs[2], rhs.col(1)), dot(lhs[2], rhs.col(2)), dot(lhs[2], rhs.col(3)),
        dot(lhs[3], rhs.col(0)), dot(lhs[3], rhs.col(1)), dot(lhs[3], rhs.col(2)), dot(lhs[3], rhs.col(3)),
    };
}

template<typename T>
constexpr tpd::mat4_t<T> tpd::math::transpose(const mat4_t<T>& mat) noexcept {
    return mat4_t<T> {
        mat[0, 0], mat[1, 0], mat[2, 0], mat[3, 0],
        mat[0, 1], mat[1, 1], mat[2, 1], mat[3, 1],
        mat[0, 2], mat[1, 2], mat[2, 2], mat[3, 2],
        mat[0, 3], mat[1, 3], mat[2, 3], mat[3, 3],
    };
}

template<typename T>
std::string tpd::utils::toString(const mat4_t<T>& mat) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    oss << "[";
    for (std::size_t row = 0; row < 4; ++row) {
        oss << "[";
        for (std::size_t col = 0; col < 4; ++col) {
            oss << mat[row, col];
            if (col != 3) {
                oss << ", ";
            }
        }
        oss << "]";
        if (row != 3) {
            oss << ", ";
        }
        else {
            oss << ", type=" << typeid(T).name();
        }
    }
    oss << "]";

    return oss.str();
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const tpd::mat4_t<T>& mat) {
    os << tpd::utils::toString(mat);
    return os;
}

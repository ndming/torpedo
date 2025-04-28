#pragma once

#include "torpedo/math/mat2.h"
#include "torpedo/math/vec3.h"

namespace tpd {
    /**
     * Represents a 3x3 row-major matrix. Each index `i` into the matrix via the subscript operator
     * results in a `vec3_t<T>` vector at row `i`, while the `col(i)` method returns a `vec3_t<T>`
     * containing values at column `i`.
     *
     * @tparam T Numeric type of the matrix elements, must be an arithmetic type.
     */
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct mat3_t {
        constexpr mat3_t() noexcept = default;
        constexpr explicit mat3_t(const T val) noexcept : data{ { val, 0, 0 }, { 0, val, 0 }, { 0, 0, val } } {}
        constexpr mat3_t(const mat2_t<T>& mat, const vec2_t<T>& vec, T r2c2 = 0) noexcept;
        constexpr mat3_t(T r0c0, T r0c1, T r0c2, T r1c0, T r1c1, T r1c2, T r2c0, T r2c1, T r2c2) noexcept;
        constexpr mat3_t(const vec3_t<T>& row0, const vec3_t<T>& row1, const vec3_t<T>& row2) noexcept;

        template<typename R>
        constexpr mat3_t& operator=(const mat3_t<R>& other) noexcept;
        constexpr mat3_t& operator=(const mat3_t& other) noexcept = default;

        template<typename R>
        constexpr explicit mat3_t(const mat3_t<R>& other) noexcept : data{ other[0], other[1], other[2] } {}
        constexpr mat3_t(const mat3_t& other) noexcept = default;

        vec3_t<T>& operator[](const std::size_t i) noexcept { return data[i]; }
        const vec3_t<T>& operator[](const std::size_t i) const noexcept { return data[i]; }
        vec3_t<T> col(const std::size_t i) const noexcept { return vec3_t<T>{ data[0][i], data[1][i], data[2][i] }; };

        T& operator[](const std::size_t row, const std::size_t col) noexcept { return data[row][col]; }
        T operator[](const std::size_t row, const std::size_t col) const noexcept { return data[row][col]; }

        constexpr explicit operator mat2_t<T>() const noexcept;

        mat3_t& operator+=(const mat3_t& mat) noexcept;
        mat3_t& operator-=(const mat3_t& mat) noexcept;
        mat3_t& operator*=(const mat3_t& mat) noexcept;

        mat3_t& operator+=(T scalar) noexcept;
        mat3_t& operator-=(T scalar) noexcept;
        mat3_t& operator*=(T scalar) noexcept;
        mat3_t& operator/=(T scalar) noexcept;

        vec3_t<T> data[3];
    };

    using mat3 = mat3_t<float>;
    using dmat3 = mat3_t<double>;
    using umat3 = mat3_t<uint32_t>;
    using imat3 = mat3_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr T det(const mat3_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat3_t<T> inv(const mat3_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat3_t<T> mul(const mat3_t<T>& lhs, const mat3_t<T>& rhs) noexcept;

    template<typename T>
    constexpr mat3_t<T> transpose(const mat3_t<T>& mat) noexcept;
} // namespace tpd::math

namespace tpd::utils {
    template<typename T>
    [[nodiscard]] std::string toString(const mat3_t<T>& mat);
} // namespace tpd::utils

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat3_t<T>::mat3_t(const mat2_t<T>& mat, const vec2_t<T>& vec, const T r2c2) noexcept
    : mat3_t{ { mat[0], vec[0] }, { mat[1], vec[1] }, { 0, 0, r2c2 } } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat3_t<T>::mat3_t(const vec3_t<T>& row0, const vec3_t<T>& row1, const vec3_t<T>& row2) noexcept
    : data{ row0, row1, row2 } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat3_t<T>::mat3_t(
    const T r0c0, const T r0c1, const T r0c2,
    const T r1c0, const T r1c1, const T r1c2,
    const T r2c0, const T r2c1, const T r2c2) noexcept
    : data{ { r0c0, r0c1, r0c2 }, { r1c0, r1c1, r1c2 }, { r2c0, r2c1, r2c2 } } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::mat3_t<T>& tpd::mat3_t<T>::operator=(const mat3_t<R>& other) noexcept {
    data[0] = other[0];
    data[1] = other[1];
    data[2] = other[2];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat3_t<T>::operator mat2_t<T>() const noexcept {
    return mat2_t<T>{
        data[0][0], data[0][1],
        data[1][0], data[1][1],
    };
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator+=(const mat3_t& mat) noexcept {
    data[0] += mat[0];
    data[1] += mat[1];
    data[2] += mat[2];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator-=(const mat3_t& mat) noexcept {
    data[0] -= mat[0];
    data[1] -= mat[1];
    data[2] -= mat[2];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator*=(const mat3_t& mat) noexcept {
    data[0] *= mat[0];
    data[1] *= mat[1];
    data[2] *= mat[2];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator+=(const T scalar) noexcept {
    data[0] += scalar;
    data[1] += scalar;
    data[2] += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator-=(const T scalar) noexcept {
    data[0] -= scalar;
    data[1] -= scalar;
    data[2] -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator*=(const T scalar) noexcept {
    data[0] *= scalar;
    data[1] *= scalar;
    data[2] *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat3_t<T>& tpd::mat3_t<T>::operator/=(const T scalar) noexcept {
    data[0] /= scalar;
    data[1] /= scalar;
    data[2] /= scalar;
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::mat3_t<T>& lhs, const tpd::mat3_t<T>& rhs) noexcept {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];
}

template<typename T>
constexpr bool operator!=(const tpd::mat3_t<T>& lhs, const tpd::mat3_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::mat3_t<T> operator+(const tpd::mat3_t<T>& lhs, const tpd::mat3_t<T>& rhs) noexcept {
    return tpd::mat3_t<T>{ lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2] };
}

template<typename T>
constexpr tpd::mat3_t<T> operator-(const tpd::mat3_t<T>& lhs, const tpd::mat3_t<T>& rhs) noexcept {
    return tpd::mat3_t<T>{ lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2] };
}

template<typename T>
constexpr tpd::mat3_t<T> operator-(const tpd::mat3_t<T>& mat) noexcept {
    return tpd::mat3_t<T>{ -mat[0], -mat[1], -mat[2] };
}

template<typename T>
constexpr tpd::mat3_t<T> operator*(const tpd::mat3_t<T>& lhs, const tpd::mat3_t<T>& rhs) noexcept {
    return tpd::mat3_t<T>{ lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2] };
}

template<typename T>
constexpr tpd::mat3_t<T> operator*(const tpd::mat3_t<T>& mat, const T scalar) noexcept {
    return tpd::mat3_t<T>{ mat[0] * scalar, mat[1] * scalar, mat[2] * scalar };
}

template<typename T>
constexpr tpd::mat3_t<T> operator*(const T scalar, const tpd::mat3_t<T>& mat) noexcept {
    return mat * scalar;
}

template<typename T>
constexpr tpd::mat3_t<T> operator/(const tpd::mat3_t<T>& mat, const T scalar) {
    return tpd::mat3_t<T>{ mat[0] / scalar, mat[1] / scalar, mat[2] / scalar };
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr T tpd::math::det(const mat3_t<T>& mat) noexcept {
    const T minor00 = dop(mat[1, 1], mat[2, 2], mat[2, 1], mat[1, 2]);
    const T minor01 = dop(mat[1, 0], mat[2, 2], mat[2, 0], mat[1, 2]);
    const T minor02 = dop(mat[1, 0], mat[2, 1], mat[2, 0], mat[1, 1]);
    return std::fma(mat[0, 2], minor02, dop(mat[0, 0], minor00, mat[0, 1], minor01));
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat3_t<T> tpd::math::inv(const mat3_t<T>& mat) noexcept {
    const T minor00 = dop(mat[1, 1], mat[2, 2], mat[2, 1], mat[1, 2]);
    const T minor01 = dop(mat[1, 0], mat[2, 2], mat[2, 0], mat[1, 2]);
    const T minor02 = dop(mat[1, 0], mat[2, 1], mat[2, 0], mat[1, 1]);

    const T minor10 = dop(mat[0, 1], mat[2, 2], mat[2, 1], mat[0, 2]);
    const T minor11 = dop(mat[0, 0], mat[2, 2], mat[2, 0], mat[0, 2]);
    const T minor12 = dop(mat[0, 0], mat[2, 1], mat[2, 0], mat[0, 1]);

    const T minor20 = dop(mat[0, 1], mat[1, 2], mat[1, 1], mat[0, 2]);
    const T minor21 = dop(mat[0, 0], mat[1, 2], mat[1, 0], mat[0, 2]);
    const T minor22 = dop(mat[0, 0], mat[1, 1], mat[1, 0], mat[0, 1]);

    const T det = std::fma(mat[0, 2], minor02, dop(mat[0, 0], minor00, mat[0, 1], minor01));
    return tpd::mat3_t<T>{
        +minor00, -minor10, +minor20,
        -minor01, +minor11, -minor21,
        +minor02, -minor12, +minor22,
    } / det;
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat3_t<T> tpd::math::mul(const mat3_t<T>& lhs, const mat3_t<T>& rhs) noexcept {
    return mat3_t<T>{
        dot(lhs[0], rhs.col(0)), dot(lhs[0], rhs.col(1)), dot(lhs[0], rhs.col(2)),
        dot(lhs[1], rhs.col(0)), dot(lhs[1], rhs.col(1)), dot(lhs[1], rhs.col(2)),
        dot(lhs[2], rhs.col(0)), dot(lhs[2], rhs.col(1)), dot(lhs[2], rhs.col(2)),
    };
}

template<typename T>
constexpr tpd::mat3_t<T> tpd::math::transpose(const mat3_t<T>& mat) noexcept {
    return mat3_t<T> {
        mat[0, 0], mat[1, 0], mat[2, 0],
        mat[0, 1], mat[1, 1], mat[2, 1],
        mat[0, 2], mat[1, 2], mat[2, 2],
    };
}

template<typename T>
std::string tpd::utils::toString(const mat3_t<T>& mat) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    oss << "[";
    for (std::size_t row = 0; row < 3; ++row) {
        oss << "[";
        for (std::size_t col = 0; col < 3; ++col) {
            oss << mat[row, col];
            if (col != 2) {
                oss << ", ";
            }
        }
        oss << "]";
        if (row != 2) {
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
std::ostream& operator<<(std::ostream& os, const tpd::mat3_t<T>& mat) {
    os << tpd::utils::toString(mat);
    return os;
}
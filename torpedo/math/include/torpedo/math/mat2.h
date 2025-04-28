#pragma once

#include "torpedo/math/vec2.h"

#include <iomanip>
#include <sstream>

namespace tpd {
    /**
     * Represents a 2x2 row-major matrix. Each index `i` into the matrix via the subscript operator
     * results in a `vec2_t<T>` vector at row `i`, while the `col(i)` method returns a `vec2_t<T>`
     * containing values at column `i`.
     *
     * @tparam T Numeric type of the matrix elements, must be an arithmetic type.
     */
    template<typename T> requires (std::is_arithmetic_v<T>)
    struct mat2_t {
        constexpr explicit mat2_t(const T val = 0) noexcept : data{ { val, 0 }, { 0, val } } {}
        constexpr mat2_t(T r0c0, T r0c1, T r1c0, T r1c1) noexcept;
        constexpr mat2_t(const vec2_t<T>& row0, const vec2_t<T>& row1) noexcept;

        template<typename R>
        constexpr mat2_t& operator=(const mat2_t<R>& other) noexcept;
        constexpr mat2_t& operator=(const mat2_t& other) noexcept = default;

        template<typename R>
        constexpr explicit mat2_t(const mat2_t<R>& other) noexcept : data{ other[0], other[1] } {}
        constexpr mat2_t(const mat2_t& other) noexcept = default;

        vec2_t<T>& operator[](const std::size_t i) noexcept { return data[i]; }
        const vec2_t<T>& operator[](const std::size_t i) const noexcept { return data[i]; }
        vec2_t<T> col(const std::size_t i) const noexcept { return vec2_t<T>{ data[0][i], data[1][i] }; };

        T& operator[](const std::size_t row, const std::size_t col) noexcept { return data[row][col]; }
        T operator[](const std::size_t row, const std::size_t col) const noexcept { return data[row][col]; }

        mat2_t& operator+=(const mat2_t& mat) noexcept;
        mat2_t& operator-=(const mat2_t& mat) noexcept;
        mat2_t& operator*=(const mat2_t& mat) noexcept;

        mat2_t& operator+=(T scalar) noexcept;
        mat2_t& operator-=(T scalar) noexcept;
        mat2_t& operator*=(T scalar) noexcept;
        mat2_t& operator/=(T scalar) noexcept;

        vec2_t<T> data[2];
    };

    using mat2 = mat2_t<float>;
    using dmat2 = mat2_t<double>;
    using umat2 = mat2_t<uint32_t>;
    using imat2 = mat2_t<int32_t>;
} // namespace tpd

namespace tpd::math {
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr T det(const mat2_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat2_t<T> inv(const mat2_t<T>& mat) noexcept;

    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr mat2_t<T> mul(const mat2_t<T>& lhs, const mat2_t<T>& rhs) noexcept;

    template<typename T>
    constexpr mat2_t<T> transpose(const mat2_t<T>& mat) noexcept;
} // namespace tpd::math

namespace tpd::utils {
    template<typename T>
    [[nodiscard]] std::string toString(const mat2_t<T>& mat);
} // namespace tpd::utils

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat2_t<T>::mat2_t(const T r0c0, const T r0c1, const T r1c0, const T r1c1) noexcept
    : data{ { r0c0, r0c1 }, { r1c0, r1c1 } } {}

template<typename T> requires (std::is_arithmetic_v<T>)
constexpr tpd::mat2_t<T>::mat2_t(const vec2_t<T>& row0, const vec2_t<T>& row1) noexcept
    : data{ row0, row1 } {}

template<typename T> requires (std::is_arithmetic_v<T>)
template<typename R>
constexpr tpd::mat2_t<T>& tpd::mat2_t<T>::operator=(const mat2_t<R>& other) noexcept {
    data[0] = other[0];
    data[1] = other[1];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator+=(const mat2_t& mat) noexcept {
    data[0] += mat[0];
    data[1] += mat[1];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator-=(const mat2_t& mat) noexcept {
    data[0] -= mat[0];
    data[1] -= mat[1];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator*=(const mat2_t& mat) noexcept {
    data[0] *= mat[0];
    data[1] *= mat[1];
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator+=(const T scalar) noexcept {
    data[0] += scalar;
    data[1] += scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator-=(const T scalar) noexcept {
    data[0] -= scalar;
    data[1] -= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator*=(const T scalar) noexcept {
    data[0] *= scalar;
    data[1] *= scalar;
    return *this;
}

template<typename T> requires (std::is_arithmetic_v<T>)
tpd::mat2_t<T>& tpd::mat2_t<T>::operator/=(const T scalar) noexcept {
    data[0] /= scalar;
    data[1] /= scalar;
    return *this;
}

template<typename T>
constexpr bool operator==(const tpd::mat2_t<T>& lhs, const tpd::mat2_t<T>& rhs) noexcept {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}

template<typename T>
constexpr bool operator!=(const tpd::mat2_t<T>& lhs, const tpd::mat2_t<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template<typename T>
constexpr tpd::mat2_t<T> operator+(const tpd::mat2_t<T>& lhs, const tpd::mat2_t<T>& rhs) noexcept {
    return tpd::mat2_t<T>{ lhs[0] + rhs[0], lhs[1] + rhs[1] };
}

template<typename T>
constexpr tpd::mat2_t<T> operator-(const tpd::mat2_t<T>& lhs, const tpd::mat2_t<T>& rhs) noexcept {
    return tpd::mat2_t<T>{ lhs[0] - rhs[0], lhs[1] - rhs[1] };
}

template<typename T>
constexpr tpd::mat2_t<T> operator-(const tpd::mat2_t<T>& mat) noexcept {
    return tpd::mat2_t<T>{ -mat[0], -mat[1] };
}

template<typename T>
constexpr tpd::mat2_t<T> operator*(const tpd::mat2_t<T>& lhs, const tpd::mat2_t<T>& rhs) noexcept {
    return tpd::mat2_t<T>{ lhs[0] * rhs[0], lhs[1] * rhs[1] };
}

template<typename T>
constexpr tpd::mat2_t<T> operator*(const tpd::mat2_t<T>& mat, const T scalar) noexcept {
    return tpd::mat2_t<T>{ mat[0] * scalar, mat[1] * scalar };
}

template<typename T>
constexpr tpd::mat2_t<T> operator*(const T scalar, const tpd::mat2_t<T>& mat) noexcept {
    return mat * scalar;
}

template<typename T>
constexpr tpd::mat2_t<T> operator/(const tpd::mat2_t<T>& mat, const T scalar) {
    return tpd::mat2_t<T>{ mat[0] / scalar, mat[1] / scalar };
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr T tpd::math::det(const mat2_t<T>& mat) noexcept {
    return dop(mat[0, 0], mat[1, 1], mat[1, 0], mat[0, 1]);
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat2_t<T> tpd::math::inv(const mat2_t<T>& mat) noexcept {
    return mat2_t<T>{ mat[1, 1], -mat[0, 2], -mat[1, 0], mat[0, 0] } / det(mat);
}

template<typename T> requires (std::is_floating_point_v<T>)
constexpr tpd::mat2_t<T> tpd::math::mul(const mat2_t<T>& lhs, const mat2_t<T>& rhs) noexcept {
    return mat2_t<T>{
        dot(lhs[0], rhs.col(0)), dot(lhs[0], rhs.col(1)),
        dot(lhs[1], rhs.col(0)), dot(lhs[1], rhs.col(1)),
    };
}

template<typename T>
constexpr tpd::mat2_t<T> tpd::math::transpose(const mat2_t<T>& mat) noexcept {
    return mat2_t<T> {
        mat[0, 0], mat[1, 0],
        mat[0, 1], mat[1, 1],
    };
}

template<typename T>
std::string tpd::utils::toString(const mat2_t<T>& mat) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    oss << "[";
    for (std::size_t row = 0; row < 2; ++row) {
        oss << "[";
        for (std::size_t col = 0; col < 2; ++col) {
            oss << mat[row, col];
            if (col != 1) {
                oss << ", ";
            }
        }
        oss << "]";
        if (row != 1) {
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
std::ostream& operator<<(std::ostream& os, const tpd::mat2_t<T>& mat) {
    os << tpd::utils::toString(mat);
    return os;
}

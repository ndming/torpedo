#pragma once

#include <cmath>
#include <utility>

namespace tpd::math {
    /**
     * Computes the compensated difference between a * b and c * d, or a * b - c * d.
     * @see https://pharr.org/matt/blog/2019/11/03/difference-of-floats#fn:cc
     */
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr T dop(const T a, const T b, const T c, const T d) noexcept {
        const T cd = c * d;
        const T err = std::fma(-c, d, cd);
        const T dop = std::fma(a, b, -cd);
        return dop + err;
    }

    /**
     * Computes the compensated summation between a and b.
     * @see https://doi.org/10.1016/j.cam.2022.114434
     */
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr std::pair<T, T> compensated_sum(const T a, const T b) noexcept {
        const T sum = a + b;
        const T z = sum - a;
        const T err = a - (sum - z) + (b - z);
        return { sum, err };
    }

    /**
     * Computes the compensated multiplication of a and b.
     * @see https://doi.org/10.1016/j.cam.2022.114434
     */
    template<typename T> requires (std::is_floating_point_v<T>)
    constexpr std::pair<T, T> compensated_mul(const T a, const T b) noexcept {
        const T mul = a * b;
        const T err = std::fma(a, b, -mul);
        return { mul, err };
    }
}

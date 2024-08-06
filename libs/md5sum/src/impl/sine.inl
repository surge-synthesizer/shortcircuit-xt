/// This is the mathematical calculation implementation of MD5. It is not an
/// efficient implementation and is only used for compile-time expansion
/// calculations. In addition, in terms of accuracy, it is only used to ensure
/// the MD5 T-table constants.

#pragma once

namespace md5::math {

constexpr double PI = 3.14159265358979323846264338327950;

constexpr double abs(const double x) {
    return x < 0 ? -x : x;
}

constexpr double fmod(const double x, const double y) {
    const auto tmp = static_cast<int64_t>(x / y);
    return x - y * static_cast<double>(tmp);
}

constexpr double pow(const double x, const int n) {
    double res = 1;
    for (int i = 0; i < n; ++i) {
        res *= x;
    }
    return res;
}

constexpr double factorial(const int n) {
    double res = 1;
    for (int i = 2 ; i <= n ; ++i) {
        res *= i;
    }
    return res;
}

/// Calculate sin(x) value with Maclaurin series.
constexpr double sin_core(const double x) {
    double res = x;
    for (int i = 1; i < 80; ++i) {
        const int n = i * 2 + 1;
        const int sign = i & 1 ? -1 : 1;
        res += sign * pow(x, n) / factorial(n);
    }
    return res;
}

/// Calculate the sin(x) value in radians.
constexpr double sin(double x) {
    x = fmod(x, 2 * PI); // -2PI < x < 2PI

    if (abs(x) > PI) {
        x -= (x > 0 ? 2 : -2) * PI; // -PI < x < PI
    }

    if (abs(x) > PI / 2) {
        x = (x > 0 ? 1 : -1) * PI - x; // -PI / 2 < x < PI / 2
    }
    return sin_core(x); // closer to 0 for better accuracy
}

} // namespace md5::math

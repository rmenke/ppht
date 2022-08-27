// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <array>
#include <cmath>
#include <iostream>
#include <utility>

namespace ppht {

/// A basic integral point.
struct point_t : std::array<long, 2> {
    using std::array<long, 2>::array;

    point_t() {
        std::get<0>(*this) = 0;
        std::get<1>(*this) = 0;
    }

    point_t(long a, long b)
        : std::array<long, 2>({{a, b}}) {}

    long length_squared() const {
        auto &x = std::get<0>(*this);
        auto &y = std::get<1>(*this);
        return x * x + y * y;
    }

    double length() const {
        return std::hypot(std::get<0>(*this), std::get<1>(*this));
    }

    point_t operator+(const point_t &p) const {
        return {std::get<0>(*this) + std::get<0>(p),
                std::get<1>(*this) + std::get<1>(p)};
    }

    point_t operator-(const point_t &p) const {
        return {std::get<0>(*this) - std::get<0>(p),
                std::get<1>(*this) - std::get<1>(p)};
    }

    point_t operator*(const point_t &p) const {
        return {std::get<0>(*this) * std::get<0>(p),
                std::get<1>(*this) * std::get<1>(p)};
    }

    point_t operator/(long d) const {
        return {std::get<0>(*this) / d, std::get<1>(*this) / d};
    }

    template <class P>
    auto dot(const P &p) const {
        return std::get<0>(*this) * std::get<0>(p) +
               std::get<1>(*this) * std::get<1>(p);
    }

    friend std::ostream &operator<<(std::ostream &o, const point_t &p) {
        return o << "(" << std::get<0>(p) << ", " << std::get<1>(p) << ")";
    }
};

/// @brief Equal-to operator for pairs of points.
///
/// Pairs of points represent primitive line segments, and should be
/// considered non-directional.
///
/// @param a a line segment
/// @param b a line segment
/// @return true if the endpoints are equal pairwise
inline bool operator==(const std::pair<point_t, point_t> &a,
                       const std::pair<point_t, point_t> &b) {
    return (a.first == b.first && a.second == b.second) ||
           (a.first == b.second && a.second == b.first);
}

/// @brief Not-equal-to operator for pairs of points.
///
/// Pairs of points represent primitive line segments, and should be
/// considered non-directional.
///
/// @param a a line segment
/// @param b a line segment
/// @return false if the endpoints are equal pairwise
inline bool operator!=(const std::pair<point_t, point_t> &a,
                       const std::pair<point_t, point_t> &b) {
    return !operator==(a, b);
}

/// The definition of a line in Hough space.
using line_t = std::pair<std::size_t, double>;

/// The status of a pixel in a @ref state map.
enum status_t {
    unset,   ///< Pixel is unset.
    pending, ///< Pixel is set but not yet voted.
    voted,   ///< Pixel is set and voted.
    done     ///< Pixel has been incorporated into
             ///  a segment.
};

} // namespace ppht

#endif /* ppht_types_hpp */

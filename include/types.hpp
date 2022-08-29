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
struct point {
    long x, y;

    point(long a, long b)
        : x(a)
        , y(b) {}

    point()
        : point(0, 0) {}

    long length_squared() const {
        return x * x + y * y;
    }

    double length() const {
        return std::hypot(x, y);
    }

    point operator+(const point &p) const {
        return {x + p.x, y + p.y};
    }

    point operator-(const point &p) const {
        return {x - p.x, y - p.y};
    }

    point operator*(const point &p) const {
        return {x * p.x, y * p.y};
    }

    point operator/(long d) const {
        return {x / d, y / d};
    }

    bool operator==(const point &p) const {
        return x == p.x && y == p.y;
    }

    bool operator!=(const point &p) const {
        return !operator==(p);
    }

    bool operator<(const point &p) const {
        if (x < p.x) return true;
        if (p.x < x) return false;
        return y < p.y;
    }

    auto dot(const std::pair<double, double> &p) const {
        using namespace std;
        return x * get<0>(p) + y * get<1>(p);
    }

    friend std::ostream &operator<<(std::ostream &o, const point &p) {
        return o << "(" << p.x << ", " << p.y << ")";
    }
};

template <std::size_t>
inline long &get(point &p);

template <>
inline long &get<0>(point &p) {
    return p.x;
}

template <>
inline long &get<1>(point &p) {
    return p.y;
}

template <std::size_t>
inline long get(const point &p);

template <>
inline long get<0>(const point &p) {
    return p.x;
}

template <>
inline long get<1>(const point &p) {
    return p.y;
}

/// @brief Equal-to operator for pairs of points.
///
/// Pairs of points represent primitive line segments, and should be
/// considered non-directional.
///
/// @param a a line segment
/// @param b a line segment
/// @return true if the endpoints are equal pairwise
inline bool operator==(const std::pair<point, point> &a,
                       const std::pair<point, point> &b) {
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
inline bool operator!=(const std::pair<point, point> &a,
                       const std::pair<point, point> &b) {
    return !operator==(a, b);
}

/// The definition of a line in Hough space.
using line_t = std::pair<std::size_t, double>;

/// The status of a pixel in a @ref state map.
enum status_t {
    unset,   ///< Pixel is unset.
    pending, ///< Pixel is set but not yet voted.
    voted,   ///< Pixel is set and voted.
    done     ///< Pixel has been incorporated into a segment.
};

} // namespace ppht

#endif /* ppht_types_hpp */

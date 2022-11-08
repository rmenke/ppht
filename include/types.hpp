// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <array>
#include <cmath>
#include <iostream>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility>

namespace ppht {

/// A basic integral point.
template <class T>
struct coord {
    static_assert(std::is_arithmetic_v<T>, "type must be arithmetic");
    static_assert(std::is_signed_v<T>, "type must be signed");

    T x, y;

    template <class U1, class U2>
    constexpr coord(U1 &&a, U2 &&b)
        : x(std::forward<U1>(a))
        , y(std::forward<U2>(b)) {}

    template <class U1, class U2>
    constexpr coord(const std::pair<U1, U2> &pair)
        : x(pair.first)
        , y(pair.second) {}

    constexpr coord()
        : coord(T{}, T{}) {}

    template <class U>
    constexpr coord(const coord<U> &r)
        : x(r.x)
        , y(r.y) {}

    template <class U>
    constexpr coord(coord<U> &&r)
        : x(std::move(r.x))
        , y(std::move(r.y)) {}

    constexpr auto length_squared() const {
        return x * x + y * y;
    }

    double length() const {
        return std::hypot(x, y);
    }

    template <class U>
    constexpr auto operator+(const coord<U> &p) const {
        using V = std::common_type_t<T, U>;
        return coord<V>{x + p.x, y + p.y};
    }

    template <class U>
    constexpr auto operator-(const coord<U> &p) const {
        using V = std::common_type_t<T, U>;
        return coord<V>{x - p.x, y - p.y};
    }

    template <class U>
    constexpr auto operator*(const coord<U> &p) const {
        using V = std::common_type_t<T, U>;
        return coord<V>{x * p.x, y * p.y};
    }

    template <class U, class V = typename std::common_type<T, U>::type>
    constexpr auto operator*(U d) const {
        return coord<V>{x * d, y * d};
    }

    template <class U>
    constexpr auto operator/(const coord<U> &p) const {
        using V = std::common_type_t<T, U>;
        return coord<V>{x / p.x, y / p.y};
    }

    template <class U, class V = typename std::common_type<T, U>::type>
    constexpr auto operator/(U d) const {
        return coord<V>{x / d, y / d};
    }

    template <class U>
    constexpr bool operator==(const coord<U> &p) const {
        return x == p.x && y == p.y;
    }

    template <class U>
    constexpr bool operator!=(const coord<U> &p) const {
        return !operator==(p);
    }

    template <class U>
    constexpr bool operator<(const coord<U> &p) const {
        if (x < p.x) return true;
        if (p.x < x) return false;
        return y < p.y;
    }

    template <class U>
    constexpr auto dot(const coord<U> &p) const {
        auto q = operator*(p);
        return q.x + q.y;
    }

    auto dot(const std::pair<double, double> &p) const {
        using namespace std;
        return x * get<0>(p) + y * get<1>(p);
    }

    friend std::ostream &operator<<(std::ostream &o, const coord &p) {
        return o << "(" << p.x << ", " << p.y << ")";
    }

    friend std::string to_string(const coord &p) {
        std::ostringstream os;
        os << p;
        return std::move(os).str();
    }
};

using point = coord<long>;

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

/// A segment is an unordered pair of points.
class segment : public std::pair<point, point> {
  public:
    segment() = default;

    segment(const point &a, const point &b)
        : std::pair<point, point>(a, b) {}

    segment(const segment &) = default;
    segment(segment &&) = default;

    segment &operator=(const segment &) = default;
    segment &operator=(segment &&) = default;

    constexpr bool operator==(const segment &rhs) const {
        return (first == rhs.first && second == rhs.second) ||
               (first == rhs.second && second == rhs.first);
    }
    constexpr bool operator!=(const segment &rhs) const {
        return !operator==(rhs);
    }

    friend std::ostream &operator<<(std::ostream &os, const segment &s) {
        return os << s.first << "--" << s.second;
    }
};

/// The definition of a line in Hough space.
struct line {
    std::size_t theta;
    double rho;

    line(std::size_t theta, double rho)
        : theta(theta)
        , rho(rho) {}

    bool operator==(const line &rhs) const {
        return theta == rhs.theta && rho == rhs.rho;
    }

    friend std::ostream &operator<<(std::ostream &os, const line &l) {
        return os << u8"(θ = " << l.theta << u8", ρ = " << l.rho << ")";
    }
};

/// The status of a pixel in a @ref state map.
enum class status_t {
    unset,   ///< Pixel is unset.
    pending, ///< Pixel is set but not yet voted.
    voted,   ///< Pixel is set and voted.
    done     ///< Pixel has been incorporated into a segment.
};

inline std::string to_string(status_t s) {
    using namespace std::string_literals;

#define CASE(X)       \
    case status_t::X: \
        return #X##s;

    switch (s) {
        CASE(unset);
        CASE(pending);
        CASE(voted);
        CASE(done);
    }

    return "unknown"s;

#undef CASE
}

inline std::ostream &operator<<(std::ostream &os, status_t s) {
    return os << to_string(s);
}

} // namespace ppht

#endif /* ppht_types_hpp */

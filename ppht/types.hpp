#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <array>
#include <iostream>
#include <utility>

namespace ppht {

/**
 * @brief Utility function to return the distance between two values.
 *
 * Returns the absolute difference of the two values, even if the
 * values are unsigned.
 *
 * @param a the first value
 * @param b the second value
 *
 * @return the absolute value of the difference between the larger and
 * smaller value.
 */
template <class T1, class T2>
static inline auto abs_diff(const T1 &a, const T2 &b) {
    return a > b ? a - b : b - a;
}

/// A basic integral point.
struct point_t {
    std::size_t x, y;

    point_t(std::size_t x, std::size_t y) : x(x), y(y) {}
    point_t() : point_t(0, 0) {}

    bool operator <(const point_t &p) const {
        return x < p.x ? true : x > p.x ? false : y < p.y;
    }

    bool operator ==(const point_t &p) const {
        return x == p.x && y == p.y;
    }
    bool operator !=(const point_t &p) const {
        return !operator ==(p);
    }

    friend std::ostream &operator <<(std::ostream &o, const point_t &p) {
        return o << '(' << p.x << ", " << p.y << ')';
    }
};

/// An offset from a point.
struct offset_t {
    std::ptrdiff_t dx, dy;

    offset_t(std::ptrdiff_t dx, std::ptrdiff_t dy) : dx(dx), dy(dy) {}
    offset_t() : offset_t(0, 0) {}

    bool operator <(const offset_t &o) const {
        return dx < o.dx ? true : dx > o.dx ? false : dy < o.dy;
    }

    bool operator ==(const offset_t &o) const {
        return dx == o.dx && dy == o.dy;
    }
    bool operator !=(const offset_t &o) const {
        return !operator ==(o);
    }

    friend std::ostream &operator <<(std::ostream &o, const offset_t &d) {
        auto flags = o.flags();
        o << std::showpos << '(' << d.dx << ", " << d.dy << ')';
        o.flags(flags);
        return o;
    }
};

point_t operator +(const point_t &p, const offset_t &o) {
    return {p.x + o.dx, p.y + o.dy};
}

/// A pair of points representing a line segment.
using segment_t = std::pair<point_t, point_t>;

/// The status of a pixel in a @ref state map.
enum status_t {
    unset,                      ///< Pixel is unset.
    pending,                    ///< Pixel is set but not yet voted.
    voted,                      ///< Pixel is set and voted.
    done                        ///< Pixel has been incorporated into
                                ///  a segment.
};

} // namespace ppht

#endif /* ppht_types_hpp */

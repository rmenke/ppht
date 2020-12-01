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

/**
 * @brief Simple type representing a two-dimensional vector.
 *
 * @tparam T the numeric type used for the vector components
 */
template <class T>
struct vec2d {
    T x, y;

    vec2d() {}

    /**
     * @brief Create a new 2-D vector.
     *
     * @param x the x component
     * @param y the y component
     */
    vec2d(T x, T y) : x(x), y(y) {}

    /**
     * @brief Output a vector as a pair.
     *
     * @param o the output stream
     * @param p the vector
     * @return the output stream
     */
    friend std::ostream &operator <<(std::ostream &o, const vec2d &p) {
        return o << '(' << p.x << ", " << p.y << ')';
    }
};

/**
 * @brief Add two vectors.
 *
 * Returns the sum of the individual components of the vectors as a
 * new vector.
 *
 * @param t the first vector
 * @param u the second vector
 *
 * @return a vector made from the sums of the components
 */
template <class T, class U>
static inline auto operator +(const vec2d<T> &t, const vec2d<U> &u) {
    return vec2d<std::common_type_t<T, U>>{t.x + u.x, t.y + u.y};
}

/**
 * @brief Mutiply two vectors.
 *
 * Returns the product of the individual components of the vectors as
 * a new vector.
 *
 * @param t the first vector
 * @param u the second vector
 *
 * @return a vector made from the products of the components
 */
template <class T, class U>
static inline auto operator *(const vec2d<T> &t, const vec2d<U> &u) {
    return vec2d<std::common_type_t<T, U>>{t.x * u.x, t.y * u.y};
}

/**
 * @brief Calculate the inner product of the vectors.
 *
 * @param t the first vector
 * @param u the second vector
 *
 * @return the inner product
 */
template <class T, class U>
auto dot_product(const vec2d<T> &t, const vec2d<U> &u) {
    auto const v = t * u;
    return v.x + v.y;
}

/// A basic integral point.
using point_t = vec2d<std::size_t>;

/// An offset from a point.
using offset_t = vec2d<std::ptrdiff_t>;

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

namespace std {

template <class T> struct equal_to<ppht::vec2d<T>> {
    bool operator()(const ppht::vec2d<T> &a, const ppht::vec2d<T> &b) const {
        return a.x == b.x && a.y == b.y;
    }
};

template <class T> struct not_equal_to<ppht::vec2d<T>> {
    bool operator()(const ppht::vec2d<T> &a, const ppht::vec2d<T> &b) const {
        return a.x != b.x || a.y != b.y;
    }
};

template <> struct equal_to<ppht::segment_t> {
    bool operator()(const ppht::segment_t &a, const ppht::segment_t &b) const {
        return equal_to<ppht::point_t>()(a.first, b.first)
            && equal_to<ppht::point_t>()(a.second, b.second);
    }
};

template <> struct not_equal_to<ppht::segment_t> {
    bool operator()(const ppht::segment_t &a, const ppht::segment_t &b) const {
        return not_equal_to<ppht::point_t>()(a.first, b.first)
            || not_equal_to<ppht::point_t>()(a.second, b.second);
    }
};

template <class T> struct less<ppht::vec2d<T>> {
    bool operator()(const ppht::vec2d<T> &a, const ppht::vec2d<T> &b) const {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        return a.y < b.y;
    }
};

} // namespace std

#endif /* ppht_types_hpp */

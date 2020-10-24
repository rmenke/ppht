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
struct vec2d : std::array<T, 2> {
    vec2d() {}

    /**
     * @brief Create a new 2-D vector.
     *
     * @param x the x component
     * @param y the y component
     */
    vec2d(T x, T y) : std::array<T, 2>({{x, y}}) {}

    /**
     * @brief Output a vector as a pair.
     *
     * @param o the output stream
     * @param p the vector
     * @return the output stream
     */
    friend std::ostream &operator <<(std::ostream &o, const vec2d &p) {
        return o << '(' << p[0] << ", " << p[1] << ')';
    }

    /**
     * @brief Calculate the inner product of the vectors.
     *
     * @param u the other vector
     *
     * @return the inner (dot) product
     */
    template <class U>
    auto dot(vec2d<U> const &u) const {
        auto const v = *this * u;
        return v[0] + v[1];
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
    return vec2d<std::common_type_t<T, U>>{t[0] + u[0], t[1] + u[1]};
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
    return vec2d<std::common_type_t<T, U>>{t[0] * u[0], t[1] * u[1]};
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

#endif /* ppht_types_hpp */

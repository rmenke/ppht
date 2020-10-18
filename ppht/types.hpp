#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <array>
#include <iostream>
#include <utility>

namespace ppht {

template <class T>
struct vec2d : std::array<T, 2> {
    vec2d() {}
    vec2d(T x, T y) : std::array<T, 2>({{x, y}}) {}

    friend std::ostream &operator <<(std::ostream &o, const vec2d &p) {
        return o << '(' << p[0] << ", " << p[1] << ')';
    }

    template <class U>
    auto dot(vec2d<U> const &u) const {
        auto v = *this * u;
        return v[0] + v[1];
    }
};

template <class T, class U>
static inline auto operator +(const vec2d<T> &t, const vec2d<U> &u) {
    return vec2d<std::common_type_t<T, U>>{t[0] + u[0], t[1] + u[1]};
}

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

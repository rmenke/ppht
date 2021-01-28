#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <array>
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

    point_t(long a, long b) : std::array<long, 2>({{a, b}}) {}

    template <class P>
    point_t operator +(const P &p) const {
        return {std::get<0>(*this) + std::get<0>(p),
                std::get<1>(*this) + std::get<1>(p)};
    }

    template <class P>
    point_t operator *(const P &p) const {
        return {std::get<0>(*this) * std::get<0>(p),
                std::get<1>(*this) * std::get<1>(p)};
    }

    template <class P>
    auto dot(const P &p) const {
        return std::get<0>(*this) * std::get<0>(p) +
               std::get<1>(*this) * std::get<1>(p);
    }

    friend std::ostream &operator <<(std::ostream &o, const point_t &p) {
        return o << "(" << std::get<0>(p) << ", " << std::get<1>(p) << ")";
    }
};

/// An offset from a point.
using offset_t [[deprecated]] = point_t;

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

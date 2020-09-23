#ifndef ppht_types_hpp
#define ppht_types_hpp

#include <utility>

namespace ppht {

/// A basic integral point.
using point_t = std::pair<std::size_t, std::size_t>;

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

#ifndef ppht_state_hpp
#define ppht_state_hpp

#include <ppht/channel.hpp>
#include <ppht/point_set.hpp>
#include <ppht/raster.hpp>
#include <ppht/types.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <set>
#include <vector>

namespace ppht {

/**
 * @brief The state class represents the current state of the algorithm.
 *
 * The class carries a cell for each pixel in the image bitmap.  Cells
 * are initially "unset", but can be set by marking them "pending".
 * Once the image is loaded, pixels marked "pending" can be extracted
 * in random order.  Extracting a pixel marks it as "voted."  Once
 * fully processed, any pixel may be marked "done."
 */
template <template <class> class Raster = raster>
class state {
    /// A uniform random bit generator.
    using URBG = std::default_random_engine;

    /// The status of each pixel in the bitmap.
    Raster<status_t> _state;

    /// A collection of pixels marked 'pending' in the raster.
    std::vector<point_t> _pending;

    /// The URBG the class will use to select the next pending pixel.
    URBG _urbg;

  public:
    /**
     * @brief Create an empty state raster and associated pending
     * queue.
     *
     * All cells in the raster will be initialized to @c
     * status_t::unset.
     *
     * @param rows the height of the represented image.
     *
     * @param cols the width of the represented image.
     *
     * @param seed the seed for the random engine.
     */
    state(std::size_t rows, std::size_t cols,
          URBG::result_type seed = std::random_device{}())
        : _state(rows, cols)
        , _urbg(seed) {}

    /**
     * @brief Use an existing state raster to construct an object.
     *
     * The raster will be consumed by this constructor.
     *
     * The pending queue will be loaded with all @c status_t::pending
     * pixels in the state raster.
     *
     * @param s the raster used to create the state
     *
     * @param seed the seed for the random engine
     */
    state(Raster<status_t> &&s,
          URBG::result_type seed = std::random_device{}())
        : _state(std::move(s))
        , _urbg(seed) {
        point_t p;

        for (p[1] = 0; p[1] < _state.rows; ++p[1]) {
            auto const row = _state[p[1]];
            for (p[0] = 0; p[0] < _state.cols; ++p[0]) {
                if (row[p[0]] == status_t::pending) {
                    _pending.push_back(p);
                }
            }
        }
    }

    /**
     * @brief Get the number of rows of the state image.
     *
     * @return the height of the underlying raster.
     */
    std::size_t const &rows() const {
        return _state.rows();
    }

    /**
     * @brief Get the number of columns of the state image.
     *
     * @return the width of the underlying raster.
     */
    std::size_t const &cols() const {
        return _state.cols();
    }

    /**
     * @brief Get the status of a pixel in the raster.
     *
     * @param point the pixel to check.
     *
     * @return the status of the pixel.
     */
    status_t status(point_t const &point) const {
        return _state[std::get<1>(point)][std::get<0>(point)];
    }

    /**
     * @brief Mark a pixel in the raster as @c pending.
     *
     * @param point the pixel to mark.
     */
    void mark_pending(point_t const &point) {
        auto const x = std::get<0>(point);
        auto const y = std::get<1>(point);

        _state[y][x] = status_t::pending;
        _pending.emplace_back(point);
    }

    /**
     * @brief Mark a pixel in the raster as @c done.
     *
     * @param point the pixel to mark.
     */
    void mark_done(point_t const &point) {
        auto const x = std::get<0>(point);
        auto const y = std::get<1>(point);

        _state[y][x] = status_t::done;
    }

    /**
     * @brief Return a random pixel with @c pending status.
     *
     * Changes the status of the pixel to @c voted.  If the status of
     * the next pixel in the queue has been updated elsewhere, the
     * pixel is removed from the queue and not returned.
     *
     * @param point the pixel to set
     *
     * @return true if @c point is a valid pending pixel; false if there
     * are no more pending pixels in the raster
     */
    bool next(point_t &point) {
        auto begin = _pending.begin();
        auto end = _pending.end();

        end = std::remove_if(begin, end, [this](auto &&p) {
            return status(p) != status_t::pending;
        });

        _pending.erase(end, _pending.end());

        if (begin == end) return false;

        using index_type = typename decltype(_pending)::difference_type;

        std::uniform_int_distribution<index_type>
            dist{0, std::distance(begin, end) - 1};

        auto iter = begin + dist(_urbg);

        point = std::exchange(*iter, *(--end));

        auto &cell = _state[std::get<1>(point)][std::get<0>(point)];

        assert(cell == status_t::pending);

        cell = status_t::voted;

        return true;
    }
};

/**
 * @brief Return a set of offsets that are within a radius.
 *
 * The number of offsets generated is controlled by the @c radius
 * parameter.  For a radius @f$r@f$, the returned offsets will be
 * @f$0,1,\ldots,r@f$ pixels from the origin, along the perpendicular
 * to the @c segment parameter.  Both directions are considered, so
 * @f$2^r+1@f$ offsets will be generated before duplicates are
 * removed.
 *
 * @param segment used to determine the orientation of the perpendicular.
 *
 * @param radius determines the number of offsets to calculate.
 *
 * @return a non-empty set of offsets.
 *
 * @sa scan()
 */
static inline std::set<offset_t>
find_offsets(segment_t const &segment, unsigned radius) {
    // The vector [xn, yn] is normal to the segment.

    double xn = std::get<1>(segment.second);
    xn -= std::get<1>(segment.first);
    double yn = std::get<0>(segment.first);
    yn -= std::get<0>(segment.second);

    // Make [xn, yn] the unit normal vector.

    double len = std::hypot(xn, yn);
    xn /= len, yn /= len;

    std::set<offset_t> result{{0, 0}};

    for (auto r = 1U; r <= radius; ++r) {
        auto dx = std::lround(xn * r);
        auto dy = std::lround(yn * r);

        result.emplace(dx, dy);
        result.emplace(-dx, -dy);
    }

    return result;
}

/**
 * @brief Trace a scan channel.
 *
 * Iterate over the points in the scan channel described by @c
 * segment: these are the canonical points.  For each canonical point,
 * examine all of the pixels within the channel radius.  If any are
 * set, add the canonical point to the current segment.  At the end of
 * a gap of @c max_gap pixels, end the current segment and start a new
 * one.  Upon completion of the scan, return the longest segment found
 * so far.
 *
 * @param s the state object to update
 *
 * @param segment the canonical segment of the scan channel
 *
 * @param radius the number of pixels to check around the canonical
 *   segment
 *
 * @param max_gap the number of consecutive missed pixels that can
 *   appear in a segment
 *
 * @returns a @ref point_set around the longest segment found
 *
 * @throws std::logic_error if no points are set in the scan channel
 *
 * @sa find_offsets()
 */
template <template <class> class Raster>
point_set scan(state<Raster> &s, segment_t const &segment, unsigned radius,
               unsigned max_gap) {
    auto const offsets = find_offsets(segment, radius);

    // The initial gap is technically infinite, but anything
    // larger than max_gap will do.
    auto gap = max_gap + 1;

    std::vector<point_set> point_sets;

    for (auto const &point : channel(segment)) {
        std::set<point_t> points;

        for (auto const &offset : offsets) {
            auto x = point[0] + offset[0];
            auto y = point[1] + offset[1];

            if (x < 0 || x >= s.cols()) continue;
            if (y < 0 || y >= s.rows()) continue;

            point_t p{x, y};

            auto status = s.status(p);

            if (status == status_t::pending || status == status_t::voted) {
                points.emplace(p);
            }
        }

        if (points.empty()) { // no hits
            ++gap;
        }
        else {
            // If the gap is too large to ignore, start a new point set.
            if (gap > max_gap) point_sets.emplace_back();
            point_sets.back().add_point(point, points);

            gap = 0;
        }
    }

    if (point_sets.empty()) {
        throw std::logic_error{"channel contained no viable segments"};
    }

    auto longest = std::max_element(point_sets.begin(), point_sets.end());

    return *longest;
}

} // namespace ppht

#endif /* ppht_state_hpp */

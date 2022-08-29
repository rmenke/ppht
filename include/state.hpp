// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_state_hpp
#define ppht_state_hpp

#include "channel.hpp"
#include "point_set.hpp"
#include "raster.hpp"
#include "trig.hpp"
#include "types.hpp"

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
    std::vector<point> _pending;

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
        point p;

        for (p.y = 0; p.y < _state.rows; ++p.y) {
            auto const row = _state[p.y];
            for (p.x = 0; p.x < _state.cols; ++p.x) {
                if (row[p.x] == status_t::pending) {
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
    std::size_t rows() const {
        return _state.rows();
    }

    /**
     * @brief Get the number of columns of the state image.
     *
     * @return the width of the underlying raster.
     */
    std::size_t cols() const {
        return _state.cols();
    }

    /**
     * @brief Get the status of a pixel in the raster.
     *
     * @param point the pixel to check.
     *
     * @return the status of the pixel.
     */
    status_t status(point const &point) const {
        return _state[point.y][point.x];
    }

    /**
     * @brief Mark a pixel in the raster as @c pending.
     *
     * @param point the pixel to mark.
     */
    void mark_pending(point const &point) {
        _state[point.y][point.x] = status_t::pending;
        _pending.emplace_back(point);
    }

    /**
     * @brief Mark a pixel in the raster as @c done.
     *
     * @param point the pixel to mark.
     */
    void mark_done(point const &point) {
        _state[point.y][point.x] = status_t::done;
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
    bool next(point &point) {
        auto begin = _pending.begin();
        auto end = _pending.end();

        end = std::remove_if(begin, end, [this](auto &&p) {
            return status(p) != status_t::pending;
        });

        _pending.erase(end, _pending.end());

        if (begin == end) return false;

        using index_type = typename decltype(_pending)::difference_type;

        std::uniform_int_distribution<index_type> dist{
            0, std::distance(begin, end) - 1};

        auto iter = begin + dist(_urbg);

        point = std::exchange(*iter, *(--end));

        auto &cell = _state[point.y][point.x];

        assert(cell == status_t::pending);

        cell = status_t::voted;

        return true;
    }

    /// @brief Find the portion of the line that lies within the bounds
    /// of the bitmap.
    ///
    /// Given a line in @f$\theta\rho@f$-space, return the line segment
    /// that is the portion of the line that intersects the bitmap.
    ///
    /// @param line the line in @f$(\theta,\rho)@f$ format
    ///
    /// @return the portion of the line within the bounds of the bitmap
    /// in integral coordinates.
    std::pair<point, point> line_intersect(line_t const &line) const {
        // There are a few degenerate cases where multiple matches for
        // the same endpoint can be found, e.g., a line through the
        // origin.  Using a set eliminates most of these cases.  See
        // the comment at the end for what happens to those that slip
        // through.

        auto const &t = std::get<0>(line);
        auto const &r = std::get<1>(line);

        std::set<point> endpoints;

        auto const &cost = std::get<0>(cossin[t]);
        auto const &sint = std::get<1>(cossin[t]);

        using limit = std::numeric_limits<long>;
        static constexpr long lo = limit::min();
        static constexpr long hi = limit::max();

        auto get_x = [&](double y) -> long {
            double x = std::rint((r - sint * y) / cost);
            return x < lo ? lo : hi < x ? hi : x;
        };

        auto get_y = [&](double x) -> long {
            double y = std::rint((r - cost * x) / sint);
            return y < lo ? lo : hi < y ? hi : y;
        };

        const long w = cols() - 1;
        const long h = rows() - 1;

        const long x_min = get_x(0);
        const long y_min = get_y(0);
        const long x_max = get_x(h);
        const long y_max = get_y(w);

        if (0 <= y_min && y_min <= h) endpoints.emplace(0, y_min);
        if (0 <= x_min && x_min <= w) endpoints.emplace(x_min, 0);
        if (0 <= y_max && y_max <= h) endpoints.emplace(w, y_max);
        if (0 <= x_max && x_max <= w) endpoints.emplace(x_max, h);

        if (endpoints.empty()) {
            throw std::logic_error{"line (" + std::to_string(t) + ", " +
                                   std::to_string(r) +
                                   ") does not intersect bitmap"};
        }

        // If endpoints.size() > 2, ignore the points in the middle.
        // If endpoints.size() == 1, create single-pixel segment.

        return std::make_pair(*endpoints.begin(), *endpoints.rbegin());
    }
};

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
point_set scan(state<Raster> &s, std::pair<point, point> const &segment,
               unsigned radius, unsigned max_gap) {
    // The initial gap is technically infinite, but anything
    // larger than max_gap will do.
    auto gap = max_gap + 1;

    std::vector<point_set> point_sets;

    auto const &p0 = std::get<0>(segment);
    auto const &p1 = std::get<1>(segment);

    long const r = s.rows();
    long const c = s.cols();

    for (auto const &[canonical, points] : channel(p0, p1, radius)) {
        std::set<point> found;

        for (auto const &pt : points) {
            if (pt.x < 0 || pt.x >= c) continue;
            if (pt.y < 0 || pt.y >= r) continue;

            auto status = s.status(pt);

            if (status == status_t::pending || status == status_t::voted) {
                found.insert(pt);
            }
        }

        if (found.empty()) { // no hits
            ++gap;
        }
        else {
            // If the gap is too large to ignore, start a new point set.
            if (gap > max_gap) point_sets.emplace_back();
            point_sets.back().add_point(canonical, found);

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

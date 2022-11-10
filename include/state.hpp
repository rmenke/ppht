// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_state_hpp
#define ppht_state_hpp

#include "channel.hpp"
#include "point_set.hpp"
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
class state {
    /// A uniform random bit generator.
    using URBG = std::default_random_engine;

    /// The status of each pixel in the bitmap.
    std::unique_ptr<status_t[]> _status;

    /// The dimensions of the bitmap.
    std::size_t const _rows, _cols;

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
        : _status(new status_t[rows * cols])
        , _rows(rows)
        , _cols(cols)
        , _urbg(seed) {}

    /**
     * @brief Get the number of rows of the state image.
     *
     * @return the height of the underlying raster.
     */
    std::size_t rows() const {
        return _rows;
    }

    /**
     * @brief Get the number of columns of the state image.
     *
     * @return the width of the underlying raster.
     */
    std::size_t cols() const {
        return _cols;
    }

    /**
     * @brief Get the status of a pixel in the raster.
     *
     * @param point the pixel to check.
     *
     * @return the status of the pixel.
     */
    status_t status(point const &point) const {
        if (point.x < 0 || point.x >= static_cast<long>(_cols) ||
            point.y < 0 || point.y >= static_cast<long>(_rows)) {
            return status_t::unset;
        }

        return _status[point.y * _cols + point.x];
    }

    /**
     * @brief Mark a pixel in the raster as @c pending.
     *
     * @param point the pixel to mark.
     */
    void mark_pending(point const &point) {
        _status[point.y * _cols + point.x] = status_t::pending;
        _pending.emplace_back(point);
    }

    /**
     * @brief Mark a pixel in the raster as @c done.
     *
     * @param point the pixel to mark.
     */
    void mark_done(point const &point) {
        _status[point.y * _cols + point.x] = status_t::done;
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

        auto &cell = _status[point.y * _cols + point.x];

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
    std::pair<point, point> line_intersect(line const &line) const {
        // There are a few degenerate cases where multiple matches for
        // the same endpoint can be found, e.g., a line through the
        // origin.  Using a set eliminates most of these cases.  See
        // the comment at the end for what happens to those that slip
        // through.

        std::set<point> endpoints;

        auto const &cos_theta = get<0>(cossin[line.theta]);
        auto const &sin_theta = get<1>(cossin[line.theta]);

        using limit = std::numeric_limits<long>;
        static constexpr long lo = limit::min();
        static constexpr long hi = limit::max();

        auto get_x = [&](double y) -> long {
            double x = std::rint((line.rho - sin_theta * y) / cos_theta);
            return x < lo ? lo : hi < x ? hi : x;
        };

        auto get_y = [&](double x) -> long {
            double y = std::rint((line.rho - cos_theta * x) / sin_theta);
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
            throw std::logic_error{"line (" + std::to_string(line.theta) +
                                   ", " + std::to_string(line.rho) +
                                   ") does not intersect bitmap"};
        }

        // If endpoints.size() > 2, ignore the points in the middle.
        // If endpoints.size() == 1, create single-pixel segment.

        return std::make_pair(*endpoints.begin(), *endpoints.rbegin());
    }

    /// @brief Trace a scan channel.
    ///
    /// Iterate over the points in the scan channel described by @c
    /// segment: these are the canonical points.  For each canonical point,
    /// examine all of the pixels within the channel radius.  If any are
    /// set, add the canonical point to the current segment.  At the end of
    /// a gap of @c max_gap pixels, end the current segment and start a new
    /// one.  Upon completion of the scan, return the longest segment found
    /// so far.
    ///
    /// @param line the channel to scan
    ///
    /// @param radius the number of pixels to check around the line
    ///
    /// @param max_gap the number of consecutive missed pixels that can
    ///   appear in a segment
    ///
    /// @returns a @ref point_set around the longest segment found
    ///
    /// @throws std::logic_error if no points are set in the scan channel
    ///
    /// @sa line_intersect()
    auto scan(const line &line, std::size_t radius, std::size_t max_gap) {
        // The initial gap is technically infinite, but anything
        // larger than max_gap will do.
        auto gap = max_gap + 1;

        std::vector<point_set> segments;

        auto [p0, p1] = line_intersect(line);

        for (auto const &[canonical, points] : channel(p0, p1, radius)) {
            std::set<point> found;

            for (auto const &pt : points) {
                if (auto s = status(pt);
                    s == status_t::pending || s == status_t::voted) {
                    found.insert(pt);
                }
            }

            if (found.empty()) {
                ++gap;
            }
            else {
                // If the gap is too large to ignore, start a new
                // point set.
                if (gap > max_gap) segments.emplace_back();
                segments.back().add_point(canonical, found);

                gap = 0;
            }
        }

        if (segments.empty()) {
            throw std::logic_error{"channel contained no viable segments"};
        }

        auto longest = std::max_element(segments.begin(), segments.end());

        return *longest;
    }
};

} // namespace ppht

#endif /* ppht_state_hpp */

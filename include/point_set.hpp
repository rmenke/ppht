// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_point_set_hpp
#define ppht_point_set_hpp

#include "types.hpp"

#include <set>

namespace ppht {

/**
 * @brief A collection of points surrounding a line segment.
 *
 * The point set represents a part of a line and a set of image pixels
 * near that portion of the line.  The segment need not pass through
 * all of the pixels and may have points not in the pixel set.
 */
class point_set {
    /// @brief The pixel points added to the set.
    std::set<point> _points;

    /// @brief The endpoints of the segment.
    std::pair<point, point> _endpoints;

  public:
    /**
     * @brief Check to see if any points have been added to the point set.
     *
     * @return true if no points are in the set
     */
    bool empty() const {
        return _points.empty();
    }

    /**
     * @brief Add a collection of points and their canonical
     * representation to the point set.
     *
     * The canonical representation is the point that lies at the
     * center of the channel, whether or not it is part of the points
     * passed in.
     *
     * @param canonical the point used to extend the segment.  Will
     * not be added to the point set
     *
     * @param points additional points to add to the set
     */
    template <class Points>
    void add_point(point canonical, Points &&points) {
        assert(!points.empty());

        auto &[a, b] = _endpoints;

        if (_points.empty()) a = canonical;
        b = canonical;

        for (auto &&p : std::forward<Points>(points)) {
            _points.insert(std::forward<decltype(p)>(p));
        }
    }

    /**
     * @brief Return the segment associated with this point set.
     *
     * The value is defined only if @ref empty() returns false.
     *
     * @return the segment associated with the point set.
     */
    const std::pair<point, point> &endpoints() const {
        return _endpoints;
    }

    /**
     * @brief Get an iterator to the start of the point collection.
     *
     * @return a valid iterator
     */
    auto begin() const {
        return _points.begin();
    }

    /**
     * @brief Get an iterator after the end of the point collection.
     *
     * @return a valid past-the-end iterator
     */
    auto end() const {
        return _points.end();
    }

    /**
     * @brief The length of the segment squared.
     *
     * The result of this function is undefined if the point set is empty.
     *
     * @return an integer that is the length of the segment squared
     */
    std::ptrdiff_t length_squared() const {
        assert(!_points.empty());

        auto const &[a, b] = _endpoints;

        const auto dx = std::abs(b.x - a.x);
        const auto dy = std::abs(b.y - a.y);

        return dx * dx + dy * dy;
    }

    /**
     * @brief Establish a "less than" relationship between point sets.
     *
     * A point set is "less than" another point set if its segment is
     * shorter.  An empty point set is considered smaller than any
     * non-empty point set even if its length is undefined.
     *
     * @param rhs the point set to compare.
     *
     * @return true if the length of this point set's segment is shorter
     * than the other point set's segment.
     *
     * @sa length_squared()
     */
    bool operator<(const point_set &rhs) const {
        if (rhs._points.empty()) return false;
        if (_points.empty()) return true;

        return length_squared() < rhs.length_squared();
    }
};

} // namespace ppht

#endif /* ppht_point_set_hpp */

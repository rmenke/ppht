#ifndef ppht_point_set_hpp
#define ppht_point_set_hpp

#include <ppht/types.hpp>

#include <set>

namespace ppht {

/**
 * @brief A collection of points surrounding a line segment.
 *
 * The point_set represents a line segment and the image pixels that
 * make up the segment.  The segment need not pass through all of the
 * points and may consist of points not in the pixel set.
 */
class point_set {
    /// @brief The points added to the set.
    std::set<point_t> _points;

    /// @brief The segment making up the canonical points of the set.
    segment_t _segment;

public:
    /**
     * @brief Check to see if any points have been added to the point set.
     *
     * @return true if no points are in the set.
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
     * not be added to the point set.
     *
     * @param points additional points to add to the set.
     */
    template <class Points>
    void add_point(point_t canonical, Points &&points) {
        if (_points.empty()) _segment.first = canonical;
        _segment.second = canonical;

        for (auto &&p : points) {
            _points.insert(p);
        }
    }

    /**
     * @brief Return the segment associated with this point set.
     *
     * The value is defined only if @ref empty() returns false.
     *
     * @return the segment associated with the point set.
     */
    const segment_t &segment() const {
        return _segment;
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
     * If the point set is empty, returns -1.  This ensures that an
     * empty point set is smaller than non-empty point sets when
     * ranked by length.
     *
     * @return an integer that is the length of the segment squared, or
     * -1 if the point set is empty.
     *
     * @sa operator <()
     */
    std::ptrdiff_t length_squared() const {
        if (_points.empty()) return -1;

        const auto &a = std::get<0>(_segment);
        const auto &b = std::get<1>(_segment);

        const auto dx = std::abs(std::get<0>(a) - std::get<0>(b));
        const auto dy = std::abs(std::get<1>(a) - std::get<1>(b));

        return dx * dx + dy * dy;
    }

    /**
     * @brief Establish a "less than" relationship between point sets.
     *
     * A point set is "less than" another point set if its segment is
     * shorter.  An empty point set is smaller than any non-empty
     * point set.
     *
     * @param rhs the point set to compare.
     *
     * @return true if the length of this point set's segment is shorter
     * than the other point set's segment.
     *
     * @sa length_squared()
     */
    bool operator<(const point_set &rhs) const {
        return length_squared() < rhs.length_squared();
    }
};

} // namespace ppht

#endif /* ppht_point_set_hpp */

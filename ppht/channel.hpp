#ifndef ppht_channel_hpp
#define ppht_channel_hpp

#include <ppht/types.hpp>

#include <cstdlib>
#include <iterator>
#include <memory>

namespace ppht {

/**
 * @brief A class for tracking the state required to advance a point
 * along a segment.
 */
struct scanner {
    virtual ~scanner() {}

    /**
     * @brief Virtual "copy constructor."
     *
     * Duplicate the underlying state and return a scanner.
     *
     * @return a unique_ptr to a scanner of the same type and state.
     */
    virtual std::unique_ptr<scanner> clone() const = 0;

    /**
     * @brief Update the given point according to the state of the scanner.
     *
     * @param point the point to update.
     */
    virtual void advance(point_t &point) = 0;
};

/**
 * @brief A scanner optimized for lines parallel to an axis.
 *
 * This class carries no state.
 *
 * @tparam Ind the independent field of point_t
 *
 * @see ppht::make_scanner(segment_t &)
 */
template <std::size_t point_t::*Ind>
struct axis_scanner : scanner {
    ~axis_scanner() override {}

    std::unique_ptr<scanner> clone() const override {
        return std::unique_ptr<scanner>(new axis_scanner{*this});
    }

    void advance(point_t &point) override {
        point.*Ind += 1;
    }
};

/**
 * @brief A scanner based on Bresenham's line algorithm.
 *
 * This approach is ideal for this task because it is designed to work
 * on monochrome rasters and minimizes the involvement of the FPU.
 * This class actually implements the four variants of the algorithm
 * by encoding the slope through the template parameters:
 *
 * @f{array}{{llr}
 * \text{Octant I:}    & m\in(0,+1]       & \Delta x\gt 0 \\
 * \text{Octant II:}   & m\in(+1,+\infty) & \Delta y\gt 0 \\
 * \text{Octant III:}  & m\in(-\infty,-1) & \Delta y\gt 0 \\
 * \text{Octant VIII:} & m\in[-1,0)       & \Delta x\gt 0 \\
 * @f}
 *
 * All other cases are handled by reversing the direction of the scan.
 *
 * @tparam Ind the independent field of @ref point_t
 * @tparam Dep the dependent field of @ref point_t
 * @tparam Increment the increment or decrement for the
 * dependent field
 *
 * @see ppht::make_scanner(segment_t &)
 *
 * @sa https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
 */
template <std::size_t point_t::*Ind, std::size_t point_t::*Dep, int Increment>
struct bresenham_scanner : scanner {
    /** The ∆x and ∆y values for the segment. */
    const point_t delta;

    /** The cumulative error of the next point. */
    long D;

    bresenham_scanner(const point_t &a, const point_t &b)
        : delta(point_t{abs_diff(a.x, b.x), abs_diff(a.y, b.y)})
        , D(delta.*Dep * 2 - delta.*Ind) {}

    ~bresenham_scanner() override {}

    std::unique_ptr<scanner> clone() const override {
        return std::unique_ptr<scanner>(new bresenham_scanner{*this});
    }

    void advance(point_t &point) override {
        if (D > 0) {
            D -= 2 * delta.*Ind;
            point.*Dep += Increment;
        }

        D += 2 * delta.*Dep;
        point.*Ind += 1;
    }
};

template <int Increment>
using h_scanner = bresenham_scanner<&point_t::x, &point_t::y, Increment>;
template <int Increment>
using v_scanner = bresenham_scanner<&point_t::y, &point_t::x, Increment>;

/**
 * Factory function to create the appropriate scanner for the given
 * segment.  If the segment is not oriented correctly, then it will
 * be corrected by exchanging the endpoints.
 *
 * @param segment the segment to scan.
 *
 * @return a pointer to an instance of @ref scanner.
 *
 * @note this function modifies its arguments.
 */
static inline std::unique_ptr<scanner> make_scanner(segment_t &segment) {
    auto &a = std::get<0>(segment);
    auto &b = std::get<1>(segment);

    const auto dx = abs_diff(a.x, b.x);
    const auto dy = abs_diff(a.y, b.y);

    if (dx >= dy) {
        if (a.x > b.x) {
            std::swap(a, b);
        }

        if (a.y < b.y) {
            return std::unique_ptr<scanner>(new h_scanner<1>{a, b});
        }
        else if (a.y == b.y) {
            return std::unique_ptr<scanner>(new axis_scanner<&point_t::x>);
        }
        else { // a.y > b.y
            return std::unique_ptr<scanner>(new h_scanner<-1>{a, b});
        }
    }
    else {
        if (a.y > b.y) {
            std::swap(a, b);
        }

        if (a.x < b.x) {
            return std::unique_ptr<scanner>(new v_scanner<1>{a, b});
        }
        else if (a.x == b.x) {
            return std::unique_ptr<scanner>(new axis_scanner<&point_t::y>);
        }
        else { // a.x > b.x
            return std::unique_ptr<scanner>(new v_scanner<-1>{a, b});
        }
    }
}

/**
 * @brief An abstract container that wraps around a line segment.
 *
 * The iterators it supplies will visit all of the pixels that make up
 * the segment.
 */
class channel {
    /**
     * @brief The segment describing the channel.
     *
     * The segment may not have the same value as the one passed into
     * the constructor.  It will be oriented to facilitate the
     * scanner.
     *
     * @sa make_scanner()
     */
    segment_t _segment;

  public:
    /**
     * @brief Construct a channel for the given segment.
     *
     * @param segment the segment describing the endpoints of the
     * channel.
     */
    channel(const segment_t &segment) noexcept
        : _segment(segment) {}

    /**
     * @brief A forward iterator wrapping a scanner.
     *
     * The iterator acts as a façade over the @ref scanner interface.
     */
    class iterator {
        /// The segment from the channel.
        segment_t _segment;

        /// The underlying scanner.
        std::unique_ptr<scanner> _scanner;

      public:
        /// Support for std::iterator_traits.
        using difference_type = std::ptrdiff_t;
        /// Support for std::iterator_traits.
        using value_type = point_t;
        /// Support for std::iterator_traits.
        using reference = const value_type &;
        /// Support for std::iterator_traits.
        using pointer = const value_type *;
        /// Support for std::iterator_traits.
        using iterator_category = std::forward_iterator_tag;

        /**
         * Constructs the end-of-sequence iterator.
         */
        iterator()
            : _segment({0, 0}, {0, 0}) {}

        /**
         * Construct an iterator that returns the points making up the
         * segment.
         *
         * @param segment the segment over which to iterate.
         *
         * @sa make_scanner()
         */
        iterator(const segment_t &segment)
            : _segment(segment)
            , _scanner(make_scanner(_segment)) {}

        /**
         * @brief Copy the iterator.
         *
         * The underlying scanner will also be copied.
         *
         * @param r the other iterator.
         */
        iterator(const iterator &r)
            : _segment(r._segment)
            , _scanner(r._scanner ? r._scanner->clone() : nullptr) {}

        /**
         * @brief Move the iterator.
         *
         * @param r the other iterator.
         */
        iterator(iterator &&r) = default;

        /**
         * @brief Copy-assign the iterator.
         *
         * The underlying scanner will also be copied.
         *
         * @param r the other iterator.
         */
        iterator &operator=(const iterator &r) {
            _scanner = r._scanner ? r._scanner->clone() : nullptr;
            _segment = r._segment;
            return *this;
        }

        /**
         * @brief Move-assign the iterator.
         *
         * @param r the other iterator.
         */
        iterator &operator=(iterator &&r) = default;

        /**
         * @brief Get the current point of the scan.
         *
         * @return a const reference to the current point.
         */
        reference operator*() const {
            return _segment.first;
        }

        /**
         * @brief Get the current point of the scan.
         *
         * @return a const pointer to the current point.
         */
        pointer operator->() const {
            return &_segment.first;
        }

        /**
         * @brief Check for equality.
         *
         * Two iterators are considered equal only if they are both
         * end-of-range iterators.
         *
         * @param r the other iterator
         *
         * @return true if both iterators are end-of-range
         */
        bool operator==(const iterator &r) const {
            return !_scanner && !r._scanner;
        }

        /**
         * @brief Check for inequality.
         *
         * Two iterators are considered equal only if they are both
         * end-of-range iterators.
         *
         * @param r the other iterator
         *
         * @return true if either iterator is not end-of-range
         */
        bool operator!=(const iterator &r) const {
            return !operator==(r);
        }

        /**
         * @brief Increment the iterator to the next point.
         *
         * The iterator will return the second point of the segment
         * before making itself end-of-range.
         *
         * @return the iterator
         */
        iterator &operator++() {
            if (_segment.first == _segment.second) {
                _scanner.reset();
            }
            else {
                _scanner->advance(_segment.first);
            }

            return *this;
        }
    };

    /**
     * @brief An iterator to the first pixel in the channel.
     *
     * @return an iterator
     */
    iterator begin() {
        return iterator{_segment};
    }

    /**
     * @brief An iterator beyond the last point in the channel.
     *
     * @return a valid past-the-end iterator
     */
    iterator end() {
        return iterator{};
    }
};

} // namespace ppht

#endif /* ppht_channel_hpp */

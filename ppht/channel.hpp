#ifndef ppht_channel_hpp
#define ppht_channel_hpp

#include <ppht/types.hpp>

#include <cstdlib>
#include <iterator>
#include <memory>

namespace ppht {

/**
 * @brief Returns the absolute difference of the two values correctly
 * even for unsigned types.
 *
 * @tparam T the argument type.
 *
 * @param a the first argument.
 *
 * @param b the second argument.
 *
 * @return the absolute value of the difference between the arguments.
 */
template <class T>
static inline T abs_diff(const T &a, const T &b) noexcept {
    return (a > b) ? (a - b) : (b - a);
}

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
 * @tparam Ind the index of the independent field of point_t.
 *
 * @see ppht::make_scanner(segment_t &)
 */
template <std::size_t Ind>
struct axis_scanner : scanner {
    ~axis_scanner() override {}

    std::unique_ptr<scanner> clone() const override {
        return std::unique_ptr<scanner>(new axis_scanner{*this});
    }

    void advance(point_t &point) override {
        std::get<Ind>(point) += 1;
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
 * <table style="border-style:none">
 * <tr>
 *   <td>octant I:</td>
 *   <td style="text-align:right">0</td>
 *   <td>&lt; m &lt;</td>
 *   <td style="text-align:right">+1,</td>
 *   <td>Δx &gt; 0</td>
 * </tr>
 * <tr>
 *   <td>octant II:</td>
 *   <td style="text-align:right">+1</td>
 *   <td>&lt; m &lt;</td>
 *   <td style="text-align:right">+∞,</td>
 *   <td>Δy &gt; 0</td>
 * </tr>
 * <tr>
 *   <td> octant III: </td>
 *   <td style="text-align:right">-1</td>
 *   <td>&gt; m &gt;</td>
 *   <td style="text-align:right">-∞,</td>
 *   <td>Δy &gt; 0</td>
 * </tr>
 * <tr>
 *   <td> octant VIII:</td>
 *   <td style="text-align:right">0</td>
 *   <td>&gt; m &gt;</td>
 *   <td style="text-align:right">-1,</td>
 *   <td>Δx &gt; 0</td>
 * </tr>
 * </table>
 *
 * All other cases are handled by reversing the direction of the scan.
 *
 * @tparam Ind the index of the independent field of point_t (0 = X, 1
 * = Y).
 *
 * @tparam Increment the increment or decrement for the
 * dependent field.
 *
 * @sa make_scanner()
 *
 * @sa https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
 */
template <std::size_t Ind, int Increment>
struct bresenham_scanner : scanner {
    /** The index of the dependent field of point_t. */
    static constexpr std::size_t Dep = 1 - Ind;

    /** The ∆x and ∆y values for the segment. */
    const point_t delta;

    /** The cumulative error of the next point. */
    long D;

    bresenham_scanner(std::size_t x0, std::size_t y0, std::size_t x1,
                      std::size_t y1)
        : delta(point_t{abs_diff(x0, x1), abs_diff(y0, y1)}) {
        D = std::get<Dep>(delta) * 2;
        D -= std::get<Ind>(delta);
    }

    ~bresenham_scanner() override {}

    std::unique_ptr<scanner> clone() const override {
        return std::unique_ptr<scanner>(new bresenham_scanner{*this});
    }

    void advance(point_t &point) override {
        if (D > 0) {
            D -= 2 * std::get<Ind>(delta);
            std::get<Dep>(point) += Increment;
        }

        D += 2 * std::get<Dep>(delta);
        std::get<Ind>(point) += 1;
    }
};

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

    auto &x0 = std::get<0>(a);
    auto &y0 = std::get<1>(a);
    auto &x1 = std::get<0>(b);
    auto &y1 = std::get<1>(b);

    const auto dx = abs_diff(x0, x1);
    const auto dy = abs_diff(y0, y1);

    if (dx >= dy) {
        if (x0 > x1) {
            std::swap(a, b);
        }

        if (y0 < y1) {
            return std::unique_ptr<scanner>(
                new bresenham_scanner<0, +1>{x0, y0, x1, y1});
        }
        else if (y0 == y1) {
            return std::unique_ptr<scanner>(new axis_scanner<0>{});
        }
        else { // y0 > y1
            return std::unique_ptr<scanner>(
                new bresenham_scanner<0, -1>{x0, y0, x1, y1});
        }
    }
    else {
        if (y0 > y1) {
            std::swap(a, b);
        }

        if (x0 < x1) {
            return std::unique_ptr<scanner>(
                new bresenham_scanner<1, +1>{x0, y0, x1, y1});
        }
        else if (x0 == x1) {
            return std::unique_ptr<scanner>(new axis_scanner<1>{});
        }
        else { // x0 > x1
            return std::unique_ptr<scanner>(
                new bresenham_scanner<1, -1>{x0, y0, x1, y1});
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

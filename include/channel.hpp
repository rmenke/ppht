// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_channel_hpp
#define ppht_channel_hpp

#include "types.hpp"

#include <cassert>
#include <cmath>
#include <memory>
#include <set>

namespace ppht {

/// @brief An object representing a line segment between two points.
///
/// A channel instance is an immutable factory object that acts as a
/// pseudo-container for range-based iteration.  The values produced
/// are sets of points describing a perpendicular line segment
/// intersecting a canonical point on the ideal line between the two
/// endpoints.  In rare cases the canonical point may not be a member
/// of the point set, but is adjacent to one.
///
/// Typical use case:
///
/// @code
/// for (const auto &[canon, pts] : channel{p1, p2, rdus}) {
///   for (const auto &[x, y] : pts) {
///     if (test(x, y)) {
///       segment.insert(canon);
///       break;
///     }
///   }
/// }
/// @endcode
class channel {
    const point _p0, _p1;
    const std::size_t _radius;

    template <class T>
    static T signum(T value) {
        return value < T{0} ? T{-1} : T{0} < value ? T{+1} : T{0};
    }

    /// @brief An interface used to advance the iterator.
    ///
    /// Depending on the segment, the process may be simple or
    /// complex.  This interface hides that detail.
    ///
    /// It is not a pure strategy pattern, as the scanner
    /// implementation may have its own internal state.
    class scanner {
      public:
        virtual ~scanner() {}

        /// @brief Create a perpendicular line segment.
        ///
        /// Adds the pixels making up the perpendicular to the given
        /// point set.  Note that in rare cases the @c point may not
        /// be added to the set @c points.
        ///
        /// @param pt the reference point
        /// @param pts the set to which points will be added
        virtual void fill(point pt, std::set<point> &pts) const = 0;

        /// @brief Advance the point along the line.
        ///
        /// This method is not idempotent as an implementation will
        /// update its internal state as part of the call.  Calling
        /// this method with different references is undefined.
        ///
        /// @param pt the point to modify
        virtual void advance(point &pt) = 0;
    };

    /// @brief A scanner optimized for axial lines.
    ///
    /// The axial scanner is optimized for handling lines
    /// perpendicular to an axis; that is, horizontal and vertical
    /// lines.
    ///
    /// The @c Major template parameter controls the orientation of
    /// the line. If it is zero, the line is horizontal; if it is one,
    /// the line is vertical.
    ///
    /// @tparam Major the axis along which the point will be moved
    template <std::size_t Major>
    class axial_scanner : public scanner {
        static constexpr std::size_t Minor = 1 - Major;

        const point step;
        const std::size_t radius;

      public:
        axial_scanner(point delta, std::size_t radius)
            : step{signum(delta.x), signum(delta.y)}
            , radius(radius) {
            assert(get<Major>(step) != 0);
            assert(get<Minor>(step) == 0);
        }

        void fill(point pt, std::set<point> &pts) const override {
            get<Minor>(pt) -= radius;

            for (std::size_t m = 1; m < 2 * radius; ++m) {
                ++get<Minor>(pt);
                pts.insert(pt);
            }
        }

        void advance(point &point) override {
            get<Major>(point) += get<Major>(step);
        }
    };

    /// @brief A general Bresenham-Murphy thick line scanner.
    ///
    /// The Bresenham scanner is designed to handle lines of any slope.
    /// It performs the Bresenham line drawing algorithm between two
    /// points, and makes the line thick by recursively applying
    /// Bresenham to draw perpendicular segments through the points,
    /// using the state of the algorithm to optimially set up the
    /// second draw.
    ///
    /// The @c major template parameter controls the orientation of the
    /// line. If it is zero, the line is horizontal; if it is one, the
    /// line is vertical.
    ///
    /// @see http://www.zoo.co.uk/murphy/thickline/
    /// @see http://kt8216.unixcab.org/murphy/index.html
    ///
    /// @tparam Major the axis along which the point will be moved
    template <std::size_t Major>
    class bresenham_scanner : public scanner {
        static constexpr std::size_t Minor = 1U - Major;

        const point _delta, _step, _perp_step;

        const double _width;

        const long _threshold =
            get<Major>(_delta) - 2 * get<Minor>(_delta);
        const long _post_minor_move = -2 * get<Major>(_delta);
        const long _post_major_move = 2 * get<Minor>(_delta);

        long _error = 0, _phase = 0;

        void perpendiculars(point pt, std::set<point> &pts,
                            long initial_phase, long initial_error) const {
            auto p = pt;
            auto phase = initial_phase;

            const auto d =
                get<Major>(_delta) + get<Minor>(_delta);

            for (auto tk = d - initial_error; tk < _width;
                 tk -= _post_minor_move) {
                pts.insert(p);

                if (phase >= _threshold) {
                    get<Major>(p) += get<Major>(_perp_step);
                    phase += _post_minor_move;
                    tk += _post_major_move;
                }

                get<Minor>(p) += get<Minor>(_perp_step);
                phase += _post_major_move;
            }

            p = pt;
            phase = -initial_phase;

            for (auto tk = d + initial_error; tk <= _width;
                 tk -= _post_minor_move) {
                pts.insert(p);

                if (phase > _threshold) {
                    get<Major>(p) -= get<Major>(_perp_step);
                    phase += _post_minor_move;
                    tk += _post_major_move;
                }

                get<Minor>(p) -= get<Minor>(_perp_step);
                phase += _post_major_move;
            }
        }

      public:
        bresenham_scanner(point delta, std::size_t radius)
            : _delta{std::abs(delta.x), std::abs(delta.y)}
            , _step{signum(delta.x), signum(delta.y)}
            , _perp_step{Major == 0 ? -_step.x : +_step.x,
                         Major == 0 ? +_step.y : -_step.y}
            , _width{2.0 * radius * std::hypot(_delta.x, _delta.y)} {
        }

        void fill(point pt, std::set<point> &pts) const override {
            perpendiculars(pt, pts, _phase, _error);

            if (_error >= _threshold && _phase >= _threshold) {
                get<Minor>(pt) += get<Minor>(_step);
                perpendiculars(
                    pt, pts,
                    (_phase + _post_minor_move + _post_major_move),
                    _error + _post_minor_move);
            }

            if (pts.empty()) pts.insert(pt);
        }

        void advance(point &pt) override {
            if (_error >= _threshold) {
                get<Minor>(pt) += get<Minor>(_step);
                _error += _post_minor_move;

                if (_phase >= _threshold) { _phase += _post_minor_move; }

                _phase += _post_major_move;
            }

            get<Major>(pt) += get<Major>(_step);
            _error += _post_major_move;
        }
    };

    /// @brief Construct the appropriate scanner based on the delta
    /// vector.
    ///
    /// This is a virtual constructor that selects which instance to
    /// create by examining the delta vector. It determines which axis
    /// has the greater rate of change — the @em major axis — and if
    /// there is any change along the minor axis.  If the rate of
    /// change along the minor axis is zero, then the function returns
    /// a scanner that is optimized for that.
    ///
    /// @param delta the change between the start and end points
    /// @param args additional arguments for the constructor
    template <class... Args>
    static inline std::unique_ptr<class scanner>
    make_scanner(point delta, Args &&...args) {
        class scanner *impl;

        if (std::abs(delta.x) > std::abs(delta.y)) { // major == 0
            if (delta.y == 0)
                impl = new axial_scanner<0>{delta,
                                            std::forward<Args>(args)...};
            else
                impl = new bresenham_scanner<0>{
                    delta, std::forward<Args>(args)...};
        }
        else { // major == 1
            if (delta.x == 0)
                impl = new axial_scanner<1>{delta,
                                            std::forward<Args>(args)...};
            else
                impl = new bresenham_scanner<1>{
                    delta, std::forward<Args>(args)...};
        }

        return std::unique_ptr<class scanner>{impl};
    }

  public:
    /// @brief Construct a channel with given radius between two points.
    ///
    /// The @c radius parameter controls how wide the channel is. It is
    /// the half-width of the channel @em including the reference pixel.
    /// For example, a radius of 3 would create a channel 5 pixels
    /// wide.
    ///
    /// @param p0 the start of the channel
    /// @param p1 the end of the channel
    /// @param radius the width of the channel
    channel(const point &p0, const point &p1, std::size_t radius)
        : _p0(p0)
        , _p1(p1)
        , _radius(radius) {
        if (std::equal_to<point>{}(p0, p1)) {
            throw std::runtime_error("endpoints must be separated");
        }
    }

    /// @brief The workhorse of the channel.
    ///
    /// The iterator contains a point @c p and an internal state based
    /// on the slope @c delta of the line. It is an input iterator:
    /// there is no multipass guarantee.  The reference returned by
    /// the dereferencing operator is to an internal variable and will
    /// not be valid after increment.
    ///
    /// The value of the iterator is a pair consisting of the
    /// canonical point (the point on the ideal Bresenham line
    /// segment) and a set of points describing a perpendicular line
    /// segment through that point of length <code>2 * radius -
    /// 1</code>.  In very rare cases the canonical point will not be
    /// a part of the perpendicular point set but will be adjacent.
    /// The thickness of the perpendicular segment varies between 1
    /// and 2 pixels, depending on the position of the canonical point
    /// and the slope of the segment. These segments are guaranteed to
    /// cover the entire thick line segment and not overlap.
    class iterator {
      public:
        /// The type of values returned by dereferencing the iterator.
        using value_type = std::pair<point, std::set<point>>;

        /// A constant reference to the value returned.
        using reference = const value_type &;

        /// A constant pointer to the value returned.
        using pointer = const value_type *;

        /// The difference between two iterators.
        using difference_type = std::ptrdiff_t;

        /// The category of this iterator class.
        using iterator_category = std::input_iterator_tag;

      private:
        /// @brief The iterator implementation to use.
        std::unique_ptr<class scanner> scanner;

        /// @brief The radius of the channel.
        std::size_t radius;

        /// @brief The current state of the iterator.
        ///
        /// This is a mutable field because it is calculated on-demand
        /// from "constant" methods.
        mutable value_type value;

      public:
        /// @brief Construct an iterator following the given segment.
        ///
        /// The iterator will originate from point @c p and move in
        /// the direction specified by @c delta.  When it reaches
        /// <code>p + delta</code> it will have the same internal
        /// state that it had from the start.
        ///
        /// @param p the initial point
        /// @param delta the difference between the initial and terminal point
        /// @param radius the radius (half-width) of the line segment
        iterator(const point &p, const point &delta, std::size_t radius)
            : scanner(make_scanner(delta, radius))
            , value(p, {}) {}

        iterator(const iterator &) = delete;

        /// @brief Move constructor.
        iterator(iterator &&r)
            : scanner(std::move(r.scanner))
            , radius(r.radius)
            , value(std::get<0>(r.value), {}) {}

        iterator &operator=(const iterator &) = delete;

        /// @brief Move assignment operator.
        iterator &operator=(iterator &&r) {
            scanner = std::move(r.scanner);
            radius = r.radius;
            std::get<0>(value) = std::get<0>(r.value);
            std::get<1>(value).clear();

            return *this;
        }

        /// @brief Dereference the iterator.
        ///
        /// Returns a reference to the internal state of the iterator.
        /// If the state is incomplete because the iterator was just
        /// created or incremented, rebuilds the state from the
        /// available information.
        ///
        /// @return a reference to a @c value_type pair
        reference operator*() const {
            if (std::get<1>(value).empty()) {
                scanner->fill(std::get<0>(value), std::get<1>(value));
            }
            return value;
        }

        /// @brief Indirectly access the state of the iterator.
        ///
        /// Returns a pointer to the internal state of the iterator.
        /// If the state is incomplete because the iterator was just
        /// created or incremented, rebuilds the state from the
        /// available information.
        ///
        /// @return a pointer to a @c value_type pair
        pointer operator->() const {
            return &(operator*());
        }

        /// @brief Compare two iterators for equality.
        ///
        /// Two iterators are considered equal if the canonical point
        /// they reference is the same by value.
        ///
        /// @param r the other iterator
        /// @return true if the iterators reference the same point
        bool operator==(const iterator &r) const {
            return std::equal_to<point>{}(std::get<0>(value),
                                            std::get<0>(r.value));
        }
        /// @brief Compare two iterators for inequality.
        ///
        /// Equivalent to <code>!(*this == r).</code>
        ///
        /// @param r the other iterator
        /// @return true if the iterators reference different points
        bool operator!=(const iterator &r) const {
            return !operator==(r);
        }

        /// @brief Advance the iterator.
        ///
        /// All references returned by this iterator prior to this
        /// operation are invalid.
        ///
        /// @return the iterator
        iterator &operator++() {
            std::get<1>(value).clear();
            scanner->advance(std::get<0>(value));
            return *this;
        }
    };

    /// @brief Create an iterator to the starting point.
    ///
    /// @return an iterator
    iterator begin() const {
        point delta{_p1.x - _p0.x, _p1.y - _p0.y};
        return iterator{_p0, delta, _radius};
    }

    /// @brief Create an iterator one past the ending point.
    ///
    /// @return an iterator
    iterator end() const {
        point delta{_p1.x - _p0.x, _p1.y - _p0.y};
        return std::move(++iterator{_p1, delta, _radius});
    }
};

} // namespace ppht

#endif

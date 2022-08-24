// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_accumulator_hpp
#define ppht_accumulator_hpp

#include <ppht/raster.hpp>
#include <ppht/trig.hpp>
#include <ppht/types.hpp>
#include <ppht/state.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ppht {

/**
 * @brief A collection of counters for line candidates.
 *
 * The accumulator class keeps track of the matrix of counters
 * (indexed by their @f$\theta\rho@f$ values) and provides operations
 * for incrementing and decrementing the values, as well as converting
 * a line in @f$\theta\rho@f$-form to a line segment within the bounds
 * of the image.
 *
 * @tparam Count the type used for the counters
 *
 * @tparam Raster the type used for the matrix of counters
 */
template <class Count = std::uint16_t,
          template <class> class Raster = raster>
class accumulator {
    /// @brief A uniform random bit generator.
    using URBG = std::default_random_engine;

    /// @brief The type of seed for the @ref URBG.
    using seed_t = std::random_device::result_type;

    /// @brief Exponent by which to scale raw rho values.
    int const _rho_scale;

    /// @brief Log-probability threshold for rejecting the null
    /// hypothesis.
    double _log_threshold = std::log(1E-12);

    /// @brief The minimum number of points required to trigger a
    /// scan.
    Count _min_trigger_points = 3;

    /// @brief Matrix of counters (quantized @f$\theta\rho@f$-space).
    Raster<Count> _counters;

    /// @brief Votes still in effect.
    Count _votes = 0;

    /// @brief Random number generator.
    URBG _urbg;

    /**
     * @brief Convert a raw rho value to an index value.
     *
     * Scale rho according to @ref _rho_scale, then add half the
     * height of the counter matrix to it.
     *
     * @param unscaled_rho the raw rho value
     *
     * @return a rho value that is scaled and translated
     *
     * @sa unscale_rho()
     */
    double scale_rho(double unscaled_rho) const noexcept {
        double const offset = _counters.rows() >> 1;
        return std::rint(std::scalbn(unscaled_rho, _rho_scale) + offset);
    }

    /**
     * @brief Convert a scaled rho value back to a raw value.
     *
     * Perform the transform of @ref scale_rho() in reverse.
     *
     * @param scaled_rho the scaled rho value
     *
     * @return a rho value that is no longer scaled or translated
     *
     */
    double unscale_rho(double scaled_rho) const noexcept {
        double const offset = _counters.rows() >> 1;
        return std::scalbn(scaled_rho - offset, -_rho_scale);
    }

    /**
     * @brief Restrict the value to the range of the target type.
     *
     *  If the value is greater than the target type will allow,
     * return the maximum value of the target type.  If the value is
     * less than the target type will allow, return the minimum value
     * of the target type.
     *
     * @tparam T the target type
     *
     * @tparam U the source type
     *
     * @param value the value to convert
     *
     * @return the value restricted to the limits of the target type
     */
    template <class T, class U>
    static constexpr T clamp(U value) noexcept {
        using limit = std::numeric_limits<T>;

        if (value >= static_cast<U>(limit::max())) {
            return limit::max();
        }
        if (value <= static_cast<U>(limit::lowest())) {
            return limit::lowest();
        }

        return static_cast<T>(value);
    }

    /**
     * @brief Construct an instance of @ref accumulator.
     *
     * This constructor is called with the results of manipulation of
     * the members of the @ref parameters object: the logarithm of the
     * threshold and the computed range and scale factor for rho.
     *
     * @param rho_info the maximum value a scaled rho can take and the
     *   scale factor
     *
     * @param seed the seed for the URBG
     */
    accumulator(const std::pair<std::size_t, int> &rho_info,
                seed_t seed) noexcept
        : _rho_scale(rho_info.second)
        , _counters(rho_info.first, max_theta)
        , _urbg(seed) {}

  public:
    /**
     * @brief Calculate parameters for scaling rho.
     *
     * Returns a pair containing the maximum scaled value rho can take
     * on (height of the counter matrix) and the exponent by which rho
     * should be scaled beforehand. The maximum scaled value is always
     * an odd number.
     *
     * If the pair returned is @f$(m, e)@f$, then
     * @f$\rho_\text{scaled} =
     * \rho_\text{unscaled}\cdot2^e+(m-1)/2@f$.
     *
     * @param rows the height of the bitmap in pixels
     *
     * @param cols the width of the bitmap in pixels
     *
     * @return a pair containing scaling information for rho
     */
    static std::pair<std::size_t, int>
    rho_info(std::size_t rows, std::size_t cols) noexcept {
        double diag = std::ceil(std::hypot(rows - 1, cols - 1));
        int rho_exp = std::ilogb(max_theta / (diag * 2.0 + 1.0));

        // lo is 2 * diag * (1 << rho_exp).

        std::size_t lo = std::ceil(std::scalbn(diag, rho_exp + 1)) + 1;

        // hi is 2 * diag * (2 << rho_exp).  This is the smallest
        // rho_exp such that 2 * diag * (1 << rho_exp) >= max_theta.

        std::size_t hi = std::ceil(std::scalbn(diag, rho_exp + 2)) + 1;

        assert(lo <= max_theta && max_theta <= hi);

        // Select the rho_exp that makes the counter matrix the most
        // square.

        if ((max_theta - lo) <= (hi - max_theta)) {
            return std::make_pair(lo, rho_exp);
        }
        else {
            return std::make_pair(hi, rho_exp + 1);
        }
    }

    /**
     * @brief Construct an instance of @ref accumulator.
     *
     * @param rows the height of the bitmap
     *
     * @param cols the width of the bitmap
     *
     * @param seed the seed for the random number generator used to
     *        break ties.
     */
    accumulator(std::size_t rows, std::size_t cols,
                seed_t seed = std::random_device{}())
        : accumulator(rho_info(rows, cols), seed) {}

    /// @brief Get the probability threshold for rejecting the null
    /// hypothesis.
    double threshold() const noexcept {
        return std::exp(_log_threshold);
    }

    /// @brief Set the probability threshold for rejecting the null
    /// hypothesis.
    ///
    /// If the probability of the null hypothesis is less than this
    /// value, then a channel scan will be triggered.  Lowering this
    /// value results in fewer false positives but increases the
    /// chance of missing small segments.
    ///
    /// @param threshold the new threshold
    ///
    /// @sa min_trigger_points(double)
    void threshold(double threshold) noexcept {
        _log_threshold = std::log(threshold);
    }

    /// @brief Get the minimum number of voting points required to
    /// trigger a scan.
    double min_trigger_points() const noexcept {
        return _min_trigger_points;
    }

    /// @brief Set the minimum number of voting points required to
    /// trigger a scan.
    ///
    /// The simplification that the Poisson distribution approximates
    /// the likelihood of random noise breaks down for small numbers.
    /// Skip the probability calculation if the number of votes is
    /// less than this.
    ///
    /// @param min_trigger_points the new count required to trigger a scan
    ///
    /// @sa threshold(double)
    void min_trigger_points(double min_trigger_points) noexcept {
        _min_trigger_points = min_trigger_points;
    }

    /// @brief Find lines that are parallel to major axes.
    ///
    /// Returns the line most likely to be correct based on its angle.
    /// A line is more "correct" if its angle is a simple fraction of
    /// π: ½, ⅔, ⅘, &c.
    ///
    /// @param begin the start of the range
    /// @param end the end of the range
    ///
    /// @return an iterator to the best candidate line
    template <class ForwardIt>
    ForwardIt best_candidate(ForwardIt begin, ForwardIt end) {
        auto best = std::gcd(std::get<0>(*begin), max_theta / 2);

        for (auto iter = std::next(begin); iter != end; ++iter) {
            auto gcd = std::gcd(std::get<0>(*iter), max_theta / 2);
            if (best < gcd) {
                begin = iter;
                best = gcd;
            }
        }

        return begin;
    }

    /**
     * @brief Add all lines passing through the given point to the
     * accumulator.
     *
     * Returns a value if the likelihood of the largest count in the
     * register exceeds the threshold.
     *
     * @param p the point to register
     *
     * @return an optional that has a value if the number of votes for
     * the line segment pass the threshold
     *
     * @see unvote()
     */
    std::optional<line_t> vote(point_t const &p) {
        auto const max_rho = _counters.rows();

        Count n = _min_trigger_points;

        std::vector<line_t> found;

        std::size_t theta;
        double rho;

        // Increment the cells in the register, keeping track of the
        // current maxima.

        for (theta = 0; theta < max_theta; ++theta) {
            rho = scale_rho(p.dot(cossin[theta]));
            if (rho < 0 || rho >= max_rho) continue;

            auto &counter = _counters[rho][theta];

            ++counter;

            // If we have found a new maxima, update n and discard the
            // old candidates.

            if (n < counter) {
                n = counter;
                found.clear();
            }

            if (n == counter) {
                found.emplace_back(theta, unscale_rho(rho));
            }
        }

        ++_votes;

        // If we do not have a candidate line, stop.

        if (found.empty()) return std::nullopt;

        // There are max_theta * max_rho cells in the register.
        // Each vote will increment max_theta of these cells, one
        // per column.
        //
        // Assuming the null hypothesis (the image is random noise),
        // E[n] = votes/max_rho for all cells in the register.

        double const lambda = static_cast<double>(_votes) / max_rho;

        // For the null hypothesis, the cells are filled (roughly)
        // according to a Poisson model:
        //
        //    p(n) = λⁿ/n!·exp(-λ)
        //         = λⁿ/Γ(n+1)·exp(-λ)
        // ln p(n) = n·ln(λ) - lnΓ(n+1) - λ

        double const lnp =
            n * std::log(lambda) - std::lgamma(n + 1) - lambda;

        // lnp is the log probability that a bin filled randomly would
        // contain a count of n. If the probability is above the
        // significance threshold, we assume that the bin was filled
        // by noise and tell the caller we did not find a segment.

        if (lnp >= _log_threshold) return std::nullopt;

        // Reject the null hypothesis.

        assert(!found.empty());

        return *best_candidate(found.begin(), found.end());
    }

    /**
     * Update the register by undoing a previous call to @ref vote().
     *
     * @param p the point to unregister
     */
    void unvote(point_t const &p) {
        auto const max_rho = _counters.rows();

        for (std::size_t theta = 0; theta < max_theta; ++theta) {
            double const rho = scale_rho(p.dot(cossin[theta]));
            if (rho < 0 || rho >= max_rho) continue;

            auto &counter = _counters[rho][theta];

            if (counter == 0) {
                throw std::logic_error{__func__};
            }

            --counter;
        }

        --_votes;
    }
};

} // namespace ppht

#endif /* ppht_accumulator_hpp */

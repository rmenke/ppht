#ifndef ppht_accumulator_hpp
#define ppht_accumulator_hpp

#include <ppht/parameters.hpp>
#include <ppht/raster.hpp>
#include <ppht/trig.hpp>
#include <ppht/types.hpp>

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
    /// A uniform random bit generator.
    using URBG = std::default_random_engine;

    /// The type of seed for the @ref URBG.
    using seed_t = std::random_device::result_type;

    /// Trigonometry tables quantized by max_theta.
    trig_table const _trig;

    /// Height of bitmap image.
    std::size_t const _rows;

    /// Width of bitmap image.
    std::size_t const _cols;

    /// Exponent by which to scale raw rho values.
    int const _rho_scale;

    /// Log-probability threshold for rejecting the null hypothesis.
    double const _log_threshold;

    /// Number of points required to trigger a channel scan.
    Count const _min_trigger_points;

    /// Matrix of counters (quantized @f$\theta\rho@f$-space).
    Raster<Count> _counters;

    /// Votes still in effect.
    Count _votes = 0;

    /// Random number generator.
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
    static T clamp(U value) noexcept {
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
     * @param rows the height of the image
     *
     * @param cols the width of the image
     *
     * @param max_theta the number of parts per semiturn
     *
     * @param rho_info the maximum value a scaled rho can take and the
     *   scale factor
     *
     * @param log_threshold the natural logarithm of the threshold at
     *   which we reject the null hypothesis
     *
     * @param min_trigger_points the number of colinear points
     *   required before testing the null hypothesis
     *
     * @param seed the seed for the URBG
     */
    accumulator(std::size_t rows, std::size_t cols, std::size_t max_theta,
                const std::pair<std::size_t, int> &rho_info,
                double log_threshold, std::size_t min_trigger_points,
                seed_t seed) noexcept
        : _trig(max_theta)
        , _rows(rows)
        , _cols(cols)
        , _rho_scale(rho_info.second)
        , _log_threshold(log_threshold)
        , _min_trigger_points(min_trigger_points)
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
     * @param max_theta the range of theta
     *
     * @return a pair containing scaling information for rho
     */
    static std::pair<std::size_t, int>
    rho_info(std::size_t rows, std::size_t cols,
             std::size_t max_theta) noexcept {
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
     * @param param parameters controlling the operation of the accumulator
     *
     * @param seed the seed for the random number generator used to
     *        break ties.
     */
    accumulator(std::size_t rows, std::size_t cols, parameters const &param,
                seed_t seed = std::random_device{}())
        : accumulator(rows, cols, param.max_theta,
                      rho_info(rows, cols, param.max_theta),
                      std::log(param.threshold), param.min_trigger_points,
                      seed) {}

    /**
     * @brief Find the portion of the line that lies within the bounds
     * of the bitmap.
     *
     * Given a line in @f$\theta\rho@f$-space, return the line segment
     * that is the portion of the line that intersects the bitmap.
     *
     * @param theta the angle of the perpendicular
     *
     * @param rho the length of the perpendicular
     *
     * @return the portion of the line within the bounds of the bitmap
     * in integral coordinates.
     */
    segment_t find_segment(std::size_t theta, double rho) {
        // There are a few degenerate cases where multiple matches for
        // the same endpoint can be found, e.g., a line through the
        // origin.  Using a set eliminates most of these cases.  See
        // the comment at the end for what happens to those that slip
        // through.

        std::set<point_t> endpoints;

        auto const &cossin = _trig[theta];
        double const &sin_theta = cossin[1];
        double const &cos_theta = cossin[0];

        auto get_x = [&](double y) -> long {
            double x = std::rint((rho - sin_theta * y) / cos_theta);
            return clamp<long>(x);
        };

        auto get_y = [&](double x) -> long {
            double y = std::rint((rho - cos_theta * x) / sin_theta);
            return clamp<long>(y);
        };

        auto x_min = get_x(0);
        auto y_min = get_y(0);
        auto x_max = get_x(_rows - 1);
        auto y_max = get_y(_cols - 1);

        if (0 <= y_min && y_min < static_cast<int>(_rows)) {
            endpoints.emplace(0, y_min);
        }
        if (0 <= x_min && x_min < static_cast<int>(_cols)) {
            endpoints.emplace(x_min, 0);
        }
        if (0 <= y_max && y_max < static_cast<int>(_rows)) {
            endpoints.emplace(_cols - 1, y_max);
        }
        if (0 <= x_max && x_max < static_cast<int>(_cols)) {
            endpoints.emplace(x_max, _rows - 1);
        }

        if (endpoints.empty()) {
            throw std::logic_error{"line (" + std::to_string(theta) + ", " +
                                   std::to_string(rho) +
                                   ") does not intersect bitmap"};
        }

        // If endpoints.size() > 2, ignore the points in the middle.
        // If endpoints.size() == 1, create single-pixel segment.

        return segment_t{*endpoints.begin(), *endpoints.rbegin()};
    }

    /**
     * @brief Add all lines passing through the given point to the
     * accumulator.
     *
     * Returns true if the likelihood of the largest count in the
     * register exceeds the threshold.
     *
     * @param p the point to register
     *
     * @param segment set to the intersection of the line found and
     *   the bounds of the image only if the function returns true
     *
     * @return true if the number of votes for the line segment pass
     *   the threshold
     *
     * @see unvote()
     */
    bool vote(point_t const &p, segment_t &segment) {
        auto const max_rho = _counters.rows();
        auto const max_theta = _counters.cols();

        Count n = _min_trigger_points;

        using line_t = std::pair<std::size_t, double>;

        std::vector<line_t> found;

        std::size_t theta;
        double rho;

        for (theta = 0; theta < max_theta; ++theta) {
            rho = scale_rho(p.dot(_trig[theta]));
            if (rho < 0 || rho >= max_rho) continue;

            auto &counter = _counters[rho][theta];

            ++counter;

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

        if (found.empty()) return false;

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

        if (lnp >= _log_threshold) return false;

        // Reject the null hypothesis.

        if (found.size() == 1) {
            std::tie(theta, rho) = found.at(0);
        }
        else {
            auto begin = found.begin();
            auto end = found.end();

            if (begin->first == 0 && (end-1)->first == max_theta - 1) {
                // If we have values at both ends of the vector there
                // is a chance that we have a "wraparound" cluster.
                // Since every line (θ, ρ) has a counterpart of (θ +
                // π/2, -ρ), replace the points at the beginning of
                // the vector with their counterparts, effectively
                // transplanting the points with angles in quadrant I
                // into points with angles in quadrant III.

                for (auto iter = begin; iter != end; ++iter) {
                    if (iter->first < max_theta / 2) {
                        iter->first += max_theta;
                        iter->second = -(iter->second);
                    }
                    else {
                        break;
                    }
                }

                std::sort(begin, end);
            }

            // Recalculate the maximum distance between the first and
            // last in the cluster.

            auto const diff = (end-1)->first - begin->first + 1;

            // If the difference is the same as the number of points
            // in the cluster, then the range is continuous and it is
            // safe to take the median value.  Otherwise, return.

            if (diff != found.size()) return false;

            auto median = begin + diff / 2;
            std::tie(theta, rho) = *median;

            // If the median is in quadrant III, move it back to
            // quadrant I.

            if (theta >= max_theta) {
                theta -= max_theta, rho = -rho;
            }
        }

        segment = find_segment(theta, rho);
        return true;
    }

    /**
     * Update the register by undoing a previous call to @ref vote().
     *
     * @param p the point to unregister
     */
    void unvote(point_t const &p) {
        auto const max_rho = _counters.rows();
        auto const max_theta = _counters.cols();

        for (std::size_t theta = 0; theta < max_theta; ++theta) {
            double const rho = scale_rho(p.dot(_trig[theta]));
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

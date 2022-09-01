// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

/**
 * @mainpage
 *
 * @section intro_sec Introduction
 *
 * PPHT is a header-only library that implements the Probabilistic
 * Progressive Hough Transform (PPHT) Line Segment Detection
 * algorithm. This library uses the dynamic threshold approach
 * proposed by the authors of the original paper.
 *
 * @subsection theory About the Algorithm
 *
 * The formula for a line @f$L@f$ passing through the point @f$(x_0,
 * y_0)@f$ is @f$\Delta y(x-x_0) = \Delta x(y-y_0).@f$ Unlike the
 * traditional slope-intercept form, this equation handles vertical
 * lines. Assume @f$(x_0, y_0)@f$ is the point where the perpendicular
 * @f$P@f$ to the line that passes through the origin intersects
 * @f$L.@f$ @f$P@f$ makes an angle @f$\theta@f$ with the x-axis;
 * therefore, the angle @f$L@f$ makes with the axis is
 * @f$\theta+\frac{\pi}{2}.@f$ From this, we get @f$\Delta
 * y=\sin(\theta+\frac{\pi}{2})=\cos\theta@f$ and @f$\Delta
 * x=\cos(\theta+\frac{\pi}{2})=-\sin\theta.@f$ We can also let
 * @f$x_0=\rho\cos\theta@f$ and @f$y_0=\rho\sin\theta@f$ for some
 * unknown @f$\rho.@f$ Plugging these values into the original
 * equation and solving for @f$\rho@f$ yields:
 *
 * @f{align}{
 * 0 &= \Delta y(x-x_0) - \Delta x(y-y_0)\\
 *   &= (x-x_0)\cos\theta + (y-y_0)\sin\theta\\
 *   &= (x-\rho\cos\theta)\cos\theta + (y-\rho\sin\theta)\sin\theta\\
 *   &= x\cos\theta - \rho\cos^2\theta + y\sin\theta - \rho\sin^2\theta\\
 *   &= x\cos\theta + y\sin\theta - \rho\\
 * \rho &= x\cos\theta + y\sin\theta
 * @f}
 *
 * This implies that every line can be defined by the pair
 * @f$(\theta,\rho)@f$ and for a given @f$x@f$, @f$y@f$, and
 * @f$\theta@f$, @f$\rho@f$ can be calculated. If we restrict
 * @f$0\le\theta<\pi@f$ then every line has a unique representation as
 * a point in a different space. Equivalently, every point in
 * @f$xy@f$-space is a sinusoid in @f$\theta\rho@f$-space. This is
 * known as the <em>Hough transform.</em> Given a set of points in
 * @f$xy@f$-space, if their sinusoids intersect at a point in
 * @f$\theta\rho@f$-space then that point represents the line on which
 * all of the points are located.  Locating those intersections is the
 * key to the <em>Hough Transform Line Detection</em> algorithm.
 *
 * The algorithm works by quantizing the @f$\theta@f$ and @f$\rho@f$
 * values, iterating over the possible values of @f$\theta@f$ for
 * every set pixel, finding the quantized value for @f$\rho@f$, and
 * incrementing a counter in an accumulator array.  The peaks in this
 * array are the lines on which a large number of pixels reside.
 * Locating these peaks is where the algorithm has the most trouble.
 *
 * <em>Progressive Hough Transform</em> algorithms work by randomly
 * sampling the set pixels in a bitmap image and incrementing the
 * counters.  When a counter reaches a given threshold (either fixed
 * or computed) the algorithm pauses, scans the line corresponding to
 * the counter, and removes the pixels from the bitmap for any line
 * segments found.
 *
 * The <em>Probabilistic Progressive Hough Transform</em> (PPHT) uses
 * combinatorics to determine the threshold for a counter to trigger a
 * scan.  It starts by assuming the null hypothesis: the image is a
 * collection of random pixels.  Since each vote increments a counter
 * in each column @f$(\theta)@f$ of the accumulator, the expected
 * value for any counter after @f$v@f$ votes would be @f$E[n] =
 * \frac{v}{\rho_{\max}}@f$, where @f$\rho_{\max}@f$ is the height of
 * the counter matrix.
 *
 * The simplifying assumption that we make is that the counters are
 * selected by a uniform process.  For this, a binomial distribution
 * would be used to calculate the probability of a bin containing
 * @f$n@f$ after @f$v@f$ voting rounds.  However, the probability mass
 * function for a binomial distribution is expensive to compute, so
 * instead we approximate the probability by using the Poisson
 * distribution instead.  Setting @f$\lambda = E[n]@f$, we get
 *
 * @f{align}{
 *     P(n)&=\frac{\lambda^n}{n!}e^{-\lambda}\\
 *         &=\frac{\lambda^n}{\Gamma(n+1)}e^{-\lambda}\\
 * \ln P(n)&=\ln(\lambda^n)-\ln\Gamma(n+1)-\lambda\\
 *         &=n\cdot\ln(\lambda)-\ln\Gamma(n+1)-\lambda
 * @f}
 *
 * If the value @f$P(n)@f$ is less than a predetermined threshold
 * @f$t@f$ -- or equivalently @f$\ln P(N)<\ln(t)@f$ -- then we reject
 * the null hypothesis and search along the line described by
 * @f$(\theta,\rho)@f$ for a line segment.
 */

#ifndef ppht_hpp
#define ppht_hpp

#include "accumulator.hpp"
#include "point_set.hpp"
#include "postprocess.hpp"
#include "raster.hpp"
#include "state.hpp"

#include <cstdint>

/**
 * @namespace ppht
 *
 * @brief The basic namespace of the library.
 */
namespace ppht {

/**
 * @brief Simplified interface to the PPHT algorithm.
 *
 * This function performs the full PPHT algorithm by iterating over
 * the set points in the @c state matrix.  The state matrix is
 * destroyed in the process.
 *
 * Typical use case:
 *
 * @code
 * ppht::state state;
 *
 * for (auto y = 0; y < height; ++y) {
 *   for (auto x = 0; x < width; ++x) {
 *     if (is_set(bitmap, x, y)) {
 *       state.mark_pending({x, y});
 *     }
 *   }
 * }
 *
 * auto segments = ppht::find_segments(std::move(state));
 * @endcode
 *
 * @tparam State the class of the state parameter
 *
 * @tparam Accumulator the class to use for the accumulator
 *
 * @param state an initialized @ref ppht::state object or something
 * similar
 *
 * @param seed a value to use as a seed for the URBG
 *
 * @returns a vector of line segments
 */
template <class State, class Accumulator = accumulator<>>
std::vector<std::pair<point, point>> find_segments(
    State &&state, std::uint16_t channel_width = 3,
    std::uint16_t max_gap = 3, std::uint16_t min_length = 10,
    std::random_device::result_type seed = std::random_device{}()) {
    const auto min_length_squared = min_length * min_length;

    std::vector<std::pair<point, point>> segments;

    Accumulator accumulator{state.rows(), state.cols(), seed};

    point pt;

    const auto channel_radius = channel_width >> 1;

    while (state.next(pt)) {
        if (auto result = accumulator.vote(pt); result) {
            auto found = state.scan(*result, channel_radius, max_gap);

            if (found.length_squared() >= min_length_squared) {
                for (auto &&point : found) {
                    auto status = state.status(point);

                    if (status == status_t::voted) {
                        accumulator.unvote(point);
                    }
                    else {
                        if (status != status_t::pending) {
                            using namespace std;
                            throw std::logic_error{
                                "Unexpected pixel with status "s +
                                to_string(status)};
                        }
                    }

                    state.mark_done(point);
                }

                segments.push_back(found.endpoints());
            }
        }
    }

    segments.erase(
        postprocess(segments.begin(), segments.end(), channel_radius),
        segments.end());

    return segments;
}

} // namespace ppht

#endif /* ppht_hpp */

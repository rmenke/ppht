// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_parameters_hpp
#define ppht_parameters_hpp

#include <cstdint>

namespace ppht {

/**
 * @brief The tunable parameters of the analyzer.
 *
 * Reasonable defaults have been supplied.  The consistency of the
 * values is not checked: correctness is the responsibility of the
 * developer.
 */
struct parameters {
    /**
     * @brief The minimum number of voting points required to
     * trigger a scan.
     *
     * The simplification that the Poisson distribution
     * approximates the likelihood of random noise breaks down for
     * small numbers.  Skip the calculation if the number of votes
     * is less than this.
     */
    std::uint16_t min_trigger_points = 3;

    /**
     * @brief The minimum probability that the null hypothesis is
     * true.
     *
     * If the probability of the null hypothesis is less than this
     * value, then a channel scan will be triggered.  Lowering
     * this value results in fewer false positives but increases
     * the chance of missing small segments.
     */
    double threshold = 1E-12;

    /**
     * @brief The width of the scan channel.
     */
    std::uint16_t channel_width = 3;

    /**
     * @brief The maximum number of pixels that can be absent
     * within a continuous segment.
     *
     * Gaps in a segment occur when previous scans erase pixels.
     * If two segments intersect, then the second segment to be
     * scanned will be missing @ref channel_width pixels but that
     * gap is spurious.  If more than this number of pixels are
     * missing during a channel scan, the scanner closes the
     * current segment and begins a new one.
     *
     * This value should be no less than @ref channel_width.
     */
    std::uint16_t max_gap = 3;

    /**
     * @brief The minimum length of a significant segment.
     *
     * This value is measured in pixels, but is not a guarantee
     * about the number of pixels that the segment will use.
     *
     * @sa min_trigger_points
     */
    std::uint16_t min_length = 10;

    /**
     * @brief Chained constructor operation.
     *
     * @param min_trigger_points the new value for @ref min_trigger_points
     *
     * @return the parameters object
     */
    parameters &set_min_trigger_points(std::uint16_t min_trigger_points) {
        this->min_trigger_points = min_trigger_points;
        return *this;
    }

    /**
     * @brief Chained constructor operation.
     *
     * @param threshold the new value for @ref threshold
     *
     * @return the parameters object
     */
    parameters &set_threshold(double threshold) {
        this->threshold = threshold;
        return *this;
    }

    /**
     * @brief Chained constructor operation.
     *
     * @param channel_width the new value for @ref channel_width
     *
     * @return the parameters object
     */
    parameters &set_channel_width(std::uint16_t channel_width) {
        this->channel_width = channel_width;
        return *this;
    }

    /**
     * @brief Chained constructor operation.
     *
     * @param max_gap the new value for @ref max_gap
     *
     * @return the parameters object
     */
    parameters &set_max_gap(std::uint16_t max_gap) {
        this->max_gap = max_gap;
        return *this;
    }

    /**
     * @brief Chained constructor operation.
     *
     * @param min_length the new value for @ref min_length
     *
     * @return the parameters object
     */
    parameters &set_min_length(std::uint16_t min_length) {
        this->min_length = min_length;
        return *this;
    }
};

} // namespace ppht

#endif /* ppht_parameters_hpp */

#ifndef ppht_trig_hpp
#define ppht_trig_hpp

#include <cassert>
#include <cmath>
#include <cstdint>
#include <math.h>
#include <memory>
#include <utility>

namespace ppht {

/**
 * @brief A precomputed table of sine and cosine values.
 *
 * A @c trig_table contains sine and cosine values for the semiturn
 * (180°) indexed by a user-specified measurement called "parts."
 */
class trig_table {
    /// Pair describing sine and cosine value for a given measurement.
    using sincos_t = std::pair<double, double>;

    /// The table of values.
    std::unique_ptr<sincos_t[]> _data;

  public:
    /**
     * @brief Number of parts per semiturn.
     *
     * 1 turn = 2 * @c max_theta parts.
     */
    const std::size_t max_theta;

    /**
     * @brief Construct a @c trig_table.
     *
     * A @c max_theta of 180 would give a table indexed by degrees.
     * The parts per semiturn @b must be divisible by two, and should
     * be a power of two.
     *
     * @param max_theta parts per semiturn
     */
    explicit trig_table(std::size_t max_theta)
        : _data(new sincos_t[max_theta])
        , max_theta(max_theta) {
        if (max_theta % 2 != 0) {
            throw std::invalid_argument{"max_theta not even"};
        }

        auto const radians_per_part =
            4.0 * std::atan2(1, 1) / static_cast<double>(max_theta);

        for (std::size_t theta = 0; theta < max_theta / 2; ++theta) {
            auto const angle = theta * radians_per_part;
            auto const s = sin(angle), c = cos(angle);

            _data[theta] = sincos_t{s, c};
            _data[theta + max_theta / 2] = sincos_t{c, -s};
        }
    }

    /**
     * Get the sine-cosine pair for the given angle.  @c theta must be
     * in [0, <code>max_theta</code>).
     *
     * @param theta the angle, measured in parts per turn.
     *
     * @return a pair with the sine and cosine for the given angle.
     *
     * @see max_theta
     */
    const sincos_t &operator[](std::size_t theta) const {
        assert(theta < max_theta);
        return _data[theta];
    }
};

} // namespace ppht

#endif /* ppht_accumulator_hpp */

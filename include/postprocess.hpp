// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_postprocess_hpp
#define ppht_postprocess_hpp

#ifndef PPHT_DEBUG
#    define PPHT_DEBUG(...) do {} while (0)
#endif

#include "trig.hpp"
#include "types.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <optional>
#include <queue>
#include <vector>

namespace ppht {

/// @brief Modified <em>kd</em>-search for line segments.
///
/// Finds the segments whose first point is within @p limit of the
/// reference point @p p.
///
/// @note This function reorders the elements in the range.
///
/// @param begin the start of the range
/// @param end the end of the range
/// @param out the output iterator
/// @param p the reference point
/// @param limit the radius of the neighborhood
///
/// @return the output iterator
///
template <std::size_t Dim, class RandomIt, class OutputIt, class Point>
static inline auto find_nearest(RandomIt begin, RandomIt end, OutputIt out,
                                Point &&p, std::size_t limit) {
    if (begin == end) return out;

    RandomIt mid = begin + std::distance(begin, end) / 2;
    assert(mid != end);

    std::nth_element(begin, mid, end, [](auto &&a, auto &&b) {
        return get<Dim>(a.first) < get<Dim>(b.first);
    });

    PPHT_DEBUG("    dividing plane through ", (Dim ? "y" : "x"), " = ",
               get<Dim>(mid->first), "; candidates ",
               std::distance(begin, end));

    auto d_plane = get<Dim>(p) - get<Dim>(mid->first);

    const bool is_lo = d_plane <= +static_cast<long>(limit);
    const bool is_hi = d_plane >= -static_cast<long>(limit);

    if (is_lo && is_hi) {
        const auto d_other = get<1 - Dim>(p) - get<1 - Dim>(mid->first);

        if (d_plane * d_plane + d_other * d_other <= limit * limit) {
            PPHT_DEBUG("    found ", *mid);
            *out = *mid;
            ++out;
        }
    }

    if (is_lo) out = find_nearest<1 - Dim>(begin, mid, out, p, limit);
    if (is_hi) out = find_nearest<1 - Dim>(mid + 1, end, out, p, limit);

    return out;
}

/// @brief Combine segments that are near-colinear.
///
/// For a given @em directed segment @f$\overrightarrow{ab}@f$,
/// another directed segment @f$\overrightarrow{cd}@f$ is considered
/// @em near-colinear if @f$\|a-c\|\le\mathtt{limit}@f$ and the angle
/// @f$\angle axd=\pi\,\mathrm{rad}\pm\varepsilon@f$ where
/// @f$x=f(b,c)@f$ (usually the average of the two points) and
/// @f$\varepsilon@f$ is the angle tolerance.  The postprocessor would
/// replace both segments with a new segment @f$\overrightarrow{ad}@f$
/// then restart the process.
///
/// @param begin the start of the range of segments
/// @param end the end of the range of segments
/// @param limit the maximum distance between two points that are
///   considered “close”
/// @param tolerance the maximum deviation from 180° measured in
///   thetas
///
/// @return the output parameter
///
template <class ForwardIt>
ForwardIt postprocess(ForwardIt begin, ForwardIt end, std::size_t limit,
                      std::size_t tolerance) {
    const double threshold = cossin.at(tolerance).y - 1.0;
    std::vector<segment> segments;

    // For each segment, insert a copy pointing in the original and
    // opposite directions.  Each segment copy is equivalent (under
    // "==") to the original segment because a segment is an unordered
    // pair; however, the "first" member (the key point) is different
    // for each copy.

    for (auto i = std::next(begin); i != end; ++i) {
        segments.emplace_back(i->first, i->second);
        segments.emplace_back(i->second, i->first);
    }

    for (auto i = begin; i != end; ++i) {
        PPHT_DEBUG("extending segment ", *i);

        auto &[a, b] = *i;

        // Remove the original and flipped counterparts of "*i" from
        // the segment set otherwise they will lead to unnecessary
        // matches.

        segments.erase(std::remove(segments.begin(), segments.end(), *i),
                       segments.end());

        if (segments.empty()) break;

        std::vector<segment> found;

    extend_head:
        found.clear();

        PPHT_DEBUG("  (head) looking for neighbors of ", a);
        find_nearest<0>(segments.begin(), segments.end(),
                        std::back_inserter(found), a, limit);

        if (found.empty()) {
            PPHT_DEBUG("  (head) no candidates found");
        }
        else {
            PPHT_DEBUG("  (head) found ", found);

            for (auto &s : found) {
                PPHT_DEBUG("  (head) examining segment ", s);

                auto &[c, d] = s;

                const auto e = (a + c) / 2.0;
                const auto v1 = b - e;
                const auto v2 = d - e;

                const auto cs = v1.dot(v2) / v1.length() / v2.length();

                PPHT_DEBUG("  (head) cosine of angle is ", cs,
                           "; threshold ", threshold);

                if (cs <= threshold) {
                    a = d;

                    end = std::remove(begin, end, s);

                    auto segments_end =
                        std::remove(segments.begin(), segments.end(), s);
                    segments.erase(segments_end, segments.end());

                    PPHT_DEBUG("  (tail) segment now is ", *i);

                    goto extend_head;
                }
                else {
                    PPHT_DEBUG("  (head) rejected");
                }
            }
        }

    extend_tail:
        found.clear();

        PPHT_DEBUG("  (tail) looking for neighbors of ", b);
        find_nearest<0>(segments.begin(), segments.end(),
                        std::back_inserter(found), b, limit);

        if (found.empty()) {
            PPHT_DEBUG("  (tail) no candidates found");
        }
        else {
            PPHT_DEBUG("  (tail) found ", found);

            for (auto &s : found) {
                PPHT_DEBUG("  (tail) examining segment ", s);

                auto &[c, d] = s;

                const auto e = (b + c) / 2.0;
                const auto v1 = a - e;
                const auto v2 = d - e;

                const auto cs = v1.dot(v2) / v1.length() / v2.length();

                PPHT_DEBUG("  (tail) cosine of angle is ", cs);

                if (cs <= threshold) {
                    b = d;

                    end = std::remove(begin, end, s);

                    auto segments_end =
                        std::remove(segments.begin(), segments.end(), s);
                    segments.erase(segments_end, segments.end());

                    PPHT_DEBUG("  (tail) segment now is ", *i);

                    goto extend_tail;
                }
                else {
                    PPHT_DEBUG("  (tail) rejected");
                }
            }
        }
    }

    return end;
}

} // namespace ppht

#endif /* ppht_postprocess_hpp */

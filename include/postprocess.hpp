// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_postprocess_hpp
#define ppht_postprocess_hpp

// clang-format off
#ifndef PPHT_DEBUG
#    define PPHT_DEBUG(...) do {} while (0)
#endif
// clang-format on

#include "trig.hpp"
#include "types.hpp"

#include <vector>

namespace ppht {

/// @brief A utility to fuse nearly colinear segments.
///
/// PPHT tends to sacrifice finding full segments because of its
/// random sampling. The end result is often a number of short
/// segments where a long segment would have been found.  The
/// postprocessor looks for segments along the same line that are
/// adjacent and combines them.
///
/// Segments are considered colinear if one segment's endpoint is
/// within a given distance to one of the other segment's endpoints
/// and the angle between the segments is near to 180°.
///
class postprocessor {
    /// @brief Cached cosine of the angle allowed for near
    /// colinearity.
    ///
    /// Making this a mutable field avoids the need for an extra
    /// parameter to extend_head().
    ///
    mutable double cosine_threshold;

  public:
    /// @brief The space between nearby points.
    std::size_t gap_limit = 2;

    /// @brief The allowed angle in thetas between segments still
    /// considered colinear.
    ///
    /// Segments are considered colinear if one segment's endpoint is
    /// nearby the other segment's endpoint and the angle between the
    /// segments at least <code>max_theta - angle_tolerance</code>.
    std::size_t angle_tolerance = 40;

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
    ///
    /// @return the output iterator
    ///
    template <std::size_t Dim = 0, class RandomIt, class OutputIt,
              class Point>
    OutputIt find_nearest(RandomIt begin, RandomIt end, OutputIt out,
                          Point &&p) const {
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

        const bool is_lo = d_plane <= +static_cast<long>(gap_limit);
        const bool is_hi = d_plane >= -static_cast<long>(gap_limit);

        if (is_lo && is_hi) {
            const auto d_other = get<1 - Dim>(p) - get<1 - Dim>(mid->first);
            const auto d_squared = d_other * d_other + d_plane * d_plane;

            if (d_squared <= gap_limit * gap_limit) {
                PPHT_DEBUG("    found ", *mid);
                *out = *mid;
                ++out;
            }
        }

        if (is_lo) out = find_nearest<1 - Dim>(begin, mid, out, p);
        if (is_hi) out = find_nearest<1 - Dim>(mid + 1, end, out, p);

        return out;
    }

    /// @brief Extend a directed segment by replacing its head endpoint.
    ///
    /// @param current an iterator pointing at the current segment
    /// @param end the end of the undirected segments
    /// @param segments a cache of directed segments
    ///
    template <class ForwardIt>
    ForwardIt extend_head(ForwardIt current, ForwardIt end,
                          std::vector<segment> &segments) const {
        if (current == end) return end;

        PPHT_DEBUG("extending segment ", *current);

        auto sbeg = segments.begin();
        auto send = std::remove(sbeg, segments.end(), *current);

        auto &[tail1, head1] = *current;

        std::vector<segment> found;

        PPHT_DEBUG("  looking for neighbors of ", head1, "...");

        find_nearest(sbeg, send, std::back_inserter(found), head1);

        if (found.empty()) {
            PPHT_DEBUG("  none found.");
            return end;
        }

        PPHT_DEBUG("  found ", found);

        for (auto const &seg : found) {
            auto &[tail2, head2] = seg;

            PPHT_DEBUG("    examining ", seg);

            auto const mdpt = (head1 + tail2) / 2.0;
            auto const vec1 = (tail1 - mdpt);
            auto const vec2 = (head2 - mdpt);

            double const cosine =
                vec1.dot(vec2) / vec1.length() / vec2.length();

            PPHT_DEBUG("    cosine is ", cosine, " (threshold ",
                       cosine_threshold, ")");

            if (cosine <= cosine_threshold) {
                PPHT_DEBUG("      merging ", *current, " and ", seg);

                head1 = head2;

                end = std::remove(current, end, seg);
                send = std::remove(sbeg, send, seg);

                segments.erase(send, segments.end());

                return extend_head(current, end, segments);
            }
        }

        return end;
    }

    /// @brief Combine segments that are near-colinear.
    ///
    /// For all of the segments @f$\overline{ab}@f$ in the range,
    /// generate the @em directed segments @f$\vec{ab}@f$ and
    /// @f$\vec{ba}.@f$ For every pair of directed segments
    /// @f$\vec{ab}@f$ and @f$\vec{cd}@f$, if the angle between the
    /// two vectors is less than or equal to @ref angle_tolerance and
    /// the distance between @f$b@f$ and @f$c@f$ is less than or equal
    /// to @ref gap_limit then replace the equivalent @em undirected
    /// segments @f$\overline{ab}@f$ and @f$\overline{cd}@f$ with a
    /// new segment @f$\overline{ad}.@f$
    ///
    /// @param begin the start of the range of segments
    /// @param end the end of the range of segments
    ///
    /// @return the new end of the range of segments
    ///
    template <class ForwardIt>
    ForwardIt operator()(ForwardIt begin, ForwardIt end) const {
        if (begin == end) return end;

        cosine_threshold = - cossin.at(angle_tolerance).x;

        PPHT_DEBUG("Updating cosine_threshold = ", cosine_threshold);

        std::vector<segment> segments;

        for (auto iter = std::next(begin); iter != end; ++iter) {
            auto const &[tail, head] = *iter;

            segments.emplace_back(tail, head);
            segments.emplace_back(head, tail);
        }

        for (auto iter = begin; iter != end; ++iter) {
            auto &[tail, head] = *iter;

            end = extend_head(iter, end, segments);
            std::swap(tail, head);
            end = extend_head(iter, end, segments);
            std::swap(tail, head);
        }

        return end;
    }
};

} // namespace ppht

#endif /* ppht_postprocess_hpp */

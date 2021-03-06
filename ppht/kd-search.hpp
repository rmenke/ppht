#ifndef ppht_kd_search_hpp
#define ppht_kd_search_hpp

#include <ppht/types.hpp>

#include <algorithm>

namespace ppht {

/**
 * Perform a modified kd-tree search on the elements of the given
 * range.
 *
 * Given a list of points [@c begin, @c end) and a reference point @c
 * p, finds the intersection of the list and the closed disc centered
 * at @c p with radius @c limit.
 *
 * The template parameter @c Dim controls the scan mode of the
 * function.  The default 0 value starts the search by partitioning
 * along the x-axis; a 1 would partition along the y-axis.  Each
 * recursive call toggles the mode.
 *
 * @note The elements of the range will be shuffled as part of the
 * search.  This may invalidate iterators.
 *
 * @param begin the start of the sequence to search
 * @param end the end of the sequence to search
 * @param output where to store the results of the search
 * @param p the center of the disc
 * @param limit the radius of the disc
 * @tparam Dim the axis along which the range will be partitioned
 */
template <std::size_t Dim = 0, class RandomIt, class OutputIt>
OutputIt kd_search(RandomIt begin, RandomIt end, OutputIt output,
                   const point_t &p, long limit) {
    if (begin == end) return output;

    // Divide the points into approximately equal sets using the line
    // perpendicular to the Dim axis through the point midpt (the
    // first component of the element pointed to by median).  Points
    // on the separating line may appear on either side of the median
    // element.

    auto median = begin + std::distance(begin, end) / 2;

    std::nth_element(begin, median, end, [] (auto &&a, auto &&b) -> bool {
        point_t &pta = std::get<0>(a), &ptb = std::get<0>(b);
        return std::get<Dim>(pta) < std::get<Dim>(ptb);
    });

    point_t &midpt = std::get<0>(*median);

    // If the midpoint is within the limit, add it to the output.

    if ((p - midpt).length_squared() <= limit * limit) {
        *output = *median; ++output;
    }

    // Get the signed distance between the line and the point.  The
    // sign determines on which side of the line the point lies; the
    // magnitude determines if the neighborhood defined by the disc
    // overlaps both sides.
    //
    // If d_plane < -limit, the disc fully resides on the "before"
    // side.  No points on the "after" side will be in the
    // neighborhood.
    //
    // If -limit <= d_plane <= +limit, the neighborhood disc crosses
    // the separating line so both sides must be searched.
    //
    // If d_plane > +limit, the disc fully resides on the "after"
    // side.  No points on the "before" side will be in the
    // neighborhood.

    auto d_plane = std::get<Dim>(p) - std::get<Dim>(midpt);

    if (d_plane <= limit) {
        output = kd_search<1 - Dim>(begin, median, output, p, limit);
    }
    if (d_plane >= -limit) {
        output = kd_search<1 - Dim>(median + 1, end, output, p, limit);
    }

    return output;
}

}

#endif

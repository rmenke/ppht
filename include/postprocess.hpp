// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_postprocess_hpp
#define ppht_postprocess_hpp

#include "kd-search.hpp"
#include "types.hpp"

#include <cassert>
#include <queue>
#include <vector>

namespace ppht {

template <class RandomIt>
static inline auto find_nearest(RandomIt begin, RandomIt end,
                                const point &p, unsigned limit) {
    std::vector<typename std::iterator_traits<RandomIt>::value_type> result;
    kd_search(begin, end, std::back_inserter(result), p, limit);
    return result;
}

static inline auto distance_to_line_squared(const point &pnt1,
                                            const point &pnt2) {
    using namespace std;

    const auto delta = pnt2 - pnt1;

    const double px = get<0>(pnt1);
    const double py = get<1>(pnt1);
    const double dx = get<0>(delta);
    const double dy = get<1>(delta);

    const double D = delta.length_squared();

    return [px, py, dx, dy, D] (const point &p0) {
        const double N = dx * (py - get<1>(p0)) - dy * (px - get<0>(p0));
        return (N * N) / D;
    };
}

template <class ForwardIt>
ForwardIt postprocess(ForwardIt begin, ForwardIt end, unsigned limit) {
    using namespace std;

    const auto limit_squared = limit * limit;

    for (auto i = begin; i != end; ++i) {
        point &a = i->first;
        point &b = i->second;

        using tuple_t = tuple<reference_wrapper<point>, reference_wrapper<point>, ForwardIt>;

        vector<tuple_t> pool;

        for (auto j = i + 1; j != end; ++j) {
            point &c = j->first;
            point &d = j->second;

            pool.emplace_back(c, d, j);
            pool.emplace_back(d, c, j);
        }

        auto bp = pool.begin();
        auto ep = pool.end();

        for (auto pass = 1; pass <= 2; ++pass) {
        restart:
            auto neighbors = find_nearest(bp, ep, b, limit);

            for (auto &&neighbor : neighbors) {
                point &c = get<0>(neighbor);
                point &d = get<1>(neighbor);

                // The order of the points along the new segment will
                // be a - b ~ c - d

                auto d2l = distance_to_line_squared(a, d);

                if (d2l(b) > limit_squared) continue;
                if (d2l(c) > limit_squared) continue;

                // All four points are (near) colinear, and
                // ‖b - c‖ < limit

                b = d;
                std::iter_swap(get<2>(neighbor), --end);

                auto past_end = [&] (auto &&t) -> bool {
                    return get<2>(t) == end;
                };

                ep = std::remove_if(bp, ep, past_end);

                goto restart;
            }

            std::swap(a, b);
        }
    }

    return end;
}

}

#endif /* ppht_postprocess_hpp */

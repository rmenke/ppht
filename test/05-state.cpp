// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include "state.hpp"

#include <iostream>
#include <vector>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

} // namespace std

static void test_intersection() {
    using namespace tap;

    ppht::state s(240, 320);

    std::pair<ppht::point, ppht::point> expected{{0, 141}, {141, 0}};
    std::pair<ppht::point, ppht::point> actual =
        s.line_intersect({900, 100});

    eq(expected, actual, "simple intersection");

    expected = std::make_pair(ppht::point{44, 239}, ppht::point{283, 0});
    actual = s.line_intersect({900, 200});

    eq(expected, actual, "truncated intersection 1");

    expected = std::make_pair(ppht::point{185, 239}, ppht::point{319, 105});
    actual = s.line_intersect({900, 300});

    eq(expected, actual, "truncated intersection 2");

    expected = std::make_pair(ppht::point{0, 0}, ppht::point{0, 0});
    actual = s.line_intersect({900, 0});

    eq(expected, actual, "degenerate intersection 1");

    expected = std::make_pair(ppht::point{0, 0}, ppht::point{239, 239});
    actual = s.line_intersect({2700, 0});

    eq(expected, actual, "degenerate intersection 2");

    try {
        actual = s.line_intersect({900, 1000});
        fail("no intersection");
    }
    catch (...) {
        pass("no intersection");
    }
}

int main() {
    using namespace tap;

    test_plan plan{13};

    ppht::state state(5, 5);

    bool all_clear = true;
    ppht::point p;

    for (p.y = 0U; p.y < static_cast<long>(state.rows()); ++p.y) {
        for (p.x = 0U; p.x < static_cast<long>(state.cols()); ++p.x) {
            all_clear =
                all_clear && state.status(p) == ppht::status_t::unset;
        }
    }

    ok(all_clear, "initialization");

    state.mark_pending({3, 2});

    eq(ppht::status_t::pending, state.status({3, 2}), "marked as pending");

    ok(state.next(p), "fetch point");

    eq(ppht::point{3, 2}, p, "correct point");

    ok(!state.next(p), "no more points");

    eq(ppht::status_t::voted, state.status({3, 2}), "marked as voted");

    state.mark_done({3, 2});

    eq(ppht::status_t::done, state.status({3, 2}), "marked as done");

    test_intersection();
}

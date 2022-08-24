// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include <ppht/state.hpp>

#include <iostream>
#include <vector>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

} // namespace std

void test_intersection() {
    using namespace tap;

    ppht::state<> s(240, 320);

    ppht::segment_t expected{{0, 141}, {141, 0}};
    ppht::segment_t actual = s.line_intersect({900, 100});

    eq(expected, actual, "simple intersection");

    expected = ppht::segment_t{{44, 239}, {283, 0}};
    actual = s.line_intersect({900, 200});

    eq(expected, actual, "truncated intersection 1");

    expected = ppht::segment_t{{185, 239}, {319, 105}};
    actual = s.line_intersect({900, 300});

    eq(expected, actual, "truncated intersection 2");

    expected = ppht::segment_t{{0, 0}, {0, 0}};
    actual = s.line_intersect({900, 0});

    eq(expected, actual, "degenerate intersection 1");

    expected = ppht::segment_t{{0, 0}, {239, 239}};
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

    ppht::state<> state(5, 5);

    bool all_clear = true;
    ppht::point_t p;

    for (p[1] = 0U; p[1] < static_cast<long>(state.rows()); ++p[1]) {
        for (p[0] = 0U; p[0] < static_cast<long>(state.cols()); ++p[0]) {
            all_clear = all_clear &&
                state.status(p) == ppht::status_t::unset;
        }
    }

    ok(all_clear, "initialization");

    state.mark_pending({3, 2});

    eq(ppht::status_t::pending, state.status({3, 2}), "marked as pending");

    ok(state.next(p), "fetch point");

    eq(ppht::point_t{3, 2}, p, "correct point");

    ok(!state.next(p), "no more points");

    eq(ppht::status_t::voted, state.status({3, 2}), "marked as voted");

    state.mark_done({3, 2});

    eq(ppht::status_t::done, state.status({3, 2}), "marked as done");

    test_intersection();
}

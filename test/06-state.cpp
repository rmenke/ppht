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

int main() {
    using namespace tap;

    test_plan plan;

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

    auto found = ppht::find_offsets({{0, 0}, {4, 4}}, 0);
    eq(1, found.size(), "one point");
    eq(ppht::point_t(0L, 0L), *found.begin(), "zero offset");

    found = ppht::find_offsets({{0, 0}, {4, 4}}, 1);
    eq(3, found.size(), "one point");

    auto iter = found.begin();

    eq(ppht::point_t(-1, 1), *iter, "neg offset");
    ++iter;
    eq(ppht::point_t(0, 0), *iter, "zero offset");
    ++iter;
    eq(ppht::point_t(1, -1), *iter, "pos offset");

    return test_status();
}

// Copyright (C) 2020-2022 by Rob Menke

#include "point_set.hpp"
#include "tap.hpp"
#include "types.hpp"

#include <iostream>
#include <map>
#include <vector>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ',' << p.second << ')';
}

} // namespace std

namespace mock {

struct state {
    std::map<ppht::point, ppht::status_t> points;

    state()
        : points({{{3, 5}, ppht::status_t::voted},
                  {{4, 4}, ppht::status_t::voted},
                  {{5, 7}, ppht::status_t::pending},
                  {{6, 6}, ppht::status_t::voted}}) {}

    ppht::status_t status(ppht::point const &p) {
        return points.at(p);
    }

    void mark_done(ppht::point const &p) {
        tap::ne(points.at(p), ppht::status_t::unset, "not unset ",
                to_string(p));
        tap::ne(points.at(p), ppht::status_t::done, "not done ",
                to_string(p));
        points.at(p) = ppht::status_t::done;
    }
};

struct accumulator {
    std::set<ppht::point> points = {{3, 5}, {4, 4}, {6, 6}};

    void unvote(ppht::point const &p) {
        tap::eq(points.erase(p), 1, "unvote once ", to_string(p));
    }
};

} // namespace mock

int main() {
    using namespace tap;

    test_plan plan = 24;

    ppht::point_set ps;

    ps.add_point({5, 5}, std::vector<ppht::point>{{4, 4}, {6, 6}});

    eq(0, ps.length_squared(), "length of one point is zero");
    eq(ppht::segment(ppht::point{5, 5}, ppht::point{5, 5}), ps.segment(),
       "canonical segment updated");

    auto singular = ps;

    ps.add_point({4, 6}, std::vector<ppht::point>{{3, 5}, {5, 7}});

    eq(2, ps.length_squared(), "length updated");
    eq(ppht::segment(ppht::point{5, 5}, ppht::point{4, 6}), ps.segment(),
       "canonical segment updated");

    ps.add_point({3, 7}, std::vector<ppht::point>{{3, 5}});

    eq(8, ps.length_squared(), "length updated");
    eq(ppht::segment(ppht::point{5, 5}, ppht::point{3, 7}), ps.segment(),
       "canonical segment updated");

    lt(ppht::point_set{}, singular,
       "empty segment is \"shorter\" than nonempty segment");
    lt(singular, ps, "segments ordered by length");

    mock::state state;
    mock::accumulator accumulator;

    std::move(ps).commit(state, accumulator);

    ok(accumulator.points.empty(), "all votes undone");

    eq(state.points.at({3, 5}), ppht::status_t::done, "marked done 1");
    eq(state.points.at({4, 4}), ppht::status_t::done, "marked done 2");
    eq(state.points.at({5, 7}), ppht::status_t::done, "marked done 3");
    eq(state.points.at({6, 6}), ppht::status_t::done, "marked done 4");
}

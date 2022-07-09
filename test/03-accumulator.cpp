// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include <ppht/accumulator.hpp>
#include <ppht/types.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <ostream>
#include <random>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, pair<F, S> const &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

template <class T>
static inline ostream &operator<<(ostream &o, vector<T> const &v) {
    auto b = v.begin();
    auto e = v.end();

    o << '[';

    if (b != e) {
        o << *b;
        while (++b != e)
            o << ", " << *b;
    }

    return o << ']';
}

} // namespace std

using seed_t = std::random_device::result_type;

void test_rho_scaling() {
    using namespace tap;
    using accumulator = ppht::accumulator<>;

    auto max_rho = accumulator::rho_info(10, 10, 1024);

    eq(833U, max_rho.first, "correct resizing 1");
    eq(5, max_rho.second, "correct scaling 1");

    max_rho = accumulator::rho_info(240, 320, 1024);

    eq(799U, max_rho.first, "correct resizing 2");
    eq(0, max_rho.second, "correct scaling 2");

    max_rho = accumulator::rho_info(240, 320, 256);

    eq(201U, max_rho.first, "correct resizing 3");
    eq(-2, max_rho.second, "correct scaling 3");
}

void test_intersection(seed_t seed) {
    using namespace tap;

    auto param = ppht::parameters{}.set_max_theta(1024);

    ppht::accumulator<> acc(240, 320, param, seed);

    ppht::segment_t expected{{0, 141}, {141, 0}};
    ppht::segment_t actual = acc.find_segment(256, 100);

    eq(expected, actual, "simple intersection");

    expected = ppht::segment_t{{44, 239}, {283, 0}};
    actual = acc.find_segment(256, 200);

    eq(expected, actual, "truncated intersection 1");

    expected = ppht::segment_t{{185, 239}, {319, 105}};
    actual = acc.find_segment(256, 300);

    eq(expected, actual, "truncated intersection 2");

    expected = ppht::segment_t{{0, 0}, {0, 0}};
    actual = acc.find_segment(256, 0);

    eq(expected, actual, "degenerate intersection 1");

    expected = ppht::segment_t{{0, 0}, {239, 239}};
    actual = acc.find_segment(768, 0);

    eq(expected, actual, "degenerate intersection 2");

    try {
        actual = acc.find_segment(256, 1000);
        fail("no intersection");
    }
    catch (...) {
        pass("no intersection");
    }
}

void test_voting(seed_t seed) {
    using namespace tap;

    unsigned point[300];

    std::default_random_engine urbg{seed};
    std::iota(std::begin(point), std::end(point), 50);
    std::shuffle(std::begin(point), std::end(point), urbg);

    ppht::segment_t segment;

    auto param = ppht::parameters{}.set_max_theta(4096);
    ppht::accumulator<> acc(240, 320, param, seed);

    bool segment_found = false;

    for (auto &&i : point) {
        segment_found = true;
        if (acc.vote(ppht::point_t{i, i}, segment)) break;
        segment_found = false;
    }

    ok(segment_found, "unique segment found 1");
    eq(std::make_pair(ppht::point_t{0, 0}, ppht::point_t{239, 239}),
       segment, "correct segment found 1");

    segment_found = false;

    for (auto &&i : point) {
        segment_found = true;
        if (acc.vote(ppht::point_t{i, i - 10}, segment)) break;
        segment_found = false;
    }

    ok(segment_found, "unique segment found 2");
    eq(std::make_pair(ppht::point_t{10, 0}, ppht::point_t{249, 239}),
       segment, "correct segment found 2");
}

void test_unvoting(seed_t seed) {
    using namespace tap;

    ppht::accumulator<> acc(240, 320, ppht::parameters{}, seed);

    try {
        ppht::segment_t segment;

        ok(!acc.vote(ppht::point_t{50, 50}, segment), "vote recorded");
        acc.unvote(ppht::point_t{50, 50});
        pass("no exception thrown");
    }
    catch (...) {
        fail("no exception thrown");
    }

    try {
        acc.unvote(ppht::point_t{50, 50});
        fail("exception thrown");
    }
    catch (...) {
        pass("exception thrown");
    }
}

int main() {
    using namespace tap;

    test_plan plan{19};

    // Make the test deterministic
    seed_t const seed = 696408486U;

    diag("random seed is ", seed);

    test_rho_scaling();
    test_intersection(seed);
    test_voting(seed);
    test_unvoting(seed);

    return test_status();
}

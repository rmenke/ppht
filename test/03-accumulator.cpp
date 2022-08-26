// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include "accumulator.hpp"
#include "types.hpp"

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
        while (++b != e) o << ", " << *b;
    }

    return o << ']';
}

} // namespace std

using seed_t = std::random_device::result_type;

void test_rho_scaling() {
    using namespace tap;
    using accumulator = ppht::accumulator<>;

    auto max_rho = accumulator::rho_info(10, 10);

    eq(3329U, max_rho.first, "correct resizing 1");
    eq(7, max_rho.second, "correct scaling 1");

    max_rho = accumulator::rho_info(240, 320);

    eq(3193U, max_rho.first, "correct resizing 2");
    eq(2, max_rho.second, "correct scaling 2");
}

void test_voting(seed_t seed) {
    using namespace tap;

    unsigned point[300];

    std::default_random_engine urbg{seed};
    std::iota(std::begin(point), std::end(point), 50);
    std::shuffle(std::begin(point), std::end(point), urbg);

    ppht::accumulator<> acc(240, 320, seed);

    std::optional<ppht::line_t> found;

    for (auto &&i : point) {
        if ((found = acc.vote({i, i}))) break;
    }

    if (ok(found.has_value(), "unique segment found 1")) {
        eq(ppht::line_t{2700, 0.0}, *found, "correct segment found 1");
    }
    else {
        fail("correct segment found 1");
    }

    found.reset();

    for (auto &&i : point) {
        if ((found = acc.vote({i, i - 10}))) break;
    }

    if (ok(found.has_value(), "unique segment found 2")) {
        eq(ppht::line_t{2700, -7.0}, *found, "correct segment found 2");
    }
    else {
        fail("correct segment found 2");
    }
}

void test_unvoting(seed_t seed) {
    using namespace tap;

    ppht::accumulator<> acc(240, 320, seed);

    try {
        ok(!acc.vote(ppht::point_t{50, 50}), "vote recorded");
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

    test_plan plan{11};

    // Make the test deterministic
    seed_t const seed = 696408486U;

    diag("random seed is ", seed);

    test_rho_scaling();
    test_voting(seed);
    test_unvoting(seed);
}

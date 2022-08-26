// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include "trig.hpp"

#include <cmath>
#include <ostream>
#include <utility>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

} // namespace std

template <class A, class B> static inline
bool eq_pair(A &&a, B &&b, double tolerance) {
    bool first = std::fabs(std::get<0>(a) - std::get<0>(b)) <= tolerance;
    bool second = std::fabs(std::get<1>(a) - std::get<1>(b)) <= tolerance;

    if (first && second) return true;

    if (!first) tap::diag(std::get<0>(b), " != ", std::get<0>(a), "±", tolerance);
    if (!second) tap::diag(std::get<1>(b), " != ", std::get<1>(a), "±", tolerance);

    return false;
}

int main() {
    using namespace tap;

    test_plan plan{6};

    eq(3600U, ppht::max_theta, "constant");

    ok(eq_pair(std::make_pair(1, 0), ppht::cossin[0], 1E-6), "cossin 1");
    ok(eq_pair(std::make_pair(0, 1), ppht::cossin[1800], 1E-6), "cossin 2");

    ok(eq_pair(std::make_pair(std::sqrt(0.5), std::sqrt(0.5)), ppht::cossin[900],
               1E-6),
       "hi-res");
    ok(eq_pair(std::make_pair(-std::sqrt(0.5), std::sqrt(0.5)), ppht::cossin[2700],
               1E-6),
       "hi-res");

    ok(eq_pair(std::make_pair(0.92387953, 0.38268343), ppht::cossin[450], 1E-6),
       "hi-res");
}

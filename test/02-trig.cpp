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
    using namespace std;

    bool first = fabs(get<0>(a) - get<0>(b)) <= tolerance;
    bool second = fabs(get<1>(a) - get<1>(b)) <= tolerance;

    if (first && second) return true;

    if (!first) tap::diag(get<0>(b), " != ", get<0>(a), "±", tolerance);
    if (!second) tap::diag(get<1>(b), " != ", get<1>(a), "±", tolerance);

    return false;
}

int main() {
    using namespace tap;

    test_plan plan{6};

    eq(3600U, ppht::max_theta, "constant");

    ok(eq_pair(ppht::coord<double>(1, 0), ppht::cossin[0], 1E-6), "cossin 1");
    ok(eq_pair(ppht::coord<double>(0, 1), ppht::cossin[1800], 1E-6), "cossin 2");

    ok(eq_pair(ppht::coord<double>(std::sqrt(0.5), std::sqrt(0.5)), ppht::cossin[900],
               1E-6),
       "hi-res");
    ok(eq_pair(ppht::coord<double>(-std::sqrt(0.5), std::sqrt(0.5)), ppht::cossin[2700],
               1E-6),
       "hi-res");

    ok(eq_pair(ppht::coord<double>(0.92387953, 0.38268343), ppht::cossin[450], 1E-6),
       "hi-res");
}

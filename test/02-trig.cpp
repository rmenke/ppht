#include <cmath>
#include <ostream>
#include <utility>

#include <tap.hpp>

#include <ppht/trig.hpp>

TAP_INITIALIZE;

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

} // namespace std

static inline
bool eq_pair(const ppht::vec2d<double> &a, const ppht::vec2d<double> &b,
             double tolerance) {
    bool first = std::fabs(a[0] - b[0]) <= tolerance;
    bool second = std::fabs(a[1] - b[1]) <= tolerance;

    if (first && second) return true;

    if (!first) tap::diag(b[0], " != ", a[0], "±", tolerance);
    if (!second) tap::diag(b[1], " != ", a[1], "±", tolerance);

    return false;
}

int main() {
    using namespace tap;

    test_plan plan{12};

    ppht::trig_table trig{1024};

    eq(1024U, trig.max_theta, "field initialized");

    ok(eq_pair(ppht::vec2d<double>(1, 0), trig[0], 1E-6), "hi-res");
    ok(eq_pair(ppht::vec2d<double>(0, 1), trig[512], 1E-6), "hi-res");

    ok(eq_pair(ppht::vec2d<double>(std::sqrt(0.5), std::sqrt(0.5)), trig[256],
               1E-6),
       "hi-res");
    ok(eq_pair(ppht::vec2d<double>(-std::sqrt(0.5), std::sqrt(0.5)), trig[768],
               1E-6),
       "hi-res");

    ok(eq_pair(ppht::vec2d<double>(0.92387953, 0.38268343), trig[128], 1E-6),
       "hi-res");

    ppht::trig_table t2{8};

    ok(eq_pair(ppht::vec2d<double>(1, 0), t2[0], 1E-6), "lo-res");
    ok(eq_pair(ppht::vec2d<double>(0, 1), t2[4], 1E-6), "lo-res");

    ok(eq_pair(ppht::vec2d<double>(std::sqrt(0.5), std::sqrt(0.5)), t2[2], 1E-6),
       "lo-res");
    ok(eq_pair(ppht::vec2d<double>(-std::sqrt(0.5), std::sqrt(0.5)), t2[6],
               1E-6),
       "lo-res");

    ok(eq_pair(ppht::vec2d<double>(0.92387953, 0.38268343), t2[1], 1E-6),
       "lo-res");

    try {
        ppht::trig_table{91};
        fail("exception thrown");
    } catch (...) {
        pass("exception thrown");
    }

    return test_status();
}

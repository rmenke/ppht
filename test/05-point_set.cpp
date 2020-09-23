#include <tap.hpp>

TAP_INITIALIZE;

#include <ppht/point_set.hpp>

#include <iostream>
#include <vector>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ',' << p.second << ')';
}

} // namespace std

namespace ppht {

static inline std::ostream &operator<<(std::ostream &o, const ppht::point_set &ps) {
    o << "ppht::point_set{";

    if (!ps.empty()) {
        o << "segment=" << ps.segment() << "; {";
        auto b = ps.begin();
        auto e = ps.end();
        if (b != e) {
            o << *b;
            while (++b != e) o << ',' << *b;
        }
        o << "}";
    }

    return o << "}";
}

}

int main() {
    using namespace tap;

    test_plan plan;

    ppht::point_set ps;

    ok(ps.empty(), "initially empty");
    eq(-1, ps.length_squared(), "length is indeterminate");

    ps.add_point({5,5}, std::vector<ppht::point_t>{{4,4}, {6,6}});

    ok(!ps.empty(), "no longer empty");
    eq(0, ps.length_squared(), "length of one point is zero");
    eq(ppht::segment_t{{5,5}, {5,5}}, ps.segment(),
       "canonical segment updated");

    auto singular = ps;

    ps.add_point({4,6}, std::vector<ppht::point_t>{{3,5}, {5,7}});

    eq(2, ps.length_squared(), "length updated");
    eq(ppht::segment_t{{5,5}, {4,6}}, ps.segment(),
       "canonical segment updated");

    ps.add_point({3,7}, std::vector<ppht::point_t>{{3,5}});

    eq(8, ps.length_squared(), "length updated");
    eq(ppht::segment_t{{5,5}, {3,7}}, ps.segment(),
       "canonical segment updated");

    lt(ppht::point_set{}, singular, "empty set is smaller than nonempty set");
    lt(singular, ps, "point_sets ordered by length");

    auto b = ps.begin();
    auto e = ps.end();

    ok(b != e, "iterator not done");
    eq(ppht::point_t{3,5}, *(b++), "point 1");
    ok(b != e, "iterator not done");
    eq(ppht::point_t{4,4}, *(b++), "point 2");
    ok(b != e, "iterator not done");
    eq(ppht::point_t{5,7}, *(b++), "point 3");
    ok(b != e, "iterator not done");
    eq(ppht::point_t{6,6}, *(b++), "point 4");
    ok(b == e, "iterator done");
}

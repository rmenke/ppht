#include <tap.hpp>

TAP_INITIALIZE;

#include <ppht/channel.hpp>

#include <cstdlib>
#include <random>

namespace std {

template <class F, class S>
static inline ostream &operator<<(ostream &o, const pair<F, S> &p) {
    return o << '(' << p.first << ", " << p.second << ')';
}

static inline ostream &operator<<(ostream &o,
                                  const ppht::channel::iterator &i) {
    return o << (i == ppht::channel::iterator{} ?
                 "(end iterator)" : "(iterator)");
}

} // namespace std

int main() {
    using namespace tap;

    test_plan plan(84);

    ppht::segment_t segment;

    ppht::channel channel({{5, 0}, {0, 5}});

    auto iter = channel.begin();
    auto end = channel.end();

    eq(ppht::point_t{0, 5}, *iter, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point_t{1,4}, *iter, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point_t{2,3}, *iter, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point_t{3,2}, *iter, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point_t{4,1}, *iter, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point_t{5,0}, *iter, "correct value");
    ++iter;
    eq(end, iter, "iterator done");

    iter = channel.begin();
    auto prev = iter;

    while (true) {
        if (++iter == end) break;
        ne(*prev, *iter, "advance");
        prev = iter;
    }

    // octent VIII

    segment = {{5, 0}, {0, 3}};

    auto result = ppht::make_scanner(segment);

    eq(ppht::segment_t{{0, 3}, {5, 0}}, segment, "segment reversed");

    #define X_AXIS &ppht::point_t::x, &ppht::point_t::y
    #define Y_AXIS &ppht::point_t::y, &ppht::point_t::x

    instanceof<ppht::bresenham_scanner<X_AXIS, -1>>(*result, "correct scanner");
    instanceof<ppht::bresenham_scanner<X_AXIS, -1>>(*result->clone(), "correct scanner copy");

    ppht::point_t p_old, p_new;

    p_old = p_new = segment.first; result->advance(p_new);
    eq(1, p_new.x, "x-coord increased by one");
    ge(p_old.y, p_new.y, "y-coord monotonically decreased");


    p_old = p_new; result->advance(p_new);
    eq(2, p_new.x, "x-coord increased by one");
    ge(p_old.y, p_new.y, "y-coord monotonically decreased");

    p_old = p_new; result->advance(p_new);
    eq(3, p_new.x, "x-coord increased by one");
    ge(p_old.y, p_new.y, "y-coord monotonically decreased");

    p_old = p_new; result->advance(p_new);
    eq(4, p_new.x, "x-coord increased by one");
    ge(p_old.y, p_new.y, "y-coord monotonically decreased");

    p_old = p_new; result->advance(p_new);
    eq(5, p_new.x, "x-coord increased by one");
    ge(p_old.y, p_new.y, "y-coord monotonically decreased");

    // y-axis

    segment = {{5, 0}, {5, 5}};

    result = ppht::make_scanner(segment);

    instanceof<ppht::axis_scanner<&ppht::point_t::y>>(*result, "correct scanner");
    instanceof<ppht::axis_scanner<&ppht::point_t::y>>(*result->clone(), "correct scanner copy");

    p_old = p_new = segment.first; result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    p_old = p_new;
    result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    p_old = p_new; result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    p_old = p_new; result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    p_old = p_new; result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    p_old = p_new; result->advance(p_new);
    eq(p_old.x, p_new.x, "x-coord unchanged");
    lt(p_old.y, p_new.y, "y-coord increased");

    // x-axis

    segment = {{0, 5}, {5, 5}};

    result = ppht::make_scanner(segment);

    instanceof<ppht::axis_scanner<&ppht::point_t::x>>(*result, "correct scanner");
    instanceof<ppht::axis_scanner<&ppht::point_t::x>>(*result->clone(), "correct scanner copy");

    p_old = p_new = segment.first; result->advance(p_new);
    eq(p_old.x + 1, p_new.x, "x-coord increased");
    eq(p_old.y, p_new.y, "y-coord unchanged");

    // octent I

    segment = {{0, 0}, {5, 3}};

    result = ppht::make_scanner(segment);

    instanceof<ppht::bresenham_scanner<&ppht::point_t::x, &ppht::point_t::y, +1>>(*result, "correct scanner");
    instanceof<ppht::bresenham_scanner<&ppht::point_t::x, &ppht::point_t::y, +1>>(*result->clone(), "correct scanner copy");

    p_old = p_new = segment.first; result->advance(p_new);

    while (std::not_equal_to<ppht::point_t>()(p_old, segment.second)) {
        eq(p_old.x + 1, p_new.x, "x-coord increased");
        le(p_old.y, p_new.y, "y-coord increased");
        p_old = p_new;
        result->advance(p_new);
    }

    // octent II

    segment = {{0, 0}, {3, 5}};

    result = ppht::make_scanner(segment);

    instanceof<ppht::bresenham_scanner<Y_AXIS, +1>>(*result, "correct scanner");
    instanceof<ppht::bresenham_scanner<Y_AXIS, +1>>(*result->clone(), "correct scanner copy");

    p_old = p_new = segment.first; result->advance(p_new);

    while (std::not_equal_to<ppht::point_t>()(p_old, segment.second)) {
        le(p_old.x, p_new.x, "x-coord increased");
        eq(p_old.y + 1, p_new.y, "y-coord increased");
        p_old = p_new;
        result->advance(p_new);
    }

    // octent III

    segment = {{0, 5}, {3, 0}};

    result = ppht::make_scanner(segment);

    instanceof<ppht::bresenham_scanner<Y_AXIS, -1>>(*result, "correct scanner");
    instanceof<ppht::bresenham_scanner<Y_AXIS, -1>>(*result->clone(), "correct scanner copy");

    p_old = p_new = segment.first; result->advance(p_new);

    while (std::not_equal_to<ppht::point_t>()(p_old, segment.second)) {
        ge(p_old.x, p_new.x, "x-coord decreased");
        eq(p_old.y + 1, p_new.y, "y-coord increased");
        p_old = p_new;
        result->advance(p_new);
    }
}

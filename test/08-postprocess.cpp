// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include "postprocess.hpp"

#include <random>

namespace std {

template <class T>
static inline ostream &print_tuple(ostream &o, const T &, index_sequence<>) {
    return o;
}

template <class T, std::size_t Ix, std::size_t... Ir>
static inline ostream &print_tuple(ostream &o, const T &t, index_sequence<Ix, Ir...>) {
    if (Ix > 0) o << ", ";
    return print_tuple(o << get<Ix>(t), t, index_sequence<Ir...>{});
}

template <class... T>
static inline ostream &operator <<(ostream &o, const tuple<T...> &t) {
    return print_tuple(o << '(', t, index_sequence_for<T...>{}) << ')';
}

template <class F, class S>
static inline ostream &operator <<(ostream &o, const pair<F, S> &p) {
    return print_tuple(o << '(', p, make_index_sequence<2>{}) << ')';
}

template <class T>
static inline ostream &operator <<(ostream &o, const vector<T> &v) {
    o << '[';

    auto b = v.begin();
    auto e = v.end();

    if (b != e) {
        o << *b;
        while (++b != e) o << ", " << *b;
    }

    return o << ']';
}

}

int main() {
    using namespace tap;

    test_plan plan;

    std::vector<std::pair<ppht::point_t, ppht::point_t>> segments;

    segments.push_back({{0, 0}, {50, 1}});
    segments.push_back({{100, 0}, {51, 0}});
    segments.push_back({{101, 1}, {150, 0}});

    auto end = ppht::postprocess(segments.begin(), segments.end(), 3.0);

    ok(segments.end() != end);

    segments.erase(end, segments.end());

    eq(1, segments.size());
    eq(std::make_pair(ppht::point_t{0, 0}, ppht::point_t{150, 0}), segments[0]);

    segments.clear();

    std::random_device rd;
    std::default_random_engine urbg{rd()};
    std::uniform_int_distribution<> dist{-1, 1};
    std::bernoulli_distribution flip;

    for (int i = 0; i < 4; ++i) {
        ppht::point_t a, b;

        a[0] = (i * 25) + dist(urbg);
        a[1] = (i * 25) + dist(urbg);
        b[0] = ((i+1) * 25) + dist(urbg);
        b[1] = ((i+1) * 25) + dist(urbg);

        if (flip(urbg)) std::swap(a, b);

        segments.emplace_back(a, b);

        a[0] = (i * 25) + dist(urbg);
        a[1] = dist(urbg);
        b[0] = ((i+1) * 25) + dist(urbg);
        b[1] = dist(urbg);

        if (flip(urbg)) std::swap(a, b);

        segments.emplace_back(a, b);
    }

    std::shuffle(segments.begin(), segments.end(), urbg);

    end = ppht::postprocess(segments.begin(), segments.end(), 4);

    ok(segments.end() != end);

    segments.erase(end, segments.end());

    if (!eq(2, segments.size())) {
        diag(segments);
    }

    return test_status();
}

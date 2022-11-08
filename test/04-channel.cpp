// Copyright (C) 2020-2022 by Rob Menke

#include "channel.hpp"
#include "tap.hpp"
#include "types.hpp"

#include <cstdlib>
#include <random>

namespace std {

template <class F, class S>
inline ostream &operator<<(ostream &os, const pair<F, S> &p) {
    return os << '(' << p.first << ", " << p.second << ')';
}

template <class T>
inline ostream &operator<<(ostream &os, const set<T> &s) {
    auto b = s.begin();
    auto e = s.end();

    os << "{";

    if (b != e) {
        os << *b;
        while (++b != e) os << ", " << *b;
    }

    return os << "}";
}

inline ostream &operator<<(ostream &os, const ppht::channel::iterator &i) {
    return os << "iterator{" << (*i) << "}";
}

} // namespace std

int main() {
    using namespace tap;

    test_plan plan;

    ppht::channel channel{{0, 5}, {5, 0}, 1};

    auto iter = channel.begin();
    auto end = channel.end();

    eq(ppht::point{0, 5}, iter->first, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point{1, 4}, iter->first, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point{2, 3}, iter->first, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point{3, 2}, iter->first, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point{4, 1}, iter->first, "correct value");
    ++iter;
    ne(end, iter, "iterator not done");

    eq(ppht::point{5, 0}, iter->first, "correct value");
    ++iter;
    eq(end, iter, "iterator done");
}

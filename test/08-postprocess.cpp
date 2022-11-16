// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#define PPHT_DEBUG(...) tap::diag("DEBUG: ", __VA_ARGS__)

#include "postprocess.hpp"

#include <random>

namespace std {

template <class T>
ostream &operator<<(ostream &os, const vector<T> &v) {
    os << '[';

    auto b = v.begin();
    auto e = v.end();

    if (b != e) {
        os << *b;
        while (++b != e) os << ", " << *b;
    }

    return os << ']';
}

} // namespace std

int main() {
    using namespace tap;

    test_plan plan = 11;

    std::vector<ppht::segment> segments;

    segments.push_back({{0, 0}, {50, 1}});
    segments.push_back({{100, 0}, {51, 0}});
    segments.push_back({{101, 1}, {150, 0}});

    ppht::postprocessor postprocessor;

    postprocessor.gap_limit = 3;
    postprocessor.angle_tolerance = 80;
    auto end = postprocessor(segments.begin(), segments.end());

    ok(segments.end() != end, "elements removed - 1");

    segments.erase(end, segments.end());

    eq(1, segments.size(), "all fusions performed - 1");
    eq(ppht::segment(ppht::point{0, 0}, ppht::point{150, 0}), segments[0],
       "segments fused correctly - 1");

    segments.clear();

    segments.push_back({{101, 1}, {150, 0}});
    segments.push_back({{100, 0}, {51, 0}});
    segments.push_back({{0, 0}, {50, 1}});

    end = postprocessor(segments.begin(), segments.end());

    ok(segments.end() != end, "elements removed - 2");

    segments.erase(end, segments.end());

    eq(1, segments.size(), "all fusions performed - 2");
    eq(ppht::segment(ppht::point{0, 0}, ppht::point{150, 0}), segments[0],
       "segments fused correctly - 2");

    segments.clear();

    // unsigned seed = std::random_device()();
    unsigned seed = 2795261323U;

    diag("seed = ", seed);

    std::default_random_engine urbg{seed};
    std::uniform_int_distribution<> dist{-1, 1};
    std::bernoulli_distribution flip;

    for (int i = 0; i < 4; ++i) {
        ppht::point a, b;

        a.x = (i * 25) + dist(urbg);
        a.y = (i * 25) + dist(urbg);
        b.x = ((i + 1) * 25) + dist(urbg);
        b.y = ((i + 1) * 25) + dist(urbg);

        if (flip(urbg)) std::swap(a, b);

        segments.emplace_back(a, b);

        a.x = (i * 25) + dist(urbg);
        a.y = dist(urbg);
        b.x = ((i + 1) * 25) + dist(urbg);
        b.y = dist(urbg);

        if (flip(urbg)) std::swap(a, b);

        segments.emplace_back(a, b);
    }

    std::shuffle(segments.begin(), segments.end(), urbg);

    postprocessor.gap_limit = 5;
    end = postprocessor(segments.begin(), segments.end());

    ok(segments.end() != end, "elements removed - 3");

    segments.erase(end, segments.end());

    if (!eq(2, segments.size(), "segments fused correctly - 3")) {
        diag(segments);
    }

    segments.clear();

    segments.push_back({{0, 0}, {50, 50}});
    segments.push_back({{100, 100}, {50, 50}});
    segments.push_back({{50, 75}, {50, 50}});

    postprocessor.gap_limit = 1;
    end = postprocessor(segments.begin(), segments.end());

    ok(segments.end() != end, "elements removed");

    segments.erase(end, segments.end());

    if (!eq(2, segments.size(), "segments fused correctly")) {
        diag(segments);
    }

    std::vector<ppht::segment> expected;

    expected.push_back({{0, 0}, {100, 100}});
    expected.push_back({{50, 50}, {50, 75}});

    std::sort(segments.begin(), segments.end());
    std::sort(expected.begin(), expected.end());

    eq(expected, segments, "oblique segment ignored");
}

// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include <ppht.hpp>

#include "image-01.hpp"
#include "image-02.hpp"

namespace std {

template <class T>
static inline ostream &operator <<(ostream &o, const vector<T> &v) {
    auto b = v.begin();
    auto e = v.end();

    o << '[';

    if (b != e) {
        o << *b;
        while (++b != e) {
            o << ", " << *b;
        }
    }

    return o << ']';
}

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

} // namespace std

template <class ForwardIt, class Pred>
std::pair<ForwardIt, ForwardIt>
remove_pairs(ForwardIt begin1, ForwardIt end1,
             ForwardIt begin2, ForwardIt end2,
             Pred pred) {
restart:
    for (auto i = begin1; i != end1; ++i) {
        for (auto j = begin2; j != end2; ++j) {
            if (pred(*i, *j)) {
                std::swap(*i, *(--end1));
                std::swap(*j, *(--end2));
                goto restart;
            }
        }
    }

    return std::make_pair(end1, end2);
}

ppht::state<> _load_image(std::size_t rows, std::size_t cols,
                          std::uint8_t *data) {
    ppht::state<> state{rows, cols};

    std::size_t bytes_per_row = (cols + 7) >> 3;

    for (unsigned y = 0; y < rows; ++y) {
        auto row = data + y * bytes_per_row;

        for (unsigned x = 0; x < cols; ++x) {
            auto bit = row[x >> 3] & (std::uint8_t{1} << (x & 7));
            if (bit) state.mark_pending({x, y});
        }
    }

    return state;
}

#define LOAD(X) _load_image(X##_height, X##_width, X##_bits)

int main() {
    using namespace tap;
    using namespace ppht;

    test_plan plan;

    auto image_01 = LOAD(image_01);
    eq(320, image_01.cols(), "image_01 cols");
    eq(120, image_01.rows(), "image_01 rows");

    auto actual = find_segments(image_01);

    std::vector<segment_t> expected = {segment_t{{20, 20}, {100, 20}},
                                       segment_t{{20, 20}, {20, 100}},
                                       segment_t{{100, 20}, {100, 100}},
                                       segment_t{{20, 100}, {100, 100}},
                                       segment_t{{120, 20}, {200, 20}},
                                       segment_t{{120, 20}, {120, 100}},
                                       segment_t{{200, 20}, {200, 100}},
                                       segment_t{{120, 100}, {200, 100}},
                                       segment_t{{220, 20}, {300, 20}},
                                       segment_t{{220, 20}, {220, 100}},
                                       segment_t{{300, 20}, {300, 100}},
                                       segment_t{{220, 100}, {300, 100}}};

    static auto within = [](const point_t &p1, const point_t &p2) {
        auto dx = static_cast<double>(p1[0]) - p2[0];
        auto dy = static_cast<double>(p1[1]) - p2[1];
        return dx * dx + dy * dy <= 25.0;
    };

    static auto similar = [](const segment_t &s1, const segment_t &s2) {
        return within(s1.first, s2.first) && within(s1.second, s2.second);
    };

    auto b1 = actual.begin();
    auto e1 = actual.end();
    auto b2 = expected.begin();
    auto e2 = expected.end();

    std::tie(e1, e2) = remove_pairs(b1, e1, b2, e2, similar);

    actual.erase(e1, actual.end());
    expected.erase(e2, expected.end());

    if (!ok(actual.empty(), "all expected segments seen")) {
        diag("expected: ", actual);
    }

    if (!ok(expected.empty(), "no unexpected segments seen")) {
        diag("unexpected: ", expected);
    }

    auto image_02 = LOAD(image_02);
    eq(100, image_02.cols(), "image_02 cols");
    eq(160, image_02.rows(), "image_02 rows");

    actual = find_segments(image_02);

    expected = {segment_t{{20, 20}, {80, 20}},
                segment_t{{20, 20}, {20, 140}},
                segment_t{{20, 140}, {80, 80}},
                segment_t{{80, 20}, {80, 80}}};

    b1 = actual.begin();
    e1 = actual.end();
    b2 = expected.begin();
    e2 = expected.end();

    std::tie(e1, e2) = remove_pairs(b1, e1, b2, e2, similar);

    actual.erase(e1, actual.end());
    expected.erase(e2, expected.end());

    if (!ok(actual.empty(), "all expected segments seen")) {
        diag("expected: ", actual);
    }

    if (!ok(expected.empty(), "no unexpected segments seen")) {
        diag("unexpected: ", expected);
    }

    return test_status();
}

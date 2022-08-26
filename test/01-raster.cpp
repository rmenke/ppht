// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include "raster.hpp"

int main() {
    using namespace tap;
    test_plan plan;

    ppht::raster<int> raster{5, 7};

    eq(5U, raster.rows(), "rows");
    eq(7U, raster.cols(), "cols");

    bool zeroed = true;

    for (auto r = 0UL; r < raster.rows(); ++r) {
        auto row = raster[r];
        for (auto c = 0UL; c < raster.rows(); ++c) {
            zeroed = zeroed && (row[c] == 0);
        }
    }

    ok(zeroed, "initialized to zero");

    raster[3][2] = 55;

    zeroed = true;

    for (auto r = 0UL; r < raster.rows(); ++r) {
        auto row = raster[r];
        for (auto c = 0UL; c < raster.rows(); ++c) {
            if (r != 3 || c != 2) zeroed = zeroed && (row[c] == 0);
        }
    }

    ok(zeroed, "changing cell does not affect other cells");
    eq(55, raster[3][2], "changed cell is visible");

    auto const &ref = raster;
    eq(55, ref[3][2], "constant access ok");

    return test_status();
}

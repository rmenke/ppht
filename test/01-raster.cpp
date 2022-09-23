// Copyright (C) 2020-2022 by Rob Menke

#include "raster.hpp"
#include "tap.hpp"
#include "types.hpp"

int main() {
    using namespace tap;
    test_plan plan;

    ppht::raster raster{5, 7};

    eq(5U, raster.rows(), "rows");
    eq(7U, raster.cols(), "cols");

    bool zeroed = true;

    for (auto r = 0UL; r < raster.rows(); ++r) {
        auto row = raster[r];
        for (auto c = 0UL; c < raster.rows(); ++c) {
            zeroed = zeroed && (row[c] == ppht::status_t::unset);
        }
    }

    ok(zeroed, "initialized to zero");

    raster[3][2] = ppht::status_t::voted;

    zeroed = true;

    for (auto r = 0UL; r < raster.rows(); ++r) {
        auto row = raster[r];
        for (auto c = 0UL; c < raster.rows(); ++c) {
            if (r != 3 || c != 2) {
                zeroed = zeroed && (row[c] == ppht::status_t::unset);
            }
        }
    }

    ok(zeroed, "changing cell does not affect other cells");
    eq(ppht::status_t::voted, raster[3][2], "changed cell is visible");

    auto const &ref = raster;
    eq(ppht::status_t::voted, ref[3][2], "constant access ok");

    return test_status();
}

// Copyright (C) 2020-2022 by Rob Menke

#include "tap.hpp"

#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

#define PPHT_DEBUG(...) tap::diag("DEBUG: ", __VA_ARGS__)

#include "types.hpp"

#include <cstdlib>
#include <cxxabi.h>
#include <type_traits>
#include <typeinfo>

std::string _type_tag(const std::type_info &t) {
    std::size_t size = 256;
    char *buffer = static_cast<char *>(malloc(size));

    int status = 0;

    char *result = abi::__cxa_demangle(t.name(), buffer, &size, &status);

    if (!result) {
        switch (status) {
            case -1:
                throw std::bad_alloc{};
            case -2:
                throw std::invalid_argument{"not a valid type"};
            case -3:
                throw std::invalid_argument{"invalid argument"};
            default:
                throw std::runtime_error{"unexpected status " +
                                         std::to_string(status)};
        }
    }

    std::string demangled{buffer, size};

    free(buffer);

    return demangled;
}

#define TYPE_TAG(X) _type_tag(typeid(X))

#define TYPE_IS(A, B) \
    ok(std::is_same_v<A, decltype(B)>, #B " is a ", TYPE_TAG(B), "; expected ", TYPE_TAG(A))

int main() {
    using namespace tap;
    using namespace ppht;

    test_plan plan = 8;

    coord<int> c1{5, 5};
    coord<int> c2{3, 8};
    coord<float> c3{1.2, 3.4};
    // coord<unsigned> c4{3U, 5U}; // fails during compilation

    TYPE_IS(coord<int>, c1 + c2);
    TYPE_IS(coord<int>, c1 - c2);
    TYPE_IS(coord<int>, c1 * c2);
    TYPE_IS(coord<int>, c1 / c2);

    TYPE_IS(coord<float>, c1 + c3);
    TYPE_IS(coord<float>, c3 + c1);

    TYPE_IS(coord<long long>, c1 / 2LL);
    TYPE_IS(coord<double>, c1 / 2.0);

    // TYPE_IS(coord<unsigned>, c1 / 2U); // fails during compilation
}

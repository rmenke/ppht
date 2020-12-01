#ifndef _tap_hpp_
#define _tap_hpp_

#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace tap {

void print() {}

template <class Arg, class... Args>
void print(Arg &&arg, Args &&... args) {
    std::cout << std::forward<Arg>(arg);
    print(std::forward<Args>(args)...);
}

template <class... Args>
void println(Args &&... args) {
    print(std::forward<Args>(args)..., '\n');
}

template <class... Args>
void diag(Args &&... args) {
    println("# ", std::forward<Args>(args)...);
}

struct skip_all_t {};

static constexpr skip_all_t skip_all{};

class test_plan {
    unsigned _plan;
    unsigned _done = 0;
    bool _failed = false;

    test_plan *_parent;

    static test_plan *_current_plan;

  public:
    test_plan(unsigned plan = 0)
        : _plan(plan)
        , _parent(_current_plan) {
        _current_plan = this;

        if (_parent) {
            if (_plan > (_parent->_plan - _parent->_done)) {
                throw std::logic_error{"too many subtests"};
            }
        }

        if (plan) println("1..", plan);
    }

    template <class... Args>
    test_plan(skip_all_t, unsigned plan, Args &&... args)
        : _plan(plan)
        , _done(plan)
        , _parent(_current_plan) {
        _current_plan = this;

        if (!_parent) {
            println("1..0 # SKIP ", std::forward<Args>(args)...);
            std::exit(0);
        }

        for (unsigned i = 1; i <= _plan; ++i) {
            _parent->skip(args...);
        }
    }

    test_plan(const test_plan &) = delete;

    test_plan(test_plan &&r)
        : _plan(std::exchange(r._plan, 0))
        , _done(std::exchange(r._done, 0))
        , _parent(std::exchange(r._parent, nullptr)) {
        _current_plan = this;
    }

    ~test_plan() noexcept(false) {
        if (_current_plan != this) {
            throw std::logic_error{"incorrect nesting of plans"};
        }

        _current_plan = _parent;

        if (_plan == 0 && !_parent) {
            println("1..", _done);
        }
        else if (_plan != _done) {
            diag("Looks like you planned ", _plan, ' ',
                 (_plan == 1 ? "test" : "tests"), " but ran ", _done);
        }
    }

    static test_plan *current_plan() {
        auto plan = _current_plan;
        if (!plan) throw std::logic_error{"no plan"};
        return plan;
    }

    int status() const {
        return _failed ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    template <class... Args>
    void pass(Args &&... args) {
        ++_done;

        if (_parent) return _parent->pass(std::forward<Args>(args)...);

        print("ok ", _done);
        if (sizeof...(args)) print(" - ", std::forward<Args>(args)...);
        println();
    }

    template <class... Args>
    void fail(Args &&... args) {
        ++_done;

        if (_parent) return _parent->fail(std::forward<Args>(args)...);

        print("not ok ", _done);
        if (sizeof...(args)) print(" - ", std::forward<Args>(args)...);
        println();

        _failed = true;
    }

    template <class... Args>
    void skip(Args &&... args) {
        println("ok ", ++_done, " # skip ", std::forward<Args>(args)...);
    }
};

template <class... Args>
static inline void pass(Args &&... args) {
    test_plan::current_plan()->pass(std::forward<Args>(args)...);
}

template <class... Args>
static inline void fail(Args &&... args) {
    test_plan::current_plan()->fail(std::forward<Args>(args)...);
}

template <class... Args>
static inline bool ok(bool result, Args &&... args) {
    auto plan = test_plan::current_plan();

    if (result)
        plan->pass(std::forward<Args>(args)...);
    else
        plan->fail(std::forward<Args>(args)...);
    return result;
}

template <template <class> class T> extern const char * const cmp_op;

template <> const char * cmp_op<std::equal_to> = "==";
template <> const char * cmp_op<std::not_equal_to> = "!=";
template <> const char * cmp_op<std::greater> = ">";
template <> const char * cmp_op<std::greater_equal> = ">=";
template <> const char * cmp_op<std::less> = "<";
template <> const char * cmp_op<std::less_equal> = "<=";

template <template <class> class Comp, class X, class Y, class... Args>
static inline bool cmp(X &&x, Y &&y, Args &&... args) {
    using Z = std::common_type_t<X, Y>;
    if (!ok(Comp<Z>{}(x, y), std::forward<Args>(args)...)) {
        diag(x, ' ', cmp_op<Comp>, ' ', y, ": failed");
        return false;
    }
    return true;
}

template <class X, class Y, class... Args>
static inline bool eq(X &&x, Y &&y, Args &&... args) {
    return cmp<std::equal_to>(std::forward<X>(x), std::forward<Y>(y),
                              std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
static inline bool ne(X &&x, Y &&y, Args &&... args) {
    return cmp<std::not_equal_to>(std::forward<X>(x), std::forward<Y>(y),
                                  std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
static inline bool gt(X &&x, Y &&y, Args &&... args) {
    return cmp<std::greater>(std::forward<X>(x), std::forward<Y>(y),
                             std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
static inline bool ge(X &&x, Y &&y, Args &&... args) {
    return cmp<std::greater_equal>(std::forward<X>(x), std::forward<Y>(y),
                                   std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
static inline bool lt(X &&x, Y &&y, Args &&... args) {
    return cmp<std::less>(std::forward<X>(x), std::forward<Y>(y),
                          std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
static inline bool le(X &&x, Y &&y, Args &&... args) {
    return cmp<std::less_equal>(std::forward<X>(x), std::forward<Y>(y),
                                std::forward<Args>(args)...);
}

template <class X, class Y, class Z, class... Args>
static inline bool within(X &&x, Y &&y, Z &&z, Args &&... args) {
    if (!ok(std::fabs(x - y) <= z, std::forward<Args>(args)...)) {
        diag(y, " = ", x, "±", z, ": failed");
        return false;
    }
    return true;
}

template <class T, class P, class... Args>
static inline bool instanceof(P &&p, Args &&... args) {
    std::type_info const &expected = typeid(T);
    std::type_info const &actual   = typeid(p);

    if(!ok(expected == actual, std::forward<Args>(args)...)) {
        diag("type of parameter is ", actual.name());
        diag("     but expected    ", expected.name());
        return false;
    }
    return true;
}

template <class... Args>
static inline test_plan skip(unsigned count, Args &&... args) {
    return test_plan{skip_all, count, args...};
}

int test_status() {
    return test_plan::current_plan()->status();
}

#define TAP_INITIALIZE \
    ::tap::test_plan * ::tap::test_plan::_current_plan = nullptr

} // namespace tap

#endif

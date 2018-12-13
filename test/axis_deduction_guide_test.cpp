// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/core/lightweight_test_trait.hpp>
#include <boost/histogram/axis.hpp>
#include <boost/histogram/axis/ostream_operators.hpp>
#include <type_traits>

using namespace boost::histogram;
namespace tr = axis::transform;

// tests requires a C++17 compatible compiler

int main() {
#if __cpp_deduction_guides >= 201606
  {
    axis::regular a(1, 0.0, 1.0);
    axis::regular b(1, 0, 1);
    axis::regular c(1, 0.0f, 1.0f);
    axis::regular d(1, 0, 1, "foo");
    axis::regular e(1, 0, 1, axis::null_type{});
    axis::regular f(tr::sqrt(), 1, 0, 1);
    axis::regular g(tr::sqrt(), 1, 0, 1, "foo");
    axis::regular h(tr::sqrt(), 1, 0, 1, axis::null_type{});

    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(a), axis::regular<>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(b), axis::regular<>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(c), axis::regular<float>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(d), axis::regular<>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(e), axis::regular<double, axis::null_type>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(f), axis::regular<tr::sqrt<>>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(g), axis::regular<tr::sqrt<>>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(h), axis::regular<tr::sqrt<>, axis::null_type>>));
  }

  {
    axis::integer a(1, 2);
    axis::integer b(1l, 2l);
    axis::integer c(1.0, 2.0);
    axis::integer d(1.0f, 2.0f);
    axis::integer e(1, 2, "foo");
    axis::integer f(1, 2, axis::null_type{});

    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(a), axis::integer<int>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(b), axis::integer<int>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(c), axis::integer<double>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(d), axis::integer<float>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(e), axis::integer<int>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(f), axis::integer<int, axis::null_type>>));
  }

  {
    axis::variable a{-1, 1};
    axis::variable b{-1.f, 1.f};
    axis::variable c{-1.0, 1.0};
    axis::variable d({-1, 1}, "foo");
    axis::variable e({-1, 1}, axis::null_type{});

    std::vector<int> vi{{-1, 1}};
    std::vector<float> vf{{-1.f, 1.f}};
    axis::variable f(vi);
    axis::variable g(vf);
    axis::variable h(vi, "foo");
    axis::variable i(vi, axis::null_type{});

    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(a), axis::variable<>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(b), axis::variable<float, axis::allocator<float>>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(c), axis::variable<>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(d), axis::variable<>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(e),
                      axis::variable<double, axis::allocator<double>, axis::null_type>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(f), axis::variable<>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(g), axis::variable<float, axis::allocator<float>>>));
    BOOST_TEST_TRAIT_TRUE((std::is_same<decltype(h), axis::variable<>>));
    BOOST_TEST_TRAIT_TRUE(
        (std::is_same<decltype(i),
                      axis::variable<double, axis::allocator<double>, axis::null_type>>));
  }
#endif
  return boost::report_errors();
}
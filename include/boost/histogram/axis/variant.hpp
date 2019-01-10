// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_VARIANT_HPP
#define BOOST_HISTOGRAM_AXIS_VARIANT_HPP

#include <boost/core/typeinfo.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/polymorphic_bin.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/detail/cat.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant.hpp>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace boost {
namespace histogram {

namespace detail {
template <typename F, typename R>
struct functor_wrapper : public boost::static_visitor<R> {
  F& fcn;
  functor_wrapper(F& f) : fcn(f) {}

  template <typename T>
  R operator()(T&& t) const {
    return fcn(std::forward<T>(t));
  }
};
} // namespace detail

namespace axis {

/// Polymorphic axis type
template <typename... Ts>
class variant : private boost::variant<Ts...>, public iterator_mixin<variant<Ts...>> {
  using base_type = boost::variant<Ts...>;
  using first_bounded_type = detail::naked<mp11::mp_first<base_type>>;

  using types = mp11::mp_transform<detail::naked, base_type>;
  template <typename T>
  using requires_bounded_type =
      mp11::mp_if<mp11::mp_contains<types, detail::naked<T>>, void>;

public:
  using metadata_type =
      detail::naked<decltype(traits::metadata(std::declval<first_bounded_type&>()))>;

  variant() = default;
  variant(const variant&) = default;
  variant& operator=(const variant&) = default;
  variant(variant&&) = default;
  variant& operator=(variant&&) = default;

  template <typename T, typename = requires_bounded_type<T>>
  variant(T&& t) : base_type(std::forward<T>(t)) {}

  template <typename T, typename = requires_bounded_type<T>>
  variant& operator=(T&& t) {
    base_type::operator=(std::forward<T>(t));
    return *this;
  }

  template <typename... Us>
  variant(const variant<Us...>& u) {
    this->operator=(u);
  }

  template <typename... Us>
  variant& operator=(const variant<Us...>& u) {
    visit(
        [this](const auto& u) {
          using U = detail::naked<decltype(u)>;
          detail::static_if<mp11::mp_contains<types, U>>(
              [this](const auto& u) { this->operator=(u); },
              [](const auto&) {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    boost::core::demangled_name(BOOST_CORE_TYPEID(U)),
                    " is not a bounded type of ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(variant)))));
              },
              u);
        },
        u);
    return *this;
  }

  int size() const {
    return visit([](const auto& x) { return x.size(); }, *this);
  }

  option_type options() const {
    return visit([](const auto& x) { return axis::traits::options(x); }, *this);
  }

  const metadata_type& metadata() const {
    return visit(
        [](const auto& x) -> const metadata_type& {
          using U = decltype(traits::metadata(x));
          return detail::static_if<std::is_same<U, const metadata_type&>>(
              [](const auto& x) -> const metadata_type& { return traits::metadata(x); },
              [](const auto&) -> const metadata_type& {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    "cannot return metadata of type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(U)),
                    " through axis::variant interface which uses type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(const metadata_type&)),
                    "; use boost::histogram::axis::get to obtain a reference "
                    "of this axis type")));
              },
              x);
        },
        *this);
  }

  metadata_type& metadata() {
    return visit(
        [](auto& x) -> metadata_type& {
          using U = decltype(traits::metadata(x));
          return detail::static_if<std::is_same<U, metadata_type&>>(
              [](auto& x) -> metadata_type& { return traits::metadata(x); },
              [](auto&) -> metadata_type& {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    "cannot return metadata of type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(U)),
                    " through axis::variant interface which uses type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(metadata_type&)))));
              },
              x);
        },
        *this);
  }

  // Will throw invalid_argument exception if axis has incompatible call signature
  template <typename U>
  int operator()(U&& x) const {
    return visit(
        [&x](const auto& a) {
          using A = detail::naked<decltype(a)>;
          using arg_t = detail::arg_type<A>;
          return detail::static_if<std::is_convertible<U, arg_t>>(
              [&x](const auto& a) -> int { return a(x); },
              [](const auto&) -> int {
                BOOST_THROW_EXCEPTION(std::invalid_argument(detail::cat(
                    "cannot convert ", boost::core::demangled_name(BOOST_CORE_TYPEID(U)),
                    " to ", boost::core::demangled_name(BOOST_CORE_TYPEID(arg_t)),
                    " for ", boost::core::demangled_name(BOOST_CORE_TYPEID(A)))));
                return 0;
              },
              a);
        },
        *this);
  }

  // Only works for axes with value method that returns something convertible to
  // double and will throw a runtime_error otherwise, see axis::traits::value
  double value(double idx) const {
    return visit([idx](const auto& a) { return traits::value_as<double>(a, idx); },
                 *this);
  }

  auto operator[](const int idx) const {
    return visit(
        [idx](const auto& a) {
          return detail::value_method_switch_with_return_type<double,
                                                              polymorphic_bin<double>>(
              [idx](const auto& a) { // axis is discrete
                const auto x = a.value(idx);
                return polymorphic_bin<double>(x, x);
              },
              [idx](const auto& a) { // axis is continuous
                return polymorphic_bin<double>(a.value(idx), a.value(idx + 1));
              },
              a);
        },
        *this);
  }

  template <typename... Us>
  bool operator==(const variant<Us...>& u) const {
    return visit([&u](const auto& x) { return u == x; }, *this);
  }

  template <typename T>
  bool operator==(const T& t) const {
    // boost::variant::operator==(T) implemented only to fail, cannot use it
    auto tp = boost::relaxed_get<T>(this);
    return tp && detail::relaxed_equal(*tp, t);
  }

  template <typename T>
  bool operator!=(const T& t) const {
    return !operator==(t);
  }

  template <typename Archive>
  void serialize(Archive& ar, unsigned);

  template <typename Functor, typename... Us>
  friend auto visit(Functor&& f, variant<Us...>& v)
      -> detail::visitor_return_type<Functor, variant<Us...>&>;

  template <typename Functor, typename... Us>
  friend auto visit(Functor&& f, const variant<Us...>& v)
      -> detail::visitor_return_type<Functor, const variant<Us...>&>;

  template <typename T, typename... Us>
  friend T& get(variant<Us...>& v);

  template <typename T, typename... Us>
  friend const T& get(const variant<Us...>& v);

  template <typename T, typename... Us>
  friend T&& get(variant<Us...>&& v);

  template <typename T, typename... Us>
  friend T* get(variant<Us...>* v);

  template <typename T, typename... Us>
  friend const T* get(const variant<Us...>* v);
};

template <typename Functor, typename... Us>
auto visit(Functor&& f, variant<Us...>& v)
    -> detail::visitor_return_type<Functor, variant<Us...>&> {
  using R = detail::visitor_return_type<Functor, variant<Us...>&>;
  return boost::apply_visitor(detail::functor_wrapper<Functor, R>(f),
                              static_cast<typename variant<Us...>::base_type&>(v));
}

template <typename Functor, typename... Us>
auto visit(Functor&& f, const variant<Us...>& v)
    -> detail::visitor_return_type<Functor, const variant<Us...>&> {
  using R = detail::visitor_return_type<Functor, const variant<Us...>&>;
  return boost::apply_visitor(detail::functor_wrapper<Functor, R>(f),
                              static_cast<const typename variant<Us...>::base_type&>(v));
}

template <typename T, typename... Us>
T& get(variant<Us...>& v) {
  return boost::get<T>(static_cast<typename variant<Us...>::base_type&>(v));
}

template <typename T, typename... Us>
T&& get(variant<Us...>&& v) {
  return boost::get<T>(static_cast<typename variant<Us...>::base_type&&>(v));
}

template <typename T, typename... Us>
const T& get(const variant<Us...>& v) {
  return boost::get<T>(static_cast<const typename variant<Us...>::base_type&>(v));
}

template <typename T, typename... Us>
T* get(variant<Us...>* v) {
  return boost::relaxed_get<T>(static_cast<typename variant<Us...>::base_type*>(v));
}

template <typename T, typename... Us>
const T* get(const variant<Us...>* v) {
  return boost::relaxed_get<T>(static_cast<const typename variant<Us...>::base_type*>(v));
}

// pass-through version for generic programming, if U is axis instead of variant
template <typename T, typename U, typename = detail::requires_axis<detail::naked<U>>>
auto get(U&& u) -> detail::copy_qualifiers<U, T> {
  return std::forward<U>(u);
}

// pass-through version for generic programming, if U is axis instead of variant
template <typename T, typename U, typename = detail::requires_axis<detail::naked<U>>>
T* get(U* u) {
  return std::is_same<T, detail::naked<U>>::value ? reinterpret_cast<T*>(u) : nullptr;
}

// pass-through version for generic programming, if U is axis instead of variant
template <typename T, typename U, typename = detail::requires_axis<detail::naked<U>>>
const T* get(const U* u) {
  return std::is_same<T, detail::naked<U>>::value ? reinterpret_cast<const T*>(u)
                                                   : nullptr;
}
} // namespace axis
} // namespace histogram
} // namespace boost

#endif

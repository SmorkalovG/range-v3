/// \file
// Concepts library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef CONCEPTS_TYPE_TRAITS_HPP
#define CONCEPTS_TYPE_TRAITS_HPP

#include <tuple>
#include <utility>
#include <type_traits>
#include <meta/meta.hpp>

namespace concepts
{
    inline namespace v1
    {
        template<typename T>
        using remove_cvref_t =
            typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;

        /// \cond
        namespace detail
        {
            template<typename From, typename To>
            struct _copy_cv_
            {
                using type = To;
            };
            template<typename From, typename To>
            struct _copy_cv_<From const, To>
            {
                using type = To const;
            };
            template<typename From, typename To>
            struct _copy_cv_<From volatile, To>
            {
                using type = To volatile;
            };
            template<typename From, typename To>
            struct _copy_cv_<From const volatile, To>
            {
                using type = To const volatile;
            };
            template<typename From, typename To>
            using _copy_cv = meta::_t<_copy_cv_<From, To>>;

            ////////////////////////////////////////////////////////////////////////////////////////
            template<typename T, typename U, typename = void>
            struct _builtin_common;

            template<typename T, typename U>
            using _builtin_common_t = meta::_t<_builtin_common<T, U>>;

            template<typename T, typename U>
            using _cond_res = decltype(true ? std::declval<T>() : std::declval<U>());

            template<typename T, typename U, typename R = _builtin_common_t<T &, U &>>
            using _rref_res =
                meta::if_<std::is_reference<R>, meta::_t<std::remove_reference<R>> &&, R>;

            template<typename T, typename U>
            using _lref_res = _cond_res<_copy_cv<T, U> &, _copy_cv<U, T> &>;

            template<typename T>
            struct as_cref_
            {
                using type = T const &;
            };
            template<typename T>
            struct as_cref_<T &>
            {
                using type = T const &;
            };
            template<typename T>
            struct as_cref_<T &&>
            {
                using type = T const &;
            };
            template<>
            struct as_cref_<void>
            {
                using type = void;
            };
            template<>
            struct as_cref_<void const>
            {
                using type = void const;
            };

            template<typename T>
            using as_cref_t = typename as_cref_<T>::type;

            template<typename T>
            using decay_t = typename std::decay<T>::type;

        #if !defined(__GNUC__) || defined(__clang__)
            template<typename T, typename U, typename /* = void */>
            struct _builtin_common
              : meta::lazy::let<
                    meta::defer<decay_t, meta::defer<_cond_res, as_cref_t<T>, as_cref_t<U>>>>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &&, U &&, meta::if_<meta::and_<
                std::is_convertible<T &&, _rref_res<T, U>>,
                std::is_convertible<U &&, _rref_res<T, U>>>>>
            {
                using type = _rref_res<T, U>;
            };
            template<typename T, typename U>
            struct _builtin_common<T &, U &>
              : meta::defer<_lref_res, T, U>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &, U &&, meta::if_<
                std::is_convertible<U &&, _builtin_common_t<T &, U const &>>>>
              : _builtin_common<T &, U const &>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &&, U &>
              : _builtin_common<U &, T &&>
            {};
        #else
            template<typename T, typename U, typename = void>
            struct _builtin_common_
            {};
            template<typename T, typename U>
            struct _builtin_common_<T, U, meta::void_<_cond_res<as_cref_t<T>, as_cref_t<U>>>>
              : std::decay<_cond_res<as_cref_t<T>, as_cref_t<U>>>
            {};
            template<typename T, typename U, typename /* = void */>
            struct _builtin_common
              : _builtin_common_<T, U>
            {};
            template<typename T, typename U, typename = void>
            struct _builtin_common_rr
              : _builtin_common_<T &&, U &&>
            {};
            template<typename T, typename U>
            struct _builtin_common_rr<T, U, meta::if_<meta::and_<
                std::is_convertible<T &&, _rref_res<T, U>>,
                std::is_convertible<U &&, _rref_res<T, U>>>>>
            {
                using type = _rref_res<T, U>;
            };
            template<typename T, typename U>
            struct _builtin_common<T &&, U &&>
              : _builtin_common_rr<T, U>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &, U &>
              : meta::defer<_lref_res, T, U>
            {};
            template<typename T, typename U, typename = void>
            struct _builtin_common_lr
              : _builtin_common_<T &, T &&>
            {};
            template<typename T, typename U>
            struct _builtin_common_lr<T, U, meta::if_<
                std::is_convertible<U &&, _builtin_common_t<T &, U const &>>>>
              : _builtin_common<T &, U const &>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &, U &&>
              : _builtin_common_lr<T, U>
            {};
            template<typename T, typename U>
            struct _builtin_common<T &&, U &>
              : _builtin_common<U &, T &&>
            {};
        #endif
        }
        /// \endcond

        /// \addtogroup group-utility Utility
        /// @{
        ///

        /// Users should specialize this to hook the \c Common concept
        /// until \c std gets a SFINAE-friendly \c std::common_type and there's
        /// some sane way to deal with cv and ref qualifiers.
        template<typename ...Ts>
        struct common_type
        {};

        template<typename T>
        struct common_type<T>
          : std::decay<T>
        {};

        template<typename T, typename U>
        struct common_type<T, U>
          : meta::if_c<
                ( std::is_same<detail::decay_t<T>, T>::value &&
                  std::is_same<detail::decay_t<U>, U>::value ),
                meta::defer<detail::_builtin_common_t, T, U>,
                common_type<detail::decay_t<T>, detail::decay_t<U>>>
        {};

        template<typename... Ts>
        using common_type_t = typename common_type<Ts...>::type;

        template<typename T, typename U, typename... Vs>
        struct common_type<T, U, Vs...>
          : meta::lazy::fold<meta::list<U, Vs...>, T, meta::quote<common_type_t>>
        {};

        /// @}

        /// \addtogroup group-utility Utility
        /// @{
        ///

        /// Users can specialize this to hook the \c CommonReference concept.
        /// \sa `common_reference`
        template<typename T, typename U, typename TQual, typename UQual>
        struct basic_common_reference
        {};

        /// \cond
        namespace detail
        {
            using _rref =
                meta::quote_trait<std::add_rvalue_reference>;
            using _lref =
                meta::quote_trait<std::add_lvalue_reference>;

            template<typename T>
            struct _xref
            {
                using type = meta::quote_trait<meta::id>;
            };
            template<typename T>
            struct _xref<T &&>
            {
                using type = meta::compose<_rref, meta::_t<_xref<T>>>;
            };
            template<typename T>
            struct _xref<T &>
            {
                using type = meta::compose<_lref, meta::_t<_xref<T>>>;
            };
            template<typename T>
            struct _xref<T const>
            {
                using type = meta::quote_trait<std::add_const>;
            };
            template<typename T>
            struct _xref<T volatile>
            {
                using type = meta::quote_trait<std::add_volatile>;
            };
            template<typename T>
            struct _xref<T const volatile>
            {
                using type = meta::quote_trait<std::add_cv>;
            };

            template<typename T, typename U>
            using _basic_common_reference =
                basic_common_reference<
                    remove_cvref_t<T>,
                    remove_cvref_t<U>,
                    meta::_t<_xref<T>>,
                    meta::_t<_xref<U>>>;

            template<typename T, typename U, typename = void>
            struct _common_reference2
              : meta::if_<
                    meta::is_trait<_basic_common_reference<T, U>>,
                    _basic_common_reference<T, U>,
                    common_type<T, U>>
            {};

        #if 0 //!defined(__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 8
            template<typename T, typename U>
            struct _common_reference2<T, U, meta::if_<meta::is_trait<_builtin_common<T, U>>>>
              : _builtin_common<T, U>
            {};
        #else
            template<typename T, typename U>
            struct _common_reference2<T, U, meta::if_<std::is_reference<_builtin_common_t<T, U>>>>
              : _builtin_common<T, U>
            {};
        #endif
        }
        /// \endcond

        /// Users can specialize this to hook the \c CommonReference concept.
        /// \sa `basic_common_reference`
        template<typename ...Ts>
        struct common_reference
        {};

        template<typename T>
        struct common_reference<T>
        {
            using type = T;
        };

        template<typename T, typename U>
        struct common_reference<T, U>
          : detail::_common_reference2<T, U>
        {};

        template<typename... Ts>
        using common_reference_t = typename common_reference<Ts...>::type;

        template<typename T, typename U, typename... Vs>
        struct common_reference<T, U, Vs...>
          : meta::lazy::fold<meta::list<U, Vs...>, T, meta::quote<common_reference_t>>
        {};
        /// @}
    } // namespace v1
} // namespace concepts

#endif
#pragma once

#include <iostream>
#include <exception>

namespace valijson {
#if defined(_MSC_VER) && _MSC_VER == 1800
#define VALIJSON_NORETURN __declspec(noreturn)
#else
#define VALIJSON_NORETURN [[noreturn]]
#endif

#if VALIJSON_USE_EXCEPTIONS
#include <stdexcept>

VALIJSON_NORETURN inline void throwRuntimeError(const std::string& msg) {
  throw std::runtime_error(msg);
}

VALIJSON_NORETURN inline void throwLogicError(const std::string& msg) {
  throw std::logic_error(msg);
}
#else
VALIJSON_NORETURN inline void throwRuntimeError(const std::string& msg) {
  std::cerr << msg << std::endl;
  abort();
}
VALIJSON_NORETURN inline void throwLogicError(const std::string& msg) {
  std::cerr << msg << std::endl;
  abort();
}

#endif

VALIJSON_NORETURN inline void throwNotSupported() {
    throwRuntimeError("Not supported");
}

} // namespace valijson
// Copyright (C) 2011 - 2012 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// The idea and interface is based on Boost.Optional library
// authored by Fernando Luis Cacciola Carballal

# ifndef ___OPTIONAL_HPP___
# define ___OPTIONAL_HPP___

# include <utility>
# include <type_traits>
# include <initializer_list>
# include <cassert>
# include <functional>
# include <string>
# include <stdexcept>

# define TR2_OPTIONAL_REQUIRES(...) typename enable_if<__VA_ARGS__::value, bool>::type = false

# if defined __GNUC__ // NOTE: GNUC is also defined for Clang
#   if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)
#     define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#   endif
#
#   if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)
#     define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#   endif
#
#   if (__GNUC__ == 4) && (__GNUC_MINOR__ == 8) && (__GNUC_PATCHLEVEL__ >= 1)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   elif (__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   endif
# endif
#
# if defined __clang_major__
#   if (__clang_major__ == 3 && __clang_minor__ >= 5)
#     define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#   elif (__clang_major__ > 3)
#     define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#   endif
#   if defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#     define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#   elif (__clang_major__ == 3 && __clang_minor__ == 4 && __clang_patchlevel__ >= 2)
#     define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#   endif
# endif
#
# if defined _MSC_VER
#   if (_MSC_VER >= 1900)
#     define TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#   endif
# endif

# if defined __clang__
#   if (__clang_major__ > 2) || (__clang_major__ == 2) && (__clang_minor__ >= 9)
#     define OPTIONAL_HAS_THIS_RVALUE_REFS 1
#   else
#     define OPTIONAL_HAS_THIS_RVALUE_REFS 0
#   endif
# elif defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 1
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 1
# else
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 0
# endif


# if defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 1
#   define OPTIONAL_CONSTEXPR_INIT_LIST constexpr
# else
#   define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 0
#   define OPTIONAL_CONSTEXPR_INIT_LIST
# endif

# if defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_ && (defined __cplusplus) && (__cplusplus != 201103L)
#   define OPTIONAL_HAS_MOVE_ACCESSORS 1
# else
#   define OPTIONAL_HAS_MOVE_ACCESSORS 0
# endif

# // In C++11 constexpr implies const, so we need to make non-const members also non-constexpr
# if (defined __cplusplus) && (__cplusplus == 201103L)
#   define OPTIONAL_MUTABLE_CONSTEXPR
# else
#   define OPTIONAL_MUTABLE_CONSTEXPR constexpr
# endif

namespace std{

    namespace experimental{

        // BEGIN workaround for missing is_trivially_destructible
# if defined TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
        // leave it: it is already there
# elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
        // leave it: it is already there
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
        // leave it: it is already there
# elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
        // leave it: the user doesn't want it
# else
        template <typename T>
        using is_trivially_destructible = std::has_trivial_destructor<T>;
# endif
        // END workaround for missing is_trivially_destructible

# if (defined TR2_OPTIONAL_GCC_4_7_AND_HIGHER___)
        // leave it; our metafunctions are already defined.
# elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
        // leave it; our metafunctions are already defined.
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
        // leave it: it is already there
# elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
        // leave it: the user doesn't want it
# else


        // workaround for missing traits in GCC and CLANG
        template <class T>
        struct is_nothrow_move_constructible
        {
            constexpr static bool value = std::is_nothrow_constructible<T, T&&>::value;
        };


        template <class T, class U>
        struct is_assignable
        {
            template <class X, class Y>
            constexpr static bool has_assign(...) { return false; }

            template <class X, class Y, size_t S = sizeof((std::declval<X>() = std::declval<Y>(), true)) >
            // the comma operator is necessary for the cases where operator= returns void
            constexpr static bool has_assign(bool) { return true; }

            constexpr static bool value = has_assign<T, U>(true);
        };


        template <class T>
        struct is_nothrow_move_assignable
        {
            template <class X, bool has_any_move_assign>
            struct has_nothrow_move_assign {
    constexpr static bool value = false;
            };

            template <class X>
            struct has_nothrow_move_assign<X, true> {
    constexpr static bool value = noexcept( std::declval<X&>() = std::declval<X&&>() );
            };

            constexpr static bool value = has_nothrow_move_assign<T, is_assignable<T&, T&&>::value>::value;
        };
        // end workaround


# endif



        // 20.5.4, optional for object types
        template <class T> class optional;

        // 20.5.5, optional for lvalue reference types
        template <class T> class optional<T&>;


        // workaround: std utility functions aren't constexpr yet
        template <class T> inline constexpr T&& constexpr_forward(typename std::remove_reference<T>::type& t) noexcept
        {
            return static_cast<T&&>(t);
        }

        template <class T> inline constexpr T&& constexpr_forward(typename std::remove_reference<T>::type&& t) noexcept
        {
            static_assert(!std::is_lvalue_reference<T>::value, "!!");
            return static_cast<T&&>(t);
        }

        template <class T> inline constexpr typename std::remove_reference<T>::type&& constexpr_move(T&& t) noexcept
        {
            return static_cast<typename std::remove_reference<T>::type&&>(t);
        }


#if defined NDEBUG
# define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) (EXPR)
#else
# define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) ((CHECK) ? (EXPR) : ([]{assert(!#CHECK);}(), (EXPR)))
#endif


        namespace detail_
        {

            // static_addressof: a constexpr version of addressof
            template <typename T>
            struct has_overloaded_addressof
            {
                template <class X>
                constexpr static bool has_overload(...) { return false; }

                template <class X, size_t S = sizeof(std::declval<X&>().operator&()) >
                constexpr static bool has_overload(bool) { return true; }

                constexpr static bool value = has_overload<T>(true);
            };

            template <typename T, TR2_OPTIONAL_REQUIRES(!has_overloaded_addressof<T>)>
            constexpr T* static_addressof(T& ref)
            {
                return &ref;
            }

            template <typename T, TR2_OPTIONAL_REQUIRES(has_overloaded_addressof<T>)>
            T* static_addressof(T& ref)
            {
                return std::addressof(ref);
            }


            // the call to convert<A>(b) has return type A and converts b to type A iff b decltype(b) is implicitly convertible to A
            template <class U>
            constexpr U convert(U v) { return v; }

        } // namespace detail


        constexpr struct trivial_init_t{} trivial_init{};


        // 20.5.6, In-place construction
        constexpr struct in_place_t{} in_place{};


        // 20.5.7, Disengaged state indicator
        struct nullopt_t
        {
            struct init{};
            constexpr explicit nullopt_t(init){}
        };
        constexpr nullopt_t nullopt{nullopt_t::init()};


        // 20.5.8, class bad_optional_access
        class bad_optional_access : public logic_error {
        public:
            explicit bad_optional_access(const string& what_arg) : logic_error{what_arg} {}
            explicit bad_optional_access(const char* what_arg) : logic_error{what_arg} {}
        };


        template <class T>
        union storage_t
        {
            unsigned char dummy_;
            T value_;

            constexpr storage_t( trivial_init_t ) noexcept : dummy_() {};

            template <class... Args>
            constexpr storage_t( Args&&... args ) : value_(constexpr_forward<Args>(args)...) {}

            ~storage_t(){}
        };


        template <class T>
        union constexpr_storage_t
        {
            unsigned char dummy_;
            T value_;

            constexpr constexpr_storage_t( trivial_init_t ) noexcept : dummy_() {};

            template <class... Args>
            constexpr constexpr_storage_t( Args&&... args ) : value_(constexpr_forward<Args>(args)...) {}

            ~constexpr_storage_t() = default;
        };


        template <class T>
        struct optional_base
        {
            bool init_;
            storage_t<T> storage_;

            constexpr optional_base() noexcept : init_(false), storage_(trivial_init) {};

            explicit constexpr optional_base(const T& v) : init_(true), storage_(v) {}

            explicit constexpr optional_base(T&& v) : init_(true), storage_(constexpr_move(v)) {}

            template <class... Args> explicit optional_base(in_place_t, Args&&... args)
            : init_(true), storage_(constexpr_forward<Args>(args)...) {}

            template <class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
            explicit optional_base(in_place_t, std::initializer_list<U> il, Args&&... args)
            : init_(true), storage_(il, std::forward<Args>(args)...) {}

            ~optional_base() { if (init_) storage_.value_.T::~T(); }
        };


        template <class T>
        struct constexpr_optional_base
        {
            bool init_;
            constexpr_storage_t<T> storage_;

            constexpr constexpr_optional_base() noexcept : init_(false), storage_(trivial_init) {};

            explicit constexpr constexpr_optional_base(const T& v) : init_(true), storage_(v) {}

            explicit constexpr constexpr_optional_base(T&& v) : init_(true), storage_(constexpr_move(v)) {}

            template <class... Args> explicit constexpr constexpr_optional_base(in_place_t, Args&&... args)
            : init_(true), storage_(constexpr_forward<Args>(args)...) {}

            template <class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
            OPTIONAL_CONSTEXPR_INIT_LIST explicit constexpr_optional_base(in_place_t, std::initializer_list<U> il, Args&&... args)
            : init_(true), storage_(il, std::forward<Args>(args)...) {}

            ~constexpr_optional_base() = default;
        };

        template <class T>
        using OptionalBase = typename std::conditional<
        is_trivially_destructible<T>::value,
        constexpr_optional_base<typename std::remove_const<T>::type>,
        optional_base<typename std::remove_const<T>::type>
        >::type;



        template <class T>
        class optional : private OptionalBase<T>
        {
            static_assert( !std::is_same<typename std::decay<T>::type, nullopt_t>::value, "bad T" );
            static_assert( !std::is_same<typename std::decay<T>::type, in_place_t>::value, "bad T" );


            constexpr bool initialized() const noexcept { return OptionalBase<T>::init_; }
            typename std::remove_const<T>::type* dataptr() {  return std::addressof(OptionalBase<T>::storage_.value_); }
            constexpr const T* dataptr() const { return detail_::static_addressof(OptionalBase<T>::storage_.value_); }

# if OPTIONAL_HAS_THIS_RVALUE_REFS == 1
            constexpr const T& contained_val() const& { return OptionalBase<T>::storage_.value_; }
#   if OPTIONAL_HAS_MOVE_ACCESSORS == 1
            OPTIONAL_MUTABLE_CONSTEXPR T&& contained_val() && { return std::move(OptionalBase<T>::storage_.value_); }
            OPTIONAL_MUTABLE_CONSTEXPR T& contained_val() & { return OptionalBase<T>::storage_.value_; }
#   else
            T& contained_val() & { return OptionalBase<T>::storage_.value_; }
            T&& contained_val() && { return std::move(OptionalBase<T>::storage_.value_); }
#   endif
# else
            constexpr const T& contained_val() const { return OptionalBase<T>::storage_.value_; }
            T& contained_val() { return OptionalBase<T>::storage_.value_; }
# endif

            void clear() noexcept {
    if (initialized()) dataptr()->T::~T();
    OptionalBase<T>::init_ = false;
            }

            template <class... Args>
            void initialize(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...)))
            {
    assert(!OptionalBase<T>::init_);
    ::new (static_cast<void*>(dataptr())) T(std::forward<Args>(args)...);
    OptionalBase<T>::init_ = true;
            }

            template <class U, class... Args>
            void initialize(std::initializer_list<U> il, Args&&... args) noexcept(noexcept(T(il, std::forward<Args>(args)...)))
            {
    assert(!OptionalBase<T>::init_);
    ::new (static_cast<void*>(dataptr())) T(il, std::forward<Args>(args)...);
    OptionalBase<T>::init_ = true;
            }

        public:
            typedef T value_type;

            // 20.5.5.1, constructors
            constexpr optional() noexcept : OptionalBase<T>()  {};
            constexpr optional(nullopt_t) noexcept : OptionalBase<T>() {};

            optional(const optional& rhs)
            : OptionalBase<T>()
            {
    if (rhs.initialized()) {
        ::new (static_cast<void*>(dataptr())) T(*rhs);
        OptionalBase<T>::init_ = true;
    }
            }

            optional(optional&& rhs) noexcept(is_nothrow_move_constructible<T>::value)
            : OptionalBase<T>()
            {
    if (rhs.initialized()) {
        ::new (static_cast<void*>(dataptr())) T(std::move(*rhs));
        OptionalBase<T>::init_ = true;
    }
            }

            constexpr optional(const T& v) : OptionalBase<T>(v) {}

            constexpr optional(T&& v) : OptionalBase<T>(constexpr_move(v)) {}

            template <class... Args>
            explicit constexpr optional(in_place_t, Args&&... args)
            : OptionalBase<T>(in_place_t{}, constexpr_forward<Args>(args)...) {}

            template <class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
            OPTIONAL_CONSTEXPR_INIT_LIST explicit optional(in_place_t, std::initializer_list<U> il, Args&&... args)
            : OptionalBase<T>(in_place_t{}, il, constexpr_forward<Args>(args)...) {}

            // 20.5.4.2, Destructor
            ~optional() = default;

            // 20.5.4.3, assignment
            optional& operator=(nullopt_t) noexcept
            {
    clear();
    return *this;
            }

            optional& operator=(const optional& rhs)
            {
    if      (initialized() == true  && rhs.initialized() == false) clear();
    else if (initialized() == false && rhs.initialized() == true)  initialize(*rhs);
    else if (initialized() == true  && rhs.initialized() == true)  contained_val() = *rhs;
    return *this;
            }

            optional& operator=(optional&& rhs)
            noexcept(is_nothrow_move_assignable<T>::value && is_nothrow_move_constructible<T>::value)
            {
    if      (initialized() == true  && rhs.initialized() == false) clear();
        else if (initialized() == false && rhs.initialized() == true)  initialize(std::move(*rhs));
            else if (initialized() == true  && rhs.initialized() == true)  contained_val() = std::move(*rhs);
    return *this;
            }

            template <class U>
            auto operator=(U&& v)
            -> typename enable_if
            <
            is_same<typename decay<U>::type, T>::value,
            optional&
            >::type
            {
    if (initialized()) { contained_val() = std::forward<U>(v); }
    else               { initialize(std::forward<U>(v));  }
    return *this;
            }


            template <class... Args>
            void emplace(Args&&... args)
            {
    clear();
    initialize(std::forward<Args>(args)...);
            }

            template <class U, class... Args>
            void emplace(initializer_list<U> il, Args&&... args)
            {
    clear();
    initialize<U, Args...>(il, std::forward<Args>(args)...);
            }

            // 20.5.4.4, Swap
            void swap(optional<T>& rhs) noexcept(is_nothrow_move_constructible<T>::value && noexcept(swap(declval<T&>(), declval<T&>())))
            {
    if      (initialized() == true  && rhs.initialized() == false) { rhs.initialize(std::move(**this)); clear(); }
    else if (initialized() == false && rhs.initialized() == true)  { initialize(std::move(*rhs)); rhs.clear(); }
    else if (initialized() == true  && rhs.initialized() == true)  { using std::swap; swap(**this, *rhs); }
            }

            // 20.5.4.5, Observers

            explicit constexpr operator bool() const noexcept { return initialized(); }

            constexpr T const* operator ->() const {
    return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), dataptr());
            }

# if OPTIONAL_HAS_MOVE_ACCESSORS == 1

            OPTIONAL_MUTABLE_CONSTEXPR T* operator ->() {
    assert (initialized());
    return dataptr();
            }

            constexpr T const& operator *() const& {
    return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
            }

            OPTIONAL_MUTABLE_CONSTEXPR T& operator *() & {
    assert (initialized());
    return contained_val();
            }

            OPTIONAL_MUTABLE_CONSTEXPR T&& operator *() && {
    assert (initialized());
    return constexpr_move(contained_val());
            }

            constexpr T const& value() const& {
    return initialized() ? contained_val() : (valijson::throwRuntimeError("bad optional access"), contained_val());
            }

            OPTIONAL_MUTABLE_CONSTEXPR T& value() & {
    return initialized() ? contained_val() : (valijson::throwRuntimeError("bad optional access"), contained_val());
            }

            OPTIONAL_MUTABLE_CONSTEXPR T&& value() && {
    if (!initialized()) valijson::throwRuntimeError("bad optional access");
        return std::move(contained_val());
            }

# else

            T* operator ->() {
    assert (initialized());
    return dataptr();
            }

            constexpr T const& operator *() const {
    return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
            }

            T& operator *() {
    assert (initialized());
    return contained_val();
            }

            constexpr T const& value() const {
    return initialized() ? contained_val() : (valijson::throwRuntimeError("bad optional access"), contained_val());
            }

            T& value() {
    return initialized() ? contained_val() : (valijson::throwRuntimeError("bad optional access"), contained_val());
            }

# endif

# if OPTIONAL_HAS_THIS_RVALUE_REFS == 1

            template <class V>
            constexpr T value_or(V&& v) const&
            {
    return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
            }

#   if OPTIONAL_HAS_MOVE_ACCESSORS == 1

            template <class V>
            OPTIONAL_MUTABLE_CONSTEXPR T value_or(V&& v) &&
            {
    return *this ? constexpr_move(const_cast<optional<T>&>(*this).contained_val()) : detail_::convert<T>(constexpr_forward<V>(v));
            }

#   else

            template <class V>
            T value_or(V&& v) &&
            {
    return *this ? constexpr_move(const_cast<optional<T>&>(*this).contained_val()) : detail_::convert<T>(constexpr_forward<V>(v));
            }

#   endif

# else

            template <class V>
            constexpr T value_or(V&& v) const
            {
    return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
            }

# endif

        };


        template <class T>
        class optional<T&>
        {
            static_assert( !std::is_same<T, nullopt_t>::value, "bad T" );
            static_assert( !std::is_same<T, in_place_t>::value, "bad T" );
            T* ref;

        public:

            // 20.5.5.1, construction/destruction
            constexpr optional() noexcept : ref(nullptr) {}

            constexpr optional(nullopt_t) noexcept : ref(nullptr) {}

            constexpr optional(T& v) noexcept : ref(detail_::static_addressof(v)) {}

            optional(T&&) = delete;

            constexpr optional(const optional& rhs) noexcept : ref(rhs.ref) {}

            explicit constexpr optional(in_place_t, T& v) noexcept : ref(detail_::static_addressof(v)) {}

            explicit optional(in_place_t, T&&) = delete;

            ~optional() = default;

            // 20.5.5.2, mutation
            optional& operator=(nullopt_t) noexcept {
    ref = nullptr;
    return *this;
            }

            // optional& operator=(const optional& rhs) noexcept {
            // ref = rhs.ref;
            // return *this;
            // }

            // optional& operator=(optional&& rhs) noexcept {
            // ref = rhs.ref;
            // return *this;
            // }

            template <typename U>
            auto operator=(U&& rhs) noexcept
            -> typename enable_if
            <
            is_same<typename decay<U>::type, optional<T&>>::value,
            optional&
            >::type
            {
    ref = rhs.ref;
    return *this;
            }

            template <typename U>
            auto operator=(U&& rhs) noexcept
            -> typename enable_if
            <
            !is_same<typename decay<U>::type, optional<T&>>::value,
            optional&
            >::type
            = delete;

            void emplace(T& v) noexcept {
    ref = detail_::static_addressof(v);
            }

            void emplace(T&&) = delete;


            void swap(optional<T&>& rhs) noexcept
            {
    std::swap(ref, rhs.ref);
            }

            // 20.5.5.3, observers
            constexpr T* operator->() const {
    return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, ref);
            }

            constexpr T& operator*() const {
    return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, *ref);
            }

            constexpr T& value() const {
    return ref ? *ref : (valijson::throwRuntimeError("bad optional access"), *ref);
            }

            explicit constexpr operator bool() const noexcept {
    return ref != nullptr;
            }

            template <class V>
            constexpr typename decay<T>::type value_or(V&& v) const
            {
    return *this ? **this : detail_::convert<typename decay<T>::type>(constexpr_forward<V>(v));
            }
        };


        template <class T>
        class optional<T&&>
        {
            static_assert( sizeof(T) == 0, "optional rvalue references disallowed" );
        };


        // 20.5.8, Relational operators
        template <class T> constexpr bool operator==(const optional<T>& x, const optional<T>& y)
        {
            return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
        }

        template <class T> constexpr bool operator!=(const optional<T>& x, const optional<T>& y)
        {
            return !(x == y);
        }

        template <class T> constexpr bool operator<(const optional<T>& x, const optional<T>& y)
        {
            return (!y) ? false : (!x) ? true : *x < *y;
        }

        template <class T> constexpr bool operator>(const optional<T>& x, const optional<T>& y)
        {
            return (y < x);
        }

        template <class T> constexpr bool operator<=(const optional<T>& x, const optional<T>& y)
        {
            return !(y < x);
        }

        template <class T> constexpr bool operator>=(const optional<T>& x, const optional<T>& y)
        {
            return !(x < y);
        }


        // 20.5.9, Comparison with nullopt
        template <class T> constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept
        {
            return (!x);
        }

        template <class T> constexpr bool operator==(nullopt_t, const optional<T>& x) noexcept
        {
            return (!x);
        }

        template <class T> constexpr bool operator!=(const optional<T>& x, nullopt_t) noexcept
        {
            return bool(x);
        }

        template <class T> constexpr bool operator!=(nullopt_t, const optional<T>& x) noexcept
        {
            return bool(x);
        }

        template <class T> constexpr bool operator<(const optional<T>&, nullopt_t) noexcept
        {
            return false;
        }

        template <class T> constexpr bool operator<(nullopt_t, const optional<T>& x) noexcept
        {
            return bool(x);
        }

        template <class T> constexpr bool operator<=(const optional<T>& x, nullopt_t) noexcept
        {
            return (!x);
        }

        template <class T> constexpr bool operator<=(nullopt_t, const optional<T>&) noexcept
        {
            return true;
        }

        template <class T> constexpr bool operator>(const optional<T>& x, nullopt_t) noexcept
        {
            return bool(x);
        }

        template <class T> constexpr bool operator>(nullopt_t, const optional<T>&) noexcept
        {
            return false;
        }

        template <class T> constexpr bool operator>=(const optional<T>&, nullopt_t) noexcept
        {
            return true;
        }

        template <class T> constexpr bool operator>=(nullopt_t, const optional<T>& x) noexcept
        {
            return (!x);
        }



        // 20.5.10, Comparison with T
        template <class T> constexpr bool operator==(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x == v : false;
        }

        template <class T> constexpr bool operator==(const T& v, const optional<T>& x)
        {
            return bool(x) ? v == *x : false;
        }

        template <class T> constexpr bool operator!=(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x != v : true;
        }

        template <class T> constexpr bool operator!=(const T& v, const optional<T>& x)
        {
            return bool(x) ? v != *x : true;
        }

        template <class T> constexpr bool operator<(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x < v : true;
        }

        template <class T> constexpr bool operator>(const T& v, const optional<T>& x)
        {
            return bool(x) ? v > *x : true;
        }

        template <class T> constexpr bool operator>(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x > v : false;
        }

        template <class T> constexpr bool operator<(const T& v, const optional<T>& x)
        {
            return bool(x) ? v < *x : false;
        }

        template <class T> constexpr bool operator>=(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x >= v : false;
        }

        template <class T> constexpr bool operator<=(const T& v, const optional<T>& x)
        {
            return bool(x) ? v <= *x : false;
        }

        template <class T> constexpr bool operator<=(const optional<T>& x, const T& v)
        {
            return bool(x) ? *x <= v : true;
        }

        template <class T> constexpr bool operator>=(const T& v, const optional<T>& x)
        {
            return bool(x) ? v >= *x : true;
        }


        // Comparison of optional<T&> with T
        template <class T> constexpr bool operator==(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x == v : false;
        }

        template <class T> constexpr bool operator==(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v == *x : false;
        }

        template <class T> constexpr bool operator!=(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x != v : true;
        }

        template <class T> constexpr bool operator!=(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v != *x : true;
        }

        template <class T> constexpr bool operator<(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x < v : true;
        }

        template <class T> constexpr bool operator>(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v > *x : true;
        }

        template <class T> constexpr bool operator>(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x > v : false;
        }

        template <class T> constexpr bool operator<(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v < *x : false;
        }

        template <class T> constexpr bool operator>=(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x >= v : false;
        }

        template <class T> constexpr bool operator<=(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v <= *x : false;
        }

        template <class T> constexpr bool operator<=(const optional<T&>& x, const T& v)
        {
            return bool(x) ? *x <= v : true;
        }

        template <class T> constexpr bool operator>=(const T& v, const optional<T&>& x)
        {
            return bool(x) ? v >= *x : true;
        }

        // Comparison of optional<T const&> with T
        template <class T> constexpr bool operator==(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x == v : false;
        }

        template <class T> constexpr bool operator==(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v == *x : false;
        }

        template <class T> constexpr bool operator!=(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x != v : true;
        }

        template <class T> constexpr bool operator!=(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v != *x : true;
        }

        template <class T> constexpr bool operator<(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x < v : true;
        }

        template <class T> constexpr bool operator>(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v > *x : true;
        }

        template <class T> constexpr bool operator>(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x > v : false;
        }

        template <class T> constexpr bool operator<(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v < *x : false;
        }

        template <class T> constexpr bool operator>=(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x >= v : false;
        }

        template <class T> constexpr bool operator<=(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v <= *x : false;
        }

        template <class T> constexpr bool operator<=(const optional<const T&>& x, const T& v)
        {
            return bool(x) ? *x <= v : true;
        }

        template <class T> constexpr bool operator>=(const T& v, const optional<const T&>& x)
        {
            return bool(x) ? v >= *x : true;
        }


        // 20.5.12, Specialized algorithms
        template <class T>
        void swap(optional<T>& x, optional<T>& y) noexcept(noexcept(x.swap(y)))
        {
            x.swap(y);
        }


        template <class T>
        constexpr optional<typename decay<T>::type> make_optional(T&& v)
        {
            return optional<typename decay<T>::type>(constexpr_forward<T>(v));
        }

        template <class X>
        constexpr optional<X&> make_optional(reference_wrapper<X> v)
        {
            return optional<X&>(v.get());
        }


    } // namespace experimental
} // namespace std

namespace std
{
    template <typename T>
    struct hash<std::experimental::optional<T>>
    {
        typedef typename hash<T>::result_type result_type;
        typedef std::experimental::optional<T> argument_type;

        constexpr result_type operator()(argument_type const& arg) const {
            return arg ? std::hash<T>{}(*arg) : result_type{};
        }
    };

    template <typename T>
    struct hash<std::experimental::optional<T&>>
    {
        typedef typename hash<T>::result_type result_type;
        typedef std::experimental::optional<T&> argument_type;

        constexpr result_type operator()(argument_type const& arg) const {
            return arg ? std::hash<T>{}(*arg) : result_type{};
        }
    };
}

# undef TR2_OPTIONAL_REQUIRES
# undef TR2_OPTIONAL_ASSERTED_EXPRESSION

# endif //___OPTIONAL_HPP___
#pragma once

namespace opt = std::experimental;
#pragma once

#include <functional>

namespace valijson {
namespace adapters {

class FrozenValue;

/**
 * @brief   An interface that encapsulates access to the JSON values provided
 *          by a JSON parser implementation.
 *
 * This interface allows JSON processing code to be parser-agnostic. It provides
 * functions to access the plain old datatypes (PODs) that are described in the
 * JSON specification, and callback-based access to the contents of arrays and
 * objects.
 *
 * The interface also defines a set of functions that allow for type-casting and
 * type-comparison based on value rather than on type.
 */
class Adapter
{
public:

    /// Typedef for callback function supplied to applyToArray.
    typedef std::function<bool (const Adapter &)>
        ArrayValueCallback;

    /// Typedef for callback function supplied to applyToObject.
    typedef std::function<bool (const std::string &, const Adapter &)>
        ObjectMemberCallback;

    /**
     * @brief   Virtual destructor defined to ensure deletion via base-class
     *          pointers is safe.
     */
    virtual ~Adapter() = default;

    /**
     * @brief   Apply a callback function to each value in an array.
     *
     * The callback function is invoked for each element in the array, until
     * it has been applied to all values, or it returns false.
     *
     * @param   fn  Callback function to invoke
     *
     * @returns true if Adapter contains an array and all values are equal,
     *          false otherwise.
     */
    virtual bool applyToArray(ArrayValueCallback fn) const = 0;

    /**
     * @brief   Apply a callback function to each member in an object.
     *
     * The callback function shall be invoked for each member in the object,
     * until it has been applied to all values, or it returns false.
     *
     * @param   fn  Callback function to invoke
     *
     * @returns true if Adapter contains an object, and callback function
     *          returns true for each member in the object, false otherwise.
     */
    virtual bool applyToObject(ObjectMemberCallback fn) const = 0;

    /**
     * @brief   Return the boolean representation of the contained value.
     *
     * This function shall return a boolean value if the Adapter contains either
     * an actual boolean value, or one of the strings 'true' or 'false'.
     * The string comparison is case sensitive.
     *
     * An exception shall be thrown if the value cannot be cast to a boolean.
     *
     * @returns  Boolean representation of contained value.
     */
    virtual bool asBool() const = 0;

    /**
     * @brief   Retrieve the boolean representation of the contained value.
     *
     * This function shall retrieve a boolean value if the Adapter contains
     * either an actual boolean value, or one of the strings 'true' or 'false'.
     * The string comparison is case sensitive.
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a bool to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asBool(bool &result) const = 0;

    /**
     * @brief   Return the double representation of the contained value.
     *
     * This function shall return a double value if the Adapter contains either
     * an actual double, an integer, or a string that contains a valid
     * representation of a numeric value (according to the C++ Std Library).
     *
     * An exception shall be thrown if the value cannot be cast to a double.
     *
     * @returns  Double representation of contained value.
     */
    virtual double asDouble() const = 0;

    /**
     * @brief   Retrieve the double representation of the contained value.
     *
     * This function shall retrieve a double value if the Adapter contains either
     * an actual double, an integer, or a string that contains a valid
     * representation of a numeric value (according to the C++ Std Library).
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a double to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asDouble(double &result) const = 0;

    /**
     * @brief   Return the int64_t representation of the contained value.
     *
     * This function shall return an int64_t value if the Adapter contains either
     * an actual integer, or a string that contains a valid representation of an
     * integer value (according to the C++ Std Library).
     *
     * An exception shall be thrown if the value cannot be cast to an int64_t.
     *
     * @returns  int64_t representation of contained value.
     */
    virtual int64_t asInteger() const = 0;

    /**
     * @brief   Retrieve the int64_t representation of the contained value.
     *
     * This function shall retrieve an int64_t value if the Adapter contains
     * either an actual integer, or a string that contains a valid
     * representation of an integer value (according to the C++ Std Library).
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a int64_t to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asInteger(int64_t &result) const = 0;

    /**
     * @brief   Return the string representation of the contained value.
     *
     * This function shall return a string value if the Adapter contains either
     * an actual string, a literal value of another POD type, an empty array,
     * an empty object, or null.
     *
     * An exception shall be thrown if the value cannot be cast to a string.
     *
     * @returns  string representation of contained value.
     */
    virtual std::string asString() const = 0;

    /**
     * @brief   Retrieve the string representation of the contained value.
     *
     * This function shall retrieve a string value if the Adapter contains either
     * an actual string, a literal value of another POD type, an empty array,
     * an empty object, or null.
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a string to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asString(std::string &result) const = 0;

    /**
     * @brief   Compare the value held by this Adapter instance with the value
     *          held by another Adapter instance.
     *
     * @param   other   the other adapter instance
     * @param   strict  flag to use strict type comparison
     *
     * @returns true if values are equal, false otherwise
     */
    virtual bool equalTo(const Adapter &other, bool strict) const = 0;

    /**
     * @brief   Create a new FrozenValue instance that is equivalent to the
     *          value contained by the Adapter.
     *
     * @returns pointer to a new FrozenValue instance, belonging to the caller.
     */
    virtual FrozenValue* freeze() const = 0;

    /**
     * @brief   Return the number of elements in the array.
     *
     * Throws an exception if the value is not an array.
     *
     * @return  number of elements if value is an array
     */
    virtual size_t getArraySize() const = 0;

    /**
     * @brief   Retrieve the number of elements in the array.
     *
     * This function shall return true or false to indicate whether or not the
     * result value was set. If the contained value is not an array, the
     * result value shall not be set. This applies even if the value could be
     * cast to an empty array. The calling code is expected to handles those
     * cases manually.
     *
     * @param   result  reference to size_t variable to set with result.
     *
     * @return  true if value retrieved successfully, false otherwise.
     */
    virtual bool getArraySize(size_t &result) const = 0;

    /**
     * @brief   Return the contained boolean value.
     *
     * This function shall throw an exception if the contained value is not a
     * boolean.
     *
     * @returns contained boolean value.
     */
    virtual bool getBool() const = 0;

    /**
     * @brief   Retrieve the contained boolean value.
     *
     * This function shall retrieve the boolean value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to boolean variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getBool(bool &result) const = 0;

    /**
     * @brief   Return the contained double value.
     *
     * This function shall throw an exception if the contained value is not a
     * double.
     *
     * @returns contained double value.
     */
    virtual double getDouble() const = 0;

    /**
     * @brief   Retrieve the contained double value.
     *
     * This function shall retrieve the double value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to double variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getDouble(double &result) const = 0;

    /**
     * @brief   Return the contained integer value.
     *
     * This function shall throw an exception if the contained value is not a
     * integer.
     *
     * @returns contained integer value.
     */
    virtual int64_t getInteger() const = 0;

    /**
     * @brief   Retrieve the contained integer value.
     *
     * This function shall retrieve the integer value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to integer variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getInteger(int64_t &result) const = 0;

    /**
     * @brief   Return the contained numeric value as a double.
     *
     * This function shall throw an exception if the contained value is not a
     * integer or a double.
     *
     * @returns contained double or integral value.
     */
    virtual double getNumber() const = 0;

    /**
     * @brief   Retrieve the contained numeric value as a double.
     *
     * This function shall retrieve the double or integral value contained by
     * this Adapter, and store it in the result variable that was passed by
     * reference.
     *
     * @param   result  reference to double variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getNumber(double &result) const = 0;

    /**
     * @brief   Return the number of members in the object.
     *
     * Throws an exception if the value is not an object.
     *
     * @return  number of members if value is an object
     */
    virtual size_t getObjectSize() const = 0;

    /**
     * @brief   Retrieve the number of members in the object.
     *
     * This function shall return true or false to indicate whether or not the
     * result value was set. If the contained value is not an object, the
     * result value shall not be set. This applies even if the value could be
     * cast to an empty object. The calling code is expected to handles those
     * cases manually.
     *
     * @param   result  reference to size_t variable to set with result.
     *
     * @return  true if value retrieved successfully, false otherwise.
     */
    virtual bool getObjectSize(size_t &result) const = 0;

    /**
     * @brief   Return the contained string value.
     *
     * This function shall throw an exception if the contained value is not a
     * string - even if the value could be cast to a string. The asString()
     * function should be used when casting is allowed.
     *
     * @returns string contained by this Adapter
     */
    virtual std::string getString() const = 0;

    /**
     * @brief   Retrieve the contained string value.
     *
     * This function shall retrieve the string value contained by this Adapter,
     * and store it in result variable that is passed by reference.
     *
     * @param   result  reference to string to set with result
     *
     * @returns true if string was retrieved, false otherwise
     */
    virtual bool getString(std::string &result) const = 0;

    /**
     * @brief   Returns whether or not this Adapter supports strict types.
     *
     * This function shall return true if the Adapter implementation supports
     * strict types, or false if the Adapter fails to store any part of the
     * type information supported by the Adapter interface.
     *
     * For example, the PropertyTreeAdapter implementation stores POD values as
     * strings, effectively discarding any other type information. If you were
     * to call isDouble() on a double stored by this Adapter, the result would
     * be false. The maybeDouble(), asDouble() and various related functions
     * are provided to perform type checking based on value rather than on type.
     *
     * The BasicAdapter template class provides implementations for the type-
     * casting functions so that Adapter implementations are semantically
     * equivalent in their type-casting behaviour.
     *
     * @returns true if Adapter supports strict types, false otherwise
     */
    virtual bool hasStrictTypes() const = 0;

    /// Returns true if the contained value is definitely an array.
    virtual bool isArray() const = 0;

    /// Returns true if the contained value is definitely a boolean.
    virtual bool isBool() const = 0;

    /// Returns true if the contained value is definitely a double.
    virtual bool isDouble() const = 0;

    /// Returns true if the contained value is definitely an integer.
    virtual bool isInteger() const = 0;

    /// Returns true if the contained value is definitely a null.
    virtual bool isNull() const = 0;

    /// Returns true if the contained value is either a double or an integer.
    virtual bool isNumber() const = 0;

    /// Returns true if the contained value is definitely an object.
    virtual bool isObject() const = 0;

    /// Returns true if the contained value is definitely a string.
    virtual bool isString() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an array.
     *
     * @returns true if the contained value is an array, an empty string, or an
     *          empty object.
     */
    virtual bool maybeArray() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a boolean.
     *
     * @returns true if the contained value is a boolean, or one of the strings
     *          'true' or 'false'. Note that numeric values are not to be cast
     *          to boolean values.
     */
    virtual bool maybeBool() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a double.
     *
     * @returns true if the contained value is a double, an integer, or a string
     *          containing a double or integral value.
     */
    virtual bool maybeDouble() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an integer.
     *
     * @returns true if the contained value is an integer, or a string
     *          containing an integral value.
     */
    virtual bool maybeInteger() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a null.
     *
     * @returns true if the contained value is null or an empty string.
     */
    virtual bool maybeNull() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an object.
     *
     * @returns true if the contained value is an object, an empty array or
     *          an empty string.
     */
    virtual bool maybeObject() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a string.
     *
     * @returns true if the contained value is a non-null POD type, an empty
     *          array, or an empty object.
     */
    virtual bool maybeString() const = 0;
};

/**
 * @brief  Template struct that should be specialised for each concrete Adapter
 *         class.
 *
 * @deprecated  This is a bit of a hack, and I'd like to remove it.
 */
template<typename T>
struct AdapterTraits
{

};

}  // namespace adapters
}  // namespace valijson
#pragma once

#include <cstdint>
#include <sstream>


namespace valijson {
namespace adapters {

/**
 * @brief  A helper for the array and object member iterators.
 *
 * See http://www.stlsoft.org/doc-1.9/group__group____pattern____dereference__proxy.html
 * for motivation
 *
 * @tparam Value  Name of the value type
 */
template<class Value>
struct DerefProxy
{
    explicit DerefProxy(const Value& x)
      : m_ref(x) { }

    Value* operator->()
    {
        return std::addressof(m_ref);
    }

    explicit operator Value*()
    {
        return std::addressof(m_ref);
    }

private:
    Value m_ref;
};

/**
 * @brief  Template class that implements the expected semantics of an Adapter.
 *
 * Implementing all of the type-casting functionality for each Adapter is error
 * prone and tedious, so this template class aims to minimise the duplication
 * of code between various Adapter implementations. This template doesn't quite
 * succeed in removing all duplication, but it has greatly simplified the
 * implementation of a new Adapter by encapsulating the type-casting semantics
 * and a lot of the trivial functionality associated with the Adapter interface.
 *
 * By inheriting from this template class, Adapter implementations will inherit
 * the exception throwing behaviour that is expected by other parts of the
 * Valijson library.
 *
 * @tparam  AdapterType       Self-referential name of the Adapter being
 *                            specialised.
 * @tparam  ArrayType         Name of the type that will be returned by the
 *                            getArray() function. Instances of this type should
 *                            provide begin(), end() and size() functions so
 *                            that it is possible to iterate over the values in
 *                            the array.
 * @tparam  ObjectMemberType  Name of the type exposed when iterating over the
 *                            contents of an object returned by getObject().
 * @tparam  ObjectType        Name of the type that will be returned by the
 *                            getObject() function. Instances of this type
 *                            should provide begin(), end(), find() and size()
 *                            functions so that it is possible to iterate over
 *                            the members of the object.
 * @tparam  ValueType         Name of the type that provides a consistent
 *                            interface to a JSON value for a parser. For
 *                            example, this type should provide the getDouble()
 *                            and isDouble() functions. But it does not need to
 *                            know how to cast values from one type to another -
 *                            that functionality is provided by this template
 *                            class.
 */
template<
    typename AdapterType,
    typename ArrayType,
    typename ObjectMemberType,
    typename ObjectType,
    typename ValueType>
class BasicAdapter: public Adapter
{
protected:

    /**
     * @brief   Functor for comparing two arrays.
     *
     * This functor is used to compare the elements in an array of the type
     * ArrayType with individual values provided as generic Adapter objects.
     * Comparison is performed by the () operator.
     *
     * The functor works by maintaining an iterator for the current position
     * in an array. Each time the () operator is called, the value at this
     * position is compared with the value passed as an argument to ().
     * Immediately after the comparison, the iterator will be incremented.
     *
     * This functor is designed to be passed to the applyToArray() function
     * of an Adapter object.
     */
    class ArrayComparisonFunctor
    {
    public:

        /**
         * @brief   Construct an ArrayComparisonFunctor for an array.
         *
         * @param   array   Array to compare values against
         * @param   strict  Flag to use strict type comparison
         */
        ArrayComparisonFunctor(const ArrayType &array, bool strict)
          : m_itr(array.begin()),
            m_end(array.end()),
            m_strict(strict) { }

        /**
         * @brief   Compare a value against the current element in the array.
         *
         * @param   adapter  Value to be compared with current element
         *
         * @returns true if values are equal, false otherwise.
         */
        bool operator()(const Adapter &adapter)
        {
            if (m_itr == m_end) {
                return false;
            }

            return AdapterType(*m_itr++).equalTo(adapter, m_strict);
        }

    private:

        /// Iterator for current element in the array
        typename ArrayType::const_iterator m_itr;

        /// Iterator for one-past the last element of the array
        typename ArrayType::const_iterator m_end;

        /// Flag to use strict type comparison
        const bool m_strict;
    };

    /**
     * @brief   Functor for comparing two objects
     *
     * This functor is used to compare the members of an object of the type
     * ObjectType with key-value pairs belonging to another object.
     *
     * The functor works by maintaining a reference to an object provided via
     * the constructor. When time the () operator is called with a key-value
     * pair as arguments, the function will attempt to find the key in the
     * base object. If found, the associated value will be compared with the
     * value provided to the () operator.
     *
     * This functor is designed to be passed to the applyToObject() function
     * of an Adapter object.
     */
    class ObjectComparisonFunctor
    {
    public:

        /**
         * @brief   Construct a new ObjectComparisonFunctor for an object.
         *
         * @param   object  object to use as comparison baseline
         * @param   strict  flag to use strict type-checking
         */
        ObjectComparisonFunctor(const ObjectType &object, bool strict)
          : m_object(object),
            m_strict(strict) { }

        /**
         * @brief   Find a key in the object and compare its value.
         *
         * @param   key    Key to find
         * @param   value  Value to be compared against
         *
         * @returns true if key is found and values are equal, false otherwise.
         */
        bool operator()(const std::string &key, const Adapter &value)
        {
            const typename ObjectType::const_iterator itr = m_object.find(key);
            if (itr == m_object.end()) {
                return false;
            }

            return (*itr).second.equalTo(value, m_strict);
        }

    private:

        /// Object to be used as a comparison baseline
        const ObjectType &m_object;

        /// Flag to use strict type-checking
        bool m_strict;
    };


public:

    /// Alias for ArrayType template parameter
    typedef ArrayType Array;

    /// Alias for ObjectMemberType template parameter
    typedef ObjectMemberType ObjectMember;

    /// Alias for ObjectType template parameter
    typedef ObjectType Object;

    /**
     * @brief   Construct an Adapter using the default value.
     *
     * This constructor relies on the default constructor of the ValueType
     * class provided as a template argument.
     */
    BasicAdapter() = default;

    /**
     * @brief   Construct an Adapter using a specified ValueType object.
     *
     * This constructor relies on the copy constructor of the ValueType
     * class provided as template argument.
     */
    explicit BasicAdapter(const ValueType &value)
      : m_value(value) { }

    bool applyToArray(ArrayValueCallback fn) const override
    {
        if (!maybeArray()) {
            return false;
        }

        // Due to the fact that the only way a value can be 'maybe an array' is
        // if it is an empty string or empty object, we only need to go to
        // effort of constructing an ArrayType instance if the value is
        // definitely an array.
        if (m_value.isArray()) {
            const opt::optional<Array> array = m_value.getArrayOptional();
            for (const AdapterType element : *array) {
                if (!fn(element)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool applyToObject(ObjectMemberCallback fn) const override
    {
        if (!maybeObject()) {
            return false;
        }

        if (m_value.isObject()) {
            const opt::optional<Object> object = m_value.getObjectOptional();
            for (const ObjectMemberType member : *object) {
                if (!fn(member.first, AdapterType(member.second))) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief   Return an ArrayType instance containing an array representation
     *          of the value held by this Adapter.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the elements in an array. The ArrayType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained value is either an empty object, or an empty string,
     * then this function will cast the value to an empty array.
     *
     * @returns ArrayType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ArrayType asArray() const
    {
        if (m_value.isArray()) {
            return *m_value.getArrayOptional();
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return ArrayType();
            }
        } else if (m_value.isString()) {
            std::string stringValue;
            if (m_value.getString(stringValue) && stringValue.empty()) {
                return ArrayType();
            }
        }

        throwRuntimeError("JSON value cannot be cast to an array.");
    }

    bool asBool() const override
    {
        bool result;
        if (asBool(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a boolean.");
    }

    bool asBool(bool &result) const override
    {
        if (m_value.isBool()) {
            return m_value.getBool(result);
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
                if (s == "true") {
                    result = true;
                    return true;
                } else if (s == "false") {
                    result = false;
                    return true;
                }
            }
        }

        return false;
    }

    double asDouble() const override
    {
        double result;
        if (asDouble(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a double.");
    }

    bool asDouble(double &result) const override
    {
        if (m_value.isDouble()) {
            return m_value.getDouble(result);
        } else if (m_value.isInteger()) {
            int64_t i;
            if (m_value.getInteger(i)) {
                result = double(i);
                return true;
            }
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
                const char *b = s.c_str();
                char *e = nullptr;
                double x = strtod(b, &e);
                if (e == b || e != b + s.length()) {
                    return false;
                }
                result = x;
                return true;
            }
        }

        return false;
    }

    int64_t asInteger() const override
    {
        int64_t result;
        if (asInteger(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast as an integer.");
    }

    bool asInteger(int64_t &result) const override
    {
        if (m_value.isInteger()) {
            return m_value.getInteger(result);
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
                std::istringstream i(s);
                int64_t x;
                char c;
                if (!(!(i >> x) || i.get(c))) {
                    result = x;
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief   Return an ObjectType instance containing an array representation
     *          of the value held by this Adapter.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the members of the object. The ObjectType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * @returns ObjectType instance containing an object representation of the
     *          value held by this Adapter.
     */
    ObjectType asObject() const
    {
        if (m_value.isObject()) {
            return *m_value.getObjectOptional();
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return ObjectType();
            }
        } else if (m_value.isString()) {
            std::string stringValue;
            if (m_value.getString(stringValue) && stringValue.empty()) {
                return ObjectType();
            }
        }

        throwRuntimeError("JSON value cannot be cast to an object.");
    }

    std::string asString() const override
    {
        std::string result;
        if (asString(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a string.");
    }

    bool asString(std::string &result) const override
    {
        if (m_value.isString()) {
            return m_value.getString(result);
        } else if (m_value.isNull()) {
            result.clear();
            return true;
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                result.clear();
                return true;
            }
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                result.clear();
                return true;
            }
        } else if (m_value.isBool()) {
            bool boolValue;
            if (m_value.getBool(boolValue)) {
                result = boolValue ? "true" : "false";
                return true;
            }
        } else if (m_value.isInteger()) {
            int64_t integerValue;
            if (m_value.getInteger(integerValue)) {
                result = std::to_string(integerValue);
                return true;
            }
        } else if (m_value.isDouble()) {
            double doubleValue;
            if (m_value.getDouble(doubleValue)) {
                result = std::to_string(doubleValue);
                return true;
            }
        }

        return false;
    }

    bool equalTo(const Adapter &other, bool strict) const override
    {
        if (isNull() || (!strict && maybeNull())) {
            return other.isNull() || (!strict && other.maybeNull());
        } else if (isBool() || (!strict && maybeBool())) {
            return (other.isBool() || (!strict && other.maybeBool())) && other.asBool() == asBool();
        } else if (isNumber() && strict) {
            return other.isNumber() && other.getNumber() == getNumber();
        } else if (!strict && maybeDouble()) {
            return (other.maybeDouble() && other.asDouble() == asDouble());
        } else if (!strict && maybeInteger()) {
            return (other.maybeInteger() && other.asInteger() == asInteger());
        } else if (isString() || (!strict && maybeString())) {
            return (other.isString() || (!strict && other.maybeString())) &&
                other.asString() == asString();
        } else if (isArray()) {
            if (other.isArray() && getArraySize() == other.getArraySize()) {
                const opt::optional<ArrayType> array = m_value.getArrayOptional();
                if (array) {
                    ArrayComparisonFunctor fn(*array, strict);
                    return other.applyToArray(fn);
                }
            } else if (!strict && other.maybeArray() && getArraySize() == 0) {
                return true;
            }
        } else if (isObject()) {
            if (other.isObject() && other.getObjectSize() == getObjectSize()) {
                const opt::optional<ObjectType> object = m_value.getObjectOptional();
                if (object) {
                    ObjectComparisonFunctor fn(*object, strict);
                    return other.applyToObject(fn);
                }
            } else if (!strict && other.maybeObject() && getObjectSize() == 0) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief   Return an ArrayType instance representing the array contained
     *          by this Adapter instance.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the elements in an array. The ArrayType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained is not an array, this function will throw an exception.
     *
     * @returns ArrayType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ArrayType getArray() const
    {
        opt::optional<ArrayType> arrayValue = m_value.getArrayOptional();
        if (arrayValue) {
            return *arrayValue;
        }

        throwRuntimeError("JSON value is not an array.");
    }

    size_t getArraySize() const override
    {
        size_t result;
        if (m_value.getArraySize(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an array.");
    }

    bool getArraySize(size_t &result) const override
    {
        return m_value.getArraySize(result);
    }

    bool getBool() const override
    {
        bool result;
        if (getBool(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a boolean.");
    }

    bool getBool(bool &result) const override
    {
        return m_value.getBool(result);
    }

    double getDouble() const override
    {
        double result;
        if (getDouble(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a double.");
    }

    bool getDouble(double &result) const override
    {
        return m_value.getDouble(result);
    }

    int64_t getInteger() const override
    {
        int64_t result;
        if (getInteger(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an integer.");
    }

    bool getInteger(int64_t &result) const override
    {
        return m_value.getInteger(result);
    }

    double getNumber() const override
    {
        double result;
        if (getNumber(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a number.");
    }

    bool getNumber(double &result) const override
    {
        if (isDouble()) {
            return getDouble(result);
        } else if (isInteger()) {
            int64_t integerResult;
            if (getInteger(integerResult)) {
                result = static_cast<double>(integerResult);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief   Return an ObjectType instance representing the object contained
     *          by this Adapter instance.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the members of an object. The ObjectType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained is not an object, this function will throw an exception.
     *
     * @returns ObjectType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ObjectType getObject() const
    {
        opt::optional<ObjectType> objectValue = m_value.getObjectOptional();
        if (objectValue) {
            return *objectValue;
        }

        throwRuntimeError("JSON value is not an object.");
    }

    size_t getObjectSize() const override
    {
        size_t result;
        if (getObjectSize(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an object.");
    }

    bool getObjectSize(size_t &result) const override
    {
        return m_value.getObjectSize(result);
    }

    std::string getString() const override
    {
        std::string result;
        if (getString(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a string.");
    }

    bool getString(std::string &result) const override
    {
        return m_value.getString(result);
    }

    FrozenValue * freeze() const override
    {
        return m_value.freeze();
    }

    bool hasStrictTypes() const override
    {
        return ValueType::hasStrictTypes();
    }

    bool isArray() const override
    {
        return m_value.isArray();
    }

    bool isBool() const override
    {
        return m_value.isBool();
    }

    bool isDouble() const override
    {
        return m_value.isDouble();
    }

    bool isInteger() const override
    {
        return m_value.isInteger();
    }

    bool isNull() const override
    {
        return m_value.isNull();
    }

    bool isNumber() const override
    {
        return m_value.isInteger() || m_value.isDouble();
    }

    bool isObject() const override
    {
        return m_value.isObject();
    }

    bool isString() const override
    {
        return m_value.isString();
    }

    bool maybeArray() const override
    {
        if (m_value.isArray()) {
            return true;
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        }

        return false;
    }

    bool maybeBool() const override
    {
        if (m_value.isBool()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (m_value.getString(stringValue)) {
                if (stringValue == "true" || stringValue == "false") {
                    return true;
                }
            }
        }

        return false;
    }

    bool maybeDouble() const override
    {
        if (m_value.isNumber()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (m_value.getString(s)) {
                const char *b = s.c_str();
                char *e = nullptr;
                strtod(b, &e);
                return e != b && e == b + s.length();
            }
        }

        return false;
    }

    bool maybeInteger() const override
    {
        if (m_value.isInteger()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (m_value.getString(s)) {
                std::istringstream i(s);
                int64_t x;
                char c;
                if (!(i >> x) || i.get(c)) {
                    return false;
                }
                return true;
            }
        }

        return false;
    }

    bool maybeNull() const override
    {
        if (m_value.isNull()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (m_value.getString(stringValue)) {
                if (stringValue.empty()) {
                    return true;
                }
            }
        }

        return false;
    }

    bool maybeObject() const override
    {
        if (m_value.isObject()) {
            return true;
        } else if (maybeArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

    bool maybeString() const override
    {
        if (m_value.isString() || m_value.isBool() || m_value.isInteger() || m_value.isDouble()) {
            return true;
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

private:

    const ValueType m_value;
};

}  // namespace adapters
}  // namespace valijson
#pragma once

namespace valijson {
namespace internal {

template<class T>
class CustomAllocator
{
public:
    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    // Standard allocator typedefs
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U>
    struct rebind
    {
        typedef CustomAllocator<U> other;
    };

    CustomAllocator()
      : m_allocFn(::operator new),
        m_freeFn(::operator delete) { }

    CustomAllocator(CustomAlloc allocFn, CustomFree freeFn)
      : m_allocFn(allocFn),
        m_freeFn(freeFn) { }

    CustomAllocator(const CustomAllocator &other)
      : m_allocFn(other.m_allocFn),
        m_freeFn(other.m_freeFn) { }

    template<typename U>
    CustomAllocator(CustomAllocator<U> const &other)
      : m_allocFn(other.m_allocFn),
        m_freeFn(other.m_freeFn) { }

    CustomAllocator & operator=(const CustomAllocator &other)
    {
        m_allocFn = other.m_allocFn;
        m_freeFn = other.m_freeFn;

        return *this;
    }

    pointer address(reference r)
    {
        return &r;
    }

    const_pointer address(const_reference r)
    {
        return &r;
    }

    pointer allocate(size_type cnt, const void * = nullptr)
    {
        return reinterpret_cast<pointer>(m_allocFn(cnt * sizeof(T)));
    }

    void deallocate(pointer p, size_type)
    {
        m_freeFn(p);
    }

    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    void construct(pointer p, const T& t)
    {
        new(p) T(t);
    }

    void destroy(pointer p)
    {
        p->~T();
    }

    bool operator==(const CustomAllocator &other) const
    {
        return other.m_allocFn == m_allocFn && other.m_freeFn == m_freeFn;
    }

    bool operator!=(const CustomAllocator &other) const
    {
        return !operator==(other);
    }

    CustomAlloc m_allocFn;

    CustomFree m_freeFn;
};

} // end namespace internal
} // end namespace valijson
#pragma once

#include <string>

namespace valijson {
namespace internal {

template<typename AdapterType>
std::string nodeTypeAsString(const AdapterType &node) {
    if (node.isArray()) {
        return "array";
    } else if (node.isObject()) {
        return "object";
    } else if (node.isString()) {
        return "string";
    } else if (node.isNull()) {
        return "null";
    } else if (node.isInteger()) {
        return "integer";
    } else if (node.isDouble()) {
        return "double";
    } else if (node.isBool()) {
        return "bool";
    }

    return "unknown";
}

} // end namespace internal
} // end namespace valijson
#pragma once


namespace valijson {
namespace adapters {

/**
 * @brief   An interface that provides minimal access to a stored JSON value.
 *
 * The main reason that this interface exists is to support the 'enum'
 * constraint. Each Adapter type is expected to provide an implementation of
 * this interface. That class should be able to maintain its own copy of a
 * JSON value, independent of the original document.
 *
 * This interface currently provides just the clone and equalTo functions, but
 * could be expanded to include other functions declared in the Adapter
 * interface.
 *
 * @todo  it would be nice to better integrate this with the Adapter interface
 */
class FrozenValue
{
public:

    /**
     * @brief   Virtual destructor defined to ensure deletion via base-class
     *          pointers is safe.
     */
    virtual ~FrozenValue() { }

    /**
     * @brief   Clone the stored value and return a pointer to a new FrozenValue
     *          object containing the value.
     */
    virtual FrozenValue *clone() const = 0;

    /**
     * @brief   Return true if the stored value is equal to the value contained
     *          by an Adapter instance.
     *
     * @param   adapter  Adapter to compare value against
     * @param   strict   Flag to use strict type comparison
     *
     * @returns true if values are equal, false otherwise
     */
    virtual bool equalTo(const Adapter &adapter, bool strict) const = 0;

};

}  // namespace adapters
}  // namespace valijson
#pragma once

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>


#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

namespace valijson {
namespace internal {
namespace json_pointer {

/**
 * @brief   Replace all occurrences of `search` with `replace`. Modifies `subject` in place.
 *
 * @param   subject  string to operate on
 * @param   search   string to search
 * @param   replace  replacement string
 */
inline void replaceAllInPlace(std::string& subject, const char* search,
                                const char* replace)
{
    size_t pos = 0;

    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, strlen(search), replace);
        pos += strlen(replace);
    }
}

/**
 * @brief   Return the char value corresponding to a 2-digit hexadecimal string
 *
 * @throws  std::runtime_error for strings that are not exactly two characters
 *          in length and for strings that contain non-hexadecimal characters
 *
 * @return  decoded char value corresponding to the hexadecimal string
 */
inline char decodePercentEncodedChar(const std::string &digits)
{
    if (digits.length() != 2) {
        throwRuntimeError("Failed to decode %-encoded character '" +
                digits + "' due to unexpected number of characters; "
                "expected two characters");
    }

    errno = 0;
    const char *begin = digits.c_str();
    char *end = nullptr;
    const unsigned long value = strtoul(begin, &end, 16);
    if (end != begin && *end != '\0') {
        throwRuntimeError("Failed to decode %-encoded character '" +
                digits + "'");
    }

    return char(value);
}

/**
 * @brief   Extract and transform the token between two iterators
 *
 * This function is responsible for extracting a JSON Reference token from
 * between two iterators, and performing any necessary transformations, before
 * returning the resulting string. Its main purpose is to replace the escaped
 * character sequences defined in the RFC-6901 (JSON Pointer), and to decode
 * %-encoded character sequences defined in RFC-3986 (URI).
 *
 * The encoding used in RFC-3986 should be familiar to many developers, but
 * the escaped character sequences used in JSON Pointers may be less so. From
 * the JSON Pointer specification (RFC 6901, April 2013):
 *
 *    Evaluation of each reference token begins by decoding any escaped
 *    character sequence.  This is performed by first transforming any
 *    occurrence of the sequence '~1' to '/', and then transforming any
 *    occurrence of the sequence '~0' to '~'.  By performing the
 *    substitutions in this order, an implementation avoids the error of
 *    turning '~01' first into '~1' and then into '/', which would be
 *    incorrect (the string '~01' correctly becomes '~1' after
 *    transformation).
 *
 * @param   begin  iterator pointing to beginning of a token
 * @param   end    iterator pointing to one character past the end of the token
 *
 * @return  string with escaped character sequences replaced
 *
 */
inline std::string extractReferenceToken(std::string::const_iterator begin,
        std::string::const_iterator end)
{
    std::string token(begin, end);

    // Replace JSON Pointer-specific escaped character sequences
    replaceAllInPlace(token, "~1", "/");
    replaceAllInPlace(token, "~0", "~");

    // Replace %-encoded character sequences with their actual characters
    for (size_t n = token.find('%'); n != std::string::npos;
            n = token.find('%', n + 1)) {

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            const char c = decodePercentEncodedChar(token.substr(n + 1, 2));
            token.replace(n, 3, &c, 1);
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::runtime_error &e) {
            throwRuntimeError(
                    std::string(e.what()) + "; in token: " + token);
        }
#endif
    }

    return token;
}

/**
 * @brief   Recursively locate the value referenced by a JSON Pointer
 *
 * This function takes both a string reference and an iterator to the beginning
 * of the substring that is being resolved. This iterator is expected to point
 * to the beginning of a reference token, whose length will be determined by
 * searching for the next delimiter ('/' or '\0'). A reference token must be
 * at least one character in length to be considered valid.
 *
 * Once the next reference token has been identified, it will be used either as
 * an array index or as an the name an object member. The validity of a
 * reference token depends on the type of the node currently being traversed,
 * and the applicability of the token to that node. For example, an array can
 * only be dereferenced by a non-negative integral index.
 *
 * Once the next node has been identified, the length of the remaining portion
 * of the JSON Pointer will be used to determine whether recursion should
 * terminate.
 *
 * @param   node            current node in recursive evaluation of JSON Pointer
 * @param   jsonPointer     string containing complete JSON Pointer
 * @param   jsonPointerItr  string iterator pointing the beginning of the next
 *                          reference token
 *
 * @return  an instance of AdapterType that wraps the dereferenced node
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &node,
        const std::string &jsonPointer,
        const std::string::const_iterator jsonPointerItr)
{
    // TODO: This function will probably need to implement support for
    // fetching documents referenced by JSON Pointers, similar to the
    // populateSchema function.

    const std::string::const_iterator jsonPointerEnd = jsonPointer.end();

    // Terminate recursion if all reference tokens have been consumed
    if (jsonPointerItr == jsonPointerEnd) {
        return node;
    }

    // Reference tokens must begin with a leading slash
    if (*jsonPointerItr != '/') {
        throwRuntimeError("Expected reference token to begin with "
                "leading slash; remaining tokens: " +
                std::string(jsonPointerItr, jsonPointerEnd));
    }

    // Find iterator that points to next slash or newline character; this is
    // one character past the end of the current reference token
    std::string::const_iterator jsonPointerNext =
            std::find(jsonPointerItr + 1, jsonPointerEnd, '/');

    // Extract the next reference token
    const std::string referenceToken = extractReferenceToken(
            jsonPointerItr + 1, jsonPointerNext);

    // Empty reference tokens should be ignored
    if (referenceToken.empty()) {
        return resolveJsonPointer(node, jsonPointer, jsonPointerNext);

    } else if (node.isArray()) {
        if (referenceToken == "-") {
            throwRuntimeError("Hyphens cannot be used as array indices "
                    "since the requested array element does not yet exist");
        }

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            // Fragment must be non-negative integer
            const uint64_t index = std::stoul(referenceToken);
            typedef typename AdapterType::Array Array;
            const Array arr = node.asArray();
            typename Array::const_iterator itr = arr.begin();
            const uint64_t arrSize = arr.size();

            if (arrSize == 0 || index > arrSize - 1) {
                throwRuntimeError("Expected reference token to identify "
                        "an element in the current array, but array index is "
                        "out of bounds; actual token: " + referenceToken);
            }

            if (index > static_cast<uint64_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
                throwRuntimeError("Array index out of bounds; hard "
                        "limit is " + std::to_string(
                                std::numeric_limits<std::ptrdiff_t>::max()));
            }

            itr.advance(static_cast<std::ptrdiff_t>(index));

            // Recursively process the remaining tokens
            return resolveJsonPointer(*itr, jsonPointer, jsonPointerNext);

#if VALIJSON_USE_EXCEPTIONS
        } catch (std::invalid_argument &) {
            throwRuntimeError("Expected reference token to contain a "
                    "non-negative integer to identify an element in the "
                    "current array; actual token: " + referenceToken);
        }
#endif
    } else if (node.maybeObject()) {
        // Fragment must identify a member of the candidate object
        typedef typename AdapterType::Object Object;

        const Object object = node.asObject();
        typename Object::const_iterator itr = object.find(
                referenceToken);
        if (itr == object.end()) {
            throwRuntimeError("Expected reference token to identify an "
                    "element in the current object; "
                    "actual token: " + referenceToken);
            abort();
        }

        // Recursively process the remaining tokens
        return resolveJsonPointer(itr->second, jsonPointer, jsonPointerNext);
    }

    throwRuntimeError("Expected end of JSON Pointer, but at least "
            "one reference token has not been processed; remaining tokens: " +
            std::string(jsonPointerNext, jsonPointerEnd));
    abort();
}

/**
 * @brief   Return the JSON Value referenced by a JSON Pointer
 *
 * @param   rootNode     node to use as root for JSON Pointer resolution
 * @param   jsonPointer  string containing JSON Pointer
 *
 * @return  an instance AdapterType in the specified document
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &rootNode,
        const std::string &jsonPointer)
{
    return resolveJsonPointer(rootNode, jsonPointer, jsonPointer.begin());
}

} // namespace json_pointer
} // namespace internal
} // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif
#pragma once

#include <stdexcept>
#include <string>


namespace valijson {
namespace internal {
namespace json_reference {

/**
  * @brief   Extract URI from JSON Reference relative to the current schema
  *
  * @param   jsonRef  JSON Reference to extract from
  * @param   schema   Schema that JSON Reference URI is relative to
  *
  * @return  Optional string containing URI
  */
inline opt::optional<std::string> getJsonReferenceUri(
    const std::string &jsonRef)
{
    const size_t ptrPos = jsonRef.find('#');
    if (ptrPos == 0) {
        // The JSON Reference does not contain a URI, but might contain a
        // JSON Pointer that refers to the current document
        return opt::optional<std::string>();
    } else if (ptrPos != std::string::npos) {
        // The JSON Reference contains a URI and possibly a JSON Pointer
        return jsonRef.substr(0, ptrPos);
    }

    // The entire JSON Reference should be treated as a URI
    return jsonRef;
}

/**
  * @brief   Extract JSON Pointer portion of a JSON Reference
  *
  * @param   jsonRef  JSON Reference to extract from
  *
  * @return  Optional string containing JSON Pointer
  */
inline opt::optional<std::string> getJsonReferencePointer(
    const std::string &jsonRef)
{
    // Attempt to extract JSON Pointer if '#' character is present. Note
    // that a valid pointer would contain at least a leading forward
    // slash character.
    const size_t ptrPos = jsonRef.find('#');
    if (ptrPos != std::string::npos) {
        return jsonRef.substr(ptrPos + 1);
    }

    return opt::optional<std::string>();
}

} // namespace json_reference
} // namespace internal
} // namespace valijson
#pragma once

#include <regex>
#include <string>

namespace valijson {
namespace internal {
namespace uri {

/**
  * @brief  Placeholder function to check whether a URI is absolute
  *
  * This function just checks for '://'
  */
inline bool isUriAbsolute(const std::string &documentUri)
{
    static const char * placeholderMarker = "://";

    return documentUri.find(placeholderMarker) != std::string::npos;
}

/**
 * @brief  Placeholder function to check whether a URI is a URN
 *
 * This function validates that the URI matches the RFC 8141 spec
 */
inline bool isUrn(const std::string &documentUri) {
  static const std::regex pattern(
      "^((urn)|(URN)):(?!urn:)([a-zA-Z0-9][a-zA-Z0-9-]{1,31})(:[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;=]+)+(\\?[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}(#[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}$");

  return std::regex_match(documentUri, pattern);
}

/**
 * Placeholder function to resolve a relative URI within a given scope
 */
inline std::string resolveRelativeUri(
        const std::string &resolutionScope,
        const std::string &relativeUri)
{
    return resolutionScope + relativeUri;
}

} // namespace uri
} // namespace internal
} // namespace valijson
#pragma once

#include <fstream>
#include <limits>

namespace valijson {
namespace utils {

/**
 * Load a file into a string
 *
 * @param  path  path to the file to be loaded
 * @param  dest  string into which file should be loaded
 *
 * @return  true if loaded, false otherwise
 */
inline bool loadFile(const std::string &path, std::string &dest)
{
    // Open file for reading
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }

    // Allocate space for file contents
    file.seekg(0, std::ios::end);
    const std::streamoff offset = file.tellg();
    if (offset < 0 || offset > std::numeric_limits<unsigned int>::max()) {
        return false;
    }

    dest.clear();
    dest.reserve(static_cast<unsigned int>(offset));

    // Assign file contents to destination string
    file.seekg(0, std::ios::beg);
    dest.assign(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());

    return true;
}

}  // namespace utils
}  // namespace valijson
#pragma once

#include <assert.h>
#include <stdexcept>
#include <string>


/*
  Basic UTF-8 manipulation routines, adapted from code that was released into
  the public domain by Jeff Bezanson.
*/

namespace valijson {
namespace utils {

static const uint32_t offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

/* is c the start of a utf8 sequence? */
inline bool isutf(char c) {
    return ((c & 0xC0) != 0x80);
}

/* reads the next utf-8 sequence out of a string, updating an index */
inline uint64_t u8_nextchar(const char *s, uint64_t *i)
{
    uint64_t ch = 0;
    int sz = 0;

    do {
        ch <<= 6;
        ch += static_cast<unsigned char>(s[(*i)++]);
        sz++;
    } while (s[*i] && !isutf(s[*i]));
    ch -= offsetsFromUTF8[sz-1];

    return ch;
}

/* number of characters */
inline uint64_t u8_strlen(const char *s)
{
    constexpr auto maxLength = std::numeric_limits<uint64_t>::max();
    uint64_t count = 0;
    uint64_t i = 0;

    while (s[i] != 0 && u8_nextchar(s, &i) != 0) {
        if (i == maxLength) {
            throwRuntimeError(
                    "String exceeded maximum size of " +
                    std::to_string(maxLength) + " bytes.");
        }
        count++;
    }

    return count;
}

}  // namespace utils
}  // namespace valijson
#pragma once

#include <memory>
#include <type_traits>

namespace valijson {
namespace constraints {

class ConstraintVisitor;

/**
 * @brief  Interface that must be implemented by concrete constraint types.
 *
 * @todo  Consider using something like the boost::cloneable concept here.
 */
struct Constraint
{
    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    /// Deleter type to be used with std::unique_ptr / std::shared_ptr
    /// @tparam  T  Const or non-const type (same as the one used in unique_ptr/shared_ptr)
    template<typename T>
    struct CustomDeleter
    {
        CustomDeleter(CustomFree freeFn)
          : m_freeFn(freeFn) { }

        void operator()(T *ptr) const
        {
            auto *nonconst = const_cast<typename std::remove_const<T>::type *>(ptr);
            nonconst->~T();
            m_freeFn(nonconst);
        }

    private:
        CustomFree m_freeFn;
    };

    /// Exclusive-ownership pointer to automatically handle deallocation
    typedef std::unique_ptr<const Constraint, CustomDeleter<const Constraint>> OwningPointer;

    /**
     * @brief  Virtual destructor.
     */
    virtual ~Constraint() = default;

    /**
     * @brief  Perform an action on the constraint using the visitor pattern.
     *
     * Note that Constraints cannot be modified by visitors.
     *
     * @param  visitor  Reference to a ConstraintVisitor object.
     *
     * @returns  the boolean value returned by one of the visitor's visit
     *           functions.
     */
    virtual bool accept(ConstraintVisitor &visitor) const = 0;

    /**
     * @brief  Make a copy of a constraint.
     *
     * Note that this should be a deep copy of the constraint.
     *
     * @returns  an owning-pointer to the new constraint.
     */
    virtual OwningPointer clone(CustomAlloc, CustomFree) const = 0;

};

} // namespace constraints
} // namespace valijson
#pragma once

#include <functional>
#include <memory>
#include <vector>


namespace valijson {

/**
 * Represents a sub-schema within a JSON Schema
 *
 * While all JSON Schemas have at least one sub-schema, the root, some will
 * have additional sub-schemas that are defined as part of constraints that are
 * included in the schema. For example, a 'oneOf' constraint maintains a set of
 * references to one or more nested sub-schemas. As per the definition of a
 * oneOf constraint, a document is valid within that constraint if it validates
 * against one of the nested sub-schemas.
 */
class Subschema
{
public:

    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    /// Typedef the Constraint class into the local namespace for convenience
    typedef constraints::Constraint Constraint;

    /// Typedef for a function that can be applied to each of the Constraint
    /// instances owned by a Schema.
    typedef std::function<bool (const Constraint &)> ApplyFunction;

    // Disable copy construction
    Subschema(const Subschema &) = delete;

    // Disable copy assignment
    Subschema & operator=(const Subschema &) = delete;

    /**
     * @brief  Construct a new Subschema object
     */
    Subschema()
      : m_allocFn(::operator new)
      , m_freeFn(::operator delete)
      , m_alwaysInvalid(false) { }

    /**
     * @brief  Construct a new Subschema using custom memory management
     *         functions
     *
     * @param  allocFn  malloc- or new-like function to allocate memory
     *                  within Schema, such as for Subschema instances
     * @param  freeFn   free-like function to free memory allocated with
     *                  the `customAlloc` function
     */
    Subschema(CustomAlloc allocFn, CustomFree freeFn)
      : m_allocFn(allocFn)
      , m_freeFn(freeFn)
      , m_alwaysInvalid(false)
    {
        // explicitly initialise optionals. See: https://github.com/tristanpenman/valijson/issues/124
        m_description = opt::nullopt;
        m_id = opt::nullopt;
        m_title = opt::nullopt;
    }

    /**
     * @brief  Clean up and free all memory managed by the Subschema
     */
    virtual ~Subschema()
    {
#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            m_constraints.clear();
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::exception &e) {
            fprintf(stderr, "Caught an exception in Subschema destructor: %s",
                    e.what());
        }
#endif
    }

    /**
     * @brief  Add a constraint to this sub-schema
     *
     * The constraint will be copied before being added to the list of
     * constraints for this Subschema. Note that constraints will be copied
     * only as deep as references to other Subschemas - e.g. copies of
     * constraints that refer to sub-schemas, will continue to refer to the
     * same Subschema instances.
     *
     * @param  constraint  Reference to the constraint to copy
     */
    void addConstraint(const Constraint &constraint)
    {
        // the vector allocation might throw but the constraint memory will be taken care of anyways
        m_constraints.push_back(constraint.clone(m_allocFn, m_freeFn));
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This function will apply the callback function to each constraint in
     * the Subschema, even if one of the invocations returns \c false. However,
     * if one or more invocations of the callback function return \c false,
     * this function will also return \c false.
     *
     * @returns  \c true if all invocations of the callback function are
     *           successful, \c false otherwise
     */
    bool apply(ApplyFunction &applyFunction) const
    {
        bool allTrue = true;
        for (auto &&constraint : m_constraints) {
            allTrue = applyFunction(*constraint) && allTrue;
        }

        return allTrue;
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This is a stricter version of the apply() function that will return
     * immediately if any of the invocations of the callback function return
     * \c false.
     *
     * @returns  \c true if all invocations of the callback function are
     *           successful, \c false otherwise
     */
    bool applyStrict(ApplyFunction &applyFunction) const
    {
        for (auto &&constraint : m_constraints) {
            if (!applyFunction(*constraint)) {
                return false;
            }
        }

        return true;
    }

    bool getAlwaysInvalid() const
    {
        return m_alwaysInvalid;
    }

    /**
     * @brief  Get the description associated with this sub-schema
     *
     * @throws  std::runtime_error if a description has not been set
     *
     * @returns  string containing sub-schema description
     */
    std::string getDescription() const
    {
        if (m_description) {
            return *m_description;
        }

        throwRuntimeError("Schema does not have a description");
    }

    /**
     * @brief  Get the ID associated with this sub-schema
     *
     * @throws  std::runtime_error if an ID has not been set
     *
     * @returns  string containing sub-schema ID
     */
    std::string getId() const
    {
        if (m_id) {
            return *m_id;
        }

        throwRuntimeError("Schema does not have an ID");
    }

    /**
     * @brief  Get the title associated with this sub-schema
     *
     * @throws  std::runtime_error if a title has not been set
     *
     * @returns  string containing sub-schema title
     */
    std::string getTitle() const
    {
        if (m_title) {
            return *m_title;
        }

        throwRuntimeError("Schema does not have a title");
    }

    /**
     * @brief  Check whether this sub-schema has a description
     *
     * @return boolean value
     */
    bool hasDescription() const
    {
        return static_cast<bool>(m_description);
    }

    /**
     * @brief  Check whether this sub-schema has an ID
     *
     * @return  boolean value
     */
    bool hasId() const
    {
        return static_cast<bool>(m_id);
    }

    /**
     * @brief  Check whether this sub-schema has a title
     *
     * @return  boolean value
     */
    bool hasTitle() const
    {
        return static_cast<bool>(m_title);
    }

    void setAlwaysInvalid(bool value)
    {
        m_alwaysInvalid = value;
    }

    /**
     * @brief  Set the description for this sub-schema
     *
     * The description will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schemas. As
     * an example, it may be used as part of the validation error descriptions
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  description  new description
     */
    void setDescription(const std::string &description)
    {
        m_description = description;
    }

    void setId(const std::string &id)
    {
        m_id = id;
    }

    /**
     * @brief  Set the title for this sub-schema
     *
     * The title will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schema. As an
     * example, it may be used as part of the validation error descriptions
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  title  new title
     */
    void setTitle(const std::string &title)
    {
        m_title = title;
    }

protected:

    CustomAlloc m_allocFn;

    CustomFree m_freeFn;

private:

    bool m_alwaysInvalid;

    /// List of pointers to constraints that apply to this schema.
    std::vector<Constraint::OwningPointer> m_constraints;

    /// Schema description (optional)
    opt::optional<std::string> m_description;

    /// Id to apply when resolving the schema URI
    opt::optional<std::string> m_id;

    /// Title string associated with the schema (optional)
    opt::optional<std::string> m_title;
};

} // namespace valijson
#pragma once

#include <cstdio>
#include <set>


namespace valijson {

/**
 * Represents the root of a JSON Schema
 *
 * The root is distinct from other sub-schemas because it is the canonical
 * starting point for validation of a document against a given a JSON Schema.
 */
class Schema: public Subschema
{
public:
    /**
     * @brief  Construct a new Schema instance with no constraints
     */
    Schema()
      : sharedEmptySubschema(newSubschema()) { }

    /**
     * @brief  Construct a new Schema using custom memory management
     *         functions
     *
     * @param  allocFn  malloc- or new-like function to allocate memory
     *                  within Schema, such as for Subschema instances
     * @param  freeFn   free-like function to free memory allocated with
     *                  the `customAlloc` function
     */
    Schema(CustomAlloc allocFn, CustomFree freeFn)
      : Subschema(allocFn, freeFn),
        sharedEmptySubschema(newSubschema()) { }

    // Disable copy construction
    Schema(const Schema &) = delete;

    // Disable copy assignment
    Schema & operator=(const Schema &) = delete;

    /**
     * @brief  Clean up and free all memory managed by the Schema
     *
     * Note that any Subschema pointers created and returned by this Schema
     * should be considered invalid.
     */
    ~Schema() override
    {
        sharedEmptySubschema->~Subschema();
        m_freeFn(const_cast<Subschema *>(sharedEmptySubschema));
        sharedEmptySubschema = nullptr;

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            for (auto subschema : subschemaSet) {
                subschema->~Subschema();
                m_freeFn(subschema);
            }
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::exception &e) {
            fprintf(stderr, "Caught an exception while destroying Schema: %s",
                    e.what());
        }
#endif
    }

    /**
     * @brief  Copy a constraint to a specific sub-schema
     *
     * @param  constraint  reference to a constraint that will be copied into
     *                     the sub-schema
     * @param  subschema   pointer to the sub-schema that will own the copied
     *                     constraint
     *
     * @throws std::runtime_error if the sub-schema is not owned by this Schema
     *         instance
     */
    void addConstraintToSubschema(const Constraint &constraint,
            const Subschema *subschema)
    {
        // TODO: Check heirarchy for subschemas that do not belong...

        mutableSubschema(subschema)->addConstraint(constraint);
    }

    /**
     * @brief  Create a new Subschema instance that is owned by this Schema
     *
     * @returns  const pointer to the new Subschema instance
     */
    const Subschema * createSubschema()
    {
        Subschema *subschema = newSubschema();

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            if (!subschemaSet.insert(subschema).second) {
                throwRuntimeError(
                        "Failed to store pointer for new sub-schema");
            }
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            subschema->~Subschema();
            m_freeFn(subschema);
            throw;
        }
#endif
        return subschema;
    }

    /**
     * @brief  Return a pointer to the shared empty schema
     */
    const Subschema * emptySubschema() const
    {
        return sharedEmptySubschema;
    }

    /**
     * @brief  Get a pointer to the root sub-schema of this Schema instance
     */
    const Subschema * root() const
    {
        return this;
    }

    void setAlwaysInvalid(const Subschema *subschema, bool value)
    {
        mutableSubschema(subschema)->setAlwaysInvalid(value);
    }

    /**
     * @brief  Update the description for one of the sub-schemas owned by this
     *         Schema instance
     *
     * @param  subschema    sub-schema to update
     * @param  description  new description
     */
    void setSubschemaDescription(const Subschema *subschema,
            const std::string &description)
    {
        mutableSubschema(subschema)->setDescription(description);
    }

    /**
     * @brief  Update the ID for one of the sub-schemas owned by this Schema
     *         instance
     *
     * @param  subschema  sub-schema to update
     * @param  id         new ID
     */
    void setSubschemaId(const Subschema *subschema, const std::string &id)
    {
        mutableSubschema(subschema)->setId(id);
    }

    /**
     * @brief  Update the title for one of the sub-schemas owned by this Schema
     *         instance
     *
     * @param  subschema  sub-schema to update
     * @param  title      new title
     */
    void setSubschemaTitle(const Subschema *subschema, const std::string &title)
    {
        mutableSubschema(subschema)->setTitle(title);
    }

private:

    Subschema *newSubschema()
    {
        void *ptr = m_allocFn(sizeof(Subschema));
        if (!ptr) {
            throwRuntimeError(
                    "Failed to allocate memory for shared empty sub-schema");
        }

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            return new (ptr) Subschema();
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            m_freeFn(ptr);
            throw;
        }
#endif
    }

    Subschema * mutableSubschema(const Subschema *subschema)
    {
        if (subschema == this) {
            return this;
        }

        if (subschema == sharedEmptySubschema) {
            throwRuntimeError(
                    "Cannot modify the shared empty sub-schema");
        }

        auto *noConst = const_cast<Subschema*>(subschema);
        if (subschemaSet.find(noConst) == subschemaSet.end()) {
            throwRuntimeError(
                    "Subschema pointer is not owned by this Schema instance");
        }

        return noConst;
    }

    /// Set of Subschema instances owned by this schema
    std::set<Subschema*> subschemaSet;

    /// Empty schema that can be reused by multiple constraints
    const Subschema *sharedEmptySubschema;
};

} // namespace valijson
#pragma once

namespace valijson {
namespace constraints {

class AllOfConstraint;
class AnyOfConstraint;
class ConditionalConstraint;
class ConstConstraint;
class ContainsConstraint;
class DependenciesConstraint;
class EnumConstraint;
class LinearItemsConstraint;
class MaxItemsConstraint;
class MaximumConstraint;
class MaxLengthConstraint;
class MaxPropertiesConstraint;
class MinItemsConstraint;
class MinimumConstraint;
class MinLengthConstraint;
class MinPropertiesConstraint;
class MultipleOfDoubleConstraint;
class MultipleOfIntConstraint;
class NotConstraint;
class OneOfConstraint;
class PatternConstraint;
class PolyConstraint;
class PropertiesConstraint;
class PropertyNamesConstraint;
class RequiredConstraint;
class SingularItemsConstraint;
class TypeConstraint;
class UniqueItemsConstraint;

/// Interface to allow usage of the visitor pattern with Constraints
class ConstraintVisitor
{
protected:
    virtual ~ConstraintVisitor() = default;

    // Shorten type names for derived classes outside of this namespace
    typedef constraints::AllOfConstraint AllOfConstraint;
    typedef constraints::AnyOfConstraint AnyOfConstraint;
    typedef constraints::ConditionalConstraint ConditionalConstraint;
    typedef constraints::ConstConstraint ConstConstraint;
    typedef constraints::ContainsConstraint ContainsConstraint;
    typedef constraints::DependenciesConstraint DependenciesConstraint;
    typedef constraints::EnumConstraint EnumConstraint;
    typedef constraints::LinearItemsConstraint LinearItemsConstraint;
    typedef constraints::MaximumConstraint MaximumConstraint;
    typedef constraints::MaxItemsConstraint MaxItemsConstraint;
    typedef constraints::MaxLengthConstraint MaxLengthConstraint;
    typedef constraints::MaxPropertiesConstraint MaxPropertiesConstraint;
    typedef constraints::MinimumConstraint MinimumConstraint;
    typedef constraints::MinItemsConstraint MinItemsConstraint;
    typedef constraints::MinLengthConstraint MinLengthConstraint;
    typedef constraints::MinPropertiesConstraint MinPropertiesConstraint;
    typedef constraints::MultipleOfDoubleConstraint MultipleOfDoubleConstraint;
    typedef constraints::MultipleOfIntConstraint MultipleOfIntConstraint;
    typedef constraints::NotConstraint NotConstraint;
    typedef constraints::OneOfConstraint OneOfConstraint;
    typedef constraints::PatternConstraint PatternConstraint;
    typedef constraints::PolyConstraint PolyConstraint;
    typedef constraints::PropertiesConstraint PropertiesConstraint;
    typedef constraints::PropertyNamesConstraint PropertyNamesConstraint;
    typedef constraints::RequiredConstraint RequiredConstraint;
    typedef constraints::SingularItemsConstraint SingularItemsConstraint;
    typedef constraints::TypeConstraint TypeConstraint;
    typedef constraints::UniqueItemsConstraint UniqueItemsConstraint;

public:

    virtual bool visit(const AllOfConstraint &) = 0;
    virtual bool visit(const AnyOfConstraint &) = 0;
    virtual bool visit(const ConditionalConstraint &) = 0;
    virtual bool visit(const ConstConstraint &) = 0;
    virtual bool visit(const ContainsConstraint &) = 0;
    virtual bool visit(const DependenciesConstraint &) = 0;
    virtual bool visit(const EnumConstraint &) = 0;
    virtual bool visit(const LinearItemsConstraint &) = 0;
    virtual bool visit(const MaximumConstraint &) = 0;
    virtual bool visit(const MaxItemsConstraint &) = 0;
    virtual bool visit(const MaxLengthConstraint &) = 0;
    virtual bool visit(const MaxPropertiesConstraint &) = 0;
    virtual bool visit(const MinimumConstraint &) = 0;
    virtual bool visit(const MinItemsConstraint &) = 0;
    virtual bool visit(const MinLengthConstraint &) = 0;
    virtual bool visit(const MinPropertiesConstraint &) = 0;
    virtual bool visit(const MultipleOfDoubleConstraint &) = 0;
    virtual bool visit(const MultipleOfIntConstraint &) = 0;
    virtual bool visit(const NotConstraint &) = 0;
    virtual bool visit(const OneOfConstraint &) = 0;
    virtual bool visit(const PatternConstraint &) = 0;
    virtual bool visit(const PolyConstraint &) = 0;
    virtual bool visit(const PropertiesConstraint &) = 0;
    virtual bool visit(const PropertyNamesConstraint &) = 0;
    virtual bool visit(const RequiredConstraint &) = 0;
    virtual bool visit(const SingularItemsConstraint &) = 0;
    virtual bool visit(const TypeConstraint &) = 0;
    virtual bool visit(const UniqueItemsConstraint &) = 0;
};

}  // namespace constraints
}  // namespace valijson
#pragma once


namespace valijson {
namespace constraints {

/**
 * @brief   Template class that implements the accept() and clone() functions of the Constraint interface.
 *
 * @tparam  ConstraintType   name of the concrete constraint type, which must provide a copy constructor.
 */
template<typename ConstraintType>
struct BasicConstraint: Constraint
{
    typedef internal::CustomAllocator<void *> Allocator;

    typedef std::basic_string<char, std::char_traits<char>, internal::CustomAllocator<char>> String;

    BasicConstraint()
      : m_allocator() { }

    BasicConstraint(Allocator::CustomAlloc allocFn, Allocator::CustomFree freeFn)
      : m_allocator(allocFn, freeFn) { }

    BasicConstraint(const BasicConstraint &other)
      : m_allocator(other.m_allocator) { }

    ~BasicConstraint() override = default;

    bool accept(ConstraintVisitor &visitor) const override
    {
        return visitor.visit(*static_cast<const ConstraintType*>(this));
    }

    OwningPointer clone(CustomAlloc allocFn, CustomFree freeFn) const override
    {
        // smart pointer to automatically free raw memory on exception
        typedef std::unique_ptr<Constraint, CustomFree> RawOwningPointer;
        auto ptr = RawOwningPointer(static_cast<Constraint*>(allocFn(sizeof(ConstraintType))), freeFn);
        if (!ptr) {
            throwRuntimeError("Failed to allocate memory for cloned constraint");
        }

        // constructor might throw but the memory will be taken care of anyways
        (void)new (ptr.get()) ConstraintType(*static_cast<const ConstraintType*>(this));

        // implicitly convert to smart pointer that will also destroy object instance
        return ptr;
    }

protected:

    Allocator m_allocator;
};

} // namespace constraints
} // namespace valijson
/**
 * @file
 *
 * @brief   Class definitions to support JSON Schema constraints
 *
 * This file contains class definitions for all of the constraints required to
 * support JSON Schema. These classes all inherit from the BasicConstraint
 * template class, which implements the common parts of the Constraint
 * interface.
 *
 * @see BasicConstraint
 * @see Constraint
 */

#pragma once

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>


#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

namespace valijson {

class ValidationResults;

namespace constraints {

/**
 * @brief  Represents an 'allOf' constraint.
 *
 * An allOf constraint provides a collection of sub-schemas that a value must
 * validate against. If a value fails to validate against any of these sub-
 * schemas, then validation fails.
 */
class AllOfConstraint: public BasicConstraint<AllOfConstraint>
{
public:
    AllOfConstraint()
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    AllOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, all of which must be satisfied
    Subschemas m_subschemas;
};

/**
 * @brief  Represents an 'anyOf' constraint
 *
 * An anyOf constraint provides a collection of sub-schemas that a value can
 * validate against. If a value validates against one of these sub-schemas,
 * then the validation passes.
 */
class AnyOfConstraint: public BasicConstraint<AnyOfConstraint>
{
public:
    AnyOfConstraint()
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    AnyOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, at least one of which must be satisfied
    Subschemas m_subschemas;
};

/**
 * @brief  Represents a combination 'if', 'then' and 'else' constraints
 *
 * The schema provided by an 'if' constraint is used as the expression for a conditional. When the
 * target validates against that schema, the 'then' subschema will be also be tested. Otherwise,
 * the 'else' subschema will be tested.
 */
class ConditionalConstraint: public BasicConstraint<ConditionalConstraint>
{
public:
    ConditionalConstraint()
      : m_ifSubschema(nullptr),
        m_thenSubschema(nullptr),
        m_elseSubschema(nullptr) { }

    ConditionalConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_ifSubschema(nullptr),
        m_thenSubschema(nullptr),
        m_elseSubschema(nullptr) { }

    const Subschema * getIfSubschema() const
    {
        return m_ifSubschema;
    }

    const Subschema * getThenSubschema() const
    {
        return m_thenSubschema;
    }

    const Subschema * getElseSubschema() const
    {
        return m_elseSubschema;
    }

    void setIfSubschema(const Subschema *subschema)
    {
        m_ifSubschema = subschema;
    }

    void setThenSubschema(const Subschema *subschema)
    {
        m_thenSubschema = subschema;
    }

    void setElseSubschema(const Subschema *subschema)
    {
        m_elseSubschema = subschema;
    }

private:
    const Subschema *m_ifSubschema;
    const Subschema *m_thenSubschema;
    const Subschema *m_elseSubschema;
};

class ConstConstraint: public BasicConstraint<ConstConstraint>
{
public:
    ConstConstraint()
      : m_value(nullptr) { }

    ConstConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(nullptr) { }

    ConstConstraint(const ConstConstraint &other)
      : BasicConstraint(other),
        m_value(other.m_value->clone()) { }

    adapters::FrozenValue * getValue() const
    {
        return m_value.get();
    }

    void setValue(const adapters::Adapter &value)
    {
        m_value = std::unique_ptr<adapters::FrozenValue>(value.freeze());
    }

private:
    std::unique_ptr<adapters::FrozenValue> m_value;
};

/**
 * @brief  Represents a 'contains' constraint
 *
 * A 'contains' constraint specifies a schema that must be satisfied by at least one
 * of the values in an array.
 */
class ContainsConstraint: public BasicConstraint<ContainsConstraint>
{
public:
    ContainsConstraint()
      : m_subschema(nullptr) { }

    ContainsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
};

/**
 * @brief  Represents a 'dependencies' constraint.
 *
 * A dependency constraint ensures that a given property is valid only if the
 * properties that it depends on are present.
 */
class DependenciesConstraint: public BasicConstraint<DependenciesConstraint>
{
public:
    DependenciesConstraint()
      : m_propertyDependencies(std::less<String>(), m_allocator),
        m_schemaDependencies(std::less<String>(), m_allocator)
    { }

    DependenciesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_propertyDependencies(std::less<String>(), m_allocator),
        m_schemaDependencies(std::less<String>(), m_allocator)
    { }

    template<typename StringType>
    DependenciesConstraint & addPropertyDependency(
            const StringType &propertyName,
            const StringType &dependencyName)
    {
        const String key(propertyName.c_str(), m_allocator);
        auto itr = m_propertyDependencies.find(key);
        if (itr == m_propertyDependencies.end()) {
            itr = m_propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), m_allocator))).first;
        }

        itr->second.insert(String(dependencyName.c_str(), m_allocator));

        return *this;
    }

    template<typename StringType, typename ContainerType>
    DependenciesConstraint & addPropertyDependencies(
            const StringType &propertyName,
            const ContainerType &dependencyNames)
    {
        const String key(propertyName.c_str(), m_allocator);
        auto itr = m_propertyDependencies.find(key);
        if (itr == m_propertyDependencies.end()) {
            itr = m_propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), m_allocator))).first;
        }

        typedef typename ContainerType::value_type ValueType;
        for (const ValueType &dependencyName : dependencyNames) {
            itr->second.insert(String(dependencyName.c_str(), m_allocator));
        }

        return *this;
    }

    template<typename StringType>
    DependenciesConstraint & addSchemaDependency(const StringType &propertyName, const Subschema *schemaDependency)
    {
        if (m_schemaDependencies.insert(SchemaDependencies::value_type(
                String(propertyName.c_str(), m_allocator),
                schemaDependency)).second) {
            return *this;
        }

        throwRuntimeError("Dependencies constraint already contains a dependent "
                "schema for the property '" + propertyName + "'");
    }

    template<typename FunctorType>
    void applyToPropertyDependencies(const FunctorType &fn) const
    {
        for (const PropertyDependencies::value_type &v : m_propertyDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaDependencies(const FunctorType &fn) const
    {
        for (const SchemaDependencies::value_type &v : m_schemaDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>, internal::CustomAllocator<String>> PropertySet;

    typedef std::map<String, PropertySet, std::less<String>,
            internal::CustomAllocator<std::pair<const String, PropertySet>>> PropertyDependencies;

    typedef std::map<String, const Subschema *, std::less<String>,
            internal::CustomAllocator<std::pair<const String, const Subschema *>>> SchemaDependencies;

    /// Mapping from property names to their property-based dependencies
    PropertyDependencies m_propertyDependencies;

    /// Mapping from property names to their schema-based dependencies
    SchemaDependencies m_schemaDependencies;
};

/**
 * @brief  Represents an 'enum' constraint
 *
 * An enum constraint provides a collection of permissible values for a JSON
 * node. The node will only validate against this constraint if it matches one
 * or more of the values in the collection.
 */
class EnumConstraint: public BasicConstraint<EnumConstraint>
{
public:
    EnumConstraint()
      : m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator)) { }

    EnumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator)) { }

    EnumConstraint(const EnumConstraint &other)
      : BasicConstraint(other),
        m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator))
    {
#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            // Clone individual enum values
            for (const EnumValue *otherValue : other.m_enumValues) {
                const EnumValue *value = otherValue->clone();
#if VALIJSON_USE_EXCEPTIONS
                try {
#endif
                    m_enumValues.push_back(value);
#if VALIJSON_USE_EXCEPTIONS
                } catch (...) {
                    delete value;
                    value = nullptr;
                    throw;
                }
            }
        } catch (...) {
            // Delete values already added to constraint
            for (const EnumValue *value : m_enumValues) {
                delete value;
            }
            throw;
#endif
        }
    }

    ~EnumConstraint() override
    {
        for (const EnumValue *value : m_enumValues) {
            delete value;
        }
    }

    void addValue(const adapters::Adapter &value)
    {
        // TODO: Freeze value using custom alloc/free functions
        m_enumValues.push_back(value.freeze());
    }

    void addValue(const adapters::FrozenValue &value)
    {
        // TODO: Clone using custom alloc/free functions
        m_enumValues.push_back(value.clone());
    }

    template<typename FunctorType>
    void applyToValues(const FunctorType &fn) const
    {
        for (const EnumValue *value : m_enumValues) {
            if (!fn(*value)) {
                return;
            }
        }
    }

private:
    typedef adapters::FrozenValue EnumValue;

    typedef std::vector<const EnumValue *, internal::CustomAllocator<const EnumValue *>> EnumValues;

    EnumValues m_enumValues;
};

/**
 * @brief  Represents non-singular 'items' and 'additionalItems' constraints
 *
 * Unlike the SingularItemsConstraint class, this class represents an 'items'
 * constraint that specifies an array of sub-schemas, which should be used to
 * validate each item in an array, in sequence. It also represents an optional
 * 'additionalItems' sub-schema that should be used when an array contains
 * more values than there are sub-schemas in the 'items' constraint.
 *
 * The prefix 'Linear' comes from the fact that this class contains a list of
 * sub-schemas that corresponding array items must be validated against, and
 * this validation is performed linearly (i.e. in sequence).
 */
class LinearItemsConstraint: public BasicConstraint<LinearItemsConstraint>
{
public:
    LinearItemsConstraint()
      : m_itemSubschemas(Allocator::rebind<const Subschema *>::other(m_allocator)),
        m_additionalItemsSubschema(nullptr) { }

    LinearItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_itemSubschemas(Allocator::rebind<const Subschema *>::other(m_allocator)),
        m_additionalItemsSubschema(nullptr) { }

    void addItemSubschema(const Subschema *subschema)
    {
        m_itemSubschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToItemSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_itemSubschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    const Subschema * getAdditionalItemsSubschema() const
    {
        return m_additionalItemsSubschema;
    }

    size_t getItemSubschemaCount() const
    {
        return m_itemSubschemas.size();
    }

    void setAdditionalItemsSubschema(const Subschema *subschema)
    {
        m_additionalItemsSubschema = subschema;
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    Subschemas m_itemSubschemas;

    const Subschema* m_additionalItemsSubschema;
};

/**
 * @brief   Represents 'maximum' and 'exclusiveMaximum' constraints
 */
class MaximumConstraint: public BasicConstraint<MaximumConstraint>
{
public:
    MaximumConstraint()
      : m_maximum(std::numeric_limits<double>::infinity()),
        m_exclusiveMaximum(false) { }

    MaximumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maximum(std::numeric_limits<double>::infinity()),
        m_exclusiveMaximum(false) { }

    bool getExclusiveMaximum() const
    {
        return m_exclusiveMaximum;
    }

    void setExclusiveMaximum(bool newExclusiveMaximum)
    {
        m_exclusiveMaximum = newExclusiveMaximum;
    }

    double getMaximum() const
    {
        return m_maximum;
    }

    void setMaximum(double newMaximum)
    {
        m_maximum = newMaximum;
    }

private:
    double m_maximum;
    bool m_exclusiveMaximum;
};

/**
 * @brief   Represents a 'maxItems' constraint
 */
class MaxItemsConstraint: public BasicConstraint<MaxItemsConstraint>
{
public:
    MaxItemsConstraint()
      : m_maxItems(std::numeric_limits<uint64_t>::max()) { }

    MaxItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxItems(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxItems() const
    {
        return m_maxItems;
    }

    void setMaxItems(uint64_t newMaxItems)
    {
        m_maxItems = newMaxItems;
    }

private:
    uint64_t m_maxItems;
};

/**
 * @brief   Represents a 'maxLength' constraint
 */
class MaxLengthConstraint: public BasicConstraint<MaxLengthConstraint>
{
public:
    MaxLengthConstraint()
      : m_maxLength(std::numeric_limits<uint64_t>::max()) { }

    MaxLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxLength(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxLength() const
    {
        return m_maxLength;
    }

    void setMaxLength(uint64_t newMaxLength)
    {
        m_maxLength = newMaxLength;
    }

private:
    uint64_t m_maxLength;
};

/**
 * @brief   Represents a 'maxProperties' constraint
 */
class MaxPropertiesConstraint: public BasicConstraint<MaxPropertiesConstraint>
{
public:
    MaxPropertiesConstraint()
      : m_maxProperties(std::numeric_limits<uint64_t>::max()) { }

    MaxPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxProperties(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxProperties() const
    {
        return m_maxProperties;
    }

    void setMaxProperties(uint64_t newMaxProperties)
    {
        m_maxProperties = newMaxProperties;
    }

private:
    uint64_t m_maxProperties;
};

/**
 * @brief   Represents 'minimum' and 'exclusiveMinimum' constraints
 */
class MinimumConstraint: public BasicConstraint<MinimumConstraint>
{
public:
    MinimumConstraint()
      : m_minimum(-std::numeric_limits<double>::infinity()),
        m_exclusiveMinimum(false) { }

    MinimumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minimum(-std::numeric_limits<double>::infinity()),
        m_exclusiveMinimum(false) { }

    bool getExclusiveMinimum() const
    {
        return m_exclusiveMinimum;
    }

    void setExclusiveMinimum(bool newExclusiveMinimum)
    {
        m_exclusiveMinimum = newExclusiveMinimum;
    }

    double getMinimum() const
    {
        return m_minimum;
    }

    void setMinimum(double newMinimum)
    {
        m_minimum = newMinimum;
    }

private:
    double m_minimum;
    bool m_exclusiveMinimum;
};

/**
 * @brief   Represents a 'minItems' constraint
 */
class MinItemsConstraint: public BasicConstraint<MinItemsConstraint>
{
public:
    MinItemsConstraint()
      : m_minItems(0) { }

    MinItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minItems(0) { }

    uint64_t getMinItems() const
    {
        return m_minItems;
    }

    void setMinItems(uint64_t newMinItems)
    {
        m_minItems = newMinItems;
    }

private:
    uint64_t m_minItems;
};

/**
 * @brief   Represents a 'minLength' constraint
 */
class MinLengthConstraint: public BasicConstraint<MinLengthConstraint>
{
public:
    MinLengthConstraint()
      : m_minLength(0) { }

    MinLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minLength(0) { }

    uint64_t getMinLength() const
    {
        return m_minLength;
    }

    void setMinLength(uint64_t newMinLength)
    {
        m_minLength = newMinLength;
    }

private:
    uint64_t m_minLength;
};

/**
 * @brief   Represents a 'minProperties' constraint
 */
class MinPropertiesConstraint: public BasicConstraint<MinPropertiesConstraint>
{
public:
    MinPropertiesConstraint()
      : m_minProperties(0) { }

    MinPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minProperties(0) { }

    uint64_t getMinProperties() const
    {
        return m_minProperties;
    }

    void setMinProperties(uint64_t newMinProperties)
    {
        m_minProperties = newMinProperties;
    }

private:
    uint64_t m_minProperties;
};

/**
 * @brief  Represents either 'multipleOf' or 'divisibleBy' constraints where
 *         the divisor is a floating point number
 */
class MultipleOfDoubleConstraint:
        public BasicConstraint<MultipleOfDoubleConstraint>
{
public:
    MultipleOfDoubleConstraint()
      : m_value(1.) { }

    MultipleOfDoubleConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(1.) { }

    double getDivisor() const
    {
        return m_value;
    }

    void setDivisor(double newValue)
    {
        m_value = newValue;
    }

private:
    double m_value;
};

/**
 * @brief  Represents either 'multipleOf' or 'divisibleBy' constraints where
 *         the divisor is of integer type
 */
class MultipleOfIntConstraint:
        public BasicConstraint<MultipleOfIntConstraint>
{
public:
    MultipleOfIntConstraint()
      : m_value(1) { }

    MultipleOfIntConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(1) { }

    int64_t getDivisor() const
    {
        return m_value;
    }

    void setDivisor(int64_t newValue)
    {
        m_value = newValue;
    }

private:
    int64_t m_value;
};

/**
 * @brief   Represents a 'not' constraint
 */
class NotConstraint: public BasicConstraint<NotConstraint>
{
public:
    NotConstraint()
      : m_subschema(nullptr) { }

    NotConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
};

/**
 * @brief   Represents a 'oneOf' constraint.
 */
class OneOfConstraint: public BasicConstraint<OneOfConstraint>
{
public:
    OneOfConstraint()
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    OneOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, exactly one of which must be satisfied
    Subschemas m_subschemas;
};

/**
 * @brief   Represents a 'pattern' constraint
 */
class PatternConstraint: public BasicConstraint<PatternConstraint>
{
public:
    PatternConstraint()
      : m_pattern(Allocator::rebind<char>::other(m_allocator)) { }

    PatternConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_pattern(Allocator::rebind<char>::other(m_allocator)) { }

    template<typename AllocatorType>
    bool getPattern(std::basic_string<char, std::char_traits<char>, AllocatorType> &result) const
    {
        result.assign(m_pattern.c_str());
        return true;
    }

    template<typename AllocatorType>
    std::basic_string<char, std::char_traits<char>, AllocatorType> getPattern(
            const AllocatorType &alloc = AllocatorType()) const
    {
        return std::basic_string<char, std::char_traits<char>, AllocatorType>(m_pattern.c_str(), alloc);
    }

    template<typename AllocatorType>
    void setPattern(const std::basic_string<char, std::char_traits<char>, AllocatorType> &pattern)
    {
        m_pattern.assign(pattern.c_str());
    }

private:
    String m_pattern;
};

class PolyConstraint : public Constraint
{
public:
    bool accept(ConstraintVisitor &visitor) const override
    {
        return visitor.visit(*static_cast<const PolyConstraint*>(this));
    }

    OwningPointer clone(CustomAlloc allocFn, CustomFree freeFn) const override
    {
        // smart pointer to automatically free raw memory on exception
        typedef std::unique_ptr<Constraint, CustomFree> RawOwningPointer;
        auto ptr = RawOwningPointer(static_cast<Constraint*>(allocFn(sizeOf())), freeFn);
        if (!ptr) {
            throwRuntimeError("Failed to allocate memory for cloned constraint");
        }

        // constructor might throw but the memory will be taken care of anyways
        (void)cloneInto(ptr.get());

        // implicitly convert to smart pointer that will also destroy object instance
        return ptr;
    }

    virtual bool validate(const adapters::Adapter &target,
            const std::vector<std::string>& context,
            valijson::ValidationResults *results) const = 0;

private:
    virtual Constraint * cloneInto(void *) const = 0;

    virtual size_t sizeOf() const = 0;
};

/**
 * @brief   Represents a combination of 'properties', 'patternProperties' and
 *          'additionalProperties' constraints
 */
class PropertiesConstraint: public BasicConstraint<PropertiesConstraint>
{
public:
    PropertiesConstraint()
      : m_properties(std::less<String>(), m_allocator),
        m_patternProperties(std::less<String>(), m_allocator),
        m_additionalProperties(nullptr) { }

    PropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_properties(std::less<String>(), m_allocator),
        m_patternProperties(std::less<String>(), m_allocator),
        m_additionalProperties(nullptr) { }

    bool addPatternPropertySubschema(const char *patternProperty, const Subschema *subschema)
    {
        return m_patternProperties.insert(PropertySchemaMap::value_type(
                String(patternProperty, m_allocator), subschema)).second;
    }

    template<typename AllocatorType>
    bool addPatternPropertySubschema(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &patternProperty,
            const Subschema *subschema)
    {
        return addPatternPropertySubschema(patternProperty.c_str(), subschema);
    }

    bool addPropertySubschema(const char *propertyName,
            const Subschema *subschema)
    {
        return m_properties.insert(PropertySchemaMap::value_type(
                String(propertyName, m_allocator), subschema)).second;
    }

    template<typename AllocatorType>
    bool addPropertySubschema(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &propertyName,
            const Subschema *subschema)
    {
        return addPropertySubschema(propertyName.c_str(), subschema);
    }

    template<typename FunctorType>
    void applyToPatternProperties(const FunctorType &fn) const
    {
        typedef typename PropertySchemaMap::value_type ValueType;
        for (const ValueType &value : m_patternProperties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToProperties(const FunctorType &fn) const
    {
        typedef typename PropertySchemaMap::value_type ValueType;
        for (const ValueType &value : m_properties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    const Subschema * getAdditionalPropertiesSubschema() const
    {
        return m_additionalProperties;
    }

    void setAdditionalPropertiesSubschema(const Subschema *subschema)
    {
        m_additionalProperties = subschema;
    }

private:
    typedef std::map<
            String,
            const Subschema *,
            std::less<String>,
            internal::CustomAllocator<std::pair<const String, const Subschema *>>
        > PropertySchemaMap;

    PropertySchemaMap m_properties;
    PropertySchemaMap m_patternProperties;

    const Subschema *m_additionalProperties;
};

class PropertyNamesConstraint: public BasicConstraint<PropertyNamesConstraint>
{
public:
    PropertyNamesConstraint()
      : m_subschema(nullptr) { }

    PropertyNamesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
};

/**
 * @brief   Represents a 'required' constraint
 */
class RequiredConstraint: public BasicConstraint<RequiredConstraint>
{
public:
    RequiredConstraint()
      : m_requiredProperties(std::less<String>(), m_allocator) { }

    RequiredConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_requiredProperties(std::less<String>(), m_allocator) { }

    bool addRequiredProperty(const char *propertyName)
    {
        return m_requiredProperties.insert(String(propertyName,
                Allocator::rebind<char>::other(m_allocator))).second;
    }

    template<typename AllocatorType>
    bool addRequiredProperty(const std::basic_string<char, std::char_traits<char>, AllocatorType> &propertyName)
    {
        return addRequiredProperty(propertyName.c_str());
    }

    template<typename FunctorType>
    void applyToRequiredProperties(const FunctorType &fn) const
    {
        for (const String &propertyName : m_requiredProperties) {
            if (!fn(propertyName)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>,
            internal::CustomAllocator<String>> RequiredProperties;

    RequiredProperties m_requiredProperties;
};

/**
 * @brief  Represents an 'items' constraint that specifies one sub-schema
 *
 * A value is considered valid against this constraint if it is an array, and
 * each item in the array validates against the sub-schema specified by this
 * constraint.
 *
 * The prefix 'Singular' comes from the fact that array items must validate
 * against exactly one sub-schema.
 */
class SingularItemsConstraint: public BasicConstraint<SingularItemsConstraint>
{
public:
    SingularItemsConstraint()
      : m_itemsSubschema(nullptr) { }

    SingularItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_itemsSubschema(nullptr) { }

    const Subschema * getItemsSubschema() const
    {
        return m_itemsSubschema;
    }

    void setItemsSubschema(const Subschema *subschema)
    {
        m_itemsSubschema = subschema;
    }

private:
    const Subschema *m_itemsSubschema;
};

/**
 * @brief   Represents a 'type' constraint.
 */
class TypeConstraint: public BasicConstraint<TypeConstraint>
{
public:
    enum JsonType {
        kAny,
        kArray,
        kBoolean,
        kInteger,
        kNull,
        kNumber,
        kObject,
        kString
    };

    TypeConstraint()
      : m_namedTypes(std::less<JsonType>(), m_allocator),
        m_schemaTypes(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    TypeConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_namedTypes(std::less<JsonType>(), m_allocator),
        m_schemaTypes(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addNamedType(JsonType type)
    {
        m_namedTypes.insert(type);
    }

    void addSchemaType(const Subschema *subschema)
    {
        m_schemaTypes.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToNamedTypes(const FunctorType &fn) const
    {
        for (const JsonType namedType : m_namedTypes) {
            if (!fn(namedType)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaTypes(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_schemaTypes) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    template<typename AllocatorType>
    static JsonType jsonTypeFromString(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &typeName)
    {
        if (typeName.compare("any") == 0) {
            return kAny;
        } else if (typeName.compare("array") == 0) {
            return kArray;
        } else if (typeName.compare("boolean") == 0) {
            return kBoolean;
        } else if (typeName.compare("integer") == 0) {
            return kInteger;
        } else if (typeName.compare("null") == 0) {
            return kNull;
        } else if (typeName.compare("number") == 0) {
            return kNumber;
        } else if (typeName.compare("object") == 0) {
            return kObject;
        } else if (typeName.compare("string") == 0) {
            return kString;
        }

        throwRuntimeError("Unrecognised JSON type name '" +
                std::string(typeName.c_str()) + "'");
        abort();
    }

private:
    typedef std::set<JsonType, std::less<JsonType>, internal::CustomAllocator<JsonType>> NamedTypes;

    typedef std::vector<const Subschema *,
            Allocator::rebind<const Subschema *>::other> SchemaTypes;

    /// Set of named JSON types that serve as valid types
    NamedTypes m_namedTypes;

    /// Set of sub-schemas that serve as valid types
    SchemaTypes m_schemaTypes;
};

/**
 * @brief   Represents a 'uniqueItems' constraint
 */
class UniqueItemsConstraint: public BasicConstraint<UniqueItemsConstraint>
{
public:
    UniqueItemsConstraint() = default;

    UniqueItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn) { }
};

} // namespace constraints
} // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif
#pragma once

namespace valijson {

namespace adapters {
    class Adapter;
}

namespace constraints {
    struct Constraint;
}

class ConstraintBuilder
{
public:
    virtual ~ConstraintBuilder() = default;

    virtual constraints::Constraint * make(const adapters::Adapter &) const = 0;
};

}  // namespace valijson
#pragma once

#include <stdexcept>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>


namespace valijson {

/**
 * @brief  Parser for populating a Schema based on a JSON Schema document.
 *
 * The SchemaParser class supports Drafts 3 and 4 of JSON Schema, however
 * Draft 3 support should be considered deprecated.
 *
 * The functions provided by this class have been templated so that they can
 * be used with different Adapter types.
 */
class SchemaParser
{
public:
    /// Supported versions of JSON Schema
    enum Version {
        kDraft3,      ///< @deprecated JSON Schema v3 has been superseded by v4
        kDraft4,
        kDraft7
    };

    /**
     * @brief  Construct a new SchemaParser for a given version of JSON Schema
     *
     * @param  version  Version of JSON Schema that will be expected
     */
    explicit SchemaParser(const Version version = kDraft7)
      : m_version(version) { }

    /**
     * @brief  Release memory associated with custom ConstraintBuilders
     */
    virtual ~SchemaParser()
    {
        for (const auto& entry : constraintBuilders) {
            delete entry.second;
        }
    }

    /**
     * @brief  Struct to contain templated function type for fetching documents
     */
    template<typename AdapterType>
    struct FunctionPtrs
    {
        typedef typename adapters::AdapterTraits<AdapterType>::DocumentType DocumentType;

        /// Templated function pointer type for fetching remote documents
        typedef std::function<const DocumentType* (const std::string &uri)> FetchDoc;

        /// Templated function pointer type for freeing fetched documents
        typedef std::function<void (const DocumentType *)> FreeDoc;
    };

    /**
     * @brief  Add a custom contraint to this SchemaParser

     * @param  key      name that will be used to identify relevant constraints
     *                  while parsing a schema document
     * @param  builder  pointer to a subclass of ConstraintBuilder that can
     *                  parse custom constraints found in a schema document,
     *                  and return an appropriate instance of Constraint; this
     *                  class guarantees that it will take ownership of this
     *                  pointer - unless this function throws an exception
     *
     * @todo   consider accepting a list of custom ConstraintBuilders in
     *         constructor, so that this class remains immutable after
     *         construction
     *
     * @todo   Add additional checks for key conflicts, empty keys, and
     *         potential restrictions relating to case sensitivity
     */
    void addConstraintBuilder(const std::string &key, const ConstraintBuilder *builder)
    {
        constraintBuilders.push_back(std::make_pair(key, builder));
    }

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSubschema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     */
    template<typename AdapterType>
    void populateSchema(
        const AdapterType &node,
        Schema &schema,
        typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc = nullptr ,
        typename FunctionPtrs<AdapterType>::FreeDoc freeDoc = nullptr )
    {
        if ((fetchDoc == nullptr ) ^ (freeDoc == nullptr)) {
            throwRuntimeError("Remote document fetching can't be enabled without both fetch and free functions");
        }

        typename DocumentCache<AdapterType>::Type docCache;
        SchemaCache schemaCache;
#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            resolveThenPopulateSchema(schema, node, node, schema, opt::optional<std::string>(), "", fetchDoc, nullptr,
                    nullptr, docCache, schemaCache);
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            freeDocumentCache<AdapterType>(docCache, freeDoc);
            throw;
        }
#endif

        freeDocumentCache<AdapterType>(docCache, freeDoc);
    }

private:

    typedef std::vector<std::pair<std::string, const ConstraintBuilder *>>
        ConstraintBuilders;

    ConstraintBuilders constraintBuilders;

    template<typename AdapterType>
    struct DocumentCache
    {
        typedef typename adapters::AdapterTraits<AdapterType>::DocumentType DocumentType;

        typedef std::map<std::string, const DocumentType*> Type;
    };

    typedef std::map<std::string, const Subschema *> SchemaCache;

    /**
     * @brief  Free memory used by fetched documents
     *
     * If a custom 'free' function has not been provided, then the default
     * delete operator will be used.
     *
     * @param  docCache  collection of fetched documents to free
     * @param  freeDoc   optional custom free function
     */
    template<typename AdapterType>
    void freeDocumentCache(const typename DocumentCache<AdapterType>::Type
            &docCache, typename FunctionPtrs<AdapterType>::FreeDoc freeDoc)
    {
        typedef typename DocumentCache<AdapterType>::Type DocCacheType;

        for (const typename DocCacheType::value_type &v : docCache) {
            freeDoc(v.second);
        }
    }

    /**
     * @brief  Find the complete URI for a document, within a resolution scope
     *
     * This function captures five different cases that can occur when
     * attempting to resolve a document URI within a particular resolution
     * scope:
     *
     *  (1) resolution scope not present, but URN or absolute document URI is
     *       => document URI as-is
     *  (2) resolution scope not present, and document URI is relative or absent
     *       => document URI, if present, otherwise no result
     *  (3) resolution scope is present, and document URI is a relative path
     *       => resolve document URI relative to resolution scope
     *  (4) resolution scope is present, and document URI is absolute
     *       => document URI as-is
     *  (5) resolution scope is present, but document URI is not
     *       => resolution scope as-is
     *
     * This function assumes that the resolution scope is absolute.
     *
     * When resolving a document URI relative to the resolution scope, the
     * document URI should be used to replace the path, query and fragment
     * portions of URI provided by the resolution scope.
     */
    virtual opt::optional<std::string> resolveDocumentUri(
            const opt::optional<std::string>& resolutionScope,
            const opt::optional<std::string>& documentUri)
    {
        if (resolutionScope) {
            if (documentUri) {
                if (internal::uri::isUriAbsolute(*documentUri) || internal::uri::isUrn(*documentUri)) {
                    // (4) resolution scope is present, and document URI is absolute
                    //      => document URI as-is
                    return *documentUri;
                } else {
                    // (3) resolution scope is present, and document URI is a relative path
                    //      => resolve document URI relative to resolution scope
                    return internal::uri::resolveRelativeUri(*resolutionScope, *documentUri);
                }
            } else {
                // (5) resolution scope is present, but document URI is not
                //      => resolution scope as-is
                return *resolutionScope;
            }
        } else if (documentUri && internal::uri::isUriAbsolute(*documentUri)) {
            // (1a) resolution scope not present, but absolute document URI is
            //      => document URI as-is
            return *documentUri;
        } else if (documentUri && internal::uri::isUrn(*documentUri)) {
            // (1b) resolution scope not present, but URN is
            //       => document URI as-is
            return *documentUri;
        } else {
            // (2) resolution scope not present, and document URI is relative or absent
            //      => document URI, if present, otherwise no result
            // documentUri is already std::optional
            return documentUri;
        }
    }

    /**
     * @brief  Extract a JSON Reference string from a node
     *
     * @param  node    node to extract the JSON Reference from
     * @param  result  reference to string to set with the result
     *
     * @throws std::invalid_argument if node is an object containing a `$ref`
     *         property but with a value that cannot be interpreted as a string
     *
     * @return \c true if a JSON Reference was extracted; \c false otherwise
     */
    template<typename AdapterType>
    bool extractJsonReference(const AdapterType &node, std::string &result)
    {
        if (!node.isObject()) {
            return false;
        }

        const typename AdapterType::Object o = node.getObject();
        const typename AdapterType::Object::const_iterator itr = o.find("$ref");
        if (itr == o.end()) {
            return false;
        } else if (!itr->second.getString(result)) {
            throwRuntimeError("$ref property expected to contain string value.");
        }

        return true;
    }

    /**
     * Sanitise an optional JSON Pointer, trimming trailing slashes
     */
    static std::string sanitiseJsonPointer(const opt::optional<std::string>& input)
    {
        if (input) {
            // Trim trailing slash(es)
            std::string sanitised = *input;
            sanitised.erase(sanitised.find_last_not_of('/') + 1,
                    std::string::npos);

            return sanitised;
        }

        // If the JSON Pointer is not set, assume that the URI points to
        // the root of the document
        return "";
    }

    /**
     * @brief  Search the schema cache for a schema matching a given key
     *
     * If the key is not present in the query cache, a nullptr will be
     * returned, and the contents of the cache will remain unchanged. This is
     * in contrast to the behaviour of the std::map [] operator, which would
     * add the nullptr to the cache.
     *
     * @param  schemaCache  schema cache to query
     * @param  queryKey     key to search for
     *
     * @return shared pointer to Schema if found, nullptr otherwise
     */
    static const Subschema * querySchemaCache(SchemaCache &schemaCache,
            const std::string &queryKey)
    {
        const SchemaCache::iterator itr = schemaCache.find(queryKey);
        if (itr == schemaCache.end()) {
            return nullptr;
        }

        return itr->second;
    }

    /**
     * @brief  Add entries to the schema cache for a given list of keys
     *
     * @param  schemaCache   schema cache to update
     * @param  keysToCreate  list of keys to create entries for
     * @param  schema        shared pointer to schema that keys will map to
     *
     * @throws std::logic_error if any of the keys are already present in the
     *         schema cache. This behaviour is intended to help detect incorrect
     *         usage of the schema cache during development, and is not expected
     *         to occur otherwise, even for malformed schemas.
     */
    static void updateSchemaCache(SchemaCache &schemaCache,
            const std::vector<std::string> &keysToCreate,
            const Subschema *schema)
    {
        for (const std::string &keyToCreate : keysToCreate) {
            const SchemaCache::value_type value(keyToCreate, schema);
            if (!schemaCache.insert(value).second) {
                throwLogicError("Key '" + keyToCreate + "' already in schema cache.");
            }
        }
    }

    /**
     * @brief  Recursive helper function for retrieving or creating schemas
     *
     * This function will be applied recursively until a concrete node is found.
     * A concrete node is a node that contains actual schema constraints rather
     * than a JSON Reference.
     *
     * This termination condition may be trigged by visiting the concrete node
     * at the end of a series of $ref nodes, or by finding a schema for one of
     * those $ref nodes in the schema cache. An entry will be added to the
     * schema cache for each node visited on the path to the concrete node.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and
     *                       modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to the node to parse
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     * @param  newCacheKeys  A list of keys that should be added to the cache
     *                       when recursion terminates
     */
    template<typename AdapterType>
    const Subschema * makeOrReuseSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache,
        std::vector<std::string> &newCacheKeys)
    {
        std::string jsonRef;

        // Check for the first termination condition (found a non-$ref node)
        if (!extractJsonReference(node, jsonRef)) {

            // Construct a key that we can use to search the schema cache for
            // a schema corresponding to the current node
            const std::string schemaCacheKey = currentScope ? (*currentScope + nodePath) : nodePath;

            // Retrieve an existing schema from the cache if possible
            const Subschema *cachedPtr = querySchemaCache(schemaCache, schemaCacheKey);

            // Create a new schema otherwise
            const Subschema *subschema = cachedPtr ? cachedPtr : rootSchema.createSubschema();

            // Add cache entries for keys belonging to any $ref nodes that were
            // visited before arriving at the current node
            updateSchemaCache(schemaCache, newCacheKeys, subschema);

            // Schema cache did not contain a pre-existing schema corresponding
            // to the current node, so the schema that was returned will need
            // to be populated
            if (!cachedPtr) {
                populateSchema(rootSchema, rootNode, node, *subschema,
                        currentScope, nodePath, fetchDoc, parentSubschema,
                        ownName, docCache, schemaCache);
            }

            return subschema;
        }

        // Returns a document URI if the reference points somewhere
        // other than the current document
        const opt::optional<std::string> documentUri = internal::json_reference::getJsonReferenceUri(jsonRef);

        // Extract JSON Pointer from JSON Reference, with any trailing
        // slashes removed so that keys in the schema cache end
        // consistently
        const std::string actualJsonPointer = sanitiseJsonPointer(
                internal::json_reference::getJsonReferencePointer(jsonRef));

        // Determine the actual document URI based on the resolution
        // scope. An absolute document URI will take precedence when
        // present, otherwise we need to resolve the URI relative to
        // the current resolution scope
        const opt::optional<std::string> actualDocumentUri = resolveDocumentUri(currentScope, documentUri);

        // Construct a key to search the schema cache for an existing schema
        const std::string queryKey = actualDocumentUri ? (*actualDocumentUri + actualJsonPointer) : actualJsonPointer;

        // Check for the second termination condition (found a $ref node that
        // already has an entry in the schema cache)
        const Subschema *cachedPtr = querySchemaCache(schemaCache, queryKey);
        if (cachedPtr) {
            updateSchemaCache(schemaCache, newCacheKeys, cachedPtr);
            return cachedPtr;
        }

        if (actualDocumentUri && (!currentScope || *actualDocumentUri != *currentScope)) {
            const typename FunctionPtrs<AdapterType>::DocumentType *newDoc = nullptr;

            // Have we seen this document before?
            typename DocumentCache<AdapterType>::Type::iterator docCacheItr =
                    docCache.find(*actualDocumentUri);
            if (docCacheItr == docCache.end()) {
                // Resolve reference against remote document
                if (!fetchDoc) {
                    throwRuntimeError("Fetching of remote JSON References not enabled.");
                }

                // Returns a pointer to the remote document that was
                // retrieved, or null if retrieval failed. This class
                // will take ownership of the pointer, and call freeDoc
                // when it is no longer needed.
                newDoc = fetchDoc(*actualDocumentUri);

                // Can't proceed without the remote document
                if (!newDoc) {
                    throwRuntimeError("Failed to fetch referenced schema document: " + *actualDocumentUri);
                }

                typedef typename DocumentCache<AdapterType>::Type::value_type
                        DocCacheValueType;

                docCache.insert(DocCacheValueType(*actualDocumentUri, newDoc));

            } else {
                newDoc = docCacheItr->second;
            }

            const AdapterType newRootNode(*newDoc);

            // Find where we need to be in the document
            const AdapterType &referencedAdapter =
                    internal::json_pointer::resolveJsonPointer(newRootNode,
                            actualJsonPointer);

            newCacheKeys.push_back(queryKey);

            // Populate the schema, starting from the referenced node, with
            // nested JSON References resolved relative to the new root node
            return makeOrReuseSchema(rootSchema, newRootNode, referencedAdapter,
                    currentScope, actualJsonPointer, fetchDoc, parentSubschema,
                    ownName, docCache, schemaCache, newCacheKeys);

        }

        // JSON References in nested schema will be resolved relative to the
        // current document
        const AdapterType &referencedAdapter =
                internal::json_pointer::resolveJsonPointer(
                        rootNode, actualJsonPointer);

        newCacheKeys.push_back(queryKey);

        // Populate the schema, starting from the referenced node, with
        // nested JSON References resolved relative to the new root node
        return makeOrReuseSchema(rootSchema, rootNode, referencedAdapter,
                currentScope, actualJsonPointer, fetchDoc, parentSubschema,
                ownName, docCache, schemaCache, newCacheKeys);
    }

    /**
     * @brief  Return pointer for the schema corresponding to a given node
     *
     * This function makes use of a schema cache, so that if the path to the
     * current node is the same as one that has already been parsed and
     * populated, a pointer to the existing Subschema will be returned.
     *
     * Should a series of $ref, or reference, nodes be resolved before reaching
     * a concrete node, an entry will be added to the schema cache for each of
     * the nodes in that path.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and
     *                       modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to the node to parse
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     */
    template<typename AdapterType>
    const Subschema * makeOrReuseSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        std::vector<std::string> schemaCacheKeysToCreate;

        return makeOrReuseSchema(rootSchema, rootNode, node, currentScope,
                nodePath, fetchDoc, parentSubschema, ownName, docCache,
                schemaCache, schemaCacheKeysToCreate);
    }

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSubschema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  rootSchema       The Schema instance, and root subschema, through
     *                          which other subschemas can be created and
     *                          modified
     * @param  rootNode         Reference to the node from which JSON References
     *                          will be resolved when they refer to the current
     *                          document
     * @param  node             Reference to node to parse
     * @param  schema           Reference to Schema to populate
     * @param  currentScope     URI for current resolution scope
     * @param  nodePath         JSON Pointer representing path to current node
     * @param  fetchDoc         Optional function to fetch remote JSON documents
     * @param  parentSubschema  Optional pointer to the parent schema, used to
     *                          support required keyword in Draft 3
     * @param  ownName          Optional pointer to a node name, used to support
     *                          the 'required' keyword in Draft 3
     * @param  docCache         Cache of resolved and fetched remote documents
     * @param  schemaCache      Cache of populated schemas
     */
    template<typename AdapterType>
    void populateSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const Subschema &subschema,
        const opt::optional<std::string>& currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        static_assert((std::is_convertible<AdapterType,
            const valijson::adapters::Adapter &>::value),
            "SchemaParser::populateSchema must be invoked with an "
            "appropriate Adapter implementation");

        if (!node.isObject()) {
            if (m_version == kDraft7 && node.maybeBool()) {
                // Boolean schema
                if (!node.asBool()) {
                    rootSchema.setAlwaysInvalid(&subschema, true);
                }
                return;
            } else {
                std::string s;
                s += "Expected node at ";
                s += nodePath;
                if (m_version == kDraft7) {
                    s += " to contain schema object or boolean value; actual node type is: ";
                } else {
                    s += " to contain schema object; actual node type is: ";
                }
                s += internal::nodeTypeAsString(node);
                throwRuntimeError(s);
            }
        }

        const typename AdapterType::Object object = node.asObject();
        typename AdapterType::Object::const_iterator itr(object.end());

        // Check for 'id' attribute and update current scope
        opt::optional<std::string> updatedScope;
        if ((itr = object.find("id")) != object.end() && itr->second.maybeString()) {
            const std::string id = itr->second.asString();
            rootSchema.setSubschemaId(&subschema, itr->second.asString());
            if (!currentScope || internal::uri::isUriAbsolute(id) || internal::uri::isUrn(id)) {
                updatedScope = id;
            } else {
                updatedScope = internal::uri::resolveRelativeUri(*currentScope, id);
            }
        } else {
            updatedScope = currentScope;
        }

        if ((itr = object.find("allOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeAllOfConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/allOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("anyOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeAnyOfConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/anyOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("const")) != object.end()) {
            rootSchema.addConstraintToSubschema(makeConstConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("contains")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeContainsConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/contains", fetchDoc,
                            docCache, schemaCache), &subschema);
        }

        if ((itr = object.find("dependencies")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeDependenciesConstraint(rootSchema, rootNode,
                            itr->second, updatedScope,
                            nodePath + "/dependencies", fetchDoc, docCache,
                            schemaCache),
                    &subschema);
        }

        if ((itr = object.find("description")) != object.end()) {
            if (itr->second.maybeString()) {
                rootSchema.setSubschemaDescription(&subschema,
                        itr->second.asString());
            } else {
                throwRuntimeError(
                        "'description' attribute should have a string value");
            }
        }

        if ((itr = object.find("divisibleBy")) != object.end()) {
            if (m_version == kDraft3) {
                if (itr->second.maybeInteger()) {
                    rootSchema.addConstraintToSubschema(
                            makeMultipleOfIntConstraint(itr->second),
                            &subschema);
                } else if (itr->second.maybeDouble()) {
                    rootSchema.addConstraintToSubschema(
                            makeMultipleOfDoubleConstraint(itr->second),
                            &subschema);
                } else {
                    throwRuntimeError("Expected an numeric value for "
                            " 'divisibleBy' constraint.");
                }
            } else {
                throwRuntimeError(
                        "'divisibleBy' constraint not valid after draft 3");
            }
        }

        if ((itr = object.find("enum")) != object.end()) {
            rootSchema.addConstraintToSubschema(makeEnumConstraint(itr->second), &subschema);
        }

        {
            const typename AdapterType::Object::const_iterator itemsItr =
                    object.find("items");

            if (object.end() != itemsItr) {
                if (!itemsItr->second.isArray()) {
                    rootSchema.addConstraintToSubschema(
                            makeSingularItemsConstraint(rootSchema, rootNode,
                                    itemsItr->second, updatedScope,
                                    nodePath + "/items", fetchDoc, docCache,
                                    schemaCache),
                            &subschema);

                } else {
                    const typename AdapterType::Object::const_iterator
                            additionalItemsItr = object.find("additionalItems");
                    rootSchema.addConstraintToSubschema(
                            makeLinearItemsConstraint(rootSchema, rootNode,
                                    itemsItr != object.end() ? &itemsItr->second : nullptr,
                                    additionalItemsItr != object.end() ? &additionalItemsItr->second : nullptr,
                                    updatedScope, nodePath + "/items",
                                    nodePath + "/additionalItems", fetchDoc,
                                    docCache, schemaCache),
                            &subschema);
                }
            }
        }

        {
            const typename AdapterType::Object::const_iterator ifItr = object.find("if");
            const typename AdapterType::Object::const_iterator thenItr = object.find("then");
            const typename AdapterType::Object::const_iterator elseItr = object.find("else");

            if (object.end() != ifItr) {
                if (m_version == kDraft7) {
                    rootSchema.addConstraintToSubschema(
                          makeConditionalConstraint(rootSchema, rootNode,
                                ifItr->second,
                                thenItr == object.end() ? nullptr : &thenItr->second,
                                elseItr == object.end() ? nullptr : &elseItr->second,
                                updatedScope, nodePath, fetchDoc, docCache, schemaCache),
                          &subschema);
                } else {
                    throwRuntimeError("Not supported");
                }
            }
        }

        if (m_version == kDraft7) {
            if ((itr = object.find("exclusiveMaximum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                    makeMaximumConstraintExclusive(itr->second),
                    &subschema);
            }

            if ((itr = object.find("maximum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                    makeMaximumConstraint<AdapterType>(itr->second, nullptr),
                    &subschema);
            }
        } else if ((itr = object.find("maximum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMaximumItr =
                    object.find("exclusiveMaximum");
            if (exclusiveMaximumItr == object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMaximumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            } else {
                rootSchema.addConstraintToSubschema(
                        makeMaximumConstraint(itr->second, &exclusiveMaximumItr->second),
                        &subschema);
            }
        } else if (object.find("exclusiveMaximum") != object.end()) {
            throwRuntimeError("'exclusiveMaximum' constraint only valid if a 'maximum' "
                    "constraint is also present");
        }

        if ((itr = object.find("maxItems")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxItemsConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("maxLength")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxLengthConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("maxProperties")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxPropertiesConstraint(itr->second), &subschema);
        }

        if (m_version == kDraft7) {
            if ((itr = object.find("exclusiveMinimum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraintExclusive(itr->second), &subschema);
            }

            if ((itr = object.find("minimum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            }
        } else if ((itr = object.find("minimum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMinimumItr = object.find("exclusiveMinimum");
            if (exclusiveMinimumItr == object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            } else {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(itr->second, &exclusiveMinimumItr->second),
                        &subschema);
            }
        } else if (object.find("exclusiveMinimum") != object.end()) {
            throwRuntimeError("'exclusiveMinimum' constraint only valid if a 'minimum' "
                    "constraint is also present");
        }

        if ((itr = object.find("minItems")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinItemsConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("minLength")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinLengthConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("minProperties")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinPropertiesConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("multipleOf")) != object.end()) {
            if (m_version == kDraft3) {
                throwRuntimeError("'multipleOf' constraint not available in draft 3");
            } else if (itr->second.maybeInteger()) {
                rootSchema.addConstraintToSubschema(
                        makeMultipleOfIntConstraint(itr->second),
                        &subschema);
            } else if (itr->second.maybeDouble()) {
                rootSchema.addConstraintToSubschema(
                        makeMultipleOfDoubleConstraint(itr->second),
                        &subschema);
            } else {
                throwRuntimeError("Expected an numeric value for 'divisibleBy' constraint.");
            }
        }

        if ((itr = object.find("not")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeNotConstraint(rootSchema, rootNode, itr->second, updatedScope, nodePath + "/not", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("oneOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeOneOfConstraint(rootSchema, rootNode, itr->second, updatedScope, nodePath + "/oneOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("pattern")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makePatternConstraint(itr->second), &subschema);
        }

        {
            // Check for schema keywords that require the creation of a
            // PropertiesConstraint instance.
            const typename AdapterType::Object::const_iterator
                propertiesItr = object.find("properties"),
                patternPropertiesItr = object.find("patternProperties"),
                additionalPropertiesItr = object.find("additionalProperties");
            if (object.end() != propertiesItr ||
                object.end() != patternPropertiesItr ||
                object.end() != additionalPropertiesItr) {
                rootSchema.addConstraintToSubschema(
                        makePropertiesConstraint(rootSchema, rootNode,
                                propertiesItr != object.end() ? &propertiesItr->second : nullptr,
                                patternPropertiesItr != object.end() ? &patternPropertiesItr->second : nullptr,
                                additionalPropertiesItr != object.end() ? &additionalPropertiesItr->second : nullptr,
                                updatedScope, nodePath + "/properties",
                                nodePath + "/patternProperties",
                                nodePath + "/additionalProperties",
                                fetchDoc, &subschema, docCache, schemaCache),
                        &subschema);
            }
        }

        if ((itr = object.find("propertyNames")) != object.end()) {
            if (m_version == kDraft7) {
                rootSchema.addConstraintToSubschema(
                      makePropertyNamesConstraint(rootSchema, rootNode, itr->second, updatedScope,
                              nodePath, fetchDoc, docCache, schemaCache),
                      &subschema);
            } else {
                throwRuntimeError("Not supported");
            }
        }

        if ((itr = object.find("required")) != object.end()) {
            if (m_version == kDraft3) {
                if (parentSubschema && ownName) {
                    opt::optional<constraints::RequiredConstraint> constraint =
                            makeRequiredConstraintForSelf(itr->second, *ownName);
                    if (constraint) {
                        rootSchema.addConstraintToSubschema(*constraint, parentSubschema);
                    }
                } else {
                    throwRuntimeError("'required' constraint not valid here");
                }
            } else {
                rootSchema.addConstraintToSubschema(makeRequiredConstraint(itr->second), &subschema);
            }
        }

        if ((itr = object.find("title")) != object.end()) {
            if (itr->second.maybeString()) {
                rootSchema.setSubschemaTitle(&subschema, itr->second.asString());
            } else {
                throwRuntimeError("'title' attribute should have a string value");
            }
        }

        if ((itr = object.find("type")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeTypeConstraint(rootSchema, rootNode, itr->second, updatedScope, nodePath + "/type", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("uniqueItems")) != object.end()) {
            opt::optional<constraints::UniqueItemsConstraint> constraint = makeUniqueItemsConstraint(itr->second);
            if (constraint) {
                rootSchema.addConstraintToSubschema(*constraint, &subschema);
            }
        }

        for (const auto & constraintBuilder : constraintBuilders) {
            if ((itr = object.find(constraintBuilder.first)) != object.end()) {
                constraints::Constraint *constraint = nullptr;
#if VALIJSON_USE_EXCEPTIONS
                try {
#endif
                    constraint = constraintBuilder.second->make(itr->second);
                    rootSchema.addConstraintToSubschema(*constraint, &subschema);
                    delete constraint;
#if VALIJSON_USE_EXCEPTIONS
                } catch (...) {
                    delete constraint;
                    throw;
                }
#endif
            }
        }
    }

    /**
     * @brief  Resolves a chain of JSON References before populating a schema
     *
     * This helper function is used directly by the publicly visible
     * populateSchema function. It ensures that the node being parsed is a
     * concrete node, and not a JSON Reference. This function will call itself
     * recursively to resolve references until a concrete node is found.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to node to parse
     * @param  subschema     Reference to Schema to populate
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     */
    template<typename AdapterType>
    void resolveThenPopulateSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const Subschema &subschema,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSchema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        std::string jsonRef;
        if (!extractJsonReference(node, jsonRef)) {
            populateSchema(rootSchema, rootNode, node, subschema, currentScope, nodePath, fetchDoc, parentSchema,
                    ownName, docCache, schemaCache);
            return;
        }

        // Returns a document URI if the reference points somewhere
        // other than the current document
        const opt::optional<std::string> documentUri = internal::json_reference::getJsonReferenceUri(jsonRef);

        // Extract JSON Pointer from JSON Reference
        const std::string actualJsonPointer = sanitiseJsonPointer(
                internal::json_reference::getJsonReferencePointer(jsonRef));

        if (documentUri && (internal::uri::isUriAbsolute(*documentUri) || internal::uri::isUrn(*documentUri))) {
            // Resolve reference against remote document
            if (!fetchDoc) {
                throwRuntimeError("Fetching of remote JSON References not enabled.");
            }

            const typename DocumentCache<AdapterType>::DocumentType *newDoc = fetchDoc(*documentUri);

            // Can't proceed without the remote document
            if (!newDoc) {
                throwRuntimeError("Failed to fetch referenced schema document: " + *documentUri);
            }

            // Add to document cache
            typedef typename DocumentCache<AdapterType>::Type::value_type DocCacheValueType;

            docCache.insert(DocCacheValueType(*documentUri, newDoc));

            const AdapterType newRootNode(*newDoc);

            const AdapterType &referencedAdapter =
                internal::json_pointer::resolveJsonPointer(newRootNode, actualJsonPointer);

            // TODO: Need to detect degenerate circular references
            resolveThenPopulateSchema(rootSchema, newRootNode, referencedAdapter, subschema, {}, actualJsonPointer,
                    fetchDoc, parentSchema, ownName, docCache, schemaCache);

        } else {
            const AdapterType &referencedAdapter =
                    internal::json_pointer::resolveJsonPointer(rootNode, actualJsonPointer);

            // TODO: Need to detect degenerate circular references
            resolveThenPopulateSchema(rootSchema, rootNode, referencedAdapter, subschema, {}, actualJsonPointer,
                    fetchDoc, parentSchema, ownName, docCache, schemaCache);
        }
    }

    /**
     * @brief   Make a new AllOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new AllOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AllOfConstraint makeAllOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeArray()) {
            throwRuntimeError("Expected array value for 'allOf' constraint.");
        }

        constraints::AllOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.asArray()) {
            if (schemaNode.maybeObject() || (m_version == kDraft7 && schemaNode.isBool())) {
                const std::string childPath = nodePath + "/" + std::to_string(index);
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, schemaNode, currentScope,
                        childPath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
                constraint.addSubschema(subschema);
                index++;
            } else {
                throwRuntimeError("Expected element to be a valid schema in 'allOf' constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new AnyOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new AnyOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AnyOfConstraint makeAnyOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeArray()) {
            throwRuntimeError("Expected array value for 'anyOf' constraint.");
        }

        constraints::AnyOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.asArray()) {
            if (schemaNode.maybeObject() || (m_version == kDraft7 && schemaNode.isBool())) {
                const std::string childPath = nodePath + "/" + std::to_string(index);
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, schemaNode, currentScope,
                        childPath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
                constraint.addSubschema(subschema);
                index++;
            } else {
                throwRuntimeError("Expected array element to be a valid schema in 'anyOf' constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new ConditionalConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   ifNode               Schema that will be used to evaluate the
     *                               conditional.
     * @param   thenNode             Optional pointer to a JSON node containing
     *                               a schema that will be used when the conditional
     *                               evaluates to true.
     * @param   elseNode             Optional pointer to a JSON node containing
     *                               a schema that will be used when the conditional
     *                               evaluates to false.
     * @param   currentScope         URI for current resolution scope
     * @param   containsPath         JSON Pointer representing the path to
     *                               the 'contains' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ContainsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ConditionalConstraint makeConditionalConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &ifNode,
        const AdapterType *thenNode,
        const AdapterType *elseNode,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::ConditionalConstraint constraint;

        const Subschema *ifSubschema = makeOrReuseSchema<AdapterType>(
                rootSchema, rootNode, ifNode, currentScope,
                nodePath + "/if", fetchDoc, nullptr, nullptr, docCache,
                schemaCache);
        constraint.setIfSubschema(ifSubschema);

        if (thenNode) {
            const Subschema *thenSubschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, *thenNode, currentScope, nodePath + "/then", fetchDoc, nullptr,
                    nullptr, docCache, schemaCache);
            constraint.setThenSubschema(thenSubschema);
        }

        if (elseNode) {
            const Subschema *elseSubschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, *elseNode, currentScope, nodePath + "/else", fetchDoc, nullptr,
                    nullptr, docCache, schemaCache);
            constraint.setElseSubschema(elseSubschema);
        }

        return constraint;
    }

    /**
     * @brief   Make a new ConstConstraint object.
     *
     * @param   node  JSON node containing an arbitrary value
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ConstConstraint makeConstConstraint(const AdapterType &node)
    {
        constraints::ConstConstraint constraint;
        constraint.setValue(node);
        return constraint;
    }

    /**
     * @brief   Make a new ContainsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   contains             Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   currentScope         URI for current resolution scope
     * @param   containsPath         JSON Pointer representing the path to
     *                               the 'contains' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ContainsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ContainsConstraint makeContainsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &contains,
        const opt::optional<std::string> currentScope,
        const std::string &containsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::ContainsConstraint constraint;

        if (contains.isObject() || (m_version == kDraft7 && contains.maybeBool())) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, contains, currentScope, containsPath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.setSubschema(subschema);

        } else if (contains.maybeObject()) {
            // If a loosely-typed Adapter type is being used, then we'll
            // assume that an empty schema has been provided.
            constraint.setSubschema(rootSchema.emptySubschema());

        } else {
            // All other formats will result in an exception being thrown.
            throwRuntimeError("Expected valid schema for 'contains' constraint.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new DependenciesConstraint object
     *
     * The dependencies for a property can be defined several ways. When parsing
     * a Draft 4 schema, the following can be used:
     *  - an array that lists the name of each property that must be present
     *    if the dependent property is present
     *  - an object that specifies a schema which must be satisfied if the
     *    dependent property is present
     *
     * When parsing a Draft 3 schema, in addition to the formats above, the
     * following format can be used:
     *  - a string that names a single property that must be present if the
     *    dependent property is presnet
     *
     * Multiple methods can be used in the same dependency constraint.
     *
     * If the format of any part of the the dependency node does not match one
     * of these formats, an exception will be thrown.
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an object that defines a
     *                        mapping of properties to their dependencies.
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new DependencyConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::DependenciesConstraint makeDependenciesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeObject()) {
            throwRuntimeError("Expected valid subschema for 'dependencies' constraint.");
        }

        constraints::DependenciesConstraint dependenciesConstraint;

        // Process each of the dependency mappings defined by the object
        for (const typename AdapterType::ObjectMember member : node.asObject()) {

            // First, we attempt to parse the value of the dependency mapping
            // as an array of strings. If the Adapter type does not support
            // strict types, then an empty string or empty object will be cast
            // to an array, and the resulting dependency list will be empty.
            // This is equivalent to using an empty object, but does mean that
            // if the user provides an actual string then this error will not
            // be detected.
            if (member.second.maybeArray()) {
                // Parse an array of dependency names
                std::vector<std::string> dependentPropertyNames;
                for (const AdapterType dependencyName : member.second.asArray()) {
                    if (dependencyName.maybeString()) {
                        dependentPropertyNames.push_back(dependencyName.getString());
                    } else {
                        throwRuntimeError("Expected string value in dependency list of property '" +
                            member.first + "' in 'dependencies' constraint.");
                    }
                }

                dependenciesConstraint.addPropertyDependencies(member.first,
                        dependentPropertyNames);

            // If the value of dependency mapping could not be processed as an
            // array, we'll try to process it as an object instead. Note that
            // strict type comparison is used here, since we've already
            // exercised the flexibility by loosely-typed Adapter types. If the
            // value of the dependency mapping is an object, then we'll try to
            // process it as a dependent schema.
            } else if (member.second.isObject() || (m_version == kDraft7 && member.second.maybeBool())) {
                // Parse dependent subschema
                const Subschema *childSubschema =
                        makeOrReuseSchema<AdapterType>(rootSchema, rootNode,
                                member.second, currentScope, nodePath, fetchDoc,
                                nullptr, nullptr, docCache, schemaCache);
                dependenciesConstraint.addSchemaDependency(member.first,
                        childSubschema);

            // If we're supposed to be parsing a Draft3 schema, then the value
            // of the dependency mapping can also be a string containing the
            // name of a single dependency.
            } else if (m_version == kDraft3 && member.second.isString()) {
                dependenciesConstraint.addPropertyDependency(member.first,
                        member.second.getString());

            // All other types result in an exception being thrown.
            } else {
                throwRuntimeError("Invalid dependencies definition.");
            }
        }

        return dependenciesConstraint;
    }

    /**
     * @brief   Make a new EnumConstraint object.
     *
     * @param   node  JSON node containing an array of values permitted by the
     *                constraint.
     *
     * @return  pointer to a new EnumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::EnumConstraint makeEnumConstraint(
        const AdapterType &node)
    {
        // Make a copy of each value in the enum array
        constraints::EnumConstraint constraint;
        for (const AdapterType value : node.getArray()) {
            constraint.addValue(value);
        }

        /// @todo This will make another copy of the values while constructing
        /// the EnumConstraint. Move semantics in C++11 should make it possible
        /// to avoid these copies without complicating the implementation of the
        /// EnumConstraint class.
        return constraint;
    }

    /**
     * @brief   Make a new ItemsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   items                Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   additionalItems      Optional pointer to a JSON node containing
     *                               an additional properties schema or a
     *                               boolean value.
     * @param   currentScope         URI for current resolution scope
     * @param   itemsPath            JSON Pointer representing the path to
     *                               the 'items' node
     * @param   additionalItemsPath  JSON Pointer representing the path to
     *                               the 'additionalItems' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::LinearItemsConstraint makeLinearItemsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType *items,
        const AdapterType *additionalItems,
        const opt::optional<std::string> currentScope,
        const std::string &itemsPath,
        const std::string &additionalItemsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::LinearItemsConstraint constraint;

        // Construct a Schema object for the the additionalItems constraint,
        // if the additionalItems property is present
        if (additionalItems) {
            if (additionalItems->maybeBool()) {
                // If the value of the additionalItems property is a boolean
                // and is set to true, then additional array items do not need
                // to satisfy any constraints.
                if (additionalItems->asBool()) {
                    constraint.setAdditionalItemsSubschema(rootSchema.emptySubschema());
                }
            } else if (additionalItems->maybeObject()) {
                // If the value of the additionalItems property is an object,
                // then it should be parsed into a Schema object, which will be
                // used to validate additional array items.
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, *additionalItems, currentScope,
                        additionalItemsPath, fetchDoc, nullptr, nullptr, docCache,
                        schemaCache);
                constraint.setAdditionalItemsSubschema(subschema);
            } else {
                // Any other format for the additionalItems property will result
                // in an exception being thrown.
                throwRuntimeError("Expected bool or object value for 'additionalItems'");
            }
        } else {
            // The default value for the additionalItems property is an empty
            // object, which means that additional array items do not need to
            // satisfy any constraints.
            constraint.setAdditionalItemsSubschema(rootSchema.emptySubschema());
        }

        // Construct a Schema object for each item in the items array.
        // If the items constraint is not provided, then array items
        // will be validated against the additionalItems schema.
        if (items) {
            if (items->isArray()) {
                // If the items constraint contains an array, then it should
                // contain a list of child schemas which will be used to
                // validate the values at the corresponding indexes in a target
                // array.
                int index = 0;
                for (const AdapterType v : items->getArray()) {
                    const std::string childPath = itemsPath + "/" +
                            std::to_string(index);
                    const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                            rootSchema, rootNode, v, currentScope, childPath,
                            fetchDoc, nullptr, nullptr, docCache, schemaCache);
                    constraint.addItemSubschema(subschema);
                    index++;
                }
            } else {
                throwRuntimeError("Expected array value for non-singular 'items' constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new ItemsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   items                Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   additionalItems      Optional pointer to a JSON node containing
     *                               an additional properties schema or a
     *                               boolean value.
     * @param   currentScope         URI for current resolution scope
     * @param   itemsPath            JSON Pointer representing the path to
     *                               the 'items' node
     * @param   additionalItemsPath  JSON Pointer representing the path to
     *                               the 'additionalItems' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::SingularItemsConstraint makeSingularItemsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &items,
        const opt::optional<std::string> currentScope,
        const std::string &itemsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::SingularItemsConstraint constraint;

        // Construct a Schema object for each item in the items array, if an
        // array is provided, or a single Schema object, in an object value is
        // provided. If the items constraint is not provided, then array items
        // will be validated against the additionalItems schema.
        if (items.isObject() || (m_version == kDraft7 && items.maybeBool())) {
            // If the items constraint contains an object value, then it
            // should contain a Schema that will be used to validate all
            // items in a target array. Any schema defined by the
            // additionalItems constraint will be ignored.
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, items, currentScope, itemsPath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.setItemsSubschema(subschema);

        } else if (items.maybeObject()) {
            // If a loosely-typed Adapter type is being used, then we'll
            // assume that an empty schema has been provided.
            constraint.setItemsSubschema(rootSchema.emptySubschema());

        } else {
            // All other formats will result in an exception being thrown.
            throwRuntimeError("Expected valid schema for singular 'items' constraint.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new MaximumConstraint object (draft 3 and 4).
     *
     * @param   rootSchema        The Schema instance, and root subschema,
     *                            through which other subschemas can be
     *                            created and modified
     * @param   rootNode          Reference to the node from which JSON
     *                            References will be resolved when they refer
     *                            to the current document; used for recursive
     *                            parsing of schemas
     * @param   node              JSON node containing the maximum value.
     * @param   exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                            indicates whether maximum value is excluded
     *                            from the range of permitted values.
     *
     * @return  pointer to a new MaximumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaximumConstraint makeMaximumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMaximum)
    {
        if (!node.maybeDouble()) {
            throwRuntimeError("Expected numeric value for maximum constraint.");
        }

        constraints::MaximumConstraint constraint;
        constraint.setMaximum(node.asDouble());

        if (exclusiveMaximum) {
            if (!exclusiveMaximum->maybeBool()) {
                throwRuntimeError("Expected boolean value for exclusiveMaximum constraint.");
            }

            constraint.setExclusiveMaximum(exclusiveMaximum->asBool());
        }

        return constraint;
    }

    /**
     * @brief   Make a new MaximumConstraint object that is always exclusive (draft 7).
     *
     * @param   node       JSON node containing an integer, representing the maximum value.
     *
     * @param   exclusive  Optional pointer to a JSON boolean value that indicates whether the
     *                     maximum value is excluded from the range of permitted values.
     *
     * @return  pointer to a new Maximum that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaximumConstraint makeMaximumConstraintExclusive(const AdapterType &node)
    {
        if (!node.maybeDouble()) {
            throwRuntimeError("Expected numeric value for exclusiveMaximum constraint.");
        }

        constraints::MaximumConstraint constraint;
        constraint.setMaximum(node.asDouble());
        constraint.setExclusiveMaximum(true);
        return constraint;
    }

    /**
     * @brief   Make a new MaxItemsConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of items that may be contaned by an array.
     *
     * @return  pointer to a new MaxItemsConstraint that belongs to the caller.
     */
    template<typename AdapterType>
    constraints::MaxItemsConstraint makeMaxItemsConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxItemsConstraint constraint;
                constraint.setMaxItems(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected non-negative integer value for 'maxItems' constraint.");
    }

    /**
     * @brief   Make a new MaxLengthConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum length of a string.
     *
     * @return  pointer to a new MaxLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaxLengthConstraint makeMaxLengthConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxLengthConstraint constraint;
                constraint.setMaxLength(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected a non-negative integer value for 'maxLength' constraint.");
    }

    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MaxPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MaxPropertiesConstraint makeMaxPropertiesConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxPropertiesConstraint constraint;
                constraint.setMaxProperties(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected a non-negative integer for 'maxProperties' constraint.");
    }

    /**
     * @brief  Make a new MinimumConstraint object (draft 3 and 4).
     *
     * @param  node              JSON node containing an integer, representing
     *                           the minimum value.
     *
     * @param  exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                           indicates whether the minimum value is
     *                           excluded from the range of permitted values.
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinimumConstraint makeMinimumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMinimum)
    {
        if (!node.maybeDouble()) {
            throwRuntimeError("Expected numeric value for minimum constraint.");
        }

        constraints::MinimumConstraint constraint;
        constraint.setMinimum(node.asDouble());

        if (exclusiveMinimum) {
            if (!exclusiveMinimum->maybeBool()) {
                throwRuntimeError("Expected boolean value for 'exclusiveMinimum' constraint.");
            }

            constraint.setExclusiveMinimum(exclusiveMinimum->asBool());
        }

        return constraint;
    }

    /**
     * @brief   Make a new MinimumConstraint object that is always exclusive (draft 7).
     *
     * @param   node       JSON node containing an integer, representing the minimum value.
     *
     * @param   exclusive  Optional pointer to a JSON boolean value that indicates whether the
     *                     minimum value is excluded from the range of permitted values.
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinimumConstraint makeMinimumConstraintExclusive(const AdapterType &node)
    {
        if (!node.maybeDouble()) {
            throwRuntimeError("Expected numeric value for exclusiveMinimum constraint.");
        }

        constraints::MinimumConstraint constraint;
        constraint.setMinimum(node.asDouble());
        constraint.setExclusiveMinimum(true);
        return constraint;
    }

    /**
     * @brief  Make a new MinItemsConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum number of items that may be contained by an array.
     *
     * @return  pointer to a new MinItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinItemsConstraint makeMinItemsConstraint(const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinItemsConstraint constraint;
                constraint.setMinItems(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected a non-negative integer value for 'minItems' constraint.");
    }

    /**
     * @brief  Make a new MinLengthConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum length of a string.
     *
     * @return  pointer to a new MinLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinLengthConstraint makeMinLengthConstraint(const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinLengthConstraint constraint;
                constraint.setMinLength(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected a non-negative integer value for 'minLength' constraint.");
    }


    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                minimum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MinPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MinPropertiesConstraint makeMinPropertiesConstraint(const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinPropertiesConstraint constraint;
                constraint.setMinProperties(value);
                return constraint;
            }
        }

        throwRuntimeError("Expected a non-negative integer for 'minProperties' constraint.");
    }

    /**
     * @brief   Make a new MultipleOfDoubleConstraint object
     *
     * @param   node  JSON node containing an numeric value that a target value
     *                must divide by in order to satisfy this constraint
     *
     * @return  a MultipleOfConstraint
     */
    template<typename AdapterType>
    constraints::MultipleOfDoubleConstraint makeMultipleOfDoubleConstraint(const AdapterType &node)
    {
        constraints::MultipleOfDoubleConstraint constraint;
        constraint.setDivisor(node.asDouble());
        return constraint;
    }

    /**
     * @brief   Make a new MultipleOfIntConstraint object
     *
     * @param   node  JSON node containing a numeric value that a target value
     *                must divide by in order to satisfy this constraint
     *
     * @return  a MultipleOfIntConstraint
     */
    template<typename AdapterType>
    constraints::MultipleOfIntConstraint makeMultipleOfIntConstraint(const AdapterType &node)
    {
        constraints::MultipleOfIntConstraint constraint;
        constraint.setDivisor(node.asInteger());
        return constraint;
    }

    /**
     * @brief   Make a new NotConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing a schema
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new NotConstraint object that belongs to the caller
     */
    template<typename AdapterType>
    constraints::NotConstraint makeNotConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (node.maybeObject() || (m_version == kDraft7 && node.maybeBool())) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, node, currentScope, nodePath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraints::NotConstraint constraint;
            constraint.setSubschema(subschema);
            return constraint;
        }

        throwRuntimeError("Expected object value for 'not' constraint.");
    }

    /**
     * @brief   Make a new OneOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new OneOfConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::OneOfConstraint makeOneOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::OneOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.getArray()) {
            const std::string childPath = nodePath + "/" + std::to_string(index);
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                rootSchema, rootNode, schemaNode, currentScope, childPath,
                fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.addSubschema(subschema);
            index++;
        }

        return constraint;
    }

    /**
     * @brief   Make a new PatternConstraint object.
     *
     * @param   node      JSON node containing a pattern string
     *
     * @return  pointer to a new PatternConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::PatternConstraint makePatternConstraint(
        const AdapterType &node)
    {
        constraints::PatternConstraint constraint;
        constraint.setPattern(node.getString());
        return constraint;
    }

    /**
     * @brief   Make a new Properties object.
     *
     * @param   rootSchema                The Schema instance, and root
     *                                    subschema, through which other
     *                                    subschemas can be created and modified
     * @param   rootNode                  Reference to the node from which JSON
     *                                    References will be resolved when they
     *                                    refer to the current document; used
     *                                    for recursive parsing of schemas
     * @param   properties                Optional pointer to a JSON node
     *                                    containing an object mapping property
     *                                    names to schemas.
     * @param   patternProperties         Optional pointer to a JSON node
     *                                    containing an object mapping pattern
     *                                    property names to schemas.
     * @param   additionalProperties      Optional pointer to a JSON node
     *                                    containing an additional properties
     *                                    schema or a boolean value.
     * @param   currentScope              URI for current resolution scope
     * @param   propertiesPath            JSON Pointer representing the path to
     *                                    the 'properties' node
     * @param   patternPropertiesPath     JSON Pointer representing the path to
     *                                    the 'patternProperties' node
     * @param   additionalPropertiesPath  JSON Pointer representing the path to
     *                                    the 'additionalProperties' node
     * @param   fetchDoc                  Function to fetch remote JSON
     *                                    documents (optional)
     * @param   parentSubschema           Optional pointer to the Schema of the
     *                                    parent object, needed to support the
     *                                    'required' keyword in Draft 3
     * @param   docCache                  Cache of resolved and fetched remote
     *                                    documents
     * @param   schemaCache               Cache of populated schemas
     *
     * @return  pointer to a new Properties that belongs to the caller
     */
    template<typename AdapterType>
    constraints::PropertiesConstraint makePropertiesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType *properties,
        const AdapterType *patternProperties,
        const AdapterType *additionalProperties,
        const opt::optional<std::string> currentScope,
        const std::string &propertiesPath,
        const std::string &patternPropertiesPath,
        const std::string &additionalPropertiesPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        typedef typename AdapterType::ObjectMember Member;

        constraints::PropertiesConstraint constraint;

        // Create subschemas for 'properties' constraint
        if (properties) {
            for (const Member m : properties->getObject()) {
                const std::string &property = m.first;
                const std::string childPath = propertiesPath + "/" + property;
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, m.second, currentScope, childPath,
                        fetchDoc, parentSubschema, &property, docCache,
                        schemaCache);
                constraint.addPropertySubschema(property, subschema);
            }
        }

        // Create subschemas for 'patternProperties' constraint
        if (patternProperties) {
            for (const Member m : patternProperties->getObject()) {
                const std::string &pattern = m.first;
                const std::string childPath = patternPropertiesPath + "/" + pattern;
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, m.second, currentScope, childPath,
                        fetchDoc, parentSubschema, &pattern, docCache,
                        schemaCache);
                constraint.addPatternPropertySubschema(pattern, subschema);
            }
        }

        // Create an additionalItems subschema if required
        if (additionalProperties) {
            // If additionalProperties has been set, check for a boolean value.
            // Setting 'additionalProperties' to true allows the values of
            // additional properties to take any form. Setting it false
            // prohibits the use of additional properties.
            // If additionalProperties is instead an object, it should be
            // parsed as a schema. If additionalProperties has any other type,
            // then the schema is not valid.
            if (additionalProperties->isBool() ||
                additionalProperties->maybeBool()) {
                // If it has a boolean value that is 'true', then an empty
                // schema should be used.
                if (additionalProperties->asBool()) {
                    constraint.setAdditionalPropertiesSubschema(rootSchema.emptySubschema());
                }
            } else if (additionalProperties->isObject()) {
                // If additionalProperties is an object, it should be used as
                // a child schema.
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, *additionalProperties,
                        currentScope, additionalPropertiesPath, fetchDoc, nullptr,
                        nullptr, docCache, schemaCache);
                constraint.setAdditionalPropertiesSubschema(subschema);
            } else {
                // All other types are invalid
                throwRuntimeError("Invalid type for 'additionalProperties' constraint.");
            }
        } else {
            // If an additionalProperties constraint is not provided, then the
            // default value is an empty schema.
            constraint.setAdditionalPropertiesSubschema(rootSchema.emptySubschema());
        }

        return constraint;
    }

    template<typename AdapterType>
    constraints::PropertyNamesConstraint makePropertyNamesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &currentNode,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        const Subschema *subschema = makeOrReuseSchema<AdapterType>(rootSchema, rootNode, currentNode, currentScope,
                nodePath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
        constraints::PropertyNamesConstraint constraint;
        constraint.setSubschema(subschema);
        return constraint;
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 3 schemas.
     *
     * @param   node  Node containing a boolean value.
     * @param   name  Name of the required attribute.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    opt::optional<constraints::RequiredConstraint>
            makeRequiredConstraintForSelf(const AdapterType &node,
                    const std::string &name)
    {
        if (!node.maybeBool()) {
            throwRuntimeError("Expected boolean value for 'required' attribute.");
        }

        if (node.asBool()) {
            constraints::RequiredConstraint constraint;
            constraint.addRequiredProperty(name);
            return constraint;
        }

        return opt::optional<constraints::RequiredConstraint>();
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 4 schemas.
     *
     * @param   node  Node containing an array of strings.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::RequiredConstraint makeRequiredConstraint(
        const AdapterType &node)
    {
        constraints::RequiredConstraint constraint;

        for (const AdapterType v : node.getArray()) {
            if (!v.maybeString()) {
                throwRuntimeError("Expected required property name to be a string value");
            }

            constraint.addRequiredProperty(v.getString());
        }

        return constraint;
    }

    /**
     * @brief   Make a new TypeConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          Node containing the name of a JSON type
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new TypeConstraint object.
     */
    template<typename AdapterType>
    constraints::TypeConstraint makeTypeConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        typedef constraints::TypeConstraint TypeConstraint;

        TypeConstraint constraint;

        if (node.maybeString()) {
            const TypeConstraint::JsonType type = TypeConstraint::jsonTypeFromString(node.getString());
            if (type == TypeConstraint::kAny && m_version == kDraft4) {
                throwRuntimeError("'any' type is not supported in version 4 schemas.");
            }

            constraint.addNamedType(type);

        } else if (node.maybeArray()) {
            int index = 0;
            for (const AdapterType v : node.getArray()) {
                if (v.maybeString()) {
                    const TypeConstraint::JsonType type = TypeConstraint::jsonTypeFromString(v.getString());
                    if (type == TypeConstraint::kAny && m_version == kDraft4) {
                        throwRuntimeError("'any' type is not supported in version 4 schemas.");
                    }

                    constraint.addNamedType(type);

                } else if (v.maybeObject() && m_version == kDraft3) {
                    const std::string childPath = nodePath + "/" + std::to_string(index);
                    const Subschema *subschema = makeOrReuseSchema<AdapterType>(rootSchema, rootNode, v, currentScope,
                            childPath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
                    constraint.addSchemaType(subschema);

                } else {
                    throwRuntimeError("Type name should be a string.");
                }

                index++;
            }

        } else if (node.maybeObject() && m_version == kDraft3) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(rootSchema, rootNode, node, currentScope,
                    nodePath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.addSchemaType(subschema);

        } else {
            throwRuntimeError("Type name should be a string.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new UniqueItemsConstraint object.
     *
     * @param   node  Node containing a boolean value.
     *
     * @return  pointer to a new UniqueItemsConstraint object that belongs to
     *          the caller, or nullptr if the boolean value is false.
     */
    template<typename AdapterType>
    opt::optional<constraints::UniqueItemsConstraint> makeUniqueItemsConstraint(const AdapterType &node)
    {
        if (node.isBool() || node.maybeBool()) {
            // If the boolean value is true, this function will return a pointer
            // to a new UniqueItemsConstraint object. If it is value, then the
            // constraint is redundant, so nullptr is returned instead.
            if (node.asBool()) {
                return constraints::UniqueItemsConstraint();
            } else {
                return opt::optional<constraints::UniqueItemsConstraint>();
            }
        }

        throwRuntimeError("Expected boolean value for 'uniqueItems' constraint.");
    }

private:

    /// Version of JSON Schema that should be expected when parsing
    Version m_version;
};

}  // namespace valijson
/**
 * @file
 *
 * @brief   Adapter implementation that wraps a single std::string value
 *
 * This allows property names to be validated against a schema as though they are a generic JSON
 * value, while allowing the rest of Valijson's API to expose property names as plain std::string
 * values.
 *
 * This was added while implementing draft 7 support. This included support for a constraint
 * called propertyNames, which can be used to ensure that the property names in an object
 * validate against a subschema.
 */

#pragma once

#include <string>


namespace valijson {
namespace adapters {

class StdStringAdapter;
class StdStringArrayValueIterator;
class StdStringObjectMemberIterator;

typedef std::pair<std::string, StdStringAdapter> StdStringObjectMember;

class StdStringArray
{
public:
    typedef StdStringArrayValueIterator const_iterator;
    typedef StdStringArrayValueIterator iterator;

    StdStringArray() = default;

    StdStringArrayValueIterator begin() const;

    StdStringArrayValueIterator end() const;

    static size_t size()
    {
        return 0;
    }
};

class StdStringObject
{
public:
    typedef StdStringObjectMemberIterator const_iterator;
    typedef StdStringObjectMemberIterator iterator;

    StdStringObject() = default;

    StdStringObjectMemberIterator begin() const;

    StdStringObjectMemberIterator end() const;

    StdStringObjectMemberIterator find(const std::string &propertyName) const;

    static size_t size()
    {
        return 0;
    }
};

class StdStringFrozenValue: public FrozenValue
{
public:
    explicit StdStringFrozenValue(std::string source)
      : value(std::move(source)) { }

    FrozenValue * clone() const override
    {
        return new StdStringFrozenValue(value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:
    std::string value;
};

class StdStringAdapter: public Adapter
{
public:
    typedef StdStringArray Array;
    typedef StdStringObject Object;
    typedef StdStringObjectMember ObjectMember;

    explicit StdStringAdapter(const std::string &value)
      : m_value(value) { }

    bool applyToArray(ArrayValueCallback) const override
    {
        return maybeArray();
    }

    bool applyToObject(ObjectMemberCallback) const override
    {
        return maybeObject();
    }

    StdStringArray asArray() const
    {
        if (maybeArray()) {
            return {};
        }

        throwRuntimeError("String value cannot be cast to array");
    }

    bool asBool() const override
    {
        return true;
    }

    bool asBool(bool &result) const override
    {
        result = true;
        return true;
    }

    double asDouble() const override
    {
        return 0;
    }

    bool asDouble(double &result) const override
    {
        result = 0;
        return true;
    }

    int64_t asInteger() const override
    {
        return 0;
    }

    bool asInteger(int64_t &result) const override
    {
        result = 0;
        return true;
    };

    StdStringObject asObject() const
    {
        if (maybeObject()) {
            return {};
        }

        throwRuntimeError("String value cannot be cast to object");
    }

    std::string asString() const override
    {
        return m_value;
    }

    bool asString(std::string &result) const override
    {
        result = m_value;
        return true;
    }

    bool equalTo(const Adapter &other, bool strict) const override
    {
        if (strict && !other.isString()) {
            return false;
        }

        return m_value == other.asString();
    }

    FrozenValue* freeze() const override
    {
        return new StdStringFrozenValue(m_value);
    }

    static StdStringArray getArray()
    {
        throwNotSupported();
    }

    size_t getArraySize() const override
    {
        throwNotSupported();
    }

    bool getArraySize(size_t &) const override
    {
        throwNotSupported();
    }

    bool getBool() const override
    {
        throwNotSupported();
    }

    bool getBool(bool &) const override
    {
        throwNotSupported();
    }

    double getDouble() const override
    {
        throwNotSupported();
    }

    bool getDouble(double &) const override
    {
        throwNotSupported();
    }

    int64_t getInteger() const override
    {
        throwNotSupported();
    }

    bool getInteger(int64_t &) const override
    {
        throwNotSupported();
    }

    double getNumber() const override
    {
        throwNotSupported();
    }

    bool getNumber(double &) const override
    {
        throwNotSupported();
    }

    size_t getObjectSize() const override
    {
        throwNotSupported();
    }

    bool getObjectSize(size_t &) const override
    {
        throwNotSupported();
    }

    std::string getString() const override
    {
        return m_value;
    }

    bool getString(std::string &result) const override
    {
        result = m_value;
        return true;
    }

    bool hasStrictTypes() const override
    {
        return true;
    }

    bool isArray() const override
    {
        return false;
    }

    bool isBool() const override
    {
        return false;
    }

    bool isDouble() const override
    {
        return false;
    }

    bool isInteger() const override
    {
        return false;
    }

    bool isNull() const override
    {
        return false;
    }

    bool isNumber() const override
    {
        return false;
    }

    bool isObject() const override
    {
        return false;
    }

    bool isString() const override
    {
        return true;
    }

    bool maybeArray() const override
    {
        return false;
    }

    bool maybeBool() const override
    {
        return m_value == "true" || m_value == "false";
    }

    bool maybeDouble() const override
    {
        const char *b = m_value.c_str();
        char *e = nullptr;
        strtod(b, &e);
        return e != b && e == b + m_value.length();
    }

    bool maybeInteger() const override
    {
        std::istringstream i(m_value);
        int64_t x;
        char c;
        if (!(i >> x) || i.get(c)) {
            return false;
        }

        return true;
    }

    bool maybeNull() const override
    {
        return m_value.empty();
    }

    bool maybeObject() const override
    {
        return m_value.empty();
    }

    bool maybeString() const override
    {
        return true;
    }

private:
    const std::string &m_value;
};

class StdStringArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = StdStringAdapter;
    using difference_type = StdStringAdapter;
    using pointer = StdStringAdapter*;
    using reference = StdStringAdapter&;

    StdStringAdapter operator*() const
    {
        throwNotSupported();
    }

    DerefProxy<StdStringAdapter> operator->() const
    {
        throwNotSupported();
    }

    bool operator==(const StdStringArrayValueIterator &) const
    {
        return true;
    }

    bool operator!=(const StdStringArrayValueIterator &) const
    {
        return false;
    }

    const StdStringArrayValueIterator& operator++()
    {
        throwNotSupported();
    }

    StdStringArrayValueIterator operator++(int)
    {
        throwNotSupported();
    }

    const StdStringArrayValueIterator& operator--()
    {
        throwNotSupported();
    }

    void advance(std::ptrdiff_t)
    {
        throwNotSupported();
    }
};

inline StdStringArrayValueIterator StdStringArray::begin() const
{
    return {};
}

inline StdStringArrayValueIterator StdStringArray::end() const
{
    return {};
}

class StdStringObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = StdStringObjectMember;
    using difference_type = StdStringObjectMember;
    using pointer = StdStringObjectMember*;
    using reference = StdStringObjectMember&;

    StdStringObjectMember operator*() const
    {
        throwNotSupported();
    }

    DerefProxy<StdStringObjectMember> operator->() const
    {
        throwNotSupported();
    }

    bool operator==(const StdStringObjectMemberIterator &) const
    {
        return true;
    }

    bool operator!=(const StdStringObjectMemberIterator &) const
    {
        return false;
    }

    const StdStringObjectMemberIterator& operator++()
    {
        throwNotSupported();
    }

    StdStringObjectMemberIterator operator++(int)
    {
        throwNotSupported();
    }

    const StdStringObjectMemberIterator& operator--()
    {
        throwNotSupported();
    }
};

inline StdStringObjectMemberIterator StdStringObject::begin() const
{
    return {};
}

inline StdStringObjectMemberIterator StdStringObject::end() const
{
    return {};
}

inline StdStringObjectMemberIterator StdStringObject::find(const std::string &) const
{
    return {};
}

template<>
struct AdapterTraits<valijson::adapters::StdStringAdapter>
{
    typedef std::string DocumentType;

    static std::string adapterName()
    {
        return "StdStringAdapter";
    }
};

inline bool StdStringFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return StdStringAdapter(value).equalTo(other, strict);
}

}  // namespace adapters
}  // namespace valijson
#pragma once

#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace valijson {

/**
 * @brief  Class that encapsulates the storage of validation errors.
 *
 * This class maintains an internal FIFO queue of errors that are reported
 * during validation. Errors are pushed on to the back of an internal
 * queue, and can retrieved by popping them from the front of the queue.
 */
class ValidationResults
{
public:

    /**
     * @brief  Describes a validation error.
     *
     * This struct is used to pass around the context and description of a
     * validation error.
     */
    struct Error
    {
        /// Path to the node that failed validation.
        std::vector<std::string> context;

        /// A detailed description of the validation error.
        std::string description;
    };

    /**
     * @brief  Return begin iterator for results in the queue.
     */
    std::deque<Error>::const_iterator begin() const
    {
        return m_errors.begin();
    }

    /**
     * @brief  Return end iterator for results in the queue.
     */
    std::deque<Error>::const_iterator end() const
    {
        return m_errors.end();
    }

    /**
     * @brief  Return the number of errors in the queue.
     */
    size_t numErrors() const
    {
        return m_errors.size();
    }

    /**
     * @brief  Copy an Error and push it on to the back of the queue.
     *
     * @param  error  Reference to an Error object to be copied.
     */
    void pushError(const Error &error)
    {
        m_errors.push_back(error);
    }

    /**
     * @brief  Push an error onto the back of the queue.
     *
     * @param  context      Context of the validation error.
     * @param  description  Description of the validation error.
     */
    void
    pushError(const std::vector<std::string> &context, const std::string &description)
    {
        m_errors.push_back({context, description});
    }

    /**
     * @brief  Pop an error from the front of the queue.
     *
     * @param  error  Reference to an Error object to populate.
     *
     * @returns  true if an Error was popped, false otherwise.
     */
    bool
    popError(Error &error)
    {
        if (m_errors.empty()) {
            return false;
        }

        error = m_errors.front();
        m_errors.pop_front();
        return true;
    }

private:

    /// FIFO queue of validation errors that have been reported
    std::deque<Error> m_errors;
};

} // namespace valijson
#pragma once

#include <cmath>
#include <string>
#include <regex>
#include <unordered_map>

#include <utility>


#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

namespace valijson {

class ValidationResults;

/**
 * @brief   Implementation of the ConstraintVisitor interface that validates a
 *          target document
 *
 * @tparam  AdapterType  Adapter type for the target document.
 */
template<typename AdapterType>
class ValidationVisitor: public constraints::ConstraintVisitor
{
public:

    /**
     * @brief  Construct a new validator for a given target value and context.
     *
     * @param  target       Target value to be validated
     * @param  context      Current context for validation error descriptions,
     *                      only used if results is set.
     * @param  strictTypes  Use strict type comparison
     * @param  results      Optional pointer to ValidationResults object, for
     *                      recording error descriptions. If this pointer is set
     *                      to nullptr, validation errors will caused validation to
     *                      stop immediately.
     * @param  regexesCache Cache of already created std::regex objects for pattern
     *                      constraints.
     */
    ValidationVisitor(const AdapterType &target,
                      std::vector<std::string> context,
                      const bool strictTypes,
                      ValidationResults *results,
                      std::unordered_map<std::string, std::regex>& regexesCache)
      : m_target(target),
        m_context(std::move(context)),
        m_results(results),
        m_strictTypes(strictTypes),
        m_regexesCache(regexesCache) { }

    /**
     * @brief  Validate the target against a schema.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param   subschema  Sub-schema that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool validateSchema(const Subschema &subschema)
    {
        if (subschema.getAlwaysInvalid()) {
            return false;
        }

        // Wrap the validationCallback() function below so that it will be
        // passed a reference to a constraint (_1), and a reference to the
        // visitor (*this).
        Subschema::ApplyFunction fn(std::bind(validationCallback, std::placeholders::_1, std::ref(*this)));

        // Perform validation against each constraint defined in the schema
        if (m_results == nullptr) {
            // The applyStrict() function will return immediately if the
            // callback function returns false
            if (!subschema.applyStrict(fn)) {
                return false;
            }
        } else {
            // The apply() function will iterate over all constraints in the
            // schema, even if the callback function returns false. Once
            // iteration is complete, the apply() function will return true
            // only if all invokations of the callback function returned true.
            if (!subschema.apply(fn)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief  Validate a value against an AllOfConstraint
     *
     * An allOf constraint provides a set of child schemas against which the
     * target must be validated in order for the constraint to the satifisfied.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the child schemas are validated
     * successfully.
     *
     * @param  constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const AllOfConstraint &constraint) override
    {
        bool validated = true;
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, true, false, *this, m_results, nullptr, &validated));

        return validated;
    }

    /**
     * @brief   Validate a value against an AnyOfConstraint
     *
     * An anyOf constraint provides a set of child schemas, any of which the
     * target may be validated against in order for the constraint to the
     * satifisfied.
     *
     * Because an anyOf constraint does not require the target to validate
     * against all child schemas, if validation against a single schema fails,
     * the results will not be added to a ValidationResults object. Only if
     * validation fails for all child schemas will an error be added to the
     * ValidationResults object.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const AnyOfConstraint &constraint) override
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (m_results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, childResults, m_regexesCache);
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, false, true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0 && m_results) {
            ValidationResults::Error childError;
            while (childResults->popError(childError)) {
                m_results->pushError( childError.context, childError.description);
            }
            m_results->pushError(m_context, "Failed to validate against any schemas allowed by anyOf constraint.");
        }

        return numValidated > 0;
    }

    /**
     * @brief   Validate current node using a set of 'if', 'then' and 'else' subschemas
     *
     * A conditional constraint allows a document to be validated against one of two additional
     * subschemas (specified via 'then' or 'else' properties) depending on whether the document
     * satifies an optional subschema (specified via the 'if' property).
     *
     * @param   constraint  ConditionalConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const ConditionalConstraint &constraint) override
    {
        ValidationResults newResults;
        ValidationResults* conditionalResults = (m_results) ? &newResults : nullptr;

        // Create a validator to evaluate the conditional
        ValidationVisitor ifValidator(m_target, m_context, m_strictTypes, nullptr, m_regexesCache);
        ValidationVisitor thenElseValidator(m_target, m_context, m_strictTypes, conditionalResults, m_regexesCache);

        bool validated = false;
        if (ifValidator.validateSchema(*constraint.getIfSubschema())) {
            const Subschema *thenSubschema = constraint.getThenSubschema();
            validated = thenSubschema == nullptr || thenElseValidator.validateSchema(*thenSubschema);
        } else {
            const Subschema *elseSubschema = constraint.getElseSubschema();
            validated = elseSubschema == nullptr || thenElseValidator.validateSchema(*elseSubschema);
        }

        if (!validated && m_results) {
            ValidationResults::Error conditionalError;
            while (conditionalResults->popError(conditionalError)) {
                m_results->pushError(conditionalError.context, conditionalError.description);
            }
            m_results->pushError(m_context, "Failed to validate against a conditional schema set by if-then-else constraints.");
        }

        return validated;
    }

    /**
     * @brief   Validate current node using a 'const' constraint
     *
     * A const constraint allows a document to be validated against a specific value.
     *
     * @param   constraint  ConstConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \f false otherwise
     */
    bool visit(const ConstConstraint &constraint) override
    {
        if (!constraint.getValue()->equalTo(m_target, m_strictTypes)) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match expected value set by 'const' constraint.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief  Validate current node using a 'contains' constraint
     *
     * A contains constraint is satisfied if the target is not an array, or if it is an array,
     * only if it contains at least one value that matches the specified schema.
     *
     * @param   constraint  ContainsConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const ContainsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const Subschema *subschema = constraint.getSubschema();
        const typename AdapterType::Array arr = m_target.asArray();

        bool validated = false;
        for (const auto &el : arr) {
            ValidationVisitor containsValidator(el, m_context, m_strictTypes, nullptr, m_regexesCache);
            if (containsValidator.validateSchema(*subschema)) {
                validated = true;
                break;
            }
        }

        if (!validated) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to any values against subschema in 'contains' constraint.");
            }

            return false;
        }

        return validated;
    }

    /**
     * @brief   Validate current node against a 'dependencies' constraint
     *
     * A 'dependencies' constraint can be used to specify property-based or
     * schema-based dependencies that must be fulfilled when a particular
     * property is present in an object.
     *
     * Property-based dependencies define a set of properties that must be
     * present in addition to a particular property, whereas a schema-based
     * dependency defines an additional schema that the current document must
     * validate against.
     *
     * @param   constraint  DependenciesConstraint that the current node
     *                      must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const DependenciesConstraint &constraint) override
    {
        // Ignore non-objects
        if ((m_strictTypes && !m_target.isObject()) || (!m_target.maybeObject())) {
            return true;
        }

        // Object to be validated
        const typename AdapterType::Object object = m_target.asObject();

        // Cleared if validation fails
        bool validated = true;

        // Iterate over all dependent properties defined by this constraint,
        // invoking the DependentPropertyValidator functor once for each
        // set of dependent properties
        constraint.applyToPropertyDependencies(ValidatePropertyDependencies(object, m_context, m_results, &validated));
        if (!m_results && !validated) {
            return false;
        }

        // Iterate over all dependent schemas defined by this constraint,
        // invoking the DependentSchemaValidator function once for each schema
        // that must be validated if a given property is present
        constraint.applyToSchemaDependencies(ValidateSchemaDependencies(
                object, m_context, *this, m_results, &validated));
        if (!m_results && !validated) {
            return false;
        }

        return validated;
    }

    /**
     * @brief   Validate current node against an EnumConstraint
     *
     * Validation succeeds if the target is equal to one of the values provided
     * by the EnumConstraint.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const EnumConstraint &constraint) override
    {
        unsigned int numValidated = 0;
        constraint.applyToValues(
                ValidateEquality(m_target, m_context, false, true, m_strictTypes, nullptr, &numValidated));

        if (numValidated == 0) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match against any enum values.");
            }

            return false;
        }

        return numValidated > 0;
    }

    /**
     * @brief   Validate a value against a LinearItemsConstraint
     *
     * A LinearItemsConstraint represents an 'items' constraint that specifies,
     * for each item in array, an individual sub-schema that the item must
     * validate against. The LinearItemsConstraint class also captures the
     * presence of an 'additionalItems' constraint, which specifies a default
     * sub-schema that should be used if an array contains more items than
     * there are sub-schemas in the 'items' constraint.
     *
     * If the current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    bool visit(const LinearItemsConstraint &constraint) override
    {
        // Ignore values that are not arrays
        if ((m_strictTypes && !m_target.isArray()) || (!m_target.maybeArray())) {
            return true;
        }

        // Sub-schema to validate against when number of items in array exceeds
        // the number of sub-schemas provided by the 'items' constraint
        const Subschema * const additionalItemsSubschema = constraint.getAdditionalItemsSubschema();

        // Track how many items are validated using 'items' constraint
        unsigned int numValidated = 0;

        // Array to validate
        const typename AdapterType::Array arr = m_target.asArray();
        const size_t arrSize = arr.size();

        // Track validation status
        bool validated = true;

        // Validate as many items as possible using 'items' sub-schemas
        const size_t itemSubschemaCount = constraint.getItemSubschemaCount();
        if (itemSubschemaCount > 0) {
            if (!additionalItemsSubschema) {
                if (arrSize > itemSubschemaCount) {
                    if (!m_results) {
                        return false;
                    }
                    m_results->pushError(m_context, "Array contains more items than allowed by items constraint.");
                    validated = false;
                }
            }

            constraint.applyToItemSubschemas(
                    ValidateItems(arr, m_context, true, m_results != nullptr, m_strictTypes, m_results, &numValidated,
                            &validated, m_regexesCache));

            if (!m_results && !validated) {
                return false;
            }
        }

        // Validate remaining items using 'additionalItems' sub-schema
        if (numValidated < arrSize) {
            if (additionalItemsSubschema) {
                // Begin validation from the first item not validated against
                // an sub-schema provided by the 'items' constraint
                unsigned int index = numValidated;
                typename AdapterType::Array::const_iterator begin = arr.begin();
                begin.advance(numValidated);
                for (typename AdapterType::Array::const_iterator itr = begin;
                        itr != arr.end(); ++itr) {

                    // Update context for current array item
                    std::vector<std::string> newContext = m_context;
                    newContext.push_back("[" + std::to_string(index) + "]");

                    ValidationVisitor<AdapterType> validator(*itr, newContext, m_strictTypes, m_results, m_regexesCache);

                    if (!validator.validateSchema(*additionalItemsSubschema)) {
                        if (m_results) {
                            m_results->pushError(m_context, "Failed to validate item #" + std::to_string(index) +
                                    " against additional items schema.");
                            validated = false;
                        } else {
                            return false;
                        }
                    }

                    index++;
                }

            } else if (m_results) {
                m_results->pushError(m_context, "Cannot validate item #" + std::to_string(numValidated) +
                        " or greater using 'items' constraint or 'additionalItems' constraint.");
                validated = false;

            } else {
                return false;
            }
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a MaximumConstraint object
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraints are satisfied; \c false otherwise
     */
    bool visit(const MaximumConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isNumber()) || !m_target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double maximum = constraint.getMaximum();

        if (constraint.getExclusiveMaximum()) {
            if (m_target.asDouble() >= maximum) {
                if (m_results) {
                    m_results->pushError(m_context, "Expected number less than " + std::to_string(maximum));
                }

                return false;
            }

        } else if (m_target.asDouble() > maximum) {
            if (m_results) {
                m_results->pushError(m_context, "Expected number less than or equal to " + std::to_string(maximum));
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MaxItemsConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxItemsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const uint64_t maxItems = constraint.getMaxItems();
        if (m_target.asArray().size() <= maxItems) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Array should contain no more than " + std::to_string(maxItems) +
                    " elements.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MaxLengthConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxLengthConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        const std::string s = m_target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t maxLength = constraint.getMaxLength();
        if (len <= maxLength) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "String should be no more than " + std::to_string(maxLength) +
                    " characters in length.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MaxPropertiesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxPropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        const uint64_t maxProperties = constraint.getMaxProperties();

        if (m_target.asObject().size() <= maxProperties) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Object should have no more than " + std::to_string(maxProperties) +
                    " properties.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinimumConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinimumConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isNumber()) || !m_target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double minimum = constraint.getMinimum();

        if (constraint.getExclusiveMinimum()) {
            if (m_target.asDouble() <= minimum) {
                if (m_results) {
                    m_results->pushError(m_context, "Expected number greater than " + std::to_string(minimum));
                }

                return false;
            }
        } else if (m_target.asDouble() < minimum) {
            if (m_results) {
                m_results->pushError(m_context, "Expected number greater than or equal to " + std::to_string(minimum));
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MinItemsConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinItemsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const uint64_t minItems = constraint.getMinItems();
        if (m_target.asArray().size() >= minItems) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Array should contain no fewer than " + std::to_string(minItems) +
                    " elements.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinLengthConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinLengthConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        const std::string s = m_target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t minLength = constraint.getMinLength();
        if (len >= minLength) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "String should be no fewer than " + std::to_string(minLength) +
                    " characters in length.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinPropertiesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinPropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        const uint64_t minProperties = constraint.getMinProperties();

        if (m_target.asObject().size() >= minProperties) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Object should have no fewer than " + std::to_string(minProperties) +
                    " properties.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MultipleOfDoubleConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MultipleOfDoubleConstraint &constraint) override
    {
        const double divisor = constraint.getDivisor();

        double d = 0.;
        if (m_target.maybeDouble()) {
            if (!m_target.asDouble(d)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted "
                            "to a number to check if it is a multiple of " + std::to_string(divisor));
                }
                return false;
            }
        } else if (m_target.maybeInteger()) {
            int64_t i = 0;
            if (!m_target.asInteger(i)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted "
                            "to a number to check if it is a multiple of " + std::to_string(divisor));
                }
                return false;
            }
            d = static_cast<double>(i);
        } else {
            return true;
        }

        if (d == 0) {
            return true;
        }

        const double r = remainder(d, divisor);

        if (fabs(r) > std::numeric_limits<double>::epsilon()) {
            if (m_results) {
                m_results->pushError(m_context, "Value should be a multiple of " + std::to_string(divisor));
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MultipleOfIntConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MultipleOfIntConstraint &constraint) override
    {
        const int64_t divisor = constraint.getDivisor();

        int64_t i = 0;
        if (m_target.maybeInteger()) {
            if (!m_target.asInteger(i)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted to an integer for multipleOf check");
                }
                return false;
            }
        } else if (m_target.maybeDouble()) {
            double d;
            if (!m_target.asDouble(d)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted to a double for multipleOf check");
                }
                return false;
            }
            i = static_cast<int64_t>(d);
        } else {
            return true;
        }

        if (i == 0) {
            return true;
        }

        if (i % divisor != 0) {
            if (m_results) {
                m_results->pushError(m_context, "Value should be a multiple of " + std::to_string(divisor));
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a NotConstraint
     *
     * If the subschema NotConstraint currently holds a nullptr, the
     * schema will be treated like the empty schema. Therefore validation
     * will always fail.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const NotConstraint &constraint) override
    {
        const Subschema *subschema = constraint.getSubschema();
        if (!subschema) {
            // Treat nullptr like empty schema
            return false;
        }

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, nullptr, m_regexesCache);
        if (v.validateSchema(*subschema)) {
            if (m_results) {
                m_results->pushError(m_context,
                        "Target should not validate against schema specified in 'not' constraint.");
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a OneOfConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const OneOfConstraint &constraint) override
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (m_results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, childResults, m_regexesCache);
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, true, true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0) {
            if (m_results) {
                ValidationResults::Error childError;
                while (childResults->popError(childError)) {
                    m_results->pushError(
                            childError.context,
                            childError.description);
                }
                m_results->pushError(m_context, "Failed to validate against any "
                        "child schemas allowed by oneOf constraint.");
            }
            return false;
        } else if (numValidated != 1) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to validate against exactly one child schema.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a PatternConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const PatternConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        std::string pattern(constraint.getPattern<std::string::allocator_type>());
        auto it = m_regexesCache.find(pattern);
        if (it == m_regexesCache.end()) {
            it = m_regexesCache.emplace(pattern, std::regex(pattern)).first;
        }

        if (!std::regex_search(m_target.asString(), it->second)) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match regex specified by 'pattern' constraint.");
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a PatternConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const constraints::PolyConstraint &constraint) override
    {
        return constraint.validate(m_target, m_context, m_results);
    }

    /**
     * @brief   Validate a value against a PropertiesConstraint
     *
     * Validation of an object against a PropertiesConstraint proceeds in three
     * stages. The first stage finds all properties in the object that have a
     * corresponding subschema in the constraint, and validates those properties
     * recursively.
     *
     * Next, the object's properties will be validated against the subschemas
     * for any 'patternProperties' that match a given property name. A property
     * is required to validate against the sub-schema for all patterns that it
     * matches.
     *
     * Finally, any properties that have not yet been validated against at least
     * one subschema will be validated against the 'additionalItems' subschema.
     * If this subschema is not present, then all properties must have been
     * validated at least once.
     *
     * Non-object values are always considered valid.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const PropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        bool validated = true;

        // Track which properties have already been validated
        std::set<std::string> propertiesMatched;

        // Validate properties against subschemas for matching 'properties'
        // constraints
        const typename AdapterType::Object object = m_target.asObject();
        constraint.applyToProperties(
                ValidatePropertySubschemas(
                        object, m_context, true, m_results != nullptr, true, m_strictTypes, m_results,
                        &propertiesMatched, &validated, m_regexesCache));

        // Exit early if validation failed, and we're not collecting exhaustive
        // validation results
        if (!validated && !m_results) {
            return false;
        }

        // Validate properties against subschemas for matching patternProperties
        // constraints
        constraint.applyToPatternProperties(
                ValidatePatternPropertySubschemas(
                        object, m_context, true, false, true, m_strictTypes, m_results, &propertiesMatched,
                        &validated, m_regexesCache));

        // Validate against additionalProperties subschema for any properties
        // that have not yet been matched
        const Subschema *additionalPropertiesSubschema =
                constraint.getAdditionalPropertiesSubschema();
        if (!additionalPropertiesSubschema) {
            if (propertiesMatched.size() != m_target.getObjectSize()) {
                if (m_results) {
                    std::string unwanted;
                    for (const typename AdapterType::ObjectMember m : object) {
                        if (propertiesMatched.find(m.first) == propertiesMatched.end()) {
                            unwanted = m.first;
                            break;
                        }
                    }
                    m_results->pushError(m_context, "Object contains a property "
                            "that could not be validated using 'properties' "
                            "or 'additionalProperties' constraints: '" + unwanted + "'.");
                }

                return false;
            }

            return validated;
        }

        for (const typename AdapterType::ObjectMember m : object) {
            if (propertiesMatched.find(m.first) == propertiesMatched.end()) {
                // Update context
                std::vector<std::string> newContext = m_context;
                newContext.push_back("[" + m.first + "]");

                // Create a validator to validate the property's value
                ValidationVisitor validator(m.second, newContext, m_strictTypes, m_results, m_regexesCache);
                if (!validator.validateSchema(*additionalPropertiesSubschema)) {
                    if (m_results) {
                        m_results->pushError(m_context, "Failed to validate against additional properties schema");
                    }

                    validated = false;
                }
            }
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a PropertyNamesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const PropertyNamesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        for (const typename AdapterType::ObjectMember m : m_target.asObject()) {
            adapters::StdStringAdapter stringAdapter(m.first);
            ValidationVisitor<adapters::StdStringAdapter> validator(stringAdapter, m_context, m_strictTypes, nullptr, m_regexesCache);
            if (!validator.validateSchema(*constraint.getSubschema())) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate a value against a RequiredConstraint
     *
     * A required constraint specifies a list of properties that must be present
     * in the target.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const RequiredConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        bool validated = true;
        const typename AdapterType::Object object = m_target.asObject();
        constraint.applyToRequiredProperties(
                ValidateProperties(object, m_context, true, m_results != nullptr, m_results, &validated));

        return validated;
    }

    /**
     * @brief  Validate a value against a SingularItemsConstraint
     *
     * A SingularItemsConstraint represents an 'items' constraint that specifies
     * a sub-schema against which all items in an array must validate. If the
     * current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    bool visit(const SingularItemsConstraint &constraint) override
    {
        // Ignore values that are not arrays
        if (!m_target.isArray()) {
            return true;
        }

        // Schema against which all items must validate
        const Subschema *itemsSubschema = constraint.getItemsSubschema();

        // Default items sub-schema accepts all values
        if (!itemsSubschema) {
            return true;
        }

        // Track whether validation has failed
        bool validated = true;

        unsigned int index = 0;
        for (const AdapterType &item : m_target.getArray()) {
            // Update context for current array item
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + std::to_string(index) + "]");

            // Create a validator for the current array item
            ValidationVisitor<AdapterType> validationVisitor(item, newContext, m_strictTypes, m_results, m_regexesCache);

            // Perform validation
            if (!validationVisitor.validateSchema(*itemsSubschema)) {
                if (m_results) {
                    m_results->pushError(m_context, "Failed to validate item #" + std::to_string(index) + " in array.");
                    validated = false;
                } else {
                    return false;
                }
            }

            index++;
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a TypeConstraint
     *
     * Checks that the target is one of the valid named types, or matches one
     * of a set of valid sub-schemas.
     *
     * @param   constraint  TypeConstraint to validate against
     *
     * @return  \c true if validation is successful; \c false otherwise
     */
    bool visit(const TypeConstraint &constraint) override
    {
        // Check named types
        {
            // ValidateNamedTypes functor assumes target is invalid
            bool validated = false;
            constraint.applyToNamedTypes(ValidateNamedTypes(m_target, false, true, m_strictTypes, &validated));
            if (validated) {
                return true;
            }
        }

        // Check schema-based types
        {
            unsigned int numValidated = 0;
            constraint.applyToSchemaTypes(
                    ValidateSubschemas(m_target, m_context, false, true, *this, nullptr, &numValidated, nullptr));
            if (numValidated > 0) {
                return true;
            } else if (m_results) {
                m_results->pushError(m_context, "Value type not permitted by 'type' constraint.");
            }
        }

        return false;
    }

    /**
     * @brief   Validate the uniqueItems constraint represented by a
     *          UniqueItems object.
     *
     * A uniqueItems constraint requires that each of the values in an array
     * are unique. Comparison is performed recursively.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  true if validation succeeds, false otherwise
     */
    bool visit(const UniqueItemsConstraint &) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        // Empty arrays are always valid
        if (m_target.getArraySize() == 0) {
            return true;
        }

        bool validated = true;

        const typename AdapterType::Array targetArray = m_target.asArray();
        const typename AdapterType::Array::const_iterator end = targetArray.end();
        const typename AdapterType::Array::const_iterator secondLast = --targetArray.end();
        unsigned int outerIndex = 0;
        typename AdapterType::Array::const_iterator outerItr = targetArray.begin();
        for (; outerItr != secondLast; ++outerItr) {
            unsigned int innerIndex = outerIndex + 1;
            typename AdapterType::Array::const_iterator innerItr(outerItr);
            for (++innerItr; innerItr != end; ++innerItr) {
                if (outerItr->equalTo(*innerItr, true)) {
                    if (!m_results) {
                        return false;
                    }
                    m_results->pushError(m_context, "Elements at indexes #" + std::to_string(outerIndex)
                        + " and #" + std::to_string(innerIndex) + " violate uniqueness constraint.");
                    validated = false;
                }
                ++innerIndex;
            }
            ++outerIndex;
        }

        return validated;
    }

private:

    /**
     * @brief  Functor to compare a node with a collection of values
     */
    struct ValidateEquality
    {
        ValidateEquality(
                const AdapterType &target,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                ValidationResults *results,
                unsigned int *numValidated)
          : m_target(target),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_results(results),
            m_numValidated(numValidated) { }

        template<typename OtherValue>
        bool operator()(const OtherValue &value) const
        {
            if (value.equalTo(m_target, m_strictTypes)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_results) {
                m_results->pushError(m_context, "Target value and comparison value are not equal");
            }

            return m_continueOnFailure;
        }

    private:
        const AdapterType &m_target;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        bool m_strictTypes;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
    };

    /**
     * @brief  Functor to validate the presence of a set of properties
     */
    struct ValidateProperties
    {
        ValidateProperties(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &property) const
        {
            if (m_object.find(property.c_str()) == m_object.end()) {
                if (m_validated) {
                    *m_validated = false;
                }

                if (m_results) {
                    m_results->pushError(m_context, "Missing required property '" +
                            std::string(property.c_str()) + "'.");
                }

                return m_continueOnFailure;
            }

            return m_continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate property-based dependencies
     */
    struct ValidatePropertyDependencies
    {
        ValidatePropertyDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType, typename ContainerType>
        bool operator()(const StringType &propertyName, const ContainerType &dependencyNames) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (m_object.find(propertyNameKey) == m_object.end()) {
                return true;
            }

            typedef typename ContainerType::value_type ValueType;
            for (const ValueType &dependencyName : dependencyNames) {
                const std::string dependencyNameKey(dependencyName.c_str());
                if (m_object.find(dependencyNameKey) == m_object.end()) {
                    if (m_validated) {
                        *m_validated = false;
                    }
                    if (m_results) {
                        m_results->pushError(m_context, "Missing dependency '" + dependencyNameKey + "'.");
                    } else {
                        return false;
                    }
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate against sub-schemas in 'items' constraint
     */
    struct ValidateItems
    {
        ValidateItems(
                const typename AdapterType::Array &arr,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_arr(arr),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_results(results),
            m_numValidated(numValidated),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            // Check that there are more elements to validate
            if (index >= m_arr.size()) {
                return false;
            }

            // Update context
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + std::to_string(index) + "]");

            // Find array item
            typename AdapterType::Array::const_iterator itr = m_arr.begin();
            itr.advance(index);

            // Validate current array item
            ValidationVisitor validator(*itr, newContext, m_strictTypes, m_results, m_regexesCache);
            if (validator.validateSchema(*subschema)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_validated) {
                *m_validated = false;
            }

            if (m_results) {
                m_results->pushError(newContext, "Failed to validate item #" + std::to_string(index) +
                    " against corresponding item schema.");
            }

            return m_continueOnFailure;
        }

    private:
        const typename AdapterType::Array &m_arr;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        bool m_strictTypes;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate value against named JSON types
     */
    struct ValidateNamedTypes
    {
        ValidateNamedTypes(
                const AdapterType &target,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                bool *validated)
          : m_target(target),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_validated(validated) { }

        bool operator()(constraints::TypeConstraint::JsonType jsonType) const
        {
            typedef constraints::TypeConstraint TypeConstraint;

            bool valid = false;

            switch (jsonType) {
            case TypeConstraint::kAny:
                valid = true;
                break;
            case TypeConstraint::kArray:
                valid = m_target.isArray();
                break;
            case TypeConstraint::kBoolean:
                valid = m_target.isBool() || (!m_strictTypes && m_target.maybeBool());
                break;
            case TypeConstraint::kInteger:
                valid = m_target.isInteger() || (!m_strictTypes && m_target.maybeInteger());
                break;
            case TypeConstraint::kNull:
                valid = m_target.isNull() || (!m_strictTypes && m_target.maybeNull());
                break;
            case TypeConstraint::kNumber:
                valid = m_target.isNumber() || (!m_strictTypes && m_target.maybeDouble());
                break;
            case TypeConstraint::kObject:
                valid = m_target.isObject();
                break;
            case TypeConstraint::kString:
                valid = m_target.isString();
                break;
            default:
                break;
            }

            if (valid && m_validated) {
                *m_validated = true;
            }

            return (valid && m_continueOnSuccess) || m_continueOnFailure;
        }

    private:
        const AdapterType m_target;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_strictTypes;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate object properties against sub-schemas
     *         defined by a 'patternProperties' constraint
     */
    struct ValidatePatternPropertySubschemas
    {
        ValidatePatternPropertySubschemas(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool continueIfUnmatched,
                bool strictTypes,
                ValidationResults *results,
                std::set<std::string> *propertiesMatched,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_continueIfUnmatched(continueIfUnmatched),
            m_strictTypes(strictTypes),
            m_results(results),
            m_propertiesMatched(propertiesMatched),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        template<typename StringType>
        bool operator()(const StringType &patternProperty, const Subschema *subschema) const
        {
            const std::string patternPropertyStr(patternProperty.c_str());

            // It would be nice to store pre-allocated regex objects in the
            // PropertiesConstraint. does std::regex currently support
            // custom allocators? Anyway, this isn't an issue here, because Valijson's
            // JSON Scheme validator does not yet support custom allocators.
            const std::regex r(patternPropertyStr);

            bool matchFound = false;

            // Recursively validate all matching properties
            typedef const typename AdapterType::ObjectMember ObjectMember;
            for (const ObjectMember m : m_object) {
                if (std::regex_search(m.first, r)) {
                    matchFound = true;
                    if (m_propertiesMatched) {
                        m_propertiesMatched->insert(m.first);
                    }

                    // Update context
                    std::vector<std::string> newContext = m_context;
                    newContext.push_back("[" + m.first + "]");

                    // Recursively validate property's value
                    ValidationVisitor validator(m.second, newContext, m_strictTypes, m_results, m_regexesCache);
                    if (validator.validateSchema(*subschema)) {
                        continue;
                    }

                    if (m_results) {
                        m_results->pushError(m_context, "Failed to validate against schema associated with pattern '" +
                                patternPropertyStr + "'.");
                    }

                    if (m_validated) {
                        *m_validated = false;
                    }

                    if (!m_continueOnFailure) {
                        return false;
                    }
                }
            }

            // Allow iteration to terminate if there was not at least one match
            if (!matchFound && !m_continueIfUnmatched) {
                return false;
            }

            return m_continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_continueIfUnmatched;
        const bool m_strictTypes;
        ValidationResults * const m_results;
        std::set<std::string> * const m_propertiesMatched;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate object properties against sub-schemas defined
     *         by a 'properties' constraint
     */
    struct ValidatePropertySubschemas
    {
        ValidatePropertySubschemas(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool continueIfUnmatched,
                bool strictTypes,
                ValidationResults *results,
                std::set<std::string> *propertiesMatched,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_continueIfUnmatched(continueIfUnmatched),
            m_strictTypes(strictTypes),
            m_results(results),
            m_propertiesMatched(propertiesMatched),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        template<typename StringType>
        bool operator()(const StringType &propertyName, const Subschema *subschema) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            const typename AdapterType::Object::const_iterator itr = m_object.find(propertyNameKey);
            if (itr == m_object.end()) {
                return m_continueIfUnmatched;
            }

            if (m_propertiesMatched) {
                m_propertiesMatched->insert(propertyNameKey);
            }

            // Update context
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + propertyNameKey + "]");

            // Recursively validate property's value
            ValidationVisitor validator(itr->second, newContext, m_strictTypes, m_results, m_regexesCache);
            if (validator.validateSchema(*subschema)) {
                return m_continueOnSuccess;
            }

            if (m_results) {
                m_results->pushError(m_context, "Failed to validate against schema associated with property name '" +
                        propertyNameKey + "'.");
            }

            if (m_validated) {
                *m_validated = false;
            }

            return m_continueOnFailure;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_continueIfUnmatched;
        const bool m_strictTypes;
        ValidationResults * const m_results;
        std::set<std::string> * const m_propertiesMatched;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate schema-based dependencies
     */
    struct ValidateSchemaDependencies
    {
        ValidateSchemaDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_validationVisitor(validationVisitor),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &propertyName, const Subschema *schemaDependency) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (m_object.find(propertyNameKey) == m_object.end()) {
                return true;
            }

            if (!m_validationVisitor.validateSchema(*schemaDependency)) {
                if (m_validated) {
                    *m_validated = false;
                }
                if (m_results) {
                    m_results->pushError(m_context, "Failed to validate against dependent schema.");
                } else {
                    return false;
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        ValidationVisitor &m_validationVisitor;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor that can be used to validate one or more subschemas
     *
     * This functor is designed to be applied to collections of subschemas
     * contained within 'allOf', 'anyOf' and 'oneOf' constraints.
     *
     * The return value depends on whether a given schema validates, with the
     * actual return value for a given case being decided at construction time.
     * The return value is used by the 'applyToSubschemas' functions in the
     * AllOfConstraint, AnyOfConstraint and OneOfConstrant classes to decide
     * whether to terminate early.
     *
     * The functor uses output parameters (provided at construction) to update
     * validation state that may be needed by the caller.
     */
    struct ValidateSubschemas
    {
        ValidateSubschemas(
                const AdapterType &adapter,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated)
          : m_adapter(adapter),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_validationVisitor(validationVisitor),
            m_results(results),
            m_numValidated(numValidated),
            m_validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            if (m_validationVisitor.validateSchema(*subschema)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_validated) {
                *m_validated = false;
            }

            if (m_results) {
                m_results->pushError(m_context,
                        "Failed to validate against child schema #" + std::to_string(index) + ".");
            }

            return m_continueOnFailure;
        }

    private:
        const AdapterType &m_adapter;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        ValidationVisitor &m_validationVisitor;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
        bool * const m_validated;
    };

    /**
     * @brief  Callback function that passes a visitor to a constraint.
     *
     * @param  constraint  Reference to constraint to be visited
     * @param  visitor     Reference to visitor to be applied
     *
     * @return  true if the visitor returns successfully, false otherwise.
     */
    static bool validationCallback(const constraints::Constraint &constraint, ValidationVisitor<AdapterType> &visitor)
    {
        return constraint.accept(visitor);
    }

    /// The JSON value being validated
    AdapterType m_target;

    /// Vector of strings describing the current object context
    std::vector<std::string> m_context;

    /// Optional pointer to a ValidationResults object to be populated
    ValidationResults *m_results;

    /// Option to use strict type comparison
    bool m_strictTypes;

    /// Cached regex objects for pattern constraint
    std::unordered_map<std::string, std::regex>& m_regexesCache;
};

}  // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif
#pragma once


namespace valijson {

class Schema;
class ValidationResults;

/**
 * @brief  Class that provides validation functionality.
 */
class Validator
{
public:
    enum TypeCheckingMode
    {
        kStrongTypes,
        kWeakTypes
    };

    /**
     * @brief  Construct a Validator that uses strong type checking by default
     */
    Validator()
      : strictTypes(true) { }

    /**
     * @brief  Construct a Validator using a specific type checking mode
     *
     * @param  typeCheckingMode  choice of strong or weak type checking
     */
    Validator(TypeCheckingMode typeCheckingMode)
      : strictTypes(typeCheckingMode == kStrongTypes) { }

    /**
     * @brief  Validate a JSON document and optionally return the results.
     *
     * When a ValidationResults object is provided via the \c results parameter,
     * validation will be performed against each constraint defined by the
     * schema, even if validation fails for some or all constraints.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param  schema   The schema to validate against
     * @param  target   A rapidjson::Value to be validated
     *
     * @param  results  An optional pointer to a ValidationResults instance that
     *                  will be used to report validation errors
     *
     * @returns  true if validation succeeds, false otherwise
     */
    template<typename AdapterType>
    bool validate(const Subschema &schema, const AdapterType &target,
            ValidationResults *results)
    {
        // Construct a ValidationVisitor to perform validation at the root level
        ValidationVisitor<AdapterType> v(target,
                std::vector<std::string>(1, "<root>"), strictTypes, results, regexesCache);

        return v.validateSchema(schema);
    }

private:

    /// Flag indicating that strict type comparisons should be used
    bool strictTypes;

    /// Cached regex objects for pattern constraint. Key - pattern.
    std::unordered_map<std::string, std::regex> regexesCache;
};

}  // namespace valijson
/**
 * @file
 *
 * @brief   Adapter implementation for the nlohmann json parser library.
 *
 * Include this file in your program to enable support for nlohmann json.
 *
 * This file defines the following classes (not in this order):
 *  - NlohmannJsonAdapter
 *  - NlohmannJsonArray
 *  - NlohmannJsonValueIterator
 *  - NlohmannJsonFrozenValue
 *  - NlohmannJsonObject
 *  - NlohmannJsonObjectMember
 *  - NlohmannJsonObjectMemberIterator
 *  - NlohmannJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is NlohmannJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include <utility>

namespace valijson {
namespace adapters {

class NlohmannJsonAdapter;
class NlohmannJsonArrayValueIterator;
class NlohmannJsonObjectMemberIterator;

typedef std::pair<std::string, NlohmannJsonAdapter> NlohmannJsonObjectMember;

/**
 * @brief  Light weight wrapper for a NlohmannJson array value.
 *
 * This class is light weight wrapper for a NlohmannJson array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * NlohmannJson value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class NlohmannJsonArray
{
public:

    typedef NlohmannJsonArrayValueIterator const_iterator;
    typedef NlohmannJsonArrayValueIterator iterator;

    /// Construct a NlohmannJsonArray referencing an empty array.
    NlohmannJsonArray()
      : m_value(emptyArray()) { }

    /**
     * @brief   Construct a NlohmannJsonArray referencing a specific NlohmannJson
     *          value.
     *
     * @param   value   reference to a NlohmannJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    NlohmannJsonArray(const nlohmann::json &value)
      : m_value(value)
    {
        if (!value.is_array()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying NlohmannJson implementation.
     */
    NlohmannJsonArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying NlohmannJson implementation.
     */
    NlohmannJsonArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to a NlohmannJson value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const nlohmann::json & emptyArray()
    {
        static const nlohmann::json array = nlohmann::json::array();
        return array;
    }

    /// Reference to the contained value
    const nlohmann::json &m_value;
};

/**
 * @brief  Light weight wrapper for a NlohmannJson object.
 *
 * This class is light weight wrapper for a NlohmannJson object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * NlohmannJson value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class NlohmannJsonObject
{
public:

    typedef NlohmannJsonObjectMemberIterator const_iterator;
    typedef NlohmannJsonObjectMemberIterator iterator;

    /// Construct a NlohmannJsonObject referencing an empty object singleton.
    NlohmannJsonObject()
      : m_value(emptyObject()) { }

    /**
     * @brief   Construct a NlohmannJsonObject referencing a specific NlohmannJson
     *          value.
     *
     * @param   value  reference to a NlohmannJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    NlohmannJsonObject(const nlohmann::json &value)
      : m_value(value)
    {
        if (!value.is_object()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying NlohmannJson implementation.
     */
    NlohmannJsonObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying NlohmannJson implementation.
     */
    NlohmannJsonObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    NlohmannJsonObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to a NlohmannJson value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const nlohmann::json & emptyObject()
    {
        static const nlohmann::json object = nlohmann::json::object();
        return object;
    }

    /// Reference to the contained object
    const nlohmann::json &m_value;
};


/**
 * @brief   Stores an independent copy of a NlohmannJson value.
 *
 * This class allows a NlohmannJson value to be stored independent of its original
 * document. NlohmannJson makes this easy to do, as it does not perform any
 * custom memory management.
 *
 * @see FrozenValue
 */
class NlohmannJsonFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a NlohmannJson value
     *
     * @param  source  the NlohmannJson value to be copied
     */
    explicit NlohmannJsonFrozenValue(nlohmann::json source)
      : m_value(std::move(source)) { }

    FrozenValue * clone() const override
    {
        return new NlohmannJsonFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored NlohmannJson value
    nlohmann::json m_value;
};


/**
 * @brief   Light weight wrapper for a NlohmannJson value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a NlohmannJson value. This class is responsible
 * for the mechanics of actually reading a NlohmannJson value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class NlohmannJsonValue
{
public:

    /// Construct a wrapper for the empty object singleton
    NlohmannJsonValue()
      : m_value(emptyObject()) { }

    /// Construct a wrapper for a specific NlohmannJson value
    NlohmannJsonValue(const nlohmann::json &value)
      : m_value(value) { }

    /**
     * @brief   Create a new NlohmannJsonFrozenValue instance that contains the
     *          value referenced by this NlohmannJsonValue instance.
     *
     * @returns pointer to a new NlohmannJsonFrozenValue instance, belonging to the
     *          caller.
     */
    FrozenValue * freeze() const
    {
        return new NlohmannJsonFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a NlohmannJsonArray instance.
     *
     * If the referenced NlohmannJson value is an array, this function will return
     * a std::optional containing a NlohmannJsonArray instance referencing the
     * array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<NlohmannJsonArray> getArrayOptional() const
    {
        if (m_value.is_array()) {
            return opt::make_optional(NlohmannJsonArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced NlohmannJson value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.is_array()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.is_boolean()) {
            result = m_value.get<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.is_number_float()) {
            result = m_value.get<double>();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if(m_value.is_number_integer()) {
            result = m_value.get<int64_t>();
            return true;
        }
        return false;
    }

    /**
     * @brief   Optionally return a NlohmannJsonObject instance.
     *
     * If the referenced NlohmannJson value is an object, this function will return a
     * std::optional containing a NlohmannJsonObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<NlohmannJsonObject> getObjectOptional() const
    {
        if (m_value.is_object()) {
            return opt::make_optional(NlohmannJsonObject(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced NlohmannJson value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.is_object()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.is_string()) {
            result = m_value.get<std::string>();
            return true;
        }

        return false;
    }

    static bool hasStrictTypes()
    {
        return true;
    }

    bool isArray() const
    {
        return m_value.is_array();
    }

    bool isBool() const
    {
        return m_value.is_boolean();
    }

    bool isDouble() const
    {
        return m_value.is_number_float();
    }

    bool isInteger() const
    {
        return m_value.is_number_integer();
    }

    bool isNull() const
    {
        return m_value.is_null();
    }

    bool isNumber() const
    {
        return m_value.is_number();
    }

    bool isObject() const
    {
        return m_value.is_object();
    }

    bool isString() const
    {
        return m_value.is_string();
    }

private:

    /// Return a reference to an empty object singleton
    static const nlohmann::json & emptyObject()
    {
        static const nlohmann::json object = nlohmann::json::object();
        return object;
    }

    /// Reference to the contained NlohmannJson value.
    const nlohmann::json &m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting NlohmannJson.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class NlohmannJsonAdapter:
    public BasicAdapter<NlohmannJsonAdapter,
        NlohmannJsonArray,
        NlohmannJsonObjectMember,
        NlohmannJsonObject,
        NlohmannJsonValue>
{
public:
    /// Construct a NlohmannJsonAdapter that contains an empty object
    NlohmannJsonAdapter()
      : BasicAdapter() { }

    /// Construct a NlohmannJsonAdapter containing a specific Nlohmann Json object
    NlohmannJsonAdapter(const nlohmann::json &value)
      : BasicAdapter(NlohmannJsonValue{value}) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * NlohmannJsonAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see NlohmannJsonArray
 */
class NlohmannJsonArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = NlohmannJsonAdapter;
    using difference_type = NlohmannJsonAdapter;
    using pointer = NlohmannJsonAdapter*;
    using reference = NlohmannJsonAdapter&;

    /**
     * @brief   Construct a new NlohmannJsonArrayValueIterator using an existing
     *          NlohmannJson iterator.
     *
     * @param   itr  NlohmannJson iterator to store
     */
    NlohmannJsonArrayValueIterator(const nlohmann::json::const_iterator &itr)
      : m_itr(itr) { }

    /// Returns a NlohmannJsonAdapter that contains the value of the current
    /// element.
    NlohmannJsonAdapter operator*() const
    {
        return NlohmannJsonAdapter(*m_itr);
    }

    DerefProxy<NlohmannJsonAdapter> operator->() const
    {
        return DerefProxy<NlohmannJsonAdapter>(**this);
    }

    /**
     * @brief   Compare this iterator against another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   other  iterator to compare against
     *
     * @returns true   if the iterators are equal, false otherwise.
     */
    bool operator==(const NlohmannJsonArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const NlohmannJsonArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const NlohmannJsonArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    NlohmannJsonArrayValueIterator operator++(int)
    {
        NlohmannJsonArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const NlohmannJsonArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

private:
    nlohmann::json::const_iterator m_itr;
};


/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of NlohmannJsonObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see NlohmannJsonObject
 * @see NlohmannJsonObjectMember
 */
class NlohmannJsonObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = NlohmannJsonObjectMember;
    using difference_type = NlohmannJsonObjectMember;
    using pointer = NlohmannJsonObjectMember*;
    using reference = NlohmannJsonObjectMember&;

    /**
     * @brief   Construct an iterator from a NlohmannJson iterator.
     *
     * @param   itr  NlohmannJson iterator to store
     */
    NlohmannJsonObjectMemberIterator(const nlohmann::json::const_iterator &itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a NlohmannJsonObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    NlohmannJsonObjectMember operator*() const
    {
        return NlohmannJsonObjectMember(m_itr.key(), m_itr.value());
    }

    DerefProxy<NlohmannJsonObjectMember> operator->() const
    {
        return DerefProxy<NlohmannJsonObjectMember>(**this);
    }

    /**
     * @brief   Compare this iterator with another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   other  Iterator to compare with
     *
     * @returns true if the underlying iterators are equal, false otherwise
     */
    bool operator==(const NlohmannJsonObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const NlohmannJsonObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const NlohmannJsonObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    NlohmannJsonObjectMemberIterator operator++(int)
    {
        NlohmannJsonObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const NlohmannJsonObjectMemberIterator& operator--()
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original NlohmannJson iterator
    nlohmann::json::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for NlohmannJsonAdapter.
template<>
struct AdapterTraits<valijson::adapters::NlohmannJsonAdapter>
{
    typedef nlohmann::json DocumentType;

    static std::string adapterName()
    {
        return "NlohmannJsonAdapter";
    }
};

inline bool NlohmannJsonFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return NlohmannJsonAdapter(m_value).equalTo(other, strict);
}

inline NlohmannJsonArrayValueIterator NlohmannJsonArray::begin() const
{
    return m_value.begin();
}

inline NlohmannJsonArrayValueIterator NlohmannJsonArray::end() const
{
    return m_value.end();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::begin() const
{
    return m_value.begin();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::end() const
{
    return m_value.end();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::find(
        const std::string &propertyName) const
{
    return m_value.find(propertyName);
}

}  // namespace adapters
}  // namespace valijson
#pragma once

#include <iostream>

#include <nlohmann/json.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, nlohmann::json &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'."
                  << std::endl;
        return false;
    }

    // Parse schema
#if VALIJSON_USE_EXCEPTION
    try {
        document = nlohmann::json::parse(file);
    } catch (std::invalid_argument const& exception) {
        std::cerr << "nlohmann::json failed to parse the document\n"
            << "Parse error:" << exception.what() << "\n";
        return false;
    }
#else
    document = nlohmann::json::parse(file, nullptr, false);
    if (document.is_discarded()) {
        std::cerr << "nlohmann::json failed to parse the document.";
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson

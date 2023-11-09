//
// option_set.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_OPTION_SET_HPP
#define URDL_OPTION_SET_HPP

#include <typeinfo>
#include "urdl/detail/config.hpp"
#include "urdl/detail/scoped_ptr.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {

/// The class @c option_set maintains a collection of options.
/**
 * @par Remarks
 * Options are uniquely identified by type, so the @c option_set class is a
 * collection of objects of differing types, indexed by type.
 *
 * The option types stored in the set must meet the type requirements for
 * CopyConstructible.
 *
 * @par Requirements
 * @e Header: @c <urdl/option_set.hpp> @n
 * @e Namespace: @c urdl
 */
class option_set
{
public:
  /// Constructs an object of class @c option_set.
  /**
   * @par Remarks
   * Creates an empty set. Any option queried using the @c get_option member
   * function will return the default value of the option.
   */
  URDL_DECL option_set();

  /// Constructs an object of class @c option_set.
  /**
   * @par Remarks
   * Creates an identical copy of another set. Any option queried using the
   * @c get_option member function will return the same value for both sets.
   */
  URDL_DECL option_set(const option_set& other);

  /// Destroys an object of class @c option_set.
  URDL_DECL ~option_set();

  /// Assignment operator.
  /**
   * @par Remarks
   * Creates an identical copy of another set. Any option queried using the
   * @c get_option member function will return the same value for both sets.
   */
  URDL_DECL option_set& operator=(const option_set& other);

  /// Sets the value of an option in the set.
  /**
   * @param o The option to be set.
   *
   * @par Remarks
   * If the type @c Option is already present in the set, first removes that
   * element. Adds the option to the set.
   */
  template <typename Option>
  void set_option(const Option& o)
  {
    set_option_wrapper_base(new option_wrapper<Option>(o));
  }

  /// Sets multiple options in a set from another set.
  /**
   * @param other An option set containing all options to be set in the target.
   *
   * @par Remarks
   * Performs a deep copy of all option values from the object @c other into
   * the target set.
   */
  URDL_DECL void set_options(const option_set& other);

  /// Gets an option from the set.
  /**
   * @returns If the option is present in the set, an object containing the
   * value of the option. Otherwise, returns a default-constructed option.
   */
  template <typename Option>
  Option get_option() const
  {
    if (option_wrapper_base* o
        = get_option_wrapper_base(typeid(option_wrapper<Option>)))
      return static_cast<option_wrapper<Option>*>(o)->value;
    return Option();
  }

  /// Removes an option from the set.
  /**
   * @par Remarks
   * If the option is queried using the @c get_option member function, it will
   * return the default value of the option.
   */
  template <typename Option>
  void clear_option()
  {
    clear_option_wrapper_base(typeid(option_wrapper<Option>));
  }

private:
  struct option_wrapper_base
  {
    virtual ~option_wrapper_base() {}
    virtual const std::type_info& type_info() const = 0;
    virtual option_wrapper_base* clone() const = 0;
    detail::scoped_ptr<option_wrapper_base> next;
  };

  template <typename Option>
  struct option_wrapper : option_wrapper_base
  {
    option_wrapper(const Option& o) : value(o) {}
    const std::type_info& type_info() const
    { return typeid(option_wrapper<Option>); }
    option_wrapper_base* clone() const
    { return new option_wrapper<Option>(value); }
    Option value;
  };

  URDL_DECL void set_option_wrapper_base(option_wrapper_base* o);
  URDL_DECL option_wrapper_base* get_option_wrapper_base(
      const std::type_info& ti) const;
  URDL_DECL void clear_option_wrapper_base(const std::type_info& ti);

  detail::scoped_ptr<option_wrapper_base> head_;
};

} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#if defined(URDL_HEADER_ONLY)
# include "urdl/impl/option_set.ipp"
#endif

#endif // URDL_OPTION_SET_HPP

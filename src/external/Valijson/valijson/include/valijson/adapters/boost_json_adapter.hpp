/**
 * @file
 *
 * @brief   Adapter implementation for the Boost.JSON library.
 *
 * Include this file in your program to enable support for boost Boost.JSONs.
 *
 * This file defines the following classes (not in this order):
 *  - BoostJsonAdapter
 *  - BoostJsonArray
 *  - BoostJsonArrayValueIterator
 *  - BoostJsonFrozenValue
 *  - BoostJsonObject
 *  - BoostJsonObjectMember
 *  - BoostJsonObjectMemberIterator
 *  - BoostJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is BoostJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>

#include <boost/json.hpp>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>

namespace valijson {
namespace adapters {

class BoostJsonAdapter;
class BoostJsonArrayValueIterator;
class BoostJsonObjectMemberIterator;

typedef std::pair<std::string, BoostJsonAdapter> BoostJsonObjectMember;

/**
 * @brief   Light weight wrapper for a Boost.JSON that contains
 *          array-like data.
 *
 * This class is light weight wrapper for a Boost.JSON array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * Boost.JSON value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class BoostJsonArray
{
public:

    typedef BoostJsonArrayValueIterator const_iterator;
    typedef BoostJsonArrayValueIterator iterator;

    /// Construct a BoostJsonArray referencing an empty array.
    BoostJsonArray()
      : m_value(emptyArray()) { }

    /**
     * @brief   Construct BoostJsonArray referencing a specific Boost.JSON
     *          value.
     *
     * @param   value   reference to a Boost.JSON value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    explicit BoostJsonArray(const boost::json::value &value)
      : m_value(value.as_array())
    {
        // boost::json::value::as_array() will already have thrown an exception
        // if the underlying value is not an array
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying Boost.JSON implementation.
     */
    BoostJsonArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying Boost.JSON implementation.
     */
    BoostJsonArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.size();
    }

private:
    /**
     * @brief   Return a reference to a Boost.JSON value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const boost::json::array & emptyArray()
    {
        static const boost::json::array array;
        return array;
    }

    /// Reference to the contained value
    const boost::json::array &m_value;
};

/**
 * @brief  Light weight wrapper for a Boost.JSON object.
 *
 * This class is light weight wrapper for a Boost.JSON. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * Boost.JSON value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class BoostJsonObject
{
public:

    typedef BoostJsonObjectMemberIterator const_iterator;
    typedef BoostJsonObjectMemberIterator iterator;

    /// Construct a BoostJsonObject referencing an empty object singleton.
    BoostJsonObject()
      : m_value(emptyObject()) { }

    /**
     * @brief   Construct a BoostJsonObject referencing a specific Boost.JSON
     *          value.
     *
     * @param   value  reference to a Boost.JSON value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    BoostJsonObject(const boost::json::value &value)
      : m_value(value.as_object())
    {
        // boost::json::value::as_object() will already have thrown an exception
        // if the underlying value is not an array
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying Boost.JSON
     * implementation.
     */
    BoostJsonObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying Boost.JSON
     * implementation.
     */
    BoostJsonObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   property   property name to search for
     */
    BoostJsonObjectMemberIterator find(const std::string &property) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to an empty Boost.JSON.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const boost::json::object & emptyObject()
    {
        static const boost::json::object object;
        return object;
    }

    /// Reference to the contained object
    const boost::json::object &m_value;

};

/**
 * @brief   Stores an independent copy of a Boost.JSON value.
 *
 * This class allows a Boost.JSON value to be stored independent of its
 * original 'document'. Boost.JSON makes this easy to do, as it does
 * not require you to use custom memory management.
 *
 * @see FrozenValue
 */
class BoostJsonFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a Boost.JSON value
     *
     * @param  source  the Boost.JSON value to be copied
     */
    explicit BoostJsonFrozenValue(const boost::json::value &source)
      : m_value(source) { }

    FrozenValue * clone() const override
    {
        return new BoostJsonFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored Boost.JSON value
    boost::json::value m_value;
};

/**
 * @brief   Light weight wrapper for a Boost.JSON value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a Boost.JSON value. This class is
 * responsible for the mechanics of actually reading a Boost.JSON value, whereas
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class BoostJsonValue
{
public:

    /// Construct a wrapper for the empty object singleton
    BoostJsonValue()
      : m_value(emptyObject()) { }

    /**
     * @brief  Construct a BoostJsonValue for for a specific Boost.JSON value
     */
    BoostJsonValue(const boost::json::value &value)
      : m_value(value) { }

    /**
     * @brief   Create a new BoostJsonFrozenValue instance that contains the
     *          value referenced by this BoostJsonValue instance.
     *
     * @returns pointer to a new BoostJsonFrozenValue instance, belonging to
     *          the caller.
     */
    FrozenValue* freeze() const
    {
        return new BoostJsonFrozenValue(m_value);
    }

    /**
     * @brief  Return an instance of BoostJsonArrayAdapter.
     *
     * If the referenced Boost.JSON value is an array, this function will
     * return a std::optional containing a BoostJsonArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<BoostJsonArray> getArrayOptional() const
    {
        if (m_value.is_array()) {
            return opt::make_optional(BoostJsonArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced Boost.JSON value is an array, this function will
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
            result = m_value.get_array().size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.is_bool()) {
            result = m_value.get_bool();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.is_double()) {
            result = m_value.get_double();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if(m_value.is_int64()) {
            result = m_value.get_int64();
            return true;
        }
        return false;
    }

    /**
     * @brief   Optionally return a BoostJsonObject instance.
     *
     * If the referenced Boost.JSON is an object, this function will return a
     * std::optional containing a BoostJsonObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<BoostJsonObject> getObjectOptional() const
    {
        if (m_value.is_object()) {
            return opt::make_optional(BoostJsonObject(m_value));
        }

#if __cplusplus >= 201703
        // std::nullopt is available since C++17
        return std::nullopt;
#else
        // This is the older way to achieve the same result, but potentially at the cost of a compiler warning
        return {};
#endif
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced Boost.JSON value is an object, this function will
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
            result = m_value.get_object().size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.is_string()) {
            result = m_value.get_string().c_str();
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
        return m_value.is_bool();
    }

    bool isDouble() const
    {
        return m_value.is_double();
    }

    bool isInteger() const
    {
        return m_value.is_int64();
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
    static const boost::json::value & emptyObject()
    {
        static const boost::json::value object;
        return object;
    }

    /// Reference to the contained Boost.JSON value.
    const boost::json::value &m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting the Boost.JSON
 *          library.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class BoostJsonAdapter:
    public BasicAdapter<BoostJsonAdapter,
                        BoostJsonArray,
                        BoostJsonObjectMember,
                        BoostJsonObject,
                        BoostJsonValue>
{
public:

    /// Construct a BoostJsonAdapter that contains an empty object
    BoostJsonAdapter()
      : BasicAdapter() { }

    /// Construct a BoostJsonAdapter using a specific Boost.JSON value
    BoostJsonAdapter(const boost::json::value &value)
      : BasicAdapter(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * BoostJsonAdapter representing a value stored in the array.
 *
 * @see BoostJsonArray
 */
class BoostJsonArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = BoostJsonAdapter;
    using difference_type = BoostJsonAdapter;
    using pointer = BoostJsonAdapter*;
    using reference = BoostJsonAdapter&;

    /**
     * @brief   Construct a new BoostJsonArrayValueIterator using an existing
     *          Boost.JSON iterator.
     *
     * @param   itr  Boost.JSON iterator to store
     */
    BoostJsonArrayValueIterator(
        const boost::json::array::const_iterator itr)
      : m_itr(itr) { }

    /// Returns a BoostJsonAdapter that contains the value of the current
    /// element.
    BoostJsonAdapter operator*() const
    {
        return BoostJsonAdapter(*m_itr);
    }

    DerefProxy<BoostJsonAdapter> operator->() const
    {
        return DerefProxy<BoostJsonAdapter>(**this);
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
     * @returns true if the iterators are equal, false otherwise.
     */
    bool operator==(const BoostJsonArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const BoostJsonArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const BoostJsonArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    BoostJsonArrayValueIterator operator++(int)
    {
        BoostJsonArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const BoostJsonArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

private:

    boost::json::array::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of BoostJsonObjectMember representing one of the members of the object.
 *
 * @see BoostJsonObject
 * @see BoostJsonObjectMember
 */
class BoostJsonObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = BoostJsonObjectMember;
    using difference_type = BoostJsonObjectMember;
    using pointer = BoostJsonObjectMember*;
    using reference = BoostJsonObjectMember&;

    /**
     * @brief   Construct an iterator from a BoostJson iterator.
     *
     * @param   itr  BoostJson iterator to store
     */
    BoostJsonObjectMemberIterator(boost::json::object::const_iterator itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a BoostJsonObjectMember that contains the key and
     *          value belonging to the object member identified by the iterator.
     */
    BoostJsonObjectMember operator*() const
    {
        return BoostJsonObjectMember(m_itr->key(), m_itr->value());
    }

    DerefProxy<BoostJsonObjectMember> operator->() const
    {
        return DerefProxy<BoostJsonObjectMember>(**this);
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
    bool operator==(const BoostJsonObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const BoostJsonObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const BoostJsonObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    BoostJsonObjectMemberIterator operator++(int)
    {
        BoostJsonObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const BoostJsonObjectMemberIterator& operator--()
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original Boost.JSON iterator
    boost::json::object::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for BoostJsonAdapter.
template<>
struct AdapterTraits<valijson::adapters::BoostJsonAdapter>
{
    typedef boost::json::value DocumentType;

    static std::string adapterName()
    {
        return "BoostJsonAdapter";
    }
};

inline bool BoostJsonFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return BoostJsonAdapter(m_value).equalTo(other, strict);
}

inline BoostJsonArrayValueIterator BoostJsonArray::begin() const
{
    return m_value.cbegin();
}

inline BoostJsonArrayValueIterator BoostJsonArray::end() const
{
    return m_value.cend();
}

inline BoostJsonObjectMemberIterator BoostJsonObject::begin() const
{
    return m_value.cbegin();
}

inline BoostJsonObjectMemberIterator BoostJsonObject::end() const
{
    return m_value.cend();
}

inline BoostJsonObjectMemberIterator BoostJsonObject::find(
    const std::string &propertyName) const
{
    return m_value.find(propertyName);
}

}  // namespace adapters
}  // namespace valijson

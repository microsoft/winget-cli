/**
 * @file
 *
 * @brief   Adapter implementation for the JsonCpp parser library.
 *
 * Include this file in your program to enable support for JsonCpp.
 *
 * This file defines the following classes (not in this order):
 *  - JsonCppAdapter
 *  - JsonCppArray
 *  - JsonCppArrayValueIterator
 *  - JsonCppFrozenValue
 *  - JsonCppObject
 *  - JsonCppObjectMember
 *  - JsonCppObjectMemberIterator
 *  - JsonCppValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is JsonCppAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <cstdint>
#include <string>
#include <iterator>

#include <json/json.h>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class JsonCppAdapter;
class JsonCppArrayValueIterator;
class JsonCppObjectMemberIterator;

typedef std::pair<std::string, JsonCppAdapter> JsonCppObjectMember;

/**
 * @brief  Light weight wrapper for a JsonCpp array value.
 *
 * This class is light weight wrapper for a JsonCpp array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * JsonCpp value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class JsonCppArray
{
public:

    typedef JsonCppArrayValueIterator const_iterator;
    typedef JsonCppArrayValueIterator iterator;

    /// Construct a JsonCppArray referencing an empty array.
    JsonCppArray()
      : m_value(emptyArray()) { }

    /**
     * @brief   Construct a JsonCppArray referencing a specific JsonCpp value.
     *
     * @param   value   reference to a JsonCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    JsonCppArray(const Json::Value &value)
      : m_value(value)
    {
        if (!value.isArray()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying JsonCpp implementation.
     */
    JsonCppArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying JsonCpp implementation.
     */
    JsonCppArrayValueIterator end() const;

    /// Return the number of elements in the array.
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to a JsonCpp value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const Json::Value & emptyArray()
    {
        static const Json::Value array(Json::arrayValue);
        return array;
    }

    /// Reference to the contained array
    const Json::Value &m_value;

};

/**
 * @brief  Light weight wrapper for a JsonCpp object.
 *
 * This class is light weight wrapper for a JsonCpp object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * JsonCpp object, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class JsonCppObject
{
public:

    typedef JsonCppObjectMemberIterator const_iterator;
    typedef JsonCppObjectMemberIterator iterator;

    /// Construct a JsonCppObject referencing an empty object singleton.
    JsonCppObject()
      : m_value(emptyObject()) { }

    /**
     * @brief   Construct a JsonCppObject referencing a specific JsonCpp value.
     *
     * @param   value  reference to a JsonCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    JsonCppObject(const Json::Value &value)
      : m_value(value)
    {
        if (!value.isObject()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying JsonCpp implementation.
     */
    JsonCppObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying JsonCpp implementation.
     */
    JsonCppObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for a member/property with the given name
     *
     * @param   propertyName   Property name
     *
     * @returns a valid iterator if found, or an invalid iterator if not found
     */
    JsonCppObjectMemberIterator find(const std::string &propertyName) const;

    /// Return the number of members in the object
    size_t size() const
    {
        return m_value.size();
    }

private:

    /// Return a reference to an empty JsonCpp object
    static const Json::Value & emptyObject()
    {
        static const Json::Value object(Json::objectValue);
        return object;
    }

    /// Reference to the contained object
    const Json::Value &m_value;
};

/**
 * @brief   Stores an independent copy of a JsonCpp value.
 *
 * This class allows a JsonCpp value to be stored independent of its original
 * document. JsonCpp makes this easy to do, as it does not perform any
 * custom memory management.
 *
 * @see FrozenValue
 */
class JsonCppFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a JsonCpp value
     *
     * @param  source  the JsonCpp value to be copied
     */
    explicit JsonCppFrozenValue(const Json::Value &source)
      : m_value(source) { }

    FrozenValue * clone() const override
    {
        return new JsonCppFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored JsonCpp value
    Json::Value m_value;
};

/**
 * @brief   Light weight wrapper for a JsonCpp value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a JsonCpp value. This class is responsible
 * for the mechanics of actually reading a JsonCpp value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class JsonCppValue
{
public:

    /// Construct a wrapper for the empty object singleton
    JsonCppValue()
      : m_value(emptyObject()) { }

    /// Construct a wrapper for a specific JsonCpp value
    JsonCppValue(const Json::Value &value)
      : m_value(value) { }

    /**
     * @brief   Create a new JsonCppFrozenValue instance that contains the
     *          value referenced by this JsonCppValue instance.
     *
     * @returns pointer to a new JsonCppFrozenValue instance, belonging to the
     *          caller.
     */
    FrozenValue * freeze() const
    {
        return new JsonCppFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a JsonCppArray instance.
     *
     * If the referenced JsonCpp value is an array, this function will return a
     * std::optional containing a JsonCppArray instance referencing the
     * array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<JsonCppArray> getArrayOptional() const
    {
        if (m_value.isArray()) {
            return opt::make_optional(JsonCppArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced JsonCpp value is an array, this function will retrieve
     * the number of elements in the array and store it in the output variable
     * provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.isArray()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.isBool()) {
            result = m_value.asBool();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.isDouble()) {
            result = m_value.asDouble();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_value.isIntegral()) {
            result = static_cast<int64_t>(m_value.asInt());
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a JsonCppObject instance.
     *
     * If the referenced JsonCpp value is an object, this function will return a
     * std::optional containing a JsonCppObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<JsonCppObject> getObjectOptional() const
    {
        if (m_value.isObject()) {
            return opt::make_optional(JsonCppObject(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced JsonCpp value is an object, this function will retrieve
     * the number of members in the object and store it in the output variable
     * provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.isObject()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.isString()) {
            result = m_value.asString();
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
        return m_value.isArray() && !m_value.isNull();
    }

    bool isBool() const
    {
        return m_value.isBool();
    }

    bool isDouble() const
    {
        return m_value.isDouble();
    }

    bool isInteger() const
    {
        return m_value.isIntegral() && !m_value.isBool();
    }

    bool isNull() const
    {
        return m_value.isNull();
    }

    bool isNumber() const
    {
        return m_value.isNumeric()  && !m_value.isBool();
    }

    bool isObject() const
    {
        return m_value.isObject() && !m_value.isNull();
    }

    bool isString() const
    {
        return m_value.isString();
    }

private:

    /// Return a reference to an empty object singleton.
    static const Json::Value &emptyObject()
    {
        static Json::Value object(Json::objectValue);
        return object;
    }

    /// Reference to the contained JsonCpp value
    const Json::Value &m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting JsonCpp.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class JsonCppAdapter:
    public BasicAdapter<JsonCppAdapter,
                        JsonCppArray,
                        JsonCppObjectMember,
                        JsonCppObject,
                        JsonCppValue>
{
public:

    /// Construct a JsonCppAdapter that contains an empty object
    JsonCppAdapter()
      : BasicAdapter() { }

    /// Construct a JsonCppAdapter containing a specific JsonCpp value
    JsonCppAdapter(const Json::Value &value)
      : BasicAdapter(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * JsonCppAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see JsonCppArray
 */
class JsonCppArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = JsonCppAdapter;
    using difference_type = JsonCppAdapter;
    using pointer = JsonCppAdapter*;
    using reference = JsonCppAdapter&;

    /**
     * @brief   Construct a new JsonCppArrayValueIterator using an existing
     *          JsonCpp iterator.
     *
     * @param   itr  JsonCpp iterator to store
     */
    JsonCppArrayValueIterator(const Json::Value::const_iterator &itr)
      : m_itr(itr) { }

    /// Returns a JsonCppAdapter that contains the value of the current element.
    JsonCppAdapter operator*() const
    {
        return JsonCppAdapter(*m_itr);
    }

    DerefProxy<JsonCppAdapter> operator->() const
    {
        return DerefProxy<JsonCppAdapter>(**this);
    }

    /**
     * @brief   Compare this iterator against another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   rhs  iterator to compare against
     *
     * @returns true if the iterators are equal, false otherwise.
     */
    bool operator==(const JsonCppArrayValueIterator &rhs) const
    {
        return m_itr == rhs.m_itr;
    }

    bool operator!=(const JsonCppArrayValueIterator &rhs) const
    {
        return !(m_itr == rhs.m_itr);
    }

    JsonCppArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    JsonCppArrayValueIterator operator++(int)
    {
        JsonCppArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    JsonCppArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        if (n > 0) {
            while (n-- > 0) {
                m_itr++;
            }
        } else {
            while (n++ < 0) {
                m_itr--;
            }
        }
    }

private:

    Json::Value::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of JsonCppObjectMember representing one of the members of the object. It has
 * been implemented using the boost iterator_facade template.
 *
 * @see JsonCppObject
 * @see JsonCppObjectMember
 */
class JsonCppObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = JsonCppObjectMember;
    using difference_type = JsonCppObjectMember;
    using pointer = JsonCppObjectMember*;
    using reference = JsonCppObjectMember&;

    /**
     * @brief   Construct an iterator from a JsonCpp iterator.
     *
     * @param   itr  JsonCpp iterator to store
     */
    JsonCppObjectMemberIterator(const Json::ValueConstIterator &itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a JsonCppObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    JsonCppObjectMember operator*() const
    {
        return JsonCppObjectMember(m_itr.key().asString(), *m_itr);
    }

    DerefProxy<JsonCppObjectMember> operator->() const
    {
        return DerefProxy<JsonCppObjectMember>(**this);
    }

    /**
     * @brief   Compare this iterator with another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   rhs  Iterator to compare with
     *
     * @returns true if the underlying iterators are equal, false otherwise
     */
    bool operator==(const JsonCppObjectMemberIterator &rhs) const
    {
        return m_itr == rhs.m_itr;
    }

    bool operator!=(const JsonCppObjectMemberIterator &rhs) const
    {
        return !(m_itr == rhs.m_itr);
    }

    const JsonCppObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    JsonCppObjectMemberIterator operator++(int)
    {
        JsonCppObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    JsonCppObjectMemberIterator operator--()
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original JsonCpp iterator
    Json::ValueConstIterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for JsonCppAdapter.
template<>
struct AdapterTraits<valijson::adapters::JsonCppAdapter>
{
    typedef Json::Value DocumentType;

    static std::string adapterName()
    {
        return "JsonCppAdapter";
    }
};

inline bool JsonCppFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return JsonCppAdapter(m_value).equalTo(other, strict);
}

inline JsonCppArrayValueIterator JsonCppArray::begin() const
{
    return m_value.begin();
}

inline JsonCppArrayValueIterator JsonCppArray::end() const
{
    return m_value.end();
}

inline JsonCppObjectMemberIterator JsonCppObject::begin() const
{
    return m_value.begin();
}

inline JsonCppObjectMemberIterator JsonCppObject::end() const
{
    return m_value.end();
}

inline JsonCppObjectMemberIterator JsonCppObject::find(
    const std::string &propertyName) const
{
    if (m_value.isMember(propertyName)) {
        Json::ValueConstIterator itr;
        for (itr = m_value.begin(); itr != m_value.end(); ++itr) {
            if (itr.key() == propertyName) {
                return itr;
            }
        }
    }

    return m_value.end();
}

}  // namespace adapters
}  // namespace valijson

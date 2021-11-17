/**
 * @file
 *
 * @brief   Adapter implementation for the PicoJson parser library.
 *
 * Include this file in your program to enable support for PicoJson.
 *
 * This file defines the following classes (not in this order):
 *  - PicoJsonAdapter
 *  - PicoJsonArray
 *  - PicoJsonArrayValueIterator
 *  - PicoJsonFrozenValue
 *  - PicoJsonObject
 *  - PicoJsonObjectMember
 *  - PicoJsonObjectMemberIterator
 *  - PicoJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is PicoJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>

#ifdef _MSC_VER
#pragma warning(disable: 4706)
#include <picojson.h>
#pragma warning(default: 4706)
#else
#include <picojson.h>
#endif

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class PicoJsonAdapter;
class PicoJsonArrayValueIterator;
class PicoJsonObjectMemberIterator;

typedef std::pair<std::string, PicoJsonAdapter> PicoJsonObjectMember;

/**
 * @brief  Light weight wrapper for a PicoJson array value.
 *
 * This class is light weight wrapper for a PicoJson array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * PicoJson value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class PicoJsonArray
{
public:

    typedef PicoJsonArrayValueIterator const_iterator;
    typedef PicoJsonArrayValueIterator iterator;

    /// Construct a PicoJsonArray referencing an empty array.
    PicoJsonArray()
      : m_value(emptyArray()) { }

    /**
     * @brief   Construct a PicoJsonArray referencing a specific PicoJson
     *          value.
     *
     * @param   value   reference to a PicoJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    explicit PicoJsonArray(const picojson::value &value)
      : m_value(value)
    {
        if (!value.is<picojson::array>()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying PicoJson implementation.
     */
    PicoJsonArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying PicoJson implementation.
     */
    PicoJsonArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        const picojson::array &array = m_value.get<picojson::array>();
        return array.size();
    }

private:

    /**
     * @brief   Return a reference to a PicoJson value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const picojson::value & emptyArray()
    {
        static const picojson::value array(picojson::array_type, false);
        return array;
    }

    /// Reference to the contained value
    const picojson::value &m_value;
};

/**
 * @brief  Light weight wrapper for a PicoJson object.
 *
 * This class is light weight wrapper for a PicoJson object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * PicoJson value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class PicoJsonObject
{
public:

    typedef PicoJsonObjectMemberIterator const_iterator;
    typedef PicoJsonObjectMemberIterator iterator;

    /// Construct a PicoJsonObject referencing an empty object singleton.
    PicoJsonObject()
      : m_value(emptyObject()) { }

    /**
     * @brief   Construct a PicoJsonObject referencing a specific PicoJson
     *          value.
     *
     * @param   value  reference to a PicoJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    PicoJsonObject(const picojson::value &value)
      : m_value(value)
    {
        if (!value.is<picojson::object>()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying PicoJson implementation.
     */
    PicoJsonObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying PicoJson implementation.
     */
    PicoJsonObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    PicoJsonObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        const picojson::object &object = m_value.get<picojson::object>();
        return object.size();
    }

private:

    /**
     * @brief   Return a reference to a PicoJson value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const picojson::value & emptyObject()
    {
        static const picojson::value object(picojson::object_type, false);
        return object;
    }

    /// Reference to the contained object
    const picojson::value &m_value;
};

/**
 * @brief   Stores an independent copy of a PicoJson value.
 *
 * This class allows a PicoJson value to be stored independent of its original
 * document. PicoJson makes this easy to do, as it does not perform any
 * custom memory management.
 *
 * @see FrozenValue
 */
class PicoJsonFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a PicoJson value
     *
     * @param  source  the PicoJson value to be copied
     */
    explicit PicoJsonFrozenValue(const picojson::value &source)
      : m_value(source) { }

    FrozenValue * clone() const override
    {
        return new PicoJsonFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored PicoJson value
    picojson::value m_value;
};

/**
 * @brief   Light weight wrapper for a PicoJson value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a PicoJson value. This class is responsible
 * for the mechanics of actually reading a PicoJson value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class PicoJsonValue
{
public:

    /// Construct a wrapper for the empty object singleton
    PicoJsonValue()
      : m_value(emptyObject()) { }

    /// Construct a wrapper for a specific PicoJson value
    PicoJsonValue(const picojson::value &value)
      : m_value(value) { }

    /**
     * @brief   Create a new PicoJsonFrozenValue instance that contains the
     *          value referenced by this PicoJsonValue instance.
     *
     * @returns pointer to a new PicoJsonFrozenValue instance, belonging to the
     *          caller.
     */
    FrozenValue * freeze() const
    {
        return new PicoJsonFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a PicoJsonArray instance.
     *
     * If the referenced PicoJson value is an array, this function will return
     * a std::optional containing a PicoJsonArray instance referencing the
     * array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<PicoJsonArray> getArrayOptional() const
    {
        if (m_value.is<picojson::array>()) {
            return opt::make_optional(PicoJsonArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced PicoJson value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.is<picojson::array>()) {
            const picojson::array& array = m_value.get<picojson::array>();
            result = array.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.is<bool>()) {
            result = m_value.get<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.is<double>()) {
            result = m_value.get<double>();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_value.is<int64_t>()) {
            result = m_value.get<int64_t>();
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a PicoJsonObject instance.
     *
     * If the referenced PicoJson value is an object, this function will return a
     * std::optional containing a PicoJsonObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<PicoJsonObject> getObjectOptional() const
    {
        if (m_value.is<picojson::object>()) {
            return opt::make_optional(PicoJsonObject(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced PicoJson value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.is<picojson::object>()) {
            const picojson::object &object = m_value.get<picojson::object>();
            result = object.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.is<std::string>()) {
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
        return m_value.is<picojson::array>();
    }

    bool isBool() const
    {
        return m_value.is<bool>();
    }

    bool isDouble() const
    {
        if (m_value.is<int64_t>()) {
            return false;
        }

        return m_value.is<double>();
    }

    bool isInteger() const
    {
        return m_value.is<int64_t>();
    }

    bool isNull() const
    {
        return m_value.is<picojson::null>();
    }

    bool isNumber() const
    {
        return m_value.is<double>();
    }

    bool isObject() const
    {
        return m_value.is<picojson::object>();
    }

    bool isString() const
    {
        return m_value.is<std::string>();
    }

private:

    /// Return a reference to an empty object singleton
    static const picojson::value & emptyObject()
    {
        static const picojson::value object(picojson::object_type, false);
        return object;
    }

    /// Reference to the contained PicoJson value.
    const picojson::value &m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting PicoJson.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class PicoJsonAdapter:
    public BasicAdapter<PicoJsonAdapter,
                        PicoJsonArray,
                        PicoJsonObjectMember,
                        PicoJsonObject,
                        PicoJsonValue>
{
public:

    /// Construct a PicoJsonAdapter that contains an empty object
    PicoJsonAdapter()
      : BasicAdapter() { }

    /// Construct a PicoJsonAdapter containing a specific PicoJson value
    PicoJsonAdapter(const picojson::value &value)
      : BasicAdapter(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * PicoJsonAdapter representing a value stored in the array. It has been
 * implemented using the std::iterator template.
 *
 * @see PicoJsonArray
 */
class PicoJsonArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PicoJsonAdapter;
    using difference_type = PicoJsonAdapter;
    using pointer = PicoJsonAdapter*;
    using reference = PicoJsonAdapter&;

    /**
     * @brief   Construct a new PicoJsonArrayValueIterator using an existing
     *          PicoJson iterator.
     *
     * @param   itr  PicoJson iterator to store
     */
    PicoJsonArrayValueIterator(const picojson::array::const_iterator &itr)
      : m_itr(itr) { }

    /// Returns a PicoJsonAdapter that contains the value of the current
    /// element.
    PicoJsonAdapter operator*() const
    {
        return PicoJsonAdapter(*m_itr);
    }

    DerefProxy<PicoJsonAdapter> operator->() const
    {
        return DerefProxy<PicoJsonAdapter>(**this);
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
    bool operator==(const PicoJsonArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const PicoJsonArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const PicoJsonArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PicoJsonArrayValueIterator operator++(int)
    {
        PicoJsonArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PicoJsonArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

private:

    picojson::array::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of PicoJsonObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see PicoJsonObject
 * @see PicoJsonObjectMember
 */
class PicoJsonObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PicoJsonObjectMember;
    using difference_type = PicoJsonObjectMember;
    using pointer = PicoJsonObjectMember*;
    using reference = PicoJsonObjectMember&;

    /**
     * @brief   Construct an iterator from a PicoJson iterator.
     *
     * @param   itr  PicoJson iterator to store
     */
    PicoJsonObjectMemberIterator(const picojson::object::const_iterator &itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a PicoJsonObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    PicoJsonObjectMember operator*() const
    {
        return PicoJsonObjectMember(m_itr->first, m_itr->second);
    }

    DerefProxy<PicoJsonObjectMember> operator->() const
    {
        return DerefProxy<PicoJsonObjectMember>(**this);
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
    bool operator==(const PicoJsonObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const PicoJsonObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const PicoJsonObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PicoJsonObjectMemberIterator operator++(int)
    {
        PicoJsonObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PicoJsonObjectMemberIterator& operator--(int)
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original PicoJson iterator
    picojson::object::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for PicoJsonAdapter.
template<>
struct AdapterTraits<valijson::adapters::PicoJsonAdapter>
{
    typedef picojson::value DocumentType;

    static std::string adapterName()
    {
        return "PicoJsonAdapter";
    }
};

inline bool PicoJsonFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return PicoJsonAdapter(m_value).equalTo(other, strict);
}

inline PicoJsonArrayValueIterator PicoJsonArray::begin() const
{
    const picojson::array &array = m_value.get<picojson::array>();
    return array.begin();
}

inline PicoJsonArrayValueIterator PicoJsonArray::end() const
{
    const picojson::array &array = m_value.get<picojson::array>();
    return array.end();
}

inline PicoJsonObjectMemberIterator PicoJsonObject::begin() const
{
    const picojson::object &object = m_value.get<picojson::object>();
    return object.begin();
}

inline PicoJsonObjectMemberIterator PicoJsonObject::end() const
{
    const picojson::object &object = m_value.get<picojson::object>();
    return object.end();
}

inline PicoJsonObjectMemberIterator PicoJsonObject::find(
    const std::string &propertyName) const
{
    const picojson::object &object = m_value.get<picojson::object>();
    return object.find(propertyName);
}

}  // namespace adapters
}  // namespace valijson

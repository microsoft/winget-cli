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
#include <json.hpp>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>

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
      : value(emptyArray()) { }

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
      : value(value)
    {
        if (!value.is_array()) {
            throw std::runtime_error("Value is not an array.");
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
        return value.size();
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
    const nlohmann::json &value;
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
      : value(emptyObject()) { }

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
      : value(value)
    {
        if (!value.is_object()) {
            throw std::runtime_error("Value is not an object.");
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
        return value.size();
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
    const nlohmann::json &value;
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
    explicit NlohmannJsonFrozenValue(const nlohmann::json &source)
      : value(source) { }

    virtual FrozenValue * clone() const
    {
        return new NlohmannJsonFrozenValue(value);
    }

    virtual bool equalTo(const Adapter &other, bool strict) const;

private:

    /// Stored NlohmannJson value
    nlohmann::json value;
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
      : value(emptyObject()) { }

    /// Construct a wrapper for a specific NlohmannJson value
    NlohmannJsonValue(const nlohmann::json &value)
      : value(value) { }

    /**
     * @brief   Create a new NlohmannJsonFrozenValue instance that contains the
     *          value referenced by this NlohmannJsonValue instance.
     *
     * @returns pointer to a new NlohmannJsonFrozenValue instance, belonging to the
     *          caller.
     */
    FrozenValue * freeze() const
    {
        return new NlohmannJsonFrozenValue(value);
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
        if (value.is_array()) {
            return opt::make_optional(NlohmannJsonArray(value));
        }

        return opt::optional<NlohmannJsonArray>();
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
        if (value.is_array()) {
            result = value.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (value.is_boolean()) {
            result = value.get<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (value.is_number_float()) {
            result = value.get<double>();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if(value.is_number_integer()) {
            result = value.get<int64_t>();
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
        if (value.is_object()) {
            return opt::make_optional(NlohmannJsonObject(value));
        }

        return opt::optional<NlohmannJsonObject>();
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
        if (value.is_object()) {
            result = value.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (value.is_string()) {
            result = value.get<std::string>();
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
        return value.is_array();
    }

    bool isBool() const
    {
        return value.is_boolean();
    }

    bool isDouble() const
    {
        return value.is_number_float();
    }

    bool isInteger() const
    {
        return value.is_number_integer();
    }

    bool isNull() const
    {
        return value.is_null();
    }

    bool isNumber() const
    {
        return value.is_number();
    }

    bool isObject() const
    {
        return value.is_object();
    }

    bool isString() const
    {
        return value.is_string();
    }

private:

    /// Return a reference to an empty object singleton
    static const nlohmann::json & emptyObject()
    {
        static const nlohmann::json object = nlohmann::json::object();
        return object;
    }

    /// Reference to the contained NlohmannJson value.
    const nlohmann::json &value;
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
class NlohmannJsonArrayValueIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,  // bi-directional iterator
        NlohmannJsonAdapter>                 // value type
{
public:

    /**
     * @brief   Construct a new NlohmannJsonArrayValueIterator using an existing
     *          NlohmannJson iterator.
     *
     * @param   itr  NlohmannJson iterator to store
     */
    NlohmannJsonArrayValueIterator(const nlohmann::json::const_iterator &itr)
      : itr(itr) { }

    /// Returns a NlohmannJsonAdapter that contains the value of the current
    /// element.
    NlohmannJsonAdapter operator*() const
    {
        return NlohmannJsonAdapter(*itr);
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
        return itr == other.itr;
    }

    bool operator!=(const NlohmannJsonArrayValueIterator &other) const
    {
        return !(itr == other.itr);
    }

    const NlohmannJsonArrayValueIterator& operator++()
    {
        itr++;

        return *this;
    }

    NlohmannJsonArrayValueIterator operator++(int)
    {
        NlohmannJsonArrayValueIterator iterator_pre(itr);
        ++(*this);
        return iterator_pre;
    }

    const NlohmannJsonArrayValueIterator& operator--()
    {
        itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        itr += n;
    }

private:
    nlohmann::json::const_iterator itr;
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
class NlohmannJsonObjectMemberIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,     // bi-directional iterator
        NlohmannJsonObjectMember>            // value type
{
public:

    /**
     * @brief   Construct an iterator from a NlohmannJson iterator.
     *
     * @param   itr  NlohmannJson iterator to store
     */
    NlohmannJsonObjectMemberIterator(const nlohmann::json::const_iterator &itr)
      : itr(itr) { }

    /**
     * @brief   Returns a NlohmannJsonObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    NlohmannJsonObjectMember operator*() const
    {
        return NlohmannJsonObjectMember(itr.key(), itr.value());
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
        return itr == other.itr;
    }

    bool operator!=(const NlohmannJsonObjectMemberIterator &other) const
    {
        return !(itr == other.itr);
    }

    const NlohmannJsonObjectMemberIterator& operator++()
    {
        itr++;

        return *this;
    }

    NlohmannJsonObjectMemberIterator operator++(int)
    {
        NlohmannJsonObjectMemberIterator iterator_pre(itr);
        ++(*this);
        return iterator_pre;
    }

    const NlohmannJsonObjectMemberIterator& operator--()
    {
        itr--;

        return *this;
    }

private:

    /// Iternal copy of the original NlohmannJson iterator
    nlohmann::json::const_iterator itr;
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
    return NlohmannJsonAdapter(value).equalTo(other, strict);
}

inline NlohmannJsonArrayValueIterator NlohmannJsonArray::begin() const
{
    return value.begin();
}

inline NlohmannJsonArrayValueIterator NlohmannJsonArray::end() const
{
    return value.end();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::begin() const
{
    return value.begin();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::end() const
{
    return value.end();
}

inline NlohmannJsonObjectMemberIterator NlohmannJsonObject::find(
        const std::string &propertyName) const
{
    return value.find(propertyName);
}

}  // namespace adapters
}  // namespace valijson

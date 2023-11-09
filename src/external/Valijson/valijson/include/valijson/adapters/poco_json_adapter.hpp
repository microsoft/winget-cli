/**
* @file
*
* @brief   Adapter implementation for the Poco json parser library.
*
* Include this file in your program to enable support for Poco json.
*
* This file defines the following classes (not in this order):
*  - PocoJsonAdapter
*  - PocoJsonArray
*  - PocoJsonValueIterator
*  - PocoJsonFrozenValue
*  - PocoJsonObject
*  - PocoJsonObjectMember
*  - PocoJsonObjectMemberIterator
*  - PocoJsonValue
*
* Due to the dependencies that exist between these classes, the ordering of
* class declarations and definitions may be a bit confusing. The best place to
* start is PocoJsonAdapter. This class definition is actually very small,
* since most of the functionality is inherited from the BasicAdapter class.
* Most of the classes in this file are provided as template arguments to the
* inherited BasicAdapter class.
*/

#pragma once

#include <string>
#include <Poco/JSON/Object.h>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class PocoJsonAdapter;
class PocoJsonArrayValueIterator;
class PocoJsonObjectMemberIterator;

typedef std::pair<std::string, PocoJsonAdapter> PocoJsonObjectMember;

/**
* @brief  Light weight wrapper for a PocoJson array value.
*
* This class is light weight wrapper for a PocoJson array. It provides a
* minimum set of container functions and typedefs that allow it to be used as
* an iterable container.
*
* An instance of this class contains a single reference to the underlying
* PocoJson value, assumed to be an array, so there is very little overhead
* associated with copy construction and passing by value.
*/
class PocoJsonArray
{
public:

    typedef PocoJsonArrayValueIterator const_iterator;
    typedef PocoJsonArrayValueIterator iterator;

    /// Construct a PocoJsonArray referencing an empty array.
    PocoJsonArray()
        : m_value(emptyArray())
    { }

    /**
    * @brief   Construct a PocoJsonArray referencing a specific PocoJson
    *          value.
    *
    * @param   value   reference to a PocoJson value
    *
    * Note that this constructor will throw an exception if the value is not
    * an array.
    */
    PocoJsonArray(const Poco::Dynamic::Var &value)
        : m_value(value)
    {
        if (value.type() != typeid(Poco::JSON::Array::Ptr)) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
    * @brief   Return an iterator for the first element of the array.
    *
    * The iterator return by this function is effectively the iterator
    * returned by the underlying PocoJson implementation.
    */
    PocoJsonArrayValueIterator begin() const;

    /**
    * @brief   Return an iterator for one-past the last element of the array.
    *
    * The iterator return by this function is effectively the iterator
    * returned by the underlying PocoJson implementation.
    */
    PocoJsonArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.extract<Poco::JSON::Array::Ptr>()->size();
    }

private:

    /**
    * @brief   Return a PocoJson value that is an empty array.
    */
    static Poco::Dynamic::Var emptyArray()
    {
        Poco::Dynamic::Var array = Poco::JSON::Array::Ptr();
        return array;
    }

    /// Contained value
    Poco::Dynamic::Var m_value;
};

/**
* @brief  Light weight wrapper for a PocoJson object.
*
* This class is light weight wrapper for a PocoJson object. It provides a
* minimum set of container functions and typedefs that allow it to be used as
* an iterable container.
*
* An instance of this class contains a single reference to the underlying
* PocoJson value, assumed to be an object, so there is very little overhead
* associated with copy construction and passing by value.
*/
class PocoJsonObject
{
public:

    typedef PocoJsonObjectMemberIterator const_iterator;
    typedef PocoJsonObjectMemberIterator iterator;

    /// Construct a PocoJsonObject an empty object.
    PocoJsonObject()
        : m_value(emptyObject())
    { }

    /**
    * @brief   Construct a PocoJsonObject referencing a specific PocoJson
    *          value.
    *
    * @param   value  reference to a PocoJson value
    *
    * Note that this constructor will throw an exception if the value is not
    * an object.
    */
    PocoJsonObject(const Poco::Dynamic::Var &value)
        : m_value(value)
    {
        if (value.type() != typeid(Poco::JSON::Object::Ptr)) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
    * @brief   Return an iterator for this first object member
    *
    * The iterator return by this function is effectively a wrapper around
    * the iterator value returned by the underlying PocoJson implementation.
    */
    PocoJsonObjectMemberIterator begin() const;

    /**
    * @brief   Return an iterator for an invalid object member that indicates
    *          the end of the collection.
    *
    * The iterator return by this function is effectively a wrapper around
    * the iterator value returned by the underlying PocoJson implementation.
    */
    PocoJsonObjectMemberIterator end() const;

    /**
    * @brief   Return an iterator for the object member with the specified
    *          property name.
    *
    * If an object member with the specified name does not exist, the iterator
    * returned will be the same as the iterator returned by the end() function.
    *
    * @param   propertyName  property name to search for
    */
    PocoJsonObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.extract<Poco::JSON::Object::Ptr>()->size();
    }

private:

    /**
    * @brief   Return a PocoJson value that is empty object.
    */
    static Poco::Dynamic::Var emptyObject()
    {
        Poco::Dynamic::Var object = Poco::JSON::Object::Ptr();
        return object;
    }

    /// Contained value
    Poco::Dynamic::Var m_value;
};

/**
* @brief   Stores an independent copy of a PocoJson value.
*
* This class allows a PocoJson value to be stored independent of its original
* document. PocoJson makes this easy to do, as it does not perform any
* custom memory management.
*
* @see FrozenValue
*/
class PocoJsonFrozenValue : public FrozenValue
{
public:

    /**
    * @brief  Make a copy of a PocoJson value
    *
    * @param  source  the PocoJson value to be copied
    */
    explicit PocoJsonFrozenValue(const Poco::Dynamic::Var &source)
        : m_value(source)
    { }

    virtual FrozenValue * clone() const
    {
        return new PocoJsonFrozenValue(m_value);
    }

    virtual bool equalTo(const Adapter &other, bool strict) const;

private:

    /// Stored PocoJson value
    Poco::Dynamic::Var m_value;
};


/**
* @brief   Light weight wrapper for a PocoJson value.
*
* This class is passed as an argument to the BasicAdapter template class,
* and is used to provide access to a PocoJson value. This class is responsible
* for the mechanics of actually reading a PocoJson value, whereas the
* BasicAdapter class is responsible for the semantics of type comparisons
* and conversions.
*
* The functions that need to be provided by this class are defined implicitly
* by the implementation of the BasicAdapter template class.
*
* @see BasicAdapter
*/
class PocoJsonValue
{
public:

    /// Construct a wrapper for the empty object
    PocoJsonValue()
        : m_value(emptyObject())
    { }

    /// Construct a wrapper for a specific PocoJson value
    PocoJsonValue(const Poco::Dynamic::Var& value)
        : m_value(value)
    { }

    /**
    * @brief   Create a new PocoJsonFrozenValue instance that contains the
    *          value referenced by this PocoJsonValue instance.
    *
    * @returns pointer to a new PocoJsonFrozenValue instance, belonging to the
    *          caller.
    */
    FrozenValue * freeze() const
    {
        return new PocoJsonFrozenValue(m_value);
    }

    /**
    * @brief   Optionally return a PocoJsonArray instance.
    *
    * If the referenced PocoJson value is an array, this function will return
    * a std::optional containing a PocoJsonArray instance referencing the
    * array.
    *
    * Otherwise it will return an empty optional.
    */
    opt::optional<PocoJsonArray> getArrayOptional() const
    {
        if (m_value.type() == typeid(Poco::JSON::Array::Ptr)) {
            return opt::make_optional(PocoJsonArray(m_value));
        }

        return opt::optional<PocoJsonArray>();
    }

    /**
    * @brief   Retrieve the number of elements in the array
    *
    * If the referenced PocoJson value is an array, this function will
    * retrieve the number of elements in the array and store it in the output
    * variable provided.
    *
    * @param   result  reference to size_t to set with result
    *
    * @returns true if the number of elements was retrieved, false otherwise.
    */
    bool getArraySize(size_t &result) const
    {
        if (m_value.type() == typeid(Poco::JSON::Array::Ptr)) {
            result = m_value.extract<Poco::JSON::Array::Ptr>()->size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.isBoolean()) {
            result = m_value.convert<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.isNumeric() && !m_value.isInteger()) {
            result = m_value.convert<double>();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_value.isInteger()) {
            result = m_value.convert<int>();
            return true;
        }
        return false;
    }

    /**
    * @brief   Optionally return a PocoJsonObject instance.
    *
    * If the referenced PocoJson value is an object, this function will return a
    * std::optional containing a PocoJsonObject instance referencing the
    * object.
    *
    * Otherwise it will return an empty optional.
    */
    opt::optional<PocoJsonObject> getObjectOptional() const
    {
        if (m_value.type() == typeid(Poco::JSON::Object::Ptr)) {
            return opt::make_optional(PocoJsonObject(m_value));
        }

        return opt::optional<PocoJsonObject>();
    }

    /**
    * @brief   Retrieve the number of members in the object
    *
    * If the referenced PocoJson value is an object, this function will
    * retrieve the number of members in the object and store it in the output
    * variable provided.
    *
    * @param   result  reference to size_t to set with result
    *
    * @returns true if the number of members was retrieved, false otherwise.
    */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.type() == typeid(Poco::JSON::Object::Ptr)) {
            result = m_value.extract<Poco::JSON::Object::Ptr>()->size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.isString()) {
            result = m_value.convert<std::string>();
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
        return m_value.type() == typeid(Poco::JSON::Array::Ptr);
    }

    bool isBool() const
    {
        return m_value.isBoolean();
    }

    bool isDouble() const
    {
        return m_value.isNumeric() && !m_value.isInteger();
    }

    bool isInteger() const
    {
        return !isBool() && m_value.isInteger();
    }

    bool isNull() const
    {
        return m_value.isEmpty();
    }

    bool isNumber() const
    {
        return m_value.isNumeric();
    }

    bool isObject() const
    {
        return m_value.type() == typeid(Poco::JSON::Object::Ptr);
    }

    bool isString() const
    {
        return m_value.isString();
    }

private:

    /// Return an empty object
    static Poco::Dynamic::Var emptyObject()
    {
        Poco::Dynamic::Var object = Poco::JSON::Object::Ptr();
        return object;
    }

    /// Contained value
    Poco::Dynamic::Var m_value;
};

/**
* @brief   An implementation of the Adapter interface supporting PocoJson.
*
* This class is defined in terms of the BasicAdapter template class, which
* helps to ensure that all of the Adapter implementations behave consistently.
*
* @see Adapter
* @see BasicAdapter
*/
class PocoJsonAdapter :
    public BasicAdapter<PocoJsonAdapter,
    PocoJsonArray,
    PocoJsonObjectMember,
    PocoJsonObject,
    PocoJsonValue>
{
public:
    /// Construct a PocoJsonAdapter that contains an empty object
    PocoJsonAdapter()
        : BasicAdapter()
    { }

    /// Construct a PocoJsonAdapter containing a specific Poco Json object
    PocoJsonAdapter(const Poco::Dynamic::Var &value)
        : BasicAdapter(PocoJsonValue {value})
    { }
};

/**
* @brief   Class for iterating over values held in a JSON array.
*
* This class provides a JSON array iterator that dereferences as an instance of
* PocoJsonAdapter representing a value stored in the array. It has been
* implemented using the boost iterator_facade template.
*
* @see PocoJsonArray
*/
class PocoJsonArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PocoJsonAdapter;
    using difference_type = PocoJsonAdapter;
    using pointer = PocoJsonAdapter*;
    using reference = PocoJsonAdapter&;

    /**
    * @brief   Construct a new PocoJsonArrayValueIterator using an existing
    *          PocoJson iterator.
    *
    * @param   itr  PocoJson iterator to store
    */
    PocoJsonArrayValueIterator(const Poco::JSON::Array::ConstIterator &itr)
        : m_itr(itr)
    { }

    /// Returns a PocoJsonAdapter that contains the value of the current
    /// element.
    PocoJsonAdapter operator*() const
    {
        return PocoJsonAdapter(*m_itr);
    }

    DerefProxy<PocoJsonAdapter> operator->() const
    {
        return DerefProxy<PocoJsonAdapter>(**this);
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
    bool operator==(const PocoJsonArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const PocoJsonArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const PocoJsonArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PocoJsonArrayValueIterator operator++(int)
    {
        PocoJsonArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PocoJsonArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

private:
    Poco::JSON::Array::ConstIterator m_itr;
};


/**
* @brief   Class for iterating over the members belonging to a JSON object.
*
* This class provides a JSON object iterator that dereferences as an instance
* of PocoJsonObjectMember representing one of the members of the object. It
* has been implemented using the boost iterator_facade template.
*
* @see PocoJsonObject
* @see PocoJsonObjectMember
*/
class PocoJsonObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PocoJsonObjectMember;
    using difference_type = PocoJsonObjectMember;
    using pointer = PocoJsonObjectMember*;
    using reference = PocoJsonObjectMember&;

    /**
    * @brief   Construct an iterator from a PocoJson iterator.
    *
    * @param   itr  PocoJson iterator to store
    */
    PocoJsonObjectMemberIterator(const Poco::JSON::Object::ConstIterator &itr)
        : m_itr(itr)
    { }

    /**
    * @brief   Returns a PocoJsonObjectMember that contains the key and value
    *          belonging to the object member identified by the iterator.
    */
    PocoJsonObjectMember operator*() const
    {
        return PocoJsonObjectMember(m_itr->first, m_itr->second);
    }

    DerefProxy<PocoJsonObjectMember> operator->() const
    {
        return DerefProxy<PocoJsonObjectMember>(**this);
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
    bool operator==(const PocoJsonObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const PocoJsonObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const PocoJsonObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PocoJsonObjectMemberIterator operator++(int)
    {
        PocoJsonObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PocoJsonObjectMemberIterator& operator--()
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original PocoJson iterator
    Poco::JSON::Object::ConstIterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for PocoJsonAdapter.
template<>
struct AdapterTraits<valijson::adapters::PocoJsonAdapter>
{
    typedef Poco::Dynamic::Var DocumentType;

    static std::string adapterName()
    {
        return "PocoJsonAdapter";
    }
};

inline PocoJsonArrayValueIterator PocoJsonArray::begin() const
{
    return m_value.extract<Poco::JSON::Array::Ptr>()->begin();
}

inline PocoJsonArrayValueIterator PocoJsonArray::end() const
{
    return m_value.extract<Poco::JSON::Array::Ptr>()->end();
}

inline PocoJsonObjectMemberIterator PocoJsonObject::begin() const
{
    return m_value.extract<Poco::JSON::Object::Ptr>()->begin();
}

inline PocoJsonObjectMemberIterator PocoJsonObject::end() const
{
    return m_value.extract<Poco::JSON::Object::Ptr>()->end();
}

inline PocoJsonObjectMemberIterator PocoJsonObject::find(const std::string &propertyName) const
{
    auto& ptr = m_value.extract<Poco::JSON::Object::Ptr>();

    auto it = std::find_if(ptr->begin(), ptr->end(), [&propertyName](const Poco::JSON::Object::ValueType& p) {
        return p.first == propertyName;
    });
    return it;
}

inline bool PocoJsonFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return PocoJsonAdapter(m_value).equalTo(other, strict);
}

}  // namespace adapters
}  // namespace valijson

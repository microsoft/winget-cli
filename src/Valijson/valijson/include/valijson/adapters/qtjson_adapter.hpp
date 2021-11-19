/**
 * @file
 *
 * @brief   Adapter implementation for the QtJson parser library.
 *
 * Include this file in your program to enable support for QtJson.
 *
 * This file defines the following classes (not in this order):
 *  - QtJsonAdapter
 *  - QtJsonArray
 *  - QtJsonArrayValueIterator
 *  - QtJsonFrozenValue
 *  - QtJsonObject
 *  - QtJsonObjectMember
 *  - QtJsonObjectMemberIterator
 *  - QtJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is QtJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>
#include <utility>

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class QtJsonAdapter;
class QtJsonArrayValueIterator;
class QtJsonObjectMemberIterator;

typedef std::pair<std::string, QtJsonAdapter> QtJsonObjectMember;

/**
 * @brief  Light weight wrapper for a QtJson array value.
 *
 * This class is light weight wrapper for a QtJson array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * QtJson value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class QtJsonArray
{
public:

    typedef QtJsonArrayValueIterator const_iterator;
    typedef QtJsonArrayValueIterator iterator;

    /// Construct a QtJsonArray referencing an empty array.
    QtJsonArray()
      : m_value(emptyArray())
    {
    }

    /**
     * @brief   Construct a QtJsonArray referencing a specific QtJson
     *          value.
     *
     * @param   value   reference to a QtJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    explicit QtJsonArray(const QJsonValue &value)
      : m_value(value.toArray())
    {
        if (!value.isArray()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying QtJson implementation.
     */
    QtJsonArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying QtJson implementation.
     */
    QtJsonArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to a QtJson value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static QJsonArray emptyArray()
    {
        static const QJsonArray array;
        return array;
    }

    /// Reference to the contained value
    const QJsonArray m_value;
};

/**
 * @brief  Light weight wrapper for a QtJson object.
 *
 * This class is light weight wrapper for a QtJson object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * QtJson value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class QtJsonObject
{
public:

    typedef QtJsonObjectMemberIterator const_iterator;
    typedef QtJsonObjectMemberIterator iterator;

    /// Construct a QtJsonObject referencing an empty object singleton.
    QtJsonObject()
      : m_value(emptyObject())
    {
    }

    /**
     * @brief   Construct a QtJsonObject referencing a specific QtJson
     *          value.
     *
     * @param   value  reference to a QtJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    QtJsonObject(const QJsonValue &value)
      : m_value(value.toObject())
    {
        if (!value.isObject()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying QtJson implementation.
     */
    QtJsonObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying QtJson implementation.
     */
    QtJsonObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    QtJsonObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.size();
    }

private:

    /**
     * @brief   Return a reference to a QtJson value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const QJsonObject emptyObject()
    {
        static const QJsonObject object;
        return object;
    }

    /// Reference to the contained object
    const QJsonObject m_value;
};

/**
 * @brief   Stores an independent copy of a QtJson value.
 *
 * This class allows a QtJson value to be stored independent of its original
 * document. QtJson makes this easy to do, as it does not perform any
 * custom memory management.
 *
 * @see FrozenValue
 */
class QtJsonFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a QtJson value
     *
     * @param  source  the QtJson value to be copied
     */
    explicit QtJsonFrozenValue(QJsonValue source)
      : m_value(std::move(source)) { }

    FrozenValue * clone() const override
    {
        return new QtJsonFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored QtJson value
    QJsonValue m_value;
};

/**
 * @brief   Light weight wrapper for a QtJson value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a QtJson value. This class is responsible
 * for the mechanics of actually reading a QtJson value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class QtJsonValue
{
public:

    /// Construct a wrapper for the empty object singleton
    QtJsonValue()
      : m_value(emptyObject()) { }

    /// Construct a wrapper for a specific QtJson value
    QtJsonValue(QJsonValue value)
      : m_value(std::move(value)) { }

    /**
     * @brief   Create a new QtJsonFrozenValue instance that contains the
     *          value referenced by this QtJsonValue instance.
     *
     * @returns pointer to a new QtJsonFrozenValue instance, belonging to the
     *          caller.
     */
    FrozenValue * freeze() const
    {
        return new QtJsonFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a QtJsonArray instance.
     *
     * If the referenced QtJson value is an array, this function will return
     * a std::optional containing a QtJsonArray instance referencing the
     * array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<QtJsonArray> getArrayOptional() const
    {
        if (m_value.isArray()) {
            return opt::make_optional(QtJsonArray(m_value));
        }

        return opt::optional<QtJsonArray>();
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced QtJson value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.isArray()) {
            const QJsonArray array = m_value.toArray();
            result = array.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.isBool()) {
            result = m_value.toBool();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.isDouble()) {
            result = m_value.toDouble();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_value.isDouble()) {
            result = m_value.toInt();
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a QtJsonObject instance.
     *
     * If the referenced QtJson value is an object, this function will return a
     * std::optional containing a QtJsonObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<QtJsonObject> getObjectOptional() const
    {
        if (m_value.isObject()) {
            return opt::make_optional(QtJsonObject(m_value));
        }

        return opt::optional<QtJsonObject>();
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced QtJson value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.isObject()) {
            const QJsonObject &object = m_value.toObject();
            result = object.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.isString()) {
            result = m_value.toString().toStdString();
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
        return m_value.isArray();
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
        //toInt returns the default value (0, 1) if the value is not a whole number
        return m_value.isDouble() && (m_value.toInt(0) == m_value.toInt(1));
    }

    bool isNull() const
    {
        return m_value.isNull();
    }

    bool isNumber() const
    {
        return m_value.isDouble();
    }

    bool isObject() const
    {
        return m_value.isObject();
    }

    bool isString() const
    {
        return m_value.isString();
    }

private:

    /// Return a reference to an empty object singleton
    static QJsonValue emptyObject()
    {
        static const QJsonValue object;
        return object;
    }

    /// Reference to the contained QtJson value.
    const QJsonValue m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting QtJson.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class QtJsonAdapter:
    public BasicAdapter<QtJsonAdapter,
                        QtJsonArray,
                        QtJsonObjectMember,
                        QtJsonObject,
                        QtJsonValue>
{
public:

    /// Construct a QtJsonAdapter that contains an empty object
    QtJsonAdapter()
      : BasicAdapter() { }

    /// Construct a QtJsonAdapter containing a specific QtJson value
    QtJsonAdapter(const QJsonValue &value)
      : BasicAdapter(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * QtJsonAdapter representing a value stored in the array.
 *
 * @see QtJsonArray
 */
class QtJsonArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = QtJsonAdapter;
    using difference_type = QtJsonAdapter;
    using pointer = QtJsonAdapter*;
    using reference = QtJsonAdapter&;

    /**
     * @brief   Construct a new QtJsonArrayValueIterator using an existing
     *          QtJson iterator.
     *
     * @param   itr  QtJson iterator to store
     */
    QtJsonArrayValueIterator(const QJsonArray::const_iterator &itr)
      : m_itr(itr) { }

    /// Returns a QtJsonAdapter that contains the value of the current
    /// element.
    QtJsonAdapter operator*() const
    {
        return QtJsonAdapter(*m_itr);
    }

    DerefProxy<QtJsonAdapter> operator->() const
    {
        return DerefProxy<QtJsonAdapter>(**this);
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
    bool operator==(const QtJsonArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const QtJsonArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const QtJsonArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    QtJsonArrayValueIterator operator++(int)
    {
        QtJsonArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const QtJsonArrayValueIterator& operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

private:

    QJsonArray::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of QtJsonObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see QtJsonObject
 * @see QtJsonObjectMember
 */
class QtJsonObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = QtJsonObjectMember;
    using difference_type = QtJsonObjectMember;
    using pointer = QtJsonObjectMember*;
    using reference = QtJsonObjectMember&;

    /**
     * @brief   Construct an iterator from a QtJson iterator.
     *
     * @param   itr  QtJson iterator to store
     */
    QtJsonObjectMemberIterator(const QJsonObject::const_iterator &itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a QtJsonObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    QtJsonObjectMember operator*() const
    {
        std::string key = m_itr.key().toStdString();
        return QtJsonObjectMember(key, m_itr.value());
    }

    DerefProxy<QtJsonObjectMember> operator->() const
    {
        return DerefProxy<QtJsonObjectMember>(**this);
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
    bool operator==(const QtJsonObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const QtJsonObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const QtJsonObjectMemberIterator& operator++()
    {
        m_itr++;
        return *this;
    }

    QtJsonObjectMemberIterator operator++(int)
    {
        QtJsonObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const QtJsonObjectMemberIterator& operator--(int)
    {
        m_itr--;

        return *this;
    }

private:

    /// Iternal copy of the original QtJson iterator
    QJsonObject::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for QtJsonAdapter.
template<>
struct AdapterTraits<valijson::adapters::QtJsonAdapter>
{
    typedef QJsonValue DocumentType;

    static std::string adapterName()
    {
        return "QtJsonAdapter";
    }
};

inline bool QtJsonFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return QtJsonAdapter(m_value).equalTo(other, strict);
}

inline QtJsonArrayValueIterator QtJsonArray::begin() const
{
    return m_value.begin();
}

inline QtJsonArrayValueIterator QtJsonArray::end() const
{
    return m_value.end();
}

inline QtJsonObjectMemberIterator QtJsonObject::begin() const
{
    return m_value.begin();
}

inline QtJsonObjectMemberIterator QtJsonObject::end() const
{
    return m_value.end();
}

inline QtJsonObjectMemberIterator QtJsonObject::find(
    const std::string &propertyName) const
{
    return m_value.find(QString::fromStdString(propertyName));
}

}  // namespace adapters
}  // namespace valijson

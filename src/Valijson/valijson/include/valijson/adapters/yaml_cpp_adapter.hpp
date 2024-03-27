/**
 * @file
 *
 * @brief   Adapter implementation for the yaml-cpp parser library.
 *
 * Include this file in your program to enable support for yaml-cpp.
 *
 * This file defines the following classes (not in this order):
 *  - YamlCppAdapter
 *  - YamlCppArray
 *  - YamlCppArrayValueIterator
 *  - YamlCppFrozenValue
 *  - YamlCppObject
 *  - YamlCppObjectMember
 *  - YamlCppObjectMemberIterator
 *  - YamlCppValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is YamlCppAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

#include <utility>
#include <valijson/internal/adapter.hpp>
#include <valijson/internal/basic_adapter.hpp>
#include <valijson/internal/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class YamlCppAdapter;
class YamlCppArrayValueIterator;
class YamlCppObjectMemberIterator;

typedef std::pair<std::string, YamlCppAdapter> YamlCppObjectMember;

/**
 * @brief  Light weight wrapper for a YamlCpp array value.
 *
 * This class is light weight wrapper for a YamlCpp array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * YamlCpp value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class YamlCppArray
{
  public:
    typedef YamlCppArrayValueIterator const_iterator;
    typedef YamlCppArrayValueIterator iterator;

    /// Construct a YamlCppArray referencing an empty array.
    YamlCppArray() : m_value(emptyArray()) {}

    /**
     * @brief   Construct a YamlCppArray referencing a specific
     * YamlCpp value.
     *
     * @param   value   reference to a YamlCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    YamlCppArray(const YAML::Node &value) : m_value(value)
    {
        if (!value.IsSequence()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying YamlCpp implementation.
     */
    YamlCppArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying YamlCpp implementation.
     */
    YamlCppArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.size();
    }

  private:
    /**
     * @brief   Return a reference to a YamlCpp value that is an empty
     * array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const YAML::Node &emptyArray()
    {
        static const YAML::Node array = YAML::Node(YAML::NodeType::Sequence);
        return array;
    }

    /// Reference to the contained value
    const YAML::Node m_value;
};

/**
 * @brief  Light weight wrapper for a YamlCpp object.
 *
 * This class is light weight wrapper for a YamlCpp object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * YamlCpp value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class YamlCppObject
{
  public:
    typedef YamlCppObjectMemberIterator const_iterator;
    typedef YamlCppObjectMemberIterator iterator;

    /// Construct a YamlCppObject referencing an empty object singleton.
    YamlCppObject() : m_value(emptyObject()) {}

    /**
     * @brief   Construct a YamlCppObject referencing a specific
     * YamlCpp value.
     *
     * @param   value  reference to a YamlCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    YamlCppObject(const YAML::Node &value) : m_value(value)
    {
        if (!value.IsMap()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying YamlCpp
     * implementation.
     */
    YamlCppObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying YamlCpp
     * implementation.
     */
    YamlCppObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    YamlCppObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.size();
    }

  private:
    /**
     * @brief   Return a reference to a YamlCpp value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const YAML::Node &emptyObject()
    {
        static const YAML::Node object = YAML::Node(YAML::NodeType::Map);
        return object;
    }

    /// Reference to the contained object
    const YAML::Node m_value;
};

/**
 * @brief   Stores an independent copy of a YamlCpp value.
 *
 * This class allows a YamlCpp value to be stored independent of its
 * original document.
 *
 * @see FrozenValue
 */
class YamlCppFrozenValue : public FrozenValue
{
  public:
    /**
     * @brief  Make a copy of a YamlCpp value
     *
     * @param  source  the YamlCpp value to be copied
     */
    explicit YamlCppFrozenValue(YAML::Node source)
        : m_value(YAML::Clone(source))
    {
    }

    FrozenValue *clone() const override
    {
        return new YamlCppFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

  private:
    /// Stored YamlCpp value
    YAML::Node m_value;
};

/**
 * @brief   Light weight wrapper for a YamlCpp value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a YamlCpp value. This class is
 * responsible for the mechanics of actually reading a YamlCpp value,
 * whereas the BasicAdapter class is responsible for the semantics of type
 * comparisons and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class YamlCppValue
{
  public:
    /// Construct a wrapper for the empty object singleton
    YamlCppValue() : m_value(emptyObject()) {}

    /// Construct a wrapper for a specific YamlCpp value
    YamlCppValue(const YAML::Node &value) : m_value(value) {}

    /**
     * @brief   Create a new YamlCppFrozenValue instance that contains the
     *          value referenced by this YamlCppValue instance.
     *
     * @returns pointer to a new YamlCppFrozenValue instance, belonging to
     * the caller.
     */
    FrozenValue *freeze() const
    {
        return new YamlCppFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a YamlCppArray instance.
     *
     * If the referenced YamlCpp value is an array, this function will
     * return a std::optional containing a YamlCppArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<YamlCppArray> getArrayOptional() const
    {
        if (m_value.IsSequence()) {
            return opt::make_optional(YamlCppArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced YamlCpp value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.IsSequence()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.IsScalar()) {
            result = m_value.as<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_value.IsScalar()) {
            result = m_value.as<double>();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_value.IsScalar()) {
            result = m_value.as<int64_t>();
            return true;
        }
        return false;
    }

    /**
     * @brief   Optionally return a YamlCppObject instance.
     *
     * If the referenced YamlCpp value is an object, this function will
     * return a std::optional containing a YamlCppObject instance
     * referencing the object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<YamlCppObject> getObjectOptional() const
    {
        if (m_value.IsMap()) {
            return opt::make_optional(YamlCppObject(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced YamlCpp value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.IsMap()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.IsScalar()) {
            result = m_value.as<std::string>();
            return true;
        }

        return false;
    }

    static bool hasStrictTypes()
    {
        return false;
    }

    bool isArray() const
    {
        return m_value.IsSequence();
    }

    bool isBool() const
    {
        return false;
    }

    bool isDouble() const
    {
        return false;
    }

    bool isInteger() const
    {
        return false;
    }

    bool isNull() const
    {
        return m_value.IsNull();
    }

    bool isNumber() const
    {
        return false;
    }

    bool isObject() const
    {
        return m_value.IsMap();
    }

    bool isString() const
    {
        return true;
    }

  private:
    /// Return a reference to an empty object singleton
    static const YAML::Node &emptyObject()
    {
        static const YAML::Node object = YAML::Node(YAML::NodeType::Map);
        return object;
    }

    /// Reference to the contained YamlCpp value.
    const YAML::Node m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting YamlCpp.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class YamlCppAdapter
    : public BasicAdapter<YamlCppAdapter, YamlCppArray, YamlCppObjectMember,
                          YamlCppObject, YamlCppValue>
{
  public:
    /// Construct a YamlCppAdapter that contains an empty object
    YamlCppAdapter() : BasicAdapter() {}

    /// Construct a YamlCppAdapter containing a specific Nlohmann Json
    /// object
    YamlCppAdapter(const YAML::Node &value) : BasicAdapter(YamlCppValue{value})
    {
    }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * YamlCppAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see YamlCppArray
 */
class YamlCppArrayValueIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = YamlCppAdapter;
    using difference_type = YamlCppAdapter;
    using pointer = YamlCppAdapter *;
    using reference = YamlCppAdapter &;

    /**
     * @brief   Construct a new YamlCppArrayValueIterator using an existing
     *          YamlCpp iterator.
     *
     * @param   itr  YamlCpp iterator to store
     */
    YamlCppArrayValueIterator(const YAML::Node::const_iterator &itr)
        : m_itr(itr)
    {
    }

    /// Returns a YamlCppAdapter that contains the value of the current
    /// element.
    YamlCppAdapter operator*() const
    {
        return YamlCppAdapter(*m_itr);
    }

    DerefProxy<YamlCppAdapter> operator->() const
    {
        return DerefProxy<YamlCppAdapter>(**this);
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
    bool operator==(const YamlCppArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const YamlCppArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const YamlCppArrayValueIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    YamlCppArrayValueIterator operator++(int)
    {
        YamlCppArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    void advance(std::ptrdiff_t n)
    {
        for (auto i = 0; i < n; ++i)
            m_itr++;
    }

  private:
    YAML::Node::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of YamlCppObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see YamlCppObject
 * @see YamlCppObjectMember
 */
class YamlCppObjectMemberIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = YamlCppObjectMember;
    using difference_type = YamlCppObjectMember;
    using pointer = YamlCppObjectMember *;
    using reference = YamlCppObjectMember &;

    /**
     * @brief   Construct an iterator from a YamlCpp iterator.
     *
     * @param   itr  YamlCpp iterator to store
     */
    YamlCppObjectMemberIterator(const YAML::Node::const_iterator &itr)
        : m_itr(itr)
    {
    }

    /**
     * @brief   Returns a YamlCppObjectMember that contains the key and
     * value belonging to the object member identified by the iterator.
     */
    YamlCppObjectMember operator*() const
    {
        return YamlCppObjectMember(m_itr->first.as<std::string>(),
                                   m_itr->second);
    }

    DerefProxy<YamlCppObjectMember> operator->() const
    {
        return DerefProxy<YamlCppObjectMember>(**this);
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
    bool operator==(const YamlCppObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const YamlCppObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const YamlCppObjectMemberIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    YamlCppObjectMemberIterator operator++(int)
    {
        YamlCppObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

  private:
    /// Iternal copy of the original YamlCpp iterator
    YAML::Node::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for YamlCppAdapter.
template <> struct AdapterTraits<valijson::adapters::YamlCppAdapter>
{
    typedef YAML::Node DocumentType;

    static std::string adapterName()
    {
        return "YamlCppAdapter";
    }
};

inline bool YamlCppFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return YamlCppAdapter(m_value).equalTo(other, strict);
}

inline YamlCppArrayValueIterator YamlCppArray::begin() const
{
    return m_value.begin();
}

inline YamlCppArrayValueIterator YamlCppArray::end() const
{
    return m_value.end();
}

inline YamlCppObjectMemberIterator YamlCppObject::begin() const
{
    return m_value.begin();
}

inline YamlCppObjectMemberIterator YamlCppObject::end() const
{
    return m_value.end();
}

inline YamlCppObjectMemberIterator
YamlCppObject::find(const std::string &propertyName) const
{
    YAML::Node result = m_value[propertyName];
    if (!result.IsDefined())
        return end();

    // yaml-cpp does not offer an iterator-based lookup,
    // so instead we create a new placeholder object and
    // return an iterator to that container instead.
    YAML::Node wrapper = YAML::Node(YAML::NodeType::Map);
    wrapper[propertyName] = result;
    return YamlCppObjectMemberIterator(wrapper.begin());
}

} // namespace adapters
} // namespace valijson

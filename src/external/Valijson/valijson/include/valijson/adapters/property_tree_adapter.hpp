/**
 * @file
 *
 * @brief   Adapter implementation for the Boost property tree library.
 *
 * Include this file in your program to enable support for boost property trees.
 *
 * This file defines the following classes (not in this order):
 *  - PropertyTreeAdapter
 *  - PropertyTreeArray
 *  - PropertyTreeArrayValueIterator
 *  - PropertyTreeFrozenValue
 *  - PropertyTreeObject
 *  - PropertyTreeObjectMember
 *  - PropertyTreeObjectMemberIterator
 *  - PropertyTreeValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is PropertyTreeAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>

namespace valijson {
namespace adapters {

class PropertyTreeAdapter;
class PropertyTreeArrayValueIterator;
class PropertyTreeObjectMemberIterator;

typedef std::pair<std::string, PropertyTreeAdapter> PropertyTreeObjectMember;

/**
 * @brief   Light weight wrapper for a Boost property tree that contains
 *          array-like data.
 *
 * This class is light weight wrapper for a Boost property tree. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to a Boost property
 * tree that is assumed to contain unnamed key-value pairs. There is very little
 * associated with copy construction and passing by value.
 */
class PropertyTreeArray
{
public:

    typedef PropertyTreeArrayValueIterator const_iterator;
    typedef PropertyTreeArrayValueIterator iterator;

    /// Construct a PropertyTreeArray referencing an empty property tree
    /// singleton.
    PropertyTreeArray()
      : m_array(emptyTree()) { }

    /**
     * @brief   Construct PropertyTreeArray referencing a specific Boost
     *          property tree.
     *
     * @param   array   reference to a property tree containing an array
     *
     * It is assumed that this value contains array-like data, but this is not
     * checked due to runtime cost.
     */
    explicit PropertyTreeArray(const boost::property_tree::ptree &array)
      : m_array(array) { }

    /// Return an iterator for the first element in the array.
    PropertyTreeArrayValueIterator begin() const;

    /// Return an iterator for one-past the last element of the array.
    PropertyTreeArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_array.size();
    }

private:

    /**
     * @brief   Return a reference to a property tree that looks like an
     *          empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const boost::property_tree::ptree & emptyTree()
    {
        static const boost::property_tree::ptree tree;
        return tree;
    }

    /// Reference to the contained value
    const boost::property_tree::ptree &m_array;
};

/**
 * @brief  Light weight wrapper for a Boost property tree that contains
 *         object-like data.
 *
 * This class is light weight wrapper for a Boost property tree. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * property tree value, assumed to be object-like, so there is very little
 * overhead associated with copy construction and passing by value.
 */
class PropertyTreeObject
{
public:

    typedef PropertyTreeObjectMemberIterator const_iterator;
    typedef PropertyTreeObjectMemberIterator iterator;

    /// Construct a PropertyTreeObject referencing an empty property tree.
    PropertyTreeObject()
      : m_object(emptyTree()) { }

    /**
     * @brief   Construct a PropertyTreeObject referencing a specific property
     *          tree.
     *
     * @param   object  reference to a property tree containing an object
     *
     * Note that the value of the property tree is not checked, due to the
     * runtime cost of doing so.
     */
    PropertyTreeObject(const boost::property_tree::ptree &object)
      : m_object(object) { }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying property tree
     * implementation.
     */
    PropertyTreeObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying property tree
     * implementation.
     */
    PropertyTreeObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   property   property name to search for
     */
    PropertyTreeObjectMemberIterator find(const std::string &property) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_object.size();
    }

private:

    /**
     * @brief   Return a reference to an empty property tree.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const boost::property_tree::ptree & emptyTree()
    {
        static const boost::property_tree::ptree tree;
        return tree;
    }

    /// Reference to the contained object
    const boost::property_tree::ptree &m_object;

};

/**
 * @brief   Stores an independent copy of a Boost property tree.
 *
 * This class allows a property tree value to be stored independent of its
 * original 'document'. Boost property trees make this easy to do, as they do
 * not perform any custom memory management.
 *
 * @see FrozenValue
 */
class PropertyTreeFrozenValue: public FrozenValue
{
public:

    /**
     * @brief  Make a copy of a Boost property tree POD value
     *
     * @param  source  string containing the POD vlaue
     */
    explicit PropertyTreeFrozenValue(const boost::property_tree::ptree::data_type &source)
      : m_value(source) { }

    /**
     * @brief  Make a copy of a Boost property tree object or array value
     *
     * @param  source  the property tree to be copied
     */
    explicit PropertyTreeFrozenValue(const boost::property_tree::ptree &source)
      : m_value(source) { }

    FrozenValue * clone() const override
    {
        return new PropertyTreeFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:

    /// Stored value
    boost::property_tree::ptree m_value;
};

/**
 * @brief   Light weight wrapper for a Boost property tree.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a Boost property tree value. This class
 * is responsible for the mechanics of actually reading a property tree, whereas
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class PropertyTreeValue
{
public:

    /// Construct a wrapper for an empty property tree
    PropertyTreeValue()
      : m_object(emptyTree()) { }

    /**
     * @brief  Construct a PropertyTreeValue from a tree object
     *
     * This function will determine whether the tree object represents an array
     * or an object by scanning the key names for any non-empty strings. In the
     * case of an empty tree object, it is not possible to determine whether it
     * is an array or an object, so it will be treated as an array by default.
     * Empty arrays are considered equal to empty objects when compared using
     * non-strict type comparison. Empty strings will also be stored as empty
     * arrays.
     *
     * @param  tree  Tree object to be wrapped
     */
    PropertyTreeValue(const boost::property_tree::ptree &tree)
    {
        if (tree.data().empty()) {    // No string content
            if (tree.empty()) {   // No children
                m_array.emplace(tree);         // Treat as empty array
            } else {
                bool isArray = true;
                for (const auto &node : tree) {
                    if (!node.first.empty()) {
                        isArray = false;
                        break;
                    }
                }

                if (isArray) {
                    m_array.emplace(tree);
                } else {
                    m_object.emplace(tree);
                }
            }
        } else {
            m_value = tree.data();
        }
    }

    /**
     * @brief   Create a new PropertyTreeFrozenValue instance that contains the
     *          value referenced by this PropertyTreeValue instance.
     *
     * @returns pointer to a new PropertyTreeFrozenValue instance, belonging to
     *          the caller.
     */
    FrozenValue* freeze() const
    {
        if (m_array) {
            return new PropertyTreeFrozenValue(*m_array);
        } else if (m_object) {
            return new PropertyTreeFrozenValue(*m_object);
        } else {
            return new PropertyTreeFrozenValue(*m_value);
        }
    }

    /**
     * @brief  Return an instance of PropertyTreeArrayAdapter.
     *
     * If the referenced property tree value is an array, this function will
     * return a std::optional containing a PropertyTreeArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<PropertyTreeArray> getArrayOptional() const
    {
        if (m_array) {
            return opt::make_optional(PropertyTreeArray(*m_array));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced property tree value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_array) {
            result = m_array->size();
            return true;
        }

        return false;
    }

    static bool getBool(bool &)
    {
        return false;
    }

    static bool getDouble(double &)
    {
        return false;
    }

    static bool getInteger(int64_t &)
    {
        return false;
    }

    /**
     * @brief   Optionally return a PropertyTreeObject instance.
     *
     * If the referenced property tree is an object, this function will return a
     * std::optional containing a PropertyTreeObject instance referencing the
     * object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<PropertyTreeObject> getObjectOptional() const
    {
        if (m_object) {
            return opt::make_optional(PropertyTreeObject(*m_object));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced property tree value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_object) {
            result = m_object->size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value) {
            result = *m_value;
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
        return static_cast<bool>(m_array);
    }

    static bool isBool()
    {
        return false;
    }

    static bool isDouble()
    {
        return false;
    }

    static bool isInteger()
    {
        return false;
    }

    static bool isNull()
    {
        return false;
    }

    static bool isNumber()
    {
        return false;
    }

    bool isObject() const
    {
        return static_cast<bool>(m_object);
    }

    bool isString() const
    {
        return static_cast<bool>(m_value);
    }

private:

    static const boost::property_tree::ptree & emptyTree()
    {
        static const boost::property_tree::ptree tree;
        return tree;
    }

    /// Reference used if the value is known to be an array
    opt::optional<const boost::property_tree::ptree &> m_array;

    /// Reference used if the value is known to be an object
    opt::optional<const boost::property_tree::ptree &> m_object;

    /// Reference used if the value is known to be a POD type
    opt::optional<std::string> m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting the Boost
 *          property tree library.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class PropertyTreeAdapter:
    public BasicAdapter<PropertyTreeAdapter,
                        PropertyTreeArray,
                        PropertyTreeObjectMember,
                        PropertyTreeObject,
                        PropertyTreeValue>
{
public:

    /// Construct a PropertyTreeAdapter for an empty property tree
    PropertyTreeAdapter()
      : BasicAdapter() { }

    /// Construct a PropertyTreeAdapter using a specific property tree
    PropertyTreeAdapter(const boost::property_tree::ptree &value)
      : BasicAdapter(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * PropertyTreeAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see PropertyTreeArray
 */
class PropertyTreeArrayValueIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PropertyTreeAdapter;
    using difference_type = PropertyTreeAdapter;
    using pointer = PropertyTreeAdapter*;
    using reference = PropertyTreeAdapter&;

    /**
     * @brief   Construct a new PropertyTreeArrayValueIterator using an existing
     *          property tree iterator.
     *
     * @param   itr  property tree iterator to store
     */
    PropertyTreeArrayValueIterator(
        const boost::property_tree::ptree::const_iterator &itr)
      : m_itr(itr) { }

    /// Returns a PropertyTreeAdapter that contains the value of the current
    /// element.
    PropertyTreeAdapter operator*() const
    {
        return PropertyTreeAdapter(m_itr->second);
    }

    DerefProxy<PropertyTreeAdapter> operator->() const
    {
        return DerefProxy<PropertyTreeAdapter>(**this);
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
    bool operator==(const PropertyTreeArrayValueIterator &rhs) const
    {
        return m_itr == rhs.m_itr;
    }

    bool operator!=(const PropertyTreeArrayValueIterator &rhs) const
    {
        return !(m_itr == rhs.m_itr);
    }

    const PropertyTreeArrayValueIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PropertyTreeArrayValueIterator operator++(int)
    {
        PropertyTreeArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PropertyTreeArrayValueIterator& operator--()
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

    boost::property_tree::ptree::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of PropertyTreeObjectMember representing one of the members of the object.
 * It has been implemented using the boost iterator_facade template.
 *
 * @see PropertyTreeObject
 * @see PropertyTreeObjectMember
 */
class PropertyTreeObjectMemberIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = PropertyTreeObjectMember;
    using difference_type = PropertyTreeObjectMember;
    using pointer = PropertyTreeObjectMember*;
    using reference = PropertyTreeObjectMember&;

    /**
     * @brief   Construct an iterator from a PropertyTree iterator.
     *
     * @param   itr  PropertyTree iterator to store
     */
    PropertyTreeObjectMemberIterator(
        boost::property_tree::ptree::const_assoc_iterator itr)
      : m_itr(itr) { }

    /**
     * @brief   Returns a PropertyTreeObjectMember that contains the key and
     *          value belonging to the object member identified by the iterator.
     */
    PropertyTreeObjectMember operator*() const
    {
        return PropertyTreeObjectMember(m_itr->first, m_itr->second);
    }

    DerefProxy<PropertyTreeObjectMember> operator->() const
    {
        return DerefProxy<PropertyTreeObjectMember>(**this);
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
    bool operator==(const PropertyTreeObjectMemberIterator &rhs) const
    {
        return m_itr == rhs.m_itr;
    }

    bool operator!=(const PropertyTreeObjectMemberIterator &rhs) const
    {
        return !(m_itr == rhs.m_itr);
    }

    const PropertyTreeObjectMemberIterator& operator++()
    {
        m_itr++;

        return *this;
    }

    PropertyTreeObjectMemberIterator operator++(int)
    {
        PropertyTreeObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const PropertyTreeObjectMemberIterator& operator--()
    {
        m_itr--;

        return *this;
    }

private:

    boost::property_tree::ptree::const_assoc_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for PropertyTreeAdapter.
template<>
struct AdapterTraits<valijson::adapters::PropertyTreeAdapter>
{
    typedef boost::property_tree::ptree DocumentType;

    static std::string adapterName()
    {
        return "PropertyTreeAdapter";
    }
};

inline bool PropertyTreeFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return PropertyTreeAdapter(m_value).equalTo(other, strict);
}

inline PropertyTreeArrayValueIterator PropertyTreeArray::begin() const
{
    return m_array.begin();
}

inline PropertyTreeArrayValueIterator PropertyTreeArray::end() const
{
    return m_array.end();
}

inline PropertyTreeObjectMemberIterator PropertyTreeObject::begin() const
{
    return m_object.ordered_begin();
}

inline PropertyTreeObjectMemberIterator PropertyTreeObject::end() const
{
    return m_object.not_found();
}

inline PropertyTreeObjectMemberIterator PropertyTreeObject::find(
    const std::string &propertyName) const
{
    const boost::property_tree::ptree::const_assoc_iterator itr = m_object.find(propertyName);

    if (itr != m_object.not_found()) {
        return itr;
    }

    return m_object.not_found();
}

}  // namespace adapters
}  // namespace valijson

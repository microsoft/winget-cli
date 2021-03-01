/**
 * @file
 *
 * @brief   Adapter implementation for the RapidJson parser library.
 *
 * Include this file in your program to enable support for RapidJson.
 *
 * This file defines the following template classes (not in this order):
 *  - GenericRapidJsonAdapter
 *  - GenericRapidJsonArray
 *  - GenericRapidJsonArrayValueIterator
 *  - GenericRapidJsonFrozenValue
 *  - GenericRapidJsonObject
 *  - GenericRapidJsonObjectMember
 *  - GenericRapidJsonObjectMemberIterator
 *  - GenericRapidJsonValue
 *
 * All of these classes share a template argument called 'ValueType', which can
 * be used to choose the underlying the RapidJson value type that is used. This
 * allows different RapidJson encodings and allocators to be used.
 *
 * This file also defines the following typedefs, which use RapidJson's default
 * ValueType:
 *  - RapidJsonAdapter
 *  - RapidJsonArray
 *  - RapidJsonArrayValueIterator
 *  - RapidJsonFrozenValue
 *  - RapidJsonObject
 *  - RapidJsonObjectMember
 *  - RapidJsonObjectMemberIterator
 *  - RapidJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is RapidJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <string>
#include <iterator>

#include <rapidjson/document.h>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>

namespace valijson {
namespace adapters {

template<class ValueType = rapidjson::Value>
class GenericRapidJsonAdapter;

template<class ValueType = rapidjson::Value>
class GenericRapidJsonArrayValueIterator;

template<class ValueType = rapidjson::Value>
class GenericRapidJsonObjectMemberIterator;

/// Container for a property name and an associated RapidJson value
template<class ValueType = rapidjson::Value>
class GenericRapidJsonObjectMember :
        public std::pair<std::string, GenericRapidJsonAdapter<ValueType> >
{
private:
    typedef std::pair<std::string, GenericRapidJsonAdapter<ValueType> > Super;

public:
    GenericRapidJsonObjectMember(
            const std::string &name,
            const GenericRapidJsonAdapter<ValueType> &value)
      : Super(name, value) { }
};

/**
 * @brief   Light weight wrapper for a RapidJson array value.
 *
 * This class is light weight wrapper for a RapidJson array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to an underlying
 * RapidJson value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
template<class ValueType = rapidjson::Value>
class GenericRapidJsonArray
{
public:

    typedef GenericRapidJsonArrayValueIterator<ValueType> const_iterator;
    typedef GenericRapidJsonArrayValueIterator<ValueType> iterator;

    /// Construct a RapidJsonArray referencing an empty array singleton.
    GenericRapidJsonArray()
      : value(emptyArray()) { }

    /**
     * @brief   Construct a RapidJsonArray referencing a specific RapidJson
     *          value.
     *
     * @param   value   reference to a RapidJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    GenericRapidJsonArray(const ValueType &value)
      : value(value)
    {
        if (!value.IsArray()) {
            throw std::runtime_error("Value is not an array.");
        }
    }

    /// Return an iterator for the first element in the array.
    iterator begin() const;

    /// Return an iterator for one-past the last element of the array.
    iterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return value.Size();
    }

private:

    /**
     * @brief   Return a reference to a RapidJson value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const ValueType & emptyArray()
    {
        static const ValueType array(rapidjson::kArrayType);
        return array;
    }

    /// Reference to the contained value
    const ValueType &value;
};

/**
 * @brief  Light weight wrapper for a RapidJson object.
 *
 * This class is light weight wrapper for a RapidJson object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * RapidJson value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
template <class ValueType = rapidjson::Value>
class GenericRapidJsonObject
{
public:

    typedef GenericRapidJsonObjectMemberIterator<ValueType> const_iterator;
    typedef GenericRapidJsonObjectMemberIterator<ValueType> iterator;

    /// Construct a GenericRapidJsonObject referencing an empty object singleton.
    GenericRapidJsonObject()
      : value(emptyObject()) { }

    /**
     * @brief   Construct a GenericRapidJsonObject referencing a specific
     *          RapidJson value.
     *
     * @param   value  reference to a RapidJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    GenericRapidJsonObject(const ValueType &value)
      : value(value)
    {
        if (!value.IsObject()) {
            throw std::runtime_error("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying RapidJson implementation.
     */
    iterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying RapidJson implementation.
     */
    iterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   property   property name to search for
     */
    iterator find(const std::string &property) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return value.MemberEnd() - value.MemberBegin();
    }

private:

    /**
     * @brief   Return a reference to a RapidJson value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const ValueType & emptyObject()
    {
        static ValueType object(rapidjson::kObjectType);
        return object;
    }

    /// Reference to the contained object
    const ValueType &value;
};

/**
 * @brief   Stores an independent copy of a RapidJson value.
 *
 * This class allows a RapidJson value to be stored independent of its original
 * document. RapidJson makes this a bit harder than usual, because RapidJson
 * values are associated with a custom memory allocator. As such, RapidJson
 * values have to be copied recursively, referencing a custom allocator held
 * by this class.
 *
 * @see FrozenValue
 */
template<class ValueType = rapidjson::Value>
class GenericRapidJsonFrozenValue: public FrozenValue
{
public:

    explicit GenericRapidJsonFrozenValue(const char *str)
    {
        value.SetString(str, allocator);
    }

    explicit GenericRapidJsonFrozenValue(const std::string &str)
    {
        value.SetString(str.c_str(), (unsigned int)str.length(), allocator);
    }

    /**
     * @brief   Make a copy of a RapidJson value
     *
     * @param   source  the RapidJson value to be copied
     */
    explicit GenericRapidJsonFrozenValue(const ValueType &source)
    {
        if (!copy(source, value, allocator)) {
            throw std::runtime_error("Failed to copy ValueType");
        }
    }

    virtual FrozenValue * clone() const
    {
        return new GenericRapidJsonFrozenValue(value);
    }

    virtual bool equalTo(const Adapter &other, bool strict) const;

private:

    /**
     * @brief   Recursively copy a RapidJson value using a separate allocator
     *
     * @param   source      value to copy from
     * @param   dest        value to copy into
     * @param   allocator   reference to an allocator held by this class
     *
     * @tparam  Allocator   type of RapidJson Allocator to be used
     *
     * @returns true if copied successfully, false otherwise.
     */
    template<typename Allocator>
    static bool copy(const ValueType &source,
                     ValueType &dest,
                     Allocator &allocator)
    {
        switch (source.GetType()) {
        case rapidjson::kNullType:
            dest.SetNull();
            return true;
        case rapidjson::kFalseType:
            dest.SetBool(false);
            return true;
        case rapidjson::kTrueType:
            dest.SetBool(true);
            return true;
        case rapidjson::kObjectType:
            dest.SetObject();
            for (typename ValueType::ConstMemberIterator itr = source.MemberBegin();
                itr != source.MemberEnd(); ++itr) {
                ValueType name(itr->name.GetString(), itr->name.GetStringLength(), allocator);
                ValueType value;
                copy(itr->value, value, allocator);
                dest.AddMember(name, value, allocator);
            }
            return true;
        case rapidjson::kArrayType:
            dest.SetArray();
            for (typename ValueType::ConstValueIterator itr = source.Begin(); itr != source.End(); ++itr) {
                ValueType value;
                copy(*itr, value, allocator);
                dest.PushBack(value, allocator);
            }
            return true;
        case rapidjson::kStringType:
            dest.SetString(source.GetString(), source.GetStringLength(), allocator);
            return true;
        case rapidjson::kNumberType:
            if (source.IsInt()) {
                dest.SetInt(source.GetInt());
            } else if (source.IsUint()) {
                dest.SetUint(source.GetUint());
            } else if (source.IsInt64()) {
                dest.SetInt64(source.GetInt64());
            } else if (source.IsUint64()) {
                dest.SetUint64(source.GetUint64());
            } else {
                dest.SetDouble(source.GetDouble());
            }
            return true;
        default:
            break;
        }

        return false;
    }

    /// Local memory allocator for RapidJson value
    typename ValueType::AllocatorType allocator;

    /// Local RapidJson value
    ValueType value;
};

/**
 * @brief   Light weight wrapper for a RapidJson value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a RapidJson value. This class is responsible
 * for the mechanics of actually reading a RapidJson value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
template<class ValueType = rapidjson::Value>
class GenericRapidJsonValue
{
public:
    /// Construct a wrapper for the empty object singleton
    GenericRapidJsonValue()
      : value(emptyObject()) { }

    /// Construct a wrapper for a specific RapidJson value
    GenericRapidJsonValue(const ValueType &value)
      : value(value) { }

    /**
     * @brief   Create a new GenericRapidJsonFrozenValue instance that contains
     *          the value referenced by this GenericRapidJsonValue instance.
     *
     * @returns pointer to a new GenericRapidJsonFrozenValue instance, belonging
     *          to the caller.
     */
    FrozenValue * freeze() const
    {
        return new GenericRapidJsonFrozenValue<ValueType>(value);
    }

    /**
     * @brief   Optionally return a GenericRapidJsonArray instance.
     *
     * If the referenced RapidJson value is an array, this function will return
     * a std::optional containing a GenericRapidJsonArray instance referencing
     * the array.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<GenericRapidJsonArray<ValueType> > getArrayOptional() const
    {
        if (value.IsArray()) {
            return opt::make_optional(GenericRapidJsonArray<ValueType>(value));
        }

        return opt::optional<GenericRapidJsonArray<ValueType> >();
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced RapidJson value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (value.IsArray()) {
            result = value.Size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (value.IsBool()) {
            result = value.GetBool();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (value.IsDouble()) {
            result = value.GetDouble();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (value.IsInt()) {
            result = value.GetInt();
            return true;
        } else if (value.IsInt64()) {
            result = value.GetInt64();
            return true;
        } else if (value.IsUint()) {
            result = static_cast<int64_t>(value.GetUint());
            return true;
        } else if (value.IsUint64()) {
            result = static_cast<int64_t>(value.GetUint64());
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a GenericRapidJsonObject instance.
     *
     * If the referenced RapidJson value is an object, this function will return
     * a std::optional containing a GenericRapidJsonObject instance
     * referencing the object.
     *
     * Otherwise it will return an empty optional.
     */
    opt::optional<GenericRapidJsonObject<ValueType> > getObjectOptional() const
    {
        if (value.IsObject()) {
            return opt::make_optional(GenericRapidJsonObject<ValueType>(value));
        }

        return opt::optional<GenericRapidJsonObject<ValueType> >();
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced RapidJson value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (value.IsObject()) {
            result = value.MemberEnd() - value.MemberBegin();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (value.IsString()) {
            result.assign(value.GetString(), value.GetStringLength());
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
        return value.IsArray();
    }

    bool isBool() const
    {
        return value.IsBool();
    }

    bool isDouble() const
    {
        return value.IsDouble();
    }

    bool isInteger() const
    {
        return value.IsInt() || value.IsInt64() || value.IsUint() ||
               value.IsUint64();
    }

    bool isNull() const
    {
        return value.IsNull();
    }

    bool isNumber() const
    {
        return value.IsNumber();
    }

    bool isObject() const
    {
        return value.IsObject();
    }

    bool isString() const
    {
        return value.IsString();
    }

private:

    /// Return a reference to an empty object singleton
    static const ValueType & emptyObject()
    {
        static const ValueType object(rapidjson::kObjectType);
        return object;
    }

    /// Reference to the contained RapidJson value.
    const ValueType &value;
};

/**
 * @brief   An implementation of the Adapter interface supporting RapidJson.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
template<class ValueType>
class GenericRapidJsonAdapter:
    public BasicAdapter<GenericRapidJsonAdapter<ValueType>,
                        GenericRapidJsonArray<ValueType>,
                        GenericRapidJsonObjectMember<ValueType>,
                        GenericRapidJsonObject<ValueType>,
                        GenericRapidJsonValue<ValueType> >
{
public:

    /// Construct a RapidJsonAdapter that contains an empty object
    GenericRapidJsonAdapter()
      : BasicAdapter<GenericRapidJsonAdapter<ValueType>,
                        GenericRapidJsonArray<ValueType>,
                        GenericRapidJsonObjectMember<ValueType>,
                        GenericRapidJsonObject<ValueType>,
                        GenericRapidJsonValue<ValueType> >() { }

    /// Construct a RapidJsonAdapter containing a specific RapidJson value
    GenericRapidJsonAdapter(const ValueType &value)
      : BasicAdapter<GenericRapidJsonAdapter<ValueType>,
                        GenericRapidJsonArray<ValueType>,
                        GenericRapidJsonObjectMember<ValueType>,
                        GenericRapidJsonObject<ValueType>,
                        GenericRapidJsonValue<ValueType> >(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * RapidJsonAdapter representing a value stored in the array. It has been
 * implemented using the std::iterator template.
 *
 * @see RapidJsonArray
 */
template<class ValueType>
class GenericRapidJsonArrayValueIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,                 // bi-directional iterator
        GenericRapidJsonAdapter<ValueType> >             // value type
{
public:

    /**
     * @brief   Construct a new GenericRapidJsonArrayValueIterator using an
     *          existing RapidJson iterator.
     *
     * @param   itr  RapidJson iterator to store
     */
    GenericRapidJsonArrayValueIterator(
        const typename ValueType::ConstValueIterator &itr)
      : itr(itr) { }

    /// Returns a GenericRapidJsonAdapter that contains the value of the current
    /// element.
    GenericRapidJsonAdapter<ValueType> operator*() const
    {
        return GenericRapidJsonAdapter<ValueType>(*itr);
    }

    /// Returns a proxy for the value of the current element
    DerefProxy<GenericRapidJsonAdapter<ValueType> > operator->() const
    {
        return DerefProxy<GenericRapidJsonAdapter<ValueType> >(**this);
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
    bool operator==(const GenericRapidJsonArrayValueIterator<ValueType> &other) const
    {
        return itr == other.itr;
    }

    bool operator!=(const GenericRapidJsonArrayValueIterator<ValueType>& other) const
    {
        return !(itr == other.itr);
    }

    GenericRapidJsonArrayValueIterator<ValueType>& operator++()
    {
        itr++;

        return *this;
    }

    GenericRapidJsonArrayValueIterator<ValueType> operator++(int) {
        GenericRapidJsonArrayValueIterator<ValueType> iterator_pre(itr);
        ++(*this);
        return iterator_pre;
    }

    GenericRapidJsonArrayValueIterator<ValueType>& operator--()
    {
        itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        itr += n;
    }

    std::ptrdiff_t difference(const GenericRapidJsonArrayValueIterator<ValueType> &other)
    {
        return std::distance(itr, other.itr);
    }

private:

    typename ValueType::ConstValueIterator itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of GenericRapidJsonObjectMember representing one of the members of the
 * object. It has been implemented using the std::iterator template.
 *
 * @see GenericRapidJsonObject
 * @see GenericRapidJsonObjectMember
 */
template<class ValueType>
class GenericRapidJsonObjectMemberIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,                 // bi-directional iterator
        GenericRapidJsonObjectMember<ValueType> >        // value type
{
public:

    /**
     * @brief   Construct an iterator from a RapidJson iterator.
     *
     * @param   itr  RapidJson iterator to store
     */
    GenericRapidJsonObjectMemberIterator(
        const typename ValueType::ConstMemberIterator &itr)
      : itr(itr) { }


    /**
     * @brief   Returns a GenericRapidJsonObjectMember that contains the key and
     *          value belonging to the object member identified by the iterator.
     */
    GenericRapidJsonObjectMember<ValueType> operator*() const
    {
        return GenericRapidJsonObjectMember<ValueType>(
            std::string(itr->name.GetString(), itr->name.GetStringLength()),
            itr->value);
    }

    /// Returns a proxy for the value of the current element
    DerefProxy<GenericRapidJsonObjectMember<ValueType> > operator->() const
    {
        return DerefProxy<GenericRapidJsonObjectMember<ValueType> >(**this);
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
    bool operator==(const GenericRapidJsonObjectMemberIterator<ValueType> &other) const
    {
        return itr == other.itr;
    }

    bool operator!=(const GenericRapidJsonObjectMemberIterator<ValueType> &other) const
    {
        return !(itr == other.itr);
    }

    GenericRapidJsonObjectMemberIterator<ValueType>& operator++()
    {
        itr++;

        return *this;
    }

    GenericRapidJsonObjectMemberIterator<ValueType> operator++(int)
    {
        GenericRapidJsonObjectMemberIterator<ValueType> iterator_pre(itr);
        ++(*this);
        return iterator_pre;
    }

    GenericRapidJsonObjectMemberIterator<ValueType>& operator--()
    {
        itr--;

        return *this;
    }

    std::ptrdiff_t difference(const GenericRapidJsonObjectMemberIterator &other)
    {
        return std::distance(itr, other.itr);
    }

private:

    /// Iternal copy of the original RapidJson iterator
    typename ValueType::ConstMemberIterator itr;
};

template<class ValueType>
inline bool GenericRapidJsonFrozenValue<ValueType>::equalTo(
        const Adapter &other, bool strict) const
{
    return GenericRapidJsonAdapter<ValueType>(value).equalTo(other, strict);
}

template<class ValueType>
inline typename GenericRapidJsonArray<ValueType>::iterator
        GenericRapidJsonArray<ValueType>::begin() const
{
    return value.Begin();
}

template<class ValueType>
inline typename GenericRapidJsonArray<ValueType>::iterator
        GenericRapidJsonArray<ValueType>::end() const
{
    return value.End();
}

template<class ValueType>
inline typename GenericRapidJsonObject<ValueType>::iterator
        GenericRapidJsonObject<ValueType>::begin() const
{
    return value.MemberBegin();
}

template<class ValueType>
inline typename GenericRapidJsonObject<ValueType>::iterator
        GenericRapidJsonObject<ValueType>::end() const
{
    return value.MemberEnd();
}

template<class ValueType>
inline typename GenericRapidJsonObject<ValueType>::iterator
        GenericRapidJsonObject<ValueType>::find(
                const std::string &propertyName) const
{
    // Hack to support older versions of rapidjson where pointers are used as
    // the built in iterator type. In those versions, the FindMember function
    // would return a null pointer when the requested member could not be
    // found. After calling FindMember on an empty object, we compare the
    // result against what we would expect if a non-null-pointer iterator was
    // returned.
    const ValueType empty(rapidjson::kObjectType);
    const typename ValueType::ConstMemberIterator maybeEnd = empty.FindMember("");
    if (maybeEnd != empty.MemberBegin() + 1) {
        // In addition to the pointer-based iterator issue, RapidJson's internal
        // string comparison code seemed to rely on the query string being
        // initialised to a length greater than or equal to that of the
        // properties being compared. We get around this by implementing our
        // own linear scan.
        const size_t propertyNameLength = propertyName.length();
        for (typename ValueType::ConstMemberIterator itr = value.MemberBegin();
                itr != value.MemberEnd(); ++itr) {
            const size_t memberNameLength = itr->name.GetStringLength();
            if (memberNameLength == propertyNameLength &&
                    strncmp(itr->name.GetString(), propertyName.c_str(),
                        itr->name.GetStringLength()) == 0) {
                return itr;
            }
        }

        return value.MemberEnd();
    }

    return value.FindMember(propertyName.c_str());      // Times are good.
}

typedef GenericRapidJsonAdapter<> RapidJsonAdapter;
typedef GenericRapidJsonArray<> RapidJsonArray;
typedef GenericRapidJsonArrayValueIterator<> RapidJsonArrayValue;
typedef GenericRapidJsonFrozenValue<> RapidJsonFrozenValue;
typedef GenericRapidJsonObject<> RapidJsonObject;
typedef GenericRapidJsonObjectMember<> RapidJsonObjectMember;
typedef GenericRapidJsonObjectMemberIterator<> RapidJsonObjectMemberIterator;
typedef GenericRapidJsonValue<> RapidJsonValue;

/**
 * @brief  Specialisation of the AdapterTraits template struct for a
 *         RapidJsonAdapter that uses a pool allocator
 */
template<>
struct AdapterTraits<valijson::adapters::RapidJsonAdapter>
{
    typedef rapidjson::Document DocumentType;

    static std::string adapterName()
    {
        return "RapidJsonAdapter";
    }
};

typedef rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator>
        RapidJsonCrt;

/**
 * @brief  Specialisation of the AdapterTraits template struct for a
 *         RapidJsonAdapter that uses the default CRT allocator
 */
template<>
struct AdapterTraits<valijson::adapters::GenericRapidJsonAdapter<RapidJsonCrt> >
{
    typedef rapidjson::GenericDocument<rapidjson::UTF8<>,
        rapidjson::CrtAllocator> DocumentType;

    static std::string adapterName()
    {
        return "GenericRapidJsonAdapter (using CrtAllocator)";
    }
};

}  // namespace adapters
}  // namespace valijson

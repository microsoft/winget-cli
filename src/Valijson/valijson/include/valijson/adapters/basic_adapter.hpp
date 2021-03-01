#pragma once

#include <stdint.h>
#include <sstream>

#include <valijson/adapters/adapter.hpp>
#include <valijson/internal/optional.hpp>

namespace valijson {
namespace adapters {

/**
 * @brief  A helper for the array and object member iterators.
 *
 * See http://www.stlsoft.org/doc-1.9/group__group____pattern____dereference__proxy.html
 * for motivation
 *
 * @tparam Value  Name of the value type
 */
template<class Value>
struct DerefProxy
{
    explicit DerefProxy(const Value& x)
      : m_ref(x) { }

    Value* operator->()
    {
        return std::addressof(m_ref);
    }

    operator Value*()
    {
        return std::addressof(m_ref);
    }

private:
    Value m_ref;
};

/**
 * @brief  Template class that implements the expected semantics of an Adapter.
 *
 * Implementing all of the type-casting functionality for each Adapter is error
 * prone and tedious, so this template class aims to minimise the duplication
 * of code between various Adapter implementations. This template doesn't quite
 * succeed in removing all duplication, but it has greatly simplified the
 * implementation of a new Adapter by encapsulating the type-casting semantics
 * and a lot of the trivial functionality associated with the Adapter interface.
 *
 * By inheriting from this template class, Adapter implementations will inherit
 * the exception throwing behaviour that is expected by other parts of the
 * Valijson library.
 *
 * @tparam  AdapterType       Self-referential name of the Adapter being
 *                            specialised.
 * @tparam  ArrayType         Name of the type that will be returned by the
 *                            getArray() function. Instances of this type should
 *                            provide begin(), end() and size() functions so
 *                            that it is possible to iterate over the values in
 *                            the array.
 * @tparam  ObjectMemberType  Name of the type exposed when iterating over the
 *                            contents of an object returned by getObject().
 * @tparam  ObjectType        Name of the type that will be returned by the
 *                            getObject() function. Instances of this type
 *                            should provide begin(), end(), find() and size()
 *                            functions so that it is possible to iterate over
 *                            the members of the object.
 * @tparam  ValueType         Name of the type that provides a consistent
 *                            interface to a JSON value for a parser. For
 *                            example, this type should provide the getDouble()
 *                            and isDouble() functions. But it does not need to
 *                            know how to cast values from one type to another -
 *                            that functionality is provided by this template
 *                            class.
 */
template<
    typename AdapterType,
    typename ArrayType,
    typename ObjectMemberType,
    typename ObjectType,
    typename ValueType>
class BasicAdapter: public Adapter
{
protected:

    /**
     * @brief   Functor for comparing two arrays.
     *
     * This functor is used to compare the elements in an array of the type
     * ArrayType with individual values provided as generic Adapter objects.
     * Comparison is performed by the () operator.
     *
     * The functor works by maintaining an iterator for the current position
     * in an array. Each time the () operator is called, the value at this
     * position is compared with the value passed as an argument to ().
     * Immediately after the comparison, the iterator will be incremented.
     *
     * This functor is designed to be passed to the applyToArray() function
     * of an Adapter object.
     */
    class ArrayComparisonFunctor
    {
    public:

        /**
         * @brief   Construct an ArrayComparisonFunctor for an array.
         *
         * @param   array   Array to compare values against
         * @param   strict  Flag to use strict type comparison
         */
        ArrayComparisonFunctor(const ArrayType &array, bool strict)
          : itr(array.begin()),
            end(array.end()),
            strict(strict) { }

        /**
         * @brief   Compare a value against the current element in the array.
         *
         * @param   adapter  Value to be compared with current element
         *
         * @returns true if values are equal, false otherwise.
         */
        bool operator()(const Adapter &adapter)
        {
            if (itr == end) {
                return false;
            }

            return AdapterType(*itr++).equalTo(adapter, strict);
        }

    private:

        /// Iterator for current element in the array
        typename ArrayType::const_iterator itr;

        /// Iterator for one-past the last element of the array
        typename ArrayType::const_iterator end;

        /// Flag to use strict type comparison
        const bool strict;
    };

    /**
     * @brief   Functor for comparing two objects
     *
     * This functor is used to compare the members of an object of the type
     * ObjectType with key-value pairs belonging to another object.
     *
     * The functor works by maintaining a reference to an object provided via
     * the constructor. When time the () operator is called with a key-value
     * pair as arguments, the function will attempt to find the key in the
     * base object. If found, the associated value will be compared with the
     * value provided to the () operator.
     *
     * This functor is designed to be passed to the applyToObject() function
     * of an Adapter object.
     */
    class ObjectComparisonFunctor
    {
    public:

        /**
         * @brief   Construct a new ObjectComparisonFunctor for an object.
         *
         * @param   object  object to use as comparison baseline
         * @param   strict  flag to use strict type-checking
         */
        ObjectComparisonFunctor(
            const ObjectType &object, bool strict)
          : object(object),
            strict(strict) { }

        /**
         * @brief   Find a key in the object and compare its value.
         *
         * @param   key    Key to find
         * @param   value  Value to be compared against
         *
         * @returns true if key is found and values are equal, false otherwise.
         */
        bool operator()(const std::string &key, const Adapter &value)
        {
            const typename ObjectType::const_iterator itr = object.find(key);
            if (itr == object.end()) {
                return false;
            }

            return (*itr).second.equalTo(value, strict);
        }

    private:

        /// Object to be used as a comparison baseline
        const ObjectType &object;

        /// Flag to use strict type-checking
        bool strict;
    };


public:

    /// Alias for ArrayType template parameter
    typedef ArrayType Array;

    /// Alias for ObjectMemberType template parameter
    typedef ObjectMemberType ObjectMember;

    /// Alias for ObjectType template parameter
    typedef ObjectType Object;

    /**
     * @brief   Construct an Adapter using the default value.
     *
     * This constructor relies on the default constructor of the ValueType
     * class provided as a template argument.
     */
    BasicAdapter() { }

    /**
     * @brief   Construct an Adapter using a specified ValueType object.
     *
     * This constructor relies on the copy constructor of the ValueType
     * class provided as template argument.
     */
    BasicAdapter(const ValueType &value)
      : value(value) { }

    virtual bool applyToArray(ArrayValueCallback fn) const
    {
        if (!maybeArray()) {
            return false;
        }

        // Due to the fact that the only way a value can be 'maybe an array' is
        // if it is an empty string or empty object, we only need to go to
        // effort of constructing an ArrayType instance if the value is
        // definitely an array.
        if (value.isArray()) {
            const opt::optional<Array> array = value.getArrayOptional();
            for (const AdapterType element : *array) {
                if (!fn(element)) {
                    return false;
                }
            }
        }

        return true;
    }

    virtual bool applyToObject(ObjectMemberCallback fn) const
    {
        if (!maybeObject()) {
            return false;
        }

        if (value.isObject()) {
            const opt::optional<Object> object = value.getObjectOptional();
            for (const ObjectMemberType member : *object) {
                if (!fn(member.first, AdapterType(member.second))) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief   Return an ArrayType instance containing an array representation
     *          of the value held by this Adapter.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the elements in an array. The ArrayType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained value is either an empty object, or an empty string,
     * then this function will cast the value to an empty array.
     *
     * @returns ArrayType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ArrayType asArray() const
    {
        if (value.isArray()) {
            return *value.getArrayOptional();
        } else if (value.isObject()) {
            size_t objectSize;
            if (value.getObjectSize(objectSize) && objectSize == 0) {
                return ArrayType();
            }
        } else if (value.isString()) {
            std::string stringValue;
            if (value.getString(stringValue) && stringValue.empty()) {
                return ArrayType();
            }
        }

        throw std::runtime_error("JSON value cannot be cast to an array.");
    }

    virtual bool asBool() const
    {
        bool result;
        if (asBool(result)) {
            return result;
        }

        throw std::runtime_error("JSON value cannot be cast to a boolean.");
    }

    virtual bool asBool(bool &result) const
    {
        if (value.isBool()) {
            return value.getBool(result);
        } else if (value.isString()) {
            std::string s;
            if (value.getString(s)) {
                if (s.compare("true") == 0) {
                    result = true;
                    return true;
                } else if (s.compare("false") == 0) {
                    result = false;
                    return true;
                }
            }
        }

        return false;
    }

    virtual double asDouble() const
    {
        double result;
        if (asDouble(result)) {
            return result;
        }

        throw std::runtime_error("JSON value cannot be cast to a double.");
    }

    virtual bool asDouble(double &result) const
    {
        if (value.isDouble()) {
            return value.getDouble(result);
        } else if (value.isInteger()) {
            int64_t i;
            if (value.getInteger(i)) {
                result = double(i);
                return true;
            }
        } else if (value.isString()) {
            std::string s;
            if (value.getString(s)) {
                const char *b = s.c_str();
                char *e = NULL;
                double x = strtod(b, &e);
                if (e == b || e != b + s.length()) {
                    return false;
                }
                result = x;
                return true;
            }
        }

        return false;
    }

    virtual int64_t asInteger() const
    {
        int64_t result;
        if (asInteger(result)) {
            return result;
        }

        throw std::runtime_error("JSON value cannot be cast as an integer.");
    }

    virtual bool asInteger(int64_t &result) const
    {
        if (value.isInteger()) {
            return value.getInteger(result);
        } else if (value.isString()) {
            std::string s;
            if (value.getString(s)) {
                std::istringstream i(s);
                int64_t x;
                char c;
                if (!(!(i >> x) || i.get(c))) {
                    result = x;
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief   Return an ObjectType instance containing an array representation
     *          of the value held by this Adapter.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the members of the object. The ObjectType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * @returns ObjectType instance containing an object representation of the
     *          value held by this Adapter.
     */
    ObjectType asObject() const
    {
        if (value.isObject()) {
            return *value.getObjectOptional();
        } else if (value.isArray()) {
            size_t arraySize;
            if (value.getArraySize(arraySize) && arraySize == 0) {
                return ObjectType();
            }
        } else if (value.isString()) {
            std::string stringValue;
            if (value.getString(stringValue) && stringValue.empty()) {
                return ObjectType();
            }
        }

        throw std::runtime_error("JSON value cannot be cast to an object.");
    }

    virtual std::string asString() const
    {
        std::string result;
        if (asString(result)) {
            return result;
        }

        throw std::runtime_error("JSON value cannot be cast to a string.");
    }

    virtual bool asString(std::string &result) const
    {
        if (value.isString()) {
            return value.getString(result);
        } else if (value.isNull()) {
            result.clear();
            return true;
        } else if (value.isArray()) {
            size_t arraySize;
            if (value.getArraySize(arraySize) && arraySize == 0) {
                result.clear();
                return true;
            }
        } else if (value.isObject()) {
            size_t objectSize;
            if (value.getObjectSize(objectSize) && objectSize == 0) {
                result.clear();
                return true;
            }
        } else if (value.isBool()) {
            bool boolValue;
            if (value.getBool(boolValue)) {
                result = boolValue ? "true" : "false";
                return true;
            }
        } else if (value.isInteger()) {
            int64_t integerValue;
            if (value.getInteger(integerValue)) {
                result = std::to_string(integerValue);
                return true;
            }
        } else if (value.isDouble()) {
            double doubleValue;
            if (value.getDouble(doubleValue)) {
                result = std::to_string(doubleValue);
                return true;
            }
        }

        return false;
    }

    virtual bool equalTo(const Adapter &other, bool strict) const
    {
        if (isNull() || (!strict && maybeNull())) {
            return other.isNull() || (!strict && other.maybeNull());
        } else if (isBool() || (!strict && maybeBool())) {
            return (other.isBool() || (!strict && other.maybeBool())) &&
                other.asBool() == asBool();
        } else if (isNumber() && strict) {
            return other.isNumber() && other.getNumber() == getNumber();
        } else if (!strict && maybeDouble()) {
            return (other.maybeDouble() &&
                    other.asDouble() == asDouble());
        } else if (!strict && maybeInteger()) {
            return (other.maybeInteger() &&
                    other.asInteger() == asInteger());
        } else if (isString() || (!strict && maybeString())) {
            return (other.isString() || (!strict && other.maybeString())) &&
                other.asString() == asString();
        } else if (isArray()) {
            if (other.isArray() && getArraySize() == other.getArraySize()) {
                const opt::optional<ArrayType> array = value.getArrayOptional();
                if (array) {
                    ArrayComparisonFunctor fn(*array, strict);
                    return other.applyToArray(fn);
                }
            } else if (!strict && other.maybeArray() && getArraySize() == 0) {
                return true;
            }
        } else if (isObject()) {
            if (other.isObject() && other.getObjectSize() == getObjectSize()) {
                const opt::optional<ObjectType> object = value.getObjectOptional();
                if (object) {
                    ObjectComparisonFunctor fn(*object, strict);
                    return other.applyToObject(fn);
                }
            } else if (!strict && other.maybeObject() && getObjectSize() == 0) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief   Return an ArrayType instance representing the array contained
     *          by this Adapter instance.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the elements in an array. The ArrayType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained is not an array, this function will throw an exception.
     *
     * @returns ArrayType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ArrayType getArray() const
    {
        opt::optional<ArrayType> arrayValue = value.getArrayOptional();
        if (arrayValue) {
            return *arrayValue;
        }

        throw std::runtime_error("JSON value is not an array.");
    }

    virtual size_t getArraySize() const
    {
        size_t result;
        if (value.getArraySize(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not an array.");
    }

    virtual bool getArraySize(size_t &result) const
    {
        return value.getArraySize(result);
    }

    virtual bool getBool() const
    {
        bool result;
        if (getBool(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not a boolean.");
    }

    virtual bool getBool(bool &result) const
    {
        return value.getBool(result);
    }

    virtual double getDouble() const
    {
        double result;
        if (getDouble(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not a double.");
    }

    virtual bool getDouble(double &result) const
    {
        return value.getDouble(result);
    }

    virtual int64_t getInteger() const
    {
        int64_t result;
        if (getInteger(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not an integer.");
    }

    virtual bool getInteger(int64_t &result) const
    {
        return value.getInteger(result);
    }

    virtual double getNumber() const
    {
        double result;
        if (getNumber(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not a number.");
    }

    virtual bool getNumber(double &result) const
    {
        if (isDouble()) {
            return getDouble(result);
        } else if (isInteger()) {
            int64_t integerResult;
            if (getInteger(integerResult)) {
                result = static_cast<double>(integerResult);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief   Return an ObjectType instance representing the object contained
     *          by this Adapter instance.
     *
     * This is a convenience function that is not actually declared in the
     * Adapter interface, but allows for useful techniques such as procedural
     * iteration over the members of an object. The ObjectType instance that is
     * returned by this function is compatible with the BOOST_FOREACH macro.
     *
     * If the contained is not an object, this function will throw an exception.
     *
     * @returns ObjectType instance containing an array representation of the
     *          value held by this Adapter.
     */
    ObjectType getObject() const
    {
        opt::optional<ObjectType> objectValue = value.getObjectOptional();
        if (objectValue) {
            return *objectValue;
        }

        throw std::runtime_error("JSON value is not an object.");
    }

    virtual size_t getObjectSize() const
    {
        size_t result;
        if (getObjectSize(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not an object.");
    }

    virtual bool getObjectSize(size_t &result) const
    {
        return value.getObjectSize(result);
    }

    virtual std::string getString() const
    {
        std::string result;
        if (getString(result)) {
            return result;
        }

        throw std::runtime_error("JSON value is not a string.");
    }

    virtual bool getString(std::string &result) const
    {
        return value.getString(result);
    }

    virtual FrozenValue * freeze() const
    {
        return value.freeze();
    }

    virtual bool hasStrictTypes() const
    {
        return ValueType::hasStrictTypes();
    }

    virtual bool isArray() const
    {
        return value.isArray();
    }

    virtual bool isBool() const
    {
        return value.isBool();
    }

    virtual bool isDouble() const
    {
        return value.isDouble();
    }

    virtual bool isInteger() const
    {
        return value.isInteger();
    }

    virtual bool isNull() const
    {
        return value.isNull();
    }

    virtual bool isNumber() const
    {
        return value.isInteger() || value.isDouble();
    }

    virtual bool isObject() const
    {
        return value.isObject();
    }

    virtual bool isString() const
    {
        return value.isString();
    }

    virtual bool maybeArray() const
    {
        if (value.isArray()) {
            return true;
        } else if (value.isObject()) {
            size_t objectSize;
            if (value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        }

        return false;
    }

    virtual bool maybeBool() const
    {
        if (value.isBool()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (value.getString(stringValue)) {
                if (stringValue.compare("true") == 0 || stringValue.compare("false") == 0) {
                    return true;
                }
            }
        }

        return false;
    }

    virtual bool maybeDouble() const
    {
        if (value.isNumber()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (value.getString(s)) {
                const char *b = s.c_str();
                char *e = NULL;
                strtod(b, &e);
                return e != b && e == b + s.length();
            }
        }

        return false;
    }

    virtual bool maybeInteger() const
    {
        if (value.isInteger()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (value.getString(s)) {
                std::istringstream i(s);
                int64_t x;
                char c;
                if (!(i >> x) || i.get(c)) {
                    return false;
                }
                return true;
            }
        }

        return false;
    }

    virtual bool maybeNull() const
    {
        if (value.isNull()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (value.getString(stringValue)) {
                if (stringValue.empty()) {
                    return true;
                }
            }
        }

        return false;
    }

    virtual bool maybeObject() const
    {
        if (value.isObject()) {
            return true;
        } else if (maybeArray()) {
            size_t arraySize;
            if (value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

    virtual bool maybeString() const
    {
        if (value.isString() || value.isBool() || value.isInteger() ||
            value.isDouble()) {
            return true;
        } else if (value.isObject()) {
            size_t objectSize;
            if (value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        } else if (value.isArray()) {
            size_t arraySize;
            if (value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

private:

    const ValueType value;

};

}  // namespace adapters
}  // namespace valijson

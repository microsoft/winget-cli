#pragma once

#include <cstdint>
#include <sstream>

#include <valijson/adapters/adapter.hpp>
#include <valijson/internal/optional.hpp>
#include <valijson/exceptions.hpp>

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

    explicit operator Value*()
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
          : m_itr(array.begin()),
            m_end(array.end()),
            m_strict(strict) { }

        /**
         * @brief   Compare a value against the current element in the array.
         *
         * @param   adapter  Value to be compared with current element
         *
         * @returns true if values are equal, false otherwise.
         */
        bool operator()(const Adapter &adapter)
        {
            if (m_itr == m_end) {
                return false;
            }

            return AdapterType(*m_itr++).equalTo(adapter, m_strict);
        }

    private:

        /// Iterator for current element in the array
        typename ArrayType::const_iterator m_itr;

        /// Iterator for one-past the last element of the array
        typename ArrayType::const_iterator m_end;

        /// Flag to use strict type comparison
        const bool m_strict;
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
        ObjectComparisonFunctor(const ObjectType &object, bool strict)
          : m_object(object),
            m_strict(strict) { }

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
            const typename ObjectType::const_iterator itr = m_object.find(key);
            if (itr == m_object.end()) {
                return false;
            }

            return (*itr).second.equalTo(value, m_strict);
        }

    private:

        /// Object to be used as a comparison baseline
        const ObjectType &m_object;

        /// Flag to use strict type-checking
        bool m_strict;
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
    BasicAdapter() = default;

    /**
     * @brief   Construct an Adapter using a specified ValueType object.
     *
     * This constructor relies on the copy constructor of the ValueType
     * class provided as template argument.
     */
    explicit BasicAdapter(const ValueType &value)
      : m_value(value) { }

    bool applyToArray(ArrayValueCallback fn) const override
    {
        if (!maybeArray()) {
            return false;
        }

        // Due to the fact that the only way a value can be 'maybe an array' is
        // if it is an empty string or empty object, we only need to go to
        // effort of constructing an ArrayType instance if the value is
        // definitely an array.
        if (m_value.isArray()) {
            const opt::optional<Array> array = m_value.getArrayOptional();
            for (const AdapterType element : *array) {
                if (!fn(element)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool applyToObject(ObjectMemberCallback fn) const override
    {
        if (!maybeObject()) {
            return false;
        }

        if (m_value.isObject()) {
            const opt::optional<Object> object = m_value.getObjectOptional();
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
        if (m_value.isArray()) {
            return *m_value.getArrayOptional();
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return ArrayType();
            }
        } else if (m_value.isString()) {
            std::string stringValue;
            if (m_value.getString(stringValue) && stringValue.empty()) {
                return ArrayType();
            }
        }

        throwRuntimeError("JSON value cannot be cast to an array.");
    }

    bool asBool() const override
    {
        bool result;
        if (asBool(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a boolean.");
    }

    bool asBool(bool &result) const override
    {
        if (m_value.isBool()) {
            return m_value.getBool(result);
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
                if (s == "true") {
                    result = true;
                    return true;
                } else if (s == "false") {
                    result = false;
                    return true;
                }
            }
        }

        return false;
    }

    double asDouble() const override
    {
        double result;
        if (asDouble(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a double.");
    }

    bool asDouble(double &result) const override
    {
        if (m_value.isDouble()) {
            return m_value.getDouble(result);
        } else if (m_value.isInteger()) {
            int64_t i;
            if (m_value.getInteger(i)) {
                result = double(i);
                return true;
            }
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
                const char *b = s.c_str();
                char *e = nullptr;
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

    int64_t asInteger() const override
    {
        int64_t result;
        if (asInteger(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast as an integer.");
    }

    bool asInteger(int64_t &result) const override
    {
        if (m_value.isInteger()) {
            return m_value.getInteger(result);
        } else if (m_value.isString()) {
            std::string s;
            if (m_value.getString(s)) {
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
        if (m_value.isObject()) {
            return *m_value.getObjectOptional();
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return ObjectType();
            }
        } else if (m_value.isString()) {
            std::string stringValue;
            if (m_value.getString(stringValue) && stringValue.empty()) {
                return ObjectType();
            }
        }

        throwRuntimeError("JSON value cannot be cast to an object.");
    }

    std::string asString() const override
    {
        std::string result;
        if (asString(result)) {
            return result;
        }

        throwRuntimeError("JSON value cannot be cast to a string.");
    }

    bool asString(std::string &result) const override
    {
        if (m_value.isString()) {
            return m_value.getString(result);
        } else if (m_value.isNull()) {
            result.clear();
            return true;
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                result.clear();
                return true;
            }
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                result.clear();
                return true;
            }
        } else if (m_value.isBool()) {
            bool boolValue;
            if (m_value.getBool(boolValue)) {
                result = boolValue ? "true" : "false";
                return true;
            }
        } else if (m_value.isInteger()) {
            int64_t integerValue;
            if (m_value.getInteger(integerValue)) {
                result = std::to_string(integerValue);
                return true;
            }
        } else if (m_value.isDouble()) {
            double doubleValue;
            if (m_value.getDouble(doubleValue)) {
                result = std::to_string(doubleValue);
                return true;
            }
        }

        return false;
    }

    bool equalTo(const Adapter &other, bool strict) const override
    {
        if (isNull() || (!strict && maybeNull())) {
            return other.isNull() || (!strict && other.maybeNull());
        } else if (isBool() || (!strict && maybeBool())) {
            return (other.isBool() || (!strict && other.maybeBool())) && other.asBool() == asBool();
        } else if (isNumber() && strict) {
            return other.isNumber() && other.getNumber() == getNumber();
        } else if (!strict && maybeDouble()) {
            return (other.maybeDouble() && other.asDouble() == asDouble());
        } else if (!strict && maybeInteger()) {
            return (other.maybeInteger() && other.asInteger() == asInteger());
        } else if (isString() || (!strict && maybeString())) {
            return (other.isString() || (!strict && other.maybeString())) &&
                other.asString() == asString();
        } else if (isArray()) {
            if (other.isArray() && getArraySize() == other.getArraySize()) {
                const opt::optional<ArrayType> array = m_value.getArrayOptional();
                if (array) {
                    ArrayComparisonFunctor fn(*array, strict);
                    return other.applyToArray(fn);
                }
            } else if (!strict && other.maybeArray() && getArraySize() == 0) {
                return true;
            }
        } else if (isObject()) {
            if (other.isObject() && other.getObjectSize() == getObjectSize()) {
                const opt::optional<ObjectType> object = m_value.getObjectOptional();
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
        opt::optional<ArrayType> arrayValue = m_value.getArrayOptional();
        if (arrayValue) {
            return *arrayValue;
        }

        throwRuntimeError("JSON value is not an array.");
    }

    size_t getArraySize() const override
    {
        size_t result;
        if (m_value.getArraySize(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an array.");
    }

    bool getArraySize(size_t &result) const override
    {
        return m_value.getArraySize(result);
    }

    bool getBool() const override
    {
        bool result;
        if (getBool(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a boolean.");
    }

    bool getBool(bool &result) const override
    {
        return m_value.getBool(result);
    }

    double getDouble() const override
    {
        double result;
        if (getDouble(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a double.");
    }

    bool getDouble(double &result) const override
    {
        return m_value.getDouble(result);
    }

    int64_t getInteger() const override
    {
        int64_t result;
        if (getInteger(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an integer.");
    }

    bool getInteger(int64_t &result) const override
    {
        return m_value.getInteger(result);
    }

    double getNumber() const override
    {
        double result;
        if (getNumber(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a number.");
    }

    bool getNumber(double &result) const override
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
        opt::optional<ObjectType> objectValue = m_value.getObjectOptional();
        if (objectValue) {
            return *objectValue;
        }

        throwRuntimeError("JSON value is not an object.");
    }

    size_t getObjectSize() const override
    {
        size_t result;
        if (getObjectSize(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not an object.");
    }

    bool getObjectSize(size_t &result) const override
    {
        return m_value.getObjectSize(result);
    }

    std::string getString() const override
    {
        std::string result;
        if (getString(result)) {
            return result;
        }

        throwRuntimeError("JSON value is not a string.");
    }

    bool getString(std::string &result) const override
    {
        return m_value.getString(result);
    }

    FrozenValue * freeze() const override
    {
        return m_value.freeze();
    }

    bool hasStrictTypes() const override
    {
        return ValueType::hasStrictTypes();
    }

    bool isArray() const override
    {
        return m_value.isArray();
    }

    bool isBool() const override
    {
        return m_value.isBool();
    }

    bool isDouble() const override
    {
        return m_value.isDouble();
    }

    bool isInteger() const override
    {
        return m_value.isInteger();
    }

    bool isNull() const override
    {
        return m_value.isNull();
    }

    bool isNumber() const override
    {
        return m_value.isInteger() || m_value.isDouble();
    }

    bool isObject() const override
    {
        return m_value.isObject();
    }

    bool isString() const override
    {
        return m_value.isString();
    }

    bool maybeArray() const override
    {
        if (m_value.isArray()) {
            return true;
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        }

        return false;
    }

    bool maybeBool() const override
    {
        if (m_value.isBool()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (m_value.getString(stringValue)) {
                if (stringValue == "true" || stringValue == "false") {
                    return true;
                }
            }
        }

        return false;
    }

    bool maybeDouble() const override
    {
        if (m_value.isNumber()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (m_value.getString(s)) {
                const char *b = s.c_str();
                char *e = nullptr;
                strtod(b, &e);
                return e != b && e == b + s.length();
            }
        }

        return false;
    }

    bool maybeInteger() const override
    {
        if (m_value.isInteger()) {
            return true;
        } else if (maybeString()) {
            std::string s;
            if (m_value.getString(s)) {
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

    bool maybeNull() const override
    {
        if (m_value.isNull()) {
            return true;
        } else if (maybeString()) {
            std::string stringValue;
            if (m_value.getString(stringValue)) {
                if (stringValue.empty()) {
                    return true;
                }
            }
        }

        return false;
    }

    bool maybeObject() const override
    {
        if (m_value.isObject()) {
            return true;
        } else if (maybeArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

    bool maybeString() const override
    {
        if (m_value.isString() || m_value.isBool() || m_value.isInteger() || m_value.isDouble()) {
            return true;
        } else if (m_value.isObject()) {
            size_t objectSize;
            if (m_value.getObjectSize(objectSize) && objectSize == 0) {
                return true;
            }
        } else if (m_value.isArray()) {
            size_t arraySize;
            if (m_value.getArraySize(arraySize) && arraySize == 0) {
                return true;
            }
        }

        return false;
    }

private:

    const ValueType m_value;
};

}  // namespace adapters
}  // namespace valijson

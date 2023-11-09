#pragma once

#include <functional>

namespace valijson {
namespace adapters {

class FrozenValue;

/**
 * @brief   An interface that encapsulates access to the JSON values provided
 *          by a JSON parser implementation.
 *
 * This interface allows JSON processing code to be parser-agnostic. It provides
 * functions to access the plain old datatypes (PODs) that are described in the
 * JSON specification, and callback-based access to the contents of arrays and
 * objects.
 *
 * The interface also defines a set of functions that allow for type-casting and
 * type-comparison based on value rather than on type.
 */
class Adapter
{
public:

    /// Typedef for callback function supplied to applyToArray.
    typedef std::function<bool (const Adapter &)>
        ArrayValueCallback;

    /// Typedef for callback function supplied to applyToObject.
    typedef std::function<bool (const std::string &, const Adapter &)>
        ObjectMemberCallback;

    /**
     * @brief   Virtual destructor defined to ensure deletion via base-class
     *          pointers is safe.
     */
    virtual ~Adapter() = default;

    /**
     * @brief   Apply a callback function to each value in an array.
     *
     * The callback function is invoked for each element in the array, until
     * it has been applied to all values, or it returns false.
     *
     * @param   fn  Callback function to invoke
     *
     * @returns true if Adapter contains an array and all values are equal,
     *          false otherwise.
     */
    virtual bool applyToArray(ArrayValueCallback fn) const = 0;

    /**
     * @brief   Apply a callback function to each member in an object.
     *
     * The callback function shall be invoked for each member in the object,
     * until it has been applied to all values, or it returns false.
     *
     * @param   fn  Callback function to invoke
     *
     * @returns true if Adapter contains an object, and callback function
     *          returns true for each member in the object, false otherwise.
     */
    virtual bool applyToObject(ObjectMemberCallback fn) const = 0;

    /**
     * @brief   Return the boolean representation of the contained value.
     *
     * This function shall return a boolean value if the Adapter contains either
     * an actual boolean value, or one of the strings 'true' or 'false'.
     * The string comparison is case sensitive.
     *
     * An exception shall be thrown if the value cannot be cast to a boolean.
     *
     * @returns  Boolean representation of contained value.
     */
    virtual bool asBool() const = 0;

    /**
     * @brief   Retrieve the boolean representation of the contained value.
     *
     * This function shall retrieve a boolean value if the Adapter contains
     * either an actual boolean value, or one of the strings 'true' or 'false'.
     * The string comparison is case sensitive.
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a bool to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asBool(bool &result) const = 0;

    /**
     * @brief   Return the double representation of the contained value.
     *
     * This function shall return a double value if the Adapter contains either
     * an actual double, an integer, or a string that contains a valid
     * representation of a numeric value (according to the C++ Std Library).
     *
     * An exception shall be thrown if the value cannot be cast to a double.
     *
     * @returns  Double representation of contained value.
     */
    virtual double asDouble() const = 0;

    /**
     * @brief   Retrieve the double representation of the contained value.
     *
     * This function shall retrieve a double value if the Adapter contains either
     * an actual double, an integer, or a string that contains a valid
     * representation of a numeric value (according to the C++ Std Library).
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a double to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asDouble(double &result) const = 0;

    /**
     * @brief   Return the int64_t representation of the contained value.
     *
     * This function shall return an int64_t value if the Adapter contains either
     * an actual integer, or a string that contains a valid representation of an
     * integer value (according to the C++ Std Library).
     *
     * An exception shall be thrown if the value cannot be cast to an int64_t.
     *
     * @returns  int64_t representation of contained value.
     */
    virtual int64_t asInteger() const = 0;

    /**
     * @brief   Retrieve the int64_t representation of the contained value.
     *
     * This function shall retrieve an int64_t value if the Adapter contains
     * either an actual integer, or a string that contains a valid
     * representation of an integer value (according to the C++ Std Library).
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a int64_t to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asInteger(int64_t &result) const = 0;

    /**
     * @brief   Return the string representation of the contained value.
     *
     * This function shall return a string value if the Adapter contains either
     * an actual string, a literal value of another POD type, an empty array,
     * an empty object, or null.
     *
     * An exception shall be thrown if the value cannot be cast to a string.
     *
     * @returns  string representation of contained value.
     */
    virtual std::string asString() const = 0;

    /**
     * @brief   Retrieve the string representation of the contained value.
     *
     * This function shall retrieve a string value if the Adapter contains either
     * an actual string, a literal value of another POD type, an empty array,
     * an empty object, or null.
     *
     * The retrieved value is returned via reference.
     *
     * @param   result  reference to a string to set with retrieved value.
     *
     * @returns true if the value could be retrieved, false otherwise
     */
    virtual bool asString(std::string &result) const = 0;

    /**
     * @brief   Compare the value held by this Adapter instance with the value
     *          held by another Adapter instance.
     *
     * @param   other   the other adapter instance
     * @param   strict  flag to use strict type comparison
     *
     * @returns true if values are equal, false otherwise
     */
    virtual bool equalTo(const Adapter &other, bool strict) const = 0;

    /**
     * @brief   Create a new FrozenValue instance that is equivalent to the
     *          value contained by the Adapter.
     *
     * @returns pointer to a new FrozenValue instance, belonging to the caller.
     */
    virtual FrozenValue* freeze() const = 0;

    /**
     * @brief   Return the number of elements in the array.
     *
     * Throws an exception if the value is not an array.
     *
     * @return  number of elements if value is an array
     */
    virtual size_t getArraySize() const = 0;

    /**
     * @brief   Retrieve the number of elements in the array.
     *
     * This function shall return true or false to indicate whether or not the
     * result value was set. If the contained value is not an array, the
     * result value shall not be set. This applies even if the value could be
     * cast to an empty array. The calling code is expected to handles those
     * cases manually.
     *
     * @param   result  reference to size_t variable to set with result.
     *
     * @return  true if value retrieved successfully, false otherwise.
     */
    virtual bool getArraySize(size_t &result) const = 0;

    /**
     * @brief   Return the contained boolean value.
     *
     * This function shall throw an exception if the contained value is not a
     * boolean.
     *
     * @returns contained boolean value.
     */
    virtual bool getBool() const = 0;

    /**
     * @brief   Retrieve the contained boolean value.
     *
     * This function shall retrieve the boolean value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to boolean variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getBool(bool &result) const = 0;

    /**
     * @brief   Return the contained double value.
     *
     * This function shall throw an exception if the contained value is not a
     * double.
     *
     * @returns contained double value.
     */
    virtual double getDouble() const = 0;

    /**
     * @brief   Retrieve the contained double value.
     *
     * This function shall retrieve the double value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to double variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getDouble(double &result) const = 0;

    /**
     * @brief   Return the contained integer value.
     *
     * This function shall throw an exception if the contained value is not a
     * integer.
     *
     * @returns contained integer value.
     */
    virtual int64_t getInteger() const = 0;

    /**
     * @brief   Retrieve the contained integer value.
     *
     * This function shall retrieve the integer value contained by this Adapter,
     * and store it in the result variable that was passed by reference.
     *
     * @param   result  reference to integer variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getInteger(int64_t &result) const = 0;

    /**
     * @brief   Return the contained numeric value as a double.
     *
     * This function shall throw an exception if the contained value is not a
     * integer or a double.
     *
     * @returns contained double or integral value.
     */
    virtual double getNumber() const = 0;

    /**
     * @brief   Retrieve the contained numeric value as a double.
     *
     * This function shall retrieve the double or integral value contained by
     * this Adapter, and store it in the result variable that was passed by
     * reference.
     *
     * @param   result  reference to double variable to set with result.
     *
     * @returns true if the value was retrieved, false otherwise.
     */
    virtual bool getNumber(double &result) const = 0;

    /**
     * @brief   Return the number of members in the object.
     *
     * Throws an exception if the value is not an object.
     *
     * @return  number of members if value is an object
     */
    virtual size_t getObjectSize() const = 0;

    /**
     * @brief   Retrieve the number of members in the object.
     *
     * This function shall return true or false to indicate whether or not the
     * result value was set. If the contained value is not an object, the
     * result value shall not be set. This applies even if the value could be
     * cast to an empty object. The calling code is expected to handles those
     * cases manually.
     *
     * @param   result  reference to size_t variable to set with result.
     *
     * @return  true if value retrieved successfully, false otherwise.
     */
    virtual bool getObjectSize(size_t &result) const = 0;

    /**
     * @brief   Return the contained string value.
     *
     * This function shall throw an exception if the contained value is not a
     * string - even if the value could be cast to a string. The asString()
     * function should be used when casting is allowed.
     *
     * @returns string contained by this Adapter
     */
    virtual std::string getString() const = 0;

    /**
     * @brief   Retrieve the contained string value.
     *
     * This function shall retrieve the string value contained by this Adapter,
     * and store it in result variable that is passed by reference.
     *
     * @param   result  reference to string to set with result
     *
     * @returns true if string was retrieved, false otherwise
     */
    virtual bool getString(std::string &result) const = 0;

    /**
     * @brief   Returns whether or not this Adapter supports strict types.
     *
     * This function shall return true if the Adapter implementation supports
     * strict types, or false if the Adapter fails to store any part of the
     * type information supported by the Adapter interface.
     *
     * For example, the PropertyTreeAdapter implementation stores POD values as
     * strings, effectively discarding any other type information. If you were
     * to call isDouble() on a double stored by this Adapter, the result would
     * be false. The maybeDouble(), asDouble() and various related functions
     * are provided to perform type checking based on value rather than on type.
     *
     * The BasicAdapter template class provides implementations for the type-
     * casting functions so that Adapter implementations are semantically
     * equivalent in their type-casting behaviour.
     *
     * @returns true if Adapter supports strict types, false otherwise
     */
    virtual bool hasStrictTypes() const = 0;

    /// Returns true if the contained value is definitely an array.
    virtual bool isArray() const = 0;

    /// Returns true if the contained value is definitely a boolean.
    virtual bool isBool() const = 0;

    /// Returns true if the contained value is definitely a double.
    virtual bool isDouble() const = 0;

    /// Returns true if the contained value is definitely an integer.
    virtual bool isInteger() const = 0;

    /// Returns true if the contained value is definitely a null.
    virtual bool isNull() const = 0;

    /// Returns true if the contained value is either a double or an integer.
    virtual bool isNumber() const = 0;

    /// Returns true if the contained value is definitely an object.
    virtual bool isObject() const = 0;

    /// Returns true if the contained value is definitely a string.
    virtual bool isString() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an array.
     *
     * @returns true if the contained value is an array, an empty string, or an
     *          empty object.
     */
    virtual bool maybeArray() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a boolean.
     *
     * @returns true if the contained value is a boolean, or one of the strings
     *          'true' or 'false'. Note that numeric values are not to be cast
     *          to boolean values.
     */
    virtual bool maybeBool() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a double.
     *
     * @returns true if the contained value is a double, an integer, or a string
     *          containing a double or integral value.
     */
    virtual bool maybeDouble() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an integer.
     *
     * @returns true if the contained value is an integer, or a string
     *          containing an integral value.
     */
    virtual bool maybeInteger() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a null.
     *
     * @returns true if the contained value is null or an empty string.
     */
    virtual bool maybeNull() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to an object.
     *
     * @returns true if the contained value is an object, an empty array or
     *          an empty string.
     */
    virtual bool maybeObject() const = 0;

    /**
     * @brief   Returns true if the contained value can be cast to a string.
     *
     * @returns true if the contained value is a non-null POD type, an empty
     *          array, or an empty object.
     */
    virtual bool maybeString() const = 0;
};

/**
 * @brief  Template struct that should be specialised for each concrete Adapter
 *         class.
 *
 * @deprecated  This is a bit of a hack, and I'd like to remove it.
 */
template<typename T>
struct AdapterTraits
{

};

}  // namespace adapters
}  // namespace valijson

/**
 * @file
 *
 * @brief   Adapter implementation that wraps a single std::string value
 *
 * This allows property names to be validated against a schema as though they are a generic JSON
 * value, while allowing the rest of Valijson's API to expose property names as plain std::string
 * values.
 *
 * This was added while implementing draft 7 support. This included support for a constraint
 * called propertyNames, which can be used to ensure that the property names in an object
 * validate against a subschema.
 */

#pragma once

#include <string>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/adapters/basic_adapter.hpp>

namespace valijson {
namespace adapters {

class StdStringAdapter;
class StdStringArrayValueIterator;
class StdStringObjectMemberIterator;

typedef std::pair<std::string, StdStringAdapter> StdStringObjectMember;

class StdStringArray
{
public:
    typedef StdStringArrayValueIterator const_iterator;
    typedef StdStringArrayValueIterator iterator;

    StdStringArray() { }

    StdStringArrayValueIterator begin() const;

    StdStringArrayValueIterator end() const;

    size_t size() const
    {
        return 0;
    }
};

class StdStringObject
{
public:
    typedef StdStringObjectMemberIterator const_iterator;
    typedef StdStringObjectMemberIterator iterator;

    StdStringObject() { }

    StdStringObjectMemberIterator begin() const;

    StdStringObjectMemberIterator end() const;

    StdStringObjectMemberIterator find(const std::string &propertyName) const;

    size_t size() const
    {
        return 0;
    }
};

class StdStringFrozenValue: public FrozenValue
{
public:
    explicit StdStringFrozenValue(const std::string &source)
      : value(source) { }

    virtual FrozenValue * clone() const
    {
        return new StdStringFrozenValue(value);
    }

    virtual bool equalTo(const Adapter &other, bool strict) const;

private:
    std::string value;
};

class StdStringAdapter: public Adapter
{
public:
    typedef StdStringArray Array;
    typedef StdStringObject Object;
    typedef StdStringObjectMember ObjectMember;

    StdStringAdapter(const std::string &value)
      : value(value) { }

    virtual bool applyToArray(ArrayValueCallback fn) const
    {
        return maybeArray();
    }

    virtual bool applyToObject(ObjectMemberCallback fn) const
    {
        return maybeObject();
    }

    StdStringArray asArray() const
    {
        if (maybeArray()) {
            return StdStringArray();
        }

        throw std::runtime_error("String value cannot be cast to array");
    }

    virtual bool asBool() const
    {
        return true;
    }

    virtual bool asBool(bool &result) const
    {
        result = true;
        return true;
    }

    virtual double asDouble() const
    {
        return 0;
    }

    virtual bool asDouble(double &result) const
    {
        result = 0;
        return true;
    }

    virtual int64_t asInteger() const
    {
        return 0;
    }

    virtual bool asInteger(int64_t &result) const
    {
        result = 0;
        return true;
    };

    StdStringObject asObject() const
    {
        if (maybeObject()) {
            return StdStringObject();
        }

        throw std::runtime_error("String value cannot be cast to object");
    }

    virtual std::string asString() const
    {
        return value;
    }

    virtual bool asString(std::string &result) const
    {
        result = value;
        return true;
    }

    virtual bool equalTo(const Adapter &other, bool strict) const
    {
        if (strict && !other.isString()) {
            return false;
        }

        return value.compare(other.asString()) == 0;
    }

    virtual FrozenValue* freeze() const
    {
        return new StdStringFrozenValue(value);
    }

    StdStringArray getArray() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual size_t getArraySize() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getArraySize(size_t &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getBool() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getBool(bool &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual double getDouble() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getDouble(double &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual int64_t getInteger() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getInteger(int64_t &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual double getNumber() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getNumber(double &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual size_t getObjectSize() const
    {
        throw std::runtime_error("Not supported");
    }

    virtual bool getObjectSize(size_t &result) const
    {
        throw std::runtime_error("Not supported");
    }

    virtual std::string getString() const
    {
        return value;
    }

    virtual bool getString(std::string &result) const
    {
        result = value;
        return true;
    }

    virtual bool hasStrictTypes() const
    {
        return true;
    }

    virtual bool isArray() const
    {
        return false;
    }

    virtual bool isBool() const
    {
        return false;
    }

    virtual bool isDouble() const
    {
        return false;
    }

    virtual bool isInteger() const
    {
        return false;
    }

    virtual bool isNull() const
    {
        return false;
    }

    virtual bool isNumber() const
    {
        return false;
    }

    virtual bool isObject() const
    {
        return false;
    }

    virtual bool isString() const
    {
        return true;
    }

    virtual bool maybeArray() const
    {
        return false;
    }

    virtual bool maybeBool() const
    {
        return value.compare("true") == 0 || value.compare("false") == 0;
    }

    virtual bool maybeDouble() const
    {
        const char *b = value.c_str();
        char *e = NULL;
        strtod(b, &e);
        return e != b && e == b + value.length();
    }

    virtual bool maybeInteger() const
    {
        std::istringstream i(value);
        int64_t x;
        char c;
        if (!(i >> x) || i.get(c)) {
            return false;
        }

        return true;
    }

    virtual bool maybeNull() const
    {
        return value.size() == 0;
    }

    virtual bool maybeObject() const
    {
        return value.size() == 0;
    }

    virtual bool maybeString() const
    {
        return true;
    }

private:
    const std::string &value;
};

class StdStringArrayValueIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,
        StdStringAdapter>
{
public:
    StdStringAdapter operator*() const
    {
        throw std::runtime_error("Not supported");
    }

    DerefProxy<StdStringAdapter> operator->() const
    {
        throw std::runtime_error("Not supported");
    }

    bool operator==(const StdStringArrayValueIterator &other) const
    {
        return true;
    }

    bool operator!=(const StdStringArrayValueIterator &other) const
    {
        return false;
    }

    const StdStringArrayValueIterator& operator++()
    {
        throw std::runtime_error("Not supported");
    }

    StdStringArrayValueIterator operator++(int)
    {
        throw std::runtime_error("Not supported");
    }

    const StdStringArrayValueIterator& operator--()
    {
        throw std::runtime_error("Not supported");
    }

    void advance(std::ptrdiff_t n)
    {
        throw std::runtime_error("Not supported");
    }
};

inline StdStringArrayValueIterator StdStringArray::begin() const
{
    return StdStringArrayValueIterator();
}

inline StdStringArrayValueIterator StdStringArray::end() const
{
    return StdStringArrayValueIterator();
}

class StdStringObjectMemberIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,
        StdStringObjectMember>
{
public:
    StdStringObjectMember operator*() const
    {
        throw std::runtime_error("Not supported");
    }

    DerefProxy<StdStringObjectMember> operator->() const
    {
        throw std::runtime_error("Not supported");
    }

    bool operator==(const StdStringObjectMemberIterator &) const
    {
        return true;
    }

    bool operator!=(const StdStringObjectMemberIterator &) const
    {
        return false;
    }

    const StdStringObjectMemberIterator& operator++()
    {
        throw std::runtime_error("Not supported");
    }

    StdStringObjectMemberIterator operator++(int)
    {
        throw std::runtime_error("Not supported");
    }

    const StdStringObjectMemberIterator& operator--()
    {
        throw std::runtime_error("Not supported");
    }
};

inline StdStringObjectMemberIterator StdStringObject::begin() const
{
    return StdStringObjectMemberIterator();
}

inline StdStringObjectMemberIterator StdStringObject::end() const
{
    return StdStringObjectMemberIterator();
}

inline StdStringObjectMemberIterator StdStringObject::find(const std::string &propertyName) const
{
    return StdStringObjectMemberIterator();
}

template<>
struct AdapterTraits<valijson::adapters::StdStringAdapter>
{
    typedef std::string DocumentType;

    static std::string adapterName()
    {
        return "StdStringAdapter";
    }
};

inline bool StdStringFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return StdStringAdapter(value).equalTo(other, strict);
}

}  // namespace adapters
}  // namespace valijson

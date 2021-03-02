/**
 * @file
 *
 * @brief   Class definitions to support JSON Schema constraints
 *
 * This file contains class definitions for all of the constraints required to
 * support JSON Schema. These classes all inherit from the BasicConstraint
 * template class, which implements the common parts of the Constraint
 * interface.
 *
 * @see BasicConstraint
 * @see Constraint
 */

#pragma once

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <valijson/adapters/frozen_value.hpp>
#include <valijson/constraints/basic_constraint.hpp>
#include <valijson/internal/custom_allocator.hpp>
#include <valijson/schema.hpp>

namespace valijson {
class ValidationResults;
namespace constraints {

/**
 * @brief  Represents an 'allOf' constraint.
 *
 * An allOf constraint provides a collection of sub-schemas that a value must
 * validate against. If a value fails to validate against any of these sub-
 * schemas, then validation fails.
 */
class AllOfConstraint: public BasicConstraint<AllOfConstraint>
{
public:
    AllOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    AllOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, all of which must be satisfied
    Subschemas subschemas;
};

/**
 * @brief  Represents an 'anyOf' constraint
 *
 * An anyOf constraint provides a collection of sub-schemas that a value can
 * validate against. If a value validates against one of these sub-schemas,
 * then the validation passes.
 */
class AnyOfConstraint: public BasicConstraint<AnyOfConstraint>
{
public:
    AnyOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    AnyOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
          internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, at least one of which must be satisfied
    Subschemas subschemas;
};

/**
 * @brief  Represents a combination 'if', 'then' and 'else' constraints
 *
 * The schema provided by an 'if' constraint is used as the expression for a conditional. When the
 * target validates against that schema, the 'then' subschema will be also be tested. Otherwise,
 * the 'else' subschema will be tested.
 */
class ConditionalConstraint: public BasicConstraint<ConditionalConstraint>
{
public:
    ConditionalConstraint()
      : ifSubschema(NULL),
        thenSubschema(NULL),
        elseSubschema(NULL) { }

    ConditionalConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        ifSubschema(NULL),
        thenSubschema(NULL),
        elseSubschema(NULL) { }

    const Subschema * getIfSubschema() const
    {
        return ifSubschema;
    }

    const Subschema * getThenSubschema() const
    {
        return thenSubschema;
    }

    const Subschema * getElseSubschema() const
    {
        return elseSubschema;
    }

    void setIfSubschema(const Subschema *subschema)
    {
        ifSubschema = subschema;
    }

    void setThenSubschema(const Subschema *subschema)
    {
        thenSubschema = subschema;
    }

    void setElseSubschema(const Subschema *subschema)
    {
        elseSubschema = subschema;
    }

private:
    const Subschema *ifSubschema;
    const Subschema *thenSubschema;
    const Subschema *elseSubschema;
};

class ConstConstraint: public BasicConstraint<ConstConstraint>
{
public:
    ConstConstraint()
      : value(nullptr) { }

    ConstConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        value(nullptr) { }

    ConstConstraint(const ConstConstraint &other)
      : BasicConstraint(other),
        value(other.value->clone()) { }

    adapters::FrozenValue * getValue() const
    {
        return value;
    }

    void setValue(const adapters::Adapter &value)
    {
        this->value = value.freeze();
    }

private:
    adapters::FrozenValue *value;
};

/**
 * @brief  Represents a 'contains' constraint
 *
 * A 'contains' constraint specifies a schema that must be satisfied by at least one
 * of the values in an array.
 */
class ContainsConstraint: public BasicConstraint<ContainsConstraint>
{
public:
    ContainsConstraint()
      : subschema(nullptr) { }

    ContainsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        this->subschema = subschema;
    }

private:
    const Subschema *subschema;
};

/**
 * @brief  Represents a 'dependencies' constraint.
 *
 * A dependency constraint ensures that a given property is valid only if the
 * properties that it depends on are present.
 */
class DependenciesConstraint: public BasicConstraint<DependenciesConstraint>
{
public:
    DependenciesConstraint()
      : propertyDependencies(std::less<String>(), allocator),
        schemaDependencies(std::less<String>(), allocator)
    { }

    DependenciesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        propertyDependencies(std::less<String>(), allocator),
        schemaDependencies(std::less<String>(), allocator)
    { }

    template<typename StringType>
    DependenciesConstraint & addPropertyDependency(
            const StringType &propertyName,
            const StringType &dependencyName)
    {
        const String key(propertyName.c_str(), allocator);
        PropertyDependencies::iterator itr = propertyDependencies.find(key);
        if (itr == propertyDependencies.end()) {
            itr = propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), allocator))).first;
        }

        itr->second.insert(String(dependencyName.c_str(), allocator));

        return *this;
    }

    template<typename StringType, typename ContainerType>
    DependenciesConstraint & addPropertyDependencies(
            const StringType &propertyName,
            const ContainerType &dependencyNames)
    {
        const String key(propertyName.c_str(), allocator);
        PropertyDependencies::iterator itr = propertyDependencies.find(key);
        if (itr == propertyDependencies.end()) {
            itr = propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), allocator))).first;
        }

        typedef typename ContainerType::value_type ValueType;
        for (const ValueType &dependencyName : dependencyNames) {
            itr->second.insert(String(dependencyName.c_str(), allocator));
        }

        return *this;
    }

    template<typename StringType>
    DependenciesConstraint & addSchemaDependency(
            const StringType &propertyName,
            const Subschema *schemaDependency)
    {
        if (schemaDependencies.insert(SchemaDependencies::value_type(
                String(propertyName.c_str(), allocator),
                schemaDependency)).second) {
            return *this;
        }

        throw std::runtime_error(
                "Dependencies constraint already contains a dependent "
                "schema for the property '" + propertyName + "'");
    }

    template<typename FunctorType>
    void applyToPropertyDependencies(const FunctorType &fn) const
    {
        for (const PropertyDependencies::value_type &v : propertyDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaDependencies(const FunctorType &fn) const
    {
        for (const SchemaDependencies::value_type &v : schemaDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>, internal::CustomAllocator<String> > PropertySet;

    typedef std::map<String, PropertySet, std::less<String>,
            internal::CustomAllocator<std::pair<const String, PropertySet> > > PropertyDependencies;

    typedef std::map<String, const Subschema *, std::less<String>,
            internal::CustomAllocator<std::pair<const String, const Subschema *> > >
                SchemaDependencies;

    /// Mapping from property names to their property-based dependencies
    PropertyDependencies propertyDependencies;

    /// Mapping from property names to their schema-based dependencies
    SchemaDependencies schemaDependencies;
};

/**
 * @brief  Represents an 'enum' constraint
 *
 * An enum constraint provides a collection of permissible values for a JSON
 * node. The node will only validate against this constraint if it matches one
 * or more of the values in the collection.
 */
class EnumConstraint: public BasicConstraint<EnumConstraint>
{
public:
    EnumConstraint()
      : enumValues(Allocator::rebind<const EnumValue *>::other(allocator)) { }

    EnumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        enumValues(Allocator::rebind<const EnumValue *>::other(allocator)) { }

    EnumConstraint(const EnumConstraint &other)
      : BasicConstraint(other),
        enumValues(Allocator::rebind<const EnumValue *>::other(allocator))
    {
        try {
            // Clone individual enum values
            for (const EnumValue *otherValue : other.enumValues) {
                const EnumValue *value = otherValue->clone();
                try {
                    enumValues.push_back(value);
                } catch (...) {
                    delete value;
                    throw;
                }
            }
        } catch (...) {
            // Delete values already added to constraint
            for (const EnumValue *value : enumValues) {
                delete value;
            }
            throw;
        }
    }

    virtual ~EnumConstraint()
    {
        for (const EnumValue *value : enumValues) {
            delete value;
        }
    }

    void addValue(const adapters::Adapter &value)
    {
        // TODO: Freeze value using custom alloc/free functions
        enumValues.push_back(value.freeze());
    }

    void addValue(const adapters::FrozenValue &value)
    {
        // TODO: Clone using custom alloc/free functions
        enumValues.push_back(value.clone());
    }

    template<typename FunctorType>
    void applyToValues(const FunctorType &fn) const
    {
        for (const EnumValue *value : enumValues) {
            if (!fn(*value)) {
                return;
            }
        }
    }

private:
    typedef adapters::FrozenValue EnumValue;

    typedef std::vector<const EnumValue *,
            internal::CustomAllocator<const EnumValue *> > EnumValues;

    EnumValues enumValues;
};

/**
 * @brief  Represents non-singular 'items' and 'additionalItems' constraints
 *
 * Unlike the SingularItemsConstraint class, this class represents an 'items'
 * constraint that specifies an array of sub-schemas, which should be used to
 * validate each item in an array, in sequence. It also represents an optional
 * 'additionalItems' sub-schema that should be used when an array contains
 * more values than there are sub-schemas in the 'items' constraint.
 *
 * The prefix 'Linear' comes from the fact that this class contains a list of
 * sub-schemas that corresponding array items must be validated against, and
 * this validation is performed linearly (i.e. in sequence).
 */
class LinearItemsConstraint: public BasicConstraint<LinearItemsConstraint>
{
public:
    LinearItemsConstraint()
      : itemSubschemas(Allocator::rebind<const Subschema *>::other(allocator)),
        additionalItemsSubschema(NULL) { }

    LinearItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        itemSubschemas(Allocator::rebind<const Subschema *>::other(allocator)),
        additionalItemsSubschema(NULL) { }

    void addItemSubschema(const Subschema *subschema)
    {
        itemSubschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToItemSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : itemSubschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    const Subschema * getAdditionalItemsSubschema() const
    {
        return additionalItemsSubschema;
    }

    size_t getItemSubschemaCount() const
    {
        return itemSubschemas.size();
    }

    void setAdditionalItemsSubschema(const Subschema *subschema)
    {
        additionalItemsSubschema = subschema;
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    Subschemas itemSubschemas;

    const Subschema* additionalItemsSubschema;
};

/**
 * @brief   Represents 'maximum' and 'exclusiveMaximum' constraints
 */
class MaximumConstraint: public BasicConstraint<MaximumConstraint>
{
public:
    MaximumConstraint()
      : maximum(std::numeric_limits<double>::infinity()),
        exclusiveMaximum(false) { }

    MaximumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        maximum(std::numeric_limits<double>::infinity()),
        exclusiveMaximum(false) { }

    bool getExclusiveMaximum() const
    {
        return exclusiveMaximum;
    }

    void setExclusiveMaximum(bool newExclusiveMaximum)
    {
        exclusiveMaximum = newExclusiveMaximum;
    }

    double getMaximum() const
    {
        return maximum;
    }

    void setMaximum(double newMaximum)
    {
        maximum = newMaximum;
    }

private:
    double maximum;
    bool exclusiveMaximum;
};

/**
 * @brief   Represents a 'maxItems' constraint
 */
class MaxItemsConstraint: public BasicConstraint<MaxItemsConstraint>
{
public:
    MaxItemsConstraint()
      : maxItems(std::numeric_limits<uint64_t>::max()) { }

    MaxItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        maxItems(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxItems() const
    {
        return maxItems;
    }

    void setMaxItems(uint64_t newMaxItems)
    {
        maxItems = newMaxItems;
    }

private:
    uint64_t maxItems;
};

/**
 * @brief   Represents a 'maxLength' constraint
 */
class MaxLengthConstraint: public BasicConstraint<MaxLengthConstraint>
{
public:
    MaxLengthConstraint()
      : maxLength(std::numeric_limits<uint64_t>::max()) { }

    MaxLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        maxLength(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxLength() const
    {
        return maxLength;
    }

    void setMaxLength(uint64_t newMaxLength)
    {
        maxLength = newMaxLength;
    }

private:
    uint64_t maxLength;
};

/**
 * @brief   Represents a 'maxProperties' constraint
 */
class MaxPropertiesConstraint: public BasicConstraint<MaxPropertiesConstraint>
{
public:
    MaxPropertiesConstraint()
      : maxProperties(std::numeric_limits<uint64_t>::max()) { }

    MaxPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        maxProperties(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxProperties() const
    {
        return maxProperties;
    }

    void setMaxProperties(uint64_t newMaxProperties)
    {
        maxProperties = newMaxProperties;
    }

private:
    uint64_t maxProperties;
};

/**
 * @brief   Represents 'minimum' and 'exclusiveMinimum' constraints
 */
class MinimumConstraint: public BasicConstraint<MinimumConstraint>
{
public:
    MinimumConstraint()
      : minimum(-std::numeric_limits<double>::infinity()),
        exclusiveMinimum(false) { }

    MinimumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        minimum(-std::numeric_limits<double>::infinity()),
        exclusiveMinimum(false) { }

    bool getExclusiveMinimum() const
    {
        return exclusiveMinimum;
    }

    void setExclusiveMinimum(bool newExclusiveMinimum)
    {
        exclusiveMinimum = newExclusiveMinimum;
    }

    double getMinimum() const
    {
        return minimum;
    }

    void setMinimum(double newMinimum)
    {
        minimum = newMinimum;
    }

private:
    double minimum;
    bool exclusiveMinimum;
};

/**
 * @brief   Represents a 'minItems' constraint
 */
class MinItemsConstraint: public BasicConstraint<MinItemsConstraint>
{
public:
    MinItemsConstraint()
      : minItems(0) { }

    MinItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        minItems(0) { }

    uint64_t getMinItems() const
    {
        return minItems;
    }

    void setMinItems(uint64_t newMinItems)
    {
        minItems = newMinItems;
    }

private:
    uint64_t minItems;
};

/**
 * @brief   Represents a 'minLength' constraint
 */
class MinLengthConstraint: public BasicConstraint<MinLengthConstraint>
{
public:
    MinLengthConstraint()
      : minLength(0) { }

    MinLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        minLength(0) { }

    int64_t getMinLength() const
    {
        return minLength;
    }

    void setMinLength(uint64_t newMinLength)
    {
        minLength = newMinLength;
    }

private:
    uint64_t minLength;
};

/**
 * @brief   Represents a 'minProperties' constraint
 */
class MinPropertiesConstraint: public BasicConstraint<MinPropertiesConstraint>
{
public:
    MinPropertiesConstraint()
      : minProperties(0) { }

    MinPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        minProperties(0) { }

    uint64_t getMinProperties() const
    {
        return minProperties;
    }

    void setMinProperties(uint64_t newMinProperties)
    {
        minProperties = newMinProperties;
    }

private:
    uint64_t minProperties;
};

/**
 * @brief  Represents either 'multipleOf' or 'divisibleBy' constraints where
 *         the divisor is a floating point number
 */
class MultipleOfDoubleConstraint:
        public BasicConstraint<MultipleOfDoubleConstraint>
{
public:
    MultipleOfDoubleConstraint()
      : value(1.) { }

    MultipleOfDoubleConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        value(1.) { }

    double getDivisor() const
    {
        return value;
    }

    void setDivisor(double newValue)
    {
        value = newValue;
    }

private:
    double value;
};

/**
 * @brief  Represents either 'multipleOf' or 'divisibleBy' constraints where
 *         the divisor is of integer type
 */
class MultipleOfIntConstraint:
        public BasicConstraint<MultipleOfIntConstraint>
{
public:
    MultipleOfIntConstraint()
      : value(1) { }

    MultipleOfIntConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        value(1) { }

    int64_t getDivisor() const
    {
        return value;
    }

    void setDivisor(int64_t newValue)
    {
        value = newValue;
    }

private:
    int64_t value;
};

/**
 * @brief   Represents a 'not' constraint
 */
class NotConstraint: public BasicConstraint<NotConstraint>
{
public:
    NotConstraint()
      : subschema(NULL) { }

    NotConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschema(NULL) { }

    const Subschema * getSubschema() const
    {
        return subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        this->subschema = subschema;
    }

private:
    const Subschema *subschema;
};

/**
 * @brief   Represents a 'oneOf' constraint.
 */
class OneOfConstraint: public BasicConstraint<OneOfConstraint>
{
public:
    OneOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    OneOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, exactly one of which must be satisfied
    Subschemas subschemas;
};

/**
 * @brief   Represents a 'pattern' constraint
 */
class PatternConstraint: public BasicConstraint<PatternConstraint>
{
public:
    PatternConstraint()
      : pattern(Allocator::rebind<char>::other(allocator)) { }

    PatternConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        pattern(Allocator::rebind<char>::other(allocator)) { }

    template<typename AllocatorType>
    bool getPattern(std::basic_string<char, std::char_traits<char>,
            AllocatorType> &result) const
    {
        result.assign(this->pattern.c_str());
        return true;
    }

    template<typename AllocatorType>
    std::basic_string<char, std::char_traits<char>, AllocatorType> getPattern(
            const AllocatorType &alloc = AllocatorType()) const
    {
        return std::basic_string<char, std::char_traits<char>, AllocatorType>(
                pattern.c_str(), alloc);
    }

    template<typename AllocatorType>
    void setPattern(const std::basic_string<char, std::char_traits<char>,
            AllocatorType> &pattern)
    {
        this->pattern.assign(pattern.c_str());
    }

private:
    String pattern;
};

class PolyConstraint : public Constraint
{
public:
    virtual bool accept(ConstraintVisitor &visitor) const
    {
        return visitor.visit(*static_cast<const PolyConstraint*>(this));
    }

    virtual Constraint * clone(CustomAlloc allocFn, CustomFree freeFn) const
    {
        void *ptr = allocFn(sizeOf());
        if (!ptr) {
            throw std::runtime_error(
                    "Failed to allocate memory for cloned constraint");
        }

        try {
            return cloneInto(ptr);
        } catch (...) {
            freeFn(ptr);
            throw;
        }
    }

    virtual bool validate(const adapters::Adapter &target,
            const std::vector<std::string>& context,
            valijson::ValidationResults *results) const = 0;

private:
    virtual Constraint * cloneInto(void *) const = 0;

    virtual size_t sizeOf() const = 0;
};

/**
 * @brief   Represents a combination of 'properties', 'patternProperties' and
 *          'additionalProperties' constraints
 */
class PropertiesConstraint: public BasicConstraint<PropertiesConstraint>
{
public:
    PropertiesConstraint()
      : properties(std::less<String>(), allocator),
        patternProperties(std::less<String>(), allocator),
        additionalProperties(NULL) { }

    PropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        properties(std::less<String>(), allocator),
        patternProperties(std::less<String>(), allocator),
        additionalProperties(NULL) { }

    bool addPatternPropertySubschema(const char *patternProperty,
            const Subschema *subschema)
    {
        return patternProperties.insert(PropertySchemaMap::value_type(
                String(patternProperty, allocator), subschema)).second;
    }

    template<typename AllocatorType>
    bool addPatternPropertySubschema(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &patternProperty,
            const Subschema *subschema)
    {
        return addPatternPropertySubschema(patternProperty.c_str(), subschema);
    }

    bool addPropertySubschema(const char *propertyName,
            const Subschema *subschema)
    {
        return properties.insert(PropertySchemaMap::value_type(
                String(propertyName, allocator), subschema)).second;
    }

    template<typename AllocatorType>
    bool addPropertySubschema(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &propertyName,
            const Subschema *subschema)
    {
        return addPropertySubschema(propertyName.c_str(), subschema);
    }

    template<typename FunctorType>
    void applyToPatternProperties(const FunctorType &fn) const
    {
        typedef typename PropertySchemaMap::value_type ValueType;
        for (const ValueType &value : patternProperties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToProperties(const FunctorType &fn) const
    {
        typedef typename PropertySchemaMap::value_type ValueType;
        for (const ValueType &value : properties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    const Subschema * getAdditionalPropertiesSubschema() const
    {
        return additionalProperties;
    }

    void setAdditionalPropertiesSubschema(const Subschema *subschema)
    {
        additionalProperties = subschema;
    }

private:
    typedef std::map<String, const Subschema *, std::less<String>, internal::CustomAllocator<std::pair<const String, const Subschema *> > >
            PropertySchemaMap;

    PropertySchemaMap properties;
    PropertySchemaMap patternProperties;

    const Subschema *additionalProperties;
};

class PropertyNamesConstraint: public BasicConstraint<PropertyNamesConstraint>
{
public:
    PropertyNamesConstraint()
      : subschema(NULL) { }

    PropertyNamesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschema(NULL) { }

    const Subschema * getSubschema() const
    {
        return subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        this->subschema = subschema;
    }

private:
    const Subschema *subschema;
};

/**
 * @brief   Represents a 'required' constraint
 */
class RequiredConstraint: public BasicConstraint<RequiredConstraint>
{
public:
    RequiredConstraint()
      : requiredProperties(std::less<String>(), allocator) { }

    RequiredConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        requiredProperties(std::less<String>(), allocator) { }

    bool addRequiredProperty(const char *propertyName)
    {
        return requiredProperties.insert(String(propertyName,
                Allocator::rebind<char>::other(allocator))).second;
    }

    template<typename AllocatorType>
    bool addRequiredProperty(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &propertyName)
    {
        return addRequiredProperty(propertyName.c_str());
    }

    template<typename FunctorType>
    void applyToRequiredProperties(const FunctorType &fn) const
    {
        for (const String &propertyName : requiredProperties) {
            if (!fn(propertyName)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>,
            internal::CustomAllocator<String> > RequiredProperties;

    RequiredProperties requiredProperties;
};

/**
 * @brief  Represents an 'items' constraint that specifies one sub-schema
 *
 * A value is considered valid against this constraint if it is an array, and
 * each item in the array validates against the sub-schema specified by this
 * constraint.
 *
 * The prefix 'Singular' comes from the fact that array items must validate
 * against exactly one sub-schema.
 */
class SingularItemsConstraint: public BasicConstraint<SingularItemsConstraint>
{
public:
    SingularItemsConstraint()
      : itemsSubschema(NULL) { }

    SingularItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        itemsSubschema(NULL) { }

    const Subschema * getItemsSubschema() const
    {
        return itemsSubschema;
    }

    void setItemsSubschema(const Subschema *subschema)
    {
        itemsSubschema = subschema;
    }

private:
    const Subschema *itemsSubschema;
};

/**
 * @brief   Represents a 'type' constraint.
 */
class TypeConstraint: public BasicConstraint<TypeConstraint>
{
public:
    enum JsonType {
        kAny,
        kArray,
        kBoolean,
        kInteger,
        kNull,
        kNumber,
        kObject,
        kString
    };

    TypeConstraint()
      : namedTypes(std::less<JsonType>(), allocator),
        schemaTypes(Allocator::rebind<const Subschema *>::other(allocator)) { }

    TypeConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        namedTypes(std::less<JsonType>(), allocator),
        schemaTypes(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addNamedType(JsonType type)
    {
        namedTypes.insert(type);
    }

    void addSchemaType(const Subschema *subschema)
    {
        schemaTypes.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToNamedTypes(const FunctorType &fn) const
    {
        for (const JsonType namedType : namedTypes) {
            if (!fn(namedType)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaTypes(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : schemaTypes) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    template<typename AllocatorType>
    static JsonType jsonTypeFromString(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &typeName)
    {
        if (typeName.compare("any") == 0) {
            return kAny;
        } else if (typeName.compare("array") == 0) {
            return kArray;
        } else if (typeName.compare("boolean") == 0) {
            return kBoolean;
        } else if (typeName.compare("integer") == 0) {
            return kInteger;
        } else if (typeName.compare("null") == 0) {
            return kNull;
        } else if (typeName.compare("number") == 0) {
            return kNumber;
        } else if (typeName.compare("object") == 0) {
            return kObject;
        } else if (typeName.compare("string") == 0) {
            return kString;
        }

        throw std::runtime_error("Unrecognised JSON type name '" +
                std::string(typeName.c_str()) + "'");
    }

private:
    typedef std::set<JsonType, std::less<JsonType>, internal::CustomAllocator<JsonType> > NamedTypes;

    typedef std::vector<const Subschema *,
            Allocator::rebind<const Subschema *>::other> SchemaTypes;

    /// Set of named JSON types that serve as valid types
    NamedTypes namedTypes;

    /// Set of sub-schemas that serve as valid types
    SchemaTypes schemaTypes;
};

/**
 * @brief   Represents a 'uniqueItems' constraint
 */
class UniqueItemsConstraint: public BasicConstraint<UniqueItemsConstraint>
{
public:
    UniqueItemsConstraint() { }

    UniqueItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn) { }
};

} // namespace constraints
} // namespace valijson

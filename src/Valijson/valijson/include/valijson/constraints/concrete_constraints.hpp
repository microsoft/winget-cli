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
#include <valijson/exceptions.hpp>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

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
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    AllOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, all of which must be satisfied
    Subschemas m_subschemas;
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
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    AnyOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, at least one of which must be satisfied
    Subschemas m_subschemas;
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
      : m_ifSubschema(nullptr),
        m_thenSubschema(nullptr),
        m_elseSubschema(nullptr) { }

    ConditionalConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_ifSubschema(nullptr),
        m_thenSubschema(nullptr),
        m_elseSubschema(nullptr) { }

    const Subschema * getIfSubschema() const
    {
        return m_ifSubschema;
    }

    const Subschema * getThenSubschema() const
    {
        return m_thenSubschema;
    }

    const Subschema * getElseSubschema() const
    {
        return m_elseSubschema;
    }

    void setIfSubschema(const Subschema *subschema)
    {
        m_ifSubschema = subschema;
    }

    void setThenSubschema(const Subschema *subschema)
    {
        m_thenSubschema = subschema;
    }

    void setElseSubschema(const Subschema *subschema)
    {
        m_elseSubschema = subschema;
    }

private:
    const Subschema *m_ifSubschema;
    const Subschema *m_thenSubschema;
    const Subschema *m_elseSubschema;
};

class ConstConstraint: public BasicConstraint<ConstConstraint>
{
public:
    ConstConstraint()
      : m_value(nullptr) { }

    ConstConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(nullptr) { }

    ConstConstraint(const ConstConstraint &other)
      : BasicConstraint(other),
        m_value(other.m_value->clone()) { }

    adapters::FrozenValue * getValue() const
    {
        return m_value.get();
    }

    void setValue(const adapters::Adapter &value)
    {
        m_value = std::unique_ptr<adapters::FrozenValue>(value.freeze());
    }

private:
    std::unique_ptr<adapters::FrozenValue> m_value;
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
      : m_subschema(nullptr) { }

    ContainsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
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
      : m_propertyDependencies(std::less<String>(), m_allocator),
        m_schemaDependencies(std::less<String>(), m_allocator)
    { }

    DependenciesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_propertyDependencies(std::less<String>(), m_allocator),
        m_schemaDependencies(std::less<String>(), m_allocator)
    { }

    template<typename StringType>
    DependenciesConstraint & addPropertyDependency(
            const StringType &propertyName,
            const StringType &dependencyName)
    {
        const String key(propertyName.c_str(), m_allocator);
        auto itr = m_propertyDependencies.find(key);
        if (itr == m_propertyDependencies.end()) {
            itr = m_propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), m_allocator))).first;
        }

        itr->second.insert(String(dependencyName.c_str(), m_allocator));

        return *this;
    }

    template<typename StringType, typename ContainerType>
    DependenciesConstraint & addPropertyDependencies(
            const StringType &propertyName,
            const ContainerType &dependencyNames)
    {
        const String key(propertyName.c_str(), m_allocator);
        auto itr = m_propertyDependencies.find(key);
        if (itr == m_propertyDependencies.end()) {
            itr = m_propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), m_allocator))).first;
        }

        typedef typename ContainerType::value_type ValueType;
        for (const ValueType &dependencyName : dependencyNames) {
            itr->second.insert(String(dependencyName.c_str(), m_allocator));
        }

        return *this;
    }

    template<typename StringType>
    DependenciesConstraint & addSchemaDependency(const StringType &propertyName, const Subschema *schemaDependency)
    {
        if (m_schemaDependencies.insert(SchemaDependencies::value_type(
                String(propertyName.c_str(), m_allocator),
                schemaDependency)).second) {
            return *this;
        }

        throwRuntimeError("Dependencies constraint already contains a dependent "
                "schema for the property '" + propertyName + "'");
    }

    template<typename FunctorType>
    void applyToPropertyDependencies(const FunctorType &fn) const
    {
        for (const PropertyDependencies::value_type &v : m_propertyDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaDependencies(const FunctorType &fn) const
    {
        for (const SchemaDependencies::value_type &v : m_schemaDependencies) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>, internal::CustomAllocator<String>> PropertySet;

    typedef std::map<String, PropertySet, std::less<String>,
            internal::CustomAllocator<std::pair<const String, PropertySet>>> PropertyDependencies;

    typedef std::map<String, const Subschema *, std::less<String>,
            internal::CustomAllocator<std::pair<const String, const Subschema *>>> SchemaDependencies;

    /// Mapping from property names to their property-based dependencies
    PropertyDependencies m_propertyDependencies;

    /// Mapping from property names to their schema-based dependencies
    SchemaDependencies m_schemaDependencies;
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
      : m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator)) { }

    EnumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator)) { }

    EnumConstraint(const EnumConstraint &other)
      : BasicConstraint(other),
        m_enumValues(Allocator::rebind<const EnumValue *>::other(m_allocator))
    {
#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            // Clone individual enum values
            for (const EnumValue *otherValue : other.m_enumValues) {
                const EnumValue *value = otherValue->clone();
#if VALIJSON_USE_EXCEPTIONS
                try {
#endif
                    m_enumValues.push_back(value);
#if VALIJSON_USE_EXCEPTIONS
                } catch (...) {
                    delete value;
                    value = nullptr;
                    throw;
                }
            }
        } catch (...) {
            // Delete values already added to constraint
            for (const EnumValue *value : m_enumValues) {
                delete value;
            }
            throw;
#endif
        }
    }

    ~EnumConstraint() override
    {
        for (const EnumValue *value : m_enumValues) {
            delete value;
        }
    }

    void addValue(const adapters::Adapter &value)
    {
        // TODO: Freeze value using custom alloc/free functions
        m_enumValues.push_back(value.freeze());
    }

    void addValue(const adapters::FrozenValue &value)
    {
        // TODO: Clone using custom alloc/free functions
        m_enumValues.push_back(value.clone());
    }

    template<typename FunctorType>
    void applyToValues(const FunctorType &fn) const
    {
        for (const EnumValue *value : m_enumValues) {
            if (!fn(*value)) {
                return;
            }
        }
    }

private:
    typedef adapters::FrozenValue EnumValue;

    typedef std::vector<const EnumValue *, internal::CustomAllocator<const EnumValue *>> EnumValues;

    EnumValues m_enumValues;
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
      : m_itemSubschemas(Allocator::rebind<const Subschema *>::other(m_allocator)),
        m_additionalItemsSubschema(nullptr) { }

    LinearItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_itemSubschemas(Allocator::rebind<const Subschema *>::other(m_allocator)),
        m_additionalItemsSubschema(nullptr) { }

    void addItemSubschema(const Subschema *subschema)
    {
        m_itemSubschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToItemSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_itemSubschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    const Subschema * getAdditionalItemsSubschema() const
    {
        return m_additionalItemsSubschema;
    }

    size_t getItemSubschemaCount() const
    {
        return m_itemSubschemas.size();
    }

    void setAdditionalItemsSubschema(const Subschema *subschema)
    {
        m_additionalItemsSubschema = subschema;
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    Subschemas m_itemSubschemas;

    const Subschema* m_additionalItemsSubschema;
};

/**
 * @brief   Represents 'maximum' and 'exclusiveMaximum' constraints
 */
class MaximumConstraint: public BasicConstraint<MaximumConstraint>
{
public:
    MaximumConstraint()
      : m_maximum(std::numeric_limits<double>::infinity()),
        m_exclusiveMaximum(false) { }

    MaximumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maximum(std::numeric_limits<double>::infinity()),
        m_exclusiveMaximum(false) { }

    bool getExclusiveMaximum() const
    {
        return m_exclusiveMaximum;
    }

    void setExclusiveMaximum(bool newExclusiveMaximum)
    {
        m_exclusiveMaximum = newExclusiveMaximum;
    }

    double getMaximum() const
    {
        return m_maximum;
    }

    void setMaximum(double newMaximum)
    {
        m_maximum = newMaximum;
    }

private:
    double m_maximum;
    bool m_exclusiveMaximum;
};

/**
 * @brief   Represents a 'maxItems' constraint
 */
class MaxItemsConstraint: public BasicConstraint<MaxItemsConstraint>
{
public:
    MaxItemsConstraint()
      : m_maxItems(std::numeric_limits<uint64_t>::max()) { }

    MaxItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxItems(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxItems() const
    {
        return m_maxItems;
    }

    void setMaxItems(uint64_t newMaxItems)
    {
        m_maxItems = newMaxItems;
    }

private:
    uint64_t m_maxItems;
};

/**
 * @brief   Represents a 'maxLength' constraint
 */
class MaxLengthConstraint: public BasicConstraint<MaxLengthConstraint>
{
public:
    MaxLengthConstraint()
      : m_maxLength(std::numeric_limits<uint64_t>::max()) { }

    MaxLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxLength(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxLength() const
    {
        return m_maxLength;
    }

    void setMaxLength(uint64_t newMaxLength)
    {
        m_maxLength = newMaxLength;
    }

private:
    uint64_t m_maxLength;
};

/**
 * @brief   Represents a 'maxProperties' constraint
 */
class MaxPropertiesConstraint: public BasicConstraint<MaxPropertiesConstraint>
{
public:
    MaxPropertiesConstraint()
      : m_maxProperties(std::numeric_limits<uint64_t>::max()) { }

    MaxPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_maxProperties(std::numeric_limits<uint64_t>::max()) { }

    uint64_t getMaxProperties() const
    {
        return m_maxProperties;
    }

    void setMaxProperties(uint64_t newMaxProperties)
    {
        m_maxProperties = newMaxProperties;
    }

private:
    uint64_t m_maxProperties;
};

/**
 * @brief   Represents 'minimum' and 'exclusiveMinimum' constraints
 */
class MinimumConstraint: public BasicConstraint<MinimumConstraint>
{
public:
    MinimumConstraint()
      : m_minimum(-std::numeric_limits<double>::infinity()),
        m_exclusiveMinimum(false) { }

    MinimumConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minimum(-std::numeric_limits<double>::infinity()),
        m_exclusiveMinimum(false) { }

    bool getExclusiveMinimum() const
    {
        return m_exclusiveMinimum;
    }

    void setExclusiveMinimum(bool newExclusiveMinimum)
    {
        m_exclusiveMinimum = newExclusiveMinimum;
    }

    double getMinimum() const
    {
        return m_minimum;
    }

    void setMinimum(double newMinimum)
    {
        m_minimum = newMinimum;
    }

private:
    double m_minimum;
    bool m_exclusiveMinimum;
};

/**
 * @brief   Represents a 'minItems' constraint
 */
class MinItemsConstraint: public BasicConstraint<MinItemsConstraint>
{
public:
    MinItemsConstraint()
      : m_minItems(0) { }

    MinItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minItems(0) { }

    uint64_t getMinItems() const
    {
        return m_minItems;
    }

    void setMinItems(uint64_t newMinItems)
    {
        m_minItems = newMinItems;
    }

private:
    uint64_t m_minItems;
};

/**
 * @brief   Represents a 'minLength' constraint
 */
class MinLengthConstraint: public BasicConstraint<MinLengthConstraint>
{
public:
    MinLengthConstraint()
      : m_minLength(0) { }

    MinLengthConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minLength(0) { }

    uint64_t getMinLength() const
    {
        return m_minLength;
    }

    void setMinLength(uint64_t newMinLength)
    {
        m_minLength = newMinLength;
    }

private:
    uint64_t m_minLength;
};

/**
 * @brief   Represents a 'minProperties' constraint
 */
class MinPropertiesConstraint: public BasicConstraint<MinPropertiesConstraint>
{
public:
    MinPropertiesConstraint()
      : m_minProperties(0) { }

    MinPropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_minProperties(0) { }

    uint64_t getMinProperties() const
    {
        return m_minProperties;
    }

    void setMinProperties(uint64_t newMinProperties)
    {
        m_minProperties = newMinProperties;
    }

private:
    uint64_t m_minProperties;
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
      : m_value(1.) { }

    MultipleOfDoubleConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(1.) { }

    double getDivisor() const
    {
        return m_value;
    }

    void setDivisor(double newValue)
    {
        m_value = newValue;
    }

private:
    double m_value;
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
      : m_value(1) { }

    MultipleOfIntConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_value(1) { }

    int64_t getDivisor() const
    {
        return m_value;
    }

    void setDivisor(int64_t newValue)
    {
        m_value = newValue;
    }

private:
    int64_t m_value;
};

/**
 * @brief   Represents a 'not' constraint
 */
class NotConstraint: public BasicConstraint<NotConstraint>
{
public:
    NotConstraint()
      : m_subschema(nullptr) { }

    NotConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
};

/**
 * @brief   Represents a 'oneOf' constraint.
 */
class OneOfConstraint: public BasicConstraint<OneOfConstraint>
{
public:
    OneOfConstraint()
      : m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    OneOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschemas(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        m_subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_subschemas) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *, internal::CustomAllocator<const Subschema *>> Subschemas;

    /// Collection of sub-schemas, exactly one of which must be satisfied
    Subschemas m_subschemas;
};

/**
 * @brief   Represents a 'pattern' constraint
 */
class PatternConstraint: public BasicConstraint<PatternConstraint>
{
public:
    PatternConstraint()
      : m_pattern(Allocator::rebind<char>::other(m_allocator)) { }

    PatternConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_pattern(Allocator::rebind<char>::other(m_allocator)) { }

    template<typename AllocatorType>
    bool getPattern(std::basic_string<char, std::char_traits<char>, AllocatorType> &result) const
    {
        result.assign(m_pattern.c_str());
        return true;
    }

    template<typename AllocatorType>
    std::basic_string<char, std::char_traits<char>, AllocatorType> getPattern(
            const AllocatorType &alloc = AllocatorType()) const
    {
        return std::basic_string<char, std::char_traits<char>, AllocatorType>(m_pattern.c_str(), alloc);
    }

    template<typename AllocatorType>
    void setPattern(const std::basic_string<char, std::char_traits<char>, AllocatorType> &pattern)
    {
        m_pattern.assign(pattern.c_str());
    }

private:
    String m_pattern;
};

class PolyConstraint : public Constraint
{
public:
    bool accept(ConstraintVisitor &visitor) const override
    {
        return visitor.visit(*static_cast<const PolyConstraint*>(this));
    }

    Constraint * clone(CustomAlloc allocFn, CustomFree freeFn) const override
    {
        void *ptr = allocFn(sizeOf());
        if (!ptr) {
            throwRuntimeError("Failed to allocate memory for cloned constraint");
        }

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            return cloneInto(ptr);
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            freeFn(ptr);
            throw;
        }
#else
        // pretend to use freeFn to avoid warning in GCC 8.3
        (void)freeFn;
#endif
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
      : m_properties(std::less<String>(), m_allocator),
        m_patternProperties(std::less<String>(), m_allocator),
        m_additionalProperties(nullptr) { }

    PropertiesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_properties(std::less<String>(), m_allocator),
        m_patternProperties(std::less<String>(), m_allocator),
        m_additionalProperties(nullptr) { }

    bool addPatternPropertySubschema(const char *patternProperty, const Subschema *subschema)
    {
        return m_patternProperties.insert(PropertySchemaMap::value_type(
                String(patternProperty, m_allocator), subschema)).second;
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
        return m_properties.insert(PropertySchemaMap::value_type(
                String(propertyName, m_allocator), subschema)).second;
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
        for (const ValueType &value : m_patternProperties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToProperties(const FunctorType &fn) const
    {
        typedef typename PropertySchemaMap::value_type ValueType;
        for (const ValueType &value : m_properties) {
            if (!fn(value.first, value.second)) {
                return;
            }
        }
    }

    const Subschema * getAdditionalPropertiesSubschema() const
    {
        return m_additionalProperties;
    }

    void setAdditionalPropertiesSubschema(const Subschema *subschema)
    {
        m_additionalProperties = subschema;
    }

private:
    typedef std::map<
            String,
            const Subschema *,
            std::less<String>,
            internal::CustomAllocator<std::pair<const String, const Subschema *>>
        > PropertySchemaMap;

    PropertySchemaMap m_properties;
    PropertySchemaMap m_patternProperties;

    const Subschema *m_additionalProperties;
};

class PropertyNamesConstraint: public BasicConstraint<PropertyNamesConstraint>
{
public:
    PropertyNamesConstraint()
      : m_subschema(nullptr) { }

    PropertyNamesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_subschema(nullptr) { }

    const Subschema * getSubschema() const
    {
        return m_subschema;
    }

    void setSubschema(const Subschema *subschema)
    {
        m_subschema = subschema;
    }

private:
    const Subschema *m_subschema;
};

/**
 * @brief   Represents a 'required' constraint
 */
class RequiredConstraint: public BasicConstraint<RequiredConstraint>
{
public:
    RequiredConstraint()
      : m_requiredProperties(std::less<String>(), m_allocator) { }

    RequiredConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_requiredProperties(std::less<String>(), m_allocator) { }

    bool addRequiredProperty(const char *propertyName)
    {
        return m_requiredProperties.insert(String(propertyName,
                Allocator::rebind<char>::other(m_allocator))).second;
    }

    template<typename AllocatorType>
    bool addRequiredProperty(const std::basic_string<char, std::char_traits<char>, AllocatorType> &propertyName)
    {
        return addRequiredProperty(propertyName.c_str());
    }

    template<typename FunctorType>
    void applyToRequiredProperties(const FunctorType &fn) const
    {
        for (const String &propertyName : m_requiredProperties) {
            if (!fn(propertyName)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>,
            internal::CustomAllocator<String>> RequiredProperties;

    RequiredProperties m_requiredProperties;
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
      : m_itemsSubschema(nullptr) { }

    SingularItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_itemsSubschema(nullptr) { }

    const Subschema * getItemsSubschema() const
    {
        return m_itemsSubschema;
    }

    void setItemsSubschema(const Subschema *subschema)
    {
        m_itemsSubschema = subschema;
    }

private:
    const Subschema *m_itemsSubschema;
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
      : m_namedTypes(std::less<JsonType>(), m_allocator),
        m_schemaTypes(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    TypeConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        m_namedTypes(std::less<JsonType>(), m_allocator),
        m_schemaTypes(Allocator::rebind<const Subschema *>::other(m_allocator)) { }

    void addNamedType(JsonType type)
    {
        m_namedTypes.insert(type);
    }

    void addSchemaType(const Subschema *subschema)
    {
        m_schemaTypes.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToNamedTypes(const FunctorType &fn) const
    {
        for (const JsonType namedType : m_namedTypes) {
            if (!fn(namedType)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaTypes(const FunctorType &fn) const
    {
        unsigned int index = 0;
        for (const Subschema *subschema : m_schemaTypes) {
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

        throwRuntimeError("Unrecognised JSON type name '" +
                std::string(typeName.c_str()) + "'");
        abort();
    }

private:
    typedef std::set<JsonType, std::less<JsonType>, internal::CustomAllocator<JsonType>> NamedTypes;

    typedef std::vector<const Subschema *,
            Allocator::rebind<const Subschema *>::other> SchemaTypes;

    /// Set of named JSON types that serve as valid types
    NamedTypes m_namedTypes;

    /// Set of sub-schemas that serve as valid types
    SchemaTypes m_schemaTypes;
};

/**
 * @brief   Represents a 'uniqueItems' constraint
 */
class UniqueItemsConstraint: public BasicConstraint<UniqueItemsConstraint>
{
public:
    UniqueItemsConstraint() = default;

    UniqueItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn) { }
};

} // namespace constraints
} // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif

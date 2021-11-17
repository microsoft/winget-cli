#pragma once

#include <cmath>
#include <string>
#include <regex>
#include <unordered_map>

#include <valijson/adapters/std_string_adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/constraints/constraint_visitor.hpp>
#include <utility>
#include <valijson/validation_results.hpp>

#include <valijson/utils/utf8_utils.hpp>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

namespace valijson {

class ValidationResults;

/**
 * @brief   Implementation of the ConstraintVisitor interface that validates a
 *          target document
 *
 * @tparam  AdapterType  Adapter type for the target document.
 */
template<typename AdapterType>
class ValidationVisitor: public constraints::ConstraintVisitor
{
public:

    /**
     * @brief  Construct a new validator for a given target value and context.
     *
     * @param  target       Target value to be validated
     * @param  context      Current context for validation error descriptions,
     *                      only used if results is set.
     * @param  strictTypes  Use strict type comparison
     * @param  results      Optional pointer to ValidationResults object, for
     *                      recording error descriptions. If this pointer is set
     *                      to nullptr, validation errors will caused validation to
     *                      stop immediately.
     * @param  regexesCache Cache of already created std::regex objects for pattern
     *                      constraints.
     */
    ValidationVisitor(const AdapterType &target,
                      std::vector<std::string> context,
                      const bool strictTypes,
                      ValidationResults *results,
                      std::unordered_map<std::string, std::regex>& regexesCache)
      : m_target(target),
        m_context(std::move(context)),
        m_results(results),
        m_strictTypes(strictTypes),
        m_regexesCache(regexesCache) { }

    /**
     * @brief  Validate the target against a schema.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param   subschema  Sub-schema that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool validateSchema(const Subschema &subschema)
    {
        if (subschema.getAlwaysInvalid()) {
            return false;
        }

        // Wrap the validationCallback() function below so that it will be
        // passed a reference to a constraint (_1), and a reference to the
        // visitor (*this).
        Subschema::ApplyFunction fn(std::bind(validationCallback, std::placeholders::_1, std::ref(*this)));

        // Perform validation against each constraint defined in the schema
        if (m_results == nullptr) {
            // The applyStrict() function will return immediately if the
            // callback function returns false
            if (!subschema.applyStrict(fn)) {
                return false;
            }
        } else {
            // The apply() function will iterate over all constraints in the
            // schema, even if the callback function returns false. Once
            // iteration is complete, the apply() function will return true
            // only if all invokations of the callback function returned true.
            if (!subschema.apply(fn)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief  Validate a value against an AllOfConstraint
     *
     * An allOf constraint provides a set of child schemas against which the
     * target must be validated in order for the constraint to the satifisfied.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the child schemas are validated
     * successfully.
     *
     * @param  constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const AllOfConstraint &constraint) override
    {
        bool validated = true;
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, true, false, *this, m_results, nullptr, &validated));

        return validated;
    }

    /**
     * @brief   Validate a value against an AnyOfConstraint
     *
     * An anyOf constraint provides a set of child schemas, any of which the
     * target may be validated against in order for the constraint to the
     * satifisfied.
     *
     * Because an anyOf constraint does not require the target to validate
     * against all child schemas, if validation against a single schema fails,
     * the results will not be added to a ValidationResults object. Only if
     * validation fails for all child schemas will an error be added to the
     * ValidationResults object.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const AnyOfConstraint &constraint) override
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (m_results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, childResults, m_regexesCache);
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, false, true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0 && m_results) {
            ValidationResults::Error childError;
            while (childResults->popError(childError)) {
                m_results->pushError( childError.context, childError.description);
            }
            m_results->pushError(m_context, "Failed to validate against any schemas allowed by anyOf constraint.");
        }

        return numValidated > 0;
    }

    /**
     * @brief   Validate current node using a set of 'if', 'then' and 'else' subschemas
     *
     * A conditional constraint allows a document to be validated against one of two additional
     * subschemas (specified via 'then' or 'else' properties) depending on whether the document
     * satifies an optional subschema (specified via the 'if' property).
     *
     * @param   constraint  ConditionalConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const ConditionalConstraint &constraint) override
    {
        ValidationResults newResults;
        ValidationResults* conditionalResults = (m_results) ? &newResults : nullptr;

        // Create a validator to evaluate the conditional
        ValidationVisitor ifValidator(m_target, m_context, m_strictTypes, nullptr, m_regexesCache);
        ValidationVisitor thenElseValidator(m_target, m_context, m_strictTypes, conditionalResults, m_regexesCache);

        bool validated = false;
        if (ifValidator.validateSchema(*constraint.getIfSubschema())) {
            const Subschema *thenSubschema = constraint.getThenSubschema();
            validated = thenSubschema == nullptr || thenElseValidator.validateSchema(*thenSubschema);
        } else {
            const Subschema *elseSubschema = constraint.getElseSubschema();
            validated = elseSubschema == nullptr || thenElseValidator.validateSchema(*elseSubschema);
        }

        if (!validated && m_results) {
            ValidationResults::Error conditionalError;
            while (conditionalResults->popError(conditionalError)) {
                m_results->pushError(conditionalError.context, conditionalError.description);
            }
            m_results->pushError(m_context, "Failed to validate against a conditional schema set by if-then-else constraints.");
        }

        return validated;
    }

    /**
     * @brief   Validate current node using a 'const' constraint
     *
     * A const constraint allows a document to be validated against a specific value.
     *
     * @param   constraint  ConstConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \f false otherwise
     */
    bool visit(const ConstConstraint &constraint) override
    {
        if (!constraint.getValue()->equalTo(m_target, m_strictTypes)) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match expected value set by 'const' constraint.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief  Validate current node using a 'contains' constraint
     *
     * A contains constraint is satisfied if the target is not an array, or if it is an array,
     * only if it contains at least one value that matches the specified schema.
     *
     * @param   constraint  ContainsConstraint that the current node must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const ContainsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const Subschema *subschema = constraint.getSubschema();
        const typename AdapterType::Array arr = m_target.asArray();

        bool validated = false;
        for (const auto &el : arr) {
            ValidationVisitor containsValidator(el, m_context, m_strictTypes, nullptr, m_regexesCache);
            if (containsValidator.validateSchema(*subschema)) {
                validated = true;
                break;
            }
        }

        if (!validated) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to any values against subschema in 'contains' constraint.");
            }

            return false;
        }

        return validated;
    }

    /**
     * @brief   Validate current node against a 'dependencies' constraint
     *
     * A 'dependencies' constraint can be used to specify property-based or
     * schema-based dependencies that must be fulfilled when a particular
     * property is present in an object.
     *
     * Property-based dependencies define a set of properties that must be
     * present in addition to a particular property, whereas a schema-based
     * dependency defines an additional schema that the current document must
     * validate against.
     *
     * @param   constraint  DependenciesConstraint that the current node
     *                      must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    bool visit(const DependenciesConstraint &constraint) override
    {
        // Ignore non-objects
        if ((m_strictTypes && !m_target.isObject()) || (!m_target.maybeObject())) {
            return true;
        }

        // Object to be validated
        const typename AdapterType::Object object = m_target.asObject();

        // Cleared if validation fails
        bool validated = true;

        // Iterate over all dependent properties defined by this constraint,
        // invoking the DependentPropertyValidator functor once for each
        // set of dependent properties
        constraint.applyToPropertyDependencies(ValidatePropertyDependencies(object, m_context, m_results, &validated));
        if (!m_results && !validated) {
            return false;
        }

        // Iterate over all dependent schemas defined by this constraint,
        // invoking the DependentSchemaValidator function once for each schema
        // that must be validated if a given property is present
        constraint.applyToSchemaDependencies(ValidateSchemaDependencies(
                object, m_context, *this, m_results, &validated));
        if (!m_results && !validated) {
            return false;
        }

        return validated;
    }

    /**
     * @brief   Validate current node against an EnumConstraint
     *
     * Validation succeeds if the target is equal to one of the values provided
     * by the EnumConstraint.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const EnumConstraint &constraint) override
    {
        unsigned int numValidated = 0;
        constraint.applyToValues(
                ValidateEquality(m_target, m_context, false, true, m_strictTypes, nullptr, &numValidated));

        if (numValidated == 0) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match against any enum values.");
            }

            return false;
        }

        return numValidated > 0;
    }

    /**
     * @brief   Validate a value against a LinearItemsConstraint
     *
     * A LinearItemsConstraint represents an 'items' constraint that specifies,
     * for each item in array, an individual sub-schema that the item must
     * validate against. The LinearItemsConstraint class also captures the
     * presence of an 'additionalItems' constraint, which specifies a default
     * sub-schema that should be used if an array contains more items than
     * there are sub-schemas in the 'items' constraint.
     *
     * If the current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    bool visit(const LinearItemsConstraint &constraint) override
    {
        // Ignore values that are not arrays
        if ((m_strictTypes && !m_target.isArray()) || (!m_target.maybeArray())) {
            return true;
        }

        // Sub-schema to validate against when number of items in array exceeds
        // the number of sub-schemas provided by the 'items' constraint
        const Subschema * const additionalItemsSubschema = constraint.getAdditionalItemsSubschema();

        // Track how many items are validated using 'items' constraint
        unsigned int numValidated = 0;

        // Array to validate
        const typename AdapterType::Array arr = m_target.asArray();
        const size_t arrSize = arr.size();

        // Track validation status
        bool validated = true;

        // Validate as many items as possible using 'items' sub-schemas
        const size_t itemSubschemaCount = constraint.getItemSubschemaCount();
        if (itemSubschemaCount > 0) {
            if (!additionalItemsSubschema) {
                if (arrSize > itemSubschemaCount) {
                    if (!m_results) {
                        return false;
                    }
                    m_results->pushError(m_context, "Array contains more items than allowed by items constraint.");
                    validated = false;
                }
            }

            constraint.applyToItemSubschemas(
                    ValidateItems(arr, m_context, true, m_results != nullptr, m_strictTypes, m_results, &numValidated,
                            &validated, m_regexesCache));

            if (!m_results && !validated) {
                return false;
            }
        }

        // Validate remaining items using 'additionalItems' sub-schema
        if (numValidated < arrSize) {
            if (additionalItemsSubschema) {
                // Begin validation from the first item not validated against
                // an sub-schema provided by the 'items' constraint
                unsigned int index = numValidated;
                typename AdapterType::Array::const_iterator begin = arr.begin();
                begin.advance(numValidated);
                for (typename AdapterType::Array::const_iterator itr = begin;
                        itr != arr.end(); ++itr) {

                    // Update context for current array item
                    std::vector<std::string> newContext = m_context;
                    newContext.push_back("[" + std::to_string(index) + "]");

                    ValidationVisitor<AdapterType> validator(*itr, newContext, m_strictTypes, m_results, m_regexesCache);

                    if (!validator.validateSchema(*additionalItemsSubschema)) {
                        if (m_results) {
                            m_results->pushError(m_context, "Failed to validate item #" + std::to_string(index) +
                                    " against additional items schema.");
                            validated = false;
                        } else {
                            return false;
                        }
                    }

                    index++;
                }

            } else if (m_results) {
                m_results->pushError(m_context, "Cannot validate item #" + std::to_string(numValidated) +
                        " or greater using 'items' constraint or 'additionalItems' constraint.");
                validated = false;

            } else {
                return false;
            }
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a MaximumConstraint object
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraints are satisfied; \c false otherwise
     */
    bool visit(const MaximumConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isNumber()) || !m_target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double maximum = constraint.getMaximum();

        if (constraint.getExclusiveMaximum()) {
            if (m_target.asDouble() >= maximum) {
                if (m_results) {
                    m_results->pushError(m_context, "Expected number less than " + std::to_string(maximum));
                }

                return false;
            }

        } else if (m_target.asDouble() > maximum) {
            if (m_results) {
                m_results->pushError(m_context, "Expected number less than or equal to " + std::to_string(maximum));
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MaxItemsConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxItemsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const uint64_t maxItems = constraint.getMaxItems();
        if (m_target.asArray().size() <= maxItems) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Array should contain no more than " + std::to_string(maxItems) +
                    " elements.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MaxLengthConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxLengthConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        const std::string s = m_target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t maxLength = constraint.getMaxLength();
        if (len <= maxLength) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "String should be no more than " + std::to_string(maxLength) +
                    " characters in length.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MaxPropertiesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MaxPropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        const uint64_t maxProperties = constraint.getMaxProperties();

        if (m_target.asObject().size() <= maxProperties) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Object should have no more than " + std::to_string(maxProperties) +
                    " properties.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinimumConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinimumConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isNumber()) || !m_target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double minimum = constraint.getMinimum();

        if (constraint.getExclusiveMinimum()) {
            if (m_target.asDouble() <= minimum) {
                if (m_results) {
                    m_results->pushError(m_context, "Expected number greater than " + std::to_string(minimum));
                }

                return false;
            }
        } else if (m_target.asDouble() < minimum) {
            if (m_results) {
                m_results->pushError(m_context, "Expected number greater than or equal to " + std::to_string(minimum));
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MinItemsConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinItemsConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        const uint64_t minItems = constraint.getMinItems();
        if (m_target.asArray().size() >= minItems) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Array should contain no fewer than " + std::to_string(minItems) +
                    " elements.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinLengthConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinLengthConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        const std::string s = m_target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t minLength = constraint.getMinLength();
        if (len >= minLength) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "String should be no fewer than " + std::to_string(minLength) +
                    " characters in length.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MinPropertiesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MinPropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        const uint64_t minProperties = constraint.getMinProperties();

        if (m_target.asObject().size() >= minProperties) {
            return true;
        }

        if (m_results) {
            m_results->pushError(m_context, "Object should have no fewer than " + std::to_string(minProperties) +
                    " properties.");
        }

        return false;
    }

    /**
     * @brief   Validate a value against a MultipleOfDoubleConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MultipleOfDoubleConstraint &constraint) override
    {
        const double divisor = constraint.getDivisor();

        double d = 0.;
        if (m_target.maybeDouble()) {
            if (!m_target.asDouble(d)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted "
                            "to a number to check if it is a multiple of " + std::to_string(divisor));
                }
                return false;
            }
        } else if (m_target.maybeInteger()) {
            int64_t i = 0;
            if (!m_target.asInteger(i)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted "
                            "to a number to check if it is a multiple of " + std::to_string(divisor));
                }
                return false;
            }
            d = static_cast<double>(i);
        } else {
            return true;
        }

        if (d == 0) {
            return true;
        }

        const double r = remainder(d, divisor);

        if (fabs(r) > std::numeric_limits<double>::epsilon()) {
            if (m_results) {
                m_results->pushError(m_context, "Value should be a multiple of " + std::to_string(divisor));
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a MultipleOfIntConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const MultipleOfIntConstraint &constraint) override
    {
        const int64_t divisor = constraint.getDivisor();

        int64_t i = 0;
        if (m_target.maybeInteger()) {
            if (!m_target.asInteger(i)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted to an integer for multipleOf check");
                }
                return false;
            }
        } else if (m_target.maybeDouble()) {
            double d;
            if (!m_target.asDouble(d)) {
                if (m_results) {
                    m_results->pushError(m_context, "Value could not be converted to a double for multipleOf check");
                }
                return false;
            }
            i = static_cast<int64_t>(d);
        } else {
            return true;
        }

        if (i == 0) {
            return true;
        }

        if (i % divisor != 0) {
            if (m_results) {
                m_results->pushError(m_context, "Value should be a multiple of " + std::to_string(divisor));
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a NotConstraint
     *
     * If the subschema NotConstraint currently holds a nullptr, the
     * schema will be treated like the empty schema. Therefore validation
     * will always fail.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const NotConstraint &constraint) override
    {
        const Subschema *subschema = constraint.getSubschema();
        if (!subschema) {
            // Treat nullptr like empty schema
            return false;
        }

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, nullptr, m_regexesCache);
        if (v.validateSchema(*subschema)) {
            if (m_results) {
                m_results->pushError(m_context,
                        "Target should not validate against schema specified in 'not' constraint.");
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a OneOfConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const OneOfConstraint &constraint) override
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (m_results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(m_target, m_context, m_strictTypes, childResults, m_regexesCache);
        constraint.applyToSubschemas(
                ValidateSubschemas(m_target, m_context, true, true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0) {
            if (m_results) {
                ValidationResults::Error childError;
                while (childResults->popError(childError)) {
                    m_results->pushError(
                            childError.context,
                            childError.description);
                }
                m_results->pushError(m_context, "Failed to validate against any "
                        "child schemas allowed by oneOf constraint.");
            }
            return false;
        } else if (numValidated != 1) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to validate against exactly one child schema.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a PatternConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const PatternConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isString()) || !m_target.maybeString()) {
            return true;
        }

        std::string pattern(constraint.getPattern<std::string::allocator_type>());
        auto it = m_regexesCache.find(pattern);
        if (it == m_regexesCache.end()) {
            it = m_regexesCache.emplace(pattern, std::regex(pattern)).first;
        }

        if (!std::regex_search(m_target.asString(), it->second)) {
            if (m_results) {
                m_results->pushError(m_context, "Failed to match regex specified by 'pattern' constraint.");
            }

            return false;
        }

        return true;
    }

    /**
     * @brief   Validate a value against a PatternConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const constraints::PolyConstraint &constraint) override
    {
        return constraint.validate(m_target, m_context, m_results);
    }

    /**
     * @brief   Validate a value against a PropertiesConstraint
     *
     * Validation of an object against a PropertiesConstraint proceeds in three
     * stages. The first stage finds all properties in the object that have a
     * corresponding subschema in the constraint, and validates those properties
     * recursively.
     *
     * Next, the object's properties will be validated against the subschemas
     * for any 'patternProperties' that match a given property name. A property
     * is required to validate against the sub-schema for all patterns that it
     * matches.
     *
     * Finally, any properties that have not yet been validated against at least
     * one subschema will be validated against the 'additionalItems' subschema.
     * If this subschema is not present, then all properties must have been
     * validated at least once.
     *
     * Non-object values are always considered valid.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if the constraint is satisfied; \c false otherwise
     */
    bool visit(const PropertiesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        bool validated = true;

        // Track which properties have already been validated
        std::set<std::string> propertiesMatched;

        // Validate properties against subschemas for matching 'properties'
        // constraints
        const typename AdapterType::Object object = m_target.asObject();
        constraint.applyToProperties(
                ValidatePropertySubschemas(
                        object, m_context, true, m_results != nullptr, true, m_strictTypes, m_results,
                        &propertiesMatched, &validated, m_regexesCache));

        // Exit early if validation failed, and we're not collecting exhaustive
        // validation results
        if (!validated && !m_results) {
            return false;
        }

        // Validate properties against subschemas for matching patternProperties
        // constraints
        constraint.applyToPatternProperties(
                ValidatePatternPropertySubschemas(
                        object, m_context, true, false, true, m_strictTypes, m_results, &propertiesMatched,
                        &validated, m_regexesCache));

        // Validate against additionalProperties subschema for any properties
        // that have not yet been matched
        const Subschema *additionalPropertiesSubschema =
                constraint.getAdditionalPropertiesSubschema();
        if (!additionalPropertiesSubschema) {
            if (propertiesMatched.size() != m_target.getObjectSize()) {
                if (m_results) {
                    std::string unwanted;
                    for (const typename AdapterType::ObjectMember m : object) {
                        if (propertiesMatched.find(m.first) == propertiesMatched.end()) {
                            unwanted = m.first;
                            break;
                        }
                    }
                    m_results->pushError(m_context, "Object contains a property "
                            "that could not be validated using 'properties' "
                            "or 'additionalProperties' constraints: '" + unwanted + "'.");
                }

                return false;
            }

            return validated;
        }

        for (const typename AdapterType::ObjectMember m : object) {
            if (propertiesMatched.find(m.first) == propertiesMatched.end()) {
                // Update context
                std::vector<std::string> newContext = m_context;
                newContext.push_back("[" + m.first + "]");

                // Create a validator to validate the property's value
                ValidationVisitor validator(m.second, newContext, m_strictTypes, m_results, m_regexesCache);
                if (!validator.validateSchema(*additionalPropertiesSubschema)) {
                    if (m_results) {
                        m_results->pushError(m_context, "Failed to validate against additional properties schema");
                    }

                    validated = false;
                }
            }
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a PropertyNamesConstraint
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const PropertyNamesConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        for (const typename AdapterType::ObjectMember m : m_target.asObject()) {
            adapters::StdStringAdapter stringAdapter(m.first);
            ValidationVisitor<adapters::StdStringAdapter> validator(stringAdapter, m_context, m_strictTypes, nullptr, m_regexesCache);
            if (!validator.validateSchema(*constraint.getSubschema())) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate a value against a RequiredConstraint
     *
     * A required constraint specifies a list of properties that must be present
     * in the target.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  \c true if validation succeeds; \c false otherwise
     */
    bool visit(const RequiredConstraint &constraint) override
    {
        if ((m_strictTypes && !m_target.isObject()) || !m_target.maybeObject()) {
            return true;
        }

        bool validated = true;
        const typename AdapterType::Object object = m_target.asObject();
        constraint.applyToRequiredProperties(
                ValidateProperties(object, m_context, true, m_results != nullptr, m_results, &validated));

        return validated;
    }

    /**
     * @brief  Validate a value against a SingularItemsConstraint
     *
     * A SingularItemsConstraint represents an 'items' constraint that specifies
     * a sub-schema against which all items in an array must validate. If the
     * current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    bool visit(const SingularItemsConstraint &constraint) override
    {
        // Ignore values that are not arrays
        if (!m_target.isArray()) {
            return true;
        }

        // Schema against which all items must validate
        const Subschema *itemsSubschema = constraint.getItemsSubschema();

        // Default items sub-schema accepts all values
        if (!itemsSubschema) {
            return true;
        }

        // Track whether validation has failed
        bool validated = true;

        unsigned int index = 0;
        for (const AdapterType &item : m_target.getArray()) {
            // Update context for current array item
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + std::to_string(index) + "]");

            // Create a validator for the current array item
            ValidationVisitor<AdapterType> validationVisitor(item, newContext, m_strictTypes, m_results, m_regexesCache);

            // Perform validation
            if (!validationVisitor.validateSchema(*itemsSubschema)) {
                if (m_results) {
                    m_results->pushError(m_context, "Failed to validate item #" + std::to_string(index) + " in array.");
                    validated = false;
                } else {
                    return false;
                }
            }

            index++;
        }

        return validated;
    }

    /**
     * @brief   Validate a value against a TypeConstraint
     *
     * Checks that the target is one of the valid named types, or matches one
     * of a set of valid sub-schemas.
     *
     * @param   constraint  TypeConstraint to validate against
     *
     * @return  \c true if validation is successful; \c false otherwise
     */
    bool visit(const TypeConstraint &constraint) override
    {
        // Check named types
        {
            // ValidateNamedTypes functor assumes target is invalid
            bool validated = false;
            constraint.applyToNamedTypes(ValidateNamedTypes(m_target, false, true, m_strictTypes, &validated));
            if (validated) {
                return true;
            }
        }

        // Check schema-based types
        {
            unsigned int numValidated = 0;
            constraint.applyToSchemaTypes(
                    ValidateSubschemas(m_target, m_context, false, true, *this, nullptr, &numValidated, nullptr));
            if (numValidated > 0) {
                return true;
            } else if (m_results) {
                m_results->pushError(m_context, "Value type not permitted by 'type' constraint.");
            }
        }

        return false;
    }

    /**
     * @brief   Validate the uniqueItems constraint represented by a
     *          UniqueItems object.
     *
     * A uniqueItems constraint requires that each of the values in an array
     * are unique. Comparison is performed recursively.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  true if validation succeeds, false otherwise
     */
    bool visit(const UniqueItemsConstraint &) override
    {
        if ((m_strictTypes && !m_target.isArray()) || !m_target.maybeArray()) {
            return true;
        }

        // Empty arrays are always valid
        if (m_target.getArraySize() == 0) {
            return true;
        }

        bool validated = true;

        const typename AdapterType::Array targetArray = m_target.asArray();
        const typename AdapterType::Array::const_iterator end = targetArray.end();
        const typename AdapterType::Array::const_iterator secondLast = --targetArray.end();
        unsigned int outerIndex = 0;
        typename AdapterType::Array::const_iterator outerItr = targetArray.begin();
        for (; outerItr != secondLast; ++outerItr) {
            unsigned int innerIndex = outerIndex + 1;
            typename AdapterType::Array::const_iterator innerItr(outerItr);
            for (++innerItr; innerItr != end; ++innerItr) {
                if (outerItr->equalTo(*innerItr, true)) {
                    if (!m_results) {
                        return false;
                    }
                    m_results->pushError(m_context, "Elements at indexes #" + std::to_string(outerIndex)
                        + " and #" + std::to_string(innerIndex) + " violate uniqueness constraint.");
                    validated = false;
                }
                ++innerIndex;
            }
            ++outerIndex;
        }

        return validated;
    }

private:

    /**
     * @brief  Functor to compare a node with a collection of values
     */
    struct ValidateEquality
    {
        ValidateEquality(
                const AdapterType &target,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                ValidationResults *results,
                unsigned int *numValidated)
          : m_target(target),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_results(results),
            m_numValidated(numValidated) { }

        template<typename OtherValue>
        bool operator()(const OtherValue &value) const
        {
            if (value.equalTo(m_target, m_strictTypes)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_results) {
                m_results->pushError(m_context, "Target value and comparison value are not equal");
            }

            return m_continueOnFailure;
        }

    private:
        const AdapterType &m_target;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        bool m_strictTypes;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
    };

    /**
     * @brief  Functor to validate the presence of a set of properties
     */
    struct ValidateProperties
    {
        ValidateProperties(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &property) const
        {
            if (m_object.find(property.c_str()) == m_object.end()) {
                if (m_validated) {
                    *m_validated = false;
                }

                if (m_results) {
                    m_results->pushError(m_context, "Missing required property '" +
                            std::string(property.c_str()) + "'.");
                }

                return m_continueOnFailure;
            }

            return m_continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate property-based dependencies
     */
    struct ValidatePropertyDependencies
    {
        ValidatePropertyDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType, typename ContainerType>
        bool operator()(const StringType &propertyName, const ContainerType &dependencyNames) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (m_object.find(propertyNameKey) == m_object.end()) {
                return true;
            }

            typedef typename ContainerType::value_type ValueType;
            for (const ValueType &dependencyName : dependencyNames) {
                const std::string dependencyNameKey(dependencyName.c_str());
                if (m_object.find(dependencyNameKey) == m_object.end()) {
                    if (m_validated) {
                        *m_validated = false;
                    }
                    if (m_results) {
                        m_results->pushError(m_context, "Missing dependency '" + dependencyNameKey + "'.");
                    } else {
                        return false;
                    }
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate against sub-schemas in 'items' constraint
     */
    struct ValidateItems
    {
        ValidateItems(
                const typename AdapterType::Array &arr,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_arr(arr),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_results(results),
            m_numValidated(numValidated),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            // Check that there are more elements to validate
            if (index >= m_arr.size()) {
                return false;
            }

            // Update context
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + std::to_string(index) + "]");

            // Find array item
            typename AdapterType::Array::const_iterator itr = m_arr.begin();
            itr.advance(index);

            // Validate current array item
            ValidationVisitor validator(*itr, newContext, m_strictTypes, m_results, m_regexesCache);
            if (validator.validateSchema(*subschema)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_validated) {
                *m_validated = false;
            }

            if (m_results) {
                m_results->pushError(newContext, "Failed to validate item #" + std::to_string(index) +
                    " against corresponding item schema.");
            }

            return m_continueOnFailure;
        }

    private:
        const typename AdapterType::Array &m_arr;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        bool m_strictTypes;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate value against named JSON types
     */
    struct ValidateNamedTypes
    {
        ValidateNamedTypes(
                const AdapterType &target,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool strictTypes,
                bool *validated)
          : m_target(target),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_strictTypes(strictTypes),
            m_validated(validated) { }

        bool operator()(constraints::TypeConstraint::JsonType jsonType) const
        {
            typedef constraints::TypeConstraint TypeConstraint;

            bool valid = false;

            switch (jsonType) {
            case TypeConstraint::kAny:
                valid = true;
                break;
            case TypeConstraint::kArray:
                valid = m_target.isArray();
                break;
            case TypeConstraint::kBoolean:
                valid = m_target.isBool() || (!m_strictTypes && m_target.maybeBool());
                break;
            case TypeConstraint::kInteger:
                valid = m_target.isInteger() || (!m_strictTypes && m_target.maybeInteger());
                break;
            case TypeConstraint::kNull:
                valid = m_target.isNull() || (!m_strictTypes && m_target.maybeNull());
                break;
            case TypeConstraint::kNumber:
                valid = m_target.isNumber() || (!m_strictTypes && m_target.maybeDouble());
                break;
            case TypeConstraint::kObject:
                valid = m_target.isObject();
                break;
            case TypeConstraint::kString:
                valid = m_target.isString();
                break;
            default:
                break;
            }

            if (valid && m_validated) {
                *m_validated = true;
            }

            return (valid && m_continueOnSuccess) || m_continueOnFailure;
        }

    private:
        const AdapterType m_target;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_strictTypes;
        bool * const m_validated;
    };

    /**
     * @brief  Functor to validate object properties against sub-schemas
     *         defined by a 'patternProperties' constraint
     */
    struct ValidatePatternPropertySubschemas
    {
        ValidatePatternPropertySubschemas(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool continueIfUnmatched,
                bool strictTypes,
                ValidationResults *results,
                std::set<std::string> *propertiesMatched,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_continueIfUnmatched(continueIfUnmatched),
            m_strictTypes(strictTypes),
            m_results(results),
            m_propertiesMatched(propertiesMatched),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        template<typename StringType>
        bool operator()(const StringType &patternProperty, const Subschema *subschema) const
        {
            const std::string patternPropertyStr(patternProperty.c_str());

            // It would be nice to store pre-allocated regex objects in the
            // PropertiesConstraint. does std::regex currently support
            // custom allocators? Anyway, this isn't an issue here, because Valijson's
            // JSON Scheme validator does not yet support custom allocators.
            const std::regex r(patternPropertyStr);

            bool matchFound = false;

            // Recursively validate all matching properties
            typedef const typename AdapterType::ObjectMember ObjectMember;
            for (const ObjectMember m : m_object) {
                if (std::regex_search(m.first, r)) {
                    matchFound = true;
                    if (m_propertiesMatched) {
                        m_propertiesMatched->insert(m.first);
                    }

                    // Update context
                    std::vector<std::string> newContext = m_context;
                    newContext.push_back("[" + m.first + "]");

                    // Recursively validate property's value
                    ValidationVisitor validator(m.second, newContext, m_strictTypes, m_results, m_regexesCache);
                    if (validator.validateSchema(*subschema)) {
                        continue;
                    }

                    if (m_results) {
                        m_results->pushError(m_context, "Failed to validate against schema associated with pattern '" +
                                patternPropertyStr + "'.");
                    }

                    if (m_validated) {
                        *m_validated = false;
                    }

                    if (!m_continueOnFailure) {
                        return false;
                    }
                }
            }

            // Allow iteration to terminate if there was not at least one match
            if (!matchFound && !m_continueIfUnmatched) {
                return false;
            }

            return m_continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_continueIfUnmatched;
        const bool m_strictTypes;
        ValidationResults * const m_results;
        std::set<std::string> * const m_propertiesMatched;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate object properties against sub-schemas defined
     *         by a 'properties' constraint
     */
    struct ValidatePropertySubschemas
    {
        ValidatePropertySubschemas(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                bool continueIfUnmatched,
                bool strictTypes,
                ValidationResults *results,
                std::set<std::string> *propertiesMatched,
                bool *validated,
                std::unordered_map<std::string, std::regex>& regexesCache)
          : m_object(object),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_continueIfUnmatched(continueIfUnmatched),
            m_strictTypes(strictTypes),
            m_results(results),
            m_propertiesMatched(propertiesMatched),
            m_validated(validated),
            m_regexesCache(regexesCache) { }

        template<typename StringType>
        bool operator()(const StringType &propertyName, const Subschema *subschema) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            const typename AdapterType::Object::const_iterator itr = m_object.find(propertyNameKey);
            if (itr == m_object.end()) {
                return m_continueIfUnmatched;
            }

            if (m_propertiesMatched) {
                m_propertiesMatched->insert(propertyNameKey);
            }

            // Update context
            std::vector<std::string> newContext = m_context;
            newContext.push_back("[" + propertyNameKey + "]");

            // Recursively validate property's value
            ValidationVisitor validator(itr->second, newContext, m_strictTypes, m_results, m_regexesCache);
            if (validator.validateSchema(*subschema)) {
                return m_continueOnSuccess;
            }

            if (m_results) {
                m_results->pushError(m_context, "Failed to validate against schema associated with property name '" +
                        propertyNameKey + "'.");
            }

            if (m_validated) {
                *m_validated = false;
            }

            return m_continueOnFailure;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        const bool m_continueOnSuccess;
        const bool m_continueOnFailure;
        const bool m_continueIfUnmatched;
        const bool m_strictTypes;
        ValidationResults * const m_results;
        std::set<std::string> * const m_propertiesMatched;
        bool * const m_validated;
        std::unordered_map<std::string, std::regex>& m_regexesCache;
    };

    /**
     * @brief  Functor to validate schema-based dependencies
     */
    struct ValidateSchemaDependencies
    {
        ValidateSchemaDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                bool *validated)
          : m_object(object),
            m_context(context),
            m_validationVisitor(validationVisitor),
            m_results(results),
            m_validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &propertyName, const Subschema *schemaDependency) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (m_object.find(propertyNameKey) == m_object.end()) {
                return true;
            }

            if (!m_validationVisitor.validateSchema(*schemaDependency)) {
                if (m_validated) {
                    *m_validated = false;
                }
                if (m_results) {
                    m_results->pushError(m_context, "Failed to validate against dependent schema.");
                } else {
                    return false;
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &m_object;
        const std::vector<std::string> &m_context;
        ValidationVisitor &m_validationVisitor;
        ValidationResults * const m_results;
        bool * const m_validated;
    };

    /**
     * @brief  Functor that can be used to validate one or more subschemas
     *
     * This functor is designed to be applied to collections of subschemas
     * contained within 'allOf', 'anyOf' and 'oneOf' constraints.
     *
     * The return value depends on whether a given schema validates, with the
     * actual return value for a given case being decided at construction time.
     * The return value is used by the 'applyToSubschemas' functions in the
     * AllOfConstraint, AnyOfConstraint and OneOfConstrant classes to decide
     * whether to terminate early.
     *
     * The functor uses output parameters (provided at construction) to update
     * validation state that may be needed by the caller.
     */
    struct ValidateSubschemas
    {
        ValidateSubschemas(
                const AdapterType &adapter,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated)
          : m_adapter(adapter),
            m_context(context),
            m_continueOnSuccess(continueOnSuccess),
            m_continueOnFailure(continueOnFailure),
            m_validationVisitor(validationVisitor),
            m_results(results),
            m_numValidated(numValidated),
            m_validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            if (m_validationVisitor.validateSchema(*subschema)) {
                if (m_numValidated) {
                    (*m_numValidated)++;
                }

                return m_continueOnSuccess;
            }

            if (m_validated) {
                *m_validated = false;
            }

            if (m_results) {
                m_results->pushError(m_context,
                        "Failed to validate against child schema #" + std::to_string(index) + ".");
            }

            return m_continueOnFailure;
        }

    private:
        const AdapterType &m_adapter;
        const std::vector<std::string> &m_context;
        bool m_continueOnSuccess;
        bool m_continueOnFailure;
        ValidationVisitor &m_validationVisitor;
        ValidationResults * const m_results;
        unsigned int * const m_numValidated;
        bool * const m_validated;
    };

    /**
     * @brief  Callback function that passes a visitor to a constraint.
     *
     * @param  constraint  Reference to constraint to be visited
     * @param  visitor     Reference to visitor to be applied
     *
     * @return  true if the visitor returns successfully, false otherwise.
     */
    static bool validationCallback(const constraints::Constraint &constraint, ValidationVisitor<AdapterType> &visitor)
    {
        return constraint.accept(visitor);
    }

    /// The JSON value being validated
    const AdapterType m_target;

    /// Vector of strings describing the current object context
    const std::vector<std::string> m_context;

    /// Optional pointer to a ValidationResults object to be populated
    ValidationResults *m_results;

    /// Option to use strict type comparison
    const bool m_strictTypes;

    /// Cached regex objects for pattern constraint
    std::unordered_map<std::string, std::regex>& m_regexesCache;
};

}  // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif

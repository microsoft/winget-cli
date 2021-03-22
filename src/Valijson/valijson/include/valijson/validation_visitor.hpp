#pragma once

#include <cmath>
#include <string>
#include <regex>

#include <valijson/adapters/std_string_adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/constraints/constraint_visitor.hpp>
#include <valijson/validation_results.hpp>

#include <valijson/utils/utf8_utils.hpp>

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
     */
    ValidationVisitor(const AdapterType &target,
                      const std::vector<std::string> &context,
                      const bool strictTypes,
                      ValidationResults *results)
      : target(target),
        context(context),
        results(results),
        strictTypes(strictTypes) { }

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
        Subschema::ApplyFunction fn(std::bind(validationCallback, std::placeholders::_1, *this));

        // Perform validation against each constraint defined in the schema
        if (results == nullptr) {
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
    virtual bool visit(const AllOfConstraint &constraint)
    {
        bool validated = true;
        constraint.applyToSubschemas(ValidateSubschemas(target, context,
                true, false, *this, results, nullptr, &validated));

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
    virtual bool visit(const AnyOfConstraint &constraint)
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(target, context, strictTypes, childResults);
        constraint.applyToSubschemas(ValidateSubschemas(target, context, false,
                true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0 && results) {
            ValidationResults::Error childError;
            while (childResults->popError(childError)) {
                results->pushError(
                        childError.context,
                        childError.description);
            }
            results->pushError(context, "Failed to validate against any child "
                    "schemas allowed by anyOf constraint.");
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
    virtual bool visit(const ConditionalConstraint &constraint)
    {
        // Create a validator to evaluate the conditional
        ValidationVisitor ifValidator(target, context, strictTypes, nullptr);
        ValidationVisitor thenElseValidator(target, context, strictTypes, nullptr);

        if (ifValidator.validateSchema(*constraint.getIfSubschema())) {
            const Subschema *thenSubschema = constraint.getThenSubschema();
            return thenSubschema == nullptr ||
                thenElseValidator.validateSchema(*thenSubschema);
        }

        const Subschema *elseSubschema = constraint.getElseSubschema();
        return elseSubschema == nullptr ||
            thenElseValidator.validateSchema(*elseSubschema);
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
    virtual bool visit(const ConstConstraint &constraint)
    {
        if (!constraint.getValue()->equalTo(target, strictTypes)) {
            if (results) {
                results->pushError(context,
                        "Failed to match expected value set by 'const' constraint.");
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
    virtual bool visit(const ContainsConstraint &constraint)
    {
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        const Subschema *subschema = constraint.getSubschema();
        const typename AdapterType::Array arr = target.asArray();

        bool validated = false;
        for (auto itr = arr.begin(); itr != arr.end(); ++itr) {
            ValidationVisitor containsValidator(*itr, context, strictTypes, nullptr);
            if (containsValidator.validateSchema(*subschema)) {
                validated = true;
                break;
            }
        }

        if (!validated) {
            if (results) {
                results->pushError(context,
                        "Failed to any values against subschema in 'contains' constraint.");
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
    virtual bool visit(const DependenciesConstraint &constraint)
    {
        // Ignore non-objects
        if ((strictTypes && !target.isObject()) || (!target.maybeObject())) {
            return true;
        }

        // Object to be validated
        const typename AdapterType::Object object = target.asObject();

        // Cleared if validation fails
        bool validated = true;

        // Iterate over all dependent properties defined by this constraint,
        // invoking the DependentPropertyValidator functor once for each
        // set of dependent properties
        constraint.applyToPropertyDependencies(ValidatePropertyDependencies(
                object, context, results, &validated));
        if (!results && !validated) {
            return false;
        }

        // Iterate over all dependent schemas defined by this constraint,
        // invoking the DependentSchemaValidator function once for each schema
        // that must be validated if a given property is present
        constraint.applyToSchemaDependencies(ValidateSchemaDependencies(
                object, context, *this, results, &validated));
        if (!results && !validated) {
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
    virtual bool visit(const EnumConstraint &constraint)
    {
        unsigned int numValidated = 0;
        constraint.applyToValues(ValidateEquality(target, context, false, true,
                strictTypes, nullptr, &numValidated));

        if (numValidated == 0) {
            if (results) {
                results->pushError(context,
                        "Failed to match against any enum values.");
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
    virtual bool visit(const LinearItemsConstraint &constraint)
    {
        // Ignore values that are not arrays
        if ((strictTypes && !target.isArray()) || (!target.maybeArray())) {
            return true;
        }

        // Sub-schema to validate against when number of items in array exceeds
        // the number of sub-schemas provided by the 'items' constraint
        const Subschema * const additionalItemsSubschema =
                constraint.getAdditionalItemsSubschema();

        // Track how many items are validated using 'items' constraint
        unsigned int numValidated = 0;

        // Array to validate
        const typename AdapterType::Array arr = target.asArray();
        const size_t arrSize = arr.size();

        // Track validation status
        bool validated = true;

        // Validate as many items as possible using 'items' sub-schemas
        const size_t itemSubschemaCount = constraint.getItemSubschemaCount();
        if (itemSubschemaCount > 0) {
            if (!additionalItemsSubschema) {
                if (arrSize > itemSubschemaCount) {
                    if (results) {
                        results->pushError(context,
                                "Array contains more items than allowed by "
                                "items constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
            }

            constraint.applyToItemSubschemas(ValidateItems(arr, context, true,
                    results != nullptr, strictTypes, results, &numValidated,
                    &validated));

            if (!results && !validated) {
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
                    std::vector<std::string> newContext = context;
                    newContext.push_back("[" +
                            std::to_string(index) + "]");

                    ValidationVisitor<AdapterType> validator(*itr, newContext,
                            strictTypes, results);

                    if (!validator.validateSchema(*additionalItemsSubschema)) {
                        if (results) {
                            results->pushError(context,
                                    "Failed to validate item #" +
                                    std::to_string(index) +
                                    " against additional items schema.");
                            validated = false;
                        } else {
                            return false;
                        }
                    }

                    index++;
                }

            } else if (results) {
                results->pushError(context, "Cannot validate item #" +
                    std::to_string(numValidated) + " or "
                    "greater using 'items' constraint or 'additionalItems' "
                    "constraint.");
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
    virtual bool visit(const MaximumConstraint &constraint)
    {
        if ((strictTypes && !target.isNumber()) || !target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double maximum = constraint.getMaximum();

        if (constraint.getExclusiveMaximum()) {
            if (target.asDouble() >= maximum) {
                if (results) {
                    results->pushError(context, "Expected number less than " +
                            std::to_string(maximum));
                }

                return false;
            }

        } else if (target.asDouble() > maximum) {
            if (results) {
                results->pushError(context,
                        "Expected number less than or equal to " +
                        std::to_string(maximum));
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
    virtual bool visit(const MaxItemsConstraint &constraint)
    {
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        const uint64_t maxItems = constraint.getMaxItems();
        if (target.asArray().size() <= maxItems) {
            return true;
        }

        if (results) {
            results->pushError(context, "Array should contain no more than " +
                    std::to_string(maxItems) + " elements.");
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
    virtual bool visit(const MaxLengthConstraint &constraint)
    {
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const std::string s = target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t maxLength = constraint.getMaxLength();
        if (len <= maxLength) {
            return true;
        }

        if (results) {
            results->pushError(context,
                    "String should be no more than " +
                    std::to_string(maxLength) +
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
    virtual bool visit(const MaxPropertiesConstraint &constraint)
    {
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        const uint64_t maxProperties = constraint.getMaxProperties();

        if (target.asObject().size() <= maxProperties) {
            return true;
        }

        if (results) {
            results->pushError(context, "Object should have no more than " +
                    std::to_string(maxProperties) +
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
    virtual bool visit(const MinimumConstraint &constraint)
    {
        if ((strictTypes && !target.isNumber()) || !target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        const double minimum = constraint.getMinimum();

        if (constraint.getExclusiveMinimum()) {
            if (target.asDouble() <= minimum) {
                if (results) {
                    results->pushError(context,
                        "Expected number greater than " +
                        std::to_string(minimum));
                }

                return false;
            }
        } else if (target.asDouble() < minimum) {
            if (results) {
                results->pushError(context,
                        "Expected number greater than or equal to " +
                        std::to_string(minimum));
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
    virtual bool visit(const MinItemsConstraint &constraint)
    {
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        const uint64_t minItems = constraint.getMinItems();
        if (target.asArray().size() >= minItems) {
            return true;
        }

        if (results) {
            results->pushError(context, "Array should contain no fewer than " +
                std::to_string(minItems) + " elements.");
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
    virtual bool visit(const MinLengthConstraint &constraint)
    {
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const std::string s = target.asString();
        const uint64_t len = utils::u8_strlen(s.c_str());
        const uint64_t minLength = constraint.getMinLength();
        if (len >= minLength) {
            return true;
        }

        if (results) {
            results->pushError(context,
                    "String should be no fewer than " +
                    std::to_string(minLength) +
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
    virtual bool visit(const MinPropertiesConstraint &constraint)
    {
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        const uint64_t minProperties = constraint.getMinProperties();

        if (target.asObject().size() >= minProperties) {
            return true;
        }

        if (results) {
            results->pushError(context, "Object should have no fewer than " +
                    std::to_string(minProperties) +
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
    virtual bool visit(const MultipleOfDoubleConstraint &constraint)
    {
        const double divisor = constraint.getDivisor();

        double d = 0.;
        if (target.maybeDouble()) {
            if (!target.asDouble(d)) {
                if (results) {
                    results->pushError(context, "Value could not be converted "
                        "to a number to check if it is a multiple of " +
                        std::to_string(divisor));
                }
                return false;
            }
        } else if (target.maybeInteger()) {
            int64_t i = 0;
            if (!target.asInteger(i)) {
                if (results) {
                    results->pushError(context, "Value could not be converted "
                        "to a number to check if it is a multiple of " +
                        std::to_string(divisor));
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
            if (results) {
                results->pushError(context, "Value should be a multiple of " +
                    std::to_string(divisor));
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
    virtual bool visit(const MultipleOfIntConstraint &constraint)
    {
        const int64_t divisor = constraint.getDivisor();

        int64_t i = 0;
        if (target.maybeInteger()) {
            if (!target.asInteger(i)) {
                if (results) {
                    results->pushError(context, "Value could not be converted "
                        "to an integer for multipleOf check");
                }
                return false;
            }
        } else if (target.maybeDouble()) {
            double d;
            if (!target.asDouble(d)) {
                if (results) {
                    results->pushError(context, "Value could not be converted "
                        "to a double for multipleOf check");
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
            if (results) {
                results->pushError(context, "Value should be a multiple of " +
                    std::to_string(divisor));
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
    virtual bool visit(const NotConstraint &constraint)
    {
        const Subschema *subschema = constraint.getSubschema();
        if (!subschema) {
            // Treat nullptr like empty schema
            return false;
        }

        ValidationVisitor<AdapterType> v(target, context, strictTypes, nullptr);
        if (v.validateSchema(*subschema)) {
            if (results) {
                results->pushError(context,
                        "Target should not validate against schema "
                        "specified in 'not' constraint.");
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
    virtual bool visit(const OneOfConstraint &constraint)
    {
        unsigned int numValidated = 0;

        ValidationResults newResults;
        ValidationResults *childResults = (results) ? &newResults : nullptr;

        ValidationVisitor<AdapterType> v(target, context, strictTypes, childResults);
        constraint.applyToSubschemas(ValidateSubschemas(target, context,
                true, true, v, childResults, &numValidated, nullptr));

        if (numValidated == 0) {
            if (results) {
                ValidationResults::Error childError;
                while (childResults->popError(childError)) {
                    results->pushError(
                            childError.context,
                            childError.description);
                }
                results->pushError(context, "Failed to validate against any "
                        "child schemas allowed by oneOf constraint.");
            }
            return false;
        } else if (numValidated != 1) {
            if (results) {
                results->pushError(context,
                        "Failed to validate against exactly one child schema.");
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
    virtual bool visit(const PatternConstraint &constraint)
    {
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const std::regex patternRegex(
                constraint.getPattern<std::string::allocator_type>());

        if (!std::regex_search(target.asString(), patternRegex)) {
            if (results) {
                results->pushError(context,
                        "Failed to match regex specified by 'pattern' "
                        "constraint.");
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
    virtual bool visit(const constraints::PolyConstraint &constraint)
    {
        return constraint.validate(target, context, results);
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
    virtual bool visit(const PropertiesConstraint &constraint)
    {
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        bool validated = true;

        // Track which properties have already been validated
        std::set<std::string> propertiesMatched;

        // Validate properties against subschemas for matching 'properties'
        // constraints
        const typename AdapterType::Object object = target.asObject();
        constraint.applyToProperties(ValidatePropertySubschemas(object, context,
                true, results != nullptr, true, strictTypes, results,
                &propertiesMatched, &validated));

        // Exit early if validation failed, and we're not collecting exhaustive
        // validation results
        if (!validated && !results) {
            return false;
        }

        // Validate properties against subschemas for matching patternProperties
        // constraints
        constraint.applyToPatternProperties(ValidatePatternPropertySubschemas(
                object, context, true, false, true, strictTypes, results,
                &propertiesMatched, &validated));

        // Validate against additionalProperties subschema for any properties
        // that have not yet been matched
        const Subschema *additionalPropertiesSubschema =
                constraint.getAdditionalPropertiesSubschema();
        if (!additionalPropertiesSubschema) {
            if (propertiesMatched.size() != target.getObjectSize()) {
                if (results) {
                    std::string unwanted;
                    for (const typename AdapterType::ObjectMember m : object) {
                        if (propertiesMatched.find(m.first) == propertiesMatched.end()) {
                            unwanted = m.first;
                            break;
                        }
                    }
                    results->pushError(context, "Object contains a property "
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
                std::vector<std::string> newContext = context;
                newContext.push_back("[" + m.first + "]");

                // Create a validator to validate the property's value
                ValidationVisitor validator(m.second, newContext, strictTypes,
                        results);
                if (!validator.validateSchema(*additionalPropertiesSubschema)) {
                    if (results) {
                        results->pushError(context, "Failed to validate "
                                "against additional properties schema");
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
    virtual bool visit(const PropertyNamesConstraint &constraint)
    {
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        for (const typename AdapterType::ObjectMember m : target.asObject()) {
            adapters::StdStringAdapter stringAdapter(m.first);
            ValidationVisitor<adapters::StdStringAdapter> validator(stringAdapter, context, strictTypes, nullptr);
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
    virtual bool visit(const RequiredConstraint &constraint)
    {
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        bool validated = true;
        const typename AdapterType::Object object = target.asObject();
        constraint.applyToRequiredProperties(ValidateProperties(object, context,
                true, results != nullptr, results, &validated));

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
    virtual bool visit(const SingularItemsConstraint &constraint)
    {
        // Ignore values that are not arrays
        if (!target.isArray()) {
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
        for (const AdapterType &item : target.getArray()) {
            // Update context for current array item
            std::vector<std::string> newContext = context;
            newContext.push_back("[" +
                    std::to_string(index) + "]");

            // Create a validator for the current array item
            ValidationVisitor<AdapterType> validationVisitor(item,
                    newContext, strictTypes, results);

            // Perform validation
            if (!validationVisitor.validateSchema(*itemsSubschema)) {
                if (results) {
                    results->pushError(context,
                            "Failed to validate item #" +
                            std::to_string(index) +
                            " in array.");
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
    virtual bool visit(const TypeConstraint &constraint)
    {
        // Check named types
        {
            // ValidateNamedTypes functor assumes target is invalid
            bool validated = false;
            constraint.applyToNamedTypes(ValidateNamedTypes(target, false,
                    true, strictTypes, &validated));
            if (validated) {
                return true;
            }
        }

        // Check schema-based types
        {
            unsigned int numValidated = 0;
            constraint.applyToSchemaTypes(ValidateSubschemas(target, context,
                    false, true, *this, nullptr, &numValidated, nullptr));
            if (numValidated > 0) {
                return true;
            } else if (results) {
                results->pushError(context,
                        "Value type not permitted by 'type' constraint.");
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
    virtual bool visit(const UniqueItemsConstraint &)
    {
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        // Empty arrays are always valid
        if (target.getArraySize() == 0) {
            return true;
        }

        const typename AdapterType::Array targetArray = target.asArray();
        const typename AdapterType::Array::const_iterator end = targetArray.end();

        bool validated = true;
        const typename AdapterType::Array::const_iterator secondLast = --targetArray.end();
        unsigned int outerIndex = 0;
        typename AdapterType::Array::const_iterator outerItr = targetArray.begin();
        for (; outerItr != secondLast; ++outerItr) {
            unsigned int innerIndex = outerIndex + 1;
            typename AdapterType::Array::const_iterator innerItr(outerItr);
            for (++innerItr; innerItr != end; ++innerItr) {
                if (outerItr->equalTo(*innerItr, true)) {
                    if (results) {
                        results->pushError(context, "Elements at indexes #" +
                            std::to_string(outerIndex) + " and #" +
                            std::to_string(innerIndex) + " violate uniqueness constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
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
          : target(target),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            strictTypes(strictTypes),
            results(results),
            numValidated(numValidated) { }

        template<typename OtherValue>
        bool operator()(const OtherValue &value) const
        {
            if (value.equalTo(target, strictTypes)) {
                if (numValidated) {
                    (*numValidated)++;
                }

                return continueOnSuccess;
            }

            if (results) {
                results->pushError(context,
                        "Target value and comparison value are not equal");
            }

            return continueOnFailure;
        }

    private:
        const AdapterType &target;
        const std::vector<std::string> &context;
        bool continueOnSuccess;
        bool continueOnFailure;
        bool strictTypes;
        ValidationResults * const results;
        unsigned int * const numValidated;
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
          : object(object),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            results(results),
            validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &property) const
        {
            if (object.find(property.c_str()) == object.end()) {
                if (validated) {
                    *validated = false;
                }

                if (results) {
                    results->pushError(context, "Missing required property '" +
                            std::string(property.c_str()) + "'.");
                }

                return continueOnFailure;
            }

            return continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        bool continueOnSuccess;
        bool continueOnFailure;
        ValidationResults * const results;
        bool * const validated;
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
          : object(object),
            context(context),
            results(results),
            validated(validated) { }

        template<typename StringType, typename ContainerType>
        bool operator()(
                const StringType &propertyName,
                const ContainerType &dependencyNames) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (object.find(propertyNameKey) == object.end()) {
                return true;
            }

            typedef typename ContainerType::value_type ValueType;
            for (const ValueType &dependencyName : dependencyNames) {
                const std::string dependencyNameKey(dependencyName.c_str());
                if (object.find(dependencyNameKey) == object.end()) {
                    if (validated) {
                        *validated = false;
                    }
                    if (results) {
                        results->pushError(context, "Missing dependency '" +
                                dependencyNameKey + "'.");
                    } else {
                        return false;
                    }
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        ValidationResults * const results;
        bool * const validated;
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
                bool *validated)
          : arr(arr),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            strictTypes(strictTypes),
            results(results),
            numValidated(numValidated),
            validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            // Check that there are more elements to validate
            if (index >= arr.size()) {
                return false;
            }

            // Update context
            std::vector<std::string> newContext = context;
            newContext.push_back(
                    "[" + std::to_string(index) + "]");

            // Find array item
            typename AdapterType::Array::const_iterator itr = arr.begin();
            itr.advance(index);

            // Validate current array item
            ValidationVisitor validator(*itr, newContext, strictTypes, results);
            if (validator.validateSchema(*subschema)) {
                if (numValidated) {
                    (*numValidated)++;
                }

                return continueOnSuccess;
            }

            if (validated) {
                *validated = false;
            }

            if (results) {
                results->pushError(newContext,
                    "Failed to validate item #" +
                    std::to_string(index) +
                    " against corresponding item schema.");
            }

            return continueOnFailure;
        }

    private:
        const typename AdapterType::Array &arr;
        const std::vector<std::string> &context;
        bool continueOnSuccess;
        bool continueOnFailure;
        bool strictTypes;
        ValidationResults * const results;
        unsigned int * const numValidated;
        bool * const validated;

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
          : target(target),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            strictTypes(strictTypes),
            validated(validated) { }

        bool operator()(constraints::TypeConstraint::JsonType jsonType) const
        {
            typedef constraints::TypeConstraint TypeConstraint;

            bool valid = false;

            switch (jsonType) {
            case TypeConstraint::kAny:
                valid = true;
                break;
            case TypeConstraint::kArray:
                valid = target.isArray();
                break;
            case TypeConstraint::kBoolean:
                valid = target.isBool() || (!strictTypes && target.maybeBool());
                break;
            case TypeConstraint::kInteger:
                valid = target.isInteger() ||
                        (!strictTypes && target.maybeInteger());
                break;
            case TypeConstraint::kNull:
                valid = target.isNull() ||
                        (!strictTypes && target.maybeNull());
                break;
            case TypeConstraint::kNumber:
                valid = target.isNumber() ||
                        (!strictTypes && target.maybeDouble());
                break;
            case TypeConstraint::kObject:
                valid = target.isObject();
                break;
            case TypeConstraint::kString:
                valid = target.isString();
                break;
            default:
                break;
            }

            if (valid && validated) {
                *validated = true;
            }

            return (valid && continueOnSuccess) || continueOnFailure;
        }

    private:
        const AdapterType target;
        const bool continueOnSuccess;
        const bool continueOnFailure;
        const bool strictTypes;
        bool * const validated;
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
                bool *validated)
          : object(object),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            continueIfUnmatched(continueIfUnmatched),
            strictTypes(strictTypes),
            results(results),
            propertiesMatched(propertiesMatched),
            validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &patternProperty,
                const Subschema *subschema) const
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
            for (const ObjectMember m : object) {
                if (std::regex_search(m.first, r)) {
                    matchFound = true;
                    if (propertiesMatched) {
                        propertiesMatched->insert(m.first);
                    }

                    // Update context
                    std::vector<std::string> newContext = context;
                    newContext.push_back("[" + m.first + "]");

                    // Recursively validate property's value
                    ValidationVisitor validator(m.second, newContext,
                            strictTypes, results);
                    if (validator.validateSchema(*subschema)) {
                        continue;
                    }

                    if (results) {
                        results->pushError(context, "Failed to validate "
                                "against schema associated with pattern '" +
                                patternPropertyStr + "'.");
                    }

                    if (validated) {
                        *validated = false;
                    }

                    if (!continueOnFailure) {
                        return false;
                    }
                }
            }

            // Allow iteration to terminate if there was not at least one match
            if (!matchFound && !continueIfUnmatched) {
                return false;
            }

            return continueOnSuccess;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        const bool continueOnSuccess;
        const bool continueOnFailure;
        const bool continueIfUnmatched;
        const bool strictTypes;
        ValidationResults * const results;
        std::set<std::string> * const propertiesMatched;
        bool * const validated;
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
                bool *validated)
          : object(object),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            continueIfUnmatched(continueIfUnmatched),
            strictTypes(strictTypes),
            results(results),
            propertiesMatched(propertiesMatched),
            validated(validated) { }

        template<typename StringType>
        bool operator()(const StringType &propertyName,
                const Subschema *subschema) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            const typename AdapterType::Object::const_iterator itr =
                    object.find(propertyNameKey);
            if (itr == object.end()) {
                return continueIfUnmatched;
            }

            if (propertiesMatched) {
                propertiesMatched->insert(propertyNameKey);
            }

            // Update context
            std::vector<std::string> newContext = context;
            newContext.push_back("[" + propertyNameKey + "]");

            // Recursively validate property's value
            ValidationVisitor validator(itr->second, newContext, strictTypes,
                    results);
            if (validator.validateSchema(*subschema)) {
                return continueOnSuccess;
            }

            if (results) {
                results->pushError(context, "Failed to validate against "
                        "schema associated with property name '" +
                        propertyNameKey + "'.");
            }

            if (validated) {
                *validated = false;
            }

            return continueOnFailure;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        const bool continueOnSuccess;
        const bool continueOnFailure;
        const bool continueIfUnmatched;
        const bool strictTypes;
        ValidationResults * const results;
        std::set<std::string> * const propertiesMatched;
        bool * const validated;
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
          : object(object),
            context(context),
            validationVisitor(validationVisitor),
            results(results),
            validated(validated) { }

        template<typename StringType>
        bool operator()(
                const StringType &propertyName,
                const Subschema *schemaDependency) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (object.find(propertyNameKey) == object.end()) {
                return true;
            }

            if (!validationVisitor.validateSchema(*schemaDependency)) {
                if (validated) {
                    *validated = false;
                }
                if (results) {
                    results->pushError(context,
                            "Failed to validate against dependent schema.");
                } else {
                    return false;
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        ValidationVisitor &validationVisitor;
        ValidationResults * const results;
        bool * const validated;
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
          : adapter(adapter),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            validationVisitor(validationVisitor),
            results(results),
            numValidated(numValidated),
            validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            if (validationVisitor.validateSchema(*subschema)) {
                if (numValidated) {
                    (*numValidated)++;
                }

                return continueOnSuccess;
            }

            if (validated) {
                *validated = false;
            }

            if (results) {
                results->pushError(context,
                        "Failed to validate against child schema #" +
                                   std::to_string(index) + ".");
            }

            return continueOnFailure;
        }

    private:
        const AdapterType &adapter;
        const std::vector<std::string> &context;
        bool continueOnSuccess;
        bool continueOnFailure;
        ValidationVisitor &validationVisitor;
        ValidationResults * const results;
        unsigned int * const numValidated;
        bool * const validated;
    };

    /**
     * @brief  Callback function that passes a visitor to a constraint.
     *
     * @param  constraint  Reference to constraint to be visited
     * @param  visitor     Reference to visitor to be applied
     *
     * @return  true if the visitor returns successfully, false otherwise.
     */
    static bool validationCallback(const constraints::Constraint &constraint,
                                   ValidationVisitor<AdapterType> &visitor)
    {
        return constraint.accept(visitor);
    }

    /// The JSON value being validated
    const AdapterType target;

    /// Vector of strings describing the current object context
    const std::vector<std::string> context;

    /// Optional pointer to a ValidationResults object to be populated
    ValidationResults *results;

    /// Option to use strict type comparison
    const bool strictTypes;
};

}  // namespace valijson

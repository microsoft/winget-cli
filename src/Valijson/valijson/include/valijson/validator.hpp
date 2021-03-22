#pragma once

#include <valijson/schema.hpp>
#include <valijson/validation_visitor.hpp>

namespace valijson {

class Schema;
class ValidationResults;

/**
 * @brief  Class that provides validation functionality.
 */
class Validator
{
public:
    enum TypeCheckingMode
    {
        kStrongTypes,
        kWeakTypes
    };

    /**
     * @brief  Construct a Validator that uses strong type checking by default
     */
    Validator()
      : strictTypes(true) { }

    /**
     * @brief  Construct a Validator using a specific type checking mode
     *
     * @param  typeCheckingMode  choice of strong or weak type checking
     */
    Validator(TypeCheckingMode typeCheckingMode)
      : strictTypes(typeCheckingMode == kStrongTypes) { }

    /**
     * @brief  Validate a JSON document and optionally return the results.
     *
     * When a ValidationResults object is provided via the \c results parameter,
     * validation will be performed against each constraint defined by the
     * schema, even if validation fails for some or all constraints.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param  schema   The schema to validate against
     * @param  target   A rapidjson::Value to be validated
     *
     * @param  results  An optional pointer to a ValidationResults instance that
     *                  will be used to report validation errors
     *
     * @returns  true if validation succeeds, false otherwise
     */
    template<typename AdapterType>
    bool validate(const Subschema &schema, const AdapterType &target,
            ValidationResults *results)
    {
        // Construct a ValidationVisitor to perform validation at the root level
        ValidationVisitor<AdapterType> v(target,
                std::vector<std::string>(1, "<root>"), strictTypes, results);

        return v.validateSchema(schema);
    }

private:

    /// Flag indicating that strict type comparisons should be used
    const bool strictTypes;

};

}  // namespace valijson

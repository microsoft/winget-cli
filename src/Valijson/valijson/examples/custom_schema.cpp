/**
 * @file
 *
 * @brief Demonstrates validation against a manually constructed schema.
 *
 * This example demonstrates the construction and composition of a Schema object
 * using manually created Constraint objects. The following Constraint classes
 * are used:
 *  - EnumConstraint
 *  - MaxLengthConstraint
 *  - MinimumConstraint
 *  - MinLengthConstraint
 *  - PropertiesConstraint
 *  - RequiredConstraint
 *  - TypeConstraint
 *
 * The MinimumConstraint class provides support for the exclusiveMinimum and
 * minimum keywords in JSON Schema. And the PropertiesConstraint class provides
 * support for the properties, patternProperties, and additionalProperties
 * keywords.
 *
 * This is the JSON Schema representation of the Schema that will be created:
 *
 *  {
 *    "properties": {
 *      "category": {
 *        "enum": [
 *          "album",
 *          "book",
 *          "other",
 *          "video"
 *        ]
 *      },
 *      "description": {
 *        "type": "string"
 *      },
 *      "price": {
 *        "exclusiveMinimum": true,
 *        "minimum": 0.0,
 *        "type": "number"
 *      },
 *      "title": {
 *        "maxLength": 200,
 *        "minLength": 1,
 *        "type": "string"
 *      }
 *    },
 *    "required": [
 *      "category",
 *      "price",
 *      "title"
 *    ],
 *    "type": "object"
 *  }
 *
 */

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <rapidjson/document.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

using std::cerr;
using std::endl;

using valijson::Schema;
using valijson::Subschema;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::RapidJsonAdapter;
using valijson::adapters::RapidJsonFrozenValue;
using valijson::constraints::EnumConstraint;
using valijson::constraints::MaxLengthConstraint;
using valijson::constraints::MinimumConstraint;
using valijson::constraints::MinLengthConstraint;
using valijson::constraints::PropertiesConstraint;
using valijson::constraints::RequiredConstraint;
using valijson::constraints::TypeConstraint;

void addPropertiesConstraint(Schema &schema)
{

    PropertiesConstraint propertiesConstraint;

    {
        // Prepare an enum constraint requires a document to be equal to at
        // least one of a set of possible values
        EnumConstraint constraint;
        constraint.addValue(RapidJsonFrozenValue("album"));
        constraint.addValue(RapidJsonFrozenValue("book"));
        constraint.addValue(RapidJsonFrozenValue("other"));
        constraint.addValue(RapidJsonFrozenValue("video"));

        // Create a subschema, owned by the root schema, with a constraint
        const Subschema *subschema = schema.createSubschema();
        schema.addConstraintToSubschema(constraint, subschema);

        // Include subschema in properties constraint
        propertiesConstraint.addPropertySubschema("category", subschema);
    }

    {
        // Create a child schema for the 'description' property that requires
        // a string, but does not enforce any length constraints.
        const Subschema *subschema = schema.createSubschema();
        TypeConstraint typeConstraint;
        typeConstraint.addNamedType(TypeConstraint::kString);
        schema.addConstraintToSubschema(typeConstraint, subschema);

        // Include subschema in properties constraint
        propertiesConstraint.addPropertySubschema("description", subschema);
    }

    {
        // Create a child schema for the 'price' property, that requires a
        // number with a value greater than zero.
        const Subschema *subschema = schema.createSubschema();
        MinimumConstraint minimumConstraint;
        minimumConstraint.setMinimum(0.0);
        minimumConstraint.setExclusiveMinimum(true);
        schema.addConstraintToSubschema(minimumConstraint, subschema);
        TypeConstraint typeConstraint;
        typeConstraint.addNamedType(TypeConstraint::kNumber);
        schema.addConstraintToSubschema(typeConstraint, subschema);

        // Include subschema in properties constraint
        propertiesConstraint.addPropertySubschema("price", subschema);
    }

    {
        // Create a child schema for the 'title' property that requires a string
        // that is between 1 and 200 characters in length.
        const Subschema *subschema = schema.createSubschema();
        MaxLengthConstraint maxLengthConstraint;
        maxLengthConstraint.setMaxLength(200);
        schema.addConstraintToSubschema(maxLengthConstraint, subschema);
        MinLengthConstraint minLengthConstraint;
        minLengthConstraint.setMinLength(0);
        schema.addConstraintToSubschema(minLengthConstraint, subschema);
        TypeConstraint typeConstraint;
        typeConstraint.addNamedType(TypeConstraint::kString);
        schema.addConstraintToSubschema(typeConstraint, subschema);

        // Include subschema in properties constraint
        propertiesConstraint.addPropertySubschema("title", subschema);
    }

    // Add a PropertiesConstraint to the root schema
    schema.addConstraint(propertiesConstraint);
}

void addRequiredConstraint(Schema &schema)
{
    // Add a RequiredConstraint to the schema, specifying that the category,
    // price, and title properties must be present.
    RequiredConstraint constraint;
    constraint.addRequiredProperty("category");
    constraint.addRequiredProperty("price");
    constraint.addRequiredProperty("title");
    schema.addConstraint(constraint);
}

void addTypeConstraint(Schema &schema)
{
    // Add a TypeConstraint to the schema, specifying that the root of the
    // document must be an object.
    TypeConstraint typeConstraint;
    typeConstraint.addNamedType(TypeConstraint::kObject);
    schema.addConstraint(typeConstraint);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cerr << "Usage:" << endl;
        cerr << "  ./custom_schema <document>" << endl;
        cerr << endl;
        return 1;
    }

    // Load the document that is to be validated
    rapidjson::Document targetDocument;
    if (!valijson::utils::loadDocument(argv[1], targetDocument)) {
        cerr << "Failed to load target document." << endl;
        return 1;
    }

    // Populate a schema
    Schema schema;
    addPropertiesConstraint(schema);
    addRequiredConstraint(schema);
    addTypeConstraint(schema);

    // Perform validation
    Validator validator;
    ValidationResults results;
    RapidJsonAdapter targetDocumentAdapter(targetDocument);
    if (!validator.validate(schema, targetDocumentAdapter, &results)) {
        std::cerr << "Validation failed." << endl;
        ValidationResults::Error error;
        unsigned int errorNum = 1;
        while (results.popError(error)) {
            cerr << "Error #" << errorNum << std::endl;
            cerr << "  ";
            for (const std::string &contextElement : error.context) {
                cerr << contextElement << " ";
            }
            cerr << endl;
            cerr << "    - " << error.description << endl;
            ++errorNum;
        }
        return 1;
    }

    return 0;
}

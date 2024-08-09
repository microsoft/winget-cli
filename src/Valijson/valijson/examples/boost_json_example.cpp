#include <boost/json/src.hpp>
#include <valijson/adapters/boost_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <valijson/validation_results.hpp>
#include <iostream>

using namespace std::string_literals;

namespace json = boost::json;

const auto schemaJson = R"({
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Product",
  "description": "A product from Acme's catalog",
  "type": "object",
  "properties": {
    "id": {
      "description": "The unique identifier for a product",
      "type": "integer"
    },
    "name": {
      "description": "Name of the product",
      "type": "string"
    },
    "price": {
      "type": "number",
      "minimum": 0
    },
    "tags": {
      "type": "array",
      "items": { "type": "string" },
      "minItems": 1,
      "uniqueItems": true
    }
  },
  "required": ["id", "name", "price" ]
})"s;

const auto targetJson = R"({
  "id": 123,
  "name": "Test"
})"s;

int main()
{
    json::error_code ec;
    auto schemaDoc = json::parse(schemaJson, ec);
    if (ec) {
        std::cerr << "Error parsing schema json: " << ec.message() << std::endl;
        return 1;
    }

    auto obj = schemaDoc.as_object();
    auto iter = obj.find("$schema");
    if (iter == obj.cend()) {
        std::cerr << "Error finding key $schema" << std::endl;
        return 2;
    }

    iter = obj.find("$ref");
    if (iter != obj.cend()) {
        std::cerr << "Invalid iterator for non-existent key $ref" << std::endl;
        return 3;
    }

    valijson::Schema schema;
    valijson::SchemaParser schemaParser;

    // Won't compile because the necessary constructor has been deleted
    // valijson::adapters::BoostJsonAdapter schemaAdapter(obj);

    valijson::adapters::BoostJsonAdapter schemaAdapter(schemaDoc);
    std::cerr << "Populating schema..." << std::endl;
    schemaParser.populateSchema(schemaAdapter, schema);

    auto targetDoc = json::parse(targetJson, ec);
    if (ec) {
        std::cerr << "Error parsing target json: " << ec.message() << std::endl;
        return 1;
    }

    valijson::Validator validator;
    valijson::ValidationResults results;
    valijson::adapters::BoostJsonAdapter targetAdapter(targetDoc);
    if (validator.validate(schema, targetAdapter, &results)) {
        std::cerr << "Validation succeeded." << std::endl;
        return 0;
    }

    std::cerr << "Validation failed." << std::endl;
    valijson::ValidationResults::Error error;
    unsigned int errorNum = 1;
    while (results.popError(error)) {
        std::cerr << "Error #" << errorNum << std::endl;
        std::cerr << "  ";
        for (const std::string &contextElement : error.context) {
            std::cerr << contextElement << " ";
        }
        std::cerr << std::endl;
        std::cerr << "    - " << error.description << std::endl;
        ++errorNum;
    }

    return 1;
}
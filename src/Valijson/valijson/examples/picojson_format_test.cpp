#include <iostream>
#include <string>

#define PICOJSON_USE_INT64
#include "picojson.h"

#include "valijson/adapters/picojson_adapter.hpp"
#include "valijson/validation_results.hpp"
#include "valijson/schema_parser.hpp"
#include "valijson/validator.hpp"

constexpr auto schemaStr = R"JSON({
  "additionalItems": false,
  "items": [
    {
      "format": "date-time",
      "type": "string"
    },
    {
      "format": "date-time",
      "type": "string"
    }
  ],
  "maxItems": 2,
  "minItems": 2,
  "type": "array"
})JSON";

constexpr auto validStr = R"JSON([
    "2023-07-18T14:46:22Z",
    "2023-07-18T14:46:22Z"
])JSON";

constexpr auto invalidStrs = R"JSON([
    ["um 12", "um 12"],
    ["2023-07-18T14:46:22Z"],
    ["2023-07-18T14:46:22Z", "2023-07-18T14:46:22Z", "2023-07-18T14:46:22Z", "2023-07-18T14:46:22Z"]
])JSON";

picojson::value Parse(std::string serialized, picojson::value def)
{
    picojson::value v;
    auto first = serialized.data();
    auto last = first + serialized.size();
    auto err = picojson::parse(v, first, last);

    if (!err.empty()) {
        return def;
    }

    return v;
}

int main(int argc, char **argv)
{
    auto validatorSchema = std::make_shared<valijson::Schema>();
    {
        auto schemaJson = Parse(schemaStr, picojson::value{});
        auto schemaAdapter = valijson::adapters::PicoJsonAdapter(schemaJson);
        valijson::SchemaParser parser;
        parser.populateSchema(schemaAdapter, *validatorSchema);
        std::cout << "Schema:" << std::endl << schemaStr << std::endl << std::endl;;
    }

    {
        // valid
        auto targetJson = Parse(validStr, picojson::value{});
        auto targetAdapter = valijson::adapters::PicoJsonAdapter(targetJson);
        std::cout << "Valid Target:" << std::endl << validStr << std::endl << std::endl;

        valijson::ValidationResults results;
        auto validator = valijson::Validator();
        auto isValid = validator.validate(
            *validatorSchema,
            targetAdapter,
            &results);

        std::cout << "Is valid: " << (isValid ? "YES" : "NO") << std::endl << std::endl;;
    }

    {
        // invalid
        auto targetJsonArray = Parse(invalidStrs, picojson::value{});
        std::cout << "Invalid Targets:" << std::endl << invalidStrs << std::endl << std::endl;

        for (auto &&testCase : targetJsonArray.get<picojson::array>()) {
            auto targetAdapter = valijson::adapters::PicoJsonAdapter(testCase);

            valijson::ValidationResults results;
            auto validator = valijson::Validator();
            auto isValid = validator.validate(
                *validatorSchema,
                targetAdapter,
                &results);

            std::cout << "Is valid: " << (isValid ? "YES" : "NO") << std::endl << std::endl;

            valijson::ValidationResults::Error error;
            unsigned int errorNum = 1;
            while (results.popError(error)) {
                std::cerr << "Error #" << errorNum << std::endl;
                std::cerr << "  ";
                for (const std::string &contextElement : error.context) {
                    std::cerr << contextElement << " ";
                }
                std::cerr << std::endl;
                std::cerr << "    - " << error.description << std::endl << std::endl;
                ++errorNum;
            }
        }
    }
}

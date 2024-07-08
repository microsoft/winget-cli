#include <nlohmann/json.hpp>

#include "valijson_nlohmann_bundled.hpp"

using namespace std;
using namespace valijson;
using namespace valijson::adapters;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <schema document> <test/target document>" << endl;
        return 1;
    }

    // Load the document containing the schema
    nlohmann::json schemaDocument;
    if (!valijson::utils::loadDocument(argv[1], schemaDocument)) {
        cerr << "Failed to load schema document." << endl;
        return 1;
    }

    // Load the document that is to be validated
    nlohmann::json targetDocument;
    if (!valijson::utils::loadDocument(argv[2], targetDocument)) {
        cerr << "Failed to load target document." << endl;
        return 1;
    }

    // Parse the json schema into an internal schema format
    Schema schema;
    SchemaParser parser;
    NlohmannJsonAdapter schemaDocumentAdapter(schemaDocument);
    try {
        parser.populateSchema(schemaDocumentAdapter, schema);
    } catch (std::exception &e) {
        cerr << "Failed to parse schema: " << e.what() << endl;
        return 1;
    }

    // Perform validation
    Validator validator(Validator::kStrongTypes);
    ValidationResults results;
    NlohmannJsonAdapter targetDocumentAdapter(targetDocument);
    if (!validator.validate(schema, targetDocumentAdapter, &results)) {
        std::cerr << "Validation failed." << endl;
        ValidationResults::Error error;
        unsigned int errorNum = 1;
        while (results.popError(error)) {

            std::string context;
            std::vector<std::string>::iterator itr = error.context.begin();
            for (; itr != error.context.end(); itr++) {
                context += *itr;
            }

            cerr << "Error #" << errorNum << std::endl
                 << "  context: " << context << endl
                 << "  desc:    " << error.description << endl;
            ++errorNum;
        }
        return 1;
    }

    return 0;
}

/**
 * @file
 *
 * @brief Demonstrates resolution of remote JSON references using cURLpp
 *
 */

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include <rapidjson/document.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

using std::cerr;
using std::cout;
using std::endl;

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::RapidJsonAdapter;

const rapidjson::Document * fetchDocument(const std::string &uri)
{
    cout << "Fetching " << uri << "..." << endl;
    curlpp::Cleanup myCleanup;
    std::ostringstream os;
    os << curlpp::options::Url(uri);
    rapidjson::Document *fetchedRoot = new rapidjson::Document();
    fetchedRoot->template Parse<0>(os.str().c_str());
    return fetchedRoot;
}

void freeDocument(const rapidjson::Document *adapter)
{
    delete adapter;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <schema document> <test/target document>" << endl;
        return 1;
    }

    // Load the document containing the schema
    rapidjson::Document schemaDocument;
    if (!valijson::utils::loadDocument(argv[1], schemaDocument)) {
        cerr << "Failed to load schema document." << endl;
        return 1;
    }

    // Load the document that is to be validated
    rapidjson::Document targetDocument;
    if (!valijson::utils::loadDocument(argv[2], targetDocument)) {
        cerr << "Failed to load target document." << endl;
        return 1;
    }

    // Parse the json schema into an internal schema format
    Schema schema;
    SchemaParser parser;
    RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    try {
        parser.populateSchema(schemaDocumentAdapter, schema, fetchDocument, freeDocument);
    } catch (std::exception &e) {
        cerr << "Failed to parse schema: " << e.what() << endl;
        return 1;
    }

    // Perform validation
    Validator validator(Validator::kWeakTypes);
    ValidationResults results;
    RapidJsonAdapter targetDocumentAdapter(targetDocument);
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

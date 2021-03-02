/**
 * @file
 *
 * @brief Demonstrates iteration over an array and type check functions
 *
 */

#include <exception>
#include <iostream>

// jsoncpp
#include <json/json.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

// RapidJSON
#include <rapidjson/document.h>
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::runtime_error;

// The first example uses RapidJson to load a JSON document. If the document
// contains an array, this function will print any array values that have a
// valid string representation.
void usingRapidJson(const char *filename);

// The second example uses JsonCpp to perform the same task, but unlike the
// RapidJson example, we see how to use strict type checks and exception
// handling.
void usingJsonCpp(const char *filename);

int main(int argc, char **argv)
{
    if (argc != 2) {
        cerr << "Usage: " << endl;
        cerr << "  " << argv[0] << " <filename>" << endl;
        return 1;
    }

    // Load the document using rapidjson
    cout << "-- Iteration using RapidJSON --" << endl;
    usingRapidJson(argv[1]);
    cout << endl;

    // Load the document using jsoncpp
    cout << "-- Iteration using jsoncpp --" << endl;
    usingJsonCpp(argv[1]);
    cout << endl;

    return 0;
}

void usingRapidJson(const char *filename)
{
    using valijson::adapters::RapidJsonAdapter;

    rapidjson::Document document;
    if (!valijson::utils::loadDocument(filename, document)) {
        return;
    }

    RapidJsonAdapter adapter(document);
    if (!adapter.isArray()) {
        cout << "Not an array." << endl;
        return;
    }

    cout << "Array values:" << endl;
    int index = 0;

    // We support the old way of doing things...
    const RapidJsonAdapter::Array array = adapter.asArray();
    for (RapidJsonAdapter::Array::const_iterator itr = array.begin(); 
                                                 itr != array.end(); ++itr) {
        cout << "  " << index++ << ": ";

        // Each element of the array is just another RapidJsonAdapter
        const RapidJsonAdapter &value = *itr;

        // maybeString is a loose type check
        if (value.maybeString()) {
            // If a value may be a string, we are allowed to get a string 
            // representation of the value using asString
            cout << value.asString();
        }
        
        cout << endl;
    }
}

void usingJsonCpp(const char *filename)
{
    Json::Value value;
    if (!valijson::utils::loadDocument(filename, value)) {
        return;
    }

    valijson::adapters::JsonCppAdapter adapter(value);

    // isArray is a strict type check
    if (!adapter.isArray()) {
        cout << "Not an array." << endl;
        return;
    }

    cout << "Array values:" << endl;
    int index = 0;

    // If a value is not an array, then calling getArray will cause a runtime
    // exception to be raised.
    for (auto value : adapter.getArray()) {
        cout << "  " << index++ << ": ";

        // isString is another strict type check. Valijson uses the convention
        // that strict type check functions are prefixed with 'is'.
        if (!value.isString()) {
            cout << "Not a string. ";
        }

        try {
            // Also by convention, functions prefixed with 'get' will raise a
            // runtime exception if the value is not of the correct type.
            cout << value.getString() << endl;
        } catch (runtime_error &e) {
            cout << "Caught exception: " << e.what() << endl;
        }
    }
}

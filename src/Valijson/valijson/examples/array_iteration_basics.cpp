/**
 * @file
 *
 * @brief Demonstrates iteration over an array, and how to use type check functions
 */

#include <iostream>

// jsoncpp
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

// RapidJSON
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::runtime_error;

// The first example uses RapidJson to load a JSON document. If the document
// contains an array, this function will print any array values that have a
// valid string representation.
void usingRapidJson(const char *filename)
{
    using valijson::adapters::RapidJsonAdapter;

    rapidjson::Document document;
    if (!valijson::utils::loadDocument(filename, document)) {
        return;
    }

    const RapidJsonAdapter adapter(document);
    if (!adapter.isArray()) {
        cout << "Not an array." << endl;
        return;
    }

    cout << "Array values:" << endl;
    int index = 0;

    const RapidJsonAdapter::Array array = adapter.asArray();
    for (auto &&item : array) {
        cout << "  " << index++ << ": ";

        // maybeString is a loose type check
        if (item.maybeString()) {
            // If a value may be a string, we are allowed to get a string
            // representation of the value using asString
            cout << item.asString();
        }

        cout << endl;
    }
}

// The second example uses JsonCpp to perform the same task, but unlike the
// RapidJson example, we see how to use strict type checks and exception
// handling.
void usingJsonCpp(const char *filename)
{
    using valijson::adapters::JsonCppAdapter;

    Json::Value value;
    if (!valijson::utils::loadDocument(filename, value)) {
        return;
    }

    const JsonCppAdapter adapter(value);
    if (!adapter.isArray()) {
        cout << "Not an array." << endl;
        return;
    }

    cout << "Array values:" << endl;
    int index = 0;

    // If a value is not an array, then calling getArray will cause a runtime
    // exception to be raised.
    const JsonCppAdapter::Array array = adapter.getArray();
    for (auto &&item : array) {
        cout << "  " << index++ << ": ";

        // isString is another strict type check. Valijson uses the convention
        // that strict type check functions are prefixed with 'is'.
        if (!item.isString()) {
            cout << "Not a string. ";
        }

        try {
            // Also by convention, functions prefixed with 'get' will raise a
            // runtime exception if the item is not of the correct type.
            cout << item.getString() << endl;
        } catch (const runtime_error &e) {
            cout << "Caught exception: " << e.what() << endl;
        }
    }
}

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

/**
 * @file
 *
 * @brief Demonstrates iteration over the members of an object
 *
 */

#include <iostream>

#include <json/json.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

using std::cerr;
using std::cout;
using std::endl;

template<typename AdapterType>
void iterateJsonObject(const AdapterType &adapter)
{
    if (!adapter.maybeObject()) {
        cout << "Not an object." << endl;
        return;
    }

    cout << "Object members:" << endl;

    // JSON objects are an unordered collection of key-value pairs,
    // so the members of the object may be printed in an order that is
    // different to that in the source JSON document.
    for (auto member : adapter.asObject()) {
        // The key is a std::string that can be accessed using 'first'
        cout << "  " << member.first << ": ";

        // The value is just another Adapter, and can be accessed using 'second'
        const AdapterType &value = member.second;
        if (value.maybeString()) {
            cout << value.asString();
        }

        cout << endl;
    }
}

void usingJsonCppWithTemplateFn(const char *filename)
{
    Json::Value value;
    if (!valijson::utils::loadDocument(filename, value)) {
        return;
    }

    valijson::adapters::JsonCppAdapter adapter(value);
    iterateJsonObject(adapter);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        cerr << "Usage: " << endl;
        cerr << "  " << argv[0] << " <filename>" << endl;
        return 1;
    }

    cout << "-- Object iteration using jsoncpp via template function --" << endl;
    usingJsonCppWithTemplateFn(argv[1]);
    cout << endl;

    return 0;
}
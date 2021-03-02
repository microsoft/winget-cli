/**
 * @file
 *
 * @brief Demonstrates how to resolve JSON pointers against the current document
 *
 */

#include <iostream>

#include <rapidjson/document.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/internal/json_pointer.hpp>
#include <valijson/internal/json_reference.hpp>
#include <valijson/utils/rapidjson_utils.hpp>

using std::cerr;
using std::cout;
using std::endl;

template<typename AdapterType>
std::string maybeResolveRef(const AdapterType &value, const AdapterType &root)
{
    if (!value.isObject()) {
        // Not an object, therefore not a JSON reference
        return "";
    }

    const auto &object = value.getObject();
    const auto itr = object.find("$ref");
    if (itr == object.end()) {
        // Object does not contain $ref property
        return "";
    }

    const AdapterType maybeRef = itr->second;
    if (!maybeRef.isString()) {
        return "[$ref did not contain a string value]";
    }

    // Attempt to extract a JSON pointer
    const std::string ref = maybeRef.getString();
    const auto maybePointer = valijson::internal::json_reference::getJsonReferencePointer(ref);
    if (!maybePointer) {
        return "[$ref did not contain valid JSON pointer]";
    }

    const auto refAdapter = valijson::internal::json_pointer::resolveJsonPointer(root, *maybePointer);
    if (!refAdapter.maybeString()) {
        return "[$ref did not point to a string value]";
    }

    return refAdapter.asString();
}

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
        } else {
            cout << maybeResolveRef(value, adapter);
        }

        cout << endl;
    }
}

void usingJsonCppWithTemplateFn(const char *filename)
{
    rapidjson::Document document;
    if (!valijson::utils::loadDocument(filename, document)) {
        return;
    }

    valijson::adapters::RapidJsonAdapter adapter(document);
    iterateJsonObject(adapter);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        cerr << "Usage: " << endl;
        cerr << "  " << argv[0] << " <filename>" << endl;
        return 1;
    }

    // Load the document using jsoncpp and iterate over array using function template
    cout << "-- Resolving JSON pointers using RapidJSON --" << endl;
    usingJsonCppWithTemplateFn(argv[1]);
    cout << endl;

    return 0;
}

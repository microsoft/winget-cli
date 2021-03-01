/**
 * @file
 *
 * @brief Demonstrates iteration over an array using template functions
 *
 */

#include <iostream>

#include <json/json.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

using std::cerr;
using std::cout;
using std::endl;

template<class AdapterType>
void iterateJsonArray(const AdapterType &adapter)
{
    if (!adapter.isArray()) {
        cout << "Not an array." << endl;
        return;
    }

    cout << "Array values:" << endl;
    int index = 0;

    for (auto value : adapter.getArray()) {
        cout << "  " << index++ << ": ";
        
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
    iterateJsonArray(adapter);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        cerr << "Usage: " << endl;
        cerr << "  " << argv[0] << " <filename>" << endl;
        return 1;
    }

    // Load the document using jsoncpp and iterate over array using function template
    cout << "-- Array iteration using jsoncpp via template function --" << endl;
    usingJsonCppWithTemplateFn(argv[1]);
    cout << endl;

    return 0;
}

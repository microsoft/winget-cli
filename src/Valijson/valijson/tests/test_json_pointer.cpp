#include <memory>

#include <gtest/gtest.h>

#include <valijson/internal/json_pointer.hpp>

#include <valijson/adapters/rapidjson_adapter.hpp>

using valijson::adapters::RapidJsonAdapter;
using valijson::internal::json_pointer::resolveJsonPointer;

typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>
        RapidJsonCrtAllocator;

class TestJsonPointer : public testing::Test
{

};

struct JsonPointerTestCase
{
    JsonPointerTestCase(const std::string &description)
      : description(description) { }

    /// Description of test case
    std::string description;

    /// Document to traverse when resolving the JSON Pointer
    rapidjson::Value value;

    /// JSON Pointer that should guide traversal of the document
    std::string jsonPointer;

    /// Optional reference to the expected result from the original document
    rapidjson::Value *expectedValue;
};

std::vector<std::shared_ptr<JsonPointerTestCase> >
        testCasesForSingleLevelObjectPointers(
                RapidJsonCrtAllocator &allocator)
{
    typedef std::shared_ptr<JsonPointerTestCase> TestCase;

    std::vector<TestCase> testCases;

    TestCase testCase = std::make_shared<JsonPointerTestCase>(
            "Resolving '#' should cause an exception to be thrown");
    testCase->value.SetNull();
    testCase->jsonPointer = "#";
    testCase->expectedValue = NULL;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolving an empty string should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolving '/' should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "/";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolving '//' should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "//";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolve '/test' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/test";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolve '/test/' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/test/";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolve '//test//' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "//test//";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = std::make_shared<JsonPointerTestCase>(
            "Resolve '/missing' in object containing one member name 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/missing";
    testCase->expectedValue = NULL;
    testCases.push_back(testCase);

    {
        rapidjson::Value nonemptyString;
        nonemptyString.SetString("hello, world");

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolve '/value/foo' fails because 'value' is not an object (but a non empty string)");
        testCase->value.SetObject();
        testCase->value.AddMember("value", nonemptyString, allocator);
        testCase->jsonPointer = "/value/bar";
        testCase->expectedValue = &testCase->value;
        testCase->expectedValue = NULL;
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value emptyString;
        emptyString.SetString("");

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolve '/empty/after_empty' fails because 'empty' is an empty string");
        testCase->value.SetObject();
        testCase->value.AddMember("empty", emptyString, allocator);
        testCase->jsonPointer = "/empty/after_empty";
        testCase->expectedValue = NULL;
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolve '/test/0' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/0";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(0)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolve '/test/1' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/1";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(1)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolve '/test/2' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/2";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(2)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolving '/test/3' in object containing one member containing "
                "an array with 3 elements should throw an exception");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/3";
        testCase->expectedValue = NULL;
        testCases.push_back(testCase);
    }

    //
    // Allow the "-" character is not useful within the context of this library,
    // there is an explicit check for it, so that a custom error message can
    // be included in the exception that is thrown.
    //
    // From the JSON Pointer specification (RFC 6901, April 2013):
    //
    //    Note that the use of the "-" character to index an array will always
    //    result in such an error condition because by definition it refers to
    //    a nonexistent array element.  Thus, applications of JSON Pointer need
    //    to specify how that character is to be handled, if it is to be
    //    useful.
    //

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolving '/test/-' in object containing one member containing "
                "an array with 3 elements should throw an exception");
        testCase->value.SetNull();
        testCase->jsonPointer = "/test/-";
        testCase->expectedValue = NULL;
        testCases.push_back(testCase);
    }

    //
    // The following tests ensure that escape sequences are handled correctly.
    //
    // From the JSON Pointer specification (RFC 6901, April 2013):
    //
    //    Evaluation of each reference token begins by decoding any escaped
    //    character sequence.  This is performed by first transforming any
    //    occurrence of the sequence '~1' to '/', and then transforming any
    //    occurrence of the sequence '~0' to '~'.  By performing the
    //    substitutions in this order, an implementation avoids the error of
    //    turning '~01' first into '~1' and then into '/', which would be
    //    incorrect (the string '~01' correctly becomes '~1' after
    //    transformation).
    //

    {
        rapidjson::Value value;
        value.SetDouble(10.);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolving '/hello~1world' in object containing one member named "
                "'hello/world' should return the associated value");
        testCase->value.SetObject();
        testCase->value.AddMember("hello/world", value, allocator);
        testCase->jsonPointer = "/hello~1world";
        testCase->expectedValue = &testCase->value.FindMember("hello/world")->value;
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value value;
        value.SetDouble(10.);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolving '/hello~0world' in object containing one member named "
                "'hello~world' should return the associated value");
        testCase->value.SetObject();
        testCase->value.AddMember("hello~world", value, allocator);
        testCase->jsonPointer = "/hello~0world";
        testCase->expectedValue = &testCase->value.FindMember("hello~world")->value;
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value value;
        value.SetDouble(10.);

        testCase = std::make_shared<JsonPointerTestCase>(
                "Resolving '/hello~01world' in object containing one member named "
                "'hello~1world' should return the associated value");
        testCase->value.SetObject();
        testCase->value.AddMember("hello~1world", value, allocator);
        testCase->jsonPointer = "/hello~01world";
        testCase->expectedValue = &testCase->value.FindMember("hello~1world")->value;
        testCases.push_back(testCase);
    }

    return testCases;
}

TEST_F(TestJsonPointer, JsonPointerTestCases)
{
    typedef std::vector<std::shared_ptr<JsonPointerTestCase> > TestCases;

    // Ensure memory used for test cases is freed when test function completes
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> allocator;

    TestCases testCases = testCasesForSingleLevelObjectPointers(allocator);

    for (TestCases::const_iterator itr = testCases.begin();
            itr != testCases.end(); ++itr) {
        const std::string &jsonPointer = (*itr)->jsonPointer;
        const RapidJsonAdapter valueAdapter((*itr)->value);
        if ((*itr)->expectedValue) {
            const RapidJsonAdapter expectedAdapter(*((*itr)->expectedValue));
            const RapidJsonAdapter actualAdapter =
                    resolveJsonPointer(valueAdapter, jsonPointer);
            EXPECT_TRUE(actualAdapter.equalTo(expectedAdapter, true)) <<
                    (*itr)->description;
        } else {
            EXPECT_THROW(
                    resolveJsonPointer(valueAdapter, jsonPointer),
                    std::runtime_error) <<
                    (*itr)->description;
        }
    }
}

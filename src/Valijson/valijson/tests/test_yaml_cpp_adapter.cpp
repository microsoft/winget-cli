#include <gtest/gtest.h>

#include <valijson/adapters/yaml_cpp_adapter.hpp>

class TestYamlCppAdapter : public testing::Test
{
};

TEST_F(TestYamlCppAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    YAML::Node document;

    for (unsigned int i = 0; i < numElements; i++) {
        document.push_back(static_cast<double>(i));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::YamlCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ(numElements, adapter.getArray().size());

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::YamlCppAdapter value : adapter.getArray()) {
        ASSERT_TRUE(value.isString());
        ASSERT_FALSE(value.isNumber());
        ASSERT_TRUE(value.maybeDouble());
        EXPECT_EQ(double(expectedValue), value.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestYamlCppAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    YAML::Node document;
    for (uint32_t i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::YamlCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ(numElements, adapter.getObject().size());

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::YamlCppAdapter::ObjectMember member :
         adapter.getObject()) {
        ASSERT_TRUE(member.second.isString());
        ASSERT_FALSE(member.second.isNumber());
        ASSERT_TRUE(member.second.maybeDouble());
        EXPECT_EQ(std::to_string(expectedValue), member.first);
        EXPECT_EQ(double(expectedValue), member.second.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestYamlCppAdapter, BasicObjectMemberAccess)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    YAML::Node document;
    for (uint32_t i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }
    valijson::adapters::YamlCppAdapter adapter(document);
    const auto adapterObject = adapter.asObject();

    // Ensure that accessing an element that exists produces the expected result.
    const auto result3 = adapterObject.find("3");
    EXPECT_NE(result3, adapterObject.end());
    EXPECT_EQ(result3->second.asDouble(), 3);

    // Ensure that accessing an element that does not exists.
    const auto result12 = adapterObject.find("12");
    EXPECT_EQ(result12, adapterObject.end());
}

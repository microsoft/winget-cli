#include <gtest/gtest.h>

#include <valijson/adapters/poco_json_adapter.hpp>

class TestPocoJsonAdapter : public testing::Test
{

};

TEST_F(TestPocoJsonAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    Poco::JSON::Array::Ptr documentImpl = new Poco::JSON::Array();

    for (unsigned int i = 0; i < numElements; i++) {
        documentImpl->set(i, static_cast<double>(i));
    }

    Poco::Dynamic::Var document = documentImpl;

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::PocoJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::PocoJsonAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestPocoJsonAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a DropBoxJson document that consists of an object that maps numeric
    // strings their corresponding numeric values
    Poco::JSON::Object::Ptr documentImpl = new Poco::JSON::Object;

    for (uint32_t i = 0; i < numElements; i++) {
        documentImpl->set(std::to_string(i), static_cast<double>(i));
    }

    Poco::Dynamic::Var document = documentImpl;

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::PocoJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::PocoJsonAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

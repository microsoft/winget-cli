
#include <gtest/gtest.h>

#include <valijson/adapters/property_tree_adapter.hpp>

class TestPropertyTreeAdapter : public testing::Test
{

};

TEST_F(TestPropertyTreeAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a boost property tree that is equivalent to a JSON array containing a
    // list of numbers.
    boost::property_tree::ptree document;
    for (unsigned int i = 0; i < numElements; i++) {
        document.push_back(std::make_pair(std::string(),
            boost::property_tree::ptree(std::to_string(i))));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::PropertyTreeAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::PropertyTreeAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isString() );
        ASSERT_FALSE( value.isNumber() );
        ASSERT_TRUE( value.maybeDouble() );
        EXPECT_EQ( double(expectedValue), value.asDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestPropertyTreeAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a boost property tree that consists of an object that maps numeric
    // strings their corresponding numeric values
    boost::property_tree::ptree document;
    for (unsigned int i = 0; i < numElements; i++) {
        std::string name(std::to_string(i));
        document.push_back(std::make_pair(name, boost::property_tree::ptree(
            std::to_string(double(i)))));
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::PropertyTreeAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::PropertyTreeAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isString() );
        ASSERT_FALSE( member.second.isNumber() );
        ASSERT_TRUE( member.second.maybeDouble() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.asDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

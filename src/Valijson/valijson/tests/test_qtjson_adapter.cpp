#include <string>

#include <gtest/gtest.h>

#include <valijson/adapters/qtjson_adapter.hpp>

class TestQtJsonAdapter : public testing::Test
{

};

TEST_F(TestQtJsonAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a picojson document that consists of an array of numbers
    QJsonArray array;
    for (unsigned int i = 0; i < numElements; i++) {
        QJsonValue value(static_cast<double>(i));
        array.push_back(value);
    }
    QJsonValue document(array);

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::QtJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::QtJsonAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestQtJsonAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a picojson document that consists of an object that maps numeric
    // strings their corresponding numeric values
    QJsonObject object;
    for (unsigned int i = 0; i < numElements; i++) {
        QString name(QString::number(i));
        object[name] = QJsonValue(static_cast<double>(i));
    }
    QJsonValue document(object);

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::QtJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::QtJsonAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

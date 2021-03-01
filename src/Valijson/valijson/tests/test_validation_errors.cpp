#include <iostream>

#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#define TEST_DATA_DIR "../tests/data"

using std::string;

using valijson::adapters::AdapterTraits;
using valijson::adapters::RapidJsonAdapter;
using valijson::utils::loadDocument;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;

class TestValidationErrors : public ::testing::Test
{

};

TEST_F(TestValidationErrors, AllOfConstraintFailure)
{
    // Load schema document
    rapidjson::Document schemaDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/schemas/allof_integers_and_numbers.schema.json", schemaDocument) );
    RapidJsonAdapter schemaAdapter(schemaDocument);

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
    ASSERT_NO_THROW( schemaParser.populateSchema(schemaAdapter, schema) );

    // Load test document
    rapidjson::Document testDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/documents/array_doubles_1_2_3.json", testDocument) );
    RapidJsonAdapter testAdapter(testDocument);

    Validator validator;
    ValidationResults results;
    EXPECT_FALSE( validator.validate(schema, testAdapter, &results) );

    ValidationResults::Error error;

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[0]", error.context[1] );
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #0 in array.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[1]", error.context[1] );
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #1 in array.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[2]", error.context[1] );
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #2 in array.", error.description );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate against child schema #0.", error.description );

    EXPECT_FALSE( results.popError(error) );

    while (results.popError(error)) {
        //std::cerr << error.context << std::endl;
        std::cerr << error.description << std::endl;
    }
}

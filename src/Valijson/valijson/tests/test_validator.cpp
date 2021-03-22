#include <picojson.h>

#include <iostream>

#include <gtest/gtest.h>

#include <valijson/adapters/json11_adapter.hpp>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/adapters/picojson_adapter.hpp>
#include <valijson/utils/json11_utils.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/utils/picojson_utils.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#ifdef VALIJSON_BUILD_POCO_ADAPTER
#include <valijson/adapters/poco_json_adapter.hpp>
#include <valijson/utils/poco_json_utils.hpp>
#endif

#define REMOTES_DIR "../thirdparty/JSON-Schema-Test-Suite/remotes/"

#define TEST_SUITE_DIR "../thirdparty/JSON-Schema-Test-Suite/tests/"

using valijson::adapters::AdapterTraits;
using valijson::adapters::RapidJsonAdapter;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;

std::string getRelativePath(const std::string &uri)
{
   const std::string dummyUri = "http://localhost:1234/";
   size_t n = uri.find(dummyUri);
   if (n != std::string::npos) {
       return REMOTES_DIR + uri.substr(dummyUri.size());
   }

   const std::string v3SchemaUri = "http://json-schema.org/draft-03/schema";
   n = uri.find(v3SchemaUri);
   if (n != std::string::npos) {
       return "../doc/schema/draft-03.json";
   }

   const std::string v4SchemaUri = "http://json-schema.org/draft-04/schema";
   n = uri.find(v4SchemaUri);
   if (n != std::string::npos) {
       return "../doc/schema/draft-04.json";
   }

   throw std::runtime_error("Attempt fetchDoc of " + uri);
}

template<typename AdapterType>
const typename AdapterTraits<AdapterType>::DocumentType * fetchDocument(
       const std::string &uri)
{
   const std::string relativePath = getRelativePath(uri);

   typename AdapterTraits<AdapterType>::DocumentType *document =
           new typename AdapterTraits<AdapterType>::DocumentType();

   if (!valijson::utils::loadDocument(relativePath, *document)) {
       delete document;
       throw std::runtime_error("Failed fetchDoc of " + uri);
   }

   return document;
}

template<typename AdapterType>
void freeDocument(const typename AdapterTraits<AdapterType>::DocumentType *ptr)
{
   delete ptr;
}

class TestValidator : public ::testing::TestWithParam<const char *>
{
protected:

    template<typename AdapterType>
    static void processTestFile(const std::string &testFile,
                                const SchemaParser::Version version)
    {
        std::string currentTestCase;
        std::string currentTest;

        try {

            // Load test document
            typename AdapterTraits<AdapterType>::DocumentType document;
            ASSERT_TRUE( valijson::utils::loadDocument(testFile, document) );
            AdapterType testCases(document);
            ASSERT_TRUE( testCases.isArray() );

            // Process each test case in the file
            for (const AdapterType testCase : testCases.getArray()) {
                currentTestCase.clear();
                currentTest.clear();

                // Ensure that testCase is an object
                ASSERT_TRUE( testCase.isObject() );
                const typename AdapterType::Object object = testCase.getObject();

                // Get test case description
                typename AdapterType::Object::const_iterator itr = object.find("description");
                ASSERT_NE( object.end(), itr );
                currentTestCase = itr->second.getString();

                // Ensure that 'schema' property is present
                itr = object.find("schema");
                ASSERT_NE( object.end(), itr );

                // Parse schema
                Schema schema;
                SchemaParser parser(version);
                parser.populateSchema(itr->second, schema,
                        fetchDocument<AdapterType>, freeDocument<AdapterType>);

                // For each test in the 'tests' array
                itr = object.find("tests");
                ASSERT_NE( object.end(), itr );
                ASSERT_TRUE( itr->second.isArray() );
                for (const AdapterType test : itr->second.getArray()) {
                    const bool strict = itr->second.hasStrictTypes();

                    ASSERT_TRUE( test.isObject() );
                    const typename AdapterType::Object testObject = test.getObject();

                    typename AdapterType::Object::const_iterator testItr = testObject.find("valid");
                    ASSERT_NE( testObject.end(), testItr );
                    ASSERT_TRUE( testItr->second.maybeBool() );
                    const bool shouldValidate = testItr->second.getBool();

                    testItr = testObject.find("description");
                    ASSERT_NE( testObject.end(), testItr );
                    currentTest = testItr->second.getString();

                    testItr = testObject.find("data");
                    ASSERT_NE( testObject.end(), testItr );
                    Validator validator(strict ?
                            Validator::kStrongTypes : Validator::kWeakTypes);

                    EXPECT_EQ( shouldValidate, validator.validate(schema, testItr->second, NULL) )
                        << "Failed while testing validate() function in '"
                        << currentTest << "' of test case '"
                        << currentTestCase << "' with adapter '"
                        << AdapterTraits<AdapterType>::adapterName() << "'";
                }
            }

        } catch (const std::exception &e) {
            FAIL() << "Exception thrown with message '" << e.what()
                   << "' in '" << currentTest << "' of test case '"
                   << currentTestCase << "' with adapter '"
                   << AdapterTraits<AdapterType>::adapterName() << "'";
        }
    }

    void processTestFile(const std::string &testFile,
                         const SchemaParser::Version version)
    {
        processTestFile<valijson::adapters::Json11Adapter>(testFile, version);
        processTestFile<valijson::adapters::JsonCppAdapter>(testFile, version);
        processTestFile<valijson::adapters::RapidJsonAdapter>(testFile, version);
        processTestFile<valijson::adapters::PicoJsonAdapter>(testFile, version);

#ifdef VALIJSON_BUILD_POCO_ADAPTER
        processTestFile<valijson::adapters::PocoJsonAdapter>(testFile, version);
#endif // VALIJSON_BUILD_POCO_ADAPTER
    }

    void processDraft3TestFile(const std::string &testFile)
    {
        return processTestFile(testFile, SchemaParser::kDraft3);
    }

    void processDraft4TestFile(const std::string &testFile)
    {
        return processTestFile(testFile, SchemaParser::kDraft4);
    }

    void processDraft7TestFile(const std::string &testFile)
    {
        return processTestFile(testFile, SchemaParser::kDraft7);
    }
};

//
// draft 3
// ------------------------------------------------------------------------------------------------
//

TEST_F(TestValidator, Draft3_AdditionalItems)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/additionalItems.json");
}

TEST_F(TestValidator, Draft3_AdditionalProperties)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/additionalProperties.json");
}

TEST_F(TestValidator, Draft3_Dependencies)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/dependencies.json");
}

TEST_F(TestValidator, Draft3_DivisibleBy)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/divisibleBy.json");
}

TEST_F(TestValidator, Draft3_Enum)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/enum.json");
}

TEST_F(TestValidator, Draft3_Items)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/items.json");
}

TEST_F(TestValidator, Draft3_Maximum)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/maximum.json");
}

TEST_F(TestValidator, Draft3_MaxItems)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/maxItems.json");
}

TEST_F(TestValidator, Draft3_MaxLength)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/maxLength.json");
}

TEST_F(TestValidator, Draft3_Minimum)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/minimum.json");
}

TEST_F(TestValidator, Draft3_MinItems)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/minItems.json");
}

TEST_F(TestValidator, Draft3_MinLength)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/minLength.json");
}

TEST_F(TestValidator, Draft3_Pattern)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/pattern.json");
}

TEST_F(TestValidator, Draft3_PatternProperties)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/patternProperties.json");
}

TEST_F(TestValidator, Draft3_Properties)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/properties.json");
}

TEST_F(TestValidator, Draft3_Ref)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/ref.json");
}

TEST_F(TestValidator, Draft3_RefRemote)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/refRemote.json");
}

TEST_F(TestValidator, Draft3_Required)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/required.json");
}

TEST_F(TestValidator, Draft3_Type)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/type.json");
}

TEST_F(TestValidator, Draft3_UniqueItems)
{
    processDraft3TestFile(TEST_SUITE_DIR "draft3/uniqueItems.json");
}

//
// draft 4
// ------------------------------------------------------------------------------------------------
//

TEST_F(TestValidator, Draft4_AdditionalItems)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/additionalItems.json");
}

TEST_F(TestValidator, Draft4_AdditionalProperties)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/additionalProperties.json");
}

TEST_F(TestValidator, Draft4_AllOf)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/allOf.json");
}

TEST_F(TestValidator, Draft4_AnyOf)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/anyOf.json");
}

TEST_F(TestValidator, Draft4_Dependencies)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/dependencies.json");
}

TEST_F(TestValidator, Draft4_Enum)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/enum.json");
}

TEST_F(TestValidator, Draft4_Items)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/items.json");
}

TEST_F(TestValidator, Draft4_Maximum)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/maximum.json");
}

TEST_F(TestValidator, Draft4_MaxItems)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/maxItems.json");
}

TEST_F(TestValidator, Draft4_MaxLength)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/maxLength.json");
}

TEST_F(TestValidator, Draft4_MaxProperties)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/maxProperties.json");
}

TEST_F(TestValidator, Draft4_Minimum)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/minimum.json");
}

TEST_F(TestValidator, Draft4_MinItems)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/minItems.json");
}

TEST_F(TestValidator, Draft4_MinLength)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/minLength.json");
}

TEST_F(TestValidator, Draft4_MinProperties)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/minProperties.json");
}

TEST_F(TestValidator, Draft4_MultipleOf)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/multipleOf.json");
}

TEST_F(TestValidator, Draft4_Not)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/not.json");
}

TEST_F(TestValidator, Draft4_OneOf)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/oneOf.json");
}

TEST_F(TestValidator, Draft4_Pattern)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/pattern.json");
}

TEST_F(TestValidator, Draft4_PatternProperties)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/patternProperties.json");
}

TEST_F(TestValidator, Draft4_Properties)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/properties.json");
}

// TODO: broken ref

// TODO: broken refRemote

// TODO: broken required

TEST_F(TestValidator, Draft4_Type)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/type.json");
}

TEST_F(TestValidator, Draft4_UniqueItems)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft4/uniqueItems.json");
}

//
// draft 7
// ------------------------------------------------------------------------------------------------
//

TEST_F(TestValidator, Draft7_AdditionalItems)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/additionalItems.json");
}

TEST_F(TestValidator, Draft7_AdditionalProperties)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/additionalProperties.json");
}

TEST_F(TestValidator, Draft7_AllOf)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/allOf.json");
}

TEST_F(TestValidator, Draft7_AnyOf)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/anyOf.json");
}

TEST_F(TestValidator, Draft7_BooleanSchema)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/boolean_schema.json");
}

TEST_F(TestValidator, Draft7_Const)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/const.json");
}

TEST_F(TestValidator, Draft7_Contains)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/contains.json");
}

// TOOD: untested default

TEST_F(TestValidator, Draft7_Dependencies)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/dependencies.json");
}

TEST_F(TestValidator, Draft7_Enum)
{
    processDraft4TestFile(TEST_SUITE_DIR "draft7/enum.json");
}

TEST_F(TestValidator, Draft7_ExclusiveMaximum)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/exclusiveMaximum.json");
}

TEST_F(TestValidator, Draft7_ExclusiveMinimum)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/exclusiveMinimum.json");
}

TEST_F(TestValidator, Draft7_IfThenElse)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/if-then-else.json");
}

TEST_F(TestValidator, Draft7_Items)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/items.json");
}

TEST_F(TestValidator, Draft7_Maximum)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/maximum.json");
}

TEST_F(TestValidator, Draft7_MaxItems)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/maxItems.json");
}

TEST_F(TestValidator, Draft7_MaxProperties)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/maxProperties.json");
}

TEST_F(TestValidator, Draft7_Minimum)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/minimum.json");
}

TEST_F(TestValidator, Draft7_MinItems)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/minItems.json");
}

TEST_F(TestValidator, Draft7_MinProperties)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/minProperties.json");
}

TEST_F(TestValidator, Draft7_MultipleOf)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/multipleOf.json");
}

TEST_F(TestValidator, Draft7_Not)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/not.json");
}

TEST_F(TestValidator, Draft7_OneOf)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/oneOf.json");
}

TEST_F(TestValidator, Draft7_Pattern)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/pattern.json");
}

TEST_F(TestValidator, Draft7_PatternProperties)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/patternProperties.json");
}

TEST_F(TestValidator, Draft7_Properties)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/properties.json");
}

TEST_F(TestValidator, Draft7_PropertyNames)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/propertyNames.json");
}

// TODO: broken ref

// TODO: broken refRemote

TEST_F(TestValidator, Draft7_Required)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/required.json");
}

TEST_F(TestValidator, Draft7_Type)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/type.json");
}

TEST_F(TestValidator, Draft7_UniqueItems)
{
    processDraft7TestFile(TEST_SUITE_DIR "draft7/uniqueItems.json");
}

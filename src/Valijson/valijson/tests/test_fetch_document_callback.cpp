
#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>

#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::adapters::RapidJsonAdapter;
using valijson::Validator;

class TestFetchDocumentCallback : public ::testing::Test
{

};

const rapidjson::Document * fetchDocument(const std::string &uri)
{
    EXPECT_STREQ("http://localhost:1234/", uri.c_str());

    rapidjson::Document *fetchedRoot = new rapidjson::Document();
    fetchedRoot->SetObject();

    rapidjson::Value valueOfTypeAttribute;
    valueOfTypeAttribute.SetString("string", fetchedRoot->GetAllocator());

    rapidjson::Value schemaOfTestProperty;
    schemaOfTestProperty.SetObject();
    schemaOfTestProperty.AddMember("type", valueOfTypeAttribute,
            fetchedRoot->GetAllocator());

    rapidjson::Value propertiesConstraint;
    propertiesConstraint.SetObject();
    propertiesConstraint.AddMember("test", schemaOfTestProperty,
            fetchedRoot->GetAllocator());

    fetchedRoot->AddMember("properties", propertiesConstraint,
            fetchedRoot->GetAllocator());

    return fetchedRoot;
}

void freeDocument(const rapidjson::Document *adapter)
{
    delete adapter;
}

TEST_F(TestFetchDocumentCallback, Basics)
{
    // Define schema
    rapidjson::Document schemaDocument;
    RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    schemaDocument.SetObject();
    schemaDocument.AddMember("$ref", "http://localhost:1234/#/",
            schemaDocument.GetAllocator());

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
    schemaParser.populateSchema(schemaDocumentAdapter, schema, fetchDocument,
            freeDocument);

    // Test resulting schema with a valid document
    rapidjson::Document validDocument;
    validDocument.SetObject();
    validDocument.AddMember("test", "valid", schemaDocument.GetAllocator());
    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument),
            NULL));

    // Test resulting schema with an invalid document
    rapidjson::Document invalidDocument;
    invalidDocument.SetObject();
    invalidDocument.AddMember("test", 123, schemaDocument.GetAllocator());
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument),
            NULL));
}

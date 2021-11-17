
#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>

#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::adapters::RapidJsonAdapter;
using valijson::Validator;

class TestFetchUrnDocumentCallback : public ::testing::Test
{

};

const rapidjson::Document * fetchUrnDocument(const std::string &uri)
{
    EXPECT_STREQ("urn:mvn:example.schema.common:status:1.1.0", uri.c_str());

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

void freeUrnDocument(const rapidjson::Document *adapter)
{
    delete adapter;
}

TEST_F(TestFetchUrnDocumentCallback, Basics)
{
    // Define schema
    rapidjson::Document schemaDocument;
    RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    schemaDocument.SetObject();
    schemaDocument.AddMember("$ref", "urn:mvn:example.schema.common:status:1.1.0",
            schemaDocument.GetAllocator());

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
    schemaParser.populateSchema(schemaDocumentAdapter, schema, fetchUrnDocument,
            freeUrnDocument);

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

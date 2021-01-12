#include <gtest/gtest.h>

#include <iostream>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace {

using valijson::adapters::Adapter;
using valijson::Schema;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::RapidJsonAdapter;

class StubPolyConstraint : public valijson::constraints::PolyConstraint
{
    bool shouldValidate;

public:
    StubPolyConstraint(bool shouldValidate)
      : shouldValidate(shouldValidate) { }

    virtual Constraint * cloneInto(void *ptr) const
    {
        return new (ptr) StubPolyConstraint(shouldValidate);
    }

    virtual size_t sizeOf() const
    {
        return sizeof(StubPolyConstraint);
    }

    virtual bool validate(const Adapter &target,
            const std::vector<std::string> &context,
            ValidationResults *results) const
    {
        if (shouldValidate) {
            return true;
        }

        if (results) {
            results->pushError(context,
                    "StubPolyConstraint intentionally failed validation");
        }

        return false;
    }
};

}  // end anonymous namespace

class TestPolyConstraint : public testing::Test
{

};

TEST_F(TestPolyConstraint, ValidationCanPass)
{
    StubPolyConstraint stubPolyConstraint(true);

    Schema schema;
    schema.addConstraintToSubschema(stubPolyConstraint, schema.root());

    rapidjson::Document document;
    RapidJsonAdapter adapter(document);

    ValidationResults results;

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, adapter, &results));
    EXPECT_EQ(size_t(0), results.numErrors());
}

TEST_F(TestPolyConstraint, ValidationCanFail)
{
    StubPolyConstraint stubPolyConstraint(false);

    Schema schema;
    schema.addConstraintToSubschema(stubPolyConstraint, schema.root());

    rapidjson::Document document;
    RapidJsonAdapter adapter(document);

    ValidationResults results;

    Validator validator;
    EXPECT_FALSE(validator.validate(schema, adapter, &results));
    EXPECT_EQ(size_t(1), results.numErrors());

    ValidationResults::Error error;
    EXPECT_TRUE(results.popError(error));
    EXPECT_STREQ("StubPolyConstraint intentionally failed validation",
            error.description.c_str());
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Rest/Schema/JsonHelper.h"
#include "cpprest/json.h"

using namespace AppInstaller::Repository::Rest::Schema;

web::json::value GetTestJsonObject()
{
    web::json::value jsonObject = web::json::value::object();
    jsonObject[L"Key1"] = web::json::value::string(L"Value1");
    jsonObject[L"Key2"] = web::json::value::string(L"Value2");
    jsonObject[L"IntKey"] = 100;

    web::json::value arrayValue = web::json::value::array();
    arrayValue[0] = web::json::value::string(L"ArrayValue1");
    arrayValue[1] = web::json::value::string(L"ArrayValue2");
    arrayValue[2] = web::json::value::string(L"ArrayValue3");
    jsonObject[L"Array"] = arrayValue;

    return jsonObject;
}

TEST_CASE("GetUtilityString", "[RestSource]")
{
    REQUIRE(JsonHelper::GetUtilityString("cpprest") == L"cpprest");
    REQUIRE(JsonHelper::GetUtilityString("  ") == L"  ");
}

TEST_CASE("GetJsonValueFromNode", "[RestSource]")
{
    web::json::value jsonObject = GetTestJsonObject();
    std::optional<std::reference_wrapper<const web::json::value>> actual = JsonHelper::GetJsonValueFromNode(jsonObject, L"Key1");
    REQUIRE(actual);
    REQUIRE(actual.value().get().as_string() == L"Value1");

    std::optional<std::reference_wrapper<const web::json::value>> absentKey = JsonHelper::GetJsonValueFromNode(jsonObject, L"Key3");
    REQUIRE(!absentKey);

    web::json::value emptyObject;
    std::optional<std::reference_wrapper<const web::json::value>> empty = JsonHelper::GetJsonValueFromNode(emptyObject, L"Key1");
    REQUIRE(!empty);
}

TEST_CASE("GetRawStringValueFromJsonValue", "[RestSource]")
{
    std::optional<std::string> stringTest = JsonHelper::GetRawStringValueFromJsonValue(web::json::value::string(L"cpprest "));
    REQUIRE(stringTest);
    REQUIRE(stringTest.value() == "cpprest ");

    std::optional<std::string> emptyTest = JsonHelper::GetRawStringValueFromJsonValue(web::json::value::string(L"   "));
    REQUIRE(emptyTest);
    REQUIRE(emptyTest.value() == "   ");

    web::json::value obj;
    std::optional<std::string> nullTest = JsonHelper::GetRawStringValueFromJsonValue(obj);
    REQUIRE(!nullTest);

    web::json::value integer = 100;
    std::optional<std::string> mismatchFieldTest = JsonHelper::GetRawStringValueFromJsonValue(integer);
    REQUIRE(!mismatchFieldTest);
}

TEST_CASE("GetRawStringValueFromJsonNode", "[RestSource]")
{
    web::json::value jsonObject = GetTestJsonObject();

    std::optional<std::string> stringTest = JsonHelper::GetRawStringValueFromJsonNode(jsonObject, L"Key1");
    REQUIRE(stringTest);
    REQUIRE(stringTest.value() == "Value1");

    std::optional<std::string> emptyTest = JsonHelper::GetRawStringValueFromJsonNode(jsonObject, L"Key3");
    REQUIRE(!emptyTest);

    std::optional<std::string> mismatchFieldTest = JsonHelper::GetRawStringValueFromJsonNode(jsonObject, L"IntKey");
    REQUIRE(!mismatchFieldTest);
}

TEST_CASE("GetRawIntValueFromJsonValue", "[RestSource]")
{
    web::json::value jsonObject = 100;
    std::optional<int> expected = JsonHelper::GetRawIntValueFromJsonValue(jsonObject);
    REQUIRE(expected);
    REQUIRE(expected.value() == 100);

    std::optional<int> mismatchFieldTest = JsonHelper::GetRawIntValueFromJsonValue(web::json::value::string(L"cpprest"));
    REQUIRE(!mismatchFieldTest);
}

TEST_CASE("GetRawJsonArrayFromJsonNode", "[RestSource]")
{
    web::json::value jsonObject = GetTestJsonObject();
    std::optional<std::reference_wrapper<const web::json::array>> expected = JsonHelper::GetRawJsonArrayFromJsonNode(jsonObject, L"Array");
    REQUIRE(expected);
    REQUIRE(expected.value().get().size() == 3);
    REQUIRE(expected.value().get().at(0).as_string() == L"ArrayValue1");

    std::optional<std::reference_wrapper<const web::json::array>> mismatchFieldTest = JsonHelper::GetRawJsonArrayFromJsonNode(jsonObject, L"Keyword");
    REQUIRE(!mismatchFieldTest);
}

TEST_CASE("GetRawStringArrayFromJsonNode", "[RestSource]")
{
    web::json::value jsonObject = GetTestJsonObject();
    std::vector<AppInstaller::Manifest::string_t> expected = JsonHelper::GetRawStringArrayFromJsonNode(jsonObject, L"Array");
    REQUIRE(expected.size() == 3);
    REQUIRE(expected[0] == "ArrayValue1");

    std::vector<AppInstaller::Manifest::string_t> mismatchFieldTest = JsonHelper::GetRawStringArrayFromJsonNode(jsonObject, L"Keyword");
    REQUIRE(mismatchFieldTest.size() == 0);
}

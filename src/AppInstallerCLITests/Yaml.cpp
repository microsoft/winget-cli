// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <winget/Yaml.h>

using namespace TestCommon;
using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;


TEST_CASE("YamlParserTypes", "[YAML]")
{
    auto document = AppInstaller::YAML::Load(TestDataFile("Node-Types.yaml"));

    auto intUnquoted = document["IntegerUnquoted"];
    CHECK(intUnquoted.GetTagType() == Node::TagType::Int);

    auto intSingleQuoted = document["IntegerSingleQuoted"];
    CHECK(intSingleQuoted.GetTagType() == Node::TagType::Str);

    auto intDoubleQuoted = document["IntegerDoubleQuoted"];
    CHECK(intDoubleQuoted.GetTagType() == Node::TagType::Str);

    auto boolTrue = document["BooleanTrue"];
    CHECK(boolTrue.GetTagType() == Node::TagType::Bool);

    auto strTrue = document["StringTrue"];
    CHECK(strTrue.GetTagType() == Node::TagType::Str);

    auto boolFalse = document["BooleanFalse"];
    CHECK(boolFalse.GetTagType() == Node::TagType::Bool);

    auto strFalse = document["StringFalse"];
    CHECK(strFalse.GetTagType() == Node::TagType::Str);

    auto localTag = document["LocalTag"];
    CHECK(localTag.GetTagType() == Node::TagType::Unknown);
}

TEST_CASE("YamlMergeMappingNode", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Mapping.yaml"));

    auto mergeNode = document["MergeNode"];
    auto mergeNode2 = document["MergeNode2"];

    REQUIRE(3 == mergeNode.size());
    REQUIRE(2 == mergeNode2.size());

    mergeNode.MergeMappingNode(mergeNode2);

    REQUIRE(5 == mergeNode.size());
}

TEST_CASE("YamlMergeMappingNode_CaseInsensitive", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Mapping.yaml"));

    auto mergeNode = document["MergeNode"];
    auto mergeNode2 = document["MergeNode2"];

    REQUIRE(3 == mergeNode.size());
    REQUIRE(2 == mergeNode2.size());

    mergeNode.MergeMappingNode(mergeNode2, true);

    REQUIRE(4 == mergeNode.size());
}

TEST_CASE("YamlMergeSequenceNode", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Merge.yaml"));
    auto document2 = Load(TestDataFile("Node-Merge2.yaml"));

    REQUIRE(3 == document["StrawHats"].size());
    REQUIRE(2 == document2["StrawHats"].size());

    // Internally will call MergeMappingNode.
    document["StrawHats"].MergeSequenceNode(document2["StrawHats"], "Bounty");
    REQUIRE(5 == document["StrawHats"].size());
}

TEST_CASE("YamlMergeSequenceNode_CaseInsensitive", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Merge.yaml"));
    auto document2 = Load(TestDataFile("Node-Merge2.yaml"));

    REQUIRE(3 == document["StrawHats"].size());
    REQUIRE(2 == document2["StrawHats"].size());

    // Internally will call MergeMappingNode.
    document["StrawHats"].MergeSequenceNode(document2["StrawHats"], "Name", true);
    REQUIRE(4 == document["StrawHats"].size());

    auto luffy = std::find_if(
        document["StrawHats"].Sequence().begin(),
        document["StrawHats"].Sequence().end(),
        [](Node const& n) { return n["Name"].as<std::string>() == "Monkey D Luffy"; });
    REQUIRE(luffy != document["StrawHats"].Sequence().end());

    // From original node
    REQUIRE((*luffy)["Bounty"].as<std::string>() == "3,000,000,000");

    // From merged node
    REQUIRE((*luffy)["Fruit"].as<std::string>() == "Gomu Gomu no Mi");
}

TEST_CASE("YamlMergeNode_MergeSequenceNoKey", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Merge.yaml"));
    auto document2 = Load(TestDataFile("Node-Merge2.yaml"));

    REQUIRE_THROWS_HR(document["StrawHats"].MergeSequenceNode(document2["StrawHats"], "Power"), APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA);
}

TEST_CASE("YamlMappingNode", "[YAML]")
{
    auto document = Load(TestDataFile("Node-Mapping.yaml"));

    auto node = document["key"];
    REQUIRE(node.as<std::string>() == "value");

    auto node2 = document.GetChildNode("KEY");
    REQUIRE(node2.as<std::string>() == "value");

    auto node3 = document.GetChildNode("key");
    REQUIRE(node3.as<std::string>() == "value");

    auto node4 = document.GetChildNode("kEy");
    REQUIRE(node4.as<std::string>() == "value");

    auto node5 = document.GetChildNode("fake");
    REQUIRE(node5.IsNull());

    auto node6 = document["repeatedkey"];
    REQUIRE(node6.as<std::string>() == "repeated value");
    REQUIRE_THROWS_HR(document.GetChildNode("repeatedkey"), APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);

    REQUIRE_THROWS_HR(document.GetChildNode("RepeatedKey"), APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);
    REQUIRE_THROWS_HR(document["RepeatedKey"], APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);
}

TEST_CASE("YamlMappingNode_const", "[YAML]")
{
    const auto document = Load(TestDataFile("Node-Mapping.yaml"));

    auto node = document["key"];
    REQUIRE(node.as<std::string>() == "value");

    auto node2 = document.GetChildNode("KEY");
    REQUIRE(node2.as<std::string>() == "value");

    auto node3 = document.GetChildNode("key");
    REQUIRE(node3.as<std::string>() == "value");

    auto node4 = document.GetChildNode("kEy");
    REQUIRE(node4.as<std::string>() == "value");

    auto node5 = document.GetChildNode("fake");
    REQUIRE(node5.IsNull());

    auto node6 = document["repeatedkey"];
    REQUIRE(node6.as<std::string>() == "repeated value");
    REQUIRE_THROWS_HR(document.GetChildNode("repeatedkey"), APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);

    REQUIRE_THROWS_HR(document.GetChildNode("RepeatedKey"), APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);
    REQUIRE_THROWS_HR(document["RepeatedKey"], APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY);
}

TEST_CASE("YamlContainsEscapeControlCode", "[YAML]")
{
    REQUIRE_THROWS_HR(Load(TestDataFile("ContainsEscapeControlCode.yaml")), APPINSTALLER_CLI_ERROR_LIBYAML_ERROR);
}

TEST_CASE("YamlContainsTooManyNestedLayers", "[YAML]")
{
    REQUIRE_THROWS_HR(Load(TestDataFile("ContainsTooManyNestedLayers.yaml")), APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
}

TEST_CASE("YamlAliases", "[YAML]")
{
    std::string_view input = R"(
Key: &key Shared
Switches: &switches
  Silent: /S
Copy: *switches
*key : Aliased key
)";

    Node document = Load(input);
    REQUIRE(document["Copy"]["Silent"].as<std::string>() == "/S");
    REQUIRE(document["Shared"].as<std::string>() == "Aliased key");
}

TEST_CASE("YamlContainsTooManyAliasedNodes", "[YAML]")
{
    // Each level references the previous one ten times, expanding to over 100,000 small nodes.
    std::string_view input = R"(
a: &a [x, x, x, x, x, x, x, x, x, x]
b: &b [*a, *a, *a, *a, *a, *a, *a, *a, *a, *a]
c: &c [*b, *b, *b, *b, *b, *b, *b, *b, *b, *b]
d: &d [*c, *c, *c, *c, *c, *c, *c, *c, *c, *c]
e: [*d, *d, *d, *d, *d, *d, *d, *d, *d, *d]
)";

    REQUIRE_THROWS_HR(Load(input), APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
}

TEST_CASE("YamlContainsTooMuchAliasedData", "[YAML]")
{
    // A few aliases of a large value copy more data than allowed.
    std::string input = "a: &a " + std::string(1024 * 1024, 'x') + "\nb: [*a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a, *a]\n";

    REQUIRE_THROWS_HR(Load(input), APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
}

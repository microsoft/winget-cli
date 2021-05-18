// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <winget/Regex.h>
#include <PackageCollection.h>

// Duplicating here because a change to these values in the product *REALLY* needs to be thought through.
using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

const std::string s_PackagesJson_Schema = "$schema";
const std::string s_PackagesJson_SchemaUri_v2_0 = "https://aka.ms/winget-packages.schema.2.0.json";
const std::string s_PackagesJson_WinGetVersion = "WinGetVersion";
const std::string s_PackagesJson_CreationDate = "CreationDate";

const std::string s_PackagesJson_Sources = "Sources";
const std::string s_PackagesJson_Source_Details = "SourceDetails";
const std::string s_PackagesJson_Source_Name = "Name";
const std::string s_PackagesJson_Source_Identifier = "Identifier";
const std::string s_PackagesJson_Source_Argument = "Argument";
const std::string s_PackagesJson_Source_Type = "Type";

const std::string s_PackagesJson_Packages = "Packages";
const std::string s_PackagesJson_Package_PackageIdentifier = "PackageIdentifier";
const std::string s_PackagesJson_Package_Version = "Version";
const std::string s_PackagesJson_Package_Channel = "Channel";

namespace
{

    Json::Value ParseJsonString(const std::string& jsonString)
    {
        Json::Value root;
        std::stringstream{ jsonString } >> root;
        return root;
    }

    void ValidateJsonStringProperty(const Json::Value& node, const std::string& propertyName, std::string_view expectedValue, bool allowMissing = false)
    {
        if (allowMissing && expectedValue.empty() && !node.isMember(propertyName))
        {
            return;
        }

        REQUIRE(node.isMember(propertyName));
        REQUIRE(node[propertyName].isString());
        REQUIRE(node[propertyName].asString() == expectedValue);
    }

    void ValidateJsonStringPropertyRegex(const Json::Value& node, const std::string& propertyName, std::string_view regex, bool allowMissing = false)
    {
        if (allowMissing && !node.isMember(propertyName))
        {
            return;
        }

        REQUIRE(node.isMember(propertyName));
        REQUIRE(node[propertyName].isString());
        const auto& value = AppInstaller::Utility::ConvertToUTF16(node[propertyName].asString());
        REQUIRE(AppInstaller::Regex::Expression(regex).IsMatch(value));
    }

    const Json::Value& GetAndValidateJsonProperty(const Json::Value& node, const std::string& propertyName, Json::ValueType valueType)
    {
        REQUIRE(node.isMember(propertyName));
        REQUIRE(node[propertyName].type() == valueType);
        return node[propertyName];
    }

    void ValidateJsonWithCollection(const Json::Value& root, const PackageCollection& collection)
    {
        ValidateJsonStringProperty(root, s_PackagesJson_Schema, s_PackagesJson_SchemaUri_v2_0);
        ValidateJsonStringProperty(root, s_PackagesJson_WinGetVersion, collection.ClientVersion);

        // valijson does not validate the date-time format, which should follow RFC3339 according to the JSON schema.
        // Ensure we write something at least reasonable.
        // The expected format is <Date>T<Time><TimeZone>
        // with Date="YYYY-MM-DD"; Time="HH:mm:ss.xxx"; TimeZone="Z" or "+HH:mm" or "-HH:mm" (offset from UTC)
        std::string_view dateTimeRegex = "[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]+(Z|[+-][0-9]{2}:[0-9]{2})"sv;
        ValidateJsonStringPropertyRegex(root, s_PackagesJson_CreationDate, dateTimeRegex);

        const auto& jsonSources = GetAndValidateJsonProperty(root, s_PackagesJson_Sources, Json::ValueType::arrayValue);
        REQUIRE(jsonSources.size() == collection.Sources.size());

        // Expect the order to be the same. Not really needed, but it makes things easier.
        auto jsonSourceItr = jsonSources.begin();
        auto sourceItr = collection.Sources.begin();
        for (; jsonSourceItr != jsonSources.end(); ++jsonSourceItr, ++sourceItr)
        {
            REQUIRE(jsonSourceItr->isObject());
            const auto& jsonSourceDetails = GetAndValidateJsonProperty(*jsonSourceItr, s_PackagesJson_Source_Details, Json::ValueType::objectValue);
            ValidateJsonStringProperty(jsonSourceDetails, s_PackagesJson_Source_Name, sourceItr->Details.Name);
            ValidateJsonStringProperty(jsonSourceDetails, s_PackagesJson_Source_Argument, sourceItr->Details.Arg);
            ValidateJsonStringProperty(jsonSourceDetails, s_PackagesJson_Source_Type, sourceItr->Details.Type);
            ValidateJsonStringProperty(jsonSourceDetails, s_PackagesJson_Source_Identifier, sourceItr->Details.Identifier);

            const auto& jsonPackages = GetAndValidateJsonProperty(*jsonSourceItr, s_PackagesJson_Packages, Json::ValueType::arrayValue);
            REQUIRE(jsonPackages.size() == sourceItr->Packages.size());

            auto jsonPackageItr = jsonPackages.begin();
            auto packageItr = sourceItr->Packages.begin();
            for (; jsonPackageItr != jsonPackages.end(); ++jsonPackageItr, ++packageItr)
            {
                REQUIRE(jsonPackageItr->isObject());
                ValidateJsonStringProperty(*jsonPackageItr, s_PackagesJson_Package_PackageIdentifier, packageItr->Id);
                ValidateJsonStringProperty(*jsonPackageItr, s_PackagesJson_Package_Version, packageItr->VersionAndChannel.GetVersion().ToString(), true);
                ValidateJsonStringProperty(*jsonPackageItr, s_PackagesJson_Package_Channel, packageItr->VersionAndChannel.GetChannel().ToString(), true);
            }
        }
    }

    void ValidateEqualCollections(const PackageCollection& first, const PackageCollection& second)
    {
        REQUIRE(first.ClientVersion == second.ClientVersion);
        REQUIRE(first.Sources.size() == second.Sources.size());

        auto firstSourceItr = first.Sources.begin();
        auto secondSourceItr = second.Sources.begin();
        for (; firstSourceItr != first.Sources.end(); ++firstSourceItr, ++secondSourceItr)
        {
            REQUIRE(firstSourceItr->Details.Name == secondSourceItr->Details.Name);
            REQUIRE(firstSourceItr->Details.Arg == secondSourceItr->Details.Arg);
            REQUIRE(firstSourceItr->Details.Type == secondSourceItr->Details.Type);
            REQUIRE(firstSourceItr->Details.Identifier == secondSourceItr->Details.Identifier);

            REQUIRE(firstSourceItr->Packages.size() == secondSourceItr->Packages.size());
            auto firstPackageItr = firstSourceItr->Packages.begin();
            auto secondPackageItr = secondSourceItr->Packages.begin();
            for (; firstPackageItr != firstSourceItr->Packages.end(); ++firstPackageItr, ++secondPackageItr)
            {
                REQUIRE(firstPackageItr->Id == secondPackageItr->Id);
                REQUIRE(firstPackageItr->VersionAndChannel.ToString() == secondPackageItr->VersionAndChannel.ToString());
            }
        }
    }
}

TEST_CASE("PackageCollection_Write_SingleSource", "[PackageCollection]")
{
    PackageCollection::Source source;
    source.Details.Name = "TestSource";
    source.Details.Arg = "https://aka.ms/winget";
    source.Details.Type = "Microsoft.PreIndexed.Package";
    source.Details.Identifier = "TestSourceId";

    source.Packages.emplace_back(LocIndString{ "test.package1"sv }, Version{ "1.0.1" }, Channel{ "" });
    source.Packages.emplace_back(LocIndString{ "test.package2"sv }, Version{ "2" }, Channel{ "Public" });

    PackageCollection pc
    {
        "0.1.2.3",
        std::vector<PackageCollection::Source>{ source }
    };

    ValidateJsonWithCollection(PackagesJson::CreateJson(pc), pc);
}

TEST_CASE("PackageCollection_Write_MultipleSources", "[PackageCollection]")
{
    PackageCollection::Source source1;
    source1.Details.Name = "TestSource";
    source1.Details.Arg = "https://aka.ms/winget";
    source1.Details.Type = "Microsoft.PreIndexed.Package";
    source1.Details.Identifier = "TestSourceId";
    source1.Packages.emplace_back(LocIndString{ "test.package1"sv }, Version{ "1.0.1" }, Channel{ "" });

    PackageCollection::Source source2;
    source2.Details.Name = "TestSource2";
    source2.Details.Arg = "https://aka.ms/winget";
    source2.Details.Type = "*Test";
    source2.Details.Identifier = "SecondId";
    source2.Packages.emplace_back(LocIndString{ "test.package2"sv }, Version{ "2.1.0" }, Channel{ "Beta" });

    PackageCollection pc
    {
        "1.0.0.0",
        std::vector<PackageCollection::Source>{ source1, source2 }
    };

    ValidateJsonWithCollection(PackagesJson::CreateJson(pc), pc);
}

TEST_CASE("PackageCollection_Read_SingleSource_1_0", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "Id": "test.WithVersion",
              "Version": "0.1",
              "Channel": "Preview"
            },
            {
              "Id": "test.NoVersion"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());

    PackageCollection::Source source;
    source.Details.Name = "TestSource";
    source.Details.Arg = "https://aka.ms/winget";
    source.Details.Type = "Microsoft.PreIndexed.Package";
    source.Details.Identifier = "TestSourceId";

    source.Packages.emplace_back(LocIndString{ "test.WithVersion"sv }, Version{ "0.1" }, Channel{ "Preview" });
    source.Packages.emplace_back(LocIndString{ "test.NoVersion"sv }, Version{ "" }, Channel{ "" });

    PackageCollection expected
    {
        "1.0.0",
        std::vector<PackageCollection::Source>{ source }
    };

    ValidateEqualCollections(parseResult.Packages, expected);
}

TEST_CASE("PackageCollection_Read_SingleSource_2_0", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.2.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "PackageIdentifier": "test.WithVersion",
              "Version": "0.1",
              "Channel": "Preview"
            },
            {
              "PackageIdentifier": "test.NoVersion"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ]
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());

    PackageCollection::Source source;
    source.Details.Name = "TestSource";
    source.Details.Arg = "https://aka.ms/winget";
    source.Details.Type = "Microsoft.PreIndexed.Package";
    source.Details.Identifier = "TestSourceId";

    source.Packages.emplace_back(LocIndString{ "test.WithVersion"sv }, Version{ "0.1" }, Channel{ "Preview" });
    source.Packages.emplace_back(LocIndString{ "test.NoVersion"sv }, Version{ "" }, Channel{ "" });

    PackageCollection expected
    {
        "",
        std::vector<PackageCollection::Source>{ source }
    };

    ValidateEqualCollections(parseResult.Packages, expected);
}

TEST_CASE("PackageCollection_Read_MultipleSources_1_0", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "WinGetVersion": "1.0.0",
      "Sources": [
        {
          "SourceDetails": {
            "Argument": "//firstSource",
            "Identifier": "Id1",
            "Name": "First",
            "Type": "Microsoft.PreIndexed.Package"
          },
          "Packages": [
            {
              "Id": "test"
            }
          ]
        },
        {
          "SourceDetails": {
            "Argument": "//secondSource",
            "Identifier": "Id2",
            "Name": "Second",
            "Type": "*TestSource"
          },
          "Packages": [
            {
              "Id": "test2",
              "Version": "1.0"
            }
          ]
        }
      ]
    })");


    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());

    PackageCollection::Source source1;
    source1.Details.Name = "First";
    source1.Details.Arg = "//firstSource";
    source1.Details.Type = "Microsoft.PreIndexed.Package";
    source1.Details.Identifier = "Id1";
    source1.Packages.emplace_back(LocIndString{ "test"sv }, Version{ "" }, Channel{ "" });

    PackageCollection::Source source2;
    source2.Details.Name = "Second";
    source2.Details.Arg = "//secondSource";
    source2.Details.Type = "*TestSource";
    source2.Details.Identifier = "Id2";
    source2.Packages.emplace_back(LocIndString{ "test2"sv }, Version{ "1.0" }, Channel{ "" });

    PackageCollection expected
    {
        "1.0.0",
        std::vector<PackageCollection::Source>{ source1, source2 }
    };

    ValidateEqualCollections(parseResult.Packages, expected);
}

TEST_CASE("PackageCollection_Read_MultipleSources_2_0", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.2.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "WinGetVersion": "1.0.0",
      "Sources": [
        {
          "SourceDetails": {
            "Argument": "//firstSource",
            "Identifier": "Id1",
            "Name": "First",
            "Type": "Microsoft.PreIndexed.Package"
          },
          "Packages": [
            {
              "PackageIdentifier": "test"
            }
          ]
        },
        {
          "SourceDetails": {
            "Argument": "//secondSource",
            "Identifier": "Id2",
            "Name": "Second",
            "Type": "*TestSource"
          },
          "Packages": [
            {
              "PackageIdentifier": "test2",
              "Version": "1.0"
            }
          ]
        }
      ]
    })");


    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());

    PackageCollection::Source source1;
    source1.Details.Name = "First";
    source1.Details.Arg = "//firstSource";
    source1.Details.Type = "Microsoft.PreIndexed.Package";
    source1.Details.Identifier = "Id1";
    source1.Packages.emplace_back(LocIndString{ "test"sv }, Version{ "" }, Channel{ "" });

    PackageCollection::Source source2;
    source2.Details.Name = "Second";
    source2.Details.Arg = "//secondSource";
    source2.Details.Type = "*TestSource";
    source2.Details.Identifier = "Id2";
    source2.Packages.emplace_back(LocIndString{ "test2"sv }, Version{ "1.0" }, Channel{ "" });

    PackageCollection expected
    {
        "1.0.0",
        std::vector<PackageCollection::Source>{ source1, source2 }
    };

    ValidateEqualCollections(parseResult.Packages, expected);
}

TEST_CASE("PackageCollection_Read_RepeatedSource", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "WinGetVersion": "1.0.0",
      "Sources": [
        {
          "SourceDetails": {
            "Argument": "//firstSource",
            "Identifier": "Id1",
            "Name": "First",
            "Type": "Microsoft.PreIndexed.Package"
          },
          "Packages": [
            {
              "Id": "test"
            }
          ]
        },
        {
          "SourceDetails": {
            "Argument": "//secondSource",
            "Identifier": "Id2",
            "Name": "Second",
            "Type": "*TestSource"
          },
          "Packages": [
            {
              "Id": "test2",
              "Version": "1.0"
            }
          ]
        },
        {
          "SourceDetails": {
            "Argument": "//secondSource",
            "Identifier": "Id2",
            "Name": "Second",
            "Type": "*TestSource"
          },
          "Packages": [
            {
              "Id": "test3",
              "Version": "1.2"
            }
          ]
        }
      ]
    })");


    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());

    PackageCollection::Source source1;
    source1.Details.Name = "First";
    source1.Details.Arg = "//firstSource";
    source1.Details.Type = "Microsoft.PreIndexed.Package";
    source1.Details.Identifier = "Id1";
    source1.Packages.emplace_back(LocIndString{ "test"sv }, Version{ "" }, Channel{ "" });

    PackageCollection::Source source2;
    source2.Details.Name = "Second";
    source2.Details.Arg = "//secondSource";
    source2.Details.Type = "*TestSource";
    source2.Details.Identifier = "Id2";
    source2.Packages.emplace_back(LocIndString{ "test2"sv }, Version{ "1.0" }, Channel{ "" });
    source2.Packages.emplace_back(LocIndString{ "test3"sv }, Version{ "1.2" }, Channel{ "" });

    PackageCollection expected
    {
        "1.0.0",
        std::vector<PackageCollection::Source>{ source1, source2 }
    };

    ValidateEqualCollections(parseResult.Packages, expected);
}

TEST_CASE("PackageCollection_Read_MissingSchema", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "CreationDate": "2021-01-01T12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "Id": "test.test"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::MissingSchema);

    json = ParseJsonString("\"Not even a JSON object\"");

    parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::MissingSchema);
}

TEST_CASE("PackageCollection_Read_WrongSchema", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-settings.schema.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "Id": "test.test"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::UnrecognizedSchema);
}

TEST_CASE("PackageCollection_Read_SchemaValidationFail", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "NotSources": [
        {
          "Packages": [
            {
              "Id": "test.test"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    INFO(parseResult.Errors);

    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::SchemaValidationFailed);
    REQUIRE(parseResult.Errors.find("Missing required property 'Sources'.") != std::string::npos);
}

TEST_CASE("PackageCollection_Read_SchemaValidationFail_Id", "[PackageCollection]")
{
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021-01-01T12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "NotId": "test.test"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    INFO(parseResult.Errors);

    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::SchemaValidationFailed);
    REQUIRE(parseResult.Errors.find("Missing required property 'Id'.") != std::string::npos);
}

TEST_CASE("PackageCollection_Read_BadTimeStamp", "[PackageCollection]")
{
    // We used to export without padding the creation date with 0s nor adding time zone.
    // Ensure we don't break with that format.
    auto json = ParseJsonString(R"(
    {
      "$schema": "https://aka.ms/winget-packages.schema.1.0.json",
      "CreationDate": "2021- 1- 1 12:00:00.000",
      "Sources": [
        {
          "Packages": [
            {
              "Id": "test"
            }
          ],
          "SourceDetails": {
            "Argument": "https://aka.ms/winget",
            "Identifier": "TestSourceId",
            "Name": "TestSource",
            "Type": "Microsoft.PreIndexed.Package"
          }
        }
      ],
      "WinGetVersion": "1.0.0"
    })");

    auto parseResult = PackagesJson::TryParseJson(json);
    INFO(parseResult.Errors);

    REQUIRE(parseResult.Result == PackagesJson::ParseResult::Type::Success);
    REQUIRE(parseResult.Errors.empty());
}
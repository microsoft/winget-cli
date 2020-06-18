// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <json.h>
#include <winget/settings/Source.h>

using namespace AppInstaller::Settings;

/* Source property example
    "source": {
        "autoUpdateIntervalInMinutes": -1
    },
*/

namespace
{
    template<class T>
    void TestAutoUpdateIntervalInMinutes(const T& value, uint32_t expected, size_t warnings)
    {
        Json::Value property;
        property["autoUpdateIntervalInMinutes"] = value;
        Source source(property);
        REQUIRE(source.Warnings().size() == warnings);
        REQUIRE(source.GetAutoUpdateTimeInMinutes() == expected);
    }
}

TEST_CASE("SourceSetting", "[settings]")
{
    Source source(Json::Value::nullSingleton());
    REQUIRE(source.GetPropertyName() == "source");
}

TEST_CASE("SourceSettingAutoUpdateIntervalInMinutes", "[settings]")
{
    const std::string progressBar = "autoUpdateIntervalInMinutes";

    Source sourceNull(Json::Value::nullSingleton());
    REQUIRE(sourceNull.Warnings().size() == 0);
    REQUIRE(sourceNull.GetAutoUpdateTimeInMinutes() == 5);

    TestAutoUpdateIntervalInMinutes(300, 300, 0);

    TestAutoUpdateIntervalInMinutes(-20, 5, 1);

    TestAutoUpdateIntervalInMinutes("not a number", 5, 1);

    Json::Value otherProperty;
    otherProperty["other"] = "property";
    Source sourceOther(otherProperty);
    REQUIRE(sourceOther.Warnings().size() == 0);
    REQUIRE(sourceOther.GetAutoUpdateTimeInMinutes() == 5);
}

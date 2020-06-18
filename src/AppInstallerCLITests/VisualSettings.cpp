// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <json.h>
#include <winget/settings/Visual.h>

using namespace AppInstaller::Settings;

/* Visual property example
    "visual": {
        "progressBar": "accent"
    }
*/

namespace
{
    template<class T>
    void TestProgressBar(T value, VisualStyle expected, int warnings)
    {
        Json::Value property;
        property["progressBar"] = value;
        Visual visual(property);
        REQUIRE(visual.Warnings().size() == warnings);
        REQUIRE(visual.GetProgressBarVisualStyle() == expected);
    }
}

TEST_CASE("VisualSetting", "[settings]")
{
    Visual visual(Json::Value::nullSingleton());
    REQUIRE(visual.GetPropertyName() == "visual");
}

TEST_CASE("VisualSettingProgressBar", "[settings]")
{
    Visual visualNull(Json::Value::nullSingleton());
    REQUIRE(visualNull.Warnings().size() == 0);
    REQUIRE(visualNull.GetProgressBarVisualStyle() == VisualStyle::Accent);

    TestProgressBar("accent", VisualStyle::Accent, 0);

    TestProgressBar("rainbow", VisualStyle::Rainbow, 0);

    TestProgressBar("retro", VisualStyle::Retro, 0);

    TestProgressBar("fake", VisualStyle::Accent, 1);

    TestProgressBar(5, VisualStyle::Accent, 1);

    Json::Value otherProperty;
    otherProperty["other"] = "property";
    Visual visualOther(otherProperty);
    REQUIRE(visualOther.Warnings().size() == 0);
    REQUIRE(visualOther.GetProgressBarVisualStyle() == VisualStyle::Accent);
}

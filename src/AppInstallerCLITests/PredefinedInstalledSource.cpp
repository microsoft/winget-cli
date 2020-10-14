// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <Microsoft/PredefinedInstalledSourceFactory.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Utility;

using Factory = AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory;

std::shared_ptr<ISource> CreatePredefinedInstalledSource(Factory::Filter filter = Factory::Filter::None)
{
    SourceDetails details;
    details.Type = Factory::Type();
    details.Arg = Factory::FilterToString(filter);

    TestProgress progress;

    auto factory = Factory::Create();
    return factory->Create(details, progress);
}

TEST_CASE("PredefinedInstalledSource_Create", "[installed][list]")
{
    auto source = CreatePredefinedInstalledSource();
}

TEST_CASE("PredefinedInstalledSource_Search", "[installed][list]")
{
    auto source = CreatePredefinedInstalledSource();

    SearchRequest request;

    auto results = source->Search(request);

    REQUIRE(!results.Matches.empty());
}

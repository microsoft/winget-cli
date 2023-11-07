// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "Microsoft/ARPHelper.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;


TEST_CASE("ARPHelper_Watcher", "[ARPHelper]")
{
    ARPHelper helper;

    wil::unique_event callbackEvent;
    callbackEvent.create();

    ScopeEnum scopeCallback = ScopeEnum::Unknown;
    Architecture architectureCallback = Architecture::Unknown;

    ScopeEnum scopeTarget = ScopeEnum::Machine;
    Architecture architectureTarget = Architecture::X86;

    auto fakeRoot = TestCommon::RegCreateVolatileTestRoot();
    TestHook::SetGetARPKey_Override arpOverride([&](ScopeEnum scope, Architecture arch)
        {
            if (scope == scopeTarget && arch == architectureTarget)
            {
                return Key(fakeRoot.get(), L"");
            }
            else
            {
                return Key{};
            }
        });

    auto watchers = helper.CreateRegistryWatchers(scopeTarget, [&](ScopeEnum scope, Architecture arch, wil::RegistryChangeKind)
        {
            scopeCallback = scope;
            architectureCallback = arch;
            callbackEvent.SetEvent();
        });

    auto arpKey = helper.GetARPKey(scopeTarget, architectureTarget);
    REQUIRE(!!arpKey);

    GUID guid;
    std::ignore = CoCreateGuid(&guid);
    std::ostringstream stream;
    stream << guid;

    auto testKey = TestCommon::RegCreateVolatileSubKey(arpKey, ConvertToUTF16(stream.str()));

    REQUIRE(callbackEvent.wait(1000));
    REQUIRE(scopeTarget == scopeCallback);
    REQUIRE(architectureTarget == architectureCallback);

    // Reset for changing a value
    scopeCallback = ScopeEnum::Unknown;
    architectureCallback = Architecture::Unknown;

    TestCommon::SetRegistryValue(testKey.get(), L"testValue", L"valueValue");

    REQUIRE(callbackEvent.wait(1000));
    REQUIRE(scopeTarget == scopeCallback);
    REQUIRE(architectureTarget == architectureCallback);
}

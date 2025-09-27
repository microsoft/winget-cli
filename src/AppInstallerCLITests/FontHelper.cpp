// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "AppInstallerStrings.h"
#include "Microsoft/FontHelper.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;


TEST_CASE("FontHelper_Watcher", "[FontHelper]")
{
    FontHelper helper;

    wil::unique_event callbackEvent;
    callbackEvent.create();

    ScopeEnum scopeCallback = ScopeEnum::Unknown;
    ScopeEnum scopeTarget = ScopeEnum::Machine;

    auto fakeRoot = TestCommon::RegCreateVolatileTestRoot();
    TestHook::SetGetFontRegistryRoot_Override fontRootOverride([&](ScopeEnum scope)
        {
            if (scope == scopeTarget)
            {
                return Key(fakeRoot.get(), L"");
            }
            else
            {
                return Key{};
            }
        });

    auto watchers = std::vector<wil::unique_registry_watcher>();
    helper.AddRegistryWatchers(scopeTarget, [&](ScopeEnum scope, wil::RegistryChangeKind)
        {
            scopeCallback = scope;
            callbackEvent.SetEvent();
        }, watchers);

    GUID guid;
    std::ignore = CoCreateGuid(&guid);
    std::ostringstream stream;
    stream << guid;

    auto testKey = TestCommon::RegCreateVolatileSubKey(fakeRoot.get(), ConvertToUTF16(stream.str()));
    REQUIRE(callbackEvent.wait(1000));
    REQUIRE(scopeTarget == scopeCallback);

    // Reset for changing a value
    scopeCallback = ScopeEnum::Unknown;
    TestCommon::SetRegistryValue(testKey.get(), L"testValue", L"valueValue");

    REQUIRE(callbackEvent.wait(1000));
    REQUIRE(scopeTarget == scopeCallback);
}

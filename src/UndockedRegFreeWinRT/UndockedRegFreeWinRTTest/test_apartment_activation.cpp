#include "pch.h"
#include <iostream>

#include <Windows.h>
#include <roapi.h>
#include <rometadataresolution.h>
#include <windows.foundation.h>
#include <stdio.h>
#include <wrl\client.h>
#include <wrl\wrappers\corewrappers.h>
#include "winrt\TestComponent.h"

using namespace Microsoft::WRL;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL::Wrappers;

TEST_CASE("Undocked Regfree WinRT Activation")
{
    SECTION("Both To Current STA")
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        winrt::TestComponent::ClassBoth c;
        REQUIRE(c.Apartment() == APTTYPE_MAINSTA);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
    SECTION("Both To Current MTA")
    {
        winrt::init_apartment();
        winrt::TestComponent::ClassBoth c;
        REQUIRE(c.Apartment() == APTTYPE_MTA);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
    SECTION("Cross Apartment MTA Activation")
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        winrt::TestComponent::ClassMta c;
        REQUIRE(c.Apartment() == APTTYPE_MTA);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
    SECTION("BLOCK STA To Current MTA")
    {
        winrt::init_apartment();
        winrt::TestComponent::ClassSta c;
        REQUIRE(RoActivateInstance(HStringReference(L"TestComponent.ClassSta").Get(), (IInspectable**)winrt::put_abi(c)) == RO_E_UNSUPPORTED_FROM_MTA);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
    SECTION("Test Get Metadata File on Type")
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        HString result;
        REQUIRE(RoGetMetaDataFile(HStringReference(L"TestComponent.ClassSta").Get(), nullptr, result.GetAddressOf(), nullptr, nullptr) == S_OK);
        REQUIRE(wcsstr(WindowsGetStringRawBuffer(result.Get(), 0), L"TestComponent.winmd") != nullptr);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
    SECTION("Test Get Metadata File on Namespace")
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        HString result;
        REQUIRE(RoGetMetaDataFile(HStringReference(L"TestComponent").Get(), nullptr, result.GetAddressOf(), nullptr, nullptr) == RO_E_METADATA_NAME_IS_NAMESPACE);
        winrt::clear_factory_cache();
        winrt::uninit_apartment();
    }
}

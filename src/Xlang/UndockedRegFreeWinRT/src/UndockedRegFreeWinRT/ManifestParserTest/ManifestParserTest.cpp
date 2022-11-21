#include <iostream>
#include "..\UndockedRegFreeWinRT\catalog.h"
#include "..\UndockedRegFreeWinRT\catalog.cpp"
#include "..\UndockedRegFreeWinRT\typeresolution.h"
#include "..\UndockedRegFreeWinRT\typeresolution.cpp"
#include "pch.h"

TEST_CASE("Manifest Parser")
{
    g_types.clear();
    PCWSTR exeFilePath = nullptr;
    UndockedRegFreeWinRT::GetProcessExeDir(&exeFilePath);
    SECTION("Multiple activatableClass")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\winrtActivation.dll1.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == S_OK);
        std::wstring moduleName = L"RegFreeWinrtInprocComponents.1.dll";
        REQUIRE(g_types.size() == 4);
        auto component_iter = g_types.find(L"RegFreeWinrt.1.threading_both");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);
        REQUIRE(component_iter->second->threading_model == ABI::Windows::Foundation::ThreadingType::ThreadingType_BOTH);

        component_iter = g_types.find(L"RegFreeWinrt.1.threading_mta");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);
        REQUIRE(component_iter->second->threading_model == ABI::Windows::Foundation::ThreadingType::ThreadingType_MTA);

        component_iter = g_types.find(L"RegFreeWinrt.1.threading_sta");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);
        REQUIRE(component_iter->second->threading_model == ABI::Windows::Foundation::ThreadingType::ThreadingType_STA);

        component_iter = g_types.find(L"RegFreeWinrt.SharedName.threading_both");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);
        REQUIRE(component_iter->second->threading_model == ABI::Windows::Foundation::ThreadingType::ThreadingType_BOTH);
    }

    SECTION("Multiple file")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.Positive.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == S_OK);
        std::wstring moduleName = L"FakeDllName.dll";
        REQUIRE(g_types.size() == 6);
        auto component_iter = g_types.find(L"FakeActivatableClass");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);

        component_iter = g_types.find(L"FakeActivatableClass2");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);

        component_iter = g_types.find(L"FakeActivatableClass3");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);

        component_iter = g_types.find(L"FakeActivatableClass4");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);

        moduleName = L"OtherFakeDllName.dll";
        component_iter = g_types.find(L"OtherFakeActivatableClass");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);

        component_iter = g_types.find(L"OtherFakeActivatableClass2");
        REQUIRE(component_iter != g_types.end());
        REQUIRE(component_iter->second->module_name == moduleName);
    }

    //Failures
    SECTION("Negative Missing ThreadingModel")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.missingThreadingModel.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }

    SECTION("Negative Missing Name")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.missingName.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }

    SECTION("Negative Invalid ThreadingModel")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.invalidThreadingModel.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }
    
    SECTION("Negative Duplicate Acid")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.duplicateAcid.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_DUPLICATE_ACTIVATABLE_CLASS));
    }

    SECTION("Negative Blank Name")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.blankName.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }

    SECTION("Negative Blank FileName")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.blankFileName.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }

    SECTION("Negative Missing FileName")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.negative.missingFileName.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR));
    }

    SECTION("Large file")
    {
        std::wstring manifest = std::wstring(exeFilePath) + L"\\basicParse.largefile.manifest";
        REQUIRE(LoadManifestFromPath(manifest.c_str()) == S_OK);

        // We had a bug where we were using a variable after an XML buffer had been invalidated by another call
        // to Read(), but it didn't manifest until the node count was high enough.  The result was that the values
        // we got for "module_name" were pointing to the wrong string data, so here we validate that the entries
        // do actually look like DLLs -- they end with ".dll", they are short, and they don't contain spaces.
        for (auto type : g_types)
        {
            const std::wstring& name = type.second->module_name;
            
            REQUIRE(wcscmp(L".dll", name.substr(name.size() - 4).c_str()) == 0);
            REQUIRE(name.size() < 50);
            REQUIRE(name.find(' ') == wstring::npos);

        }

    }
}

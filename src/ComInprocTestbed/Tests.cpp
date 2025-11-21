// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Tests.h"

using namespace std::string_view_literals;

BOOL CALLBACK CheckForWinGetWindow(HWND hwnd, LPARAM param)
{
    bool* result = reinterpret_cast<bool*>(param);

    int textLength = GetWindowTextLengthW(hwnd) + 1;
    std::wstring windowText(textLength, '\0');
    textLength = GetWindowTextW(hwnd, &windowText[0], textLength);
    windowText.resize(textLength);

    if (L"WingetMessageOnlyWindow"sv == windowText)
    {
        *result = true;
        return FALSE;
    }

    return TRUE;
}

// Look for the set of well known objects that should be present after we have spun everything up.
// Returns true if all well known objects are found in the expected state.
bool SearchForWellKnownObjects(bool expectExist)
{
    bool result = true;

    auto coreApplicationProperties = winrt::Windows::ApplicationModel::Core::CoreApplication::Properties();

    // COM statics
    for (std::wstring_view item : {
        L"WindowsPackageManager.CachedInstalledIndex"sv,
        L"WindowsPackageManager.TerminationSignalHandler"sv,
    })
    {
        bool present = coreApplicationProperties.HasKey(item);

        if (present != expectExist)
        {
            std::cout << "CoreApplication property `" << winrt::to_string(item) << "` was not in expected state [" << (expectExist ? "should exist" : "should not exist") << "]\n";
            result = false;
        }
    }

    // Shutdown monitoring window
    bool foundWindow = false;
    EnumWindows(CheckForWinGetWindow, reinterpret_cast<LPARAM>(&foundWindow));

    if (foundWindow != expectExist)
    {
        std::cout << "WinGet Window `WingetMessageOnlyWindow` was not in expected state [" << (expectExist ? "should exist" : "should not exist") << "]\n";
        result = false;
    }

    return result;
}

Snapshot::Snapshot()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE, GetCurrentProcessId());

    // Count threads in this process
    // TODO

    // Count modules
    // TODO

    // Get memory stats
    // TODO

    CloseHandle(snapshot);
}

// TODO: Move this to be object based and have it just hold all of the snapshots
std::pair<bool, std::optional<Snapshot>> UnloadAndCheckForLeaks(std::optional<Snapshot> previousSnapshot, bool expectUnload)
{
    if (!SearchForWellKnownObjects(true))
    {
        return { false , std::nullopt };
    }

    Snapshot loadedSnapshot;

    CoFreeUnusedLibrariesEx(0, 0);

    Snapshot afterFreeSnapshot;

    if (!SearchForWellKnownObjects(!expectUnload))
    {
        return { false , afterFreeSnapshot };
    }

    if (!CompareSnapshots(previousSnapshot, loadedSnapshot, afterFreeSnapshot))
    {
        return { false , afterFreeSnapshot };
    }

    return { true, afterFreeSnapshot };
}

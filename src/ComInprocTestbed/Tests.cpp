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
    const DWORD processId = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE, processId);

    // Count threads in this process
    THREADENTRY32 threadEntry{};
    threadEntry.dwSize = sizeof(threadEntry);

    if (Thread32First(snapshot, &threadEntry))
    {
        do
        {
            if (processId == threadEntry.th32OwnerProcessID)
            {
                ++ThreadCount;
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }

    // Count modules
    MODULEENTRY32 moduleEntry{};
    moduleEntry.dwSize = sizeof(moduleEntry);

    if (Module32First(snapshot, &moduleEntry))
    {
        do
        {
            ++ModuleCount;
        } while (Module32Next(snapshot, &moduleEntry));
    }

    // Get memory stats
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&Memory), sizeof(Memory));

    CloseHandle(snapshot);
}

UnloadAndCheckForLeaks::UnloadAndCheckForLeaks(bool shouldUnload) : m_shouldUnload(shouldUnload)
{
}

bool UnloadAndCheckForLeaks::RunIteration()
{
    if (!SearchForWellKnownObjects(true))
    {
        return false;
    }

    Snapshot beforeUnload;

    CoFreeUnusedLibrariesEx(0, 0);

    m_iterationSnapshots.emplace_back(beforeUnload, Snapshot{});

    if (!SearchForWellKnownObjects(!m_shouldUnload))
    {
        return false;
    }

    return true;
}

bool UnloadAndCheckForLeaks::RunFinal()
{
    constexpr std::streamsize s_columnWidth = 4;

    bool result = true;

    std::cout << "--- UnloadAndCheckForLeaks results ---\n";
    std::cout << std::setfill(' ');

    // Threads
    std::cout << "Thread Count [Initial: " << m_initialSnapshot.ThreadCount << "]\n";

    std::cout << "Iteration  ";
    for (size_t i = 0; i < m_iterationSnapshots.size(); ++i)
    {
        std::cout << std::setw(s_columnWidth) << (i + 1);
    }
    std::cout << '\n';

    std::cout << "Pre Unload ";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << snapshot.first.ThreadCount;
    }
    std::cout << '\n';

    std::cout << "Post Unload";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << snapshot.second.ThreadCount;
    }
    std::cout << '\n';

    // Modules

    // Memory

    return result;
}

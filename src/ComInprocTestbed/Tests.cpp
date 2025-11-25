// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Tests.h"
#include "PackageManager.h"

using namespace std::string_view_literals;

namespace
{
#define ADVANCE_ARG_PARAMETER if (++i >= argc) { winrt::throw_hresult(E_INVALIDARG); }

    std::string ToLower(std::string_view in)
    {
        std::string result(in);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

#define BEGIN_ENUM_PARSE_FUNC(_type_) \
    _type_ Parse ## _type_(const char* input) \
    { \
        auto lower = ToLower(input); \
        if (lower.empty()) {}

#define ITEM_ENUM_PARSE_FUNC(_type_, _value_) \
        else if (ToLower(#_value_) == lower) \
        { \
            return _type_ ## :: ## _value_; \
        }

#define END_ENUM_PARSE_FUNC \
        winrt::throw_hresult(E_INVALIDARG); \
    }

#define BEGIN_ENUM_NAME_FUNC(_type_) \
    std::string_view ToString(_type_); \
    std::ostream& operator<<(std::ostream& o, _type_ input) { return (o << ToString(input)); } \
    std::string_view ToString(_type_ input) \
    { \
        switch (input) {

#define ITEM_ENUM_NAME_FUNC(_type_, _value_) \
        case _type_ ## :: ## _value_: return #_value_;

#define END_ENUM_NAME_FUNC \
        } \
        return "Unknown"; \
    }

    BEGIN_ENUM_PARSE_FUNC(ComInitializationType)
        ITEM_ENUM_PARSE_FUNC(ComInitializationType, STA)
        ITEM_ENUM_PARSE_FUNC(ComInitializationType, MTA)
    END_ENUM_PARSE_FUNC

    BEGIN_ENUM_NAME_FUNC(ComInitializationType)
        ITEM_ENUM_NAME_FUNC(ComInitializationType, STA)
        ITEM_ENUM_NAME_FUNC(ComInitializationType, MTA)
    END_ENUM_NAME_FUNC

    BEGIN_ENUM_PARSE_FUNC(UnloadBehavior)
        ITEM_ENUM_PARSE_FUNC(UnloadBehavior, Allow)
        ITEM_ENUM_PARSE_FUNC(UnloadBehavior, AtExit)
        ITEM_ENUM_PARSE_FUNC(UnloadBehavior, Never)
    END_ENUM_PARSE_FUNC

    BEGIN_ENUM_NAME_FUNC(UnloadBehavior)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, Allow)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, AtExit)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, Never)
    END_ENUM_NAME_FUNC

    BEGIN_ENUM_PARSE_FUNC(ActivationType)
        ITEM_ENUM_PARSE_FUNC(ActivationType, ClassName)
        ITEM_ENUM_PARSE_FUNC(ActivationType, CLSID_WinRT)
        ITEM_ENUM_PARSE_FUNC(ActivationType, CLSID_CoCreateInstance)
    END_ENUM_PARSE_FUNC

    BEGIN_ENUM_NAME_FUNC(ActivationType)
        ITEM_ENUM_NAME_FUNC(ActivationType, ClassName)
        ITEM_ENUM_NAME_FUNC(ActivationType, CLSID_WinRT)
        ITEM_ENUM_NAME_FUNC(ActivationType, CLSID_CoCreateInstance)
    END_ENUM_NAME_FUNC

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
}

TestParameters::TestParameters(int argc, const char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        if ("-test"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            TestToRun = ToLower(argv[i]);
        }
        else if ("-com"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            ComInit = ParseComInitializationType(argv[i]);
        }
        else if ("-leak-com"sv == argv[i])
        {
            LeakCOM = true;
        }
        else if ("-itr"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            Iterations = atoi(argv[i]);
        }
        else if ("-pkg"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            PackageName = argv[i];
        }
        else if ("-unload"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            UnloadBehavior = ParseUnloadBehavior(argv[i]);
        }
        else if ("-activation"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            ActivationType = ParseActivationType(argv[i]);
        }
        else if ("-clear-factories"sv == argv[i])
        {
            ClearFactories = true;
        }
    }
}

void TestParameters::OutputDetails() const
{
    std::cout << "Running inproc testbed with:\n"
        "  COM Init : " << ComInit << "\n"
        "  Activate : " << ActivationType << "\n"
        "  Clear    : " << std::boolalpha << ClearFactories << "\n"
        "  Leak COM : " << std::boolalpha << LeakCOM << "\n"
        "  Unload   : " << UnloadBehavior << "\n"
        "  Test     : " << TestToRun << "\n"
        "  Package  : " << PackageName << "\n"
        "  Passes   : " << Iterations << std::endl;
}

bool TestParameters::InitializeTestState() const
{
    HRESULT hr = S_OK;

    if (ComInitializationType::STA == ComInit)
    {
        hr = RoInitialize(RO_INIT_SINGLETHREADED);
    }
    else if (ComInitializationType::MTA == ComInit)
    {
        hr = RoInitialize(RO_INIT_MULTITHREADED);
    }

    if (FAILED(hr))
    {
        std::cout << "RoInitialize returned " << hr << std::endl;
        return false;
    }

    if (UnloadBehavior::Never == UnloadBehavior || UnloadBehavior::AtExit == UnloadBehavior)
    {
        SetUnloadPreference(false);
    }

    return true;
}

std::unique_ptr<ITest> TestParameters::CreateTest() const
{
    if ("unload_check"sv == TestToRun)
    {
        return std::make_unique<UnloadAndCheckForLeaks>(*this);
    }

    return {};
}

void TestParameters::UninitializeTestState() const
{
    if (!LeakCOM)
    {
        RoUninitialize();
    }

    if (UnloadBehavior::AtExit == UnloadBehavior)
    {
        SetUnloadPreference(true);
    }
}

bool TestParameters::UnloadExpected() const
{
    bool shouldUnload = true;
    if (UnloadBehavior::Never == UnloadBehavior || UnloadBehavior::AtExit == UnloadBehavior)
    {
        shouldUnload = false;
    }
    return shouldUnload;
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

UnloadAndCheckForLeaks::UnloadAndCheckForLeaks(const TestParameters& parameters) : m_parameters(parameters)
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

    if (!SearchForWellKnownObjects(!m_parameters.UnloadExpected()))
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

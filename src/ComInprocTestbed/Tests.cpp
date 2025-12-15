// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Tests.h"
#include "PackageManager.h"

using namespace std::string_view_literals;
using namespace winrt::Microsoft::Management::Deployment;

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
        ITEM_ENUM_PARSE_FUNC(UnloadBehavior, AtUninitialize)
        ITEM_ENUM_PARSE_FUNC(UnloadBehavior, Never)
    END_ENUM_PARSE_FUNC

    BEGIN_ENUM_NAME_FUNC(UnloadBehavior)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, Allow)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, AtUninitialize)
        ITEM_ENUM_NAME_FUNC(UnloadBehavior, Never)
    END_ENUM_NAME_FUNC

    BEGIN_ENUM_PARSE_FUNC(ActivationType)
        ITEM_ENUM_PARSE_FUNC(ActivationType, ClassName)
        ITEM_ENUM_PARSE_FUNC(ActivationType, CoCreateInstance)
    END_ENUM_PARSE_FUNC

    BEGIN_ENUM_NAME_FUNC(ActivationType)
        ITEM_ENUM_NAME_FUNC(ActivationType, ClassName)
        ITEM_ENUM_NAME_FUNC(ActivationType, CoCreateInstance)
    END_ENUM_NAME_FUNC

    BOOL CALLBACK CheckForWinGetWindow(HWND hwnd, LPARAM param)
    {
        bool* result = reinterpret_cast<bool*>(param);

        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (GetCurrentProcessId() == windowProcessId)
        {
            int textLength = GetWindowTextLengthW(hwnd) + 1;
            std::wstring windowText(textLength, '\0');
            textLength = GetWindowTextW(hwnd, &windowText[0], textLength);
            windowText.resize(textLength);

            if (L"WingetMessageOnlyWindow"sv == windowText)
            {
                *result = true;
                return FALSE;
            }
        }

        return TRUE;
    }

    // Look for the set of well known objects that should be present after we have spun everything up.
    // Returns true if all well known objects are found in the expected state.
    bool SearchForWellKnownObjects(bool expectExist, const Snapshot& snapshot)
    {
        bool result = true;

        // Known modules in snapshot
        bool knownModulesLoaded = snapshot.MicrosoftManagementDeploymentInProcLoaded && snapshot.WindowsPackageManagerLoaded;
        if (knownModulesLoaded != expectExist)
        {
            std::cout << "Known modules were not in expected state [" << (expectExist ? "loaded" : "unloaded") << "]\n";
            result = false;
        }

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

    std::string GetBytesString(SIZE_T bytes)
    {
        constexpr SIZE_T s_kilo = 1024;
        constexpr std::string_view s_sizes = "BKMG"sv;
        size_t i = 0;

        while ((i + 1) < s_sizes.size() && bytes > s_kilo)
        {
            bytes /= s_kilo;
            ++i;
        }

        return std::to_string(bytes) + s_sizes[i];
    }

    const CLSID CLSID_PackageManager = { 0x2DDE4456, 0x64D9, 0x4673, 0x8F, 0x7E, 0xA4, 0xF1, 0x9A, 0x2E, 0x6C, 0xC3 }; // 2DDE4456-64D9-4673-8F7E-A4F19A2E6CC3
    const CLSID CLSID_FindPackagesOptions = { 0x96B9A53A, 0x9228, 0x4DA0, 0xB0, 0x13, 0xBB, 0x1B, 0x20, 0x31, 0xAB, 0x3D }; // 96B9A53A-9228-4DA0-B013-BB1B2031AB3D
    const CLSID CLSID_CreateCompositePackageCatalogOptions = { 0x768318A6, 0x2EB5, 0x400D, 0x84, 0xD0, 0xDF, 0x35, 0x34, 0xC3, 0x0F, 0x5D }; // 768318A6-2EB5-400D-84D0-DF3534C30F5D
    const CLSID CLSID_InstallOptions = { 0xE2AF3BA8, 0x8A88, 0x4766, 0x9D, 0xDA, 0xAE, 0x40, 0x13, 0xAD, 0xE2, 0x86 }; // E2AF3BA8-8A88-4766-9DDA-AE4013ADE286
    const CLSID CLSID_UninstallOptions = { 0x869CB959, 0xEB54, 0x425C, 0xA1, 0xE4, 0x1A, 0x1C, 0x29, 0x1C, 0x64, 0xE9 }; // 869CB959-EB54-425C-A1E4-1A1C291C64E9
    const CLSID CLSID_PackageMatchFilter = { 0x57DC8962, 0x7343, 0x42CD, 0xB9, 0x1C, 0x04, 0xF6, 0xA2, 0x5D, 0xB1, 0xD0 }; // 57DC8962-7343-42CD-B91C-04F6A25DB1D0
    const CLSID CLSID_PackageManagerSettings = { 0x80CF9D63, 0x5505, 0x4342, 0xB9, 0xB4, 0xBB, 0x87, 0x89, 0x5C, 0xA8, 0xBB }; // 80CF9D63-5505-4342-B9B4-BB87895CA8BB
    const CLSID CLSID_DownloadOptions = { 0x4288DF96, 0xFDC9, 0x4B68, 0xB4, 0x03, 0x19, 0x3D, 0xBB, 0xF5, 0x6A, 0x24 }; // 4288DF96-FDC9-4B68-B403-193DBBF56A24
    const CLSID CLSID_AuthenticationArguments = { 0x8D593114, 0x1CF1, 0x43B9, 0x87, 0x22, 0x4D, 0xBB, 0x30, 0x10, 0x32, 0x96 }; // 8D593114-1CF1-43B9-8722-4DBB30103296
    const CLSID CLSID_RepairOptions = { 0x30c024c4, 0x852c, 0x4dd4, 0x98, 0x10, 0x13, 0x48, 0xc5, 0x1e, 0xf9, 0xbb }; // {30C024C4-852C-4DD4-9810-1348C51EF9BB}
    const CLSID CLSID_AddPackageCatalogOptions = { 0x24e6f1fa, 0xe4c3, 0x4acd, 0x96, 0x5d, 0xdf, 0x21, 0x3f, 0xd5, 0x8f, 0x15 }; // {24E6F1FA-E4C3-4ACD-965D-DF213FD58F15}
    const CLSID CLSID_RemovePackageCatalogOptions = { 0x1125d3a6, 0xe2ce, 0x479a, 0x91, 0xd5, 0x71, 0xa3, 0xf6, 0xf8, 0xb0, 0xb }; // {1125D3A6-E2CE-479A-91D5-71A3F6F8B00B}

    template <typename T>
    T CreatePackageManagerObject(ActivationType activationType, const CLSID& clsid)
    {
        if (ActivationType::ClassName == activationType)
        {
            return T{};
        }
        else if (ActivationType::CoCreateInstance == activationType)
        {
            return winrt::create_instance<T>(clsid);
        }

        winrt::throw_hresult(E_UNEXPECTED);
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
        else if ("-src"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            SourceName = argv[i];
        }
        else if ("-url"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            SourceURL = argv[i];
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
        else if ("-keep-factories"sv == argv[i])
        {
            SkipClearFactories = true;
        }
    }
}

void TestParameters::OutputDetails() const
{
    std::cout << "Running inproc testbed with:\n"
        "  COM Init : " << ComInit << "\n"
        "  Activate : " << ActivationType << "\n"
        "    Clear  : " << std::boolalpha << !SkipClearFactories << "\n"
        "  Leak COM : " << std::boolalpha << LeakCOM << "\n"
        "  Unload   : " << UnloadBehavior << "\n"
        "    Expect : " << std::boolalpha << UnloadExpected() << "\n"
        "  Test     : " << TestToRun << "\n"
        "  Package  : " << PackageName << "\n"
        "  Source   : " << SourceName << "\n"
        "    URL    : " << SourceURL << "\n"
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

    InitializePackageManagerGlobals();

    if (UnloadBehavior::Never == UnloadBehavior || UnloadBehavior::AtUninitialize == UnloadBehavior)
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
    else if ("install_detect"sv == TestToRun)
    {
        return std::make_unique<InstallForSystem_DetectPresence>(*this);
    }

    return {};
}

void TestParameters::UninitializeTestState() const
{
    if (UnloadBehavior::AtUninitialize == UnloadBehavior)
    {
        SetUnloadPreference(true);
    }

    if (!LeakCOM)
    {
        RoUninitialize();
    }
}

bool TestParameters::UnloadExpected() const
{
    bool shouldUnload = true;
    if (UnloadBehavior::Never == UnloadBehavior || UnloadBehavior::AtUninitialize == UnloadBehavior ||
        (ActivationType::ClassName == ActivationType && SkipClearFactories))
    {
        shouldUnload = false;
    }
    return shouldUnload;
}

PackageManager TestParameters::CreatePackageManager() const
{
    return CreatePackageManagerObject<PackageManager>(ActivationType, CLSID_PackageManager);
}

CreateCompositePackageCatalogOptions TestParameters::CreateCreateCompositePackageCatalogOptions() const
{
    return CreatePackageManagerObject<CreateCompositePackageCatalogOptions>(ActivationType, CLSID_CreateCompositePackageCatalogOptions);
}

PackageMatchFilter TestParameters::CreatePackageMatchFilter() const
{
    return CreatePackageManagerObject<PackageMatchFilter>(ActivationType, CLSID_PackageMatchFilter);
}

FindPackagesOptions TestParameters::CreateFindPackagesOptions() const
{
    return CreatePackageManagerObject<FindPackagesOptions>(ActivationType, CLSID_FindPackagesOptions);
}

DownloadOptions TestParameters::CreateDownloadOptions() const
{
    return CreatePackageManagerObject<DownloadOptions>(ActivationType, CLSID_DownloadOptions);
}

AddPackageCatalogOptions TestParameters::CreateAddPackageCatalogOptions() const
{
    return CreatePackageManagerObject<AddPackageCatalogOptions>(ActivationType, CLSID_AddPackageCatalogOptions);
}

InstallOptions TestParameters::CreateInstallOptions() const
{
    return CreatePackageManagerObject<InstallOptions>(ActivationType, CLSID_InstallOptions);
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
            if (moduleEntry.szModule == L"Microsoft.Management.Deployment.InProc.dll"sv)
            {
                MicrosoftManagementDeploymentInProcLoaded = true;
            }
            else if (moduleEntry.szModule == L"WindowsPackageManager.dll"sv)
            {
                WindowsPackageManagerLoaded = true;
            }

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

bool UnloadAndCheckForLeaks::RunIterationWork()
{
    std::cout << "UnloadAndCheckForLeaks::RunIterationWork\n";
    return UsePackageManager(m_parameters);
}

bool UnloadAndCheckForLeaks::RunIterationTest()
{
    std::cout << "UnloadAndCheckForLeaks::RunIterationTest\n";

    Snapshot beforeUnload;
    if (!SearchForWellKnownObjects(true, beforeUnload))
    {
        return false;
    }

    CoFreeUnusedLibrariesEx(0, 0);

    Snapshot afterUnload;
    m_iterationSnapshots.emplace_back(beforeUnload, afterUnload);

    if (!SearchForWellKnownObjects(!m_parameters.UnloadExpected(), afterUnload))
    {
        return false;
    }

    return true;
}

bool UnloadAndCheckForLeaks::RunFinal()
{
    constexpr std::streamsize s_columnWidth = 5;

    bool result = true;

    std::cout << "--- UnloadAndCheckForLeaks results ---\n";
    std::cout << std::setfill(' ');

    // --- Threads ---
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

    // Look for consistent increase in measured values
    if (m_iterationSnapshots.size() > 1)
    {
        size_t previousValue = m_iterationSnapshots[0].second.ThreadCount;
        bool consistentIncrease = true;

        for (size_t i = 1; i < m_iterationSnapshots.size(); ++i)
        {
            size_t currentValue = m_iterationSnapshots[i].second.ThreadCount;
            if (currentValue > previousValue)
            {
                previousValue = currentValue;
            }
            else
            {
                consistentIncrease = false;
                break;
            }
        }

        if (consistentIncrease)
        {
            std::cout << "Post unload thread count shows consistent increase; failing test.\n";
            result = false;
        }
    }

    // --- Modules ---
    std::cout << "Module Count [Initial: " << m_initialSnapshot.ModuleCount << "]\n";

    std::cout << "Iteration  ";
    for (size_t i = 0; i < m_iterationSnapshots.size(); ++i)
    {
        std::cout << std::setw(s_columnWidth) << (i + 1);
    }
    std::cout << '\n';

    std::cout << "Pre Unload ";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << snapshot.first.ModuleCount;
    }
    std::cout << '\n';

    std::cout << "Post Unload";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << snapshot.second.ModuleCount;
    }
    std::cout << '\n';

    // Look for modules not unloading
    if (m_parameters.UnloadExpected() && m_iterationSnapshots.size() > 1)
    {
        bool noUnloadFound = false;

        for (size_t i = 0; i < m_iterationSnapshots.size(); ++i)
        {
            if (m_iterationSnapshots[i].first.ModuleCount == m_iterationSnapshots[i].second.ModuleCount)
            {
                noUnloadFound = true;
            }
        }

        if (noUnloadFound)
        {
            std::cout << "Module count did not decrease during at least one iteration; failing test.\n";
            result = false;
        }
    }

    // --- Memory ---
    std::cout << "Private Usage [Initial: " << GetBytesString(m_initialSnapshot.Memory.PrivateUsage) << "]\n";

    std::cout << "Iteration  ";
    for (size_t i = 0; i < m_iterationSnapshots.size(); ++i)
    {
        std::cout << std::setw(s_columnWidth) << (i + 1);
    }
    std::cout << '\n';

    std::cout << "Pre Unload ";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << GetBytesString(snapshot.first.Memory.PrivateUsage);
    }
    std::cout << '\n';

    std::cout << "Post Unload";
    for (const auto& snapshot : m_iterationSnapshots)
    {
        std::cout << std::setw(s_columnWidth) << GetBytesString(snapshot.second.Memory.PrivateUsage);
    }
    std::cout << '\n';

    return result;
}

InstallForSystem_DetectPresence::InstallForSystem_DetectPresence(const TestParameters& parameters) : m_parameters(parameters)
{
}

bool InstallForSystem_DetectPresence::RunIterationWork()
{
    std::cout << "Before installing, the detection state was: " << std::boolalpha << DetectForSystem(m_parameters) << '\n';

    return InstallForSystem(m_parameters);
}

bool InstallForSystem_DetectPresence::RunIterationTest()
{
    bool result = DetectForSystem(m_parameters);
    std::cout << "After installing, the detection state was: " << std::boolalpha << result << '\n';
    return result;
}

bool InstallForSystem_DetectPresence::RunFinal()
{
    return true;
}

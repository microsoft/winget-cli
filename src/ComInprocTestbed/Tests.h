// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>
#include <winrt/Microsoft.Management.Deployment.h>
#include <memory>
#include <vector>

// Represents a test that will be performed.
struct ITest
{
    virtual ~ITest() = default;

    // Runs an iteration of the work. Performed at the beginning of the iteration.
    virtual bool RunIterationWork() = 0;

    // Runs an iteration of the test. Performed at the end of the iteration.
    virtual bool RunIterationTest() = 0;

    // Performs the final test validation.
    virtual bool RunFinal() = 0;
};

enum class ComInitializationType
{
    STA,
    MTA,
};

enum class UnloadBehavior
{
    Allow,
    AtUninitialize,
    Never,
};

enum class ActivationType
{
    ClassName,
    CoCreateInstance,
};

// Test parameters from command line
struct TestParameters
{
    TestParameters(int argc, const char** argv);

    void OutputDetails() const;

    bool InitializeTestState() const;

    std::unique_ptr<ITest> CreateTest() const;

    void UninitializeTestState() const;

    // Determines if we expect COM to unload the module based on inputs.
    bool UnloadExpected() const;

    winrt::Microsoft::Management::Deployment::PackageManager CreatePackageManager() const;
    winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions() const;
    winrt::Microsoft::Management::Deployment::PackageMatchFilter CreatePackageMatchFilter() const;
    winrt::Microsoft::Management::Deployment::FindPackagesOptions CreateFindPackagesOptions() const;
    winrt::Microsoft::Management::Deployment::DownloadOptions CreateDownloadOptions() const;
    winrt::Microsoft::Management::Deployment::AddPackageCatalogOptions CreateAddPackageCatalogOptions() const;
    winrt::Microsoft::Management::Deployment::InstallOptions CreateInstallOptions() const;

    std::string TestToRun;
    ComInitializationType ComInit = ComInitializationType::MTA;
    bool LeakCOM = false;
    int Iterations = 1;
    std::string PackageName = "Microsoft.Edit";
    std::string SourceName = "winget";
    std::string SourceURL;
    UnloadBehavior UnloadBehavior = UnloadBehavior::Allow;
    ActivationType ActivationType = ActivationType::ClassName;
    bool SkipClearFactories = false;
};

// Captures a snapshot of current resource usage.
struct Snapshot
{
    Snapshot();

    size_t ThreadCount = 0;
    size_t ModuleCount = 0;
    bool MicrosoftManagementDeploymentInProcLoaded = false;
    bool WindowsPackageManagerLoaded = false;
    PROCESS_MEMORY_COUNTERS_EX2 Memory{};
};

// A test that unloads the COM module and looks for resources that were not released.
struct UnloadAndCheckForLeaks : public ITest
{
    UnloadAndCheckForLeaks(const TestParameters& parameters);

    bool RunIterationWork() override;

    bool RunIterationTest() override;

    bool RunFinal() override;

private:
    const TestParameters& m_parameters;
    Snapshot m_initialSnapshot;
    std::vector<std::pair<Snapshot, Snapshot>> m_iterationSnapshots;
};

// A test that installs the package machine wide and then attempts to detect that it is installed.
struct InstallForSystem_DetectPresence : public ITest
{
    InstallForSystem_DetectPresence(const TestParameters& parameters);

    bool RunIterationWork() override;

    bool RunIterationTest() override;

    bool RunFinal() override;

private:
    const TestParameters& m_parameters;
};

#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "DependenciesTestSource.h"
#include <winget/DependenciesGraph.h>
#include <Workflows/DependencyNodeProcessor.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <Workflows/DependenciesFlow.h>
#include <Workflows/WorkflowBase.h>
#include <winget/Filesystem.h>
#include <winget/ManifestYamlParser.h>
#include <winget/PathVariable.h>
#include <winget/RepositorySource.h>
#include <Resources.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Logging;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

TEST_CASE("DependencyGraph_BFirst", "[dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    std::vector<Dependency> installationOrder;

    const auto& manifest = CreateFakeManifestWithDependencies("NeedsToInstallBFirst");
    const auto& installers = manifest.Installers;
    const Dependency& rootAsDependency = Dependency(DependencyType::Package, manifest.Id);
    DependencyList rootDependencies;
    std::for_each(installers.begin(), installers.end(), [&](ManifestInstaller installer) { rootDependencies.Add(installer.Dependencies); });

    DependencyGraph graph(rootAsDependency, rootDependencies, [&](Dependency node)
        {
            DependencyList dependencyList;
            auto dependencyManifest = CreateFakeManifestWithDependencies(manifest.Id);

            for (auto installer : dependencyManifest.Installers)
            {
                dependencyList.Add(installer.Dependencies);
            }

            return dependencyList;
        });

    graph.BuildGraph();

    installationOrder = graph.GetInstallationOrder();

    REQUIRE(installationOrder.size() == 3);
    REQUIRE(installationOrder.at(0).Id() == "C");
    REQUIRE(installationOrder.at(1).Id() == "B");
    REQUIRE(installationOrder.at(2).Id() == "NeedsToInstallBFirst");
}

TEST_CASE("DependencyGraph_InStackNoLoop", "[dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    std::vector<Dependency> installationOrder;

    const auto& manifest = CreateFakeManifestWithDependencies("DependencyAlreadyInStackButNoLoop");
    const auto& installers = manifest.Installers;
    const Dependency& rootAsDependency = Dependency(DependencyType::Package, manifest.Id);
    DependencyList rootDependencies;
    std::for_each(installers.begin(), installers.end(), [&](ManifestInstaller installer) { rootDependencies.Add(installer.Dependencies); });

    DependencyGraph graph(rootAsDependency, rootDependencies, [&](Dependency node)
        {
            DependencyList dependencyList;
            auto dependencyManifest = CreateFakeManifestWithDependencies(manifest.Id);

            for (auto installer : dependencyManifest.Installers)
            {
                dependencyList.Add(installer.Dependencies);
            }

            return dependencyList;
        });

    graph.BuildGraph();

    installationOrder = graph.GetInstallationOrder();

    REQUIRE(installationOrder.size() == 3);
    REQUIRE(installationOrder.at(0).Id() == "F");
    REQUIRE(installationOrder.at(1).Id() == "C");
    REQUIRE(installationOrder.at(2).Id() == "DependencyAlreadyInStackButNoLoop");
}

TEST_CASE("DependencyGraph_EasyToSeeLoop", "[dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    std::vector<Dependency> installationOrder;

    const auto& manifest = CreateFakeManifestWithDependencies("EasyToSeeLoop");
    const auto& installers = manifest.Installers;
    const Dependency& rootAsDependency = Dependency(DependencyType::Package, manifest.Id);
    DependencyList rootDependencies;
    std::for_each(installers.begin(), installers.end(), [&](ManifestInstaller installer) { rootDependencies.Add(installer.Dependencies); });

    DependencyGraph graph(rootAsDependency, rootDependencies, [&](Dependency node) {
        DependencyList dependencyList;
        auto dependencyManifest = CreateFakeManifestWithDependencies(manifest.Id);

        for (auto installer : dependencyManifest.Installers)
        {
            dependencyList.Add(installer.Dependencies);
        }

        return dependencyList;
        });

    graph.BuildGraph();

    installationOrder = graph.GetInstallationOrder();

    bool hasLoop = graph.HasLoop();

    REQUIRE(hasLoop);

    REQUIRE(installationOrder.size() == 2);
    REQUIRE(installationOrder.at(0).Id() == "D");
    REQUIRE(installationOrder.at(1).Id() == "EasyToSeeLoop");
}

TEST_CASE("DependencyNodeProcessor_SkipInstalled", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Context context{ installOutput, std::cin };

    Manifest manifest = CreateFakeManifestWithDependencies("installed1");

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    DependencyNodeProcessor nodeProcessor(context);

    Dependency rootAsDependency(DependencyType::Package, manifest.Id);

    DependencyNodeProcessorResult result = nodeProcessor.EvaluateDependencies(rootAsDependency);
    REQUIRE(result == DependencyNodeProcessorResult::Skipped);
}

TEST_CASE("DependencyNodeProcessor_NoInstallers", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Context context { installOutput, std::cin };

    Manifest manifest = CreateFakeManifestWithDependencies("withoutInstallers");

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    DependencyNodeProcessor nodeProcessor(context);

    Dependency rootAsDependency(DependencyType::Package, manifest.Id);

    DependencyNodeProcessorResult result = nodeProcessor.EvaluateDependencies(rootAsDependency);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowNoInstallerFound("withoutInstallers"_liv))) != std::string::npos);
    REQUIRE(result == DependencyNodeProcessorResult::Error);
}

TEST_CASE("DependencyNodeProcessor_StackOrderIsOk", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Context context{ installOutput, std::cin };

    Manifest manifest = CreateFakeManifestWithDependencies("StackOrderIsOk");

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    DependencyNodeProcessor nodeProcessor(context);

    Dependency rootAsDependency(DependencyType::Package, manifest.Id);

    DependencyNodeProcessorResult result = nodeProcessor.EvaluateDependencies(rootAsDependency);
    auto dependencyList = nodeProcessor.GetDependencyList();
    REQUIRE(dependencyList.Size() == 1);
    REQUIRE(dependencyList.HasDependency(Dependency(DependencyType::Package, "C")));
    REQUIRE(result == DependencyNodeProcessorResult::Success);
}

TEST_CASE("DependencyNodeProcessor_NoMatches", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Context context{ installOutput, std::cin };

    Manifest manifest = CreateFakeManifestWithDependencies("NoMatches");

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    DependencyNodeProcessor nodeProcessor(context);

    Dependency rootAsDependency(DependencyType::Package, manifest.Id);

    DependencyNodeProcessorResult result = nodeProcessor.EvaluateDependencies(rootAsDependency);
    auto dependencyList = nodeProcessor.GetDependencyList();
    REQUIRE(dependencyList.Size() == 0);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowNoMatches)) != std::string::npos);
    REQUIRE(result == DependencyNodeProcessorResult::Error);
}

std::wstring GetCurrentProcessPathVariable()
{
    size_t requiredSize;
    _wgetenv_s(&requiredSize, nullptr, 0, L"PATH");

    if (requiredSize > 0)
    {
        auto buffer = std::make_unique<wchar_t[]>(requiredSize);
        errno_t errorResult = _wgetenv_s(&requiredSize, buffer.get(), requiredSize, L"PATH");
        if (errorResult == 0)
        {
            return std::wstring(buffer.get());
        }
    }
    return {};
}

TEST_CASE("RefreshEnvironmentVariable_User", "[dependencies]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::wstring testPathEntry = L"testUserPathEntry";
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::User);
    pathVariable.Append(testPathEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testPathEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    bool secondCheck = updatedPathValue.find(testPathEntry) != std::string::npos;

    pathVariable.Remove(testPathEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}

TEST_CASE("RefreshEnvironmentVariable_System", "[dependencies]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::wstring testPathEntry = L"testSystemPathEntry";
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::Machine);
    pathVariable.Append(testPathEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testPathEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    bool secondCheck = updatedPathValue.find(testPathEntry) != std::string::npos;

    pathVariable.Remove(testPathEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}

TEST_CASE("VerifyPathRefreshExpandsValues", "[dependencies]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::filesystem::path testEntry{ "%USERPROFILE%\\testPath" };
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::User);
    pathVariable.Append(testEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    // %USERPROFILE% should be replaced with the actual path.
    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    std::wstring expandedTestPath = AppInstaller::Filesystem::GetExpandedPath(testEntry.u8string());
    bool secondCheck = updatedPathValue.find(expandedTestPath) != std::string::npos;

    pathVariable.Remove(testEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}

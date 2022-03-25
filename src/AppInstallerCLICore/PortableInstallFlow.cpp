// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Portable.h"
#include <filesystem>

using namespace AppInstaller::Manifest;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Portable;

    namespace
    {
        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries, const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            std::string displayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            std::string displayVersion = manifest.Version;
            std::string publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();
            std::string packageId = manifest.Id;

            if (appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = {};
            }
            else
            {
                appsAndFeaturesEntry = appsAndFeaturesEntries[0];
            }

            if (appsAndFeaturesEntry.DisplayName.empty())
            {
                appsAndFeaturesEntry.DisplayName = displayName;
            }
            if (appsAndFeaturesEntry.DisplayVersion.empty())
            {
                appsAndFeaturesEntry.DisplayVersion = displayVersion;
            }
            if (appsAndFeaturesEntry.Publisher.empty())
            {
                appsAndFeaturesEntry.Publisher = publisher;
            }
            if (appsAndFeaturesEntry.ProductCode.empty())
            {
                appsAndFeaturesEntry.ProductCode = packageId;
            }

            return appsAndFeaturesEntry;
        }

        PortableArguments GetPortableInstallerArguments(Execution::Context& context)
        {
            PortableArguments portableArgs = {};
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));

            if (scope == Manifest::ScopeEnum::Machine)
            {
                portableArgs.RootKey = HKEY_LOCAL_MACHINE;
            }
            else
            {
                portableArgs.RootKey = HKEY_CURRENT_USER;
            }

            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            std::string fileName = installerPath.filename().u8string();

            portableArgs.InstallRootDirectory = GetPortableInstallRoot(scope, arch, installLocationArg);

            std::vector<string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
            std::string_view rename = context.Args.GetArg(Execution::Args::Type::Rename);

            if (!rename.empty())
            {
                portableArgs.AppPathEntry = rename;
            }
            else if (commands.size() > 0)
            {
                portableArgs.AppPathEntry = commands[0];
            }
            else
            {
                portableArgs.AppPathEntry = fileName;
            }

            if (!HasSuffix(portableArgs.AppPathEntry, ".exe"))
            {
                portableArgs.AppPathEntry += ".exe";
            }

            std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries;
            AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
            portableArgs.PackageId = manifest.Id;
            portableArgs.AppsAndFeatureEntry = GetAppsAndFeaturesEntryForPortableInstall(appsAndFeaturesEntries, manifest);

            return portableArgs;
        }

        bool InvokePortableInstall(const std::filesystem::path& installerPath, PortableArguments& portableArgs, IProgressCallback& callback)
        {
            std::filesystem::path portableInstallerPath;
            std::filesystem::path installRootPackageDirectory = portableArgs.InstallRootDirectory / portableArgs.PackageId;

            callback.BeginProgress();
            std::filesystem::create_directories(installRootPackageDirectory);
            std::string fileName = installerPath.filename().u8string();
            std::filesystem::path portableInstallerDestPath = installRootPackageDirectory / fileName;

            // TODO: Copying file for a single portable exe is sufficient, but will need to change to checking file handle when dealing with
            // multiple files (archive) to guarantee all files can be successfully copied.
            bool copyResult = CopyFileExW(installerPath.c_str(), portableInstallerDestPath.c_str(), &CopyPortableExeProgressCallback, &callback, FALSE, 0);
            if (!copyResult)
            {
                DWORD copyError = GetLastError();
                THROW_IF_WIN32_ERROR(copyError);
                return false;
            }

            bool registrationResult;
            registrationResult = WriteToAppPathsRegistry(portableArgs.RootKey, portableArgs.AppPathEntry, portableInstallerDestPath, true);
            registrationResult = WriteToUninstallRegistry(portableArgs.RootKey, portableArgs.PackageId, portableArgs.AppsAndFeatureEntry);

            if (registrationResult)
            {
                callback.OnProgress(100, 100, AppInstaller::ProgressType::Percent);
            }
            else
            {
                // TODO: Handle clean up process if registration fails when implementing uninstall flow.
            }

            callback.EndProgress(true);
            return registrationResult;
        }
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        PortableArguments portableArgs = GetPortableInstallerArguments(context);

        bool result = context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
            {
                return InvokePortableInstall(installerPath, portableArgs, callback);
            });

        if (result)
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            context.Reporter.Warn() << Resource::String::InstallationAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
    }
}

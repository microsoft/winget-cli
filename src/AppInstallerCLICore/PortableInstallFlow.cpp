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
        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries, AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            std::string packageId = manifest.Id;
            std::string displayVersion = manifest.Version;
            std::string displayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            std::string publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();

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
            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();

            // Retrieve command-line arguments.
            std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));

            AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
            Manifest::ManifestInstaller installer = context.Get<Execution::Data::Installer>().value();
            Utility::Architecture arch = installer.Arch;
            std::vector<string_t> commands = installer.Commands;
            std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = installer.AppsAndFeaturesEntries;

            PortableArguments portableArgs = {};
            portableArgs.PackageId = manifest.Id;
            portableArgs.RootKey = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;

            if (!renameArg.empty())
            {
                std::string renameArgValue = Normalize(renameArg);
                portableArgs.CommandAlias = renameArgValue;
                portableArgs.FileName = renameArgValue;
            }
            else
            {
                portableArgs.FileName = installerPath.filename().u8string();
                portableArgs.CommandAlias = !commands.empty() ? commands[0] : portableArgs.FileName;
            }

            if (!HasSuffix(portableArgs.CommandAlias, ".exe"))
            {
                portableArgs.CommandAlias += ".exe";
            }
            if (!HasSuffix(portableArgs.FileName, ".exe"))
            {
                portableArgs.FileName += ".exe";
            }

            portableArgs.InstallRootDirectory = !installLocationArg.empty() ? std::filesystem::path{ ConvertToUTF16(installLocationArg) } : GetPortableInstallRoot(scope, arch);
            portableArgs.LinksLocation = GetPortableLinksLocation(scope, arch);
            portableArgs.AppsAndFeatureEntry = GetAppsAndFeaturesEntryForPortableInstall(appsAndFeaturesEntries, manifest);
            return portableArgs;
        }

        bool InvokePortableInstall(const std::filesystem::path& installerPath, PortableArguments& portableArgs, IProgressCallback& callback)
        {
            std::filesystem::path installRootPackageDirectory = portableArgs.InstallRootDirectory / portableArgs.PackageId;
            std::filesystem::create_directories(installRootPackageDirectory);
            std::filesystem::path portableTargetPath = installRootPackageDirectory / portableArgs.FileName;

            // TODO: Copying file for a single portable exe is sufficient, but will need to change to checking file handles when dealing with
            // multiple files (archive) to guarantee all files can be successfully copied.
            bool copyResult = CopyFileExW(installerPath.c_str(), portableTargetPath.c_str(), &CopyPortableExeProgressCallback, &callback, FALSE, 0);
            if (!copyResult)
            {
                DWORD copyError = GetLastError();
                THROW_IF_WIN32_ERROR(copyError);
                return false;
            }

            std::filesystem::path symlinkPath = portableArgs.LinksLocation / portableArgs.CommandAlias;
            CreateSymlink(portableTargetPath, symlinkPath);
            AddToPathEnvironmentRegistry(portableArgs.RootKey, portableArgs.LinksLocation.u8string());
            WriteToUninstallRegistry(portableArgs.RootKey, portableArgs.PackageId, portableArgs.AppsAndFeatureEntry);
            return true;
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
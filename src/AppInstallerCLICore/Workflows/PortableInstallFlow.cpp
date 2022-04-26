// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Filesystem.h"
#include "winget/PortableARPEntry.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::string_view s_LocalSource = "*Local"sv;

        void AppendExeExtension(std::filesystem::path& value)
        {
            if (value.extension() != ".exe")
            {
                value += ".exe";
            }
        }

        std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX86);
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX64);
                }
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortablePackageUserRoot);
            }
        }

        std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocation);
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation);
            }
        }

        std::filesystem::path GetPortableTargetDirectory(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            std::string_view locationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            const std::string& packageId = context.Get<Execution::Data::Manifest>().Id;

            std::string source;
            if (context.Contains(Execution::Data::Source))
            {
                source = context.Get<Execution::Data::Source>().GetIdentifier();
            }
            else
            {
                source = s_LocalSource;
            }

            // Make sure that virtual sources don't contain "*" or creating the directory will fail.
            FindAndReplace(source, "*", ""); 
            std::filesystem::path targetInstallDirectory;

            if (!locationArg.empty())
            {
                targetInstallDirectory = std::filesystem::path{ ConvertToUTF16(locationArg) };
            }
            else
            {
                targetInstallDirectory = GetPortableInstallRoot(scope, arch);
                targetInstallDirectory /= packageId + "_" + source;
            }

            return targetInstallDirectory;
        }

        std::filesystem::path GetPortableTargetFullPath(Execution::Context& context)
        {
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::filesystem::path& targetInstallDirectory = GetPortableTargetDirectory(context);
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            std::filesystem::path fileName;
            if (!renameArg.empty())
            {
                fileName = ConvertToUTF16(renameArg);
            }
            else
            {
                fileName = installerPath.filename();
            }

            AppendExeExtension(fileName);
            return targetInstallDirectory / fileName;
        }

        std::filesystem::path GetPortableSymlinkFullPath(Execution::Context& context)
        {
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::vector<string_t>& commands = context.Get<Execution::Data::Installer>()->Commands;
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            std::filesystem::path commandAlias;
            if (!renameArg.empty())
            {
                commandAlias = ConvertToUTF16(renameArg);
            }
            else
            {
                if (!commands.empty())
                {
                    commandAlias = ConvertToUTF16(commands[0]);
                }
                else
                {
                    commandAlias = installerPath.filename();
                }
            }

            AppendExeExtension(commandAlias);
            return GetPortableLinksLocation(scope) / commandAlias;
        }

        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& appsAndFeaturesEntries, const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            if (!appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = appsAndFeaturesEntries[0];
            }

            if (appsAndFeaturesEntry.DisplayName.empty())
            {
                appsAndFeaturesEntry.DisplayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            }
            if (appsAndFeaturesEntry.DisplayVersion.empty())
            {
                appsAndFeaturesEntry.DisplayVersion = manifest.Version;
            }
            if (appsAndFeaturesEntry.Publisher.empty())
            {
                appsAndFeaturesEntry.Publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();
            }

            return appsAndFeaturesEntry;
        }

        bool AddToPathRegistry(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            const std::filesystem::path& linksDirectory = GetPortableLinksLocation(scope);

            Key key;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                key = Registry::Key::Create(HKEY_LOCAL_MACHINE, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::Create(HKEY_CURRENT_USER, Normalize(s_PathSubkey_User));
            }

            std::wstring pathName = Normalize(s_PathName);
            std::string portableLinksDir = linksDirectory.u8string();
            std::string pathValue = key[pathName]->GetValue<Value::Type::String>();
            
            if (pathValue.find(portableLinksDir) == std::string::npos)
            {
                if (pathValue.back() != ';')
                {
                    pathValue += ";";
                }

                pathValue += portableLinksDir + ";";
                AICLI_LOG(CLI, Info, << "Adding to Path environment variable: " << portableLinksDir);
                key.SetValue(pathName, ConvertToUTF16(pathValue), REG_EXPAND_SZ);
                return true;
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path already existed in environment variable. Skipping...");
                return false;
            }
        }

        Registry::Key WritePortableEntryToUninstallRegistry(Execution::Context& context)
        {
            const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
            const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);
            std::string packageIdentifier = manifest.Id;

            std::string sourceIdentifier;
            if (context.Contains(Execution::Data::Source))
            {
                sourceIdentifier = context.Get<Execution::Data::Source>().GetIdentifier();
            }
            else
            {
                sourceIdentifier = s_LocalSource;
            }

            std::wstring productCode;
            if (!entry.ProductCode.empty())
            {
                productCode = ConvertToUTF16(entry.ProductCode);
            }
            else
            {
                productCode = ConvertToUTF16(packageIdentifier + "_" + sourceIdentifier);
            }

            Portable::PortableARPEntry uninstallEntry = Portable::PortableARPEntry(
                ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)),
                context.Get<Execution::Data::Installer>()->Arch,
                productCode);

            if(uninstallEntry.Exists())
            {
                if (uninstallEntry.IsSamePortablePackageEntry(packageIdentifier, sourceIdentifier))
                {
                    // TODO: Replace HashOverride with --Force when argument behavior gets updated.
                    if (!context.Args.Contains(Execution::Args::Type::HashOverride))
                    {
                        AICLI_LOG(CLI, Error, << "Registry match failed, skipping write to uninstall registry");
                        context.Reporter.Error() << Resource::String::PortableRegistryCollisionOverrideRequired << std::endl;
                        AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, {} );
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Overriding registry match check...");
                        context.Reporter.Warn() << Resource::String::PortableRegistryCollisionOverridden << std::endl;
                    }
                }
            }

            AICLI_LOG(CLI, Info, << "Begin writing to Uninstall registry.");
            uninstallEntry.SetValue(PortableValueName::DisplayName, ConvertToUTF16(entry.DisplayName));
            uninstallEntry.SetValue(PortableValueName::DisplayVersion, ConvertToUTF16(entry.DisplayVersion));
            uninstallEntry.SetValue(PortableValueName::Publisher, ConvertToUTF16(entry.Publisher));
            uninstallEntry.SetValue(PortableValueName::InstallDate, ConvertToUTF16(Utility::GetCurrentDateForARP()));
            uninstallEntry.SetValue(PortableValueName::URLInfoAbout, ConvertToUTF16(manifest.DefaultLocalization.Get<Manifest::Localization::PackageUrl>()));
            uninstallEntry.SetValue(PortableValueName::HelpLink, ConvertToUTF16(manifest.DefaultLocalization.Get<Manifest::Localization::PublisherSupportUrl>()));
            uninstallEntry.SetValue(PortableValueName::UninstallString, L"winget uninstall --product-code " + productCode);
            uninstallEntry.SetValue(PortableValueName::WinGetInstallerType, ConvertToUTF16(InstallerTypeToString(InstallerTypeEnum::Portable)));
            uninstallEntry.SetValue(PortableValueName::WinGetPackageIdentifier, ConvertToUTF16(manifest.Id));
            uninstallEntry.SetValue(PortableValueName::WinGetSourceIdentifier, ConvertToUTF16(sourceIdentifier));
            uninstallEntry.SetValue(PortableValueName::PortableTargetFullPath, GetPortableTargetFullPath(context).wstring());
            uninstallEntry.SetValue(PortableValueName::PortableSymlinkFullPath, GetPortableSymlinkFullPath(context).wstring());
            uninstallEntry.SetValue(PortableValueName::SHA256, Utility::SHA256::ConvertToWideString(context.Get<Execution::Data::HashPair>().second));
            uninstallEntry.SetValue(PortableValueName::InstallLocation, GetPortableTargetDirectory(context).wstring());
            AICLI_LOG(CLI, Info, << "Writing to Uninstall registry complete.");
            return uninstallEntry.GetKey();
        }
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            Registry::Key entry = WritePortableEntryToUninstallRegistry(context);
            if (context.IsTerminated())
            {
                return;
            }

            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::filesystem::path& targetFullPath = entry[ToString(PortableValueName::PortableTargetFullPath)].value().GetValue<Value::Type::UTF16String>();
            const std::filesystem::path& symlinkFullPath = entry[ToString(PortableValueName::PortableSymlinkFullPath)].value().GetValue<Value::Type::UTF16String>();

            std::filesystem::path targetDirectory = targetFullPath.parent_path();
            if (std::filesystem::create_directories(targetDirectory))
            {
                AICLI_LOG(CLI, Info, << "Creating target install directories: " << targetDirectory);
            }

            Filesystem::RenameFile(installerPath, targetFullPath);
            AICLI_LOG(CLI, Info, << "Portable exe moved to: " << targetFullPath);

            std::filesystem::file_status status = std::filesystem::status(symlinkFullPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkFullPath << "points to an existing directory.");
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY);
            }
            else
            {
                context.Reporter.Warn() << Resource::String::OverwritingExistingFileAtMessage << symlinkFullPath.u8string() << std::endl;
                std::filesystem::remove(symlinkFullPath);
            }

            std::filesystem::create_symlink(targetFullPath, symlinkFullPath);
            AICLI_LOG(CLI, Info, << "Symlink created at: " << symlinkFullPath);

            if (AddToPathRegistry(context))
            {
                context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            }

            context.Add<Execution::Data::OperationReturnCode>(0);
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        catch (...)
        {
            Workflow::HandleException(context, std::current_exception());
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED);
        }
    }

    void EnsureFeatureEnabledForPortableInstall(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().InstallerType;

        if (installerType == InstallerTypeEnum::Portable)
        {
            context << Workflow::EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::PortableInstall);
        }
    }

    void EnsureValidArgsForPortableInstall(Execution::Context& context)
    {
        std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
        std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);

        if (MakeSuitablePathPart(renameArg) != renameArg)
        {
            context.Reporter.Error() << Resource::String::InvalidRenameArgumentError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS);
        }

        const std::filesystem::path& targetDirectory = GetPortableTargetDirectory(context);
        if (!std::filesystem::is_directory(targetDirectory) || !std::filesystem::exists(targetDirectory))
        {
            if (!std::filesystem::create_directories(targetDirectory))
            {
                context.Reporter.Error() << Resource::String::PortableInstallDirectoryNotCreated;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
            }
        }
    }

    void EnsureVolumeSupportsReparsePoints(Execution::Context& context)
    {
        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        const std::filesystem::path& symlinkDirectory = GetPortableLinksLocation(scope);

        if (!AppInstaller::Filesystem::SupportsReparsePoints(symlinkDirectory))
        {
            context.Reporter.Error() << Resource::String::ReparsePointsNotSupportedError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_REPARSE_POINT_NOT_SUPPORTED);
        }
    }

    void EnsureOSVersionSupportForPortableInstall(Execution::Context& context)
    {
        // Unvirtualized resources restricted capability is only supported for >= 10.0.18362
        // TODO: Add support for OS versions that don't support virtualization.
        if (!Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version("10.0.18362")))
        {
            context.Reporter.Error() << Resource::String::OSVersionDoesNotSupportPortableInstall << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_OSVERSION_NOT_SUPPORTED);
        }
    }

}
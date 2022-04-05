// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Filesystem.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_UninstallSubkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

        std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                std::filesystem::path portableAppMachineRoot = Settings::User().Get<Settings::Setting::PortableAppMachineRoot>();
                if (!portableAppMachineRoot.empty())
                {
                    return portableAppMachineRoot;
                }
                else
                {
                    if (arch == Utility::Architecture::X86)
                    {
                        return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX86);
                    }
                    else
                    {
                        return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX64);
                    }
                }
            }
            else
            {
                std::filesystem::path portableAppUserRoot = Settings::User().Get<Settings::Setting::PortableAppUserRoot>();
                if (!portableAppUserRoot.empty())
                {
                    return portableAppUserRoot;
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableAppUserRoot);
                }
            }
        }

        std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX86);
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocationX64);
                }
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation);
            }
        }

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

        bool AddToPathEnvironmentRegistry(HKEY root, const std::string& value)
        {
            AppInstaller::Registry::Key key;
            if (root == HKEY_LOCAL_MACHINE)
            {
                key = Registry::Key::CreateKeyAndOpen(root, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::CreateKeyAndOpen(root, Normalize(s_PathSubkey_User));
            }

            std::wstring pathKey = { L"Path" };
            std::string pathValue = key[pathKey]->GetValue<AppInstaller::Registry::Value::Type::String>();
            if (pathValue.find(value) == std::string::npos)
            {
                std::string modifiedPathValue = pathValue;

                if (modifiedPathValue.back() != ';')
                {
                    modifiedPathValue += ";";
                }

                modifiedPathValue += value + ";";
                return key.SetKeyValue(pathKey, ConvertToUTF16(modifiedPathValue), REG_EXPAND_SZ);
            }

            return true;
        }

        bool WriteToUninstallRegistry(HKEY root, std::string_view packageIdentifier, Manifest::AppsAndFeaturesEntry& entry)
        {
            std::wstring productCode = ConvertToUTF16(entry.ProductCode);
            std::wstring displayName = ConvertToUTF16(entry.DisplayName);
            std::wstring displayVersion = ConvertToUTF16(entry.DisplayVersion);
            std::wstring publisher = ConvertToUTF16(entry.Publisher);
            std::wstring uninstallString = L"winget uninstall --id " + ConvertToUTF16(packageIdentifier);
            std::wstring fullRegistryKey = Normalize(s_UninstallSubkey) + productCode;

            AppInstaller::Registry::Key key = Registry::Key::CreateKeyAndOpen(root, fullRegistryKey);

            bool result;
            result = key.SetKeyValue(L"DisplayName", displayName, REG_SZ);
            result = key.SetKeyValue(L"DisplayVersion", displayVersion, REG_SZ);
            result = key.SetKeyValue(L"Publisher", publisher, REG_SZ);
            result = key.SetKeyValue(L"UninstallString", uninstallString, REG_SZ);

            return result;
        }

        void AppendExeExtension(std::string& value)
        {
            if (!HasSuffix(value, ".exe"))
            {
                value += ".exe";
            }
        }

        std::optional<DWORD> InvokePortableInstall(Execution::Context& context, IProgressCallback& progress)
        {
            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));

            Manifest::ManifestInstaller installer = context.Get<Execution::Data::Installer>().value();
            std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = installer.AppsAndFeaturesEntries;
            Utility::Architecture arch = installer.Arch;
            std::vector<string_t> commands = installer.Commands;

            AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
            std::string packageId = manifest.Id;

            std::string fileName;
            std::string commandAlias;
            if (!renameArg.empty())
            {
                std::string renameArgValue = Normalize(renameArg);
                commandAlias = renameArgValue;
                fileName = renameArgValue;
            }
            else
            {
                fileName = installerPath.filename().u8string();
                commandAlias = !commands.empty() ? commands[0] : fileName;
            }

            AppendExeExtension(commandAlias);
            AppendExeExtension(fileName);

            std::filesystem::path installRootPackageDirectory = GetPortableInstallRoot(scope, arch) / packageId;
            std::filesystem::create_directories(installRootPackageDirectory);
            std::filesystem::path portableTargetPath = installRootPackageDirectory / fileName;

            // TODO: Copying file for a single portable exe is sufficient, but will need to change to checking file handles when dealing with
            // multiple files (archive) to guarantee all files can be successfully copied.
            DWORD exitCode = 0;
            bool copyResult = CopyFileExW(installerPath.c_str(), portableTargetPath.c_str(), &AppInstaller::Filesystem::CopyFileProgressCallback, &progress, FALSE, 0);
            if (!copyResult)
            {
                exitCode = GetLastError();
                return exitCode;
            }

            Manifest::AppsAndFeaturesEntry appsAndFeatureEntry = GetAppsAndFeaturesEntryForPortableInstall(appsAndFeaturesEntries, manifest);
            std::filesystem::path portableLinksLocation = GetPortableLinksLocation(scope, arch);
            std::filesystem::path symlinkPath = portableLinksLocation / commandAlias;
            AppInstaller::Filesystem::CreateSymlink(portableTargetPath, symlinkPath);

            HKEY rootKey = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            AddToPathEnvironmentRegistry(rootKey, portableLinksLocation.u8string());
            WriteToUninstallRegistry(rootKey, packageId, appsAndFeatureEntry);
            return exitCode;
        }
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        auto installResult = context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
            {
                return InvokePortableInstall(context, callback);
            });

        if (!installResult)
        {
            context.Reporter.Warn() << Resource::String::InstallationAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else
        {
            context.Add<Execution::Data::OperationReturnCode>(installResult.value());
        }
    }
}
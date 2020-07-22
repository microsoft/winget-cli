// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ShellExecuteInstallerHandler.h"
#include "WorkflowBase.h"
#include<stdlib.h>
#include<stdio.h>
#include<PathCch.h>
//#include <windows.management.deployment.h>

//#include <winrt/Windows.Foundation.h>
//#include <winrt/Windows.Management.Deployment.h>




//using namespace winrt::Windows::Foundation;


using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
//using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

//using IDeploymentOperation =
//ABI::Windows::Foundation::__FIAsyncOperationWithProgress_2_Windows__CManagement__CDeployment__CDeploymentResult_Windows__CManagement__CDeployment__CDeploymentProgress_t;
namespace AppInstaller::CLI::Workflow
{
    void EnsureMinOSVersion(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();

        if (!manifest.MinOSVersion.empty() &&
            !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Version(manifest.MinOSVersion)))
        {
            context.Reporter.Error() << Resource::String::InstallationRequiresHigherWindows << ' ' << manifest.MinOSVersion << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION));
        }
    }

    void EnsureApplicableInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();

        if (!installer.has_value())
        {
            context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }

    void ShowInstallationDisclaimer(Execution::Context& context)
    {
        context.Reporter.Info() << 
            Resource::String::InstallationDisclaimer1 << std::endl <<
            Resource::String::InstallationDisclaimer2 << std::endl;
    }

    void DownloadInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        switch (installer.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::PWA:
            context << GeneratePwaMsix;
            break;
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << DownloadInstallerFile;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            if (installer.SignatureSha256.empty())
            {
                context << DownloadInstallerFile;
            }
            else
            {
                // Signature hash provided. No download needed. Just verify signature hash.
                context << GetMsixSignatureHash;
            }
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }


    void InstallPWA(Execution::Context& context)
    {
        const auto& path = context.Get<Execution::Data::InstallerPath>();
        Uri uri(path.c_str());
        PackageManager packageManager;
        AddPackageOptions options;
        options.AllowUnsigned(true);
        options.DeferRegistrationWhenPackagesAreInUse(true);
        IPackageManager9 packageManager9 = packageManager.as<IPackageManager9>();
        auto test = packageManager9.AddPackageByUriAsync(uri, options).get();
    }
    //Move to Utilities later
    std::wstring GetPathToExecutable()
    {
        std::vector<wchar_t> pathBuf;
        DWORD copied = 0;
        do {
            pathBuf.resize(pathBuf.size() + MAX_PATH);
            copied = GetModuleFileName(0, &pathBuf.at(0), pathBuf.size());
        } while (copied >= pathBuf.size());

        pathBuf.resize(copied);

  

        return std::wstring(pathBuf.begin(), pathBuf.end());
    }
    void GeneratePwaMsix(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        //const auto& installer = context.Get<Execution::Data::Installer>().value();
       
        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);
        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);
        std::filesystem::create_directories(tempInstallerPath);
        const std::filesystem::path& filePath = tempInstallerPath;
        //const std::filesystem::path& filePath = context.Get<Execution::Data::InstallerPath>();
        std::wstring appPath = GetPathToExecutable();
        PathCchRemoveFileSpec(&appPath[0], appPath.size());

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &appPath[0], (int)appPath.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &appPath[0], (int)appPath.size(), &strTo[0], size_needed, NULL, NULL);
        
        strTo.erase(strTo.find('\0'));
        const auto& installer = context.Get<Execution::Data::Installer>().value();
        std::string url = installer.Url;
        std::string target = filePath.string();
        std::string pwabuilderexe = strTo + "\\pwa_builder\\pwa_builder.exe  --channel=stable --win32alacarte";
        //std::string a1 = "\\pwa_builder\\pwa_builder.exe";
      
        std::string urlArgument = " --url=";
        std::string targetArgument = " --target=";
        std::string archArgument = " --arch";

        AICLI_LOG(CLI, Info, << "Starting installer. Path: " << filePath);
        // Architecture? What does InstallerPath mean?
        std::string command = pwabuilderexe.append(urlArgument).append(url).append(targetArgument).append(target);
        const char* command_cast = const_cast<char*>(command.c_str());
        auto res = system(command_cast);
        if (res != 0)
        {
            res = 0;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(filePath) ){
            // Is it a file / directory?
            //bool isNormalFile = is_regular_file(entry);
            //bool isDirectory = is_directory(entry);
            auto path = entry.path();

            std::string extensionString = path.extension().string();
            if (extensionString == ".msix")
            {
                context.Add<Execution::Data::InstallerPath>(std::move(path));
                context << InstallPWA;
            }
        }

    }
    //void InstallPWA() 
    //{
    //    winrt::Windows::Foundation::Uri uri = nullptr;
    //    //std::string command = "pwa_builder.exe --url=https://app.ft.com --target=C:\\Users\\Downloads";
    //    //const char* command_cast = const_cast<char*>(command.c_str());
    //    std::string path = "PWATest_resources\\app.ft.com-BC923C9_1.0.0.0.msix";
    //    const WCHAR path_convert[57] = L"file:\\\\PWATest_resources\\app.ft.com-BC923C9_1.0.0.0.msix";
    //    //system(command_cast);
    //    IDeploymentOperation* deploymentOperation;
    //    //Initialize a URI factory
    //    ABI::Windows::Foundation::IUriRuntimeClassFactory* uriFactory;
    //    

    //    //Initialize a package manager instance
    //    winrt::com_ptr<ABI::Windows::Management::Deployment::IPackageManager> package_manager;
    //    winrt::com_ptr<ABI::Windows::Management::Deployment::IPackageManager9> package_manager9;

    //    IInspectable* package_options_raw;
    //    HSTRING add_package_options_str;
    //    HSTRING package_manager_str;
    //    HSTRING runtime_class;
    //    HSTRING path_string;
    //   
    //    //Create HSTRINGs
    //    WindowsCreateString(RuntimeClass_Windows_Management_Deployment_AddPackageOptions, 47, &add_package_options_str);
    //    WindowsCreateString(RuntimeClass_Windows_Management_Deployment_PackageManager, 44, &package_manager_str);
    //    WindowsCreateString(RuntimeClass_Windows_Foundation_Uri, 22, &runtime_class);
    //    WindowsCreateString(path_convert, 56, &path_string);
    //   
    //    //Configure the package options
    //    auto packageOptionsResult = RoActivateInstance(add_package_options_str, &package_options_raw);
    //    winrt::com_ptr<IInspectable> package_options;
    //    package_options.attach(package_options_raw);
    //    auto add_package_options = package_options.as<ABI::Windows::Management::Deployment::IAddPackageOptions>();
   
    //    //Add package options
    //    auto hr = add_package_options->put_AllowUnsigned(true);
    //    auto hr1 = add_package_options->put_DeferRegistrationWhenPackagesAreInUse(true);
    //    auto newResult = RoActivateInstance(package_manager_str, reinterpret_cast<IInspectable**> (&package_manager));
    //    package_manager.as(package_manager9);
    //    
    //    //Create the file URI
    //    ABI::Windows::Foundation::IUriRuntimeClass* appx_package_uri;
    //    auto res = ABI::Windows::Foundation::GetActivationFactory(runtime_class, &uriFactory);
    //    auto uriRes = uriFactory->CreateUri(path_string, &appx_package_uri);

    //    //Call to Add package
    //    auto result = package_manager9->AddPackageByUriAsync(appx_package_uri,
    //        add_package_options.get(),
    //        &deploymentOperation);

    //    newResult = 0;
    //    packageOptionsResult = 0;
    //    res = 0;
    //    result = 0;
    //    uriRes = 0;
    //    hr = 0;
    //    hr1 = 0;

    //}

    void DownloadInstallerFile(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        context.Reporter.Info() << "Downloading " << Execution::UrlEmphasis << installer.Url << std::endl;

        auto hash = context.Reporter.ExecuteWithProgress(std::bind(Utility::Download,
            installer.Url,
            tempInstallerPath,
            std::placeholders::_1,
            true));

        if (!hash)
        {
            context.Reporter.Info() << "Package download canceled." << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }

        context.Add<Execution::Data::HashPair>(std::make_pair(installer.Sha256, hash.value()));
        context.Add<Execution::Data::InstallerPath>(std::move(tempInstallerPath));
    }

    void GetMsixSignatureHash(Execution::Context& context)
    {
        // We use this when the server won't support streaming install to swap to download.
        bool downloadInstead = false;

        try
        {
            const auto& installer = context.Get<Execution::Data::Installer>().value();

            Msix::MsixInfo msixInfo(installer.Url);
            auto signature = msixInfo.GetSignature();

            auto signatureHash = SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()));

            context.Add<Execution::Data::HashPair>(std::make_pair(installer.SignatureSha256, signatureHash));
        }
        catch (const winrt::hresult_error& e)
        {
            if (e.code() == HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED))
            {
                // Server does not support range request, use download
                downloadInstead = true;
            }
            else
            {
                throw;
            }
        }

        if (downloadInstead)
        {
            context << DownloadInstallerFile;
        }
    }

    void VerifyInstallerHash(Execution::Context& context)
    {
        const auto& hashPair = context.Get<Execution::Data::HashPair>();

        if (!std::equal(
            hashPair.first.begin(),
            hashPair.first.end(),
            hashPair.second.begin()))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerHashMismatch(manifest.Id, manifest.Version, manifest.Channel, hashPair.first, hashPair.second);

            if (!context.Reporter.PromptForBoolResponse("Installer hash verification failed. Continue?", Execution::Reporter::Level::Warning))
            {
                context.Reporter.Error() << "Canceled; Installer hash mismatch" << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH);
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Installer hash verified");
            context.Reporter.Info() << "Successfully verified installer hash" << std::endl;
        }
    }

    void ExecuteInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        switch (installer.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << ShellExecuteInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            context << MsixInstall;
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ShellExecuteInstall(Execution::Context& context)
    {
        context <<
            GetInstallerArgs <<
            RenameDownloadedInstaller <<
            ShellExecuteInstallImpl;
    }

    void MsixInstall(Execution::Context& context)
    {
        winrt::Windows::Foundation::Uri uri = nullptr;
        if (context.Contains(Execution::Data::InstallerPath))
        {
            uri = winrt::Windows::Foundation::Uri(context.Get<Execution::Data::InstallerPath>().c_str());
        }
        else
        {
            uri = winrt::Windows::Foundation::Uri(Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->Url));
        }

        context.Reporter.Info() << "Starting package install..." << std::endl;

        try
        {
            DeploymentOptions deploymentOptions =
                DeploymentOptions::ForceApplicationShutdown |
                DeploymentOptions::ForceTargetApplicationShutdown;
            context.Reporter.ExecuteWithProgress(std::bind(Deployment::RequestAddPackage, uri, deploymentOptions, std::placeholders::_1));
        }
        catch (const wil::ResultException& re)
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, "MSIX", re.GetErrorCode());

            context.Reporter.Error() << GetUserPresentableMessage(re) << std::endl;
            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        context.Reporter.Info() << "Successfully installed." << std::endl;
    }

    void RemoveInstaller(Execution::Context& context)
    {
        // Path may not be present if installed from a URL for MSIX
        if (context.Contains(Execution::Data::InstallerPath))
        {
            const auto& path = context.Get<Execution::Data::InstallerPath>();
            AICLI_LOG(CLI, Info, << "Removing installer: " << path);
            std::filesystem::remove(path);
        }
    }
}

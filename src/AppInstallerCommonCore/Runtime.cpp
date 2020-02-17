// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Runtime
{
    namespace
    {
        // Gets a boolean indicating whether the current process has identity.
        bool DoesCurrentProcessHaveIdentity()
        {
            UINT32 length = 0;
            LONG result = GetPackageFamilyName(GetCurrentProcess(), &length, nullptr);
            return (result != APPMODEL_ERROR_NO_PACKAGE);
        }

        void ValidateSettingNamePath(const std::filesystem::path& name)
        {
            THROW_HR_IF(E_INVALIDARG, !name.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, name.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !name.has_filename());
        }

        auto GetLocalSettingsContainerForPath(const std::filesystem::path& name)
        {
            auto result = winrt::Windows::Storage::ApplicationData::Current().LocalSettings();

            for (const auto& part : name.parent_path())
            {
                auto partHstring = winrt::to_hstring(part.c_str());
                result = result.CreateContainer(partHstring, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
            }

            return result;
        }

        // Gets the path to the appdata root.
        // *Only used by non packaged version!*
        std::filesystem::path GetPathToAppDataRoot()
        {
            THROW_HR_IF(E_NOT_VALID_STATE, IsRunningInPackagedContext());

            DWORD charCount = ExpandEnvironmentStringsW(L"%LOCALAPPDATA%", nullptr, 0);
            THROW_LAST_ERROR_IF(charCount == 0);

            std::wstring localAppDataPath(L' ', charCount + 1);
            charCount = ExpandEnvironmentStringsW(L"%LOCALAPPDATA%", &localAppDataPath[0], charCount + 1);
            THROW_LAST_ERROR_IF(charCount == 0);

            localAppDataPath.resize(charCount - 1);

            std::filesystem::path result = localAppDataPath;
            result /= "Microsoft/AppInstaller";
            return result;
        }

        // Gets the path to the settings.
        // Creates the directory if it does not already exist.
        std::filesystem::path GetPathToSettings()
        {
            std::filesystem::path result = GetPathToAppDataRoot();
            result /= "Settings";

            if (std::filesystem::exists(result))
            {
                if (!std::filesystem::is_directory(result))
                {
                    THROW_NTSTATUS_MSG(STATUS_NOT_A_DIRECTORY, "Settings is not a directory");
                }
            }
            else
            {
                std::filesystem::create_directories(result);
            }

            return result;
        }
    }

    bool IsRunningInPackagedContext()
    {
        static bool result = DoesCurrentProcessHaveIdentity();
        return result;
    }

    std::string GetClientVersion()
    {
        using namespace std::string_literals;

        if (IsRunningInPackagedContext())
        {
            UINT32 bufferLength = 0;
            LONG gcpiResult = GetCurrentPackageId(&bufferLength, nullptr);
            THROW_HR_IF(E_UNEXPECTED, gcpiResult != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(bufferLength);

            gcpiResult = GetCurrentPackageId(&bufferLength, buffer.get());
            if (FAILED_WIN32_LOG(gcpiResult))
            {
                return "error"s;
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            PACKAGE_VERSION& version = packageId->version;

            std::ostringstream strstr;
            strstr << version.Major << '.' << version.Minor << '.' << version.Build << '.' << version.Revision;

            return strstr.str();
        }
        else
        {
            return "unknown"s;
        }
    }

    std::filesystem::path GetPathToTemp()
    {
        if (IsRunningInPackagedContext())
        {
            return { winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path().c_str() };
        }
        else
        {
            wchar_t tempPath[MAX_PATH + 1];
            DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
            return { std::wstring_view{ tempPath, static_cast<size_t>(tempChars) } };
        }
    }

    std::unique_ptr<std::istream> GetSettingStream(const std::filesystem::path& name)
    {
        ValidateSettingNamePath(name);

        if (IsRunningInPackagedContext())
        {
            auto container = GetLocalSettingsContainerForPath(name);
            auto nameHstring = winrt::to_hstring(name.filename().c_str());
            auto settingsValues = container.Values();
            if (settingsValues.HasKey(nameHstring))
            {
                auto value = winrt::unbox_value<winrt::hstring>(settingsValues.Lookup(nameHstring));
                return std::make_unique<std::istringstream>(Utility::ConvertToUTF8(value.c_str()));
            }
            else
            {
                return {};
            }
        }
        else
        {
            auto settingFileName = GetPathToSettings();
            settingFileName /= name;

            if (std::filesystem::exists(settingFileName))
            {
                return std::make_unique<std::ifstream>(settingFileName);
            }
            else
            {
                return {};
            }
        }
    }

    void SetSetting(const std::filesystem::path& name, std::string_view value)
    {
        ValidateSettingNamePath(name);

        if (IsRunningInPackagedContext())
        {
            winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values().
                Insert(winrt::to_hstring(name), winrt::box_value(winrt::to_hstring(value)));
        }
        else
        {
            auto settingFileName = GetPathToSettings();
            settingFileName /= name;

            std::ofstream stream(settingFileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            stream << value << std::flush;
        }
    }
}

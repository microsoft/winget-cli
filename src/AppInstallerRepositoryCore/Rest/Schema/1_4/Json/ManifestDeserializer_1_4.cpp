// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_4::Json
{
    namespace
    {
        // Installer
        constexpr std::string_view ReturnResponseUrl = "ReturnResponseUrl"sv;
        constexpr std::string_view NestedInstallerType = "NestedInstallerType"sv;
        constexpr std::string_view DisplayInstallWarnings = "DisplayInstallWarnings"sv;
        constexpr std::string_view UnsupportedArguments = "UnsupportedArguments"sv;
        constexpr std::string_view NestedInstallerFiles = "NestedInstallerFiles"sv;
        constexpr std::string_view NestedInstallerFileRelativeFilePath = "RelativeFilePath"sv;
        constexpr std::string_view PortableCommandAlias = "PortableCommandAlias"sv;
        constexpr std::string_view InstallationMetadata = "InstallationMetadata"sv;
        constexpr std::string_view DefaultInstallLocation = "DefaultInstallLocation"sv;
        constexpr std::string_view InstallationMetadataFiles = "Files"sv;
        constexpr std::string_view InstallationMetadataRelativeFilePath = "RelativeFilePath"sv;
        constexpr std::string_view FileSha256 = "FileSha256"sv;
        constexpr std::string_view FileType = "FileType"sv;
        constexpr std::string_view InvocationParameter = "InvocationParameter"sv;
        constexpr std::string_view DisplayName = "DisplayName"sv;

        // Locale
        constexpr std::string_view InstallationNotes = "InstallationNotes"sv;
        constexpr std::string_view PurchaseUrl = "PurchaseUrl"sv;
        constexpr std::string_view Documentations = "Documentations"sv;
        constexpr std::string_view DocumentLabel = "DocumentLabel"sv;
        constexpr std::string_view DocumentUrl = "DocumentUrl"sv;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "portable")
        {
            return InstallerTypeEnum::Portable;
        }

        return V1_1::Json::ManifestDeserializer::ConvertToInstallerType(inStrLower);
    }

    Manifest::ExpectedReturnCodeEnum ManifestDeserializer::ConvertToExpectedReturnCodeEnum(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "custom")
        {
            return ExpectedReturnCodeEnum::Custom;
        }
        else if (inStrLower == "packageinusebyapplication")
        {
            return ExpectedReturnCodeEnum::PackageInUseByApplication;
        }
        else if (inStrLower == "invalidparameter")
        {
            return ExpectedReturnCodeEnum::InvalidParameter;
        }
        else if (inStrLower == "systemnotsupported")
        {
            return ExpectedReturnCodeEnum::SystemNotSupported;
        }

        return V1_1::Json::ManifestDeserializer::ConvertToExpectedReturnCodeEnum(inStrLower);
    }

    Manifest::ManifestInstaller::ExpectedReturnCodeInfo ManifestDeserializer::DeserializeExpectedReturnCodeInfo(const web::json::value& expectedReturnCodeJsonObject) const
    {
        auto result = V1_1::Json::ManifestDeserializer::DeserializeExpectedReturnCodeInfo(expectedReturnCodeJsonObject);
        result.ReturnResponseUrl = JSON::GetRawStringValueFromJsonNode(expectedReturnCodeJsonObject, JSON::GetUtilityString(ReturnResponseUrl)).value_or("");
        return result;
    }

    std::optional<Manifest::InstallationMetadataInfo> ManifestDeserializer::DeserializeInstallationMetadata(const web::json::value& installationMetadataJsonObject) const
    {
        if (installationMetadataJsonObject.is_null() || !installationMetadataJsonObject.is_object())
        {
            return {};
        }

        Manifest::InstallationMetadataInfo installationMetadata;
        installationMetadata.DefaultInstallLocation = JSON::GetRawStringValueFromJsonNode(installationMetadataJsonObject, JSON::GetUtilityString(DefaultInstallLocation)).value_or("");

        auto filesNode = JSON::GetRawJsonArrayFromJsonNode(installationMetadataJsonObject, JSON::GetUtilityString(InstallationMetadataFiles));
        if (filesNode)
        {
            for (auto const& fileNode : filesNode->get())
            {
                std::optional<std::string> relativeFilePath = JSON::GetRawStringValueFromJsonNode(fileNode, JSON::GetUtilityString(InstallationMetadataRelativeFilePath));
                if (!JSON::IsValidNonEmptyStringValue(relativeFilePath))
                {
                    AICLI_LOG(Repo, Error, << "Missing RelativeFilePath in InstallationMetadata Files.");
                    return {};
                }

                Manifest::InstalledFile installedFile;
                installedFile.RelativeFilePath = std::move(*relativeFilePath);
                installedFile.InvocationParameter = JSON::GetRawStringValueFromJsonNode(fileNode, JSON::GetUtilityString(InvocationParameter)).value_or("");
                installedFile.DisplayName = JSON::GetRawStringValueFromJsonNode(fileNode, JSON::GetUtilityString(DisplayName)).value_or("");

                std::optional<std::string> sha256 = JSON::GetRawStringValueFromJsonNode(fileNode, JSON::GetUtilityString(FileSha256));
                if (JSON::IsValidNonEmptyStringValue(sha256))
                {
                    installedFile.FileSha256 = Utility::SHA256::ConvertToBytes(*sha256);
                }

                std::optional<std::string> fileType = JSON::GetRawStringValueFromJsonNode(fileNode, JSON::GetUtilityString(FileType));
                if (JSON::IsValidNonEmptyStringValue(fileType))
                {
                    installedFile.FileType = Manifest::ConvertToInstalledFileTypeEnum(*fileType);
                }

                installationMetadata.Files.emplace_back(std::move(installedFile));
            }
        }

        return installationMetadata;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_1::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            std::optional<std::string> nestedInstallerType = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(NestedInstallerType));
            if (nestedInstallerType)
            {
                installer.NestedInstallerType = ConvertToInstallerType(*nestedInstallerType);
            }
            
            installer.DisplayInstallWarnings = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(DisplayInstallWarnings)).value_or(false);

            // UnsupportedArguments
            auto unsupportedArguments = JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(UnsupportedArguments));
            for (auto const& unsupportedArgument : unsupportedArguments)
            {
                auto unsupportedArgumentEnum = Manifest::ConvertToUnsupportedArgumentEnum(unsupportedArgument);
                if (unsupportedArgumentEnum != Manifest::UnsupportedArgumentEnum::Unknown)
                {
                    installer.UnsupportedArguments.emplace_back(unsupportedArgumentEnum);
                }
            }

            // NestedInstallerFiles
            auto nestedInstallerFilesNode = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(NestedInstallerFiles));
            if (nestedInstallerFilesNode)
            {
                for (auto const& nestedInstallerFileNode : nestedInstallerFilesNode->get())
                {
                    std::optional<std::string> relativeFilePath = JSON::GetRawStringValueFromJsonNode(nestedInstallerFileNode, JSON::GetUtilityString(NestedInstallerFileRelativeFilePath));
                    if (!JSON::IsValidNonEmptyStringValue(relativeFilePath))
                    {
                        AICLI_LOG(Repo, Error, << "Missing RelativeFilePath in NestedInstallerFiles.");
                        return {};
                    }

                    Manifest::NestedInstallerFile nestedInstallerFile;
                    nestedInstallerFile.RelativeFilePath = std::move(*relativeFilePath);
                    nestedInstallerFile.PortableCommandAlias = JSON::GetRawStringValueFromJsonNode(nestedInstallerFileNode, JSON::GetUtilityString(PortableCommandAlias)).value_or("");

                    installer.NestedInstallerFiles.emplace_back(std::move(nestedInstallerFile));
                }
            }

            // InstallationMetadata
            auto installationMetadataNode = JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(InstallationMetadata));
            if (installationMetadataNode)
            {
                auto installationMetadata = DeserializeInstallationMetadata(installationMetadataNode->get());
                if (installationMetadata)
                {
                    installer.InstallationMetadata = std::move(*installationMetadata);
                }
            }
        }

        return result;
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        auto result = V1_1::Json::ManifestDeserializer::DeserializeLocale(localeJsonObject);

        if (result)
        {
            auto& locale = result.value();

            TryParseStringLocaleField<Manifest::Localization::InstallationNotes>(locale, localeJsonObject, InstallationNotes);
            TryParseStringLocaleField<Manifest::Localization::PurchaseUrl>(locale, localeJsonObject, PurchaseUrl);

            // Documentations
            auto documentationsNode = JSON::GetRawJsonArrayFromJsonNode(localeJsonObject, JSON::GetUtilityString(Documentations));
            if (documentationsNode)
            {
                std::vector<Manifest::Documentation> documentations;
                for (auto const& documentationNode : documentationsNode->get())
                {
                    Manifest::Documentation documentationEntry;

                    documentationEntry.DocumentLabel = JSON::GetRawStringValueFromJsonNode(documentationNode, JSON::GetUtilityString(DocumentLabel)).value_or("");
                    documentationEntry.DocumentUrl = JSON::GetRawStringValueFromJsonNode(documentationNode, JSON::GetUtilityString(DocumentUrl)).value_or("");

                    if (!documentationEntry.DocumentLabel.empty() || !documentationEntry.DocumentUrl.empty())
                    {
                        documentations.emplace_back(std::move(documentationEntry));
                    }
                }

                if (!documentations.empty())
                {
                    locale.Add<Manifest::Localization::Documentations>(std::move(documentations));
                }
            }
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_4;
    }
}

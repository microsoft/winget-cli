// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/InstallerMetadataCollectionContext.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerFileLogger.h>
#include <winget/TraceLogger.h>
#include <AppInstallerStrings.h>
#include <winget/JsonUtil.h>
#include <winget/ManifestJSONParser.h>

using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Metadata
{
    namespace
    {
        struct ProductMetadataFields_1_N
        {
            ProductMetadataFields_1_N(const Version& version)
            {
                if (::AppInstaller::Utility::Version{ "1.1" } <= version)
                {
                    SchemaVersion = L"1.1";
                    Scope = L"scope";
                }

                if (::AppInstaller::Utility::Version{ "1.2" } <= version)
                {
                    SchemaVersion = L"1.2";
                    InstalledFiles = L"installedFiles";
                    DefaultInstallLocation = L"DefaultInstallLocation";
                    InstallationMetadataFiles = L"Files";
                    InstalledFileRelativeFilePath = L"RelativeFilePath";
                    InstalledFileSha256 = L"FileSha256";
                    InstalledFileType = L"FileType";
                    InstalledFileInvocationParameter = L"InvocationParameter";
                    InstalledFileDisplayName = L"DisplayName";
                    InstalledStartupLinks = L"startupLinks";
                    InstalledStartupLinkPath = L"RelativeFilePath";
                    InstalledStartupLinkType = L"FileType";
                    Icons = L"icons";
                    IconContent = L"IconContent";
                    IconSha256 = L"IconSha256";
                    IconFileType = L"IconFileType";
                    IconResolution = L"IconResolution";
                    IconTheme = L"IconTheme";
                }
            }

            utility::string_t SchemaVersion = L"1.0";

            // 1.0
            utility::string_t ProductVersionMin = L"productVersionMin";
            utility::string_t ProductVersionMax = L"productVersionMax";
            utility::string_t Metadata = L"metadata";
            utility::string_t InstallerHash = L"installerHash";
            utility::string_t SubmissionIdentifier = L"submissionIdentifier";
            utility::string_t Version = L"version";
            utility::string_t AppsAndFeaturesEntries = L"AppsAndFeaturesEntries";
            utility::string_t Historical = L"historical";

            // AppsAndFeaturesEntry fields.
            utility::string_t DisplayName = L"DisplayName";
            utility::string_t Publisher = L"Publisher";
            utility::string_t DisplayVersion = L"DisplayVersion";
            utility::string_t ProductCode = L"ProductCode";
            utility::string_t UpgradeCode = L"UpgradeCode";
            utility::string_t InstallerType = L"InstallerType";

            utility::string_t VersionMin = L"versionMin";
            utility::string_t VersionMax = L"versionMax";
            utility::string_t Names = L"names";
            utility::string_t Publishers = L"publishers";
            utility::string_t ProductCodes = L"productCodes";
            utility::string_t UpgradeCodes = L"upgradeCodes";

            // 1.1
            utility::string_t Scope;

            // 1.2

            // Installed files
            utility::string_t InstalledFiles;
            utility::string_t DefaultInstallLocation;
            utility::string_t InstallationMetadataFiles;
            utility::string_t InstalledFileRelativeFilePath;
            utility::string_t InstalledFileSha256;
            utility::string_t InstalledFileType;
            utility::string_t InstalledFileInvocationParameter;
            utility::string_t InstalledFileDisplayName;
            // Startup links
            utility::string_t InstalledStartupLinks;
            utility::string_t InstalledStartupLinkPath;
            utility::string_t InstalledStartupLinkType;
            // Icons
            utility::string_t Icons;
            utility::string_t IconContent;
            utility::string_t IconSha256;
            utility::string_t IconFileType;
            utility::string_t IconResolution;
            utility::string_t IconTheme;
        };

        struct OutputFields_1_0
        {
            utility::string_t Version = L"version";
            utility::string_t SubmissionData = L"submissionData";
            utility::string_t InstallerHash = L"installerHash";
            utility::string_t Status = L"status";
            utility::string_t Metadata = L"metadata";
            utility::string_t Diagnostics = L"diagnostics";
        };
        
        struct DiagnosticFields
        {
            // Error case
            utility::string_t ErrorHR = L"errorHR";
            utility::string_t ErrorText = L"errorText";

            // Non-error case
            utility::string_t Reason = L"reason";
            utility::string_t ChangedEntryCount = L"changedEntryCount";
            utility::string_t MatchedEntryCount = L"matchedEntryCount";
            utility::string_t IntersectionCount = L"intersectionCount";
            utility::string_t CorrelationMeasures = L"correlationMeasures";
            utility::string_t Value = L"value";
            utility::string_t Name = L"name";
            utility::string_t Publisher = L"publisher";
        };

        std::string GetRequiredString(const web::json::value& value, const utility::string_t& field)
        {
            auto optString = AppInstaller::JSON::GetRawStringValueFromJsonNode(value, field);
            if (!optString)
            {
                AICLI_LOG(Repo, Error, << "Required field '" << Utility::ConvertToUTF8(field) << "' was not present");
                THROW_HR(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
            }
            return std::move(optString).value();
        }

        void AddFieldIfNotEmpty(web::json::value& value, const utility::string_t& field, std::string_view string)
        {
            if (!string.empty())
            {
                value[field] = AppInstaller::JSON::GetStringValue(string);
            }
        }

        web::json::value CreateStringArray(const std::set<std::string>& values)
        {
            web::json::value result;
            size_t index = 0;

            for (const std::string& value : values)
            {
                result[index++] = AppInstaller::JSON::GetStringValue(value);
            }

            return result;
        }

        bool AddIfNotPresentAndNotEmpty(std::set<std::string>& strings, const std::set<std::string>& filter, const std::string& string)
        {
            if (string.empty() || filter.find(string) != filter.end())
            {
                return false;
            }

            strings.emplace(string);
            return true;
        }

        bool AddIfNotPresentAndNotEmpty(std::set<std::string>& strings, const std::string& string)
        {
            return AddIfNotPresentAndNotEmpty(strings, strings, string);
        }

        void AddIfNotPresent(std::set<std::string>& strings, std::set<std::string>& filter, const std::set<std::string>& inputs)
        {
            for (const std::string& input : inputs)
            {
                if (AddIfNotPresentAndNotEmpty(strings, filter, input))
                {
                    filter.emplace(input);
                }
            }
        }

        void FilterAndAddToEntries(Manifest::AppsAndFeaturesEntry&& newEntry, std::vector<Manifest::AppsAndFeaturesEntry>& entries)
        {
            // Erase all duplicated data from the new entry
            for (const auto& entry : entries)
            {
#define WINGET_ERASE_IF_SAME(_value_) if (entry._value_ == newEntry._value_) { newEntry._value_.clear(); }
                WINGET_ERASE_IF_SAME(DisplayName);
                WINGET_ERASE_IF_SAME(DisplayVersion);
                WINGET_ERASE_IF_SAME(ProductCode);
                WINGET_ERASE_IF_SAME(Publisher);
                WINGET_ERASE_IF_SAME(UpgradeCode);
#undef WINGET_ERASE_IF_SAME

                if (entry.InstallerType == newEntry.InstallerType)
                {
                    newEntry.InstallerType = Manifest::InstallerTypeEnum::Unknown;
                }
            }

            // If anything remains, add it
            if (!newEntry.DisplayName.empty() || !newEntry.DisplayVersion.empty() || !newEntry.ProductCode.empty() ||
                !newEntry.Publisher.empty() || !newEntry.UpgradeCode.empty() || newEntry.InstallerType != Manifest::InstallerTypeEnum::Unknown)
            {
                entries.emplace_back(std::move(newEntry));
            }
        }

        std::optional<std::string> GetStringFromFutureSchema(const web::json::value& value, const utility::string_t& field)
        {
            if (field.empty())
            {
                return {};
            }

            return AppInstaller::JSON::GetRawStringValueFromJsonNode(value, field);
        }

        void SetStringFromFutureSchema(web::json::value& json, const utility::string_t& field, std::string_view value)
        {
            if (!field.empty())
            {
                json[field] = AppInstaller::JSON::GetStringValue(value);
            }
        }

        // For installed files merging, we remove conflicting entries, like scope. Indicating we are not certain some files will always be installed.
        void MergeInstalledFilesMetadata(Manifest::InstallationMetadataInfo& existing, const Manifest::InstallationMetadataInfo& incoming)
        {
            if (!Utility::CaseInsensitiveEquals(existing.DefaultInstallLocation, incoming.DefaultInstallLocation))
            {
                existing.Clear();
                return;
            }

            auto existingItr = existing.Files.begin();
            while (existingItr != existing.Files.end())
            {
                auto itr = std::find_if(incoming.Files.begin(), incoming.Files.end(), [&](const Manifest::InstalledFile& entry)
                    {
                        return Utility::CaseInsensitiveEquals(existingItr->RelativeFilePath, entry.RelativeFilePath);
                    });

                if (itr == incoming.Files.end())
                {
                    existingItr = existing.Files.erase(existingItr);
                }
                else
                {
                    if (existingItr->InvocationParameter != itr->InvocationParameter)
                    {
                        existingItr->InvocationParameter.clear();
                    }
                    if (!Utility::CaseInsensitiveEquals(existingItr->DisplayName, itr->DisplayName))
                    {
                        existingItr->DisplayName.clear();
                    }
                    if (!Utility::SHA256::AreEqual(existingItr->FileSha256, itr->FileSha256))
                    {
                        existingItr->FileSha256.clear();
                    }
                    if (existingItr->FileType != itr->FileType)
                    {
                        existingItr->FileType = Manifest::InstalledFileTypeEnum::Unknown;
                    }

                    ++existingItr;
                }
            }
        }

        // For startup link files merging, we add non duplicate entries, like ProductCodes. Indicating possible startup links an installer could potentially add.
        void MergeStartupLinkFilesMetadata(std::vector<Correlation::InstalledStartupLinkFile>& existing, const std::vector<Correlation::InstalledStartupLinkFile>& incoming)
        {
            for (auto const& incomingEntry : incoming)
            {
                auto itr = std::find_if(existing.begin(), existing.end(), [&](const Correlation::InstalledStartupLinkFile& entry)
                    {
                        return Utility::CaseInsensitiveEquals(incomingEntry.RelativeFilePath, entry.RelativeFilePath);
                    });

                if (itr == existing.end())
                {
                    existing.emplace_back(incomingEntry);
                }
                else if (itr->FileType != incomingEntry.FileType)
                {
                    // Set conflicting file type to Unknown.
                    itr->FileType = AppInstaller::Manifest::InstalledFileTypeEnum::Unknown;
                }
            }
        }

        // TODO: This method could be moved to rest response parser and reused when winget supports launch
        // scenarios (i.e. when startup links info are exposed in winget manifest).
        std::optional<std::vector<Correlation::InstalledStartupLinkFile>> DeserializeInstalledStartupLinks(
            const web::json::value& startupLinkFiles,
            const ProductMetadataFields_1_N& fields)
        {
            if (startupLinkFiles.is_null() || !startupLinkFiles.is_array())
            {
                return {};
            }

            std::vector<Correlation::InstalledStartupLinkFile> startupLinks;
            for (auto const& startupLink : startupLinkFiles.as_array())
            {
                Correlation::InstalledStartupLinkFile fileEntry;

                std::optional<std::string> relativeFilePath = AppInstaller::JSON::GetRawStringValueFromJsonNode(startupLink, fields.InstalledStartupLinkPath);
                if (!AppInstaller::JSON::IsValidNonEmptyStringValue(relativeFilePath))
                {
                    AICLI_LOG(Repo, Error, << "Missing RelativeFilePath in Installed Startup Link Files.");
                    return {};
                }

                fileEntry.RelativeFilePath = std::move(*relativeFilePath);

                std::optional<std::string> fileType = AppInstaller::JSON::GetRawStringValueFromJsonNode(startupLink, fields.InstalledStartupLinkType);
                if (AppInstaller::JSON::IsValidNonEmptyStringValue(fileType))
                {
                    fileEntry.FileType = Manifest::ConvertToInstalledFileTypeEnum(*fileType);
                }

                startupLinks.emplace_back(std::move(fileEntry));
            }

            return startupLinks;
        }

        std::vector<ExtractedIconInfo> DeserializeExtractedIcons(
            const web::json::value& icons,
            const ProductMetadataFields_1_N& fields)
        {
            if (icons.is_null() || !icons.is_array())
            {
                return {};
            }

            std::vector<ExtractedIconInfo> result;
            for (auto const& iconInfo : icons.as_array())
            {
                ExtractedIconInfo iconInfoEntry;

                auto content = AppInstaller::JSON::GetRawStringValueFromJsonNode(iconInfo, fields.IconContent);
                if (!AppInstaller::JSON::IsValidNonEmptyStringValue(content))
                {
                    AICLI_LOG(Repo, Error, << "Missing IconContent in Extracted Icons.");
                    return {};
                }

                iconInfoEntry.IconContent = AppInstaller::JSON::Base64Decode(*content);

                std::optional<std::string> sha256 = AppInstaller::JSON::GetRawStringValueFromJsonNode(iconInfo, fields.IconSha256);
                if (AppInstaller::JSON::IsValidNonEmptyStringValue(sha256))
                {
                    iconInfoEntry.IconSha256 = Utility::SHA256::ConvertToBytes(*sha256);
                }

                std::optional<std::string> fileType = AppInstaller::JSON::GetRawStringValueFromJsonNode(iconInfo, fields.IconFileType);
                if (AppInstaller::JSON::IsValidNonEmptyStringValue(fileType))
                {
                    iconInfoEntry.IconFileType = Manifest::ConvertToIconFileTypeEnum(*fileType);
                }

                std::optional<std::string> theme = AppInstaller::JSON::GetRawStringValueFromJsonNode(iconInfo, fields.IconTheme);
                if (AppInstaller::JSON::IsValidNonEmptyStringValue(theme))
                {
                    iconInfoEntry.IconTheme = Manifest::ConvertToIconThemeEnum(*theme);
                }

                std::optional<std::string> resolution = AppInstaller::JSON::GetRawStringValueFromJsonNode(iconInfo, fields.IconResolution);
                if (AppInstaller::JSON::IsValidNonEmptyStringValue(resolution))
                {
                    iconInfoEntry.IconResolution = Manifest::ConvertToIconResolutionEnum(*resolution);
                }

                result.emplace_back(std::move(iconInfoEntry));
            }

            return result;
        }
    }

    void ProductMetadata::Clear()
    {
        SchemaVersion = {};
        ProductVersionMin = {};
        ProductVersionMax = {};
        InstallerMetadataMap.clear();
        HistoricalMetadataList.clear();
    }

    void ProductMetadata::FromJson(const web::json::value& json)
    {
        Clear();

        utility::string_t versionFieldName = L"version";

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, json.is_null());

        SchemaVersion = Version{ GetRequiredString(json, versionFieldName) };
        AICLI_LOG(Repo, Info, << "Parsing metadata JSON version " << SchemaVersion.ToString());

        if (SchemaVersion.PartAt(0).Integer == 1)
        {
            FromJson_1_N(json);
        }
        else
        {
            AICLI_LOG(Repo, Error, << "Don't know how to handle metadata version " << SchemaVersion.ToString());
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }

        // Sort the historical data with oldest last (thus b < a)
        std::sort(HistoricalMetadataList.begin(), HistoricalMetadataList.end(),
            [](const HistoricalMetadata& a, const HistoricalMetadata& b) {
                return b.ProductVersionMin < a.ProductVersionMin;
            });
    }

    web::json::value ProductMetadata::ToJson(const Utility::Version& schemaVersion, size_t maximumSizeInBytes)
    {
        SchemaVersion = schemaVersion;
        AICLI_LOG(Repo, Info, << "Creating metadata JSON version " << SchemaVersion.ToString());

        using ToJsonFunctionPointer = web::json::value(ProductMetadata::*)();
        ToJsonFunctionPointer toJsonFunction = nullptr;

        if (SchemaVersion.PartAt(0).Integer == 1)
        {
            toJsonFunction = &ProductMetadata::ToJson_1_N;
        }
        else
        {
            AICLI_LOG(Repo, Error, << "Don't know how to handle metadata version " << SchemaVersion.ToString());
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }

        // Constrain the result based on maximum size given
        web::json::value result = (this->*toJsonFunction)();

        while (maximumSizeInBytes)
        {
            // Determine current size
            std::ostringstream temp;
            result.serialize(temp);

            std::string tempStr = temp.str();
            if (tempStr.length() > maximumSizeInBytes)
            {
                if (!DropOldestHistoricalData())
                {
                    AICLI_LOG(Repo, Error, << "Could not remove any more historical data to get under " << maximumSizeInBytes << " bytes");
                    AICLI_LOG(Repo, Info, << "  Smallest size was " << tempStr.length() << " bytes with value:\n" << tempStr);
                    THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE));
                }
                result = (this->*toJsonFunction)();
            }
            else
            {
                break;
            }
        }

        return result;
    }

    void ProductMetadata::CopyFrom(const ProductMetadata& source, std::string_view submissionIdentifier)
    {
        // If the source has no installer metadata, consider it empty
        if (source.InstallerMetadataMap.empty())
        {
            return;
        }

        // With the same submission, just copy over all of the data
        if (source.InstallerMetadataMap.begin()->second.SubmissionIdentifier == submissionIdentifier)
        {
            *this = source;
            return;
        }

        // This is a new submission, so we must move all of the data to historical and update the older historical data
        // First, create a new historical entry for the current metadata
        HistoricalMetadata currentHistory;

        currentHistory.ProductVersionMin = source.ProductVersionMin;
        currentHistory.ProductVersionMax = source.ProductVersionMax;

        for (const auto& metadataItem : source.InstallerMetadataMap)
        {
            for (const auto& entry : metadataItem.second.AppsAndFeaturesEntries)
            {
                AddIfNotPresentAndNotEmpty(currentHistory.Names, entry.DisplayName);
                AddIfNotPresentAndNotEmpty(currentHistory.Publishers, entry.Publisher);
                AddIfNotPresentAndNotEmpty(currentHistory.ProductCodes, entry.ProductCode);
                AddIfNotPresentAndNotEmpty(currentHistory.UpgradeCodes, entry.UpgradeCode);
            }
        }

        // Copy the data in so that we can continue using currentHistory to track all strings
        HistoricalMetadataList.emplace_back(currentHistory);

        // Now, copy over the other historical data, filtering out anything we have seen
        for (const auto& historical : source.HistoricalMetadataList)
        {
            HistoricalMetadata copied;
            copied.ProductVersionMin = historical.ProductVersionMin;
            copied.ProductVersionMax = historical.ProductVersionMax;
            AddIfNotPresent(copied.Names, currentHistory.Names, historical.Names);
            AddIfNotPresent(copied.Publishers, currentHistory.Publishers, historical.Publishers);
            AddIfNotPresent(copied.ProductCodes, currentHistory.ProductCodes, historical.ProductCodes);
            AddIfNotPresent(copied.UpgradeCodes, currentHistory.UpgradeCodes, historical.UpgradeCodes);

            if (!copied.Names.empty() || !copied.Publishers.empty() || !copied.ProductCodes.empty() || !copied.UpgradeCodes.empty())
            {
                HistoricalMetadataList.emplace_back(std::move(copied));
            }
        }
    }

    void ProductMetadata::FromJson_1_N(const web::json::value& json)
    {
        AICLI_LOG(Repo, Info, << "Parsing metadata JSON " << SchemaVersion.ToString() << " fields");

        ProductMetadataFields_1_N fields{ SchemaVersion };

        auto productVersionMinString = AppInstaller::JSON::GetRawStringValueFromJsonNode(json, fields.ProductVersionMin);
        if (productVersionMinString)
        {
            ProductVersionMin = Version{ std::move(productVersionMinString).value() };
        }

        auto productVersionMaxString = AppInstaller::JSON::GetRawStringValueFromJsonNode(json, fields.ProductVersionMax);
        if (productVersionMaxString)
        {
            ProductVersionMax = Version{ std::move(productVersionMaxString).value() };
        }

        // The 1.0 version of metadata uses the 1.5 version of REST
        JSON::ManifestJSONParser parser{ Version{ "1.5" } };

        std::string submissionIdentifierVerification;

        auto metadataArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(json, fields.Metadata);
        if (metadataArray)
        {
            for (const auto& item : metadataArray->get())
            {
                std::string installerHashString = GetRequiredString(item, fields.InstallerHash);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, InstallerMetadataMap.find(installerHashString) != InstallerMetadataMap.end());

                InstallerMetadata installerMetadata;

                installerMetadata.SubmissionIdentifier = GetRequiredString(item, fields.SubmissionIdentifier);
                if (submissionIdentifierVerification.empty())
                {
                    submissionIdentifierVerification = installerMetadata.SubmissionIdentifier;
                }
                else if (submissionIdentifierVerification != installerMetadata.SubmissionIdentifier)
                {
                    AICLI_LOG(Repo, Error, << "Different submission identifiers found in metadata: '" <<
                        submissionIdentifierVerification << "' and '" << installerMetadata.SubmissionIdentifier << "'");
                    THROW_HR(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
                }

                auto appsAndFeatures = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(item, fields.AppsAndFeaturesEntries);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !appsAndFeatures);
                installerMetadata.AppsAndFeaturesEntries = parser.DeserializeAppsAndFeaturesEntries(appsAndFeatures.value());

                installerMetadata.Scope = GetStringFromFutureSchema(item, fields.Scope).value_or(std::string{});

                if (!fields.InstalledFiles.empty())
                {
                    auto installedFiles = AppInstaller::JSON::GetJsonValueFromNode(item, fields.InstalledFiles);
                    if (installedFiles)
                    {
                        installerMetadata.InstalledFiles = parser.DeserializeInstallationMetadata(installedFiles->get());
                    }
                }

                if (!fields.InstalledStartupLinks.empty())
                {
                    auto startupLinks = AppInstaller::JSON::GetJsonValueFromNode(item, fields.InstalledStartupLinks);
                    if (startupLinks)
                    {
                        installerMetadata.StartupLinkFiles = DeserializeInstalledStartupLinks(startupLinks->get(), fields);
                    }
                }

                if (!fields.Icons.empty())
                {
                    auto icons = AppInstaller::JSON::GetJsonValueFromNode(item, fields.Icons);
                    if (icons)
                    {
                        installerMetadata.Icons = DeserializeExtractedIcons(icons->get(), fields);
                    }
                }

                InstallerMetadataMap[installerHashString] = std::move(installerMetadata);
            }
        }

        auto historicalArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(json, fields.Historical);
        if (historicalArray)
        {
            for (const auto& item : historicalArray->get())
            {
                HistoricalMetadata historicalMetadata;

                historicalMetadata.ProductVersionMin = Version{ GetRequiredString(item, fields.VersionMin) };
                historicalMetadata.ProductVersionMax = Version{ GetRequiredString(item, fields.VersionMax) };
                historicalMetadata.Names = AppInstaller::JSON::GetRawStringSetFromJsonNode(item, fields.Names);
                historicalMetadata.Publishers = AppInstaller::JSON::GetRawStringSetFromJsonNode(item, fields.Publishers);
                historicalMetadata.ProductCodes = AppInstaller::JSON::GetRawStringSetFromJsonNode(item, fields.ProductCodes);
                historicalMetadata.UpgradeCodes = AppInstaller::JSON::GetRawStringSetFromJsonNode(item, fields.UpgradeCodes);

                HistoricalMetadataList.emplace_back(std::move(historicalMetadata));
            }
        }
    }

    web::json::value ProductMetadata::ToJson_1_N()
    {
        AICLI_LOG(Repo, Info, << "Creating metadata JSON " << SchemaVersion.ToString() << " fields");

        ProductMetadataFields_1_N fields{ SchemaVersion };

        web::json::value result;

        result[fields.Version] = web::json::value::string(fields.SchemaVersion);
        result[fields.ProductVersionMin] = AppInstaller::JSON::GetStringValue(ProductVersionMin.ToString());
        result[fields.ProductVersionMax] = AppInstaller::JSON::GetStringValue(ProductVersionMax.ToString());

        web::json::value metadataArray = web::json::value::array();
        size_t metadataItemIndex = 0;
        for (const auto& item : InstallerMetadataMap)
        {
            web::json::value itemValue;

            itemValue[fields.InstallerHash] = AppInstaller::JSON::GetStringValue(item.first);
            itemValue[fields.SubmissionIdentifier] = AppInstaller::JSON::GetStringValue(item.second.SubmissionIdentifier);
            SetStringFromFutureSchema(itemValue, fields.Scope, item.second.Scope);
            if (!fields.InstalledFiles.empty() && item.second.InstalledFiles.has_value())
            {
                web::json::value installationMetadata;

                installationMetadata[fields.DefaultInstallLocation] = AppInstaller::JSON::GetStringValue(item.second.InstalledFiles->DefaultInstallLocation);

                web::json::value installedFilesArray = web::json::value::array();
                size_t installedFileIndex = 0;
                for (const auto& entry : item.second.InstalledFiles->Files)
                {
                    web::json::value entryValue;
                    AddFieldIfNotEmpty(entryValue, fields.InstalledFileRelativeFilePath, entry.RelativeFilePath);
                    AddFieldIfNotEmpty(entryValue, fields.InstalledFileInvocationParameter, entry.InvocationParameter);
                    AddFieldIfNotEmpty(entryValue, fields.InstalledFileDisplayName, entry.DisplayName);
                    entryValue[fields.InstalledFileType] = AppInstaller::JSON::GetStringValue(Manifest::InstalledFileTypeToString(entry.FileType));
                    if (!entry.FileSha256.empty())
                    {
                        entryValue[fields.InstalledFileSha256] = AppInstaller::JSON::GetStringValue(SHA256::ConvertToString(entry.FileSha256));
                    }
                    installedFilesArray[installedFileIndex++] = std::move(entryValue);
                }
                installationMetadata[fields.InstallationMetadataFiles] = std::move(installedFilesArray);

                itemValue[fields.InstalledFiles] = std::move(installationMetadata);
            }

            if (!fields.InstalledStartupLinks.empty() && item.second.StartupLinkFiles.has_value())
            {
                web::json::value startupLinkFilesArray = web::json::value::array();
                size_t startupLinkFileIndex = 0;
                for (const auto& entry : item.second.StartupLinkFiles.value())
                {
                    web::json::value entryValue;
                    entryValue[fields.InstalledStartupLinkPath] = AppInstaller::JSON::GetStringValue(entry.RelativeFilePath);
                    entryValue[fields.InstalledStartupLinkType] = AppInstaller::JSON::GetStringValue(Manifest::InstalledFileTypeToString(entry.FileType));

                    startupLinkFilesArray[startupLinkFileIndex++] = std::move(entryValue);
                }

                itemValue[fields.InstalledStartupLinks] = std::move(startupLinkFilesArray);
            }

            if (!fields.Icons.empty() && !item.second.Icons.empty())
            {
                web::json::value iconsArray = web::json::value::array();
                size_t iconIndex = 0;
                for (const auto& entry : item.second.Icons)
                {
                    web::json::value entryValue;
                    entryValue[fields.IconContent] = AppInstaller::JSON::GetStringValue(AppInstaller::JSON::Base64Encode(entry.IconContent));
                    if (!entry.IconSha256.empty())
                    {
                        entryValue[fields.IconSha256] = AppInstaller::JSON::GetStringValue(SHA256::ConvertToString(entry.IconSha256));
                    }
                    entryValue[fields.IconFileType] = AppInstaller::JSON::GetStringValue(Manifest::IconFileTypeToString(entry.IconFileType));
                    entryValue[fields.IconTheme] = AppInstaller::JSON::GetStringValue(Manifest::IconThemeToString(entry.IconTheme));
                    entryValue[fields.IconResolution] = AppInstaller::JSON::GetStringValue(Manifest::IconResolutionToString(entry.IconResolution));

                    iconsArray[iconIndex++] = std::move(entryValue);
                }

                itemValue[fields.Icons] = std::move(iconsArray);
            }

            web::json::value appsAndFeaturesArray = web::json::value::array();
            size_t appsAndFeaturesEntryIndex = 0;
            for (const auto& entry : item.second.AppsAndFeaturesEntries)
            {
                web::json::value entryValue;

                AddFieldIfNotEmpty(entryValue, fields.DisplayName, entry.DisplayName);
                AddFieldIfNotEmpty(entryValue, fields.Publisher, entry.Publisher);
                AddFieldIfNotEmpty(entryValue, fields.DisplayVersion, entry.DisplayVersion);
                AddFieldIfNotEmpty(entryValue, fields.ProductCode, entry.ProductCode);
                AddFieldIfNotEmpty(entryValue, fields.UpgradeCode, entry.UpgradeCode);
                if (entry.InstallerType != Manifest::InstallerTypeEnum::Unknown)
                {
                    entryValue[fields.InstallerType] = AppInstaller::JSON::GetStringValue(Manifest::InstallerTypeToString(entry.InstallerType));
                }

                appsAndFeaturesArray[appsAndFeaturesEntryIndex++] = std::move(entryValue);
            }

            itemValue[fields.AppsAndFeaturesEntries] = std::move(appsAndFeaturesArray);

            metadataArray[metadataItemIndex++] = std::move(itemValue);
        }

        result[fields.Metadata] = std::move(metadataArray);

        web::json::value historicalArray = web::json::value::array();
        size_t historicalItemIndex = 0;
        for (const auto& item : HistoricalMetadataList)
        {
            web::json::value itemValue;

            itemValue[fields.VersionMin] = AppInstaller::JSON::GetStringValue(item.ProductVersionMin.ToString());
            itemValue[fields.VersionMax] = AppInstaller::JSON::GetStringValue(item.ProductVersionMax.ToString());
            itemValue[fields.Names] = CreateStringArray(item.Names);
            itemValue[fields.Publishers] = CreateStringArray(item.Publishers);
            itemValue[fields.ProductCodes] = CreateStringArray(item.ProductCodes);
            itemValue[fields.UpgradeCodes] = CreateStringArray(item.UpgradeCodes);

            historicalArray[historicalItemIndex++] = std::move(itemValue);
        }

        result[fields.Historical] = std::move(historicalArray);

        return result;
    }

    bool ProductMetadata::DropOldestHistoricalData()
    {
        if (HistoricalMetadataList.empty())
        {
            return false;
        }

        HistoricalMetadataList.pop_back();
        return true;
    }

    InstallerMetadataCollectionContext::InstallerMetadataCollectionContext() :
        m_correlationData(std::make_unique<Correlation::ARPCorrelationData>()),
        m_installedFilesCorrelation(std::make_unique<Correlation::InstalledFilesCorrelation>())
    {}

    InstallerMetadataCollectionContext::InstallerMetadataCollectionContext(
        std::unique_ptr<Correlation::ARPCorrelationData> correlationData,
        std::unique_ptr<Correlation::InstalledFilesCorrelation> installedFilesCorrelation,
        const std::wstring& json) :
        m_correlationData(std::move(correlationData)), m_installedFilesCorrelation(std::move(installedFilesCorrelation))
    {
        auto threadGlobalsLifetime = InitializeLogging({});
        InitializePreinstallState(json);
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromFile(const std::filesystem::path& file, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, file.empty());
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !std::filesystem::exists(file));

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);

        AICLI_LOG(Repo, Info, << "Opening InstallerMetadataCollectionContext input file: " << file);
        std::ifstream fileStream{ file };

        auto content = ReadEntireStream(fileStream);
        // CppRestSdk's implementation of json parsing does not work with '\0', so trimming them here
        content.erase(std::find(content.begin(), content.end(), '\0'), content.end());

        result->InitializePreinstallState(ConvertToUTF16(content));

        return result;
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromURI(std::wstring_view uri, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, uri.empty());

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);

        std::string utf8Uri = ConvertToUTF8(uri);
        THROW_HR_IF(E_INVALIDARG, !IsUrlRemote(utf8Uri));

        AICLI_LOG(Repo, Info, << "Downloading InstallerMetadataCollectionContext input file: " << utf8Uri);

        std::ostringstream jsonStream;
        ProgressCallback emptyCallback;

        const int MaxRetryCount = 2;
        for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
        {
            try
            {
                auto downloadHash = DownloadToStream(utf8Uri, jsonStream, DownloadType::InstallerMetadataCollectionInput, emptyCallback);
                break;
            }
            catch (...)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    AICLI_LOG(Repo, Info, << "  Downloading InstallerMetadataCollectionContext input failed, waiting a bit and retrying...");
                    Sleep(500);
                }
                else
                {
                    throw;
                }
            }
        }

        result->InitializePreinstallState(ConvertToUTF16(jsonStream.str()));

        return result;
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromJSON(const std::wstring& json, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, json.empty());

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);
        result->InitializePreinstallState(json);

        return result;
    }

    void InstallerMetadataCollectionContext::Complete(const std::filesystem::path& output)
    {
        auto threadGlobalsLifetime = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_INVALIDARG, !output.has_filename());

        if (output.has_parent_path())
        {
            std::filesystem::create_directories(output.parent_path());
        }

        std::ofstream outputStream{ output };
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_OPEN_FAILED), !outputStream);

        CompleteWithThreadGlobalsSet(outputStream);
    }

    void InstallerMetadataCollectionContext::Complete(std::ostream& output)
    {
        auto threadGlobalsLifetime = m_threadGlobals.SetForCurrentThread();
        CompleteWithThreadGlobalsSet(output);
    }

    std::wstring InstallerMetadataCollectionContext::Merge(const std::wstring& json, size_t maximumSizeInBytes, const std::filesystem::path& logFile)
    {
        ThreadLocalStorage::WingetThreadGlobals threadGlobals;
        auto globalsLifetime = InitializeLogging(threadGlobals, logFile);

        AICLI_LOG(Repo, Info, << "Parsing input JSON:\n" << ConvertToUTF8(json));

        // Parse and validate JSON
        try
        {
            utility::string_t versionFieldName = L"version";

            web::json::value inputValue = web::json::value::parse(json);

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, inputValue.is_null());

            Version inputVersion = Version{ GetRequiredString(inputValue, versionFieldName) };
            AICLI_LOG(Repo, Info, << "Parsing input JSON version " << inputVersion.ToString());

            web::json::value mergedResult;

            if (inputVersion.PartAt(0).Integer == 1)
            {
                mergedResult = Merge_1_0(inputValue, maximumSizeInBytes);
            }
            else
            {
                AICLI_LOG(Repo, Error, << "Don't know how to handle version " << inputVersion.ToString());
                THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
            }

            std::wostringstream outputStream;
            mergedResult.serialize(outputStream);

            return std::move(outputStream).str();
        }
        catch (const web::json::json_exception& exc)
        {
            AICLI_LOG(Repo, Error, << "Exception parsing input JSON: " << exc.what());
        }

        // We will return within the try or throw a non-json exception, so if we get here it was a json exception.
        THROW_HR(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
    }

    void InstallerMetadataCollectionContext::CompleteWithThreadGlobalsSet(std::ostream& output)
    {
        web::json::value outputJSON;

        if (!ContainsError())
        {
            try
            {
                // Collect post-install system state
                m_correlationData->CapturePostInstallSnapshot();
                m_installedFilesCorrelation->StopFileWatcher();

                ComputeOutputData();

                // Construct output JSON
                AICLI_LOG(Repo, Info, << "Creating output JSON version for input version " << m_inputVersion.ToString());

                if (m_inputVersion.PartAt(0).Integer == 1)
                {
                    // We only have one version currently, so use that as long as the major version is 1
                    outputJSON = CreateOutputJson_1_0();
                }
                else
                {
                    AICLI_LOG(Repo, Error, << "Don't know how to output for version " << m_inputVersion.ToString());
                    THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
                }
            }
            catch (...)
            {
                CollectErrorDataFromException(std::current_exception());
            }
        }

        if (ContainsError())
        {
            // We only have one version currently
            outputJSON = CreateErrorJson_1_0();
        }

        // Write output
        outputJSON.serialize(output);
    }

    std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InstallerMetadataCollectionContext::InitializeLogging(ThreadLocalStorage::WingetThreadGlobals& threadGlobals, const std::filesystem::path& logFile)
    {
        auto threadGlobalsLifetime = threadGlobals.SetForCurrentThread();

        Logging::Log().SetLevel(Logging::Level::Info);
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::EnableWilFailureTelemetry();
        Logging::TraceLogger::Add();

        if (!logFile.empty())
        {
            Logging::FileLogger::Add(logFile);
        }

        Logging::Telemetry().SetCaller("installer-metadata-collection");
        Logging::Telemetry().LogStartup();

        return threadGlobalsLifetime;
    }

    std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InstallerMetadataCollectionContext::InitializeLogging(const std::filesystem::path& logFile)
    {
        return InitializeLogging(m_threadGlobals, logFile);
    }

    void InstallerMetadataCollectionContext::InitializePreinstallState(const std::wstring& json)
    {
        try
        {
            AICLI_LOG(Repo, Info, << "Parsing input JSON:\n" << ConvertToUTF8(json));

            // Parse and validate JSON
            try
            {
                utility::string_t versionFieldName = L"version";

                web::json::value inputValue = web::json::value::parse(json);

                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, inputValue.is_null());

                m_inputVersion = Version{ GetRequiredString(inputValue, versionFieldName) };
                AICLI_LOG(Repo, Info, << "Parsing input JSON version " << m_inputVersion.ToString());

                if (m_inputVersion.PartAt(0).Integer == 1)
                {
                    // We only have one version currently, so use that as long as the major version is 1
                    ParseInputJson_1_0(inputValue);
                }
                else
                {
                    AICLI_LOG(Repo, Error, << "Don't know how to handle version " << m_inputVersion.ToString());
                    THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
                }
            }
            catch (const web::json::json_exception& exc)
            {
                AICLI_LOG(Repo, Error, << "Exception parsing input JSON: " << exc.what());
                throw;
            }

            // Collect pre-install system state
            m_correlationData->CapturePreInstallSnapshot();
            m_installedFilesCorrelation->StartFileWatcher();
        }
        catch (...)
        {
            CollectErrorDataFromException(std::current_exception());
        }
    }

    void InstallerMetadataCollectionContext::ComputeOutputData()
    {
        // Copy the metadata from the current; this function takes care of moving data to historical if the submission is new.
        m_outputMetadata.CopyFrom(m_currentMetadata, m_submissionIdentifier);

        Correlation::ARPCorrelationSettings settings;
        std::string arpInstallLocation;
        // As this code is typically run in a controlled environment, we can assume that a single value change is very likely the correct value.
        settings.AllowSingleChange = true;

        // ARP entry correlation
        Correlation::ARPCorrelationResult correlationResult = m_correlationData->CorrelateForNewlyInstalled(m_incomingManifest, settings);

        if (correlationResult.Package)
        {
            auto& package = correlationResult.Package;

            // Update min and max versions based on the version of the correlated package
            Version packageVersion{ package->GetProperty(PackageVersionProperty::Version) };

            if (m_outputMetadata.ProductVersionMin.IsEmpty() || packageVersion < m_outputMetadata.ProductVersionMin)
            {
                m_outputMetadata.ProductVersionMin = packageVersion;
            }

            if (m_outputMetadata.ProductVersionMax.IsEmpty() || m_outputMetadata.ProductVersionMax < packageVersion)
            {
                m_outputMetadata.ProductVersionMax = packageVersion;
            }

            // Create the AppsAndFeaturesEntry that we need to add
            Manifest::AppsAndFeaturesEntry newEntry;
            auto packageMetadata = package->GetMetadata();

            // Arp installed location will be used in later installed files correlation.
            arpInstallLocation = packageMetadata[PackageVersionMetadata::InstalledLocation];

            // TODO: Use some amount of normalization here to prevent things like versions being in the name from bloating the data
            newEntry.DisplayName = package->GetProperty(PackageVersionProperty::Name).get();
            newEntry.DisplayVersion = packageVersion.ToString();
            if (packageMetadata.count(PackageVersionMetadata::InstalledType))
            {
                newEntry.InstallerType = Manifest::ConvertToInstallerTypeEnum(packageMetadata[PackageVersionMetadata::InstalledType]);
            }
            auto productCodes = package->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            if (!productCodes.empty())
            {
                newEntry.ProductCode = std::move(productCodes[0]).get();
            }
            newEntry.Publisher = package->GetProperty(PackageVersionProperty::Publisher).get();
            // TODO: Support upgrade code throughout the code base...

            Manifest::ScopeEnum scope = Manifest::ConvertToScopeEnum(packageMetadata[PackageVersionMetadata::InstalledScope]);

            // ARP entry icon extraction upon ARP correlation success
            auto icons = ExtractIconFromArpEntry(newEntry.ProductCode, scope);

            // Add or update the metadata for the installer hash
            auto itr = m_outputMetadata.InstallerMetadataMap.find(m_installerHash);

            if (itr == m_outputMetadata.InstallerMetadataMap.end())
            {
                // New entry needed
                ProductMetadata::InstallerMetadata newMetadata;

                newMetadata.SubmissionIdentifier = m_submissionIdentifier;
                newMetadata.AppsAndFeaturesEntries.emplace_back(std::move(newEntry));

                if (scope != Manifest::ScopeEnum::Unknown)
                {
                    newMetadata.Scope = Manifest::ScopeToString(scope);
                }

                if (!icons.empty())
                {
                    newMetadata.Icons = std::move(icons);
                }

                m_outputMetadata.InstallerMetadataMap[m_installerHash] = std::move(newMetadata);
            }
            else
            {
                if (itr->second.Scope.empty())
                {
                    itr->second.Scope = Manifest::ScopeToString(scope);
                }
                // If there is a conflicting scope already present, force it to Unknown
                else if (scope != Manifest::ScopeEnum::Unknown && Manifest::ConvertToScopeEnum(itr->second.Scope) != scope)
                {
                    itr->second.Scope = Manifest::ScopeToString(Manifest::ScopeEnum::Unknown);
                }

                // We will always use the latest extracted icons upon confliction.
                if (!icons.empty())
                {
                    itr->second.Icons = std::move(icons);
                }

                // Existing entry for installer hash, add/update the entry
                FilterAndAddToEntries(std::move(newEntry), itr->second.AppsAndFeaturesEntries);
            }
        }

        // Installation files correlation
        auto installationMetadata = m_installedFilesCorrelation->CorrelateForNewlyInstalled(m_incomingManifest, arpInstallLocation);

        if (installationMetadata.InstalledFiles.HasData() || !installationMetadata.StartupLinkFiles.empty())
        {
            // Add or update the metadata for the installer hash
            auto itr = m_outputMetadata.InstallerMetadataMap.find(m_installerHash);

            if (itr == m_outputMetadata.InstallerMetadataMap.end())
            {
                // New entry needed
                ProductMetadata::InstallerMetadata newMetadata;

                newMetadata.SubmissionIdentifier = m_submissionIdentifier;

                if (installationMetadata.InstalledFiles.HasData())
                {
                    newMetadata.InstalledFiles = std::move(installationMetadata.InstalledFiles);
                }
                if (!installationMetadata.StartupLinkFiles.empty())
                {
                    newMetadata.StartupLinkFiles = std::move(installationMetadata.StartupLinkFiles);
                }

                m_outputMetadata.InstallerMetadataMap[m_installerHash] = std::move(newMetadata);
            }
            else
            {
                // Add new or merge with existing entry
                if (installationMetadata.InstalledFiles.HasData())
                {
                    if (!itr->second.InstalledFiles.has_value())
                    {
                        itr->second.InstalledFiles = std::move(installationMetadata.InstalledFiles);
                    }
                    else
                    {
                        MergeInstalledFilesMetadata(*(itr->second.InstalledFiles), installationMetadata.InstalledFiles);
                    }
                }

                if (!installationMetadata.StartupLinkFiles.empty())
                {
                    if (!itr->second.StartupLinkFiles.has_value())
                    {
                        itr->second.StartupLinkFiles = std::move(installationMetadata.StartupLinkFiles);
                    }
                    else
                    {
                        MergeStartupLinkFilesMetadata(*(itr->second.StartupLinkFiles), installationMetadata.StartupLinkFiles);
                    }
                }
            }
        }

        if (correlationResult.Package)
        {
            m_outputStatus = OutputStatus::Success;
        }
        else
        {
            m_outputStatus = OutputStatus::LowConfidence;
        }

        // Create the diagnostics data, based on the other values from the correlation result.
        DiagnosticFields fields;

        m_outputDiagnostics[fields.Reason] = AppInstaller::JSON::GetStringValue(correlationResult.Reason);
        m_outputDiagnostics[fields.ChangedEntryCount] = web::json::value::number(static_cast<int64_t>(correlationResult.ChangesToARP));
        m_outputDiagnostics[fields.MatchedEntryCount] = web::json::value::number(static_cast<int64_t>(correlationResult.MatchesInARP));
        m_outputDiagnostics[fields.IntersectionCount] = web::json::value::number(static_cast<int64_t>(correlationResult.CountOfIntersectionOfChangesAndMatches));

        constexpr size_t MaximumDiagnosticMeasures = 10;
        web::json::value measuresArray = web::json::value::array();
        for (size_t i = 0; i < correlationResult.Measures.size() && i < MaximumDiagnosticMeasures; ++i)
        {
            web::json::value measureValue;
            const auto& measure = correlationResult.Measures[i];

            measureValue[fields.Value] = web::json::value::number(measure.Measure);
            measureValue[fields.Name] = AppInstaller::JSON::GetStringValue(measure.Package->GetProperty(PackageVersionProperty::Name));
            measureValue[fields.Publisher] = AppInstaller::JSON::GetStringValue(measure.Package->GetProperty(PackageVersionProperty::Publisher));

            measuresArray[i] = std::move(measureValue);
        }

        m_outputDiagnostics[fields.CorrelationMeasures] = std::move(measuresArray);
    }

    void InstallerMetadataCollectionContext::ParseInputJson_1_0(web::json::value& input)
    {
        AICLI_LOG(Repo, Info, << "Parsing input JSON 1.0 fields");

        // Field names
        utility::string_t metadataVersionFieldName = L"supportedMetadataVersion";
        utility::string_t metadataFieldName = L"currentMetadata";
        utility::string_t submissionDataFieldName = L"submissionData";
        utility::string_t submissionIdentifierFieldName = L"submissionIdentifier";
        utility::string_t packageDataFieldName = L"packageData";
        utility::string_t installerHashFieldName = L"installerHash";
        utility::string_t defaultLocaleFieldName = L"DefaultLocale";
        utility::string_t localesFieldName = L"Locales";

        // root fields
        m_supportedMetadataVersion = Version{ GetRequiredString(input, metadataVersionFieldName) };

        auto currentMetadataValue = AppInstaller::JSON::GetJsonValueFromNode(input, metadataFieldName);
        if (currentMetadataValue)
        {
            m_currentMetadata.FromJson(currentMetadataValue.value());
        }

        // submissionData fields
        auto submissionDataValue = AppInstaller::JSON::GetJsonValueFromNode(input, submissionDataFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !submissionDataValue);
        m_submissionData = submissionDataValue.value();

        m_submissionIdentifier = GetRequiredString(m_submissionData, submissionIdentifierFieldName);

        // packageData fields
        auto packageDataValue = AppInstaller::JSON::GetJsonValueFromNode(input, packageDataFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !packageDataValue);

        m_installerHash = GetRequiredString(packageDataValue.value(), installerHashFieldName);

        // The 1.0 version of input uses the 1.5 version of REST
        JSON::ManifestJSONParser parser{ Version{ "1.5" }};

        {
            auto defaultLocaleValue = AppInstaller::JSON::GetJsonValueFromNode(packageDataValue.value(), defaultLocaleFieldName);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !defaultLocaleValue);

            auto defaultLocale = parser.DeserializeLocale(defaultLocaleValue.value());
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE,
                !defaultLocale ||
                !defaultLocale->Contains(Manifest::Localization::PackageName) ||
                !defaultLocale->Contains(Manifest::Localization::Publisher));

            m_incomingManifest.DefaultLocalization = std::move(defaultLocale).value();

            auto localesArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(packageDataValue.value(), localesFieldName);
            if (localesArray)
            {
                for (const auto& locale : localesArray->get())
                {
                    auto localization = parser.DeserializeLocale(locale);
                    if (localization)
                    {
                        m_incomingManifest.Localizations.emplace_back(std::move(localization).value());
                    }
                }
            }
        }
    }

    web::json::value InstallerMetadataCollectionContext::CreateOutputJson_1_0()
    {
        AICLI_LOG(Repo, Info, << "Setting output JSON 1.0 fields");

        OutputFields_1_0 fields;

        web::json::value result;

        result[fields.Version] = web::json::value::string(L"1.0");
        result[fields.SubmissionData] = m_submissionData;
        result[fields.InstallerHash] = AppInstaller::JSON::GetStringValue(m_installerHash);

        // Limit output status to 1.0 known values
        OutputStatus statusToUse = OutputStatus::Unknown;
        if (m_outputStatus == OutputStatus::Success || m_outputStatus == OutputStatus::Error || m_outputStatus == OutputStatus::LowConfidence)
        {
            statusToUse = m_outputStatus;
        }
        result[fields.Status] = web::json::value::string(ToString(statusToUse));

        if (m_outputStatus == OutputStatus::Success)
        {
            result[fields.Metadata] = m_outputMetadata.ToJson(m_supportedMetadataVersion, 0);
        }

        result[fields.Diagnostics] = m_outputDiagnostics;

        return result;
    }

    utility::string_t InstallerMetadataCollectionContext::ToString(OutputStatus status)
    {
        switch (status)
        {
        case OutputStatus::Success: return L"Success";
        case OutputStatus::Error: return L"Error";
        case OutputStatus::LowConfidence: return L"LowConfidence";
        }

        // For both the status value of Unknown and anything else
        return L"Unknown";
    }

    bool InstallerMetadataCollectionContext::ContainsError() const
    {
        return m_outputStatus == OutputStatus::Error;
    }

    void InstallerMetadataCollectionContext::CollectErrorDataFromException(std::exception_ptr exception)
    {
        m_outputStatus = OutputStatus::Error;

        try
        {
            std::rethrow_exception(exception);
        }
        catch (const wil::ResultException& re)
        {
            m_errorHR = re.GetErrorCode();
            m_errorText = GetUserPresentableMessage(re);
        }
        catch (const winrt::hresult_error& hre)
        {
            m_errorHR = hre.code();
            m_errorText = GetUserPresentableMessage(hre);
        }
        catch (const std::exception& e)
        {
            m_errorHR = E_FAIL;
            m_errorText = GetUserPresentableMessage(e);
        }
        catch (...)
        {
            m_errorHR = E_UNEXPECTED;
            m_errorText = "An unexpected exception type was thrown.";
        }
    }

    web::json::value InstallerMetadataCollectionContext::CreateErrorJson_1_0()
    {
        AICLI_LOG(Repo, Info, << "Setting error JSON 1.0 fields");

        OutputFields_1_0 fields;
        DiagnosticFields diagnosticFields;

        web::json::value result;

        result[fields.Version] = web::json::value::string(L"1.0");
        result[fields.SubmissionData] = m_submissionData;
        result[fields.InstallerHash] = AppInstaller::JSON::GetStringValue(m_installerHash);
        result[fields.Status] = web::json::value::string(ToString(OutputStatus::Error));
        result[fields.Metadata] = web::json::value::null();

        web::json::value error;

        error[diagnosticFields.ErrorHR] = web::json::value::number(static_cast<int64_t>(m_errorHR));
        error[diagnosticFields.ErrorText] = AppInstaller::JSON::GetStringValue(m_errorText);

        result[fields.Diagnostics] = std::move(error);

        return result;
    }

    web::json::value InstallerMetadataCollectionContext::Merge_1_0(web::json::value& input, size_t maximumSizeInBytes)
    {
        AICLI_LOG(Repo, Info, << "Merging 1.0 input metadatas");

        utility::string_t metadatasFieldName = L"metadatas";

        auto metadatasValue = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(input, metadatasFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !metadatasValue);

        std::vector<ProductMetadata> metadatas;
        for (const auto& value : metadatasValue->get())
        {
            ProductMetadata current;
            current.FromJson(value);
            metadatas.emplace_back(std::move(current));
        }

        THROW_HR_IF(E_NOT_SET, metadatas.empty());

        // Require that all merging values use the same submission
        for (const ProductMetadata& metadata : metadatas)
        {
            const std::string& firstSubmission = metadatas[0].InstallerMetadataMap.begin()->second.SubmissionIdentifier;
            const std::string& metadataSubmission = metadata.InstallerMetadataMap.begin()->second.SubmissionIdentifier;
            if (firstSubmission != metadataSubmission)
            {
                AICLI_LOG(Repo, Info, << "Found submission identifier mismatch: " << firstSubmission << " != " << metadataSubmission);
                THROW_HR(E_NOT_VALID_STATE);
            }
        }

        // Do the actual merging
        ProductMetadata resultMetadata;

        // The historical data should be the same across the board, so we can just copy the first one.
        resultMetadata.HistoricalMetadataList = metadatas[0].HistoricalMetadataList;

        for (const ProductMetadata& metadata : metadatas)
        {
            // Get the minimum and maximum versions from the individual values
            if (resultMetadata.ProductVersionMin.IsEmpty() || metadata.ProductVersionMin < resultMetadata.ProductVersionMin)
            {
                resultMetadata.ProductVersionMin = metadata.ProductVersionMin;
            }

            if (resultMetadata.ProductVersionMax < metadata.ProductVersionMax)
            {
                resultMetadata.ProductVersionMax = metadata.ProductVersionMax;
            }

            if (resultMetadata.SchemaVersion < metadata.SchemaVersion)
            {
                resultMetadata.SchemaVersion = metadata.SchemaVersion;
            }

            for (const auto& installerMetadata : metadata.InstallerMetadataMap)
            {
                auto itr = resultMetadata.InstallerMetadataMap.find(installerMetadata.first);
                if (itr == resultMetadata.InstallerMetadataMap.end())
                {
                    // Installer hash not in the result, so just copy it
                    resultMetadata.InstallerMetadataMap.emplace(installerMetadata);
                }
                else
                {
                    if (itr->second.Scope.empty())
                    {
                        itr->second.Scope = installerMetadata.second.Scope;
                    }
                    else if (!installerMetadata.second.Scope.empty())
                    {
                        // If there is a conflicting scope already present, force it to Unknown
                        if (Manifest::ConvertToScopeEnum(itr->second.Scope) != Manifest::ConvertToScopeEnum(installerMetadata.second.Scope))
                        {
                            itr->second.Scope = Manifest::ScopeToString(Manifest::ScopeEnum::Unknown);
                        }
                    }

                    // We will always use the latest extracted icons upon confliction.
                    if (!installerMetadata.second.Icons.empty())
                    {
                        itr->second.Icons = installerMetadata.second.Icons;
                    }

                    if (!itr->second.InstalledFiles.has_value())
                    {
                        itr->second.InstalledFiles = installerMetadata.second.InstalledFiles;
                    }
                    else if (installerMetadata.second.InstalledFiles.has_value())
                    {
                        MergeInstalledFilesMetadata(*(itr->second.InstalledFiles), *(installerMetadata.second.InstalledFiles));
                    }

                    if (!itr->second.StartupLinkFiles.has_value())
                    {
                        itr->second.StartupLinkFiles = installerMetadata.second.StartupLinkFiles;
                    }
                    else if (installerMetadata.second.StartupLinkFiles.has_value())
                    {
                        MergeStartupLinkFilesMetadata(*(itr->second.StartupLinkFiles), *(installerMetadata.second.StartupLinkFiles));
                    }

                    // Merge into existing installer data
                    for (const auto& targetEntry : installerMetadata.second.AppsAndFeaturesEntries)
                    {
                        FilterAndAddToEntries(Manifest::AppsAndFeaturesEntry{ targetEntry }, itr->second.AppsAndFeaturesEntries);
                    }
                }
            }
        }

        // Convert to JSON
        return resultMetadata.ToJson(resultMetadata.SchemaVersion, maximumSizeInBytes);
    }
}

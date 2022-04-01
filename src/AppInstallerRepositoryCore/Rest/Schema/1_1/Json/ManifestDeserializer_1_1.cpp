// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/JsonHelper.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    namespace
    {
        // Installer
        constexpr std::string_view MSStoreProductIdentifier = "MSStoreProductIdentifier"sv;
        constexpr std::string_view ReleaseDate = "ReleaseDate"sv;
        constexpr std::string_view InstallerAbortsTerminal = "InstallerAbortsTerminal"sv;
        constexpr std::string_view InstallLocationRequired = "InstallLocationRequired"sv;
        constexpr std::string_view RequireExplicitUpgrade = "RequireExplicitUpgrade"sv;
        constexpr std::string_view UnsupportedOSArchitectures = "UnsupportedOSArchitectures"sv;
        constexpr std::string_view AppsAndFeaturesEntries = "AppsAndFeaturesEntries"sv;
        constexpr std::string_view DisplayName = "DisplayName"sv;
        constexpr std::string_view Publisher = "Publisher"sv;
        constexpr std::string_view DisplayVersion = "DisplayVersion"sv;
        constexpr std::string_view ProductCode = "ProductCode"sv;
        constexpr std::string_view UpgradeCode = "UpgradeCode"sv;
        constexpr std::string_view InstallerType = "InstallerType"sv;
        constexpr std::string_view Markets = "Markets"sv;
        constexpr std::string_view AllowedMarkets = "AllowedMarkets"sv;
        constexpr std::string_view ExcludedMarkets = "ExcludedMarkets"sv;
        constexpr std::string_view ElevationRequirement = "ElevationRequirement"sv;
        constexpr std::string_view ExpectedReturnCodes = "ExpectedReturnCodes"sv;
        constexpr std::string_view InstallerReturnCode = "InstallerReturnCode"sv;
        constexpr std::string_view ReturnResponse = "ReturnResponse"sv;

        // Locale
        constexpr std::string_view ReleaseNotes = "ReleaseNotes"sv;
        constexpr std::string_view ReleaseNotesUrl = "ReleaseNotesUrl"sv;
        constexpr std::string_view Agreements = "Agreements"sv;
        constexpr std::string_view AgreementLabel = "AgreementLabel"sv;
        constexpr std::string_view Agreement = "Agreement"sv;
        constexpr std::string_view AgreementUrl = "AgreementUrl"sv;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "msstore")
        {
            return InstallerTypeEnum::MSStore;
        }

        return V1_0::Json::ManifestDeserializer::ConvertToInstallerType(in);
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_0::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.ProductId = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(MSStoreProductIdentifier)).value_or("");
            installer.ReleaseDate = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(ReleaseDate)).value_or("");
            installer.InstallerAbortsTerminal = JsonHelper::GetRawBoolValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerAbortsTerminal)).value_or(false);
            installer.InstallLocationRequired = JsonHelper::GetRawBoolValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallLocationRequired)).value_or(false);
            installer.RequireExplicitUpgrade = JsonHelper::GetRawBoolValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(RequireExplicitUpgrade)).value_or(false);
            installer.ElevationRequirement = Manifest::ConvertToElevationRequirementEnum(
                JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(ElevationRequirement)).value_or(""));

            // list of unsupported OS architectures
            std::optional<std::reference_wrapper<const web::json::array>> unsupportedOSArchitectures = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(UnsupportedOSArchitectures));
            if (unsupportedOSArchitectures)
            {
                for (auto& archValue : unsupportedOSArchitectures.value().get())
                {
                    std::optional<std::string> arch = JsonHelper::GetRawStringValueFromJsonValue(archValue);
                    if (JsonHelper::IsValidNonEmptyStringValue(arch))
                    {
                        auto archEnum = Utility::ConvertToArchitectureEnum(arch.value());

                        if (archEnum == Utility::Architecture::Neutral)
                        {
                            AICLI_LOG(Repo, Error, << "Unsupported OS architectures cannot contain neutral value.");
                            return {};
                        }

                        if (archEnum != Utility::Architecture::Unknown)
                        {
                            installer.UnsupportedOSArchitectures.emplace_back(archEnum);
                        }
                    }
                }
            }

            // Apps and Features Entries
            std::optional<std::reference_wrapper<const web::json::array>> arpEntriesNode = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(AppsAndFeaturesEntries));
            if (arpEntriesNode)
            {
                for (auto& arpEntryNode : arpEntriesNode.value().get())
                {
                    AppsAndFeaturesEntry arpEntry;
                    arpEntry.DisplayName = JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(DisplayName)).value_or("");
                    arpEntry.Publisher = JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(Publisher)).value_or("");
                    arpEntry.DisplayVersion = JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(DisplayVersion)).value_or("");
                    arpEntry.ProductCode = JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(ProductCode)).value_or("");
                    arpEntry.UpgradeCode = JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(UpgradeCode)).value_or("");
                    arpEntry.InstallerType = Manifest::ConvertToInstallerTypeEnum(JsonHelper::GetRawStringValueFromJsonNode(arpEntryNode, JsonHelper::GetUtilityString(InstallerType)).value_or(""));

                    // Only add when at least one field is valid
                    if (!arpEntry.DisplayName.empty() || !arpEntry.Publisher.empty() || !arpEntry.DisplayVersion.empty() ||
                        !arpEntry.ProductCode.empty() || !arpEntry.UpgradeCode.empty() || arpEntry.InstallerType != InstallerTypeEnum::Unknown)
                    {
                        installer.AppsAndFeaturesEntries.emplace_back(std::move(arpEntry));
                    }
                }
            }

            // Markets
            std::optional<std::reference_wrapper<const web::json::value>> marketsNode = JsonHelper::GetJsonValueFromNode(installerJsonObject, JsonHelper::GetUtilityString(Markets));
            if (marketsNode && !marketsNode.value().get().is_null())
            {
                installer.Markets.ExcludedMarkets = V1_0::Json::ManifestDeserializer::ConvertToManifestStringArray(
                    JsonHelper::GetRawStringArrayFromJsonNode(marketsNode.value().get(), JsonHelper::GetUtilityString(ExcludedMarkets)));
                installer.Markets.AllowedMarkets = V1_0::Json::ManifestDeserializer::ConvertToManifestStringArray(
                    JsonHelper::GetRawStringArrayFromJsonNode(marketsNode.value().get(), JsonHelper::GetUtilityString(AllowedMarkets)));
            }

            // Expected return codes
            std::optional<std::reference_wrapper<const web::json::array>> expectedReturnCodesNode = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(ExpectedReturnCodes));
            if (expectedReturnCodesNode)
            {
                for (auto& returnCodeNode : expectedReturnCodesNode.value().get())
                {
                    ExpectedReturnCodeEnum returnResponse = Manifest::ConvertToExpectedReturnCodeEnum(JsonHelper::GetRawStringValueFromJsonNode(returnCodeNode, JsonHelper::GetUtilityString(ReturnResponse)).value_or(""));
                    DWORD installerReturnCode = static_cast<DWORD>(JsonHelper::GetRawIntValueFromJsonNode(returnCodeNode, JsonHelper::GetUtilityString(InstallerReturnCode)).value_or(0));

                    // Only add when it is valid
                    if (installerReturnCode != 0 && returnResponse != ExpectedReturnCodeEnum::Unknown)
                    {
                        if (!installer.ExpectedReturnCodes.insert({ installerReturnCode, { returnResponse, "" } }).second)
                        {
                            AICLI_LOG(Repo, Error, << "Expected return codes cannot have repeated value.");
                            return {};
                        }
                    }
                }
            }

            // Populate installer default return codes if not present in ExpectedReturnCodes and InstallerSuccessCodes
            auto defaultReturnCodes = GetDefaultKnownReturnCodes(installer.InstallerType);
            for (auto const& defaultReturnCode : defaultReturnCodes)
            {
                if (installer.ExpectedReturnCodes.find(defaultReturnCode.first) == installer.ExpectedReturnCodes.end() &&
                    std::find(installer.InstallerSuccessCodes.begin(), installer.InstallerSuccessCodes.end(), defaultReturnCode.first) == installer.InstallerSuccessCodes.end())
                {
                    installer.ExpectedReturnCodes[defaultReturnCode.first].ReturnResponseEnum = defaultReturnCode.second;
                }
            }
        }

        return result;
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        auto result = V1_0::Json::ManifestDeserializer::DeserializeLocale(localeJsonObject);

        if (result)
        {
            auto& locale = result.value();

            TryParseStringLocaleField<Manifest::Localization::ReleaseNotes>(locale, localeJsonObject, ReleaseNotes);
            TryParseStringLocaleField<Manifest::Localization::ReleaseNotesUrl>(locale, localeJsonObject, ReleaseNotesUrl);

            // Agreements
            auto agreementsNode = JsonHelper::GetRawJsonArrayFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Agreements));
            if (agreementsNode)
            {
                std::vector<Manifest::Agreement> agreements;
                for (auto const& agreementNode : agreementsNode.value().get())
                {
                    Manifest::Agreement agreementEntry;

                    agreementEntry.Label = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(AgreementLabel)).value_or("");
                    agreementEntry.AgreementText = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(Agreement)).value_or("");
                    agreementEntry.AgreementUrl = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(AgreementUrl)).value_or("");

                    if (!agreementEntry.Label.empty() || !agreementEntry.AgreementText.empty() || !agreementEntry.AgreementUrl.empty())
                    {
                        agreements.emplace_back(std::move(agreementEntry));
                    }
                }

                if (!agreements.empty())
                {
                    locale.Add<Manifest::Localization::Agreements>(std::move(agreements));
                }
            }
        }

        return result;
    }
}

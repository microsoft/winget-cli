// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

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

    std::vector<Manifest::AppsAndFeaturesEntry> ManifestDeserializer::DeserializeAppsAndFeaturesEntries(const web::json::array& entries) const
    {
        std::vector<Manifest::AppsAndFeaturesEntry> result;

        for (auto& arpEntryNode : entries)
        {
            AppsAndFeaturesEntry arpEntry;
            arpEntry.DisplayName = JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(DisplayName)).value_or("");
            arpEntry.Publisher = JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(Publisher)).value_or("");
            arpEntry.DisplayVersion = JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(DisplayVersion)).value_or("");
            arpEntry.ProductCode = JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(ProductCode)).value_or("");
            arpEntry.UpgradeCode = JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(UpgradeCode)).value_or("");
            arpEntry.InstallerType = ConvertToInstallerType(JSON::GetRawStringValueFromJsonNode(arpEntryNode, JSON::GetUtilityString(InstallerType)).value_or(""));

            // Only add when at least one field is valid
            if (!arpEntry.DisplayName.empty() || !arpEntry.Publisher.empty() || !arpEntry.DisplayVersion.empty() ||
                !arpEntry.ProductCode.empty() || !arpEntry.UpgradeCode.empty() || arpEntry.InstallerType != InstallerTypeEnum::Unknown)
            {
                result.emplace_back(std::move(arpEntry));
            }
        }

        return result;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "msstore")
        {
            return InstallerTypeEnum::MSStore;
        }

        return V1_0::Json::ManifestDeserializer::ConvertToInstallerType(inStrLower);
    }

    Manifest::ExpectedReturnCodeEnum ManifestDeserializer::ConvertToExpectedReturnCodeEnum(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);
        ExpectedReturnCodeEnum result = ExpectedReturnCodeEnum::Unknown;

        if (inStrLower == "packageinuse")
        {
            result = ExpectedReturnCodeEnum::PackageInUse;
        }
        else if (inStrLower == "installinprogress")
        {
            result = ExpectedReturnCodeEnum::InstallInProgress;
        }
        else if (inStrLower == "fileinuse")
        {
            result = ExpectedReturnCodeEnum::FileInUse;
        }
        else if (inStrLower == "missingdependency")
        {
            result = ExpectedReturnCodeEnum::MissingDependency;
        }
        else if (inStrLower == "diskfull")
        {
            result = ExpectedReturnCodeEnum::DiskFull;
        }
        else if (inStrLower == "insufficientmemory")
        {
            result = ExpectedReturnCodeEnum::InsufficientMemory;
        }
        else if (inStrLower == "nonetwork")
        {
            result = ExpectedReturnCodeEnum::NoNetwork;
        }
        else if (inStrLower == "contactsupport")
        {
            result = ExpectedReturnCodeEnum::ContactSupport;
        }
        else if (inStrLower == "rebootrequiredtofinish")
        {
            result = ExpectedReturnCodeEnum::RebootRequiredToFinish;
        }
        else if (inStrLower == "rebootrequiredforinstall")
        {
            result = ExpectedReturnCodeEnum::RebootRequiredForInstall;
        }
        else if (inStrLower == "rebootinitiated")
        {
            result = ExpectedReturnCodeEnum::RebootInitiated;
        }
        else if (inStrLower == "cancelledbyuser")
        {
            result = ExpectedReturnCodeEnum::CancelledByUser;
        }
        else if (inStrLower == "alreadyinstalled")
        {
            result = ExpectedReturnCodeEnum::AlreadyInstalled;
        }
        else if (inStrLower == "downgrade")
        {
            result = ExpectedReturnCodeEnum::Downgrade;
        }
        else if (inStrLower == "blockedbypolicy")
        {
            result = ExpectedReturnCodeEnum::BlockedByPolicy;
        }

        return result;
    }

    Manifest::ManifestInstaller::ExpectedReturnCodeInfo ManifestDeserializer::DeserializeExpectedReturnCodeInfo(const web::json::value& expectedReturnCodeJsonObject) const
    {
        ExpectedReturnCodeEnum returnResponse = ConvertToExpectedReturnCodeEnum(JSON::GetRawStringValueFromJsonNode(expectedReturnCodeJsonObject, JSON::GetUtilityString(ReturnResponse)).value_or(""));
        
        return { returnResponse, "" };
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_0::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.ProductId = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(MSStoreProductIdentifier)).value_or("");
            installer.ReleaseDate = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(ReleaseDate)).value_or("");
            installer.InstallerAbortsTerminal = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerAbortsTerminal)).value_or(false);
            installer.InstallLocationRequired = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallLocationRequired)).value_or(false);
            installer.RequireExplicitUpgrade = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(RequireExplicitUpgrade)).value_or(false);
            installer.ElevationRequirement = Manifest::ConvertToElevationRequirementEnum(
                JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(ElevationRequirement)).value_or(""));

            // list of unsupported OS architectures
            std::optional<std::reference_wrapper<const web::json::array>> unsupportedOSArchitectures = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(UnsupportedOSArchitectures));
            if (unsupportedOSArchitectures)
            {
                for (auto& archValue : unsupportedOSArchitectures.value().get())
                {
                    std::optional<std::string> arch = JSON::GetRawStringValueFromJsonValue(archValue);
                    if (JSON::IsValidNonEmptyStringValue(arch))
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
            std::optional<std::reference_wrapper<const web::json::array>> arpEntriesNode = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(AppsAndFeaturesEntries));
            if (arpEntriesNode)
            {
                installer.AppsAndFeaturesEntries = DeserializeAppsAndFeaturesEntries(arpEntriesNode.value());
            }

            // Markets
            std::optional<std::reference_wrapper<const web::json::value>> marketsNode = JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(Markets));
            if (marketsNode && !marketsNode.value().get().is_null())
            {
                installer.Markets.ExcludedMarkets = V1_0::Json::ManifestDeserializer::ConvertToManifestStringArray(
                    JSON::GetRawStringArrayFromJsonNode(marketsNode.value().get(), JSON::GetUtilityString(ExcludedMarkets)));
                installer.Markets.AllowedMarkets = V1_0::Json::ManifestDeserializer::ConvertToManifestStringArray(
                    JSON::GetRawStringArrayFromJsonNode(marketsNode.value().get(), JSON::GetUtilityString(AllowedMarkets)));
            }

            // Expected return codes
            std::optional<std::reference_wrapper<const web::json::array>> expectedReturnCodesNode = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(ExpectedReturnCodes));
            if (expectedReturnCodesNode)
            {
                for (auto& returnCodeNode : expectedReturnCodesNode.value().get())
                {
                    DWORD installerReturnCode = static_cast<DWORD>(JSON::GetRawIntValueFromJsonNode(returnCodeNode, JSON::GetUtilityString(InstallerReturnCode)).value_or(0));
                    auto returnCodeInfo = DeserializeExpectedReturnCodeInfo(returnCodeNode);

                    // Only add when it is valid
                    if (installerReturnCode != 0 && returnCodeInfo.ReturnResponseEnum != ExpectedReturnCodeEnum::Unknown)
                    {
                        if (!installer.ExpectedReturnCodes.insert({ installerReturnCode, std::move(returnCodeInfo) }).second)
                        {
                            AICLI_LOG(Repo, Error, << "Expected return codes cannot have repeated value.");
                            return {};
                        }
                    }
                }
            }

            // Populate installer default return codes if not present in ExpectedReturnCodes and InstallerSuccessCodes
            auto defaultReturnCodes = GetDefaultKnownReturnCodes(installer.EffectiveInstallerType());
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
            auto agreementsNode = JSON::GetRawJsonArrayFromJsonNode(localeJsonObject, JSON::GetUtilityString(Agreements));
            if (agreementsNode)
            {
                std::vector<Manifest::Agreement> agreements;
                for (auto const& agreementNode : agreementsNode.value().get())
                {
                    Manifest::Agreement agreementEntry;

                    agreementEntry.Label = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(AgreementLabel)).value_or("");
                    agreementEntry.AgreementText = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(Agreement)).value_or("");
                    agreementEntry.AgreementUrl = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(AgreementUrl)).value_or("");

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

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_1;
    }
}

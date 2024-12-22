// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/JsonUtil.h>
#include "Rest/Schema/CommonRestConstants.h"
#include "AuthenticationInfoParser.h"
#include "InformationResponseDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema
{
    namespace
    {
        // Information response constants
        constexpr std::string_view SourceIdentifier = "SourceIdentifier"sv;
        constexpr std::string_view ServerSupportedVersions = "ServerSupportedVersions"sv;

        constexpr std::string_view SourceAgreements = "SourceAgreements"sv;
        constexpr std::string_view SourceAgreementsIdentifier = "AgreementsIdentifier"sv;
        constexpr std::string_view SourceAgreementsContent = "Agreements"sv;
        constexpr std::string_view SourceAgreementLabel = "AgreementLabel"sv;
        constexpr std::string_view SourceAgreementText = "Agreement"sv;
        constexpr std::string_view SourceAgreementUrl = "AgreementUrl"sv;

        constexpr std::string_view UnsupportedPackageMatchFields = "UnsupportedPackageMatchFields"sv;
        constexpr std::string_view RequiredPackageMatchFields = "RequiredPackageMatchFields"sv;
        constexpr std::string_view UnsupportedQueryParameters = "UnsupportedQueryParameters"sv;
        constexpr std::string_view RequiredQueryParameters = "RequiredQueryParameters"sv;
    }

    IRestClient::Information InformationResponseDeserializer::Deserialize(const web::json::value& dataObject) const
    {
        // Get information result from json output.
        std::optional<IRestClient::Information> information = DeserializeInformation(dataObject);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !information);

        return information.value();
    }

    std::optional<IRestClient::Information> InformationResponseDeserializer::DeserializeInformation(const web::json::value& dataObject) const
    {
        try
        {
            if (dataObject.is_null())
            {
                AICLI_LOG(Repo, Error, << "Missing json object.");
                return {};
            }

            std::optional<std::reference_wrapper<const web::json::value>> data = JSON::GetJsonValueFromNode(dataObject, JSON::GetUtilityString(Data));
            if (!data)
            {
                AICLI_LOG(Repo, Error, << "Missing data");
                return {};
            }

            const auto& dataValue = data.value().get();
            std::optional<std::string> sourceId = JSON::GetRawStringValueFromJsonNode(dataValue, JSON::GetUtilityString(SourceIdentifier));
            if (!JSON::IsValidNonEmptyStringValue(sourceId))
            {
                AICLI_LOG(Repo, Error, << "Missing source identifier");
                return {};
            }

            std::vector<std::string> allVersions = JSON::GetRawStringArrayFromJsonNode(dataValue, JSON::GetUtilityString(ServerSupportedVersions));
            if (allVersions.size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing supported versions.");
                return {};
            }

            IRestClient::Information info{ std::move(sourceId.value()), std::move(allVersions) };

            auto agreements = JSON::GetJsonValueFromNode(dataValue, JSON::GetUtilityString(SourceAgreements));
            if (agreements)
            {
                const auto& agreementsValue = agreements.value().get();

                auto agreementsIdentifier = JSON::GetRawStringValueFromJsonNode(agreementsValue, JSON::GetUtilityString(SourceAgreementsIdentifier));
                if (!JSON::IsValidNonEmptyStringValue(agreementsIdentifier))
                {
                    AICLI_LOG(Repo, Error, << "SourceAgreements node exists but AgreementsIdentifier is missing.");
                    return {};
                }

                info.SourceAgreementsIdentifier = std::move(agreementsIdentifier.value());

                auto agreementsContent = JSON::GetRawJsonArrayFromJsonNode(agreementsValue, JSON::GetUtilityString(SourceAgreementsContent));
                if (agreementsContent)
                {
                    for (auto const& agreementNode : agreementsContent.value().get())
                    {
                        IRestClient::SourceAgreementEntry agreementEntry;

                        std::optional<std::string> label = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(SourceAgreementLabel));
                        if (JSON::IsValidNonEmptyStringValue(label))
                        {
                            agreementEntry.Label = std::move(label.value());
                        }

                        std::optional<std::string> text = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(SourceAgreementText));
                        if (JSON::IsValidNonEmptyStringValue(text))
                        {
                            agreementEntry.Text = std::move(text.value());
                        }

                        std::optional<std::string> url = JSON::GetRawStringValueFromJsonNode(agreementNode, JSON::GetUtilityString(SourceAgreementUrl));
                        if (JSON::IsValidNonEmptyStringValue(url))
                        {
                            agreementEntry.Url = std::move(url.value());
                        }

                        if (!agreementEntry.Label.empty() || !agreementEntry.Text.empty() || !agreementEntry.Url.empty())
                        {
                            info.SourceAgreements.emplace_back(std::move(agreementEntry));
                        }
                    }
                }
            }

            info.RequiredPackageMatchFields = JSON::GetRawStringArrayFromJsonNode(dataValue, JSON::GetUtilityString(RequiredPackageMatchFields));
            info.UnsupportedPackageMatchFields = JSON::GetRawStringArrayFromJsonNode(dataValue, JSON::GetUtilityString(UnsupportedPackageMatchFields));
            info.RequiredQueryParameters = JSON::GetRawStringArrayFromJsonNode(dataValue, JSON::GetUtilityString(RequiredQueryParameters));
            info.UnsupportedQueryParameters = JSON::GetRawStringArrayFromJsonNode(dataValue, JSON::GetUtilityString(UnsupportedQueryParameters));

            info.Authentication = ParseAuthenticationInfo(dataValue, ParseAuthenticationInfoType::Source);

            return info;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing Information. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Received invalid information.");
        }

        return {};
    }
}

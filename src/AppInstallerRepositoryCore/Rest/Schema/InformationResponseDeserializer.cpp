// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
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

            std::optional<std::reference_wrapper<const web::json::value>> data = JsonHelper::GetJsonValueFromNode(dataObject, JsonHelper::GetUtilityString(Data));
            if (!data)
            {
                AICLI_LOG(Repo, Error, << "Missing data");
                return {};
            }

            const auto& dataValue = data.value().get();
            std::optional<std::string> sourceId = JsonHelper::GetRawStringValueFromJsonNode(dataValue, JsonHelper::GetUtilityString(SourceIdentifier));
            if (!JsonHelper::IsValidNonEmptyStringValue(sourceId))
            {
                AICLI_LOG(Repo, Error, << "Missing source identifier");
                return {};
            }

            std::vector<std::string> allVersions = JsonHelper::GetRawStringArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(ServerSupportedVersions));
            if (allVersions.size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing supported versions.");
                return {};
            }

            IRestClient::Information info{ std::move(sourceId.value()), std::move(allVersions) };

            auto agreements = JsonHelper::GetJsonValueFromNode(dataValue, JsonHelper::GetUtilityString(SourceAgreements));
            if (agreements)
            {
                const auto& agreementsValue = agreements.value().get();

                auto agreementsIdentifier = JsonHelper::GetRawStringValueFromJsonNode(agreementsValue, JsonHelper::GetUtilityString(SourceAgreementsIdentifier));
                if (!JsonHelper::IsValidNonEmptyStringValue(agreementsIdentifier))
                {
                    AICLI_LOG(Repo, Error, << "SourceAgreements node exists but AgreementsIdentifier is missing.");
                    return {};
                }

                info.SourceAgreementsIdentifier = std::move(agreementsIdentifier.value());

                auto agreementsContent = JsonHelper::GetRawJsonArrayFromJsonNode(agreementsValue, JsonHelper::GetUtilityString(SourceAgreementsContent));
                if (agreementsContent)
                {
                    for (auto const& agreementNode : agreementsContent.value().get())
                    {
                        IRestClient::SourceAgreementEntry agreementEntry;

                        std::optional<std::string> label = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(SourceAgreementLabel));
                        if (JsonHelper::IsValidNonEmptyStringValue(label))
                        {
                            agreementEntry.Label = std::move(label.value());
                        }

                        std::optional<std::string> text = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(SourceAgreementText));
                        if (JsonHelper::IsValidNonEmptyStringValue(text))
                        {
                            agreementEntry.Text = std::move(text.value());
                        }

                        std::optional<std::string> url = JsonHelper::GetRawStringValueFromJsonNode(agreementNode, JsonHelper::GetUtilityString(SourceAgreementUrl));
                        if (JsonHelper::IsValidNonEmptyStringValue(url))
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

            info.RequiredPackageMatchFields = JsonHelper::GetRawStringArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(RequiredPackageMatchFields));
            info.UnsupportedPackageMatchFields = JsonHelper::GetRawStringArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(UnsupportedPackageMatchFields));
            info.RequiredQueryParameters = JsonHelper::GetRawStringArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(RequiredQueryParameters));
            info.UnsupportedQueryParameters = JsonHelper::GetRawStringArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(UnsupportedQueryParameters));

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

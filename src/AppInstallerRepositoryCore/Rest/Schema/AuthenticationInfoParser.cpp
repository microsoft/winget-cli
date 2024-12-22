// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AuthenticationInfoParser.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema
{
    namespace
    {
        // Authentication info constants
        constexpr std::string_view Authentication = "Authentication"sv;
        constexpr std::string_view AuthenticationType = "AuthenticationType"sv;
        constexpr std::string_view MicrosoftEntraIdAuthenticationInfo = "MicrosoftEntraIdAuthenticationInfo"sv;
        constexpr std::string_view MicrosoftEntraId_Resource = "Resource"sv;
        constexpr std::string_view MicrosoftEntraId_Scope = "Scope"sv;

        Authentication::AuthenticationType ConvertToAuthenticationTypeForSource(std::string_view in)
        {
            std::string inStrLower = Utility::ToLower(in);
            Authentication::AuthenticationType result = Authentication::AuthenticationType::Unknown;

            if (inStrLower == "none")
            {
                result = Authentication::AuthenticationType::None;
            }
            else if (inStrLower == "microsoftentraid")
            {
                result = Authentication::AuthenticationType::MicrosoftEntraId;
            }

            return result;
        }

        Authentication::AuthenticationType ConvertToAuthenticationTypeForInstaller(std::string_view in, Manifest::ManifestVer manifestVersion)
        {
            if (manifestVersion >= Manifest::ManifestVer{ Manifest::s_ManifestVersionV1_10 })
            {
                return Authentication::ConvertToAuthenticationType(in);
            }

            return Authentication::AuthenticationType::Unknown;
        }
    }

    // The authentication info json looks like below:
    // "Authentication": {
    //     "AuthenticationType": "microsoftEntraId",
    //     "MicrosoftEntraIdAuthenticationInfo" : {
    //         "Resource": "GUID",
    //         "Scope" : "test"
    //     }
    // }
    Authentication::AuthenticationInfo ParseAuthenticationInfo(const web::json::value& dataObject, ParseAuthenticationInfoType parseType, std::optional<Manifest::ManifestVer> manifestVersion)
    {
        auto authenticationObject = JSON::GetJsonValueFromNode(dataObject, JSON::GetUtilityString(Authentication));
        if (!authenticationObject)
        {
            AICLI_LOG(Repo, Info, << "Authentication node not found. Assuming authentication type none.");
            return {};
        }

        const auto& authenticationObjectNode = authenticationObject.value().get();
        if (authenticationObjectNode.is_null())
        {
            AICLI_LOG(Repo, Info, << "Authentication node is null. Assuming authentication type none.");
            return {};
        }

        Authentication::AuthenticationInfo result;
        result.Type = Authentication::AuthenticationType::Unknown;

        auto authenticationTypeString = JSON::GetRawStringValueFromJsonNode(authenticationObjectNode, JSON::GetUtilityString(AuthenticationType));
        // AuthenticationType required if Authentication exists and is not null.
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !JSON::IsValidNonEmptyStringValue(authenticationTypeString));
        if (parseType == ParseAuthenticationInfoType::Source)
        {
            result.Type = ConvertToAuthenticationTypeForSource(authenticationTypeString.value());
        }
        else if (parseType == ParseAuthenticationInfoType::Installer)
        {
            THROW_HR_IF(E_INVALIDARG, !manifestVersion);
            result.Type = ConvertToAuthenticationTypeForInstaller(authenticationTypeString.value(), manifestVersion.value());
        }

        // Parse MicrosoftEntraId info
        auto microsoftEntraIdInfoObject = JSON::GetJsonValueFromNode(authenticationObjectNode, JSON::GetUtilityString(MicrosoftEntraIdAuthenticationInfo));
        if (microsoftEntraIdInfoObject)
        {
            const auto& microsoftEntraIdInfoNode = microsoftEntraIdInfoObject.value().get();

            Authentication::MicrosoftEntraIdAuthenticationInfo microsoftEntraIdInfo;

            auto resourceString = JSON::GetRawStringValueFromJsonNode(microsoftEntraIdInfoNode, JSON::GetUtilityString(MicrosoftEntraId_Resource));
            // Resource required if MicrosoftEntraIdAuthenticationInfo exists and is not null.
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !JSON::IsValidNonEmptyStringValue(resourceString));
            microsoftEntraIdInfo.Resource = std::move(resourceString.value());

            auto scopeString = JSON::GetRawStringValueFromJsonNode(microsoftEntraIdInfoNode, JSON::GetUtilityString(MicrosoftEntraId_Scope));
            if (JSON::IsValidNonEmptyStringValue(scopeString))
            {
                microsoftEntraIdInfo.Scope = std::move(scopeString.value());
            }

            result.MicrosoftEntraIdInfo = std::move(microsoftEntraIdInfo);
        }

        result.UpdateRequiredFieldsIfNecessary();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !result.ValidateIntegrity());

        return result;
    }
}

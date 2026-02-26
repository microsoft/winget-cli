// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationUnitResultInformation.h"
#include "AppInstallerErrors.h"
#include "AppInstallerStrings.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        ConfigurationUnitResultSource FromHRESULT(hresult resultCode)
        {
            switch (resultCode)
            {
                case WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED:
                case WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY:
                case WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES:
                case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE:
                case WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT:
                case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE_ADMIN:
                    return ConfigurationUnitResultSource::ConfigurationSet;
                case WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT:
                    return ConfigurationUnitResultSource::SystemState;
            }

            return ConfigurationUnitResultSource::Internal;
        }

        hstring SanitizeString(std::wstring_view value)
        {
            using namespace AppInstaller::Utility;
            return hstring{ ConvertToUTF16(ConvertControlCodesToPictures(ConvertToUTF8(value))) };
        }
    }

    void ConfigurationUnitResultInformation::Initialize(const Configuration::IConfigurationUnitResultInformation& other)
    {
        if (other)
        {
            m_resultCode = other.ResultCode();
            m_description = SanitizeString(other.Description());
            m_details = SanitizeString(other.Details());
            m_resultSource = other.ResultSource();
        }
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, std::wstring_view description)
    {
        m_resultCode = resultCode;
        m_description = SanitizeString(description);
        m_resultSource = FromHRESULT(resultCode);
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, hstring description)
    {
        m_resultCode = resultCode;
        m_description = SanitizeString(description);
        m_resultSource = FromHRESULT(resultCode);
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, ConfigurationUnitResultSource resultSource)
    {
        m_resultCode = resultCode;
        m_resultSource = resultSource;
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, std::wstring_view description, std::wstring_view details, ConfigurationUnitResultSource resultSource)
    {
        m_resultCode = resultCode;
        m_description = SanitizeString(description);
        m_details = SanitizeString(details);
        m_resultSource = resultSource;
    }

    hresult ConfigurationUnitResultInformation::ResultCode() const
    {
        return m_resultCode;
    }

    void ConfigurationUnitResultInformation::ResultCode(hresult resultCode)
    {
        m_resultCode = resultCode;
    }

    hstring ConfigurationUnitResultInformation::Description()
    {
        return m_description;
    }

    void ConfigurationUnitResultInformation::Description(hstring value)
    {
        m_description = value;
    }

    hstring ConfigurationUnitResultInformation::Details()
    {
        return m_details;
    }

    void ConfigurationUnitResultInformation::Details(hstring value)
    {
        m_details = value;
    }

    ConfigurationUnitResultSource ConfigurationUnitResultInformation::ResultSource() const
    {
        return m_resultSource;
    }

    void ConfigurationUnitResultInformation::ResultSource(ConfigurationUnitResultSource value)
    {
        m_resultSource = value;
    }
}

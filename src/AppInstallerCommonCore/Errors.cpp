// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerStrings.h"


namespace AppInstaller
{
    namespace
    {
        const char* const GetMessageForAppInstallerHR(HRESULT hr)
        {
            switch (hr)
            {
            case APPINSTALLER_CLI_ERROR_INTERNAL_ERROR:
                return "Internal Error";
            case APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS:
                return "Invalid command line arguments";
            case APPINSTALLER_CLI_ERROR_COMMAND_FAILED:
                return "Executing command failed";
            case APPINSTALLER_CLI_ERROR_MANIFEST_FAILED:
                return "Opening manifest failed";
            case APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED:
                return "Running ShellExecute failed";
            case APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION:
                return "Cannot process manifest. The manifest version is higher than supported. Please update the client.";
            case APPINSTALLER_CLI_ERROR_DOWNLOAD_FAILED:
                return "Downloading installer failed";
            case APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX:
                return "Cannot write to index; it is a higher schema version";
            case APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED:
                return "The index is corrupt";
            case APPINSTALLER_CLI_ERROR_SOURCES_INVALID:
                return "The configured source information is corrupt";
            case APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS:
                return "The source name is already configured";
            case APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE:
                return "The source type is invalid";
            case APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE:
                return "The MSIX file is a bundle, not a package";
            case APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING:
                return "Data required by the source is missing";
            case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER:
                return "None of the installers are applicable for the current system";
            case APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH:
                return "The installer file's hash does not match the manifest";
            case APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST:
                return "The source name does not exist";
            case APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS:
                return "The source location is already configured under another name";
            case APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND:
                return "No packages found";
            case APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED:
                return "No sources are configured";
            case APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND:
                return "Multiple packages found matching the criteria";
            case APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND:
                return "No manifest found matching the criteria";
            case APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN:
                return "Command requires administrator privileges to run";
            case APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE:
                return "The source location is not secure";
            default:
                return "Uknown Error Code";
            }
        }

        void GetUserPresentableMessageForHR(std::ostringstream& strstr, HRESULT hr)
        {
            strstr << "0x" << std::hex << std::setw(8) << std::setfill('0') << hr << " : ";

            if (HRESULT_FACILITY(hr) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                strstr << GetMessageForAppInstallerHR(hr);
            }
            else
            {
                strstr << std::system_category().message(hr);
            }
        }
    }

    std::string GetUserPresentableMessage(const wil::ResultException& re)
    {
        const auto& info = re.GetFailureInfo();

        std::ostringstream strstr;

        // We assume that if the exception has a message, that message is relevant to show to the user.
        if (info.pszMessage)
        {
            strstr << Utility::ConvertToUTF8(info.pszMessage) << std::endl;
        }

        GetUserPresentableMessageForHR(strstr, re.GetErrorCode());

        return strstr.str();
    }

    std::string GetUserPresentableMessage(const winrt::hresult_error& hre)
    {
        return Utility::ConvertToUTF8(hre.message());
    }

    std::string GetUserPresentableMessage(const std::exception& e)
    {
        return e.what();
    }
}

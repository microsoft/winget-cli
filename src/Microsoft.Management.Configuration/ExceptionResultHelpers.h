// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <exception>
#include <AppInstallerLogging.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    template <typename UnitResult>
    void ExtractUnitResultInformation(std::exception_ptr exc, const UnitResult& unitResult)
    {
        try
        {
            std::rethrow_exception(exc);
        }
        catch (const winrt::hresult_error& hre)
        {
            unitResult->Initialize(hre.code(), hre.message());
        }
        catch (...)
        {
            unitResult->Initialize(E_FAIL, hstring{});
        }

        AICLI_LOG(Config, Error, << "Unit Processor exception: " << AppInstaller::Logging::SetHRFormat << unitResult->ResultCode() << " [" << AppInstaller::Utility::ConvertToUTF8(unitResult->Description()) << "]");
    }
}

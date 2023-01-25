// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <exception>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    template <typename UnitResult, typename Processor>
    void ExtractUnitResultInformation(std::exception_ptr exc, const UnitResult& unitResult, const Processor& processor)
    {
        try
        {
            std::rethrow_exception(exc);
        }
        catch (const winrt::hresult_error& hre)
        {
            unitResult->Initialize(hre.code(), processor.GetLastErrorMessage());
        }
        catch (...)
        {
            unitResult->Initialize(E_FAIL, hstring{});
        }
    }
}

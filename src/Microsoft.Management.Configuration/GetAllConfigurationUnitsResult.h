// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetAllConfigurationUnitsResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetAllConfigurationUnitsResult : GetAllConfigurationUnitsResultT<GetAllConfigurationUnitsResult>
    {
        GetAllConfigurationUnitsResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void ResultInformation(const IConfigurationUnitResultInformation& resultInformation);
        void Units(Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>&& value);
#endif

        IConfigurationUnitResultInformation ResultInformation() const;
        Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> Units();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        IConfigurationUnitResultInformation m_resultInformation;
        Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> m_units;
#endif
    };
}

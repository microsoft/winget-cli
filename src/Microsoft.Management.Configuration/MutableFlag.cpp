// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MutableFlag.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void MutableFlag::RequireMutable() const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_isMutable);
    }
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MyRuntimeClass.h"
#include "MyRuntimeClass.g.cpp"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    hstring MyRuntimeClass::Name()
    {
        throw hresult_not_implemented();
    }
    void MyRuntimeClass::Name(hstring const& /*value*/)
    {
        throw hresult_not_implemented();
    }
}

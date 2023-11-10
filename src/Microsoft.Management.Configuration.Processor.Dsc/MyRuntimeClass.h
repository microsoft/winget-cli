// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MyRuntimeClass.g.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct MyRuntimeClass : MyRuntimeClassT<MyRuntimeClass>
    {
        MyRuntimeClass() = default;

        hstring Name();
        void Name(hstring const& value);
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::factory_implementation
{
    struct MyRuntimeClass : MyRuntimeClassT<MyRuntimeClass, implementation::MyRuntimeClass>
    {
    };
}
#endif

#pragma once

#include "ClassBoth.g.h"
#include "ClassSta.g.h"
#include "ClassMta.g.h"

namespace winrt::TestComponent::implementation
{
    struct ClassBoth : ClassBothT<ClassBoth, winrt::non_agile>
    {
        ClassBoth() = default;

        int32_t Apartment();
        void Apartment(int32_t value);
    };

    struct ClassSta : ClassStaT<ClassSta, winrt::non_agile>
    {
        ClassSta() = default;

        int32_t Apartment();
        void Apartment(int32_t value);
    };

    struct ClassMta : ClassMtaT<ClassMta, winrt::non_agile>
    {
        ClassMta() = default;

        int32_t Apartment();
        void Apartment(int32_t value);
    };
}

namespace winrt::TestComponent::factory_implementation
{
    struct ClassBoth : ClassBothT<ClassBoth, implementation::ClassBoth, winrt::non_agile>
    {
    };

    struct ClassSta : ClassStaT<ClassSta, implementation::ClassSta, winrt::non_agile>
    {
    };

    struct ClassMta : ClassMtaT<ClassMta, implementation::ClassMta, winrt::non_agile>
    {
    };
}

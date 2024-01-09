// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscProcessorResult
    {
        std::string stdOut;
        std::string stdErr;
        DWORD exitCode;
    };

    struct DscProcessor
    {
        DscProcessor();
        ~DscProcessor() noexcept = default;

        DscProcessor(const DscProcessor&) = delete;
        DscProcessor& operator=(const DscProcessor&) = delete;
        DscProcessor(DscProcessor&&) = default;
        DscProcessor& operator=(DscProcessor&&) = default;

        DscProcessorResult Execute();
    };
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DscCommandBase.h"

namespace AppInstaller::CLI
{
    // A test resource implementing JSON content configuration of a well known file.
    // This is exists to enable an input-less export, which is required in DSC v3.0.0
    struct DscTestJsonResource : public DscCommandBase
    {
        DscTestJsonResource(std::string_view parent);

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;

        std::string ResourceType() const override;

        void ResourceFunctionGet(Execution::Context& context) const override;
        void ResourceFunctionSet(Execution::Context& context) const override;
        void ResourceFunctionExport(Execution::Context& context) const override;
        void ResourceFunctionSchema(Execution::Context& context) const override;
    };
}

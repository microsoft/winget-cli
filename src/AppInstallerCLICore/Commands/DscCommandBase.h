// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <json/json.h>
#include <optional>

namespace AppInstaller::CLI
{
    // The kind of resource that this command is implementing.
    enum class DscResourceKind
    {
        // Standard resource
        Resource,
        // Group resource
        Group,
        // Adapter resource
        Adapter,
        // Import resource
        Import,
    };

    // The functions that a DSC resource can provide.
    enum class DscFunctions
    {
        None        = 0x000,
        // Gets the current state; should always be implemented.
        Get         = 0x001,
        // Sets the state; should always be implemented.
        Set         = 0x002,
        // Produces the output of Set without modifying state.
        WhatIf      = 0x004,
        // Determines if the current state matches the given state.
        Test        = 0x008,
        // Deletes the given state.
        Delete      = 0x010,
        // Gets all instances of the resource.
        Export      = 0x020,
        // Required for a Group resource, ignored for all others.
        Validate    = 0x040,
        // Required for an Import resource.
        Resolve     = 0x080,
        // Required for an Adapter resource.
        Adapter     = 0x100,
        // Gets the schema for the resource's properties.
        Schema      = 0x200,
    };

    DEFINE_ENUM_FLAG_OPERATORS(DscFunctions);

    // Behavior changes for DSC functions.
    enum class DscFunctionModifiers
    {
        None                = 0x00,
        // The resource implements a check during Set (and WhatIf) to determine if already in the correct state.
        // If not provided, DSC will ensure that the state is tested beforehand.
        ImplementsPretest   = 0x01,
        // The resource will act on the `_exist` property during Set (and WhatIf).
        // If not provided, the resource should implement Delete.
        HandlesExist        = 0x02,
    };

    DEFINE_ENUM_FLAG_OPERATORS(DscFunctionModifiers);

    // Provides infrastructure for DSC commands to be implemented.
    struct DscCommandBase : public Command
    {
        DscCommandBase(std::string_view parent, std::string_view resourceName, DscResourceKind kind, DscFunctions functions, DscFunctionModifiers modifiers);

        std::vector<Argument> GetArguments() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;

        // Gets the resource specific type name.
        virtual std::string ResourceType() const = 0;

        virtual void ResourceFunctionGet(Execution::Context& context) const;
        virtual void ResourceFunctionSet(Execution::Context& context) const;
        virtual void ResourceFunctionWhatIf(Execution::Context& context) const;
        virtual void ResourceFunctionTest(Execution::Context& context) const;
        virtual void ResourceFunctionDelete(Execution::Context& context) const;
        virtual void ResourceFunctionExport(Execution::Context& context) const;
        virtual void ResourceFunctionValidate(Execution::Context& context) const;
        virtual void ResourceFunctionResolve(Execution::Context& context) const;
        virtual void ResourceFunctionAdapter(Execution::Context& context) const;
        virtual void ResourceFunctionSchema(Execution::Context& context) const;
        virtual void ResourceFunctionManifest(Execution::Context& context) const;

        // Parses a JSON object from stdin.
        std::optional<Json::Value> GetJsonFromInput(Execution::Context& context) const;

        // Writes the value to the context output.
        void WriteJsonOutput(Execution::Context& context, const Json::Value& value) const;

    private:
        DscResourceKind m_kind = DscResourceKind::Resource;
        DscFunctions m_functions = DscFunctions::None;
        DscFunctionModifiers m_modifiers = DscFunctionModifiers::None;
    };
}

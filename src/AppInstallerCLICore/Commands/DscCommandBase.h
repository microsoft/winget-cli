// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <json/json.h>
#include <optional>

#ifndef USE_PROD_CLSIDS
#define WINGET_DSCV3_MODULE_NAME "Microsoft.WinGet.Dev"
#define WINGET_DSCV3_MODULE_NAME_WIDE L"Microsoft.WinGet.Dev"
#else
#define WINGET_DSCV3_MODULE_NAME "Microsoft.WinGet"
#define WINGET_DSCV3_MODULE_NAME_WIDE L"Microsoft.WinGet"
#endif

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
        // Functions that may return state information (set, what-if, test) return only the state.
        ReturnsState        = 0x04,
        // Functions that may return state information (set, what-if, test) return the state and property difference.
        ReturnsStateAndDiff = 0x08,
    };

    DEFINE_ENUM_FLAG_OPERATORS(DscFunctionModifiers);

    // Provides infrastructure for DSC commands to be implemented.
    struct DscCommandBase : public Command
    {
        DscCommandBase(std::string_view parent, std::string_view resourceName, DscResourceKind kind, DscFunctions functions, DscFunctionModifiers modifiers);

        std::vector<Argument> GetArguments() const override;

        Utility::LocIndView HelpLink() const override;

        static constexpr std::string_view ModuleName()
        {
            return WINGET_DSCV3_MODULE_NAME;
        }

        // Writes the manifest for the command to the file path.
        // If the path is empty, writes the manifest to the output stream.
        void WriteManifest(Execution::Context& context, const std::filesystem::path& filePath) const;

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
        std::optional<Json::Value> GetJsonFromInput(Execution::Context& context, bool terminateContextOnError = true) const;

        // Writes the value to the context output.
        void WriteJsonOutputLine(Execution::Context& context, const Json::Value& value) const;

    private:
        DscResourceKind m_kind = DscResourceKind::Resource;
        DscFunctions m_functions = DscFunctions::None;
        DscFunctionModifiers m_modifiers = DscFunctionModifiers::None;
    };
}

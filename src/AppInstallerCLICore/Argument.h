// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "Resources.h"
#include <winget/UserSettings.h>
#include <winget/ExperimentalFeature.h>
#include <winget/GroupPolicy.h>
#include <winget/AdminSettings.h>
#include <winget/LocIndependent.h>

#include <string>
#include <string_view>


#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR       '-'
#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING     "-"
#define APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR            '='
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR        '?'
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING      "?"
#define APPINSTALLER_CLI_HELP_ARGUMENT                  APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Utility::literals;

    // The type of argument.
    enum class ArgumentType
    {
        // Argument requires specifying the name before the value.
        Standard,
        // Argument value can be specified alone; position indicates argument name.
        Positional,
        // Only argument name can be specified and indicates a bool value.
        Flag,
    };

    // Categories an arg type can belong to.
    // Used to reason about the arguments present without having to repeat the same
    // lists every time.
    enum class ArgTypeCategory
    {
        None = 0,
        // The --manifest argument.
        Manifest = 0x1,
        // Arguments for querying or selecting a package.
        // E.g.: --query
        PackageQuery = 0x2,
        // Arguments for querying or selecting a package, which do not work for multiple packages.
        // E.g.: --version
        SinglePackageQuery = 0x4,
        // Arguments for installer or uninstaller selection.
        // E.g.: --scope
        InstallerSelection = 0x8,
        // Arguments for installer or uninstaller behavior.
        // E.g.: --interactive
        InstallerBehavior = 0x10,
        // Arguments for installer or uninstaller behavior, which do not work for multiple packages.
        // E.g.: --override
        SingleInstallerBehavior = 0x20,
        // Arguments for selecting or interacting with the source used for initial querying
        // E.g.: --header
        QuerySource = 0x40,
        // Arguments that only make sense when talking about multiple packages
        MultiplePackages = 0x80,
        // Flag arguments that should be copied over when creating a sub-context
        CopyFlagToSubContext = 0x100,
        // Arguments with associated values that should be copied over when creating a sub-context
        CopyValueToSubContext = 0x200,
        // Arguments for selecting or interacting with dependencies or setting specific source behaviors
        // E.g.: --dependency-source
        // E.g.: --accept-source-agreements
        ExtendedSource = 0x400,
        // Arguments for selecting a configuration set (file or history).
        ConfigurationSetChoice = 0x800,
    };

    DEFINE_ENUM_FLAG_OPERATORS(ArgTypeCategory);

    // Exclusive sets an argument can belong to.
    // Only one argument from each exclusive set is allowed at a time.
    enum class ArgTypeExclusiveSet : uint32_t
    {
        None = 0x0,
        ProgressBarOption = 0x1,
        EnableDisable = 0x2,
        PurgePreserve = 0x4,
        PinType = 0x8,
        StubType = 0x10,
        Proxy = 0x20,
        AllAndTargetVersion = 0x40,
        ConfigurationSetChoice = 0x80,

        // This must always be at the end
        Max
    };

    DEFINE_ENUM_FLAG_OPERATORS(ArgTypeExclusiveSet);

    // An argument to a command; containing only data that is common to all its uses.
    // Argument extends this by adding command-specific values, like help strings.
    struct ArgumentCommon
    {
        // Defines an argument with no alias.
        constexpr static char NoAlias = '\0';

        ArgumentCommon(Execution::Args::Type execArgType, Utility::LocIndView name, char alias, Utility::LocIndView alternateName, ArgTypeCategory typeCategory = ArgTypeCategory::None, ArgTypeExclusiveSet exclusiveSet = ArgTypeExclusiveSet::None)
            : Type(execArgType), Name(name), Alias(alias), AlternateName(alternateName), TypeCategory(typeCategory), ExclusiveSet(exclusiveSet) {}

        ArgumentCommon(Execution::Args::Type execArgType, Utility::LocIndView name, char alias, ArgTypeCategory typeCategory = ArgTypeCategory::None, ArgTypeExclusiveSet exclusiveSet = ArgTypeExclusiveSet::None)
            : Type(execArgType), Name(name), Alias(alias), TypeCategory(typeCategory), ExclusiveSet(exclusiveSet) {}

        ArgumentCommon(Execution::Args::Type execArgType, Utility::LocIndView name, Utility::LocIndView alternateName, ArgTypeCategory typeCategory = ArgTypeCategory::None, ArgTypeExclusiveSet exclusiveSet = ArgTypeExclusiveSet::None)
            : Type(execArgType), Name(name), Alias(NoAlias), AlternateName(alternateName), TypeCategory(typeCategory), ExclusiveSet(exclusiveSet) {}

        ArgumentCommon(Execution::Args::Type execArgType, Utility::LocIndView name, ArgTypeCategory typeCategory = ArgTypeCategory::None, ArgTypeExclusiveSet exclusiveSet = ArgTypeExclusiveSet::None)
            : Type(execArgType), Name(name), Alias(NoAlias), TypeCategory(typeCategory), ExclusiveSet(exclusiveSet) {}

        // Gets the argument for the given type.
        static ArgumentCommon ForType(Execution::Args::Type execArgType);

        static std::vector<ArgumentCommon> GetFromExecArgs(const Execution::Args& execArgs);

        Execution::Args::Type Type;
        Utility::LocIndView Name;
        char Alias;
        Utility::LocIndView AlternateName;
        ArgTypeCategory TypeCategory;
        ArgTypeExclusiveSet ExclusiveSet;
    };

    // An argument to a command.
    struct Argument
    {
        // Controls the visibility of the field.
        enum class Visibility
        {
            // Shown in the example.
            Example,
            // Shown only in the table below the example.
            Help,
            // Not shown in help.
            Hidden,
        };

        // Defines an argument with no alternate name
        constexpr static std::string_view NoAlternateName = "";

        Argument(Execution::Args::Type execArgType, Resource::StringId desc) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, bool required) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_required(required) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Argument::Visibility visibility) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_visibility(visibility) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, bool required) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_required(required) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Argument::Visibility visibility, bool required) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_visibility(visibility), m_required(required) {}

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Constructors for arguments with custom names and aliases to use in tests
        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type) :
            m_argCommon(execArgType, Utility::LocIndView{ name }, alias), m_desc(std::move(desc)), m_type(type) {}

        Argument(std::string_view name, char alias, std::string_view alternateName, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type) :
            m_argCommon(execArgType, Utility::LocIndView{ name }, alias, Utility::LocIndView{ alternateName }), m_desc(std::move(desc)), m_type(type) {}
#endif

        ~Argument() = default;

        Argument(const Argument&) = default;
        Argument& operator=(const Argument&) = default;

        Argument(Argument&&) = default;
        Argument& operator=(Argument&&) = default;

        // Gets the argument for the given type.
        static Argument ForType(Execution::Args::Type type);

        // Gets the common arguments for all commands.
        static void GetCommon(std::vector<Argument>& args);

        // Static argument validation helpers; throw CommandException when validation fails.

        // Requires that at most one argument from the list is present.
        static void ValidateExclusiveArguments(const Execution::Args& args);

        // Requires that if an argument depends on another one, it is not present without the dependency.
        static void ValidateArgumentDependency(const Execution::Args& args, Execution::Args::Type type, Execution::Args::Type dependencyArgType);

        static ArgTypeCategory GetCategoriesPresent(const Execution::Args& arg);

        // Requires that arguments meet common requirements
        static ArgTypeCategory GetCategoriesAndValidateCommonArguments(const Execution::Args& args, bool requirePackageSelectionArg = true);
        static void ValidateCommonArguments(const Execution::Args& args, bool requirePackageSelectionArg = true) { std::ignore = GetCategoriesAndValidateCommonArguments(args, requirePackageSelectionArg); }

        // Gets the argument usage string in the format of "-alias,--name".
        std::string GetUsageString() const;

        // Arguments are not localized at this time.
        Utility::LocIndView Name() const { return m_argCommon.Name; }
        char Alias() const { return m_argCommon.Alias; }
        std::string_view AlternateName() const { return m_argCommon.AlternateName; }
        Execution::Args::Type ExecArgType() const { return m_argCommon.Type; }
        const Resource::StringId& Description() const { return m_desc; }
        bool Required() const { return m_required; }
        ArgumentType Type() const { return m_type; }
        size_t Limit() const { return m_countLimit; }
        Argument::Visibility GetVisibility() const;
        Settings::ExperimentalFeature::Feature Feature() const { return m_feature; }
        Settings::TogglePolicy::Policy GroupPolicy() const { return m_groupPolicy; }
        Settings::BoolAdminSetting AdminSetting() const { return m_adminSetting; }

        Argument& SetRequired(bool required) { m_required = required; return *this; }
        Argument& SetCountLimit(size_t countLimit) { m_countLimit = countLimit; return *this; }

    private:
        // Constructors that set a Feature or Policy are private to force callers to go through the ForType() function.
        // This helps keep it all in one place to reduce chances of missing it somewhere.
        Argument(Execution::Args::Type execArgType, Resource::StringId desc, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, bool required, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_required(required), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Argument::Visibility visibility, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_visibility(visibility), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, bool required, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_required(required), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Argument::Visibility visibility, bool required, Settings::ExperimentalFeature::Feature feature) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_visibility(visibility), m_required(required), m_feature(feature) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Settings::TogglePolicy::Policy groupPolicy, Settings::BoolAdminSetting adminSetting) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_groupPolicy(groupPolicy), m_adminSetting(adminSetting) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Argument::Visibility visibility, Settings::TogglePolicy::Policy groupPolicy, Settings::BoolAdminSetting adminSetting) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_visibility(visibility), m_groupPolicy(groupPolicy), m_adminSetting(adminSetting) {}

        Argument(Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Settings::ExperimentalFeature::Feature feature, Settings::TogglePolicy::Policy groupPolicy, Settings::BoolAdminSetting adminSetting) :
            m_argCommon(ArgumentCommon::ForType(execArgType)), m_desc(std::move(desc)), m_type(type), m_feature(feature), m_groupPolicy(groupPolicy), m_adminSetting(adminSetting) {}

        ArgumentCommon m_argCommon;
        Resource::StringId m_desc;
        bool m_required = false;
        ArgumentType m_type = ArgumentType::Standard;
        Argument::Visibility m_visibility = Argument::Visibility::Example;
        size_t m_countLimit = 1;
        Settings::ExperimentalFeature::Feature m_feature = Settings::ExperimentalFeature::Feature::None;
        Settings::TogglePolicy::Policy m_groupPolicy = Settings::TogglePolicy::Policy::None;
        Settings::BoolAdminSetting m_adminSetting = Settings::BoolAdminSetting::Unknown;
    };
}

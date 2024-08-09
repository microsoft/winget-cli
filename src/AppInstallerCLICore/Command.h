// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Argument.h"
#include "ExecutionContext.h"
#include "Invocation.h"
#include "Resources.h"
#include <winget/UserSettings.h>
#include <winget/ExperimentalFeature.h>
#include <winget/GroupPolicy.h>

#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace AppInstaller::CLI
{
    struct CommandException
    {
        CommandException(Resource::LocString message) : m_message(std::move(message)) {}
        const Utility::LocIndString Message() const { return m_message; }

    private:
        Resource::LocString m_message;
    };

    // Flags to control the behavior of the command output.
    enum class CommandOutputFlags : int
    {
        None = 0x0,
        IgnoreSettingsWarnings = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(CommandOutputFlags);

    struct Command
    {
        // Controls the visibility of the field.
        enum class Visibility
        {
            // Shown in help.
            Show,
            // Not shown in help.
            Hidden,
        };

        Command(std::string_view name, std::string_view parent) :
            Command(name, {}, parent) {}
        Command(std::string_view name, std::vector<std::string_view> aliases, std::string_view parent) :
            Command(name, aliases, parent, Settings::ExperimentalFeature::Feature::None) {}
        Command(std::string_view name, std::string_view parent, CommandOutputFlags outputFlags) :
            Command(name, {}, parent, Command::Visibility::Show, Settings::ExperimentalFeature::Feature::None, Settings::TogglePolicy::Policy::None, outputFlags) {}
        Command(std::string_view name, std::vector<std::string_view> aliases, std::string_view parent, Command::Visibility visibility) :
            Command(name, aliases, parent, visibility, Settings::ExperimentalFeature::Feature::None) {}
        Command(std::string_view name, std::string_view parent, Settings::ExperimentalFeature::Feature feature) :
            Command(name, {}, parent, Command::Visibility::Show, feature) {}
        Command(std::string_view name, std::vector<std::string_view> aliases, std::string_view parent, Settings::ExperimentalFeature::Feature feature) :
            Command(name, aliases, parent, Command::Visibility::Show, feature) {}
        Command(std::string_view name, std::vector<std::string_view> aliases, std::string_view parent, Settings::TogglePolicy::Policy groupPolicy) :
            Command(name, aliases, parent, Command::Visibility::Show, Settings::ExperimentalFeature::Feature::None, groupPolicy, CommandOutputFlags::None) {}
        Command(std::string_view name, std::vector<std::string_view> aliases, std::string_view parent, Command::Visibility visibility, Settings::ExperimentalFeature::Feature feature) :
            Command(name, aliases, parent, visibility, feature, Settings::TogglePolicy::Policy::None, CommandOutputFlags::None) {}

        Command(std::string_view name,
            std::vector<std::string_view> aliases,
            std::string_view parent,
            Command::Visibility visibility,
            Settings::ExperimentalFeature::Feature feature,
            Settings::TogglePolicy::Policy groupPolicy,
            CommandOutputFlags outputFlags);

        virtual ~Command() = default;

        Command(const Command&) = default;
        Command& operator=(const Command&) = default;

        Command(Command&&) = default;
        Command& operator=(Command&&) = default;

        // The character used to split between commands and their parents in FullName.
        constexpr static char ParentSplitChar = ':';

        std::string_view Name() const { return m_name; }
        const std::vector<std::string_view>& Aliases() const& { return m_aliases; }
        const std::string& FullName() const { return m_fullName; }
        Command::Visibility GetVisibility() const;
        Settings::ExperimentalFeature::Feature Feature() const { return m_feature; }
        Settings::TogglePolicy::Policy GroupPolicy() const { return m_groupPolicy; }
        CommandOutputFlags GetOutputFlags() const { return m_outputFlags; }

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const { return {}; }
        virtual std::vector<Argument> GetArguments() const { return {}; }
        std::vector<std::unique_ptr<Command>> GetVisibleCommands() const;
        std::vector<Argument> GetVisibleArguments() const;

        virtual Resource::LocString ShortDescription() const { return {}; }
        virtual Resource::LocString LongDescription() const { return {}; }

        virtual void OutputIntroHeader(Execution::Reporter& reporter) const;
        virtual void OutputHelp(Execution::Reporter& reporter, const CommandException* exception = nullptr) const;
        virtual Utility::LocIndView HelpLink() const { return {}; }

        virtual std::unique_ptr<Command> FindSubCommand(Invocation& inv) const;
        virtual void ParseArguments(Invocation& inv, Execution::Args& execArgs) const;
        virtual void ValidateArguments(Execution::Args& execArgs) const;

        virtual void Complete(Execution::Context& context) const;
        virtual void Complete(Execution::Context& context, Execution::Args::Type valueType) const;

        virtual void Execute(Execution::Context& context) const;

        virtual void Resume(Execution::Context& context) const;

    protected:
        void SelectCurrentCommandIfUnrecognizedSubcommandFound(bool value);

        virtual void ValidateArgumentsInternal(Execution::Args& execArgs) const;
        virtual void ExecuteInternal(Execution::Context& context) const;

    private:
        std::string_view m_name;
        std::string m_fullName;
        std::vector<std::string_view> m_aliases;
        Command::Visibility m_visibility;
        Settings::ExperimentalFeature::Feature m_feature;
        Settings::TogglePolicy::Policy m_groupPolicy;
        CommandOutputFlags m_outputFlags;
        bool m_selectCurrentCommandIfUnrecognizedSubcommandFound = false;
        std::string m_commandArguments;
    };

    template <typename Container>
    Container InitializeFromMoveOnly(std::initializer_list<typename Container::value_type> il)
    {
        using Value = typename Container::value_type;
        Container result;

        for (const auto& v : il)
        {
            result.emplace_back(std::move(*const_cast<Value*>(&v)));
        }

        return result;
    }

    void ExecuteWithoutLoggingSuccess(Execution::Context& context, Command* command);

    int Execute(Execution::Context& context, std::unique_ptr<Command>& command);
}

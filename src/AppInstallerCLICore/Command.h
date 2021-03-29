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
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace AppInstaller::CLI
{
    struct CommandException
    {
        // The message should be a localized string.
        // The parameters can be either localized or not.
        // We 'convert' the param to a localization independent view here if needed.
        CommandException(Resource::LocString message, Resource::LocString param) : m_message(std::move(message)), m_param(param) {}
        CommandException(Resource::LocString message, std::string_view param) : m_message(std::move(message)), m_param(param) {}

        const Resource::LocString& Message() const { return m_message; }
        const Utility::LocIndString& Param() const { return m_param; }

    private:
        Resource::LocString m_message;
        Utility::LocIndString m_param;
    };

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
            Command(name, parent, Settings::ExperimentalFeature::Feature::None) {}
        Command(std::string_view name, std::string_view parent, Command::Visibility visibility) :
            Command(name, parent, visibility, Settings::ExperimentalFeature::Feature::None) {}
        Command(std::string_view name, std::string_view parent, Settings::ExperimentalFeature::Feature feature) :
            Command(name, parent, Command::Visibility::Show, feature) {}
        Command(std::string_view name, std::string_view parent, Settings::TogglePolicy::Policy groupPolicy) :
            Command(name, parent, Command::Visibility::Show, Settings::ExperimentalFeature::Feature::None, groupPolicy) {}
        Command(std::string_view name, std::string_view parent, Command::Visibility visibility, Settings::ExperimentalFeature::Feature feature) :
            Command(name, parent, visibility, feature, Settings::TogglePolicy::Policy::None) {}
        Command(std::string_view name, std::string_view parent, Command::Visibility visibility, Settings::ExperimentalFeature::Feature feature, Settings::TogglePolicy::Policy groupPolicy);
        virtual ~Command() = default;

        Command(const Command&) = default;
        Command& operator=(const Command&) = default;

        Command(Command&&) = default;
        Command& operator=(Command&&) = default;

        // The character used to split between commands and their parents in FullName.
        constexpr static char ParentSplitChar = ':';

        std::string_view Name() const { return m_name; }
        const std::string& FullName() const { return m_fullName; }
        Command::Visibility GetVisibility() const;
        Settings::ExperimentalFeature::Feature Feature() const { return m_feature; }
        Settings::TogglePolicy::Policy GroupPolicy() const { return m_groupPolicy; }

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const { return {}; }
        virtual std::vector<Argument> GetArguments() const { return {}; }
        std::vector<std::unique_ptr<Command>> GetVisibleCommands() const;
        std::vector<Argument> GetVisibleArguments() const;

        virtual Resource::LocString ShortDescription() const { return {}; }
        virtual Resource::LocString LongDescription() const { return {}; }

        virtual void OutputIntroHeader(Execution::Reporter& reporter) const;
        virtual void OutputHelp(Execution::Reporter& reporter, const CommandException* exception = nullptr) const;
        virtual std::string HelpLink() const { return {}; }

        virtual std::unique_ptr<Command> FindSubCommand(Invocation& inv) const;
        virtual void ParseArguments(Invocation& inv, Execution::Args& execArgs) const;
        virtual void ValidateArguments(Execution::Args& execArgs) const;

        virtual void Complete(Execution::Context& context) const;
        virtual void Complete(Execution::Context& context, Execution::Args::Type valueType) const;

        virtual void Execute(Execution::Context& context) const;

    protected:
        virtual void ValidateArgumentsInternal(Execution::Args& execArgs) const;
        virtual void ExecuteInternal(Execution::Context& context) const;

    private:
        std::string_view m_name;
        std::string m_fullName;
        Command::Visibility m_visibility;
        Settings::ExperimentalFeature::Feature m_feature;
        Settings::TogglePolicy::Policy m_groupPolicy;
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

    int Execute(Execution::Context& context, std::unique_ptr<Command>& command);
}

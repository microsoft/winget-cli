// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Argument.h"
#include "ExecutionContext.h"
#include "Invocation.h"

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
        CommandException(std::string message, std::string_view param) : m_message(message), m_param(param) {}

        const std::string& Message() const { return m_message; }
        const std::string_view Param() const { return m_param; }

    private:
        std::string m_message;
        std::string_view m_param;
    };

    struct Command
    {
        Command(std::string_view name, std::string_view parent);
        virtual ~Command() = default;

        Command(const Command&) = default;
        Command& operator=(const Command&) = default;

        Command(Command&&) = default;
        Command& operator=(Command&&) = default;

        // The character used to split between commands and their parents in FullName.
        constexpr static char ParentSplitChar = ':';

        std::string_view Name() const { return m_name; }
        const std::string& FullName() const { return m_fullName; }

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const { return {}; }
        virtual std::vector<Argument> GetArguments() const { return {}; }

        virtual std::string ShortDescription() const { return {}; }
        virtual std::string GetLongDescription() const { return {}; }

        virtual void OutputIntroHeader(Execution::Reporter& reporter) const;
        virtual void OutputHelp(Execution::Reporter& reporter, const CommandException* exception = nullptr) const;

        virtual std::unique_ptr<Command> FindSubCommand(Invocation& inv) const;
        virtual void ParseArguments(Invocation& inv, Execution::Args& execArgs) const;
        virtual void ValidateArguments(Execution::Args& execArgs) const;

        virtual void Execute(Execution::Context& context) const;

    protected:
        virtual void ValidateArgumentsInternal(Execution::Args& execArgs) const;
        virtual void ExecuteInternal(Execution::Context& context) const;

    private:
        std::string_view m_name;
        std::string m_fullName;
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
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <initializer_list>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include "Invocation.h"
#include "ExecutionContext.h"

#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR      '-'
#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING    "-"
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR       '?'
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING     "?"
#define APPINSTALLER_CLI_HELP_ARGUMENT          APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING

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

    enum class ArgumentType
    {
        // Argument requires specifying the name before the value.
        Standard,
        // Argument value can be specified alone; position indicates argument name.
        Positional,
        // Only argument name can be specified and indicates a bool value.
        Flag,
    };

    struct Argument
    {
        Argument(std::string_view name, ExecutionArgs::Type execArgType, std::string desc) :
            m_name(name), m_execArgType(execArgType), m_desc(std::move(desc)) {}

        Argument(std::string_view name, ExecutionArgs::Type execArgType, std::string desc, bool required) :
            m_name(name), m_execArgType(execArgType), m_desc(std::move(desc)), m_required(required) {}

        Argument(std::string_view name, ExecutionArgs::Type execArgType, std::string desc, ArgumentType type) :
            m_name(name), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type) {}

        Argument(std::string_view name, ExecutionArgs::Type execArgType, std::string desc, ArgumentType type, bool required) :
            m_name(name), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type), m_required(required) {}

        ~Argument() = default;

        Argument(const Argument&) = default;
        Argument& operator=(const Argument&) = default;

        Argument(Argument&&) = default;
        Argument& operator=(Argument&&) = default;

        std::string_view Name() const { return m_name; }
        ExecutionArgs::Type ExecArgType() const { return m_execArgType; }
        std::string Description() const { return m_desc; }
        bool Required() const { return m_required; }
        ArgumentType Type() const { return m_type; }
        size_t Limit() const { return m_countLimit; }

    private:
        std::string_view m_name;
        ExecutionArgs::Type m_execArgType;
        std::string m_desc;
        bool m_required = false;
        ArgumentType m_type = ArgumentType::Standard;
        size_t m_countLimit = 1;
    };

    struct Command
    {
        Command(std::string_view name) : m_name(name) {}
        virtual ~Command() = default;

        Command(const Command&) = default;
        Command& operator=(const Command&) = default;

        Command(Command&&) = default;
        Command& operator=(Command&&) = default;

        std::string_view Name() const { return m_name; }

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const { return {}; }
        virtual std::vector<Argument> GetArguments() const { return {}; }

        virtual std::string ShortDescription() const { return {}; }
        virtual std::vector<std::string> GetLongDescription() const { return {}; }

        virtual void OutputIntroHeader(ExecutionReporter& reporter) const;
        virtual void OutputHelp(ExecutionReporter& reporter, const CommandException* exception = nullptr) const;

        virtual std::unique_ptr<Command> FindInvokedCommand(Invocation& inv) const;
        virtual void ParseArguments(Invocation& inv, ExecutionArgs& execArgs) const;
        virtual void ValidateArguments(ExecutionArgs& execArgs) const;

        virtual void Execute(ExecutionContext& context) const;

    protected:
        virtual void ExecuteInternal(ExecutionContext& context) const;

    private:
        std::string_view m_name;
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

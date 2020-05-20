// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "Resources.h"

#include <string>
#include <string_view>


#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR       '-'
#define APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING     "-"
#define APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR            '='
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR        '?'
#define APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING      "?"
#define APPINSTALLER_CLI_HELP_ARGUMENT                  APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING

#define APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER          '\0'

namespace AppInstaller::CLI
{
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

    // An argument to a command.
    struct Argument
    {
        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)) {}

        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, bool required) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)), m_required(required) {}

        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type) {}

        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Visibility visibility) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type), m_visibility(visibility) {}

        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, bool required) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type), m_required(required) {}

        Argument(std::string_view name, char alias, Execution::Args::Type execArgType, Resource::StringId desc, ArgumentType type, Visibility visibility, bool required) :
            m_name(name), m_alias(alias), m_execArgType(execArgType), m_desc(std::move(desc)), m_type(type), m_visibility(visibility), m_required(required) {}

        ~Argument() = default;

        Argument(const Argument&) = default;
        Argument& operator=(const Argument&) = default;

        Argument(Argument&&) = default;
        Argument& operator=(Argument&&) = default;

        // Gets the argument for the given type.
        static Argument ForType(Execution::Args::Type type);

        // Gets the common arguments for all commands.
        static void GetCommon(std::vector<Argument>& args);

        // Arguments are not localized at this time.
        Utility::LocIndView Name() const { return Utility::LocIndView{ m_name }; }
        char Alias() const { return m_alias; }
        Execution::Args::Type ExecArgType() const { return m_execArgType; }
        const Resource::StringId& Description() const { return m_desc; }
        bool Required() const { return m_required; }
        ArgumentType Type() const { return m_type; }
        size_t Limit() const { return m_countLimit; }
        Visibility Visibility() const { return m_visibility; }

        Argument& SetRequired(bool required) { m_required = required; return *this; }

    private:
        std::string_view m_name;
        char m_alias;
        Execution::Args::Type m_execArgType;
        Resource::StringId m_desc;
        bool m_required = false;
        ArgumentType m_type = ArgumentType::Standard;
        ::AppInstaller::CLI::Visibility m_visibility = Visibility::Example;
        size_t m_countLimit = 1;
    };
}

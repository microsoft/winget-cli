// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Public/winget/MsiExecArguments.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Msi
{
    using namespace std::string_view_literals;

    namespace
    {
        const char MSIEXEC_QUIET_OPTION = 'q';
        const char MSIEXEC_LOG_OPTION = 'l';

        // Modifiers for UI level
        // Description of how a long option is replaced by a short option.
        struct TokenReplacement
        {
            TokenReplacement(std::string_view longOption, std::string_view shortOption) : LongOption(longOption), ShortOption({ shortOption }) {}
            TokenReplacement(std::string_view longOption, std::vector<std::string_view>&& shortOption) : LongOption(longOption), ShortOption(std::move(shortOption)) {}
            std::string_view LongOption;
            std::vector<std::string_view> ShortOption;
        };

        // Determines whether an argument token is a switch/option.
        bool IsSwitch(std::string_view token)
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, token.empty());
            return token[0] == '-' || token[0] == '/';
        }

        void ParseLogOption(std::string_view modifier, std::string_view value, MsiParsedArguments& parsedArgs)
        {
            UNREFERENCED_PARAMETER(modifier);
            UNREFERENCED_PARAMETER(value);
            UNREFERENCED_PARAMETER(parsedArgs);
        }

        // Parses the modifier for the UI Level option (/q)
        // The modifier starts with a base (b, f, n, r), followed by extra flags (+, -, !).
        // E.g. /qn, /qb-!
        void ParseQuietOption(std::string_view modifier, MsiParsedArguments& parsedArgs)
        {
            if (modifier.empty())
            {
                // /q is treated as equivalent to /qn
                modifier = "n"sv;
            }

            // Lower values in INSTALLUILEVEL work like a base enum (e.g. None=2, Basic=3)
            // with higher values being modifying flags (e.g. HideCancel=0x20, ProgressOnly=0x40).
            // Some steps depend on the base enum, so we keep it separate for easier checking.
            INSTALLUILEVEL uiLevelBase = {};
            INSTALLUILEVEL uiLevelModifiers = {};

            // Parse the base level
            switch (std::tolower(modifier[0]))
            {
            case 'f':
                uiLevelBase = INSTALLUILEVEL_FULL;
                break;
            case 'r':
                uiLevelBase = INSTALLUILEVEL_REDUCED;
                break;
            case 'b':
                uiLevelBase = INSTALLUILEVEL_BASIC;
                break;
            case '+':
                uiLevelBase = INSTALLUILEVEL_NONE;
                uiLevelModifiers = INSTALLUILEVEL_ENDDIALOG;
                break;
            case 'n':
                uiLevelBase = INSTALLUILEVEL_NONE;
                break;
            default:
                AICLI_LOG(Core, Error, << "Invalid modifier for msiexec /q argument: " << modifier);
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            };

            // Parse the modifiers
            for (size_t i = 1; i < modifier.size(); ++i)
            {
                const char c = modifier[i];

                if (c == '+')
                {
                    WI_SetFlag(uiLevelModifiers, INSTALLUILEVEL_ENDDIALOG);
                }
                else if (c == '-')
                {
                    if (uiLevelBase == INSTALLUILEVEL_BASIC)
                    {
                        WI_SetFlag(uiLevelModifiers, INSTALLUILEVEL_PROGRESSONLY);
                    }
                    else
                    {
                        AICLI_LOG(Core, Error, << "msiexec UI option Progress Only (-) is only valid with UI level Basic (b)");
                        THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                    }
                }
                else if (c == '!')
                {
                    if (uiLevelBase == INSTALLUILEVEL_BASIC)
                    {
                        WI_SetFlag(uiLevelModifiers, INSTALLUILEVEL_HIDECANCEL);
                    }
                    else
                    {
                        AICLI_LOG(Core, Error, << "msiexec UI option Hide Cancel (!) is only valid with UI level Basic (b)");
                        THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                    }
                }
            }

            // Only deviation from msiexec:
            // When using UI Level None, allow showing the UAC prompt.
            WI_SetFlagIf(uiLevelModifiers, INSTALLUILEVEL_UACONLY, uiLevelBase == INSTALLUILEVEL_NONE);


            parsedArgs.UILevel = uiLevelBase | uiLevelModifiers;
        }

        // Split the arguments string into tokens. Tokens are delimited by whitespace,
        // but consider quotes. Each token represents an option (like /q), an argument
        // for an option, or a property.
        std::list<std::string> TokenizeMsiArguments(std::string_view arguments)
        {
            // TODO: This only splits in whitespace. Need to split better around quotes.
            std::stringstream ss(arguments.data());
            std::list<std::string> result;
            std::string s;
            while (ss >> s)
            {
                result.push_back(std::move(s));
            }

            return result;
        }

        // Replaces long options in the arguments (e.g. /quiet), by their short equivalents
        // (e.g. /qn). The replacement is done in-place.
        void ReplaceLongOptions(std::list<std::string>& tokens)
        {
            // We don't handle all possible options because we don't need to.
            // Options not handled:
            //   /update
            //   /uninstall
            //   /package
            //   /help
            const std::vector<TokenReplacement> Replacements
            {
                { "quiet"sv, "/qn"sv },
                { "passive"sv, { "/qb!-"sv, "REBOOTPROMPT=S"sv } },
                { "norestart"sv, "REBOOT=ReallySuppress"sv },
                { "forcerestart"sv, "REBOOT=Force"sv },
                { "promptrestart"sv, "REBOOTPROMPT=\"\""sv },
                { "log"sv, "/l*"sv },
            };

            auto itr = tokens.begin();
            while (itr != tokens.end())
            {
                if (!IsSwitch(*itr))
                {
                    // We only need to replace switches.
                    ++itr;
                    continue;
                }

                // Find if there is a replacement for this option.
                // We ignore the leading / or - when comparing.
                auto option = std::string_view(*itr).substr(1);
                auto replacementItr = std::find_if(Replacements.begin(), Replacements.end(), [&](const TokenReplacement& replacement) { return Utility::CaseInsensitiveEquals(replacement.LongOption, option); });
                if (replacementItr == Replacements.end())
                {
                    // There is no replacement for this switch;
                    ++itr;
                    continue;
                }

                // Add all the replacements tokens needed before this one, then delete the existing token.
                tokens.insert(itr, replacementItr->ShortOption.begin(), replacementItr->ShortOption.end());

                // Delete the current token an move to the next one.
                // We don't need to do anything more to the newly added tokens.
                itr = tokens.erase(itr);
            }
        }

        // Consumes the next argument token in the list. If the token is an option
        // that takes an argument, also consumes it. After consumen the token(s),
        // removes it from the list and updates the parsed arguments accordingly.
        void ConsumeNextToken(std::list<std::string>& tokens, MsiParsedArguments& parsedArgs)
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, tokens.empty());

            const auto token = std::move(tokens.front());
            tokens.pop_front();
            if (!IsSwitch(token))
            {
                // Token is a property, i.e. NAME=value. Add it to the parsed args.
                parsedArgs.Properties += L" " + Utility::ConvertToUTF16(token);
                tokens.pop_front();
                return;
            }

            // Token is an option.
            if (token.size() <= 1)
            {
                AICLI_LOG(Core, Error, << "Invalid command line argument for msiexec: " << token);
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            }

            char option = token[1];
            auto optionModifier = std::string_view(token).substr(2);

            // Options are case-insensitive
            switch (std::tolower(option))
            {
            case MSIEXEC_QUIET_OPTION:
            {
                ParseQuietOption(optionModifier, parsedArgs);
                break;
            }
            case MSIEXEC_LOG_OPTION:
            {
                if (tokens.empty())
                {
                    // Log option must be followed by an option argument
                    AICLI_LOG(Core, Error, << "msiexec option " << token << " must be followed by a value");
                    THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                }

                const auto optionValue = std::move(tokens.front());
                tokens.pop_front();
                ParseLogOption(optionModifier, optionValue, parsedArgs);
                break;
            }
            default:
            {
                AICLI_LOG(Core, Error, << "Invalid option for msiexec: " << token);
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            }
            }
        }
    }

    MsiParsedArguments ParseMSIArguments(std::string_view arguments)
    {
        // Split the arguments into tokens, which we will process one by one.
        auto argumentTokens = TokenizeMsiArguments(arguments);

        // Replace long options so we can work only with short ones.
        ReplaceLongOptions(argumentTokens);

        // Process the arguments.
        MsiParsedArguments result;
        while (!argumentTokens.empty())
        {
            ConsumeNextToken(argumentTokens, result);
        }

        return result;
    }
}
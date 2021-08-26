// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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
        const char MsiExecQuietOption = 'q';
        const char MsiExecLogOption = 'l';

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

        // Parses the log mode and log file for the Log (/l) option.
        // The option has a modifier specifying the log mode (what is logged)
        // and a value specifying the log file.
        // E.g. /l* log.txt, /lw warnings.txt
        void ParseLogOption(std::string_view logModeString, std::string_view logFile, MsiParsedArguments& parsedArgs)
        {
            if (Utility::IsEmptyOrWhitespace(logFile))
            {
                AICLI_LOG(Core, Error, << "MSI log file path cannot be empty");
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            }

            INSTALLLOGMODE logMode = {};
            INSTALLLOGATTRIBUTES logAttributes = {};

            // Note: These flags are mostly consecutive bits in the order given, except where indicated.
            // Skipped flags are not mapped to a command line option.
            std::map<char, INSTALLLOGMODE> ValidLogModes
            {
                { 'm', INSTALLLOGMODE_FATALEXIT },
                { 'e', INSTALLLOGMODE_ERROR },
                { 'w', INSTALLLOGMODE_WARNING },
                { 'u', INSTALLLOGMODE_USER },
                { 'i', INSTALLLOGMODE_INFO },
                // FILESINUSE
                // RESOLVESOURCE
                { 'o', INSTALLLOGMODE_OUTOFDISKSPACE },
                { 'a', INSTALLLOGMODE_ACTIONSTART },
                { 'r', INSTALLLOGMODE_ACTIONDATA },
                { 'p', INSTALLLOGMODE_PROPERTYDUMP },
                { 'c', INSTALLLOGMODE_COMMONDATA },
                { 'v', INSTALLLOGMODE_VERBOSE },
                { 'x', INSTALLLOGMODE_EXTRADEBUG },
                // LOGONLYONERROR
                // LOGPERFORMANCE
            };

            std::map<char, INSTALLLOGATTRIBUTES> ValidLogAttributes
            {
                { '+', INSTALLLOGATTRIBUTES_APPEND },
                { '!', INSTALLLOGATTRIBUTES_FLUSHEACHLINE },
            };

            bool isLogModeSet = false;
            for (char c : logModeString)
            {
                // Log-all option
                if (c == '*')
                {
                    logMode |= AllLogMode;
                    isLogModeSet = true;
                    continue;
                }

                auto modeItr = ValidLogModes.find(c);
                if (modeItr != ValidLogModes.end())
                {
                    logMode |= modeItr->second;
                    isLogModeSet = true;
                    continue;
                }

                auto attributeItr = ValidLogAttributes.find(c);
                if (attributeItr != ValidLogAttributes.end())
                {
                    logAttributes |= attributeItr->second;
                    continue;
                }

                AICLI_LOG(Core, Error, << "Unknown msiexec log modifier: " << c);
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            }

            if (!isLogModeSet)
            {
                logMode = DefaultLogMode;
            }

            parsedArgs.LogMode = logMode;
            parsedArgs.LogAttributes = logAttributes;
            parsedArgs.LogFile = Utility::ConvertToUTF16(logFile);
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

        bool IsWhiteSpace(char c)
        {
            return c == ' ' || c == '\t';
        }

        // Gets the next token found in the arguments string, starting the search on the given position.
        // If there are no more tokens, return empty.
        // After finding the token, updates `start` to point to the next place we need to start the next token search.
        std::string_view GetNextToken(std::string_view arguments, size_t& start)
        {
            // Eat leading whitespace
            while (start < arguments.size() && IsWhiteSpace(arguments[start]))
            {
                ++start;
            }

            if (start >= arguments.size())
            {
                // We reached the end
                return {};
            }

            size_t pos = start;
            bool seekingSpaceSeparator = ('"' != arguments[pos]);
            bool withinQuotes = false;

            // Start looking from the next character
            ++pos;

            // Advance until we hit the end or the next separator
            while (pos < arguments.size())
            {
                bool isSpace = IsWhiteSpace(arguments[pos]);
                bool isQuote = ('"' == arguments[pos]);

                if (isSpace || isQuote)
                {
                    // We've encountered one of the two separators we're interested in
                    if (seekingSpaceSeparator)
                    {
                        if (isQuote)
                        {
                            // We will ignore space characters enclosed between double quotes
                            withinQuotes = !withinQuotes;
                        }
                        else
                        {
                            // This is a space character. If it is between quotes we ignore it;
                            // otherwise it is a separator.
                            if (!withinQuotes)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (isQuote)
                        {
                            // we've got what we needed, it is OK to stop
                            break;
                        }
                    }
                }

                ++pos;
            }

            if (!seekingSpaceSeparator)
            {
                // We were looking for a terminating " character.
                if (pos < arguments.size())
                {
                    // We move past the " character (it is OK for the end of the line
                    // to act as the matching " character in some cases)
                    ++pos;
                }
            }

            auto result = arguments.substr(start, pos - start);
            start = pos;
            return result;
        }

        // Split the arguments string into tokens. Tokens are delimited by whitespace
        // unless quoted. Each token represents an option (like /q), an argument
        // for an option, or a property.
        std::list<std::string> TokenizeMsiArguments(std::string_view arguments)
        {
            size_t start = 0;
            std::list<std::string> result;
            auto token = GetNextToken(arguments, start);
            while (!token.empty())
            {
                result.emplace_back(token);
                token = GetNextToken(arguments, start);
            }

            return result;
        }

        // Parses a token that represents an argument to an option.
        // If the value is unquoted, returns it as is.
        // If the value is quoted, removes the quotes and replaces escaped characters.
        std::string ParseValue(std::string_view valueToken)
        {
            if (valueToken.empty() || valueToken[0] != '"')
            {
                // Nothing to do for empty or unquoted tokens
                return std::string{ valueToken };
            }

            // Copy the string ignoring the quotes and replacing escaped characters.
            // In quoted tokens, the back quote represents double quotes (` means ")
            // and can be escaped with back slash (\` means `).
            // Note that we accept quoted values with a missing closing quote (the end
            // of string signals the end).
            std::string result;
            for (size_t i = 1; i < valueToken.size(); ++i)
            {
                if (valueToken[i] == '"')
                {
                    // The tokenizer can leave several pairs of quotes in the token
                    // but they are not accepted in this case. We only accept the final
                    // closing quotes.
                    if (i + 1 == valueToken.size())
                    {
                        break;
                    }
                    else
                    {
                        AICLI_LOG(Core, Error, << "Invalid msiexec argument: " << valueToken);
                        THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                    }
                }

                if (i + 1 < valueToken.size() && valueToken[i] == '\\' && valueToken[i + 1] == '`')
                {
                    result += '`';
                    ++i;
                }
                else if (valueToken[i] == '`')
                {
                    result += '"';
                }
                else
                {
                    result += valueToken[i];
                }
            }

            return result;
        }

        // Validates that a token represents a property.
        // This checks that the property has the form PropertyName=Value,
        // with the value optionally quoted.
        bool IsValidPropertyToken(std::string_view token)
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, token.empty());

            if (token[0] != '%' && !IsCharAlphaNumericA(token[0]))
            {
                AICLI_LOG(Core, Error, << "Bad property for msiexec: " << token);
                return false;
            }

            // Find the = separator at the end of the property name
            size_t pos = 0;
            while (pos < token.size() && !IsWhiteSpace(token[pos]) && token[pos] != '=')
            {
                ++pos;
            }

            if (pos == token.size() || token[pos] != '=')
            {
                AICLI_LOG(Core, Error, << "Expected property for call to msiexec, but couldn't find separator: " << token);
                return false;
            }

            // Validate the property value.
            // It should be completely enclosed in quotes, or not contain white space.
            // If quoted, there can be pairs of consecutive quotes that work as escape sequences.
            // We accept empty property values.
            ++pos;
            if (pos == token.size())
            {
                // Empty value
                return true;
            }

            // If quoted, we will only inspect the values between the quotes.
            bool quoted = false;
            size_t end = token.size();
            if (token[pos] == '"')
            {
                ++pos;

                if (pos >= end || token.back() != '"')
                {
                    AICLI_LOG(Core, Error, << "Badly quoted msiexec property: " << token);
                    THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                }

                --end;
                quoted = true;
            }

            while (pos < end)
            {
                if (quoted)
                {
                    // For quoted values, any internal quote must be followed by another one.
                    if (token[pos] == '"')
                    {
                        if (pos + 1 < end && token[pos + 1] == '"')
                        {
                            // Skip the two quotes
                            ++pos;
                        }
                        else
                        {
                            AICLI_LOG(Core, Error, << "Unexpected quotes in msiexec property arg: " << token);
                            THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                        }
                    }
                }
                else
                {
                    // For unquoted values, we only check that there is no whitespace
                    if (IsWhiteSpace(token[pos]))
                    {
                        AICLI_LOG(Core, Error, << "Unexpected space in msiexec property arg: " << token);
                        THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                    }
                }

                ++pos;
            }

            return true;
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
        // that takes an argument, also consumes it. After consuming the token(s),
        // removes it from the list and updates the parsed arguments accordingly.
        void ConsumeNextToken(std::list<std::string>& tokens, MsiParsedArguments& parsedArgs)
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, tokens.empty());

            auto token = std::move(tokens.front());
            tokens.pop_front();
            if (!IsSwitch(token))
            {
                // Token is a property, i.e. NAME=value. Add it to the parsed args.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT, !IsValidPropertyToken(token));
                parsedArgs.Properties += L" " + Utility::ConvertToUTF16(token);
                return;
            }

            // Token is an option.
            if (token.size() <= 1)
            {
                AICLI_LOG(Core, Error, << "Invalid command line argument for msiexec: " << token);
                THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
            }

            char option = token[1];
            auto optionModifier = ParseValue(std::string_view(token).substr(2));

            // Options are case-insensitive
            switch (std::tolower(option))
            {
            case MsiExecQuietOption:
            {
                ParseQuietOption(optionModifier, parsedArgs);
                break;
            }
            case MsiExecLogOption:
            {
                if (tokens.empty())
                {
                    // Log option must be followed by an option argument
                    AICLI_LOG(Core, Error, << "msiexec option " << token << " must be followed by a value");
                    THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
                }

                const auto optionValue = ParseValue(tokens.front());
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
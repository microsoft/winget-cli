// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompletionData.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Utility::literals;
    using namespace Settings;

    // Completion takes in the following values:
    //  Word :: The token from the command line that is being targeted for completion.
    //          This value may have quotes surrounding it, and will need to be removed in such a case.
    //  CommandLine :: The full command line that contains the word to be completed.
    //                 This value has the fully quoted strings, as well as escaped quotations if needed.
    //  Position :: The position of the cursor within the command line.
    //
    // Completions here will not attempt to take exact cursor position into account; meaning if the cursor
    // is in the middle of the word, it is not different than at the beginning or end. This functionality
    // could be added later.
    CompletionData::CompletionData(std::string_view word, std::string_view commandLine, std::string_view position)
    {
        m_word = word;

        AICLI_LOG(CLI, Info, << "Completing word '" << m_word << '\'');

        // Determine position as an integer
        size_t cursor = wil::safe_cast<size_t>(std::stoull(std::string{ position }));

        AICLI_LOG(CLI, Info, << "Cursor position starts at '" << cursor << '\'');

        // First, move the cursor from the UTF-8 grapheme position to the UTF-8 byte position.
        // This simplifies the rest of the code.
        cursor = Utility::UTF8Substring(commandLine, 0, cursor).length();

        AICLI_LOG(CLI, Info, << "Cursor position moved to '" << cursor << '\'');

        std::vector<std::string> argsBeforeWord;
        std::vector<std::string> argsAfterWord;

        // If the word is empty, we must determine where the split is. We operate as PowerShell does; the cursor
        // being at the front of a token results in an empty word and an insertion rather than a replacement.
        // If the user put spaces at the front of the statement, this can lead to the position being out of sorts;
        // PowerShell sends the cursor position, but does not include leading spaces in the AST output. If the
        // user puts too many spaces at the front we will be unable to determine the true location.
        if (m_word.empty())
        {
            // The cursor is past the end, so everything is before the word.
            if (cursor >= commandLine.length())
            {
                // Move the position to the end in case it was extended past it.
                ParseInto(commandLine, argsBeforeWord, true);
            }
            // The cursor is not past the end; ensure that the preceding character is whitespace or move the
            // position back until it is. This is far from foolproof, but until we have evidence otherwise,
            // very few users are likely to put any spaces at the front of their statements, let alone many.
            else
            {
                for (; cursor > 0 && !std::isspace(commandLine[cursor - 1]); --cursor);

                AICLI_LOG(CLI, Info, << "Cursor position moved to '" << cursor << '\'');

                // If we actually hit the front of the string, something bad probably happened.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD, cursor == 0);

                ParseInto(commandLine.substr(0, cursor), argsBeforeWord, true);
                ParseInto(commandLine.substr(cursor), argsAfterWord, false);
            }
        }
        // If the word is not empty, the cursor is either in the middle of a token, or at the end of one.
        // The value will be replaced, and we will remove it from the args here.
        else
        {
            std::vector<std::string> allArgs;
            ParseInto(commandLine, allArgs, true);

            // Find the word amongst the arguments
            std::vector<size_t> wordIndeces;
            for (size_t i = 0; i < allArgs.size(); ++i)
            {
                if (m_word == allArgs[i])
                {
                    wordIndeces.push_back(i);
                }
            }

            // If we didn't find a matching string, we probably made some bad assumptions.
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD, wordIndeces.empty());

            // If we find an exact match only once, we can just split on that.
            size_t wordIndexForSplit = wordIndeces[0];

            // If we found more than one match, we have to rely on the position to
            // determine which argument is the word in question.
            if (wordIndeces.size() > 1)
            {
                // Escape the word and search for it in the command line.
                std::string escapedWord = m_word;
                Utility::FindAndReplace(escapedWord, "\"", "\"\"");

                std::vector<size_t> escapedIndeces;
                for (size_t offset = 0; offset < commandLine.length();)
                {
                    size_t pos = commandLine.find(escapedWord, offset);

                    if (pos == std::string::npos)
                    {
                        break;
                    }

                    escapedIndeces.push_back(pos);
                    offset = pos + escapedWord.length();
                }

                // If these are out of sync we don't have much hope.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD, wordIndeces.size() != escapedIndeces.size());

                // Find the closest one to the position. This can be fooled as above if there is
                // leading whitespace in the statement. But it is the best we can do.
                size_t indexToUse = std::numeric_limits<size_t>::max();
                size_t distanceToCursor = std::numeric_limits<size_t>::max();

                for (size_t i = 0; i < escapedIndeces.size(); ++i)
                {
                    size_t lowerBound = escapedIndeces[i];
                    size_t upperBound = lowerBound + escapedWord.length();
                    size_t distance = 0;

                    // The cursor is square in the middle of this location, this is the one.
                    if (cursor > lowerBound && cursor <= upperBound)
                    {
                        indexToUse = i;
                        break;
                    }
                    else if (cursor <= lowerBound)
                    {
                        distance = lowerBound - cursor;
                    }
                    else // cursor > upperBound
                    {
                        distance = cursor - upperBound;
                    }

                    if (distance < distanceToCursor)
                    {
                        indexToUse = i;
                        distanceToCursor = distance;
                    }
                }

                // It really would be unexpected to not find a closest one.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD, indexToUse == std::numeric_limits<size_t>::max());

                wordIndexForSplit = wordIndeces[indexToUse];
            }

            std::vector<std::string>* moveTarget = &argsBeforeWord;
            for (size_t i = 0; i < allArgs.size(); ++i)
            {
                if (i == wordIndexForSplit)
                {
                    // Intentionally leave the matched arg behind.
                    moveTarget = &argsAfterWord;
                }
                else
                {
                    moveTarget->emplace_back(std::move(allArgs[i]));
                }
            }
        }

        // Move the arguments into an Invocation for future use.
        m_argsBeforeWord = std::make_unique<CLI::Invocation>(std::move(argsBeforeWord));
        m_argsAfterWord = std::make_unique<CLI::Invocation>(std::move(argsAfterWord));

        AICLI_LOG(CLI, Info, << "Completion invoked for arguments:" << [&]() {
            std::stringstream strstr;
            for (const auto& arg : *m_argsBeforeWord)
            {
                strstr << " '" << arg << '\'';
            }
            if (m_word.empty())
            {
                strstr << " << [insert] >> ";
            }
            else
            {
                strstr << " << [replace] '" << m_word << "' >> ";
            }
            for (const auto& arg : *m_argsAfterWord)
            {
                strstr << " '" << arg << '\'';
            }
            return strstr.str();
            }());
    }

    void CompletionData::ParseInto(std::string_view line, std::vector<std::string>& args, bool skipFirst)
    {
        std::wstring commandLineW = Utility::ConvertToUTF16(line);
        int argc = 0;
        wil::unique_hlocal_ptr<LPWSTR> argv{ CommandLineToArgvW(commandLineW.c_str(), &argc) };
        THROW_LAST_ERROR_IF_NULL(argv);

        for (int i = (skipFirst ? 1 : 0); i < argc; ++i)
        {
            args.emplace_back(Utility::ConvertToUTF8(argv.get()[i]));
        }
    }
}

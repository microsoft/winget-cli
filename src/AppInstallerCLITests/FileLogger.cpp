// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerFileLogger.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Logging;
using namespace AppInstaller::Utility;
using namespace TestCommon;


std::string GetHeaderString()
{
    return "TIME [CHAN] Header Message";
}

std::string GetLargeString()
{
    return R"([===|Clearly defined start to large string|===]
While this string does not need to be particularly unique, it is still good if it is not easily duplicated by any other random set of data.
It should also end in a character that is not used in any other way within these tests, so please don't include that character when writing tests.
That character is &)";
}

namespace
{
#define WINGET_DEFINE_STRING_ENUM(_enum_,_value_) constexpr std::string_view _enum_##_##_value_ = #_value_##sv

    WINGET_DEFINE_STRING_ENUM(TagState, Unset);
    WINGET_DEFINE_STRING_ENUM(TagState, SetAtStart);
    WINGET_DEFINE_STRING_ENUM(TagState, SetAfterLogging);

    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, Zero);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, SmallerThanLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, EqualToLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, SlightlyLargerThanLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, MuchLargerThanLargeString);

    constexpr std::string_view WrapIndicator = "--- log file has wrapped ---"sv;

    constexpr size_t NewLineCharacterCount = 1;
    constexpr size_t SmallDifferenceSize = 10;
    constexpr AppInstaller::Logging::Channel DefaultChannel = AppInstaller::Logging::Channel::Core;
    constexpr AppInstaller::Logging::Level DefaultLevel = AppInstaller::Logging::Level::Info;

    void ValidateFileContents(const std::filesystem::path& file, const std::vector<std::string_view>& expectedContents)
    {
        std::ifstream fileStream{ file };
        auto fileContents = ReadEntireStream(fileStream);
        std::string_view fileContentsView = fileContents;

        std::string fileContentsCopy = fileContents;
        FindAndReplace(fileContentsCopy, "\r", "\\r");
        FindAndReplace(fileContentsCopy, "\n", "\\n");
        INFO("File contents:\n" << fileContentsCopy);

        size_t currentPosition = 0;
        for (std::string_view expectedContent : expectedContents)
        {
            REQUIRE(currentPosition < fileContents.size());

            if (expectedContent == WrapIndicator)
            {
                auto endLinePosition = fileContentsView.find('\n', currentPosition);
                REQUIRE(endLinePosition != -1);
                REQUIRE(endLinePosition >= expectedContent.size() + NewLineCharacterCount);
                auto actualContent = fileContentsView.substr(endLinePosition + 1 - expectedContent.size() - NewLineCharacterCount, expectedContent.size());
                REQUIRE(expectedContent == actualContent);
                currentPosition = endLinePosition + 1;
            }
            else
            {
                auto actualContent = fileContentsView.substr(currentPosition, expectedContent.size());
                REQUIRE(expectedContent == actualContent);
                currentPosition += expectedContent.size() + NewLineCharacterCount;
            }
        }
    }
}

TEST_CASE("FileLogger_MaximumSize", "[logging]")
{
    auto headerString = GetHeaderString();
    auto largeString = GetLargeString();
    auto tagState = GENERATE(TagState_Unset, TagState_SetAtStart, TagState_SetAfterLogging);
    auto sizeState = GENERATE(MaximumSizeState_Zero, MaximumSizeState_SmallerThanLargeString, MaximumSizeState_EqualToLargeString, MaximumSizeState_SlightlyLargerThanLargeString, MaximumSizeState_MuchLargerThanLargeString);

    // Determine maximum size
    size_t maximumSize = 0;

    if (sizeState == MaximumSizeState_SmallerThanLargeString)
    {
        maximumSize = largeString.size() - SmallDifferenceSize;
    }
    else if (sizeState == MaximumSizeState_EqualToLargeString)
    {
        maximumSize = largeString.size();
    }
    else if (sizeState == MaximumSizeState_SlightlyLargerThanLargeString)
    {
        maximumSize = largeString.size() + SmallDifferenceSize;
    }
    else if (sizeState == MaximumSizeState_MuchLargerThanLargeString)
    {
        maximumSize = largeString.size() * 2;
    }

    INFO("Tag State: " << tagState << ", Size State: " << sizeState << "[" << maximumSize << "]");

    TempFile tempFile{ "FileLogger_MaximumSize", ".log" };
    FileLogger logger{ tempFile };

    INFO("File: " << tempFile.GetPath().u8string());

    logger.SetMaximumSize(wil::safe_cast<std::ofstream::off_type>(maximumSize));

    // Set tag and log strings
    size_t tagPosition = 0;
    if (tagState == TagState_SetAtStart)
    {
        logger.SetTag(Tag::HeadersComplete);
    }

    logger.WriteDirect(DefaultChannel, DefaultLevel, headerString);

    if (tagState == TagState_SetAfterLogging)
    {
        logger.SetTag(Tag::HeadersComplete);
        tagPosition = headerString.size() + NewLineCharacterCount;
    }

    logger.WriteDirect(DefaultChannel, DefaultLevel, largeString);

    size_t maximumAvailableSpace = std::numeric_limits<size_t>::max();
    if (maximumSize)
    {
        maximumAvailableSpace = maximumSize - tagPosition;
    }

    // Calculate current state
    size_t currentAvailableSpace = maximumAvailableSpace - headerString.size() - NewLineCharacterCount;
    bool shouldWrap = largeString.size() > currentAvailableSpace;

    INFO("Maximum Avilable: " << maximumAvailableSpace << ", Current Available: " << currentAvailableSpace << ", ShouldWrap: " << shouldWrap);

    std::vector<std::string_view> expectedFileContents;

    if (tagPosition || !shouldWrap)
    {
        expectedFileContents.push_back(headerString);
    }

    if (shouldWrap)
    {
        expectedFileContents.push_back(WrapIndicator);
    }

    std::string_view largeStringView = largeString;
    expectedFileContents.push_back(largeStringView.substr(0, std::min(largeString.size(), maximumAvailableSpace)));

    ValidateFileContents(tempFile, expectedFileContents);

    // Log again
    INFO("Second time logging large string");
    logger.WriteDirect(DefaultChannel, DefaultLevel, largeString);

    // The maximum size is twice the large log, so anything with a limit will wrap
    shouldWrap = maximumSize != 0;

    expectedFileContents.clear();

    if (tagPosition || !shouldWrap)
    {
        expectedFileContents.push_back(headerString);
    }

    if (shouldWrap)
    {
        expectedFileContents.push_back(WrapIndicator);
    }
    else
    {
        expectedFileContents.push_back(largeStringView);
    }

    expectedFileContents.push_back(largeStringView.substr(0, std::min(largeString.size(), maximumAvailableSpace)));

    ValidateFileContents(tempFile, expectedFileContents);
}

// TODO: Test case where we make wrapping happen a very large number of times

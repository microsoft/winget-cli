// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <ChannelStreams.h>
#include <ExecutionReporter.h>
#include <TableOutput.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Utility;

namespace
{
    Resource::LocString MakeHeader(std::string text)
    {
        return Resource::LocString{ LocIndString{ std::move(text) } };
    }
}

// Test that all rows are buffered and column widths account for values beyond the first 50 rows.
// In the old sizing-buffer design, a row at position 55 with a longer value than any of the
// first 50 rows would be truncated. The new design buffers every row so no value is clipped.
TEST_CASE("TableOutput_AllRowsBuffered_NoTruncation", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;
    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });

    // 54 rows with a short first-column value
    for (int i = 0; i < 54; ++i)
    {
        table.OutputLine({ "ShortValue", "id" + std::to_string(i) });
    }

    // Row 55: first column is much longer than any previous row
    std::string longValue(50, 'A');
    table.OutputLine({ longValue, "id54" });

    table.Complete();

    std::string result = output.str();

    // The full long value must appear; the ellipsis character (U+2026, UTF-8: E2 80 A6)
    // must not appear anywhere, confirming no truncation occurred.
    REQUIRE(result.find(longValue) != std::string::npos);
    REQUIRE(result.find("\xE2\x80\xA6") == std::string::npos);
}

// Test that every data row has its second column starting at the same horizontal position,
// i.e., column 1 is consistently padded regardless of the individual value lengths.
TEST_CASE("TableOutput_ColumnsAligned", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;
    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });

    table.OutputLine({ "Short",          "id.short" });
    table.OutputLine({ "MediumValue",    "id.medium" });
    table.OutputLine({ "LongerValueHere","id.longer" });

    table.Complete();

    std::string result = output.str();
    std::istringstream ss(result);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ss, line))
    {
        if (!line.empty()) lines.push_back(line);
    }

    // Expect: header, separator, 3 data rows = 5 lines minimum
    REQUIRE(lines.size() >= 5);

    // The "id." prefix of the second-column value should start at the same position in every data row.
    size_t pos0 = lines[2].find("id.");
    size_t pos1 = lines[3].find("id.");
    size_t pos2 = lines[4].find("id.");

    REQUIRE(pos0 != std::string::npos);
    REQUIRE(pos0 == pos1);
    REQUIRE(pos0 == pos2);
}

// Test that calling Complete() on an empty table produces no output.
TEST_CASE("TableOutput_Empty_ProducesNoOutput", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;
    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });

    REQUIRE(table.IsEmpty());
    table.Complete();
    REQUIRE(output.str().empty());
}

// Test that GetConsoleWidth does not throw and returns a sensible result.
// In test runner contexts (CI, test explorer) stdout is typically redirected,
// so nullopt is the expected value; when run interactively nullopt or a positive
// width are both valid.
TEST_CASE("GetConsoleWidth_DoesNotCrash", "[channelstreams]")
{
    auto width = GetConsoleWidth();
    if (width.has_value())
    {
        REQUIRE(*width > 0);
    }
    // nullopt is also valid when stdout is not attached to a console
}

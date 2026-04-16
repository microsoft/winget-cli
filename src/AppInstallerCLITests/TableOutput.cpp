// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
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

    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };

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

    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };

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

    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };

    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });

    REQUIRE(table.IsEmpty());
    table.Complete();
    REQUIRE(output.str().empty());
}

// Test that the console width override works and that TableOutput truncates the widest column
// (with ellipsis) when the console is too narrow to fit all columns.
TEST_CASE("TableOutput_ConsoleWidth_TruncatesWhenNarrow", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;

    // Simulate a console that is too narrow for the content.
    // Column 0 max = 20 ("VeryLongPackageName0"), column 1 max = 6 ("pkg.id").
    // SpaceAfter on col 0 = true -> totalRequired = 20 + 1 + 6 = 27.
    // With width = 20, extra = (27 - 20) + 1 = 8, so col 0 shrinks to 12.
    // "VeryLongPackageName0" (20 chars) > 12 -> ellipsis in output.
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{20} };

    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });
    table.OutputLine({ "VeryLongPackageName0", "pkg.id" });
    table.Complete();

    std::string result = output.str();
    REQUIRE(result.find("\xE2\x80\xA6") != std::string::npos); // ellipsis present = truncation occurred
}

// Test that with a wide enough console, values are output in full (no ellipsis).
TEST_CASE("TableOutput_ConsoleWidth_NoTruncationWhenWide", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;

    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{200} };

    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });
    std::string longValue(50, 'A');
    table.OutputLine({ longValue, "pkg.id" });
    table.Complete();

    std::string result = output.str();
    REQUIRE(result.find(longValue) != std::string::npos);
    REQUIRE(result.find("\xE2\x80\xA6") == std::string::npos);
}

// Test that with no-console override (nullopt), content is never truncated regardless of length.
TEST_CASE("TableOutput_NoConsoleOverride_NeverTruncates", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;

    TestHook::SetConsoleWidth_Override widthOverride{ std::nullopt }; // simulate redirected output

    Reporter reporter(output, input);

    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });
    std::string longValue(200, 'B');
    table.OutputLine({ longValue, "pkg.id" });
    table.Complete();

    std::string result = output.str();
    REQUIRE(result.find(longValue) != std::string::npos);
    REQUIRE(result.find("\xE2\x80\xA6") == std::string::npos);
}

// Test that a large number of rows can be buffered and output in a single table without
// any truncation or missing rows.
TEST_CASE("TableOutput_ManyRowsBuffered", "[tableoutput]")
{
    // At the time of creating this test, there were ~12k unique package IDs in the community repository
    constexpr size_t RowCount = 25000;
    std::ostringstream output;
    std::istringstream input;

    TestHook::SetConsoleWidth_Override widthOverride{ std::nullopt }; // no console: no truncation

    Reporter reporter(output, input);

    TableOutput<3> table(reporter, { MakeHeader("Name"), MakeHeader("Id"), MakeHeader("Version") });

    for (size_t i = 0; i < RowCount; ++i)
    {
        table.OutputLine({
            "Package.Name." + std::to_string(i),
            "pkg.id." + std::to_string(i),
            "1.0." + std::to_string(i)
        });
    }

    table.Complete();

    REQUIRE_FALSE(table.IsEmpty());

    std::string result = output.str();

    // Spot-check first, middle, and last rows
    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("pkg.id.0") != std::string::npos);
    REQUIRE(result.find("pkg.id." + std::to_string(RowCount / 2)) != std::string::npos);
    REQUIRE(result.find("pkg.id." + std::to_string(RowCount - 1)) != std::string::npos);

    // Count newlines to verify all rows were written (header + separator + RowCount data rows)
    size_t lineCount = std::count(result.begin(), result.end(), '\n');
    REQUIRE(lineCount == RowCount + 2); // +2 for header and separator lines

    // No truncation ellipsis anywhere
    REQUIRE(result.find("\xE2\x80\xA6") == std::string::npos);
}

// Test that when output is redirected (no console), triggering progress/spinner produces no
// spinner characters in the output stream — only the table content is present.
TEST_CASE("TableOutput_Redirected_NoSpinnerOutput", "[tableoutput]")
{
    std::ostringstream output;
    std::istringstream input;

    TestHook::SetConsoleWidth_Override widthOverride{ std::nullopt }; // simulate redirected output

    Reporter reporter(output, input);

    // Simulate a spinner lifecycle that would emit characters if a spinner were active.
    reporter.BeginProgress();
    reporter.SetProgressMessage("Searching...");
    reporter.EndProgress(true);

    // Now output a table with known content.
    TableOutput<2> table(reporter, { MakeHeader("Name"), MakeHeader("Id") });
    table.OutputLine({ "TestPackage", "test.pkg" });
    table.Complete();

    std::string result = output.str();

    // The table data must be present.
    REQUIRE(result.find("TestPackage") != std::string::npos);
    REQUIRE(result.find("test.pkg") != std::string::npos);

    // Spinner frames written with \r must not appear.
    // The character spinner uses '-', '\', '|', '/' preceded by '\r'.
    REQUIRE(result.find('\r') == std::string::npos);
}
